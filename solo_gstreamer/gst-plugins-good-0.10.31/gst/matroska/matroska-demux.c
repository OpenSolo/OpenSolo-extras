/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2006 Tim-Philipp Müller <tim centricular net>
 * (c) 2008 Sebastian Dröge <slomo@circular-chaos.org>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-demux.c: matroska file/stream demuxer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* TODO: check CRC32 if present
 * TODO: there can be a segment after the first segment. Handle like
 *       chained oggs. Fixes #334082
 * TODO: Test samples: http://www.matroska.org/samples/matrix/index.html
 *                     http://samples.mplayerhq.hu/Matroska/
 * TODO: check if demuxing is done correct for all codecs according to spec
 * TODO: seeking with incomplete or without CUE
 */

/**
 * SECTION:element-matroskademux
 *
 * matroskademux demuxes a Matroska file into the different contained streams.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v filesrc location=/path/to/mkv ! matroskademux ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink
 * ]| This pipeline demuxes a Matroska file and outputs the contained Vorbis audio.
 * </refsect2>
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* FIXME 0.11: suppress warnings for deprecated API such as GStaticRecMutex
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <math.h>
#include <string.h>
#include <glib/gprintf.h>

/* For AVI compatibility mode
   and for fourcc stuff */
#include <gst/riff/riff-read.h>
#include <gst/riff/riff-ids.h>
#include <gst/riff/riff-media.h>

#include <gst/tag/tag.h>

#include <gst/pbutils/pbutils.h>

#include "matroska-demux.h"
#include "matroska-ids.h"

GST_DEBUG_CATEGORY_STATIC (matroskademux_debug);
#define GST_CAT_DEFAULT matroskademux_debug

#define DEBUG_ELEMENT_START(demux, ebml, element) \
    GST_DEBUG_OBJECT (demux, "Parsing " element " element at offset %" \
        G_GUINT64_FORMAT, gst_ebml_read_get_pos (ebml))

#define DEBUG_ELEMENT_STOP(demux, ebml, element, ret) \
    GST_DEBUG_OBJECT (demux, "Parsing " element " element " \
        " finished with '%s'", gst_flow_get_name (ret))

enum
{
  ARG_0,
  ARG_METADATA,
  ARG_STREAMINFO,
  ARG_MAX_GAP_TIME
};

#define  DEFAULT_MAX_GAP_TIME      (2 * GST_SECOND)

static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-matroska; video/webm")
    );

/* TODO: fill in caps! */

static GstStaticPadTemplate audio_src_templ =
GST_STATIC_PAD_TEMPLATE ("audio_%02d",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate video_src_templ =
GST_STATIC_PAD_TEMPLATE ("video_%02d",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate subtitle_src_templ =
    GST_STATIC_PAD_TEMPLATE ("subtitle_%02d",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("text/x-pango-markup; application/x-ssa; "
        "application/x-ass;application/x-usf; video/x-dvd-subpicture; "
        "subpicture/x-pgs; subtitle/x-kate; " "application/x-subtitle-unknown")
    );

static GstFlowReturn gst_matroska_demux_parse_id (GstMatroskaDemux * demux,
    guint32 id, guint64 length, guint needed);

/* element functions */
static void gst_matroska_demux_loop (GstPad * pad);

static gboolean gst_matroska_demux_element_send_event (GstElement * element,
    GstEvent * event);
static gboolean gst_matroska_demux_element_query (GstElement * element,
    GstQuery * query);

/* pad functions */
static gboolean gst_matroska_demux_sink_activate_pull (GstPad * sinkpad,
    gboolean active);
static gboolean gst_matroska_demux_sink_activate (GstPad * sinkpad);

static gboolean gst_matroska_demux_handle_seek_event (GstMatroskaDemux * demux,
    GstPad * pad, GstEvent * event);
static gboolean gst_matroska_demux_handle_src_event (GstPad * pad,
    GstEvent * event);
static const GstQueryType *gst_matroska_demux_get_src_query_types (GstPad *
    pad);
static gboolean gst_matroska_demux_handle_src_query (GstPad * pad,
    GstQuery * query);

static gboolean gst_matroska_demux_handle_sink_event (GstPad * pad,
    GstEvent * event);
static GstFlowReturn gst_matroska_demux_chain (GstPad * pad,
    GstBuffer * buffer);

static GstStateChangeReturn
gst_matroska_demux_change_state (GstElement * element,
    GstStateChange transition);
static void
gst_matroska_demux_set_index (GstElement * element, GstIndex * index);
static GstIndex *gst_matroska_demux_get_index (GstElement * element);

/* caps functions */
static GstCaps *gst_matroska_demux_video_caps (GstMatroskaTrackVideoContext
    * videocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint32 * riff_fourcc);
static GstCaps *gst_matroska_demux_audio_caps (GstMatroskaTrackAudioContext
    * audiocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint16 * riff_audio_fmt);
static GstCaps
    * gst_matroska_demux_subtitle_caps (GstMatroskaTrackSubtitleContext *
    subtitlecontext, const gchar * codec_id, gpointer data, guint size);

/* stream methods */
static void gst_matroska_demux_reset (GstElement * element);
static gboolean perform_seek_to_offset (GstMatroskaDemux * demux,
    guint64 offset);

/* gobject functions */
static void gst_matroska_demux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_matroska_demux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

GType gst_matroska_demux_get_type (void);
GST_BOILERPLATE (GstMatroskaDemux, gst_matroska_demux, GstElement,
    GST_TYPE_ELEMENT);

static void
gst_matroska_demux_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class, &video_src_templ);
  gst_element_class_add_static_pad_template (element_class, &audio_src_templ);
  gst_element_class_add_static_pad_template (element_class,
      &subtitle_src_templ);
  gst_element_class_add_static_pad_template (element_class, &sink_templ);

  gst_element_class_set_details_simple (element_class, "Matroska demuxer",
      "Codec/Demuxer",
      "Demuxes Matroska/WebM streams into video/audio/subtitles",
      "GStreamer maintainers <gstreamer-devel@lists.sourceforge.net>");
}

static void
gst_matroska_demux_finalize (GObject * object)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (object);

  if (demux->common.src) {
    g_ptr_array_free (demux->common.src, TRUE);
    demux->common.src = NULL;
  }

  if (demux->common.global_tags) {
    gst_tag_list_free (demux->common.global_tags);
    demux->common.global_tags = NULL;
  }

  g_object_unref (demux->common.adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_matroska_demux_class_init (GstMatroskaDemuxClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  GST_DEBUG_CATEGORY_INIT (matroskademux_debug, "matroskademux", 0,
      "Matroska demuxer");

  gobject_class->finalize = gst_matroska_demux_finalize;

  gobject_class->get_property = gst_matroska_demux_get_property;
  gobject_class->set_property = gst_matroska_demux_set_property;

  g_object_class_install_property (gobject_class, ARG_MAX_GAP_TIME,
      g_param_spec_uint64 ("max-gap-time", "Maximum gap time",
          "The demuxer sends out newsegment events for skipping "
          "gaps longer than this (0 = disabled).", 0, G_MAXUINT64,
          DEFAULT_MAX_GAP_TIME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_change_state);
  gstelement_class->send_event =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_element_send_event);
  gstelement_class->query =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_element_query);

  gstelement_class->set_index =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_set_index);
  gstelement_class->get_index =
      GST_DEBUG_FUNCPTR (gst_matroska_demux_get_index);
}

static void
gst_matroska_demux_init (GstMatroskaDemux * demux,
    GstMatroskaDemuxClass * klass)
{
  demux->common.sinkpad = gst_pad_new_from_static_template (&sink_templ,
      "sink");
  gst_pad_set_activate_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_sink_activate));
  gst_pad_set_activatepull_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_sink_activate_pull));
  gst_pad_set_chain_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_chain));
  gst_pad_set_event_function (demux->common.sinkpad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_sink_event));
  gst_element_add_pad (GST_ELEMENT (demux), demux->common.sinkpad);

  /* initial stream no. */
  demux->common.src = NULL;

  demux->common.writing_app = NULL;
  demux->common.muxing_app = NULL;
  demux->common.index = NULL;
  demux->common.global_tags = NULL;

  demux->common.adapter = gst_adapter_new ();

  /* property defaults */
  demux->max_gap_time = DEFAULT_MAX_GAP_TIME;

  /* finish off */
  gst_matroska_demux_reset (GST_ELEMENT (demux));
}

static void
gst_matroska_track_free (GstMatroskaTrackContext * track)
{
  g_free (track->codec_id);
  g_free (track->codec_name);
  g_free (track->name);
  g_free (track->language);
  g_free (track->codec_priv);
  g_free (track->codec_state);

  if (track->encodings != NULL) {
    int i;

    for (i = 0; i < track->encodings->len; ++i) {
      GstMatroskaTrackEncoding *enc = &g_array_index (track->encodings,
          GstMatroskaTrackEncoding,
          i);

      g_free (enc->comp_settings);
    }
    g_array_free (track->encodings, TRUE);
  }

  if (track->pending_tags)
    gst_tag_list_free (track->pending_tags);

  if (track->index_table)
    g_array_free (track->index_table, TRUE);

  g_free (track);
}

/*
 * Returns the aggregated GstFlowReturn.
 */
static GstFlowReturn
gst_matroska_demux_combine_flows (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * track, GstFlowReturn ret)
{
  guint i;

  /* store the value */
  track->last_flow = ret;

  /* any other error that is not-linked can be returned right away */
  if (ret != GST_FLOW_NOT_LINKED)
    goto done;

  /* only return NOT_LINKED if all other pads returned NOT_LINKED */
  g_assert (demux->common.src->len == demux->common.num_streams);
  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *ostream = g_ptr_array_index (demux->common.src,
        i);

    if (ostream == NULL)
      continue;

    ret = ostream->last_flow;
    /* some other return value (must be SUCCESS but we can return
     * other values as well) */
    if (ret != GST_FLOW_NOT_LINKED)
      goto done;
  }
  /* if we get here, all other pads were unlinked and we return
   * NOT_LINKED then */
done:
  GST_LOG_OBJECT (demux, "combined return %s", gst_flow_get_name (ret));
  return ret;
}

static void
gst_matroska_demux_free_parsed_el (gpointer mem, gpointer user_data)
{
  g_slice_free (guint64, mem);
}

static void
gst_matroska_demux_reset (GstElement * element)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);
  guint i;

  GST_DEBUG_OBJECT (demux, "Resetting state");

  /* reset input */
  demux->common.state = GST_MATROSKA_READ_STATE_START;

  /* clean up existing streams */
  if (demux->common.src) {
    g_assert (demux->common.src->len == demux->common.num_streams);
    for (i = 0; i < demux->common.src->len; i++) {
      GstMatroskaTrackContext *context = g_ptr_array_index (demux->common.src,
          i);

      if (context->pad != NULL)
        gst_element_remove_pad (GST_ELEMENT (demux), context->pad);

      gst_caps_replace (&context->caps, NULL);
      gst_matroska_track_free (context);
    }
    g_ptr_array_free (demux->common.src, TRUE);
  }
  demux->common.src = g_ptr_array_new ();

  demux->common.num_streams = 0;
  demux->num_a_streams = 0;
  demux->num_t_streams = 0;
  demux->num_v_streams = 0;

  /* reset media info */
  g_free (demux->common.writing_app);
  demux->common.writing_app = NULL;
  g_free (demux->common.muxing_app);
  demux->common.muxing_app = NULL;

  /* reset indexes */
  if (demux->common.index) {
    g_array_free (demux->common.index, TRUE);
    demux->common.index = NULL;
  }

  if (demux->clusters) {
    g_array_free (demux->clusters, TRUE);
    demux->clusters = NULL;
  }

  /* reset timers */
  demux->clock = NULL;
  demux->common.time_scale = 1000000;
  demux->common.created = G_MININT64;

  demux->common.index_parsed = FALSE;
  demux->tracks_parsed = FALSE;
  demux->common.segmentinfo_parsed = FALSE;
  demux->common.attachments_parsed = FALSE;

  g_list_foreach (demux->common.tags_parsed,
      (GFunc) gst_matroska_demux_free_parsed_el, NULL);
  g_list_free (demux->common.tags_parsed);
  demux->common.tags_parsed = NULL;

  g_list_foreach (demux->seek_parsed,
      (GFunc) gst_matroska_demux_free_parsed_el, NULL);
  g_list_free (demux->seek_parsed);
  demux->seek_parsed = NULL;

  gst_segment_init (&demux->common.segment, GST_FORMAT_TIME);
  demux->last_stop_end = GST_CLOCK_TIME_NONE;
  demux->seek_block = 0;
  demux->stream_start_time = GST_CLOCK_TIME_NONE;

  demux->common.offset = 0;
  demux->cluster_time = GST_CLOCK_TIME_NONE;
  demux->cluster_offset = 0;
  demux->next_cluster_offset = 0;
  demux->index_offset = 0;
  demux->seekable = FALSE;
  demux->need_newsegment = FALSE;
  demux->building_index = FALSE;
  if (demux->seek_event) {
    gst_event_unref (demux->seek_event);
    demux->seek_event = NULL;
  }

  demux->seek_index = NULL;
  demux->seek_entry = 0;

  if (demux->close_segment) {
    gst_event_unref (demux->close_segment);
    demux->close_segment = NULL;
  }

  if (demux->new_segment) {
    gst_event_unref (demux->new_segment);
    demux->new_segment = NULL;
  }

  if (demux->common.element_index) {
    gst_object_unref (demux->common.element_index);
    demux->common.element_index = NULL;
  }
  demux->common.element_index_writer_id = -1;

  if (demux->common.global_tags) {
    gst_tag_list_free (demux->common.global_tags);
  }
  demux->common.global_tags = gst_tag_list_new ();

  if (demux->common.cached_buffer) {
    gst_buffer_unref (demux->common.cached_buffer);
    demux->common.cached_buffer = NULL;
  }

  demux->invalid_duration = FALSE;
}

static GstBuffer *
gst_matroska_decode_buffer (GstMatroskaTrackContext * context, GstBuffer * buf)
{
  guint8 *data;
  guint size;
  GstBuffer *new_buf;

  g_return_val_if_fail (GST_IS_BUFFER (buf), NULL);

  GST_DEBUG ("decoding buffer %p", buf);

  data = GST_BUFFER_DATA (buf);
  size = GST_BUFFER_SIZE (buf);

  g_return_val_if_fail (data != NULL && size > 0, buf);

  if (gst_matroska_decode_data (context->encodings, &data, &size,
          GST_MATROSKA_TRACK_ENCODING_SCOPE_FRAME, FALSE)) {
    new_buf = gst_buffer_new ();
    GST_BUFFER_MALLOCDATA (new_buf) = (guint8 *) data;
    GST_BUFFER_DATA (new_buf) = (guint8 *) data;
    GST_BUFFER_SIZE (new_buf) = size;

    gst_buffer_unref (buf);
    buf = new_buf;

    return buf;
  } else {
    GST_DEBUG ("decode data failed");
    gst_buffer_unref (buf);
    return NULL;
  }
}

static GstFlowReturn
gst_matroska_demux_add_stream (GstMatroskaDemux * demux, GstEbmlRead * ebml)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (demux);
  GstMatroskaTrackContext *context;
  GstPadTemplate *templ = NULL;
  GstCaps *caps = NULL;
  gchar *padname = NULL;
  GstFlowReturn ret;
  guint32 id, riff_fourcc = 0;
  guint16 riff_audio_fmt = 0;
  GstTagList *list = NULL;
  gchar *codec = NULL;

  DEBUG_ELEMENT_START (demux, ebml, "TrackEntry");

  /* start with the master */
  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "TrackEntry", ret);
    return ret;
  }

  /* allocate generic... if we know the type, we'll g_renew()
   * with the precise type */
  context = g_new0 (GstMatroskaTrackContext, 1);
  g_ptr_array_add (demux->common.src, context);
  context->index = demux->common.num_streams;
  context->index_writer_id = -1;
  context->type = 0;            /* no type yet */
  context->default_duration = 0;
  context->pos = 0;
  context->set_discont = TRUE;
  context->timecodescale = 1.0;
  context->flags =
      GST_MATROSKA_TRACK_ENABLED | GST_MATROSKA_TRACK_DEFAULT |
      GST_MATROSKA_TRACK_LACING;
  context->last_flow = GST_FLOW_OK;
  context->to_offset = G_MAXINT64;
  context->alignment = 1;
  demux->common.num_streams++;
  g_assert (demux->common.src->len == demux->common.num_streams);

  GST_DEBUG_OBJECT (demux, "Stream number %d", context->index);

  /* try reading the trackentry headers */
  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* track number (unique stream ID) */
      case GST_MATROSKA_ID_TRACKNUMBER:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          GST_ERROR_OBJECT (demux, "Invalid TrackNumber 0");
          ret = GST_FLOW_ERROR;
          break;
        } else if (!gst_matroska_read_common_tracknumber_unique (&demux->common,
                num)) {
          GST_ERROR_OBJECT (demux, "TrackNumber %" G_GUINT64_FORMAT
              " is not unique", num);
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackNumber: %" G_GUINT64_FORMAT, num);
        context->num = num;
        break;
      }
        /* track UID (unique identifier) */
      case GST_MATROSKA_ID_TRACKUID:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          GST_ERROR_OBJECT (demux, "Invalid TrackUID 0");
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackUID: %" G_GUINT64_FORMAT, num);
        context->uid = num;
        break;
      }

        /* track type (video, audio, combined, subtitle, etc.) */
      case GST_MATROSKA_ID_TRACKTYPE:{
        guint64 track_type;

        if ((ret = gst_ebml_read_uint (ebml, &id, &track_type)) != GST_FLOW_OK) {
          break;
        }

        if (context->type != 0 && context->type != track_type) {
          GST_WARNING_OBJECT (demux,
              "More than one tracktype defined in a TrackEntry - skipping");
          break;
        } else if (track_type < 1 || track_type > 254) {
          GST_WARNING_OBJECT (demux, "Invalid TrackType %" G_GUINT64_FORMAT,
              track_type);
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackType: %" G_GUINT64_FORMAT, track_type);

        /* ok, so we're actually going to reallocate this thing */
        switch (track_type) {
          case GST_MATROSKA_TRACK_TYPE_VIDEO:
            gst_matroska_track_init_video_context (&context);
            break;
          case GST_MATROSKA_TRACK_TYPE_AUDIO:
            gst_matroska_track_init_audio_context (&context);
            break;
          case GST_MATROSKA_TRACK_TYPE_SUBTITLE:
            gst_matroska_track_init_subtitle_context (&context);
            break;
          case GST_MATROSKA_TRACK_TYPE_COMPLEX:
          case GST_MATROSKA_TRACK_TYPE_LOGO:
          case GST_MATROSKA_TRACK_TYPE_BUTTONS:
          case GST_MATROSKA_TRACK_TYPE_CONTROL:
          default:
            GST_WARNING_OBJECT (demux,
                "Unknown or unsupported TrackType %" G_GUINT64_FORMAT,
                track_type);
            context->type = 0;
            break;
        }
        g_ptr_array_index (demux->common.src, demux->common.num_streams - 1)
            = context;
        break;
      }

        /* tracktype specific stuff for video */
      case GST_MATROSKA_ID_TRACKVIDEO:{
        GstMatroskaTrackVideoContext *videocontext;

        DEBUG_ELEMENT_START (demux, ebml, "TrackVideo");

        if (!gst_matroska_track_init_video_context (&context)) {
          GST_WARNING_OBJECT (demux,
              "TrackVideo element in non-video track - ignoring track");
          ret = GST_FLOW_ERROR;
          break;
        } else if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
          break;
        }
        videocontext = (GstMatroskaTrackVideoContext *) context;
        g_ptr_array_index (demux->common.src, demux->common.num_streams - 1)
            = context;

        while (ret == GST_FLOW_OK &&
            gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
          if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
            break;

          switch (id) {
              /* Should be one level up but some broken muxers write it here. */
            case GST_MATROSKA_ID_TRACKDEFAULTDURATION:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackDefaultDuration 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackDefaultDuration: %" G_GUINT64_FORMAT, num);
              context->default_duration = num;
              break;
            }

              /* video framerate */
              /* NOTE: This one is here only for backward compatibility.
               * Use _TRACKDEFAULDURATION one level up. */
            case GST_MATROSKA_ID_VIDEOFRAMERATE:{
              gdouble num;

              if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num <= 0.0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoFPS %lf", num);
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackVideoFrameRate: %lf", num);
              if (context->default_duration == 0)
                context->default_duration =
                    gst_gdouble_to_guint64 ((gdouble) GST_SECOND * (1.0 / num));
              videocontext->default_fps = num;
              break;
            }

              /* width of the size to display the video at */
            case GST_MATROSKA_ID_VIDEODISPLAYWIDTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoDisplayWidth 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoDisplayWidth: %" G_GUINT64_FORMAT, num);
              videocontext->display_width = num;
              break;
            }

              /* height of the size to display the video at */
            case GST_MATROSKA_ID_VIDEODISPLAYHEIGHT:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoDisplayHeight 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoDisplayHeight: %" G_GUINT64_FORMAT, num);
              videocontext->display_height = num;
              break;
            }

              /* width of the video in the file */
            case GST_MATROSKA_ID_VIDEOPIXELWIDTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoPixelWidth 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoPixelWidth: %" G_GUINT64_FORMAT, num);
              videocontext->pixel_width = num;
              break;
            }

              /* height of the video in the file */
            case GST_MATROSKA_ID_VIDEOPIXELHEIGHT:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackVideoPixelHeight 0");
                break;
              }

              GST_DEBUG_OBJECT (demux,
                  "TrackVideoPixelHeight: %" G_GUINT64_FORMAT, num);
              videocontext->pixel_height = num;
              break;
            }

              /* whether the video is interlaced */
            case GST_MATROSKA_ID_VIDEOFLAGINTERLACED:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num)
                context->flags |= GST_MATROSKA_VIDEOTRACK_INTERLACED;
              else
                context->flags &= ~GST_MATROSKA_VIDEOTRACK_INTERLACED;
              GST_DEBUG_OBJECT (demux, "TrackVideoInterlaced: %d",
                  (context->flags & GST_MATROSKA_VIDEOTRACK_INTERLACED) ? 1 :
                  0);
              break;
            }

              /* aspect ratio behaviour */
            case GST_MATROSKA_ID_VIDEOASPECTRATIOTYPE:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num != GST_MATROSKA_ASPECT_RATIO_MODE_FREE &&
                  num != GST_MATROSKA_ASPECT_RATIO_MODE_KEEP &&
                  num != GST_MATROSKA_ASPECT_RATIO_MODE_FIXED) {
                GST_WARNING_OBJECT (demux,
                    "Unknown TrackVideoAspectRatioType 0x%x", (guint) num);
                break;
              }
              GST_DEBUG_OBJECT (demux,
                  "TrackVideoAspectRatioType: %" G_GUINT64_FORMAT, num);
              videocontext->asr_mode = num;
              break;
            }

              /* colourspace (only matters for raw video) fourcc */
            case GST_MATROSKA_ID_VIDEOCOLOURSPACE:{
              guint8 *data;
              guint64 datalen;

              if ((ret =
                      gst_ebml_read_binary (ebml, &id, &data,
                          &datalen)) != GST_FLOW_OK)
                break;

              if (datalen != 4) {
                g_free (data);
                GST_WARNING_OBJECT (demux,
                    "Invalid TrackVideoColourSpace length %" G_GUINT64_FORMAT,
                    datalen);
                break;
              }

              memcpy (&videocontext->fourcc, data, 4);
              GST_DEBUG_OBJECT (demux,
                  "TrackVideoColourSpace: %" GST_FOURCC_FORMAT,
                  GST_FOURCC_ARGS (videocontext->fourcc));
              g_free (data);
              break;
            }

            default:
              GST_WARNING_OBJECT (demux,
                  "Unknown TrackVideo subelement 0x%x - ignoring", id);
              /* fall through */
            case GST_MATROSKA_ID_VIDEOSTEREOMODE:
            case GST_MATROSKA_ID_VIDEODISPLAYUNIT:
            case GST_MATROSKA_ID_VIDEOPIXELCROPBOTTOM:
            case GST_MATROSKA_ID_VIDEOPIXELCROPTOP:
            case GST_MATROSKA_ID_VIDEOPIXELCROPLEFT:
            case GST_MATROSKA_ID_VIDEOPIXELCROPRIGHT:
            case GST_MATROSKA_ID_VIDEOGAMMAVALUE:
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }

        DEBUG_ELEMENT_STOP (demux, ebml, "TrackVideo", ret);
        break;
      }

        /* tracktype specific stuff for audio */
      case GST_MATROSKA_ID_TRACKAUDIO:{
        GstMatroskaTrackAudioContext *audiocontext;

        DEBUG_ELEMENT_START (demux, ebml, "TrackAudio");

        if (!gst_matroska_track_init_audio_context (&context)) {
          GST_WARNING_OBJECT (demux,
              "TrackAudio element in non-audio track - ignoring track");
          ret = GST_FLOW_ERROR;
          break;
        }

        if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
          break;

        audiocontext = (GstMatroskaTrackAudioContext *) context;
        g_ptr_array_index (demux->common.src, demux->common.num_streams - 1)
            = context;

        while (ret == GST_FLOW_OK &&
            gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
          if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
            break;

          switch (id) {
              /* samplerate */
            case GST_MATROSKA_ID_AUDIOSAMPLINGFREQ:{
              gdouble num;

              if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
                break;


              if (num <= 0.0) {
                GST_WARNING_OBJECT (demux,
                    "Invalid TrackAudioSamplingFrequency %lf", num);
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackAudioSamplingFrequency: %lf", num);
              audiocontext->samplerate = num;
              break;
            }

              /* bitdepth */
            case GST_MATROSKA_ID_AUDIOBITDEPTH:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackAudioBitDepth 0");
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackAudioBitDepth: %" G_GUINT64_FORMAT,
                  num);
              audiocontext->bitdepth = num;
              break;
            }

              /* channels */
            case GST_MATROSKA_ID_AUDIOCHANNELS:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
                break;

              if (num == 0) {
                GST_WARNING_OBJECT (demux, "Invalid TrackAudioChannels 0");
                break;
              }

              GST_DEBUG_OBJECT (demux, "TrackAudioChannels: %" G_GUINT64_FORMAT,
                  num);
              audiocontext->channels = num;
              break;
            }

            default:
              GST_WARNING_OBJECT (demux,
                  "Unknown TrackAudio subelement 0x%x - ignoring", id);
              /* fall through */
            case GST_MATROSKA_ID_AUDIOCHANNELPOSITIONS:
            case GST_MATROSKA_ID_AUDIOOUTPUTSAMPLINGFREQ:
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }

        DEBUG_ELEMENT_STOP (demux, ebml, "TrackAudio", ret);

        break;
      }

        /* codec identifier */
      case GST_MATROSKA_ID_CODECID:{
        gchar *text;

        if ((ret = gst_ebml_read_ascii (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (demux, "CodecID: %s", GST_STR_NULL (text));
        context->codec_id = text;
        break;
      }

        /* codec private data */
      case GST_MATROSKA_ID_CODECPRIVATE:{
        guint8 *data;
        guint64 size;

        if ((ret =
                gst_ebml_read_binary (ebml, &id, &data, &size)) != GST_FLOW_OK)
          break;

        context->codec_priv = data;
        context->codec_priv_size = size;

        GST_DEBUG_OBJECT (demux, "CodecPrivate of size %" G_GUINT64_FORMAT,
            size);
        break;
      }

        /* name of the codec */
      case GST_MATROSKA_ID_CODECNAME:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (demux, "CodecName: %s", GST_STR_NULL (text));
        context->codec_name = text;
        break;
      }

        /* name of this track */
      case GST_MATROSKA_ID_TRACKNAME:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        context->name = text;
        GST_DEBUG_OBJECT (demux, "TrackName: %s", GST_STR_NULL (text));
        break;
      }

        /* language (matters for audio/subtitles, mostly) */
      case GST_MATROSKA_ID_TRACKLANGUAGE:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;


        context->language = text;

        /* fre-ca => fre */
        if (strlen (context->language) >= 4 && context->language[3] == '-')
          context->language[3] = '\0';

        GST_DEBUG_OBJECT (demux, "TrackLanguage: %s",
            GST_STR_NULL (context->language));
        break;
      }

        /* whether this is actually used */
      case GST_MATROSKA_ID_TRACKFLAGENABLED:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_ENABLED;
        else
          context->flags &= ~GST_MATROSKA_TRACK_ENABLED;

        GST_DEBUG_OBJECT (demux, "TrackEnabled: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
        break;
      }

        /* whether it's the default for this track type */
      case GST_MATROSKA_ID_TRACKFLAGDEFAULT:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_DEFAULT;
        else
          context->flags &= ~GST_MATROSKA_TRACK_DEFAULT;

        GST_DEBUG_OBJECT (demux, "TrackDefault: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
        break;
      }

        /* whether the track must be used during playback */
      case GST_MATROSKA_ID_TRACKFLAGFORCED:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_FORCED;
        else
          context->flags &= ~GST_MATROSKA_TRACK_FORCED;

        GST_DEBUG_OBJECT (demux, "TrackForced: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
        break;
      }

        /* lacing (like MPEG, where blocks don't end/start on frame
         * boundaries) */
      case GST_MATROSKA_ID_TRACKFLAGLACING:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num)
          context->flags |= GST_MATROSKA_TRACK_LACING;
        else
          context->flags &= ~GST_MATROSKA_TRACK_LACING;

        GST_DEBUG_OBJECT (demux, "TrackLacing: %d",
            (context->flags & GST_MATROSKA_TRACK_ENABLED) ? 1 : 0);
        break;
      }

        /* default length (in time) of one data block in this track */
      case GST_MATROSKA_ID_TRACKDEFAULTDURATION:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;


        if (num == 0) {
          GST_WARNING_OBJECT (demux, "Invalid TrackDefaultDuration 0");
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackDefaultDuration: %" G_GUINT64_FORMAT,
            num);
        context->default_duration = num;
        break;
      }

      case GST_MATROSKA_ID_CONTENTENCODINGS:{
        ret = gst_matroska_read_common_read_track_encodings (&demux->common,
            ebml, context);
        break;
      }

      case GST_MATROSKA_ID_TRACKTIMECODESCALE:{
        gdouble num;

        if ((ret = gst_ebml_read_float (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num <= 0.0) {
          GST_WARNING_OBJECT (demux, "Invalid TrackTimeCodeScale %lf", num);
          break;
        }

        GST_DEBUG_OBJECT (demux, "TrackTimeCodeScale: %lf", num);
        context->timecodescale = num;
        break;
      }

      default:
        GST_WARNING ("Unknown TrackEntry subelement 0x%x - ignoring", id);
        /* pass-through */

        /* we ignore these because they're nothing useful (i.e. crap)
         * or simply not implemented yet. */
      case GST_MATROSKA_ID_TRACKMINCACHE:
      case GST_MATROSKA_ID_TRACKMAXCACHE:
      case GST_MATROSKA_ID_MAXBLOCKADDITIONID:
      case GST_MATROSKA_ID_TRACKATTACHMENTLINK:
      case GST_MATROSKA_ID_TRACKOVERLAY:
      case GST_MATROSKA_ID_TRACKTRANSLATE:
      case GST_MATROSKA_ID_TRACKOFFSET:
      case GST_MATROSKA_ID_CODECSETTINGS:
      case GST_MATROSKA_ID_CODECINFOURL:
      case GST_MATROSKA_ID_CODECDOWNLOADURL:
      case GST_MATROSKA_ID_CODECDECODEALL:
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (demux, ebml, "TrackEntry", ret);

  /* Decode codec private data if necessary */
  if (context->encodings && context->encodings->len > 0 && context->codec_priv
      && context->codec_priv_size > 0) {
    if (!gst_matroska_decode_data (context->encodings,
            &context->codec_priv, &context->codec_priv_size,
            GST_MATROSKA_TRACK_ENCODING_SCOPE_CODEC_DATA, TRUE)) {
      GST_WARNING_OBJECT (demux, "Decoding codec private data failed");
      ret = GST_FLOW_ERROR;
    }
  }

  if (context->type == 0 || context->codec_id == NULL || (ret != GST_FLOW_OK
          && ret != GST_FLOW_UNEXPECTED)) {
    if (ret == GST_FLOW_OK || ret == GST_FLOW_UNEXPECTED)
      GST_WARNING_OBJECT (ebml, "Unknown stream/codec in track entry header");

    demux->common.num_streams--;
    g_ptr_array_remove_index (demux->common.src, demux->common.num_streams);
    g_assert (demux->common.src->len == demux->common.num_streams);
    if (context) {
      gst_matroska_track_free (context);
    }

    return ret;
  }

  /* now create the GStreamer connectivity */
  switch (context->type) {
    case GST_MATROSKA_TRACK_TYPE_VIDEO:{
      GstMatroskaTrackVideoContext *videocontext =
          (GstMatroskaTrackVideoContext *) context;

      padname = g_strdup_printf ("video_%02d", demux->num_v_streams++);
      templ = gst_element_class_get_pad_template (klass, "video_%02d");
      caps = gst_matroska_demux_video_caps (videocontext,
          context->codec_id, (guint8 *) context->codec_priv,
          context->codec_priv_size, &codec, &riff_fourcc);

      if (codec) {
        list = gst_tag_list_new ();
        gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
            GST_TAG_VIDEO_CODEC, codec, NULL);
        g_free (codec);
      }
      break;
    }

    case GST_MATROSKA_TRACK_TYPE_AUDIO:{
      GstMatroskaTrackAudioContext *audiocontext =
          (GstMatroskaTrackAudioContext *) context;

      padname = g_strdup_printf ("audio_%02d", demux->num_a_streams++);
      templ = gst_element_class_get_pad_template (klass, "audio_%02d");
      caps = gst_matroska_demux_audio_caps (audiocontext,
          context->codec_id, context->codec_priv, context->codec_priv_size,
          &codec, &riff_audio_fmt);

      if (codec) {
        list = gst_tag_list_new ();
        gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
            GST_TAG_AUDIO_CODEC, codec, NULL);
        g_free (codec);
      }
      break;
    }

    case GST_MATROSKA_TRACK_TYPE_SUBTITLE:{
      GstMatroskaTrackSubtitleContext *subtitlecontext =
          (GstMatroskaTrackSubtitleContext *) context;

      padname = g_strdup_printf ("subtitle_%02d", demux->num_t_streams++);
      templ = gst_element_class_get_pad_template (klass, "subtitle_%02d");
      caps = gst_matroska_demux_subtitle_caps (subtitlecontext,
          context->codec_id, context->codec_priv, context->codec_priv_size);
      break;
    }

    case GST_MATROSKA_TRACK_TYPE_COMPLEX:
    case GST_MATROSKA_TRACK_TYPE_LOGO:
    case GST_MATROSKA_TRACK_TYPE_BUTTONS:
    case GST_MATROSKA_TRACK_TYPE_CONTROL:
    default:
      /* we should already have quit by now */
      g_assert_not_reached ();
  }

  if ((context->language == NULL || *context->language == '\0') &&
      (context->type == GST_MATROSKA_TRACK_TYPE_AUDIO ||
          context->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE)) {
    GST_LOG ("stream %d: language=eng (assuming default)", context->index);
    context->language = g_strdup ("eng");
  }

  if (context->language) {
    const gchar *lang;

    if (!list)
      list = gst_tag_list_new ();

    /* Matroska contains ISO 639-2B codes, we want ISO 639-1 */
    lang = gst_tag_get_language_code (context->language);
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
        GST_TAG_LANGUAGE_CODE, (lang) ? lang : context->language, NULL);
  }

  if (caps == NULL) {
    GST_WARNING_OBJECT (demux, "could not determine caps for stream with "
        "codec_id='%s'", context->codec_id);
    switch (context->type) {
      case GST_MATROSKA_TRACK_TYPE_VIDEO:
        caps = gst_caps_new_simple ("video/x-unknown", NULL);
        break;
      case GST_MATROSKA_TRACK_TYPE_AUDIO:
        caps = gst_caps_new_simple ("audio/x-unknown", NULL);
        break;
      case GST_MATROSKA_TRACK_TYPE_SUBTITLE:
        caps = gst_caps_new_simple ("application/x-subtitle-unknown", NULL);
        break;
      case GST_MATROSKA_TRACK_TYPE_COMPLEX:
      default:
        caps = gst_caps_new_simple ("application/x-matroska-unknown", NULL);
        break;
    }
    gst_caps_set_simple (caps, "codec-id", G_TYPE_STRING, context->codec_id,
        NULL);

    /* add any unrecognised riff fourcc / audio format, but after codec-id */
    if (context->type == GST_MATROSKA_TRACK_TYPE_AUDIO && riff_audio_fmt != 0)
      gst_caps_set_simple (caps, "format", G_TYPE_INT, riff_audio_fmt, NULL);
    else if (context->type == GST_MATROSKA_TRACK_TYPE_VIDEO && riff_fourcc != 0)
      gst_caps_set_simple (caps, "fourcc", GST_TYPE_FOURCC, riff_fourcc, NULL);
  }

  /* the pad in here */
  context->pad = gst_pad_new_from_template (templ, padname);
  context->caps = caps;

  gst_pad_set_event_function (context->pad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_src_event));
  gst_pad_set_query_type_function (context->pad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_get_src_query_types));
  gst_pad_set_query_function (context->pad,
      GST_DEBUG_FUNCPTR (gst_matroska_demux_handle_src_query));

  GST_INFO_OBJECT (demux, "Adding pad '%s' with caps %" GST_PTR_FORMAT,
      padname, caps);

  context->pending_tags = list;

  gst_pad_set_element_private (context->pad, context);

  gst_pad_use_fixed_caps (context->pad);
  gst_pad_set_caps (context->pad, context->caps);
  gst_pad_set_active (context->pad, TRUE);
  gst_element_add_pad (GST_ELEMENT (demux), context->pad);

  g_free (padname);

  /* tadaah! */
  return ret;
}

static const GstQueryType *
gst_matroska_demux_get_src_query_types (GstPad * pad)
{
  static const GstQueryType query_types[] = {
    GST_QUERY_POSITION,
    GST_QUERY_DURATION,
    GST_QUERY_SEEKING,
    0
  };

  return query_types;
}

static gboolean
gst_matroska_demux_query (GstMatroskaDemux * demux, GstPad * pad,
    GstQuery * query)
{
  gboolean res = FALSE;
  GstMatroskaTrackContext *context = NULL;

  if (pad) {
    context = gst_pad_get_element_private (pad);
  }

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      GstFormat format;

      gst_query_parse_position (query, &format, NULL);

      if (format == GST_FORMAT_TIME) {
        GST_OBJECT_LOCK (demux);
        if (context)
          gst_query_set_position (query, GST_FORMAT_TIME,
              MAX (context->pos, demux->stream_start_time) -
              demux->stream_start_time);
        else
          gst_query_set_position (query, GST_FORMAT_TIME,
              MAX (demux->common.segment.last_stop, demux->stream_start_time) -
              demux->stream_start_time);
        GST_OBJECT_UNLOCK (demux);
      } else if (format == GST_FORMAT_DEFAULT && context
          && context->default_duration) {
        GST_OBJECT_LOCK (demux);
        gst_query_set_position (query, GST_FORMAT_DEFAULT,
            context->pos / context->default_duration);
        GST_OBJECT_UNLOCK (demux);
      } else {
        GST_DEBUG_OBJECT (demux,
            "only position query in TIME and DEFAULT format is supported");
      }

      res = TRUE;
      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      if (format == GST_FORMAT_TIME) {
        GST_OBJECT_LOCK (demux);
        gst_query_set_duration (query, GST_FORMAT_TIME,
            demux->common.segment.duration);
        GST_OBJECT_UNLOCK (demux);
      } else if (format == GST_FORMAT_DEFAULT && context
          && context->default_duration) {
        GST_OBJECT_LOCK (demux);
        gst_query_set_duration (query, GST_FORMAT_DEFAULT,
            demux->common.segment.duration / context->default_duration);
        GST_OBJECT_UNLOCK (demux);
      } else {
        GST_DEBUG_OBJECT (demux,
            "only duration query in TIME and DEFAULT format is supported");
      }

      res = TRUE;
      break;
    }

    case GST_QUERY_SEEKING:
    {
      GstFormat fmt;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);
      GST_OBJECT_LOCK (demux);
      if (fmt == GST_FORMAT_TIME) {
        gboolean seekable;

        if (demux->streaming) {
          /* assuming we'll be able to get an index ... */
          seekable = demux->seekable;
        } else {
          seekable = TRUE;
        }

        gst_query_set_seeking (query, GST_FORMAT_TIME, seekable,
            0, demux->common.segment.duration);
        res = TRUE;
      }
      GST_OBJECT_UNLOCK (demux);
      break;
    }
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  return res;
}

static gboolean
gst_matroska_demux_element_query (GstElement * element, GstQuery * query)
{
  return gst_matroska_demux_query (GST_MATROSKA_DEMUX (element), NULL, query);
}

static gboolean
gst_matroska_demux_handle_src_query (GstPad * pad, GstQuery * query)
{
  gboolean ret;
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (gst_pad_get_parent (pad));

  ret = gst_matroska_demux_query (demux, pad, query);

  gst_object_unref (demux);

  return ret;
}

/* returns FALSE if there are no pads to deliver event to,
 * otherwise TRUE (whatever the outcome of event sending),
 * takes ownership of the passed event! */
static gboolean
gst_matroska_demux_send_event (GstMatroskaDemux * demux, GstEvent * event)
{
  gboolean is_newsegment;
  gboolean ret = FALSE;
  gint i;

  g_return_val_if_fail (event != NULL, FALSE);

  GST_DEBUG_OBJECT (demux, "Sending event of type %s to all source pads",
      GST_EVENT_TYPE_NAME (event));

  is_newsegment = (GST_EVENT_TYPE (event) == GST_EVENT_NEWSEGMENT);

  g_assert (demux->common.src->len == demux->common.num_streams);
  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream;

    stream = g_ptr_array_index (demux->common.src, i);
    gst_event_ref (event);
    gst_pad_push_event (stream->pad, event);
    ret = TRUE;

    /* FIXME: send global tags before stream tags */
    if (G_UNLIKELY (is_newsegment && stream->pending_tags != NULL)) {
      GST_DEBUG_OBJECT (demux, "Sending pending_tags %p for pad %s:%s : %"
          GST_PTR_FORMAT, stream->pending_tags,
          GST_DEBUG_PAD_NAME (stream->pad), stream->pending_tags);
      gst_element_found_tags_for_pad (GST_ELEMENT (demux), stream->pad,
          stream->pending_tags);
      stream->pending_tags = NULL;
    }
  }

  if (G_UNLIKELY (is_newsegment && demux->common.global_tags != NULL)) {
    gst_tag_list_add (demux->common.global_tags, GST_TAG_MERGE_REPLACE,
        GST_TAG_CONTAINER_FORMAT, "Matroska", NULL);
    GST_DEBUG_OBJECT (demux, "Sending global_tags %p : %" GST_PTR_FORMAT,
        demux->common.global_tags, demux->common.global_tags);
    gst_element_found_tags (GST_ELEMENT (demux), demux->common.global_tags);
    demux->common.global_tags = NULL;
  }

  gst_event_unref (event);
  return ret;
}

static gboolean
gst_matroska_demux_element_send_event (GstElement * element, GstEvent * event)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);
  gboolean res;

  g_return_val_if_fail (event != NULL, FALSE);

  if (GST_EVENT_TYPE (event) == GST_EVENT_SEEK) {
    res = gst_matroska_demux_handle_seek_event (demux, NULL, event);
  } else {
    GST_WARNING_OBJECT (demux, "Unhandled event of type %s",
        GST_EVENT_TYPE_NAME (event));
    res = FALSE;
  }
  gst_event_unref (event);
  return res;
}

static gboolean
gst_matroska_demux_move_to_entry (GstMatroskaDemux * demux,
    GstMatroskaIndex * entry, gboolean reset)
{
  gint i;

  GST_OBJECT_LOCK (demux);

  /* seek (relative to matroska segment) */
  /* position might be invalid; will error when streaming resumes ... */
  demux->common.offset = entry->pos + demux->common.ebml_segment_start;

  GST_DEBUG_OBJECT (demux, "Seeked to offset %" G_GUINT64_FORMAT ", block %d, "
      "time %" GST_TIME_FORMAT, entry->pos + demux->common.ebml_segment_start,
      entry->block, GST_TIME_ARGS (entry->time));

  /* update the time */
  gst_matroska_read_common_reset_streams (&demux->common, entry->time, TRUE);
  demux->common.segment.last_stop = entry->time;
  demux->seek_block = entry->block;
  demux->seek_first = TRUE;
  demux->last_stop_end = GST_CLOCK_TIME_NONE;

  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream = g_ptr_array_index (demux->common.src, i);

    if (reset) {
      stream->to_offset = G_MAXINT64;
    } else {
      if (stream->from_offset != -1)
        stream->to_offset = stream->from_offset;
    }
    stream->from_offset = -1;
  }

  GST_OBJECT_UNLOCK (demux);

  return TRUE;
}

static gint
gst_matroska_cluster_compare (gint64 * i1, gint64 * i2)
{
  if (*i1 < *i2)
    return -1;
  else if (*i1 > *i2)
    return 1;
  else
    return 0;
}

/* searches for a cluster start from @pos,
 * return GST_FLOW_OK and cluster position in @pos if found */
static GstFlowReturn
gst_matroska_demux_search_cluster (GstMatroskaDemux * demux, gint64 * pos)
{
  gint64 newpos = *pos;
  gint64 orig_offset;
  GstFlowReturn ret = GST_FLOW_OK;
  const guint chunk = 64 * 1024;
  GstBuffer *buf = NULL;
  guint64 length;
  guint32 id;
  guint needed;

  orig_offset = demux->common.offset;

  GST_LOG_OBJECT (demux, "searching cluster following offset %" G_GINT64_FORMAT,
      *pos);

  if (demux->clusters) {
    gint64 *cpos;

    cpos = gst_util_array_binary_search (demux->clusters->data,
        demux->clusters->len, sizeof (gint64),
        (GCompareDataFunc) gst_matroska_cluster_compare,
        GST_SEARCH_MODE_AFTER, pos, NULL);
    /* sanity check */
    if (cpos) {
      GST_DEBUG_OBJECT (demux,
          "cluster reported at offset %" G_GINT64_FORMAT, *cpos);
      demux->common.offset = *cpos;
      ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
          GST_ELEMENT_CAST (demux), &id, &length, &needed);
      if (ret == GST_FLOW_OK && id == GST_MATROSKA_ID_CLUSTER) {
        newpos = *cpos;
        goto exit;
      }
    }
  }

  /* read in at newpos and scan for ebml cluster id */
  while (1) {
    GstByteReader reader;
    gint cluster_pos;

    if (buf != NULL) {
      gst_buffer_unref (buf);
      buf = NULL;
    }
    ret = gst_pad_pull_range (demux->common.sinkpad, newpos, chunk, &buf);
    if (ret != GST_FLOW_OK)
      break;
    GST_DEBUG_OBJECT (demux, "read buffer size %d at offset %" G_GINT64_FORMAT,
        GST_BUFFER_SIZE (buf), newpos);
    gst_byte_reader_init_from_buffer (&reader, buf);
  resume:
    cluster_pos = gst_byte_reader_masked_scan_uint32 (&reader, 0xffffffff,
        GST_MATROSKA_ID_CLUSTER, 0, gst_byte_reader_get_remaining (&reader));
    if (cluster_pos >= 0) {
      newpos += cluster_pos;
      /* prepare resuming at next byte */
      gst_byte_reader_skip (&reader, cluster_pos + 1);
      GST_DEBUG_OBJECT (demux,
          "found cluster ebml id at offset %" G_GINT64_FORMAT, newpos);
      /* extra checks whether we really sync'ed to a cluster:
       * - either it is the first and only cluster
       * - either there is a cluster after this one
       * - either cluster length is undefined
       */
      /* ok if first cluster (there may not a subsequent one) */
      if (newpos == demux->first_cluster_offset) {
        GST_DEBUG_OBJECT (demux, "cluster is first cluster -> OK");
        break;
      }
      demux->common.offset = newpos;
      ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
          GST_ELEMENT_CAST (demux), &id, &length, &needed);
      if (ret != GST_FLOW_OK) {
        GST_DEBUG_OBJECT (demux, "need more data -> continue");
        continue;
      }
      g_assert (id == GST_MATROSKA_ID_CLUSTER);
      GST_DEBUG_OBJECT (demux, "cluster size %" G_GUINT64_FORMAT ", prefix %d",
          length, needed);
      /* ok if undefined length or first cluster */
      if (length == GST_EBML_SIZE_UNKNOWN || length == G_MAXUINT64) {
        GST_DEBUG_OBJECT (demux, "cluster has undefined length -> OK");
        break;
      }
      /* skip cluster */
      demux->common.offset += length + needed;
      ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
          GST_ELEMENT_CAST (demux), &id, &length, &needed);
      if (ret != GST_FLOW_OK)
        goto resume;
      GST_DEBUG_OBJECT (demux, "next element is %scluster",
          id == GST_MATROSKA_ID_CLUSTER ? "" : "not ");
      if (id == GST_MATROSKA_ID_CLUSTER)
        break;
      /* not ok, resume */
      goto resume;
    } else {
      /* partial cluster id may have been in tail of buffer */
      newpos += MAX (gst_byte_reader_get_remaining (&reader), 4) - 3;
    }
  }

  if (buf) {
    gst_buffer_unref (buf);
    buf = NULL;
  }

exit:
  demux->common.offset = orig_offset;
  *pos = newpos;
  return ret;
}

/* bisect and scan through file for cluster starting before @time,
 * returns fake index entry with corresponding info on cluster */
static GstMatroskaIndex *
gst_matroska_demux_search_pos (GstMatroskaDemux * demux, GstClockTime time)
{
  GstMatroskaIndex *entry = NULL;
  GstMatroskaReadState current_state;
  GstClockTime otime, prev_cluster_time, current_cluster_time, cluster_time;
  gint64 opos, newpos, startpos = 0, current_offset;
  gint64 prev_cluster_offset = -1, current_cluster_offset, cluster_offset;
  const guint chunk = 64 * 1024;
  GstFlowReturn ret;
  guint64 length;
  guint32 id;
  guint needed;

  /* (under)estimate new position, resync using cluster ebml id,
   * and scan forward to appropriate cluster
   * (and re-estimate if need to go backward) */

  prev_cluster_time = GST_CLOCK_TIME_NONE;

  /* store some current state */
  current_state = demux->common.state;
  g_return_val_if_fail (current_state == GST_MATROSKA_READ_STATE_DATA, NULL);

  current_cluster_offset = demux->cluster_offset;
  current_cluster_time = demux->cluster_time;
  current_offset = demux->common.offset;

  demux->common.state = GST_MATROSKA_READ_STATE_SCANNING;

  /* estimate using start and current position */
  GST_OBJECT_LOCK (demux);
  opos = demux->common.offset - demux->common.ebml_segment_start;
  otime = demux->common.segment.last_stop;
  GST_OBJECT_UNLOCK (demux);

  /* sanitize */
  time = MAX (time, demux->stream_start_time);

  /* avoid division by zero in first estimation below */
  if (otime <= demux->stream_start_time)
    otime = time;

retry:
  GST_LOG_OBJECT (demux,
      "opos: %" G_GUINT64_FORMAT ", otime: %" GST_TIME_FORMAT ", %"
      GST_TIME_FORMAT " in stream time (start %" GST_TIME_FORMAT "), time %"
      GST_TIME_FORMAT, opos, GST_TIME_ARGS (otime),
      GST_TIME_ARGS (otime - demux->stream_start_time),
      GST_TIME_ARGS (demux->stream_start_time), GST_TIME_ARGS (time));
  newpos =
      gst_util_uint64_scale (opos - demux->common.ebml_segment_start,
      time - demux->stream_start_time,
      otime - demux->stream_start_time) - chunk;
  if (newpos < 0)
    newpos = 0;
  /* favour undershoot */
  newpos = newpos * 90 / 100;
  newpos += demux->common.ebml_segment_start;

  GST_DEBUG_OBJECT (demux,
      "estimated offset for %" GST_TIME_FORMAT ": %" G_GINT64_FORMAT,
      GST_TIME_ARGS (time), newpos);

  /* and at least start scanning before previous scan start to avoid looping */
  startpos = startpos * 90 / 100;
  if (startpos && startpos < newpos)
    newpos = startpos;

  /* read in at newpos and scan for ebml cluster id */
  startpos = newpos;
  while (1) {

    ret = gst_matroska_demux_search_cluster (demux, &newpos);
    if (ret == GST_FLOW_UNEXPECTED) {
      /* heuristic HACK */
      newpos = startpos * 80 / 100;
      GST_DEBUG_OBJECT (demux, "EOS; "
          "new estimated offset for %" GST_TIME_FORMAT ": %" G_GINT64_FORMAT,
          GST_TIME_ARGS (time), newpos);
      startpos = newpos;
      continue;
    } else if (ret != GST_FLOW_OK) {
      goto exit;
    } else {
      break;
    }
  }

  /* then start scanning and parsing for cluster time,
   * re-estimate if overshoot, otherwise next cluster and so on */
  demux->common.offset = newpos;
  demux->cluster_time = cluster_time = GST_CLOCK_TIME_NONE;
  while (1) {
    guint64 cluster_size = 0;

    /* peek and parse some elements */
    ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
        GST_ELEMENT_CAST (demux), &id, &length, &needed);
    if (ret != GST_FLOW_OK)
      goto error;
    GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
        "size %" G_GUINT64_FORMAT ", needed %d", demux->common.offset, id,
        length, needed);
    ret = gst_matroska_demux_parse_id (demux, id, length, needed);
    if (ret != GST_FLOW_OK)
      goto error;

    if (id == GST_MATROSKA_ID_CLUSTER) {
      cluster_time = GST_CLOCK_TIME_NONE;
      if (length == G_MAXUINT64)
        cluster_size = 0;
      else
        cluster_size = length + needed;
    }
    if (demux->cluster_time != GST_CLOCK_TIME_NONE &&
        cluster_time == GST_CLOCK_TIME_NONE) {
      cluster_time = demux->cluster_time * demux->common.time_scale;
      cluster_offset = demux->cluster_offset;
      GST_DEBUG_OBJECT (demux, "found cluster at offset %" G_GINT64_FORMAT
          " with time %" GST_TIME_FORMAT, cluster_offset,
          GST_TIME_ARGS (cluster_time));
      if (cluster_time > time) {
        GST_DEBUG_OBJECT (demux, "overshot target");
        /* cluster overshoots */
        if (cluster_offset == demux->first_cluster_offset) {
          /* but no prev one */
          GST_DEBUG_OBJECT (demux, "but using first cluster anyway");
          prev_cluster_time = cluster_time;
          prev_cluster_offset = cluster_offset;
          break;
        }
        if (prev_cluster_time != GST_CLOCK_TIME_NONE) {
          /* prev cluster did not overshoot, so prev cluster is target */
          break;
        } else {
          /* re-estimate using this new position info */
          opos = cluster_offset;
          otime = cluster_time;
          goto retry;
        }
      } else {
        /* cluster undershoots, goto next one */
        prev_cluster_time = cluster_time;
        prev_cluster_offset = cluster_offset;
        /* skip cluster if length is defined,
         * otherwise will be skippingly parsed into */
        if (cluster_size) {
          GST_DEBUG_OBJECT (demux, "skipping to next cluster");
          demux->common.offset = cluster_offset + cluster_size;
          demux->cluster_time = GST_CLOCK_TIME_NONE;
        } else {
          GST_DEBUG_OBJECT (demux, "parsing/skipping cluster elements");
        }
      }
    }
    continue;

  error:
    if (ret == GST_FLOW_UNEXPECTED) {
      if (prev_cluster_time != GST_CLOCK_TIME_NONE)
        break;
    }
    goto exit;
  }

  entry = g_new0 (GstMatroskaIndex, 1);
  entry->time = prev_cluster_time;
  entry->pos = prev_cluster_offset - demux->common.ebml_segment_start;
  GST_DEBUG_OBJECT (demux, "simulated index entry; time %" GST_TIME_FORMAT
      ", pos %" G_GUINT64_FORMAT, GST_TIME_ARGS (entry->time), entry->pos);

exit:

  /* restore some state */
  demux->cluster_offset = current_cluster_offset;
  demux->cluster_time = current_cluster_time;
  demux->common.offset = current_offset;
  demux->common.state = current_state;

  return entry;
}

static gboolean
gst_matroska_demux_handle_seek_event (GstMatroskaDemux * demux,
    GstPad * pad, GstEvent * event)
{
  GstMatroskaIndex *entry = NULL;
  GstMatroskaIndex scan_entry;
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  GstFormat format;
  gboolean flush, keyunit;
  gdouble rate;
  gint64 cur, stop;
  GstMatroskaTrackContext *track = NULL;
  GstSegment seeksegment = { 0, };
  gboolean update = TRUE;

  if (pad)
    track = gst_pad_get_element_private (pad);

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
      &stop_type, &stop);

  /* we can only seek on time */
  if (format != GST_FORMAT_TIME) {
    GST_DEBUG_OBJECT (demux, "Can only seek on TIME");
    return FALSE;
  }

  /* copy segment, we need this because we still need the old
   * segment when we close the current segment. */
  memcpy (&seeksegment, &demux->common.segment, sizeof (GstSegment));

  /* pull mode without index means that the actual duration is not known,
   * we might be playing a file that's still being recorded
   * so, invalidate our current duration, which is only a moving target,
   * and should not be used to clamp anything */
  if (!demux->streaming && !demux->common.index &&
      demux->invalid_duration) {
    gst_segment_set_duration (&seeksegment, GST_FORMAT_TIME,
        GST_CLOCK_TIME_NONE);
  }

  if (event) {
    GST_DEBUG_OBJECT (demux, "configuring seek");
    gst_segment_set_seek (&seeksegment, rate, format, flags,
        cur_type, cur, stop_type, stop, &update);
    /* compensate for clip start time */
    if (GST_CLOCK_TIME_IS_VALID (demux->stream_start_time)) {
      seeksegment.last_stop += demux->stream_start_time;
      seeksegment.start += demux->stream_start_time;
      if (GST_CLOCK_TIME_IS_VALID (seeksegment.stop))
        seeksegment.stop += demux->stream_start_time;
      /* note that time should stay at indicated position */
    }
  }

  /* restore segment duration (if any effect),
   * would be determined again when parsing, but anyway ... */
  gst_segment_set_duration (&seeksegment, GST_FORMAT_TIME,
      demux->common.segment.duration);

  flush = ! !(flags & GST_SEEK_FLAG_FLUSH);
  keyunit = ! !(flags & GST_SEEK_FLAG_KEY_UNIT);

  GST_DEBUG_OBJECT (demux, "New segment %" GST_SEGMENT_FORMAT, &seeksegment);

  if (!update) {
    /* only have to update some segment,
     * but also still have to honour flush and so on */
    GST_DEBUG_OBJECT (demux, "... no update");
    /* bad goto, bad ... */
    goto next;
  }

  /* check sanity before we start flushing and all that */
  GST_OBJECT_LOCK (demux);
  track = gst_matroska_read_common_get_seek_track (&demux->common, track);
  if ((entry = gst_matroska_read_common_do_index_seek (&demux->common, track,
              seeksegment.last_stop, &demux->seek_index, &demux->seek_entry)) ==
      NULL) {
    /* pull mode without index can scan later on */
    if (demux->streaming) {
      GST_DEBUG_OBJECT (demux, "No matching seek entry in index");
      GST_OBJECT_UNLOCK (demux);
      return FALSE;
    }
  }
  GST_DEBUG_OBJECT (demux, "Seek position looks sane");
  GST_OBJECT_UNLOCK (demux);

  if (demux->streaming) {
    /* need to seek to cluster start to pick up cluster time */
    /* upstream takes care of flushing and all that
     * ... and newsegment event handling takes care of the rest */
    return perform_seek_to_offset (demux,
        entry->pos + demux->common.ebml_segment_start);
  }

next:
  if (flush) {
    GST_DEBUG_OBJECT (demux, "Starting flush");
    gst_pad_push_event (demux->common.sinkpad, gst_event_new_flush_start ());
    gst_matroska_demux_send_event (demux, gst_event_new_flush_start ());
  } else {
    GST_DEBUG_OBJECT (demux, "Non-flushing seek, pausing task");
    gst_pad_pause_task (demux->common.sinkpad);
  }
  /* ouch */
  if (!update)
    goto exit;

  /* now grab the stream lock so that streaming cannot continue, for
   * non flushing seeks when the element is in PAUSED this could block
   * forever. */
  GST_DEBUG_OBJECT (demux, "Waiting for streaming to stop");
  GST_PAD_STREAM_LOCK (demux->common.sinkpad);

  /* pull mode without index can do some scanning */
  if (!demux->streaming && !entry) {
    /* need to stop flushing upstream as we need it next */
    if (flush)
      gst_pad_push_event (demux->common.sinkpad, gst_event_new_flush_stop ());
    entry = gst_matroska_demux_search_pos (demux, seeksegment.last_stop);
    /* keep local copy */
    if (entry) {
      scan_entry = *entry;
      g_free (entry);
      entry = &scan_entry;
    } else {
      GST_DEBUG_OBJECT (demux, "Scan failed to find matching position");
      if (flush)
        gst_matroska_demux_send_event (demux, gst_event_new_flush_stop ());
      goto seek_error;
    }
  }

  if (keyunit) {
    GST_DEBUG_OBJECT (demux, "seek to key unit, adjusting segment start to %"
        GST_TIME_FORMAT, GST_TIME_ARGS (entry->time));
    seeksegment.start = MAX (entry->time, demux->stream_start_time);
    seeksegment.last_stop = seeksegment.start;
    seeksegment.time = seeksegment.start - demux->stream_start_time;
  }

exit:
  if (flush) {
    GST_DEBUG_OBJECT (demux, "Stopping flush");
    gst_pad_push_event (demux->common.sinkpad, gst_event_new_flush_stop ());
    gst_matroska_demux_send_event (demux, gst_event_new_flush_stop ());
  } else if (demux->segment_running && update) {
    GST_DEBUG_OBJECT (demux, "Closing currently running segment");

    GST_OBJECT_LOCK (demux);
    if (demux->close_segment)
      gst_event_unref (demux->close_segment);

    demux->close_segment = gst_event_new_new_segment (TRUE,
        demux->common.segment.rate, GST_FORMAT_TIME,
        demux->common.segment.start, demux->common.segment.last_stop,
        demux->common.segment.time);
    GST_OBJECT_UNLOCK (demux);
  }

  GST_OBJECT_LOCK (demux);
  /* now update the real segment info */
  GST_DEBUG_OBJECT (demux, "Committing new seek segment");
  memcpy (&demux->common.segment, &seeksegment, sizeof (GstSegment));
  GST_OBJECT_UNLOCK (demux);

  /* update some (segment) state */
  if (update && !gst_matroska_demux_move_to_entry (demux, entry, TRUE))
    goto seek_error;

  /* notify start of new segment */
  if (demux->common.segment.flags & GST_SEEK_FLAG_SEGMENT) {
    GstMessage *msg;

    msg = gst_message_new_segment_start (GST_OBJECT (demux),
        GST_FORMAT_TIME, demux->common.segment.start);
    gst_element_post_message (GST_ELEMENT (demux), msg);
  }

  GST_OBJECT_LOCK (demux);
  if (demux->new_segment)
    gst_event_unref (demux->new_segment);
  demux->new_segment = gst_event_new_new_segment_full (!update,
      demux->common.segment.rate, demux->common.segment.applied_rate,
      demux->common.segment.format, demux->common.segment.start,
      demux->common.segment.stop, demux->common.segment.time);
  GST_OBJECT_UNLOCK (demux);

  /* restart our task since it might have been stopped when we did the
   * flush. */
  demux->segment_running = TRUE;
  gst_pad_start_task (demux->common.sinkpad,
      (GstTaskFunction) gst_matroska_demux_loop, demux->common.sinkpad);

  /* streaming can continue now */
  GST_PAD_STREAM_UNLOCK (demux->common.sinkpad);

  return TRUE;

seek_error:
  {
    GST_PAD_STREAM_UNLOCK (demux->common.sinkpad);
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("Got a seek error"));
    return FALSE;
  }
}

/*
 * Handle whether we can perform the seek event or if we have to let the chain
 * function handle seeks to build the seek indexes first.
 */
static gboolean
gst_matroska_demux_handle_seek_push (GstMatroskaDemux * demux, GstPad * pad,
    GstEvent * event)
{
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  GstFormat format;
  gdouble rate;
  gint64 cur, stop;

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
      &stop_type, &stop);

  /* sanity checks */

  /* we can only seek on time */
  if (format != GST_FORMAT_TIME) {
    GST_DEBUG_OBJECT (demux, "Can only seek on TIME");
    return FALSE;
  }

  if (stop_type != GST_SEEK_TYPE_NONE && stop != GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (demux, "Seek end-time not supported in streaming mode");
    return FALSE;
  }

  if (!(flags & GST_SEEK_FLAG_FLUSH)) {
    GST_DEBUG_OBJECT (demux,
        "Non-flushing seek not supported in streaming mode");
    return FALSE;
  }

  if (flags & GST_SEEK_FLAG_SEGMENT) {
    GST_DEBUG_OBJECT (demux, "Segment seek not supported in streaming mode");
    return FALSE;
  }

  /* check for having parsed index already */
  if (!demux->common.index_parsed) {
    gboolean building_index;
    guint64 offset = 0;

    if (!demux->index_offset) {
      GST_DEBUG_OBJECT (demux, "no index (location); no seek in push mode");
      return FALSE;
    }

    GST_OBJECT_LOCK (demux);
    /* handle the seek event in the chain function */
    demux->common.state = GST_MATROSKA_READ_STATE_SEEK;
    /* no more seek can be issued until state reset to _DATA */

    /* copy the event */
    if (demux->seek_event)
      gst_event_unref (demux->seek_event);
    demux->seek_event = gst_event_ref (event);

    /* set the building_index flag so that only one thread can setup the
     * structures for index seeking. */
    building_index = demux->building_index;
    if (!building_index) {
      demux->building_index = TRUE;
      offset = demux->index_offset;
    }
    GST_OBJECT_UNLOCK (demux);

    if (!building_index) {
      /* seek to the first subindex or legacy index */
      GST_INFO_OBJECT (demux, "Seeking to Cues at %" G_GUINT64_FORMAT, offset);
      return perform_seek_to_offset (demux, offset);
    }

    /* well, we are handling it already */
    return TRUE;
  }

  /* delegate to tweaked regular seek */
  return gst_matroska_demux_handle_seek_event (demux, pad, event);
}

static gboolean
gst_matroska_demux_handle_src_event (GstPad * pad, GstEvent * event)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (gst_pad_get_parent (pad));
  gboolean res = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      /* no seeking until we are (safely) ready */
      if (demux->common.state != GST_MATROSKA_READ_STATE_DATA) {
        GST_DEBUG_OBJECT (demux, "not ready for seeking yet");
        return FALSE;
      }
      if (!demux->streaming)
        res = gst_matroska_demux_handle_seek_event (demux, pad, event);
      else
        res = gst_matroska_demux_handle_seek_push (demux, pad, event);
      gst_event_unref (event);
      break;

    case GST_EVENT_QOS:
    {
      GstMatroskaTrackContext *context = gst_pad_get_element_private (pad);
      if (context->type == GST_MATROSKA_TRACK_TYPE_VIDEO) {
        GstMatroskaTrackVideoContext *videocontext =
            (GstMatroskaTrackVideoContext *) context;
        gdouble proportion;
        GstClockTimeDiff diff;
        GstClockTime timestamp;

        gst_event_parse_qos (event, &proportion, &diff, &timestamp);

        GST_OBJECT_LOCK (demux);
        videocontext->earliest_time = timestamp + diff;
        GST_OBJECT_UNLOCK (demux);
      }
      res = TRUE;
      gst_event_unref (event);
      break;
    }

      /* events we don't need to handle */
    case GST_EVENT_NAVIGATION:
      gst_event_unref (event);
      res = FALSE;
      break;

    case GST_EVENT_LATENCY:
    default:
      res = gst_pad_push_event (demux->common.sinkpad, event);
      break;
  }

  gst_object_unref (demux);

  return res;
}

static GstFlowReturn
gst_matroska_demux_seek_to_previous_keyframe (GstMatroskaDemux * demux)
{
  GstFlowReturn ret = GST_FLOW_UNEXPECTED;
  gboolean done = TRUE;
  gint i;

  g_return_val_if_fail (demux->seek_index, GST_FLOW_UNEXPECTED);
  g_return_val_if_fail (demux->seek_entry < demux->seek_index->len,
      GST_FLOW_UNEXPECTED);

  GST_DEBUG_OBJECT (demux, "locating previous keyframe");

  if (!demux->seek_entry) {
    GST_DEBUG_OBJECT (demux, "no earlier index entry");
    goto exit;
  }

  for (i = 0; i < demux->common.src->len; i++) {
    GstMatroskaTrackContext *stream = g_ptr_array_index (demux->common.src, i);

    GST_DEBUG_OBJECT (demux, "segment start %" GST_TIME_FORMAT
        ", stream %d at %" GST_TIME_FORMAT,
        GST_TIME_ARGS (demux->common.segment.start), stream->index,
        GST_TIME_ARGS (stream->from_time));
    if (GST_CLOCK_TIME_IS_VALID (stream->from_time)) {
      if (stream->from_time > demux->common.segment.start) {
        GST_DEBUG_OBJECT (demux, "stream %d not finished yet", stream->index);
        done = FALSE;
      }
    } else {
      /* nothing pushed for this stream;
       * likely seek entry did not start at keyframe, so all was skipped.
       * So we need an earlier entry */
      done = FALSE;
    }
  }

  if (!done) {
    GstMatroskaIndex *entry;

    entry = &g_array_index (demux->seek_index, GstMatroskaIndex,
        --demux->seek_entry);
    if (!gst_matroska_demux_move_to_entry (demux, entry, FALSE))
      goto exit;

    ret = GST_FLOW_OK;
  }

exit:
  return ret;
}

static GstFlowReturn
gst_matroska_demux_parse_tracks (GstMatroskaDemux * demux, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;

  DEBUG_ELEMENT_START (demux, ebml, "Tracks");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "Tracks", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* one track within the "all-tracks" header */
      case GST_MATROSKA_ID_TRACKENTRY:
        ret = gst_matroska_demux_add_stream (demux, ebml);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "Track", id);
        break;
    }
  }
  DEBUG_ELEMENT_STOP (demux, ebml, "Tracks", ret);

  demux->tracks_parsed = TRUE;

  return ret;
}

/*
 * Read signed/unsigned "EBML" numbers.
 * Return: number of bytes processed.
 */

static gint
gst_matroska_ebmlnum_uint (guint8 * data, guint size, guint64 * num)
{
  gint len_mask = 0x80, read = 1, n = 1, num_ffs = 0;
  guint64 total;

  if (size <= 0) {
    return -1;
  }

  total = data[0];
  while (read <= 8 && !(total & len_mask)) {
    read++;
    len_mask >>= 1;
  }
  if (read > 8)
    return -1;

  if ((total &= (len_mask - 1)) == len_mask - 1)
    num_ffs++;
  if (size < read)
    return -1;
  while (n < read) {
    if (data[n] == 0xff)
      num_ffs++;
    total = (total << 8) | data[n];
    n++;
  }

  if (read == num_ffs && total != 0)
    *num = G_MAXUINT64;
  else
    *num = total;

  return read;
}

static gint
gst_matroska_ebmlnum_sint (guint8 * data, guint size, gint64 * num)
{
  guint64 unum;
  gint res;

  /* read as unsigned number first */
  if ((res = gst_matroska_ebmlnum_uint (data, size, &unum)) < 0)
    return -1;

  /* make signed */
  if (unum == G_MAXUINT64)
    *num = G_MAXINT64;
  else
    *num = unum - ((1 << ((7 * res) - 1)) - 1);

  return res;
}

/*
 * Mostly used for subtitles. We add void filler data for each
 * lagging stream to make sure we don't deadlock.
 */

static void
gst_matroska_demux_sync_streams (GstMatroskaDemux * demux)
{
  gint stream_nr;

  GST_OBJECT_LOCK (demux);

  GST_LOG_OBJECT (demux, "Sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (demux->common.segment.last_stop));

  g_assert (demux->common.num_streams == demux->common.src->len);
  for (stream_nr = 0; stream_nr < demux->common.src->len; stream_nr++) {
    GstMatroskaTrackContext *context;

    context = g_ptr_array_index (demux->common.src, stream_nr);

    GST_LOG_OBJECT (demux,
        "Checking for resync on stream %d (%" GST_TIME_FORMAT ")", stream_nr,
        GST_TIME_ARGS (context->pos));

    if (G_LIKELY (context->type != GST_MATROSKA_TRACK_TYPE_SUBTITLE)) {
      GST_LOG_OBJECT (demux, "Skipping sync on non-subtitle stream");
      continue;
    }

    /* does it lag? 0.5 seconds is a random threshold...
     * lag need only be considered if we have advanced into requested segment */
    if (GST_CLOCK_TIME_IS_VALID (context->pos) &&
        GST_CLOCK_TIME_IS_VALID (demux->common.segment.last_stop) &&
        demux->common.segment.last_stop > demux->common.segment.start &&
        context->pos + (GST_SECOND / 2) < demux->common.segment.last_stop) {
      gint64 new_start;
      GstEvent *event;

      new_start = demux->common.segment.last_stop - (GST_SECOND / 2);
      if (GST_CLOCK_TIME_IS_VALID (demux->common.segment.stop))
        new_start = MIN (new_start, demux->common.segment.stop);
      GST_DEBUG_OBJECT (demux,
          "Synchronizing stream %d with others by advancing time " "from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT, stream_nr,
          GST_TIME_ARGS (context->pos), GST_TIME_ARGS (new_start));

      context->pos = new_start;

      /* advance stream time */
      event = gst_event_new_new_segment (TRUE, demux->common.segment.rate,
          demux->common.segment.format, new_start, demux->common.segment.stop,
          new_start);
      GST_OBJECT_UNLOCK (demux);
      gst_pad_push_event (context->pad, event);
      GST_OBJECT_LOCK (demux);
    }
  }

  GST_OBJECT_UNLOCK (demux);
}

static GstFlowReturn
gst_matroska_demux_push_hdr_buf (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream, guint8 * data, guint len)
{
  GstFlowReturn ret, cret;
  GstBuffer *header_buf;

  header_buf = gst_buffer_new_and_alloc (len);
  gst_buffer_set_caps (header_buf, stream->caps);
  memcpy (GST_BUFFER_DATA (header_buf), data, len);

  if (stream->set_discont) {
    GST_BUFFER_FLAG_SET (header_buf, GST_BUFFER_FLAG_DISCONT);
    stream->set_discont = FALSE;
  }

  ret = gst_pad_push (stream->pad, header_buf);

  /* combine flows */
  cret = gst_matroska_demux_combine_flows (demux, stream, ret);

  return cret;
}

static GstFlowReturn
gst_matroska_demux_push_flac_codec_priv_data (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream)
{
  GstFlowReturn ret;
  guint8 *pdata;
  guint off, len;

  GST_LOG_OBJECT (demux, "priv data size = %u", stream->codec_priv_size);

  pdata = (guint8 *) stream->codec_priv;

  /* need at least 'fLaC' marker + STREAMINFO metadata block */
  if (stream->codec_priv_size < ((4) + (4 + 34))) {
    GST_WARNING_OBJECT (demux, "not enough codec priv data for flac headers");
    return GST_FLOW_ERROR;
  }

  if (memcmp (pdata, "fLaC", 4) != 0) {
    GST_WARNING_OBJECT (demux, "no flac marker at start of stream headers");
    return GST_FLOW_ERROR;
  }

  ret = gst_matroska_demux_push_hdr_buf (demux, stream, pdata, 4);
  if (ret != GST_FLOW_OK)
    return ret;

  off = 4;                      /* skip fLaC marker */
  while (off < stream->codec_priv_size) {
    len = GST_READ_UINT8 (pdata + off + 1) << 16;
    len |= GST_READ_UINT8 (pdata + off + 2) << 8;
    len |= GST_READ_UINT8 (pdata + off + 3);

    GST_DEBUG_OBJECT (demux, "header packet: len=%u bytes, flags=0x%02x",
        len, (guint) pdata[off]);

    ret = gst_matroska_demux_push_hdr_buf (demux, stream, pdata + off, len + 4);
    if (ret != GST_FLOW_OK)
      return ret;

    off += 4 + len;
  }
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_push_speex_codec_priv_data (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream)
{
  GstFlowReturn ret;
  guint8 *pdata;

  GST_LOG_OBJECT (demux, "priv data size = %u", stream->codec_priv_size);

  pdata = (guint8 *) stream->codec_priv;

  /* need at least 'fLaC' marker + STREAMINFO metadata block */
  if (stream->codec_priv_size < 80) {
    GST_WARNING_OBJECT (demux, "not enough codec priv data for speex headers");
    return GST_FLOW_ERROR;
  }

  if (memcmp (pdata, "Speex   ", 8) != 0) {
    GST_WARNING_OBJECT (demux, "no Speex marker at start of stream headers");
    return GST_FLOW_ERROR;
  }

  ret = gst_matroska_demux_push_hdr_buf (demux, stream, pdata, 80);
  if (ret != GST_FLOW_OK)
    return ret;

  if (stream->codec_priv_size == 80)
    return ret;
  else
    return gst_matroska_demux_push_hdr_buf (demux, stream, pdata + 80,
        stream->codec_priv_size - 80);
}

static GstFlowReturn
gst_matroska_demux_push_xiph_codec_priv_data (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream)
{
  GstFlowReturn ret;
  guint8 *p = (guint8 *) stream->codec_priv;
  gint i, offset, num_packets;
  guint *length, last;

  if (stream->codec_priv == NULL || stream->codec_priv_size == 0) {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("Missing codec private data for xiph headers, broken file"));
    return GST_FLOW_ERROR;
  }

  /* start of the stream and vorbis audio or theora video, need to
   * send the codec_priv data as first three packets */
  num_packets = p[0] + 1;
  GST_DEBUG_OBJECT (demux, "%u stream headers, total length=%u bytes",
      (guint) num_packets, stream->codec_priv_size);

  length = g_alloca (num_packets * sizeof (guint));
  last = 0;
  offset = 1;

  /* first packets, read length values */
  for (i = 0; i < num_packets - 1; i++) {
    length[i] = 0;
    while (offset < stream->codec_priv_size) {
      length[i] += p[offset];
      if (p[offset++] != 0xff)
        break;
    }
    last += length[i];
  }
  if (offset + last > stream->codec_priv_size)
    return GST_FLOW_ERROR;

  /* last packet is the remaining size */
  length[i] = stream->codec_priv_size - offset - last;

  for (i = 0; i < num_packets; i++) {
    GST_DEBUG_OBJECT (demux, "buffer %d: length=%u bytes", i,
        (guint) length[i]);
    if (offset + length[i] > stream->codec_priv_size)
      return GST_FLOW_ERROR;

    ret =
        gst_matroska_demux_push_hdr_buf (demux, stream, p + offset, length[i]);
    if (ret != GST_FLOW_OK)
      return ret;

    offset += length[i];
  }
  return GST_FLOW_OK;
}

static void
gst_matroska_demux_push_dvd_clut_change_event (GstMatroskaDemux * demux,
    GstMatroskaTrackContext * stream)
{
  gchar *buf, *start;

  g_assert (!strcmp (stream->codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_VOBSUB));

  if (!stream->codec_priv)
    return;

  /* ideally, VobSub private data should be parsed and stored more convenient
   * elsewhere, but for now, only interested in a small part */

  /* make sure we have terminating 0 */
  buf = g_strndup ((gchar *) stream->codec_priv, stream->codec_priv_size);

  /* just locate and parse palette part */
  start = strstr (buf, "palette:");
  if (start) {
    gint i;
    guint32 clut[16];
    guint32 col;
    guint8 r, g, b, y, u, v;

    start += 8;
    while (g_ascii_isspace (*start))
      start++;
    for (i = 0; i < 16; i++) {
      if (sscanf (start, "%06x", &col) != 1)
        break;
      start += 6;
      while ((*start == ',') || g_ascii_isspace (*start))
        start++;
      /* sigh, need to convert this from vobsub pseudo-RGB to YUV */
      r = (col >> 16) & 0xff;
      g = (col >> 8) & 0xff;
      b = col & 0xff;
      y = CLAMP ((0.1494 * r + 0.6061 * g + 0.2445 * b) * 219 / 255 + 16, 0,
          255);
      u = CLAMP (0.6066 * r - 0.4322 * g - 0.1744 * b + 128, 0, 255);
      v = CLAMP (-0.08435 * r - 0.3422 * g + 0.4266 * b + 128, 0, 255);
      clut[i] = (y << 16) | (u << 8) | v;
    }

    /* got them all without problems; build and send event */
    if (i == 16) {
      GstStructure *s;

      s = gst_structure_new ("application/x-gst-dvd", "event", G_TYPE_STRING,
          "dvd-spu-clut-change", "clut00", G_TYPE_INT, clut[0], "clut01",
          G_TYPE_INT, clut[1], "clut02", G_TYPE_INT, clut[2], "clut03",
          G_TYPE_INT, clut[3], "clut04", G_TYPE_INT, clut[4], "clut05",
          G_TYPE_INT, clut[5], "clut06", G_TYPE_INT, clut[6], "clut07",
          G_TYPE_INT, clut[7], "clut08", G_TYPE_INT, clut[8], "clut09",
          G_TYPE_INT, clut[9], "clut10", G_TYPE_INT, clut[10], "clut11",
          G_TYPE_INT, clut[11], "clut12", G_TYPE_INT, clut[12], "clut13",
          G_TYPE_INT, clut[13], "clut14", G_TYPE_INT, clut[14], "clut15",
          G_TYPE_INT, clut[15], NULL);

      gst_pad_push_event (stream->pad,
          gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM, s));
    }
  }
  g_free (buf);
}

static GstFlowReturn
gst_matroska_demux_add_mpeg_seq_header (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  guint8 *seq_header;
  guint seq_header_len;
  guint32 header;

  if (stream->codec_state) {
    seq_header = stream->codec_state;
    seq_header_len = stream->codec_state_size;
  } else if (stream->codec_priv) {
    seq_header = stream->codec_priv;
    seq_header_len = stream->codec_priv_size;
  } else {
    return GST_FLOW_OK;
  }

  /* Sequence header only needed for keyframes */
  if (GST_BUFFER_FLAG_IS_SET (*buf, GST_BUFFER_FLAG_DELTA_UNIT))
    return GST_FLOW_OK;

  if (GST_BUFFER_SIZE (*buf) < 4)
    return GST_FLOW_OK;

  header = GST_READ_UINT32_BE (GST_BUFFER_DATA (*buf));
  /* Sequence start code, if not found prepend */
  if (header != 0x000001b3) {
    GstBuffer *newbuf;

    newbuf = gst_buffer_new_and_alloc (GST_BUFFER_SIZE (*buf) + seq_header_len);
    gst_buffer_set_caps (newbuf, stream->caps);

    GST_DEBUG_OBJECT (element, "Prepending MPEG sequence header");
    gst_buffer_copy_metadata (newbuf, *buf, GST_BUFFER_COPY_TIMESTAMPS |
        GST_BUFFER_COPY_FLAGS);
    g_memmove (GST_BUFFER_DATA (newbuf), seq_header, seq_header_len);
    g_memmove (GST_BUFFER_DATA (newbuf) + seq_header_len,
        GST_BUFFER_DATA (*buf), GST_BUFFER_SIZE (*buf));
    gst_buffer_unref (*buf);
    *buf = newbuf;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_add_wvpk_header (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  GstMatroskaTrackAudioContext *audiocontext =
      (GstMatroskaTrackAudioContext *) stream;
  GstBuffer *newbuf = NULL;
  guint8 *data;
  guint newlen;
  Wavpack4Header wvh;

  wvh.ck_id[0] = 'w';
  wvh.ck_id[1] = 'v';
  wvh.ck_id[2] = 'p';
  wvh.ck_id[3] = 'k';

  wvh.version = GST_READ_UINT16_LE (stream->codec_priv);
  wvh.track_no = 0;
  wvh.index_no = 0;
  wvh.total_samples = -1;
  wvh.block_index = audiocontext->wvpk_block_index;

  if (audiocontext->channels <= 2) {
    guint32 block_samples;

    block_samples = GST_READ_UINT32_LE (GST_BUFFER_DATA (*buf));
    /* we need to reconstruct the header of the wavpack block */

    /* -20 because ck_size is the size of the wavpack block -8
     * and lace_size is the size of the wavpack block + 12
     * (the three guint32 of the header that already are in the buffer) */
    wvh.ck_size = GST_BUFFER_SIZE (*buf) + sizeof (Wavpack4Header) - 20;

    /* block_samples, flags and crc are already in the buffer */
    newlen = GST_BUFFER_SIZE (*buf) + sizeof (Wavpack4Header) - 12;
    newbuf = gst_buffer_new_and_alloc (newlen);
    gst_buffer_set_caps (newbuf, stream->caps);

    data = GST_BUFFER_DATA (newbuf);
    data[0] = 'w';
    data[1] = 'v';
    data[2] = 'p';
    data[3] = 'k';
    GST_WRITE_UINT32_LE (data + 4, wvh.ck_size);
    GST_WRITE_UINT16_LE (data + 8, wvh.version);
    GST_WRITE_UINT8 (data + 10, wvh.track_no);
    GST_WRITE_UINT8 (data + 11, wvh.index_no);
    GST_WRITE_UINT32_LE (data + 12, wvh.total_samples);
    GST_WRITE_UINT32_LE (data + 16, wvh.block_index);
    g_memmove (data + 20, GST_BUFFER_DATA (*buf), GST_BUFFER_SIZE (*buf));
    gst_buffer_copy_metadata (newbuf, *buf,
        GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_FLAGS);
    gst_buffer_unref (*buf);
    *buf = newbuf;
    audiocontext->wvpk_block_index += block_samples;
  } else {
    guint8 *outdata;
    guint outpos = 0;
    guint size;
    guint32 block_samples, flags, crc, blocksize;

    data = GST_BUFFER_DATA (*buf);
    size = GST_BUFFER_SIZE (*buf);

    if (size < 4) {
      GST_ERROR_OBJECT (element, "Too small wavpack buffer");
      return GST_FLOW_ERROR;
    }

    block_samples = GST_READ_UINT32_LE (data);
    data += 4;
    size -= 4;

    while (size > 12) {
      flags = GST_READ_UINT32_LE (data);
      data += 4;
      size -= 4;
      crc = GST_READ_UINT32_LE (data);
      data += 4;
      size -= 4;
      blocksize = GST_READ_UINT32_LE (data);
      data += 4;
      size -= 4;

      if (blocksize == 0 || size < blocksize)
        break;

      if (newbuf == NULL) {
        newbuf = gst_buffer_new_and_alloc (sizeof (Wavpack4Header) + blocksize);
        gst_buffer_set_caps (newbuf, stream->caps);

        gst_buffer_copy_metadata (newbuf, *buf,
            GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_FLAGS);

        outpos = 0;
        outdata = GST_BUFFER_DATA (newbuf);
      } else {
        GST_BUFFER_SIZE (newbuf) += sizeof (Wavpack4Header) + blocksize;
        GST_BUFFER_DATA (newbuf) =
            g_realloc (GST_BUFFER_DATA (newbuf), GST_BUFFER_SIZE (newbuf));
        GST_BUFFER_MALLOCDATA (newbuf) = GST_BUFFER_DATA (newbuf);
        outdata = GST_BUFFER_DATA (newbuf);
      }

      outdata[outpos] = 'w';
      outdata[outpos + 1] = 'v';
      outdata[outpos + 2] = 'p';
      outdata[outpos + 3] = 'k';
      outpos += 4;

      GST_WRITE_UINT32_LE (outdata + outpos,
          blocksize + sizeof (Wavpack4Header) - 8);
      GST_WRITE_UINT16_LE (outdata + outpos + 4, wvh.version);
      GST_WRITE_UINT8 (outdata + outpos + 6, wvh.track_no);
      GST_WRITE_UINT8 (outdata + outpos + 7, wvh.index_no);
      GST_WRITE_UINT32_LE (outdata + outpos + 8, wvh.total_samples);
      GST_WRITE_UINT32_LE (outdata + outpos + 12, wvh.block_index);
      GST_WRITE_UINT32_LE (outdata + outpos + 16, block_samples);
      GST_WRITE_UINT32_LE (outdata + outpos + 20, flags);
      GST_WRITE_UINT32_LE (outdata + outpos + 24, crc);
      outpos += 28;

      g_memmove (outdata + outpos, data, blocksize);
      outpos += blocksize;
      data += blocksize;
      size -= blocksize;
    }
    gst_buffer_unref (*buf);
    *buf = newbuf;
    audiocontext->wvpk_block_index += block_samples;
  }

  return GST_FLOW_OK;
}

/* @text must be null-terminated */
static gboolean
gst_matroska_demux_subtitle_chunk_has_tag (GstElement * element,
    const gchar * text)
{
  gchar *tag;

  /* yes, this might all lead to false positives ... */
  tag = (gchar *) text;
  while ((tag = strchr (tag, '<'))) {
    tag++;
    if (*tag != '\0' && *(tag + 1) == '>') {
      /* some common convenience ones */
      /* maybe any character will do here ? */
      switch (*tag) {
        case 'b':
        case 'i':
        case 'u':
        case 's':
          return TRUE;
        default:
          return FALSE;
      }
    }
  }

  if (strstr (text, "<span"))
    return TRUE;

  return FALSE;
}

static GstFlowReturn
gst_matroska_demux_check_subtitle_buffer (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  GstMatroskaTrackSubtitleContext *sub_stream;
  const gchar *encoding, *data;
  GError *err = NULL;
  GstBuffer *newbuf;
  gchar *utf8;
  guint size;

  sub_stream = (GstMatroskaTrackSubtitleContext *) stream;

  data = (const gchar *) GST_BUFFER_DATA (*buf);
  size = GST_BUFFER_SIZE (*buf);

  if (!sub_stream->invalid_utf8) {
    if (g_utf8_validate (data, size, NULL)) {
      goto next;
    }
    GST_WARNING_OBJECT (element, "subtitle stream %d is not valid UTF-8, this "
        "is broken according to the matroska specification", stream->num);
    sub_stream->invalid_utf8 = TRUE;
  }

  /* file with broken non-UTF8 subtitle, do the best we can do to fix it */
  encoding = g_getenv ("GST_SUBTITLE_ENCODING");
  if (encoding == NULL || *encoding == '\0') {
    /* if local encoding is UTF-8 and no encoding specified
     * via the environment variable, assume ISO-8859-15 */
    if (g_get_charset (&encoding)) {
      encoding = "ISO-8859-15";
    }
  }

  utf8 = g_convert_with_fallback (data, size, "UTF-8", encoding, (char *) "*",
      NULL, NULL, &err);

  if (err) {
    GST_LOG_OBJECT (element, "could not convert string from '%s' to UTF-8: %s",
        encoding, err->message);
    g_error_free (err);
    g_free (utf8);

    /* invalid input encoding, fall back to ISO-8859-15 (always succeeds) */
    encoding = "ISO-8859-15";
    utf8 = g_convert_with_fallback (data, size, "UTF-8", encoding, (char *) "*",
        NULL, NULL, NULL);
  }

  GST_LOG_OBJECT (element, "converted subtitle text from %s to UTF-8 %s",
      encoding, (err) ? "(using ISO-8859-15 as fallback)" : "");

  if (utf8 == NULL)
    utf8 = g_strdup ("invalid subtitle");

  newbuf = gst_buffer_new ();
  GST_BUFFER_MALLOCDATA (newbuf) = (guint8 *) utf8;
  GST_BUFFER_DATA (newbuf) = (guint8 *) utf8;
  GST_BUFFER_SIZE (newbuf) = strlen (utf8);
  gst_buffer_copy_metadata (newbuf, *buf, GST_BUFFER_COPY_ALL);
  gst_buffer_unref (*buf);

  *buf = newbuf;
  data = (const gchar *) GST_BUFFER_DATA (*buf);
  size = GST_BUFFER_SIZE (*buf);

next:

  if (sub_stream->check_markup) {
    /* caps claim markup text, so we need to escape text,
     * except if text is already markup and then needs no further escaping */
    sub_stream->seen_markup_tag = sub_stream->seen_markup_tag ||
        gst_matroska_demux_subtitle_chunk_has_tag (element, data);

    if (!sub_stream->seen_markup_tag) {
      utf8 = g_markup_escape_text (data, size);

      newbuf = gst_buffer_new ();
      GST_BUFFER_MALLOCDATA (newbuf) = (guint8 *) utf8;
      GST_BUFFER_DATA (newbuf) = (guint8 *) utf8;
      GST_BUFFER_SIZE (newbuf) = strlen (utf8);
      gst_buffer_copy_metadata (newbuf, *buf, GST_BUFFER_COPY_ALL);
      gst_buffer_unref (*buf);

      *buf = newbuf;
    }
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_check_aac (GstElement * element,
    GstMatroskaTrackContext * stream, GstBuffer ** buf)
{
  const guint8 *data;
  guint size;

  data = GST_BUFFER_DATA (*buf);
  size = GST_BUFFER_SIZE (*buf);

  if (size > 2 && data[0] == 0xff && (data[1] >> 4 == 0x0f)) {
    GstCaps *new_caps;
    GstStructure *s;

    /* tss, ADTS data, remove codec_data
     * still assume it is at least parsed */
    new_caps = gst_caps_copy (stream->caps);
    s = gst_caps_get_structure (new_caps, 0);
    g_assert (s);
    gst_structure_remove_field (s, "codec_data");
    gst_caps_replace (&stream->caps, new_caps);
    gst_pad_set_caps (stream->pad, new_caps);
    gst_buffer_set_caps (*buf, new_caps);
    GST_DEBUG_OBJECT (element, "ADTS AAC audio data; removing codec-data, "
        "new caps: %" GST_PTR_FORMAT, new_caps);
    gst_caps_unref (new_caps);
  }

  /* disable subsequent checking */
  stream->postprocess_frame = NULL;

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_demux_parse_blockgroup_or_simpleblock (GstMatroskaDemux * demux,
    GstEbmlRead * ebml, guint64 cluster_time, guint64 cluster_offset,
    gboolean is_simpleblock)
{
  GstMatroskaTrackContext *stream = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  gboolean readblock = FALSE;
  guint32 id;
  guint64 block_duration = -1;
  GstBuffer *buf = NULL;
  gint stream_num = -1, n, laces = 0;
  guint size = 0;
  gint *lace_size = NULL;
  gint64 time = 0;
  gint flags = 0;
  gint64 referenceblock = 0;
  gint64 offset;

  offset = gst_ebml_read_get_offset (ebml);

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if (!is_simpleblock) {
      if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK) {
        goto data_error;
      }
    } else {
      id = GST_MATROSKA_ID_SIMPLEBLOCK;
    }

    switch (id) {
        /* one block inside the group. Note, block parsing is one
         * of the harder things, so this code is a bit complicated.
         * See http://www.matroska.org/ for documentation. */
      case GST_MATROSKA_ID_SIMPLEBLOCK:
      case GST_MATROSKA_ID_BLOCK:
      {
        guint64 num;
        guint8 *data;

        if (buf) {
          gst_buffer_unref (buf);
          buf = NULL;
        }
        if ((ret = gst_ebml_read_buffer (ebml, &id, &buf)) != GST_FLOW_OK)
          break;

        data = GST_BUFFER_DATA (buf);
        size = GST_BUFFER_SIZE (buf);

        /* first byte(s): blocknum */
        if ((n = gst_matroska_ebmlnum_uint (data, size, &num)) < 0)
          goto data_error;
        data += n;
        size -= n;

        /* fetch stream from num */
        stream_num = gst_matroska_read_common_stream_from_num (&demux->common,
            num);
        if (G_UNLIKELY (size < 3)) {
          GST_WARNING_OBJECT (demux, "Invalid size %u", size);
          /* non-fatal, try next block(group) */
          ret = GST_FLOW_OK;
          goto done;
        } else if (G_UNLIKELY (stream_num < 0 ||
                stream_num >= demux->common.num_streams)) {
          /* let's not give up on a stray invalid track number */
          GST_WARNING_OBJECT (demux,
              "Invalid stream %d for track number %" G_GUINT64_FORMAT
              "; ignoring block", stream_num, num);
          goto done;
        }

        stream = g_ptr_array_index (demux->common.src, stream_num);

        /* time (relative to cluster time) */
        time = ((gint16) GST_READ_UINT16_BE (data));
        data += 2;
        size -= 2;
        flags = GST_READ_UINT8 (data);
        data += 1;
        size -= 1;

        GST_LOG_OBJECT (demux, "time %" G_GUINT64_FORMAT ", flags %d", time,
            flags);

        switch ((flags & 0x06) >> 1) {
          case 0x0:            /* no lacing */
            laces = 1;
            lace_size = g_new (gint, 1);
            lace_size[0] = size;
            break;

          case 0x1:            /* xiph lacing */
          case 0x2:            /* fixed-size lacing */
          case 0x3:            /* EBML lacing */
            if (size == 0)
              goto invalid_lacing;
            laces = GST_READ_UINT8 (data) + 1;
            data += 1;
            size -= 1;
            lace_size = g_new0 (gint, laces);

            switch ((flags & 0x06) >> 1) {
              case 0x1:        /* xiph lacing */  {
                guint temp, total = 0;

                for (n = 0; ret == GST_FLOW_OK && n < laces - 1; n++) {
                  while (1) {
                    if (size == 0)
                      goto invalid_lacing;
                    temp = GST_READ_UINT8 (data);
                    lace_size[n] += temp;
                    data += 1;
                    size -= 1;
                    if (temp != 0xff)
                      break;
                  }
                  total += lace_size[n];
                }
                lace_size[n] = size - total;
                break;
              }

              case 0x2:        /* fixed-size lacing */
                for (n = 0; n < laces; n++)
                  lace_size[n] = size / laces;
                break;

              case 0x3:        /* EBML lacing */  {
                guint total;

                if ((n = gst_matroska_ebmlnum_uint (data, size, &num)) < 0)
                  goto data_error;
                data += n;
                size -= n;
                total = lace_size[0] = num;
                for (n = 1; ret == GST_FLOW_OK && n < laces - 1; n++) {
                  gint64 snum;
                  gint r;

                  if ((r = gst_matroska_ebmlnum_sint (data, size, &snum)) < 0)
                    goto data_error;
                  data += r;
                  size -= r;
                  lace_size[n] = lace_size[n - 1] + snum;
                  total += lace_size[n];
                }
                if (n < laces)
                  lace_size[n] = size - total;
                break;
              }
            }
            break;
        }

        if (stream->send_xiph_headers) {
          ret = gst_matroska_demux_push_xiph_codec_priv_data (demux, stream);
          stream->send_xiph_headers = FALSE;
        }

        if (stream->send_flac_headers) {
          ret = gst_matroska_demux_push_flac_codec_priv_data (demux, stream);
          stream->send_flac_headers = FALSE;
        }

        if (stream->send_speex_headers) {
          ret = gst_matroska_demux_push_speex_codec_priv_data (demux, stream);
          stream->send_speex_headers = FALSE;
        }

        if (stream->send_dvd_event) {
          gst_matroska_demux_push_dvd_clut_change_event (demux, stream);
          /* FIXME: should we send this event again after (flushing) seek ? */
          stream->send_dvd_event = FALSE;
        }

        if (ret != GST_FLOW_OK)
          break;

        readblock = TRUE;
        break;
      }

      case GST_MATROSKA_ID_BLOCKDURATION:{
        ret = gst_ebml_read_uint (ebml, &id, &block_duration);
        GST_DEBUG_OBJECT (demux, "BlockDuration: %" G_GUINT64_FORMAT,
            block_duration);
        break;
      }

      case GST_MATROSKA_ID_REFERENCEBLOCK:{
        ret = gst_ebml_read_sint (ebml, &id, &referenceblock);
        GST_DEBUG_OBJECT (demux, "ReferenceBlock: %" G_GINT64_FORMAT,
            referenceblock);
        break;
      }

      case GST_MATROSKA_ID_CODECSTATE:{
        guint8 *data;
        guint64 data_len = 0;

        if ((ret =
                gst_ebml_read_binary (ebml, &id, &data,
                    &data_len)) != GST_FLOW_OK)
          break;

        if (G_UNLIKELY (stream == NULL)) {
          GST_WARNING_OBJECT (demux,
              "Unexpected CodecState subelement - ignoring");
          break;
        }

        g_free (stream->codec_state);
        stream->codec_state = data;
        stream->codec_state_size = data_len;

        /* Decode if necessary */
        if (stream->encodings && stream->encodings->len > 0
            && stream->codec_state && stream->codec_state_size > 0) {
          if (!gst_matroska_decode_data (stream->encodings,
                  &stream->codec_state, &stream->codec_state_size,
                  GST_MATROSKA_TRACK_ENCODING_SCOPE_CODEC_DATA, TRUE)) {
            GST_WARNING_OBJECT (demux, "Decoding codec state failed");
          }
        }

        GST_DEBUG_OBJECT (demux, "CodecState of %u bytes",
            stream->codec_state_size);
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "BlockGroup", id);
        break;

      case GST_MATROSKA_ID_BLOCKVIRTUAL:
      case GST_MATROSKA_ID_BLOCKADDITIONS:
      case GST_MATROSKA_ID_REFERENCEPRIORITY:
      case GST_MATROSKA_ID_REFERENCEVIRTUAL:
      case GST_MATROSKA_ID_SLICES:
        GST_DEBUG_OBJECT (demux,
            "Skipping BlockGroup subelement 0x%x - ignoring", id);
        ret = gst_ebml_read_skip (ebml);
        break;
    }

    if (is_simpleblock)
      break;
  }

  /* reading a number or so could have failed */
  if (ret != GST_FLOW_OK)
    goto data_error;

  if (ret == GST_FLOW_OK && readblock) {
    guint64 duration = 0;
    gint64 lace_time = 0;
    gboolean delta_unit;

    stream = g_ptr_array_index (demux->common.src, stream_num);

    if (cluster_time != GST_CLOCK_TIME_NONE) {
      /* FIXME: What to do with negative timestamps? Give timestamp 0 or -1?
       * Drop unless the lace contains timestamp 0? */
      if (time < 0 && (-time) > cluster_time) {
        lace_time = 0;
      } else {
        if (stream->timecodescale == 1.0)
          lace_time = (cluster_time + time) * demux->common.time_scale;
        else
          lace_time =
              gst_util_guint64_to_gdouble ((cluster_time + time) *
              demux->common.time_scale) * stream->timecodescale;
      }
    } else {
      lace_time = GST_CLOCK_TIME_NONE;
    }

    /* need to refresh segment info ASAP */
    if (GST_CLOCK_TIME_IS_VALID (lace_time) && demux->need_newsegment) {
      guint64 clace_time;

      GST_DEBUG_OBJECT (demux,
          "generating segment starting at %" GST_TIME_FORMAT,
          GST_TIME_ARGS (lace_time));
      if (!GST_CLOCK_TIME_IS_VALID (demux->stream_start_time)) {
        demux->stream_start_time = lace_time;
        GST_DEBUG_OBJECT (demux,
            "Setting stream start time to %" GST_TIME_FORMAT,
            GST_TIME_ARGS (lace_time));
      }
      clace_time = MAX (lace_time, demux->stream_start_time);
      gst_segment_set_newsegment (&demux->common.segment, FALSE,
          demux->common.segment.rate, GST_FORMAT_TIME, clace_time,
          GST_CLOCK_TIME_NONE, clace_time - demux->stream_start_time);
      /* now convey our segment notion downstream */
      gst_matroska_demux_send_event (demux, gst_event_new_new_segment (FALSE,
              demux->common.segment.rate, demux->common.segment.format,
              demux->common.segment.start, demux->common.segment.stop,
              demux->common.segment.start));
      demux->need_newsegment = FALSE;
    }

    if (block_duration != -1) {
      if (stream->timecodescale == 1.0)
        duration = gst_util_uint64_scale (block_duration,
            demux->common.time_scale, 1);
      else
        duration =
            gst_util_gdouble_to_guint64 (gst_util_guint64_to_gdouble
            (gst_util_uint64_scale (block_duration, demux->common.time_scale,
                    1)) * stream->timecodescale);
    } else if (stream->default_duration) {
      duration = stream->default_duration * laces;
    }
    /* else duration is diff between timecode of this and next block */

    /* For SimpleBlock, look at the keyframe bit in flags. Otherwise,
       a ReferenceBlock implies that this is not a keyframe. In either
       case, it only makes sense for video streams. */
    delta_unit = stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO &&
        ((is_simpleblock && !(flags & 0x80)) || referenceblock);

    if (delta_unit && stream->set_discont) {
      /* When doing seeks or such, we need to restart on key frames or
       * decoders might choke. */
      GST_DEBUG_OBJECT (demux, "skipping delta unit");
      goto done;
    }

    for (n = 0; n < laces; n++) {
      GstBuffer *sub;

      if (G_UNLIKELY (lace_size[n] > size)) {
        GST_WARNING_OBJECT (demux, "Invalid lace size");
        break;
      }

      /* QoS for video track with an index. the assumption is that
         index entries point to keyframes, but if that is not true we
         will instad skip until the next keyframe. */
      if (GST_CLOCK_TIME_IS_VALID (lace_time) &&
          stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO &&
          stream->index_table && demux->common.segment.rate > 0.0) {
        GstMatroskaTrackVideoContext *videocontext =
            (GstMatroskaTrackVideoContext *) stream;
        GstClockTime earliest_time;
        GstClockTime earliest_stream_time;

        GST_OBJECT_LOCK (demux);
        earliest_time = videocontext->earliest_time;
        GST_OBJECT_UNLOCK (demux);
        earliest_stream_time = gst_segment_to_position (&demux->common.segment,
            GST_FORMAT_TIME, earliest_time);

        if (GST_CLOCK_TIME_IS_VALID (lace_time) &&
            GST_CLOCK_TIME_IS_VALID (earliest_stream_time) &&
            lace_time <= earliest_stream_time) {
          /* find index entry (keyframe) <= earliest_stream_time */
          GstMatroskaIndex *entry =
              gst_util_array_binary_search (stream->index_table->data,
              stream->index_table->len, sizeof (GstMatroskaIndex),
              (GCompareDataFunc) gst_matroska_index_seek_find,
              GST_SEARCH_MODE_BEFORE, &earliest_stream_time, NULL);

          /* if that entry (keyframe) is after the current the current
             buffer, we can skip pushing (and thus decoding) all
             buffers until that keyframe. */
          if (entry && GST_CLOCK_TIME_IS_VALID (entry->time) &&
              entry->time > lace_time) {
            GST_LOG_OBJECT (demux, "Skipping lace before late keyframe");
            stream->set_discont = TRUE;
            goto next_lace;
          }
        }
      }

      sub = gst_buffer_create_sub (buf,
          GST_BUFFER_SIZE (buf) - size, lace_size[n]);
      GST_DEBUG_OBJECT (demux, "created subbuffer %p", sub);

      if (delta_unit)
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DELTA_UNIT);
      else
        GST_BUFFER_FLAG_UNSET (sub, GST_BUFFER_FLAG_DELTA_UNIT);

      if (stream->encodings != NULL && stream->encodings->len > 0)
        sub = gst_matroska_decode_buffer (stream, sub);

      if (sub == NULL) {
        GST_WARNING_OBJECT (demux, "Decoding buffer failed");
        goto next_lace;
      }

      GST_BUFFER_TIMESTAMP (sub) = lace_time;

      if (GST_CLOCK_TIME_IS_VALID (lace_time)) {
        GstClockTime last_stop_end;

        /* Check if this stream is after segment stop */
        if (GST_CLOCK_TIME_IS_VALID (demux->common.segment.stop) &&
            lace_time >= demux->common.segment.stop) {
          GST_DEBUG_OBJECT (demux,
              "Stream %d after segment stop %" GST_TIME_FORMAT, stream->index,
              GST_TIME_ARGS (demux->common.segment.stop));
          gst_buffer_unref (sub);
          goto eos;
        }
        if (offset >= stream->to_offset) {
          GST_DEBUG_OBJECT (demux, "Stream %d after playback section",
              stream->index);
          gst_buffer_unref (sub);
          goto eos;
        }

        /* handle gaps, e.g. non-zero start-time, or an cue index entry
         * that landed us with timestamps not quite intended */
        GST_OBJECT_LOCK (demux);
        if (demux->max_gap_time &&
            GST_CLOCK_TIME_IS_VALID (demux->last_stop_end) &&
            demux->common.segment.rate > 0.0) {
          GstClockTimeDiff diff;
          GstEvent *event1, *event2;

          /* only send newsegments with increasing start times,
           * otherwise if these go back and forth downstream (sinks) increase
           * accumulated time and running_time */
          diff = GST_CLOCK_DIFF (demux->last_stop_end, lace_time);
          if (diff > 0 && diff > demux->max_gap_time
              && lace_time > demux->common.segment.start
              && (!GST_CLOCK_TIME_IS_VALID (demux->common.segment.stop)
                  || lace_time < demux->common.segment.stop)) {
            GST_DEBUG_OBJECT (demux,
                "Gap of %" G_GINT64_FORMAT " ns detected in"
                "stream %d (%" GST_TIME_FORMAT " -> %" GST_TIME_FORMAT "). "
                "Sending updated NEWSEGMENT events", diff,
                stream->index, GST_TIME_ARGS (stream->pos),
                GST_TIME_ARGS (lace_time));
            /* send newsegment events such that the gap is not accounted in
             * accum time, hence running_time */
            /* close ahead of gap */
            event1 = gst_event_new_new_segment (TRUE,
                demux->common.segment.rate, demux->common.segment.format,
                demux->last_stop_end, demux->last_stop_end,
                demux->last_stop_end);
            /* skip gap */
            event2 = gst_event_new_new_segment (FALSE,
                demux->common.segment.rate,
                demux->common.segment.format, lace_time,
                demux->common.segment.stop, lace_time);
            GST_OBJECT_UNLOCK (demux);
            gst_matroska_demux_send_event (demux, event1);
            gst_matroska_demux_send_event (demux, event2);
            GST_OBJECT_LOCK (demux);
            /* align segment view with downstream,
             * prevents double-counting accum when closing segment */
            gst_segment_set_newsegment (&demux->common.segment, FALSE,
                demux->common.segment.rate, demux->common.segment.format,
                lace_time, demux->common.segment.stop, lace_time);
            demux->common.segment.last_stop = lace_time;
          }
        }

        if (!GST_CLOCK_TIME_IS_VALID (demux->common.segment.last_stop)
            || demux->common.segment.last_stop < lace_time) {
          demux->common.segment.last_stop = lace_time;
        }
        GST_OBJECT_UNLOCK (demux);

        last_stop_end = lace_time;
        if (duration) {
          GST_BUFFER_DURATION (sub) = duration / laces;
          last_stop_end += GST_BUFFER_DURATION (sub);
        }

        if (!GST_CLOCK_TIME_IS_VALID (demux->last_stop_end) ||
            demux->last_stop_end < last_stop_end)
          demux->last_stop_end = last_stop_end;

        GST_OBJECT_LOCK (demux);
        if (demux->common.segment.duration == -1 ||
            demux->stream_start_time + demux->common.segment.duration <
            last_stop_end) {
          gst_segment_set_duration (&demux->common.segment, GST_FORMAT_TIME,
              last_stop_end - demux->stream_start_time);
          GST_OBJECT_UNLOCK (demux);
          if (!demux->invalid_duration) {
            gst_element_post_message (GST_ELEMENT_CAST (demux),
                gst_message_new_duration (GST_OBJECT_CAST (demux),
                    GST_FORMAT_TIME, GST_CLOCK_TIME_NONE));
            demux->invalid_duration = TRUE;
          }
        } else {
          GST_OBJECT_UNLOCK (demux);
        }
      }

      stream->pos = lace_time;

      gst_matroska_demux_sync_streams (demux);

      if (stream->set_discont) {
        GST_DEBUG_OBJECT (demux, "marking DISCONT");
        GST_BUFFER_FLAG_SET (sub, GST_BUFFER_FLAG_DISCONT);
        stream->set_discont = FALSE;
      }

      /* reverse playback book-keeping */
      if (!GST_CLOCK_TIME_IS_VALID (stream->from_time))
        stream->from_time = lace_time;
      if (stream->from_offset == -1)
        stream->from_offset = offset;

      GST_DEBUG_OBJECT (demux,
          "Pushing lace %d, data of size %d for stream %d, time=%"
          GST_TIME_FORMAT " and duration=%" GST_TIME_FORMAT, n,
          GST_BUFFER_SIZE (sub), stream_num,
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (sub)),
          GST_TIME_ARGS (GST_BUFFER_DURATION (sub)));

      if (demux->common.element_index) {
        if (stream->index_writer_id == -1)
          gst_index_get_writer_id (demux->common.element_index,
              GST_OBJECT (stream->pad), &stream->index_writer_id);

        GST_LOG_OBJECT (demux, "adding association %" GST_TIME_FORMAT "-> %"
            G_GUINT64_FORMAT " for writer id %d",
            GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (sub)), cluster_offset,
            stream->index_writer_id);
        gst_index_add_association (demux->common.element_index,
            stream->index_writer_id, GST_BUFFER_FLAG_IS_SET (sub,
                GST_BUFFER_FLAG_DELTA_UNIT) ? 0 : GST_ASSOCIATION_FLAG_KEY_UNIT,
            GST_FORMAT_TIME, GST_BUFFER_TIMESTAMP (sub), GST_FORMAT_BYTES,
            cluster_offset, NULL);
      }

      gst_buffer_set_caps (sub, GST_PAD_CAPS (stream->pad));

      /* Postprocess the buffers depending on the codec used */
      if (stream->postprocess_frame) {
        GST_LOG_OBJECT (demux, "running post process");
        ret = stream->postprocess_frame (GST_ELEMENT (demux), stream, &sub);
      }

      /* At this point, we have a sub-buffer pointing at data within a larger
         buffer. This data might not be aligned with anything. If the data is
         raw samples though, we want it aligned to the raw type (eg, 4 bytes
         for 32 bit samples, etc), or bad things will happen downstream as
         elements typically assume minimal alignment.
         Therefore, create an aligned copy if necessary. */
      g_assert (stream->alignment <= G_MEM_ALIGN);
      if (((guintptr) GST_BUFFER_DATA (sub)) & (stream->alignment - 1)) {
        GstBuffer *buffer = gst_buffer_new_and_alloc (GST_BUFFER_SIZE (sub));
        memcpy (GST_BUFFER_DATA (buffer), GST_BUFFER_DATA (sub),
            GST_BUFFER_SIZE (sub));
        gst_buffer_copy_metadata (buffer, sub, GST_BUFFER_COPY_ALL);
        GST_DEBUG_OBJECT (demux, "We want output aligned on %d, reallocated",
            stream->alignment);
        gst_buffer_unref (sub);
        sub = buffer;
      }

      ret = gst_pad_push (stream->pad, sub);
      if (demux->common.segment.rate < 0) {
        if (lace_time > demux->common.segment.stop
            && ret == GST_FLOW_UNEXPECTED) {
          /* In reverse playback we can get a GST_FLOW_UNEXPECTED when
           * we are at the end of the segment, so we just need to jump
           * back to the previous section. */
          GST_DEBUG_OBJECT (demux, "downstream has reached end of segment");
          ret = GST_FLOW_OK;
        }
      }
      /* combine flows */
      ret = gst_matroska_demux_combine_flows (demux, stream, ret);

    next_lace:
      size -= lace_size[n];
      if (lace_time != GST_CLOCK_TIME_NONE && duration)
        lace_time += duration / laces;
      else
        lace_time = GST_CLOCK_TIME_NONE;
    }
  }

done:
  if (buf)
    gst_buffer_unref (buf);
  g_free (lace_size);

  return ret;

  /* EXITS */
eos:
  {
    stream->eos = TRUE;
    ret = GST_FLOW_OK;
    /* combine flows */
    ret = gst_matroska_demux_combine_flows (demux, stream, ret);
    goto done;
  }
invalid_lacing:
  {
    GST_ELEMENT_WARNING (demux, STREAM, DEMUX, (NULL), ("Invalid lacing size"));
    /* non-fatal, try next block(group) */
    ret = GST_FLOW_OK;
    goto done;
  }
data_error:
  {
    GST_ELEMENT_WARNING (demux, STREAM, DEMUX, (NULL), ("Data error"));
    /* non-fatal, try next block(group) */
    ret = GST_FLOW_OK;
    goto done;
  }
}

/* return FALSE if block(group) should be skipped (due to a seek) */
static inline gboolean
gst_matroska_demux_seek_block (GstMatroskaDemux * demux)
{
  if (G_UNLIKELY (demux->seek_block)) {
    if (!(--demux->seek_block)) {
      return TRUE;
    } else {
      GST_LOG_OBJECT (demux, "should skip block due to seek");
      return FALSE;
    }
  } else {
    return TRUE;
  }
}

static GstFlowReturn
gst_matroska_demux_parse_contents_seekentry (GstMatroskaDemux * demux,
    GstEbmlRead * ebml)
{
  GstFlowReturn ret;
  guint64 seek_pos = (guint64) - 1;
  guint32 seek_id = 0;
  guint32 id;

  DEBUG_ELEMENT_START (demux, ebml, "Seek");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "Seek", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_SEEKID:
      {
        guint64 t;

        if ((ret = gst_ebml_read_uint (ebml, &id, &t)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (demux, "SeekID: %" G_GUINT64_FORMAT, t);
        seek_id = t;
        break;
      }

      case GST_MATROSKA_ID_SEEKPOSITION:
      {
        guint64 t;

        if ((ret = gst_ebml_read_uint (ebml, &id, &t)) != GST_FLOW_OK)
          break;

        if (t > G_MAXINT64) {
          GST_WARNING_OBJECT (demux,
              "Too large SeekPosition %" G_GUINT64_FORMAT, t);
          break;
        }

        GST_DEBUG_OBJECT (demux, "SeekPosition: %" G_GUINT64_FORMAT, t);
        seek_pos = t;
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common, ebml,
            "SeekHead", id);
        break;
    }
  }

  if (ret != GST_FLOW_OK && ret != GST_FLOW_UNEXPECTED)
    return ret;

  if (!seek_id || seek_pos == (guint64) - 1) {
    GST_WARNING_OBJECT (demux, "Incomplete seekhead entry (0x%x/%"
        G_GUINT64_FORMAT ")", seek_id, seek_pos);
    return GST_FLOW_OK;
  }

  switch (seek_id) {
    case GST_MATROSKA_ID_SEEKHEAD:
    {
    }
    case GST_MATROSKA_ID_CUES:
    case GST_MATROSKA_ID_TAGS:
    case GST_MATROSKA_ID_TRACKS:
    case GST_MATROSKA_ID_SEGMENTINFO:
    case GST_MATROSKA_ID_ATTACHMENTS:
    case GST_MATROSKA_ID_CHAPTERS:
    {
      guint64 before_pos, length;
      guint needed;

      /* remember */
      length = gst_matroska_read_common_get_length (&demux->common);
      before_pos = demux->common.offset;

      if (length == (guint64) - 1) {
        GST_DEBUG_OBJECT (demux, "no upstream length, skipping SeakHead entry");
        break;
      }

      /* check for validity */
      if (seek_pos + demux->common.ebml_segment_start + 12 >= length) {
        GST_WARNING_OBJECT (demux,
            "SeekHead reference lies outside file!" " (%"
            G_GUINT64_FORMAT "+%" G_GUINT64_FORMAT "+12 >= %"
            G_GUINT64_FORMAT ")", seek_pos, demux->common.ebml_segment_start,
            length);
        break;
      }

      /* only pick up index location when streaming */
      if (demux->streaming) {
        if (seek_id == GST_MATROSKA_ID_CUES) {
          demux->index_offset = seek_pos + demux->common.ebml_segment_start;
          GST_DEBUG_OBJECT (demux, "Cues located at offset %" G_GUINT64_FORMAT,
              demux->index_offset);
        }
        break;
      }

      /* seek */
      demux->common.offset = seek_pos + demux->common.ebml_segment_start;

      /* check ID */
      if ((ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
                  GST_ELEMENT_CAST (demux), &id, &length, &needed)) !=
          GST_FLOW_OK)
        goto finish;

      if (id != seek_id) {
        GST_WARNING_OBJECT (demux,
            "We looked for ID=0x%x but got ID=0x%x (pos=%" G_GUINT64_FORMAT ")",
            seek_id, id, seek_pos + demux->common.ebml_segment_start);
      } else {
        /* now parse */
        ret = gst_matroska_demux_parse_id (demux, id, length, needed);
      }

    finish:
      /* seek back */
      demux->common.offset = before_pos;
      break;
    }

    case GST_MATROSKA_ID_CLUSTER:
    {
      guint64 pos = seek_pos + demux->common.ebml_segment_start;

      GST_LOG_OBJECT (demux, "Cluster position");
      if (G_UNLIKELY (!demux->clusters))
        demux->clusters = g_array_sized_new (TRUE, TRUE, sizeof (guint64), 100);
      g_array_append_val (demux->clusters, pos);
      break;
    }

    default:
      GST_DEBUG_OBJECT (demux, "Ignoring Seek entry for ID=0x%x", seek_id);
      break;
  }
  DEBUG_ELEMENT_STOP (demux, ebml, "Seek", ret);

  return ret;
}

static GstFlowReturn
gst_matroska_demux_parse_contents (GstMatroskaDemux * demux, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;

  DEBUG_ELEMENT_START (demux, ebml, "SeekHead");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (demux, ebml, "SeekHead", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_SEEKENTRY:
      {
        ret = gst_matroska_demux_parse_contents_seekentry (demux, ebml);
        /* Ignore EOS and errors here */
        if (ret != GST_FLOW_OK) {
          GST_DEBUG_OBJECT (demux, "Ignoring %s", gst_flow_get_name (ret));
          ret = GST_FLOW_OK;
        }
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (&demux->common,
            ebml, "SeekHead", id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (demux, ebml, "SeekHead", ret);

  /* Sort clusters by position for easier searching */
  if (demux->clusters)
    g_array_sort (demux->clusters, (GCompareFunc) gst_matroska_cluster_compare);

  return ret;
}

#define GST_FLOW_OVERFLOW   GST_FLOW_CUSTOM_ERROR

#define MAX_BLOCK_SIZE (15 * 1024 * 1024)

static inline GstFlowReturn
gst_matroska_demux_check_read_size (GstMatroskaDemux * demux, guint64 bytes)
{
  if (G_UNLIKELY (bytes > MAX_BLOCK_SIZE)) {
    /* only a few blocks are expected/allowed to be large,
     * and will be recursed into, whereas others will be read and must fit */
    if (demux->streaming) {
      /* fatal in streaming case, as we can't step over easily */
      GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
          ("reading large block of size %" G_GUINT64_FORMAT " not supported; "
              "file might be corrupt.", bytes));
      return GST_FLOW_ERROR;
    } else {
      /* indicate higher level to quietly give up */
      GST_DEBUG_OBJECT (demux,
          "too large block of size %" G_GUINT64_FORMAT, bytes);
      return GST_FLOW_ERROR;
    }
  } else {
    return GST_FLOW_OK;
  }
}

/* returns TRUE if we truely are in error state, and should give up */
static inline gboolean
gst_matroska_demux_check_parse_error (GstMatroskaDemux * demux)
{
  if (!demux->streaming && demux->next_cluster_offset > 0) {
    /* just repositioning to where next cluster should be and try from there */
    GST_WARNING_OBJECT (demux, "parse error, trying next cluster expected at %"
        G_GUINT64_FORMAT, demux->next_cluster_offset);
    demux->common.offset = demux->next_cluster_offset;
    demux->next_cluster_offset = 0;
    return FALSE;
  } else {
    gint64 pos;

    /* sigh, one last attempt above and beyond call of duty ...;
     * search for cluster mark following current pos */
    pos = demux->common.offset;
    GST_WARNING_OBJECT (demux, "parse error, looking for next cluster");
    if (gst_matroska_demux_search_cluster (demux, &pos) != GST_FLOW_OK) {
      /* did not work, give up */
      return TRUE;
    } else {
      GST_DEBUG_OBJECT (demux, "... found at  %" G_GUINT64_FORMAT, pos);
      /* try that position */
      demux->common.offset = pos;
      return FALSE;
    }
  }
}

static inline GstFlowReturn
gst_matroska_demux_flush (GstMatroskaDemux * demux, guint flush)
{
  GST_LOG_OBJECT (demux, "skipping %d bytes", flush);
  demux->common.offset += flush;
  if (demux->streaming) {
    GstFlowReturn ret;

    /* hard to skip large blocks when streaming */
    ret = gst_matroska_demux_check_read_size (demux, flush);
    if (ret != GST_FLOW_OK)
      return ret;
    if (flush <= gst_adapter_available (demux->common.adapter))
      gst_adapter_flush (demux->common.adapter, flush);
    else
      return GST_FLOW_UNEXPECTED;
  }
  return GST_FLOW_OK;
}

/* initializes @ebml with @bytes from input stream at current offset.
 * Returns UNEXPECTED if insufficient available,
 * ERROR if too much was attempted to read. */
static inline GstFlowReturn
gst_matroska_demux_take (GstMatroskaDemux * demux, guint64 bytes,
    GstEbmlRead * ebml)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG_OBJECT (demux, "taking %" G_GUINT64_FORMAT " bytes for parsing",
      bytes);
  ret = gst_matroska_demux_check_read_size (demux, bytes);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    if (!demux->streaming) {
      /* in pull mode, we can skip */
      if ((ret = gst_matroska_demux_flush (demux, bytes)) == GST_FLOW_OK)
        ret = GST_FLOW_OVERFLOW;
    } else {
      /* otherwise fatal */
      ret = GST_FLOW_ERROR;
    }
    goto exit;
  }
  if (demux->streaming) {
    if (gst_adapter_available (demux->common.adapter) >= bytes)
      buffer = gst_adapter_take_buffer (demux->common.adapter, bytes);
    else
      ret = GST_FLOW_UNEXPECTED;
  } else
    ret = gst_matroska_read_common_peek_bytes (&demux->common,
        demux->common.offset, bytes, &buffer, NULL);
  if (G_LIKELY (buffer)) {
    gst_ebml_read_init (ebml, GST_ELEMENT_CAST (demux), buffer,
        demux->common.offset);
    demux->common.offset += bytes;
  }
exit:
  return ret;
}

static void
gst_matroska_demux_check_seekability (GstMatroskaDemux * demux)
{
  GstQuery *query;
  gboolean seekable = FALSE;
  gint64 start = -1, stop = -1;

  query = gst_query_new_seeking (GST_FORMAT_BYTES);
  if (!gst_pad_peer_query (demux->common.sinkpad, query)) {
    GST_DEBUG_OBJECT (demux, "seeking query failed");
    goto done;
  }

  gst_query_parse_seeking (query, NULL, &seekable, &start, &stop);

  /* try harder to query upstream size if we didn't get it the first time */
  if (seekable && stop == -1) {
    GstFormat fmt = GST_FORMAT_BYTES;

    GST_DEBUG_OBJECT (demux, "doing duration query to fix up unset stop");
    gst_pad_query_peer_duration (demux->common.sinkpad, &fmt, &stop);
  }

  /* if upstream doesn't know the size, it's likely that it's not seekable in
   * practice even if it technically may be seekable */
  if (seekable && (start != 0 || stop <= start)) {
    GST_DEBUG_OBJECT (demux, "seekable but unknown start/stop -> disable");
    seekable = FALSE;
  }

done:
  GST_INFO_OBJECT (demux, "seekable: %d (%" G_GUINT64_FORMAT " - %"
      G_GUINT64_FORMAT ")", seekable, start, stop);
  demux->seekable = seekable;

  gst_query_unref (query);
}

static GstFlowReturn
gst_matroska_demux_find_tracks (GstMatroskaDemux * demux)
{
  guint32 id;
  guint64 before_pos;
  guint64 length;
  guint needed;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_WARNING_OBJECT (demux,
      "Found Cluster element before Tracks, searching Tracks");

  /* remember */
  before_pos = demux->common.offset;

  /* Search Tracks element */
  while (TRUE) {
    ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
        GST_ELEMENT_CAST (demux), &id, &length, &needed);
    if (ret != GST_FLOW_OK)
      break;

    if (id != GST_MATROSKA_ID_TRACKS) {
      /* we may be skipping large cluster here, so forego size check etc */
      /* ... but we can't skip undefined size; force error */
      if (length == G_MAXUINT64) {
        ret = gst_matroska_demux_check_read_size (demux, length);
        break;
      } else {
        demux->common.offset += needed;
        demux->common.offset += length;
      }
      continue;
    }

    /* will lead to track parsing ... */
    ret = gst_matroska_demux_parse_id (demux, id, length, needed);
    break;
  }

  /* seek back */
  demux->common.offset = before_pos;

  return ret;
}

#define GST_READ_CHECK(stmt)  \
G_STMT_START { \
  if (G_UNLIKELY ((ret = (stmt)) != GST_FLOW_OK)) { \
    if (ret == GST_FLOW_OVERFLOW) { \
      ret = GST_FLOW_OK; \
    } \
    goto read_error; \
  } \
} G_STMT_END

static GstFlowReturn
gst_matroska_demux_parse_id (GstMatroskaDemux * demux, guint32 id,
    guint64 length, guint needed)
{
  GstEbmlRead ebml = { 0, };
  GstFlowReturn ret = GST_FLOW_OK;
  guint64 read;

  GST_LOG_OBJECT (demux, "Parsing Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", prefix %d", id, length, needed);

  /* if we plan to read and parse this element, we need prefix (id + length)
   * and the contents */
  /* mind about overflow wrap-around when dealing with undefined size */
  read = length;
  if (G_LIKELY (length != G_MAXUINT64))
    read += needed;

  switch (demux->common.state) {
    case GST_MATROSKA_READ_STATE_START:
      switch (id) {
        case GST_EBML_ID_HEADER:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_header (&demux->common, &ebml);
          if (ret != GST_FLOW_OK)
            goto parse_failed;
          demux->common.state = GST_MATROSKA_READ_STATE_SEGMENT;
          gst_matroska_demux_check_seekability (demux);
          break;
        default:
          goto invalid_header;
          break;
      }
      break;
    case GST_MATROSKA_READ_STATE_SEGMENT:
      switch (id) {
        case GST_MATROSKA_ID_SEGMENT:
          /* eat segment prefix */
          GST_READ_CHECK (gst_matroska_demux_flush (demux, needed));
          GST_DEBUG_OBJECT (demux,
              "Found Segment start at offset %" G_GUINT64_FORMAT,
              demux->common.offset);
          /* seeks are from the beginning of the segment,
           * after the segment ID/length */
          demux->common.ebml_segment_start = demux->common.offset;
          demux->common.state = GST_MATROSKA_READ_STATE_HEADER;
          break;
        default:
          GST_WARNING_OBJECT (demux,
              "Expected a Segment ID (0x%x), but received 0x%x!",
              GST_MATROSKA_ID_SEGMENT, id);
          GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          break;
      }
      break;
    case GST_MATROSKA_READ_STATE_SCANNING:
      if (id != GST_MATROSKA_ID_CLUSTER &&
          id != GST_MATROSKA_ID_CLUSTERTIMECODE)
        goto skip;
      /* fall-through */
    case GST_MATROSKA_READ_STATE_HEADER:
    case GST_MATROSKA_READ_STATE_DATA:
    case GST_MATROSKA_READ_STATE_SEEK:
      switch (id) {
        case GST_MATROSKA_ID_SEGMENTINFO:
          if (!demux->common.segmentinfo_parsed) {
            GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
            ret = gst_matroska_read_common_parse_info (&demux->common,
                GST_ELEMENT_CAST (demux), &ebml);
          } else {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          }
          break;
        case GST_MATROSKA_ID_TRACKS:
          if (!demux->tracks_parsed) {
            GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
            ret = gst_matroska_demux_parse_tracks (demux, &ebml);
          } else {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          }
          break;
        case GST_MATROSKA_ID_CLUSTER:
          if (G_UNLIKELY (!demux->tracks_parsed)) {
            if (demux->streaming) {
              GST_DEBUG_OBJECT (demux, "Cluster before Track");
              goto not_streamable;
            } else {
              ret = gst_matroska_demux_find_tracks (demux);
              if (!demux->tracks_parsed)
                goto no_tracks;
            }
          }
          if (G_UNLIKELY (demux->common.state
                  == GST_MATROSKA_READ_STATE_HEADER)) {
            demux->common.state = GST_MATROSKA_READ_STATE_DATA;
            demux->first_cluster_offset = demux->common.offset;
            GST_DEBUG_OBJECT (demux, "signaling no more pads");
            gst_element_no_more_pads (GST_ELEMENT (demux));
            /* send initial newsegment - we wait till we know the first
               incoming timestamp, so we can properly set the start of
               the segment. */
            demux->need_newsegment = TRUE;
          }
          demux->cluster_time = GST_CLOCK_TIME_NONE;
          demux->cluster_offset = demux->common.offset;
          if (G_UNLIKELY (!demux->seek_first && demux->seek_block)) {
            GST_DEBUG_OBJECT (demux, "seek target block %" G_GUINT64_FORMAT
                " not found in Cluster, trying next Cluster's first block instead",
                demux->seek_block);
            demux->seek_block = 0;
          }
          demux->seek_first = FALSE;
          /* record next cluster for recovery */
          if (read != G_MAXUINT64)
            demux->next_cluster_offset = demux->cluster_offset + read;
          /* eat cluster prefix */
          gst_matroska_demux_flush (demux, needed);
          break;
        case GST_MATROSKA_ID_CLUSTERTIMECODE:
        {
          guint64 num;

          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          if ((ret = gst_ebml_read_uint (&ebml, &id, &num)) != GST_FLOW_OK)
            goto parse_failed;
          GST_DEBUG_OBJECT (demux, "ClusterTimeCode: %" G_GUINT64_FORMAT, num);
          demux->cluster_time = num;
          if (demux->common.element_index) {
            if (demux->common.element_index_writer_id == -1)
              gst_index_get_writer_id (demux->common.element_index,
                  GST_OBJECT (demux), &demux->common.element_index_writer_id);
            GST_LOG_OBJECT (demux, "adding association %" GST_TIME_FORMAT "-> %"
                G_GUINT64_FORMAT " for writer id %d",
                GST_TIME_ARGS (demux->cluster_time), demux->cluster_offset,
                demux->common.element_index_writer_id);
            gst_index_add_association (demux->common.element_index,
                demux->common.element_index_writer_id,
                GST_ASSOCIATION_FLAG_KEY_UNIT,
                GST_FORMAT_TIME, demux->cluster_time,
                GST_FORMAT_BYTES, demux->cluster_offset, NULL);
          }
          break;
        }
        case GST_MATROSKA_ID_BLOCKGROUP:
          if (!gst_matroska_demux_seek_block (demux))
            goto skip;
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          DEBUG_ELEMENT_START (demux, &ebml, "BlockGroup");
          if ((ret = gst_ebml_read_master (&ebml, &id)) == GST_FLOW_OK) {
            ret = gst_matroska_demux_parse_blockgroup_or_simpleblock (demux,
                &ebml, demux->cluster_time, demux->cluster_offset, FALSE);
          }
          DEBUG_ELEMENT_STOP (demux, &ebml, "BlockGroup", ret);
          break;
        case GST_MATROSKA_ID_SIMPLEBLOCK:
          if (!gst_matroska_demux_seek_block (demux))
            goto skip;
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          DEBUG_ELEMENT_START (demux, &ebml, "SimpleBlock");
          ret = gst_matroska_demux_parse_blockgroup_or_simpleblock (demux,
              &ebml, demux->cluster_time, demux->cluster_offset, TRUE);
          DEBUG_ELEMENT_STOP (demux, &ebml, "SimpleBlock", ret);
          break;
        case GST_MATROSKA_ID_ATTACHMENTS:
          if (!demux->common.attachments_parsed) {
            GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
            ret = gst_matroska_read_common_parse_attachments (&demux->common,
                GST_ELEMENT_CAST (demux), &ebml);
          } else {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          }
          break;
        case GST_MATROSKA_ID_TAGS:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_metadata (&demux->common,
              GST_ELEMENT_CAST (demux), &ebml);
          break;
        case GST_MATROSKA_ID_CHAPTERS:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_chapters (&demux->common, &ebml);
          break;
        case GST_MATROSKA_ID_SEEKHEAD:
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_demux_parse_contents (demux, &ebml);
          break;
        case GST_MATROSKA_ID_CUES:
          if (demux->common.index_parsed) {
            GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
            break;
          }
          GST_READ_CHECK (gst_matroska_demux_take (demux, read, &ebml));
          ret = gst_matroska_read_common_parse_index (&demux->common, &ebml);
          /* only push based; delayed index building */
          if (ret == GST_FLOW_OK
              && demux->common.state == GST_MATROSKA_READ_STATE_SEEK) {
            GstEvent *event;

            GST_OBJECT_LOCK (demux);
            event = demux->seek_event;
            demux->seek_event = NULL;
            GST_OBJECT_UNLOCK (demux);

            g_assert (event);
            /* unlikely to fail, since we managed to seek to this point */
            if (!gst_matroska_demux_handle_seek_event (demux, NULL, event))
              goto seek_failed;
            /* resume data handling, main thread clear to seek again */
            GST_OBJECT_LOCK (demux);
            demux->common.state = GST_MATROSKA_READ_STATE_DATA;
            GST_OBJECT_UNLOCK (demux);
          }
          break;
        case GST_MATROSKA_ID_POSITION:
        case GST_MATROSKA_ID_PREVSIZE:
        case GST_MATROSKA_ID_ENCRYPTEDBLOCK:
        case GST_MATROSKA_ID_SILENTTRACKS:
          GST_DEBUG_OBJECT (demux,
              "Skipping Cluster subelement 0x%x - ignoring", id);
          /* fall-through */
        default:
        skip:
          GST_DEBUG_OBJECT (demux, "skipping Element 0x%x", id);
          GST_READ_CHECK (gst_matroska_demux_flush (demux, read));
          break;
      }
      break;
  }

  if (ret == GST_FLOW_PARSE)
    goto parse_failed;

exit:
  gst_ebml_read_clear (&ebml);
  return ret;

  /* ERRORS */
read_error:
  {
    /* simply exit, maybe not enough data yet */
    /* no ebml to clear if read error */
    return ret;
  }
parse_failed:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("Failed to parse Element 0x%x", id));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
not_streamable:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("File layout does not permit streaming"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
no_tracks:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL),
        ("No Tracks element found"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
invalid_header:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("Invalid header"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
seek_failed:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("Failed to seek"));
    ret = GST_FLOW_ERROR;
    goto exit;
  }
}

static void
gst_matroska_demux_loop (GstPad * pad)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (GST_PAD_PARENT (pad));
  GstFlowReturn ret;
  guint32 id;
  guint64 length;
  guint needed;

  /* If we have to close a segment, send a new segment to do this now */
  if (G_LIKELY (demux->common.state == GST_MATROSKA_READ_STATE_DATA)) {
    if (G_UNLIKELY (demux->close_segment)) {
      gst_matroska_demux_send_event (demux, demux->close_segment);
      demux->close_segment = NULL;
    }
    if (G_UNLIKELY (demux->new_segment)) {
      gst_matroska_demux_send_event (demux, demux->new_segment);
      demux->new_segment = NULL;
    }
  }

  ret = gst_matroska_read_common_peek_id_length_pull (&demux->common,
      GST_ELEMENT_CAST (demux), &id, &length, &needed);
  if (ret == GST_FLOW_UNEXPECTED)
    goto eos;
  if (ret != GST_FLOW_OK) {
    if (gst_matroska_demux_check_parse_error (demux))
      goto pause;
    else
      return;
  }

  GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", needed %d", demux->common.offset, id,
      length, needed);

  ret = gst_matroska_demux_parse_id (demux, id, length, needed);
  if (ret == GST_FLOW_UNEXPECTED)
    goto eos;
  if (ret != GST_FLOW_OK)
    goto pause;

  /* check if we're at the end of a configured segment */
  if (G_LIKELY (demux->common.src->len)) {
    guint i;

    g_assert (demux->common.num_streams == demux->common.src->len);
    for (i = 0; i < demux->common.src->len; i++) {
      GstMatroskaTrackContext *context = g_ptr_array_index (demux->common.src,
          i);
      GST_DEBUG_OBJECT (context->pad, "pos %" GST_TIME_FORMAT,
          GST_TIME_ARGS (context->pos));
      if (context->eos == FALSE)
        goto next;
    }

    GST_INFO_OBJECT (demux, "All streams are EOS");
    ret = GST_FLOW_UNEXPECTED;
    goto eos;
  }

next:
  if (G_UNLIKELY (demux->common.offset ==
          gst_matroska_read_common_get_length (&demux->common))) {
    GST_LOG_OBJECT (demux, "Reached end of stream");
    ret = GST_FLOW_UNEXPECTED;
    goto eos;
  }

  return;

  /* ERRORS */
eos:
  {
    if (demux->common.segment.rate < 0.0) {
      ret = gst_matroska_demux_seek_to_previous_keyframe (demux);
      if (ret == GST_FLOW_OK)
        return;
    }
    /* fall-through */
  }
pause:
  {
    const gchar *reason = gst_flow_get_name (ret);
    gboolean push_eos = FALSE;

    GST_LOG_OBJECT (demux, "pausing task, reason %s", reason);
    demux->segment_running = FALSE;
    gst_pad_pause_task (demux->common.sinkpad);

    if (ret == GST_FLOW_UNEXPECTED) {
      /* perform EOS logic */

      /* If we were in the headers, make sure we send no-more-pads.
         This will ensure decodebin2 does not get stuck thinking
         the chain is not complete yet, and waiting indefinitely. */
      if (G_UNLIKELY (demux->common.state == GST_MATROSKA_READ_STATE_HEADER)) {
        if (demux->common.src->len == 0) {
          GST_ELEMENT_ERROR (demux, STREAM, FAILED, (NULL),
              ("No pads created"));
        } else {
          GST_ELEMENT_WARNING (demux, STREAM, DEMUX, (NULL),
              ("Failed to finish reading headers"));
        }
        gst_element_no_more_pads (GST_ELEMENT (demux));
      }

      /* Close the segment, i.e. update segment stop with the duration
       * if no stop was set */
      if (GST_CLOCK_TIME_IS_VALID (demux->last_stop_end) &&
          !GST_CLOCK_TIME_IS_VALID (demux->common.segment.stop) &&
          GST_CLOCK_TIME_IS_VALID (demux->common.segment.start) &&
          demux->last_stop_end > demux->common.segment.start) {
        /* arrange to accumulate duration downstream, but avoid sending
         * newsegment with decreasing start (w.r.t. sync newsegment events) */
        GstEvent *event =
            gst_event_new_new_segment_full (TRUE, demux->common.segment.rate,
            demux->common.segment.applied_rate, demux->common.segment.format,
            demux->last_stop_end, demux->last_stop_end,
            demux->common.segment.time + (demux->last_stop_end -
                demux->common.segment.start));
        gst_matroska_demux_send_event (demux, event);
      }

      if (demux->common.segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gint64 stop;

        /* for segment playback we need to post when (in stream time)
         * we stopped, this is either stop (when set) or the duration. */
        if ((stop = demux->common.segment.stop) == -1)
          stop = demux->last_stop_end;

        GST_LOG_OBJECT (demux, "Sending segment done, at end of segment");
        gst_element_post_message (GST_ELEMENT (demux),
            gst_message_new_segment_done (GST_OBJECT (demux), GST_FORMAT_TIME,
                stop));
      } else {
        push_eos = TRUE;
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_UNEXPECTED) {
      /* for fatal errors we post an error message */
      GST_ELEMENT_ERROR (demux, STREAM, FAILED, (NULL),
          ("stream stopped, reason %s", reason));
      push_eos = TRUE;
    }
    if (push_eos) {
      /* send EOS, and prevent hanging if no streams yet */
      GST_LOG_OBJECT (demux, "Sending EOS, at end of stream");
      if (!gst_matroska_demux_send_event (demux, gst_event_new_eos ()) &&
          (ret == GST_FLOW_UNEXPECTED)) {
        GST_ELEMENT_ERROR (demux, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      }
    }
    return;
  }
}

/*
 * Create and push a flushing seek event upstream
 */
static gboolean
perform_seek_to_offset (GstMatroskaDemux * demux, guint64 offset)
{
  GstEvent *event;
  gboolean res = 0;

  GST_DEBUG_OBJECT (demux, "Seeking to %" G_GUINT64_FORMAT, offset);

  event =
      gst_event_new_seek (1.0, GST_FORMAT_BYTES,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, offset,
      GST_SEEK_TYPE_NONE, -1);

  res = gst_pad_push_event (demux->common.sinkpad, event);

  /* newsegment event will update offset */
  return res;
}

static GstFlowReturn
gst_matroska_demux_chain (GstPad * pad, GstBuffer * buffer)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (GST_PAD_PARENT (pad));
  guint available;
  GstFlowReturn ret = GST_FLOW_OK;
  guint needed = 0;
  guint32 id;
  guint64 length;

  if (G_UNLIKELY (GST_BUFFER_IS_DISCONT (buffer))) {
    GST_DEBUG_OBJECT (demux, "got DISCONT");
    gst_adapter_clear (demux->common.adapter);
    GST_OBJECT_LOCK (demux);
    gst_matroska_read_common_reset_streams (&demux->common,
        GST_CLOCK_TIME_NONE, FALSE);
    GST_OBJECT_UNLOCK (demux);
  }

  gst_adapter_push (demux->common.adapter, buffer);
  buffer = NULL;

next:
  available = gst_adapter_available (demux->common.adapter);

  ret = gst_matroska_read_common_peek_id_length_push (&demux->common,
      GST_ELEMENT_CAST (demux), &id, &length, &needed);
  if (G_UNLIKELY (ret != GST_FLOW_OK && ret != GST_FLOW_UNEXPECTED))
    return ret;

  GST_LOG_OBJECT (demux, "Offset %" G_GUINT64_FORMAT ", Element id 0x%x, "
      "size %" G_GUINT64_FORMAT ", needed %d, available %d",
      demux->common.offset, id, length, needed, available);

  if (needed > available)
    return GST_FLOW_OK;

  ret = gst_matroska_demux_parse_id (demux, id, length, needed);
  if (ret == GST_FLOW_UNEXPECTED) {
    /* need more data */
    return GST_FLOW_OK;
  } else if (ret != GST_FLOW_OK) {
    return ret;
  } else
    goto next;
}

static gboolean
gst_matroska_demux_handle_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean res = TRUE;
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (GST_PAD_PARENT (pad));

  GST_DEBUG_OBJECT (demux,
      "have event type %s: %p on sink pad", GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NEWSEGMENT:
    {
      GstFormat format;
      gdouble rate, arate;
      gint64 start, stop, time = 0;
      gboolean update;
      GstSegment segment;

      /* some debug output */
      gst_segment_init (&segment, GST_FORMAT_UNDEFINED);
      gst_event_parse_new_segment_full (event, &update, &rate, &arate, &format,
          &start, &stop, &time);
      gst_segment_set_newsegment_full (&segment, update, rate, arate, format,
          start, stop, time);
      GST_DEBUG_OBJECT (demux,
          "received format %d newsegment %" GST_SEGMENT_FORMAT, format,
          &segment);

      if (demux->common.state < GST_MATROSKA_READ_STATE_DATA) {
        GST_DEBUG_OBJECT (demux, "still starting");
        goto exit;
      }

      /* we only expect a BYTE segment, e.g. following a seek */
      if (format != GST_FORMAT_BYTES) {
        GST_DEBUG_OBJECT (demux, "unsupported segment format, ignoring");
        goto exit;
      }

      GST_DEBUG_OBJECT (demux, "clearing segment state");
      GST_OBJECT_LOCK (demux);
      /* clear current segment leftover */
      gst_adapter_clear (demux->common.adapter);
      /* and some streaming setup */
      demux->common.offset = start;
      /* do not know where we are;
       * need to come across a cluster and generate newsegment */
      demux->common.segment.last_stop = GST_CLOCK_TIME_NONE;
      demux->cluster_time = GST_CLOCK_TIME_NONE;
      demux->cluster_offset = 0;
      demux->need_newsegment = TRUE;
      /* but keep some of the upstream segment */
      demux->common.segment.rate = rate;
      GST_OBJECT_UNLOCK (demux);
    exit:
      /* chain will send initial newsegment after pads have been added,
       * or otherwise come up with one */
      GST_DEBUG_OBJECT (demux, "eating event");
      gst_event_unref (event);
      res = TRUE;
      break;
    }
    case GST_EVENT_EOS:
    {
      if (demux->common.state != GST_MATROSKA_READ_STATE_DATA) {
        gst_event_unref (event);
        GST_ELEMENT_ERROR (demux, STREAM, DEMUX,
            (NULL), ("got eos and didn't receive a complete header object"));
      } else if (demux->common.num_streams == 0) {
        GST_ELEMENT_ERROR (demux, STREAM, DEMUX,
            (NULL), ("got eos but no streams (yet)"));
      } else {
        gst_matroska_demux_send_event (demux, event);
      }
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      gst_adapter_clear (demux->common.adapter);
      GST_OBJECT_LOCK (demux);
      gst_matroska_read_common_reset_streams (&demux->common,
          GST_CLOCK_TIME_NONE, TRUE);
      demux->common.segment.last_stop = GST_CLOCK_TIME_NONE;
      demux->cluster_time = GST_CLOCK_TIME_NONE;
      demux->cluster_offset = 0;
      GST_OBJECT_UNLOCK (demux);
      /* fall-through */
    }
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  return res;
}

static gboolean
gst_matroska_demux_sink_activate (GstPad * sinkpad)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (GST_PAD_PARENT (sinkpad));

  if (gst_pad_check_pull_range (sinkpad)) {
    GST_DEBUG ("going to pull mode");
    demux->streaming = FALSE;
    return gst_pad_activate_pull (sinkpad, TRUE);
  } else {
    GST_DEBUG ("going to push (streaming) mode");
    demux->streaming = TRUE;
    return gst_pad_activate_push (sinkpad, TRUE);
  }

  return FALSE;
}

static gboolean
gst_matroska_demux_sink_activate_pull (GstPad * sinkpad, gboolean active)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (GST_PAD_PARENT (sinkpad));

  if (active) {
    /* if we have a scheduler we can start the task */
    demux->segment_running = TRUE;
    gst_pad_start_task (sinkpad, (GstTaskFunction) gst_matroska_demux_loop,
        sinkpad);
  } else {
    demux->segment_running = FALSE;
    gst_pad_stop_task (sinkpad);
  }

  return TRUE;
}

static void
gst_duration_to_fraction (guint64 duration, gint * dest_n, gint * dest_d)
{
  static const int common_den[] = { 1, 2, 3, 4, 1001 };
  int n, d;
  int i;
  guint64 a;

  for (i = 0; i < G_N_ELEMENTS (common_den); i++) {
    d = common_den[i];
    n = floor (0.5 + (d * 1e9) / duration);
    a = gst_util_uint64_scale_int (1000000000, d, n);
    if (duration >= a - 1 && duration <= a + 1) {
      goto out;
    }
  }

  gst_util_double_to_fraction (1e9 / duration, &n, &d);

out:
  /* set results */
  *dest_n = n;
  *dest_d = d;
}

static GstCaps *
gst_matroska_demux_video_caps (GstMatroskaTrackVideoContext *
    videocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint32 * riff_fourcc)
{
  GstMatroskaTrackContext *context = (GstMatroskaTrackContext *) videocontext;
  GstCaps *caps = NULL;

  g_assert (videocontext != NULL);
  g_assert (codec_name != NULL);

  context->send_xiph_headers = FALSE;
  context->send_flac_headers = FALSE;
  context->send_speex_headers = FALSE;

  if (riff_fourcc)
    *riff_fourcc = 0;

  /* TODO: check if we have all codec types from matroska-ids.h
   *       check if we have to do more special things with codec_private
   *
   * Add support for
   *  GST_MATROSKA_CODEC_ID_VIDEO_QUICKTIME
   *  GST_MATROSKA_CODEC_ID_VIDEO_SNOW
   */

  if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VFW_FOURCC)) {
    gst_riff_strf_vids *vids = NULL;

    if (data) {
      GstBuffer *buf = NULL;

      vids = (gst_riff_strf_vids *) data;

      /* assure size is big enough */
      if (size < 24) {
        GST_WARNING ("Too small BITMAPINFOHEADER (%d bytes)", size);
        return NULL;
      }
      if (size < sizeof (gst_riff_strf_vids)) {
        vids = g_new (gst_riff_strf_vids, 1);
        memcpy (vids, data, size);
      }

      /* little-endian -> byte-order */
      vids->size = GUINT32_FROM_LE (vids->size);
      vids->width = GUINT32_FROM_LE (vids->width);
      vids->height = GUINT32_FROM_LE (vids->height);
      vids->planes = GUINT16_FROM_LE (vids->planes);
      vids->bit_cnt = GUINT16_FROM_LE (vids->bit_cnt);
      vids->compression = GUINT32_FROM_LE (vids->compression);
      vids->image_size = GUINT32_FROM_LE (vids->image_size);
      vids->xpels_meter = GUINT32_FROM_LE (vids->xpels_meter);
      vids->ypels_meter = GUINT32_FROM_LE (vids->ypels_meter);
      vids->num_colors = GUINT32_FROM_LE (vids->num_colors);
      vids->imp_colors = GUINT32_FROM_LE (vids->imp_colors);

      if (size > sizeof (gst_riff_strf_vids)) { /* some extra_data */
        buf = gst_buffer_new_and_alloc (size - sizeof (gst_riff_strf_vids));
        memcpy (GST_BUFFER_DATA (buf),
            (guint8 *) vids + sizeof (gst_riff_strf_vids),
            GST_BUFFER_SIZE (buf));
      }

      if (riff_fourcc)
        *riff_fourcc = vids->compression;

      caps = gst_riff_create_video_caps (vids->compression, NULL, vids,
          buf, NULL, codec_name);

      if (caps == NULL) {
        GST_WARNING ("Unhandled RIFF fourcc %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (vids->compression));
      }

      if (buf)
        gst_buffer_unref (buf);

      if (vids != (gst_riff_strf_vids *) data)
        g_free (vids);
    }
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_UNCOMPRESSED)) {
    guint32 fourcc = 0;

    switch (videocontext->fourcc) {
      case GST_MAKE_FOURCC ('I', '4', '2', '0'):
        *codec_name = g_strdup ("Raw planar YUV 4:2:0");
        fourcc = videocontext->fourcc;
        break;
      case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
        *codec_name = g_strdup ("Raw packed YUV 4:2:2");
        fourcc = videocontext->fourcc;
        break;
      case GST_MAKE_FOURCC ('Y', 'V', '1', '2'):
        *codec_name = g_strdup ("Raw packed YUV 4:2:0");
        fourcc = videocontext->fourcc;
        break;
      case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
        *codec_name = g_strdup ("Raw packed YUV 4:2:2");
        fourcc = videocontext->fourcc;
        break;
      case GST_MAKE_FOURCC ('A', 'Y', 'U', 'V'):
        *codec_name = g_strdup ("Raw packed YUV 4:4:4 with alpha channel");
        fourcc = videocontext->fourcc;
        break;

      default:
        GST_DEBUG ("Unknown fourcc %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (videocontext->fourcc));
        return NULL;
    }

    caps = gst_caps_new_simple ("video/x-raw-yuv",
        "format", GST_TYPE_FOURCC, fourcc, NULL);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_SP)) {
    caps = gst_caps_new_simple ("video/x-divx",
        "divxversion", G_TYPE_INT, 4, NULL);
    *codec_name = g_strdup ("MPEG-4 simple profile");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_ASP) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AP)) {
#if 0
    caps = gst_caps_new_full (gst_structure_new ("video/x-divx",
            "divxversion", G_TYPE_INT, 5, NULL),
        gst_structure_new ("video/x-xvid", NULL),
        gst_structure_new ("video/mpeg",
            "mpegversion", G_TYPE_INT, 4,
            "systemstream", G_TYPE_BOOLEAN, FALSE, NULL), NULL);
#endif
    caps = gst_caps_new_simple ("video/mpeg",
        "mpegversion", G_TYPE_INT, 4,
        "systemstream", G_TYPE_BOOLEAN, FALSE, NULL);
    if (data) {
      GstBuffer *priv = gst_buffer_new_and_alloc (size);

      memcpy (GST_BUFFER_DATA (priv), data, size);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      gst_buffer_unref (priv);
    }
    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_ASP))
      *codec_name = g_strdup ("MPEG-4 advanced simple profile");
    else
      *codec_name = g_strdup ("MPEG-4 advanced profile");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MSMPEG4V3)) {
#if 0
    caps = gst_caps_new_full (gst_structure_new ("video/x-divx",
            "divxversion", G_TYPE_INT, 3, NULL),
        gst_structure_new ("video/x-msmpeg",
            "msmpegversion", G_TYPE_INT, 43, NULL), NULL);
#endif
    caps = gst_caps_new_simple ("video/x-msmpeg",
        "msmpegversion", G_TYPE_INT, 43, NULL);
    *codec_name = g_strdup ("Microsoft MPEG-4 v.3");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG1) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG2)) {
    gint mpegversion;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG1))
      mpegversion = 1;
    else
      mpegversion = 2;

    caps = gst_caps_new_simple ("video/mpeg",
        "systemstream", G_TYPE_BOOLEAN, FALSE,
        "mpegversion", G_TYPE_INT, mpegversion, NULL);
    *codec_name = g_strdup_printf ("MPEG-%d video", mpegversion);
    context->postprocess_frame = gst_matroska_demux_add_mpeg_seq_header;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MJPEG)) {
    caps = gst_caps_new_simple ("image/jpeg", NULL);
    *codec_name = g_strdup ("Motion-JPEG");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AVC)) {
    caps = gst_caps_new_simple ("video/x-h264", NULL);
    if (data) {
      GstBuffer *priv = gst_buffer_new_and_alloc (size);

      /* First byte is the version, second is the profile indication, and third
       * is the 5 contraint_set_flags and 3 reserved bits. Fourth byte is the
       * level indication. */
      gst_codec_utils_h264_caps_set_level_and_profile (caps, data + 1,
          size - 1);

      memcpy (GST_BUFFER_DATA (priv), data, size);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      gst_buffer_unref (priv);

      gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "avc",
          "alignment", G_TYPE_STRING, "au", NULL);
    } else {
      GST_WARNING ("No codec data found, assuming output is byte-stream");
      gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "byte-stream",
          NULL);
    }
    *codec_name = g_strdup ("H264");
  } else if ((!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO1)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO2)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO3)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO4))) {
    gint rmversion = -1;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO1))
      rmversion = 1;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO2))
      rmversion = 2;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO3))
      rmversion = 3;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO4))
      rmversion = 4;

    caps = gst_caps_new_simple ("video/x-pn-realvideo",
        "rmversion", G_TYPE_INT, rmversion, NULL);
    GST_DEBUG ("data:%p, size:0x%x", data, size);
    /* We need to extract the extradata ! */
    if (data && (size >= 0x22)) {
      GstBuffer *priv;
      guint rformat;
      guint subformat;

      subformat = GST_READ_UINT32_BE (data + 0x1a);
      rformat = GST_READ_UINT32_BE (data + 0x1e);

      priv = gst_buffer_new_and_alloc (size - 0x1a);

      memcpy (GST_BUFFER_DATA (priv), data + 0x1a, size - 0x1a);
      gst_caps_set_simple (caps,
          "codec_data", GST_TYPE_BUFFER, priv,
          "format", G_TYPE_INT, rformat,
          "subformat", G_TYPE_INT, subformat, NULL);
      gst_buffer_unref (priv);

    }
    *codec_name = g_strdup_printf ("RealVideo %d.0", rmversion);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_THEORA)) {
    caps = gst_caps_new_simple ("video/x-theora", NULL);
    context->send_xiph_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_DIRAC)) {
    caps = gst_caps_new_simple ("video/x-dirac", NULL);
    *codec_name = g_strdup_printf ("Dirac");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_VIDEO_VP8)) {
    caps = gst_caps_new_simple ("video/x-vp8", NULL);
    *codec_name = g_strdup_printf ("On2 VP8");
  } else {
    GST_WARNING ("Unknown codec '%s', cannot build Caps", codec_id);
    return NULL;
  }

  if (caps != NULL) {
    int i;
    GstStructure *structure;

    for (i = 0; i < gst_caps_get_size (caps); i++) {
      structure = gst_caps_get_structure (caps, i);

      /* FIXME: use the real unit here! */
      GST_DEBUG ("video size %dx%d, target display size %dx%d (any unit)",
          videocontext->pixel_width,
          videocontext->pixel_height,
          videocontext->display_width, videocontext->display_height);

      /* pixel width and height are the w and h of the video in pixels */
      if (videocontext->pixel_width > 0 && videocontext->pixel_height > 0) {
        gint w = videocontext->pixel_width;
        gint h = videocontext->pixel_height;

        gst_structure_set (structure,
            "width", G_TYPE_INT, w, "height", G_TYPE_INT, h, NULL);
      }

      if (videocontext->display_width > 0 || videocontext->display_height > 0) {
        int n, d;

        if (videocontext->display_width <= 0)
          videocontext->display_width = videocontext->pixel_width;
        if (videocontext->display_height <= 0)
          videocontext->display_height = videocontext->pixel_height;

        /* calculate the pixel aspect ratio using the display and pixel w/h */
        n = videocontext->display_width * videocontext->pixel_height;
        d = videocontext->display_height * videocontext->pixel_width;
        GST_DEBUG ("setting PAR to %d/%d", n, d);
        gst_structure_set (structure, "pixel-aspect-ratio",
            GST_TYPE_FRACTION,
            videocontext->display_width * videocontext->pixel_height,
            videocontext->display_height * videocontext->pixel_width, NULL);
      }

      if (videocontext->default_fps > 0.0) {
        GValue fps_double = { 0, };
        GValue fps_fraction = { 0, };

        g_value_init (&fps_double, G_TYPE_DOUBLE);
        g_value_init (&fps_fraction, GST_TYPE_FRACTION);
        g_value_set_double (&fps_double, videocontext->default_fps);
        g_value_transform (&fps_double, &fps_fraction);

        GST_DEBUG ("using default fps %f", videocontext->default_fps);

        gst_structure_set_value (structure, "framerate", &fps_fraction);
        g_value_unset (&fps_double);
        g_value_unset (&fps_fraction);
      } else if (context->default_duration > 0) {
        int fps_n, fps_d;

        gst_duration_to_fraction (context->default_duration, &fps_n, &fps_d);

        GST_INFO ("using default duration %" G_GUINT64_FORMAT
            " framerate %d/%d", context->default_duration, fps_n, fps_d);

        gst_structure_set (structure, "framerate", GST_TYPE_FRACTION,
            fps_n, fps_d, NULL);
      } else {
        /* sort of a hack to get most codecs to support,
         * even if the default_duration is missing */
        gst_structure_set (structure, "framerate", GST_TYPE_FRACTION,
            25, 1, NULL);
      }

      if (videocontext->parent.flags & GST_MATROSKA_VIDEOTRACK_INTERLACED)
        gst_structure_set (structure, "interlaced", G_TYPE_BOOLEAN, TRUE, NULL);
    }

    gst_caps_do_simplify (caps);
  }

  return caps;
}

/*
 * Some AAC specific code... *sigh*
 * FIXME: maybe we should use '15' and code the sample rate explicitly
 * if the sample rate doesn't match the predefined rates exactly? (tpm)
 */

static gint
aac_rate_idx (gint rate)
{
  if (92017 <= rate)
    return 0;
  else if (75132 <= rate)
    return 1;
  else if (55426 <= rate)
    return 2;
  else if (46009 <= rate)
    return 3;
  else if (37566 <= rate)
    return 4;
  else if (27713 <= rate)
    return 5;
  else if (23004 <= rate)
    return 6;
  else if (18783 <= rate)
    return 7;
  else if (13856 <= rate)
    return 8;
  else if (11502 <= rate)
    return 9;
  else if (9391 <= rate)
    return 10;
  else
    return 11;
}

static gint
aac_profile_idx (const gchar * codec_id)
{
  gint profile;

  if (strlen (codec_id) <= 12)
    profile = 3;
  else if (!strncmp (&codec_id[12], "MAIN", 4))
    profile = 0;
  else if (!strncmp (&codec_id[12], "LC", 2))
    profile = 1;
  else if (!strncmp (&codec_id[12], "SSR", 3))
    profile = 2;
  else
    profile = 3;

  return profile;
}

#define AAC_SYNC_EXTENSION_TYPE 0x02b7

static GstCaps *
gst_matroska_demux_audio_caps (GstMatroskaTrackAudioContext *
    audiocontext, const gchar * codec_id, guint8 * data, guint size,
    gchar ** codec_name, guint16 * riff_audio_fmt)
{
  GstMatroskaTrackContext *context = (GstMatroskaTrackContext *) audiocontext;
  GstCaps *caps = NULL;

  g_assert (audiocontext != NULL);
  g_assert (codec_name != NULL);

  if (riff_audio_fmt)
    *riff_audio_fmt = 0;

  context->send_xiph_headers = FALSE;
  context->send_flac_headers = FALSE;
  context->send_speex_headers = FALSE;

  /* TODO: check if we have all codec types from matroska-ids.h
   *       check if we have to do more special things with codec_private
   *       check if we need bitdepth in different places too
   *       implement channel position magic
   * Add support for:
   *  GST_MATROSKA_CODEC_ID_AUDIO_AC3_BSID9
   *  GST_MATROSKA_CODEC_ID_AUDIO_AC3_BSID10
   *  GST_MATROSKA_CODEC_ID_AUDIO_QUICKTIME_QDMC
   *  GST_MATROSKA_CODEC_ID_AUDIO_QUICKTIME_QDM2
   */

  if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L1) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L2) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L3)) {
    gint layer;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L1))
      layer = 1;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L2))
      layer = 2;
    else
      layer = 3;

    caps = gst_caps_new_simple ("audio/mpeg",
        "mpegversion", G_TYPE_INT, 1, "layer", G_TYPE_INT, layer, NULL);
    *codec_name = g_strdup_printf ("MPEG-1 layer %d", layer);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_BE) ||
      !strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_LE)) {
    gint endianness;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_BE))
      endianness = G_BIG_ENDIAN;
    else
      endianness = G_LITTLE_ENDIAN;

    caps = gst_caps_new_simple ("audio/x-raw-int",
        "width", G_TYPE_INT, audiocontext->bitdepth,
        "depth", G_TYPE_INT, audiocontext->bitdepth,
        "signed", G_TYPE_BOOLEAN, audiocontext->bitdepth != 8,
        "endianness", G_TYPE_INT, endianness, NULL);

    *codec_name = g_strdup_printf ("Raw %d-bit PCM audio",
        audiocontext->bitdepth);
    context->alignment = audiocontext->bitdepth / 8;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_PCM_FLOAT)) {
    caps = gst_caps_new_simple ("audio/x-raw-float",
        "endianness", G_TYPE_INT, G_LITTLE_ENDIAN,
        "width", G_TYPE_INT, audiocontext->bitdepth, NULL);
    *codec_name = g_strdup_printf ("Raw %d-bit floating-point audio",
        audiocontext->bitdepth);
    context->alignment = audiocontext->bitdepth / 8;
  } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AC3,
          strlen (GST_MATROSKA_CODEC_ID_AUDIO_AC3))) {
    caps = gst_caps_new_simple ("audio/x-ac3",
        "framed", G_TYPE_BOOLEAN, TRUE, NULL);
    *codec_name = g_strdup ("AC-3 audio");
  } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_EAC3,
          strlen (GST_MATROSKA_CODEC_ID_AUDIO_EAC3))) {
    caps = gst_caps_new_simple ("audio/x-eac3",
        "framed", G_TYPE_BOOLEAN, TRUE, NULL);
    *codec_name = g_strdup ("E-AC-3 audio");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_DTS)) {
    caps = gst_caps_new_simple ("audio/x-dts", NULL);
    *codec_name = g_strdup ("DTS audio");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_VORBIS)) {
    caps = gst_caps_new_simple ("audio/x-vorbis", NULL);
    context->send_xiph_headers = TRUE;
    /* vorbis decoder does tags */
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_FLAC)) {
    caps = gst_caps_new_simple ("audio/x-flac", NULL);
    context->send_flac_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_SPEEX)) {
    caps = gst_caps_new_simple ("audio/x-speex", NULL);
    context->send_speex_headers = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_ACM)) {
    gst_riff_strf_auds auds;

    if (data) {
      GstBuffer *codec_data = gst_buffer_new ();

      /* little-endian -> byte-order */
      auds.format = GST_READ_UINT16_LE (data);
      auds.channels = GST_READ_UINT16_LE (data + 2);
      auds.rate = GST_READ_UINT32_LE (data + 4);
      auds.av_bps = GST_READ_UINT32_LE (data + 8);
      auds.blockalign = GST_READ_UINT16_LE (data + 12);
      auds.size = GST_READ_UINT16_LE (data + 16);

      /* 18 is the waveformatex size */
      gst_buffer_set_data (codec_data, data + 18, auds.size);

      if (riff_audio_fmt)
        *riff_audio_fmt = auds.format;

      caps = gst_riff_create_audio_caps (auds.format, NULL, &auds, NULL,
          codec_data, codec_name);
      gst_buffer_unref (codec_data);

      if (caps == NULL) {
        GST_WARNING ("Unhandled RIFF audio format 0x%02x", auds.format);
      }
    }
  } else if (g_str_has_prefix (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC)) {
    GstBuffer *priv = NULL;
    gint mpegversion;
    gint rate_idx, profile;
    guint8 *data = NULL;

    /* unspecified AAC profile with opaque private codec data */
    if (strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC) == 0) {
      if (context->codec_priv_size >= 2) {
        guint obj_type, freq_index, explicit_freq_bytes = 0;

        codec_id = GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4;
        mpegversion = 4;
        freq_index = (GST_READ_UINT16_BE (context->codec_priv) & 0x780) >> 7;
        obj_type = (GST_READ_UINT16_BE (context->codec_priv) & 0xF800) >> 11;
        if (freq_index == 15)
          explicit_freq_bytes = 3;
        GST_DEBUG ("obj_type = %u, freq_index = %u", obj_type, freq_index);
        priv = gst_buffer_new_and_alloc (context->codec_priv_size);
        memcpy (GST_BUFFER_DATA (priv), context->codec_priv,
            context->codec_priv_size);
        /* assume SBR if samplerate <= 24kHz */
        if (obj_type == 5 || (freq_index >= 6 && freq_index != 15) ||
            (context->codec_priv_size == (5 + explicit_freq_bytes))) {
          audiocontext->samplerate *= 2;
        }
      } else {
        GST_WARNING ("Opaque A_AAC codec ID, but no codec private data");
        /* this is pretty broken;
         * maybe we need to make up some default private,
         * or maybe ADTS data got dumped in.
         * Let's set up some private data now, and check actual data later */
        /* just try this and see what happens ... */
        codec_id = GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4;
        context->postprocess_frame = gst_matroska_demux_check_aac;
      }
    }

    /* make up decoder-specific data if it is not supplied */
    if (priv == NULL) {
      priv = gst_buffer_new_and_alloc (5);
      data = GST_BUFFER_DATA (priv);
      rate_idx = aac_rate_idx (audiocontext->samplerate);
      profile = aac_profile_idx (codec_id);

      data[0] = ((profile + 1) << 3) | ((rate_idx & 0xE) >> 1);
      data[1] = ((rate_idx & 0x1) << 7) | (audiocontext->channels << 3);
      GST_BUFFER_SIZE (priv) = 2;

      if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG2,
              strlen (GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG2))) {
        mpegversion = 2;
      } else if (!strncmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4,
              strlen (GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4))) {
        mpegversion = 4;

        if (g_strrstr (codec_id, "SBR")) {
          /* HE-AAC (aka SBR AAC) */
          audiocontext->samplerate *= 2;
          rate_idx = aac_rate_idx (audiocontext->samplerate);
          data[2] = AAC_SYNC_EXTENSION_TYPE >> 3;
          data[3] = ((AAC_SYNC_EXTENSION_TYPE & 0x07) << 5) | 5;
          data[4] = (1 << 7) | (rate_idx << 3);
          GST_BUFFER_SIZE (priv) = 5;
        }
      } else {
        gst_buffer_unref (priv);
        priv = NULL;
        GST_ERROR ("Unknown AAC profile and no codec private data");
      }
    }

    if (priv) {
      caps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, mpegversion,
          "framed", G_TYPE_BOOLEAN, TRUE, NULL);
      gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
      *codec_name = g_strdup_printf ("MPEG-%d AAC audio", mpegversion);
      gst_buffer_unref (priv);
    }
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_TTA)) {
    caps = gst_caps_new_simple ("audio/x-tta",
        "width", G_TYPE_INT, audiocontext->bitdepth, NULL);
    *codec_name = g_strdup ("TTA audio");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_WAVPACK4)) {
    caps = gst_caps_new_simple ("audio/x-wavpack",
        "width", G_TYPE_INT, audiocontext->bitdepth,
        "framed", G_TYPE_BOOLEAN, TRUE, NULL);
    *codec_name = g_strdup ("Wavpack audio");
    context->postprocess_frame = gst_matroska_demux_add_wvpk_header;
    audiocontext->wvpk_block_index = 0;
  } else if ((!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4)) ||
      (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_COOK))) {
    gint raversion = -1;

    if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4))
      raversion = 1;
    else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_COOK))
      raversion = 8;
    else
      raversion = 2;

    caps = gst_caps_new_simple ("audio/x-pn-realaudio",
        "raversion", G_TYPE_INT, raversion, NULL);
    /* Extract extra information from caps, mapping varies based on codec */
    if (data && (size >= 0x50)) {
      GstBuffer *priv;
      guint flavor;
      guint packet_size;
      guint height;
      guint leaf_size;
      guint sample_width;
      guint extra_data_size;

      GST_ERROR ("real audio raversion:%d", raversion);
      if (raversion == 8) {
        /* COOK */
        flavor = GST_READ_UINT16_BE (data + 22);
        packet_size = GST_READ_UINT32_BE (data + 24);
        height = GST_READ_UINT16_BE (data + 40);
        leaf_size = GST_READ_UINT16_BE (data + 44);
        sample_width = GST_READ_UINT16_BE (data + 58);
        extra_data_size = GST_READ_UINT32_BE (data + 74);

        GST_ERROR
            ("flavor:%d, packet_size:%d, height:%d, leaf_size:%d, sample_width:%d, extra_data_size:%d",
            flavor, packet_size, height, leaf_size, sample_width,
            extra_data_size);
        gst_caps_set_simple (caps, "flavor", G_TYPE_INT, flavor, "packet_size",
            G_TYPE_INT, packet_size, "height", G_TYPE_INT, height, "leaf_size",
            G_TYPE_INT, leaf_size, "width", G_TYPE_INT, sample_width, NULL);

        if ((size - 78) >= extra_data_size) {
          priv = gst_buffer_new_and_alloc (extra_data_size);
          memcpy (GST_BUFFER_DATA (priv), data + 78, extra_data_size);
          gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, priv, NULL);
          gst_buffer_unref (priv);
        }
      }
    }

    *codec_name = g_strdup_printf ("RealAudio %d.0", raversion);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_SIPR)) {
    caps = gst_caps_new_simple ("audio/x-sipro", NULL);
    *codec_name = g_strdup ("Sipro/ACELP.NET Voice Codec");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_RALF)) {
    caps = gst_caps_new_simple ("audio/x-ralf-mpeg4-generic", NULL);
    *codec_name = g_strdup ("Real Audio Lossless");
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_AUDIO_REAL_ATRC)) {
    caps = gst_caps_new_simple ("audio/x-vnd.sony.atrac3", NULL);
    *codec_name = g_strdup ("Sony ATRAC3");
  } else {
    GST_WARNING ("Unknown codec '%s', cannot build Caps", codec_id);
    return NULL;
  }

  if (caps != NULL) {
    if (audiocontext->samplerate > 0 && audiocontext->channels > 0) {
      gint i;

      for (i = 0; i < gst_caps_get_size (caps); i++) {
        gst_structure_set (gst_caps_get_structure (caps, i),
            "channels", G_TYPE_INT, audiocontext->channels,
            "rate", G_TYPE_INT, audiocontext->samplerate, NULL);
      }
    }

    gst_caps_do_simplify (caps);
  }

  return caps;
}

static GstCaps *
gst_matroska_demux_subtitle_caps (GstMatroskaTrackSubtitleContext *
    subtitlecontext, const gchar * codec_id, gpointer data, guint size)
{
  GstCaps *caps = NULL;
  GstMatroskaTrackContext *context =
      (GstMatroskaTrackContext *) subtitlecontext;

  /* for backwards compatibility */
  if (!g_ascii_strcasecmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_ASCII))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_UTF8;
  else if (!g_ascii_strcasecmp (codec_id, "S_SSA"))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_SSA;
  else if (!g_ascii_strcasecmp (codec_id, "S_ASS"))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_ASS;
  else if (!g_ascii_strcasecmp (codec_id, "S_USF"))
    codec_id = GST_MATROSKA_CODEC_ID_SUBTITLE_USF;

  /* TODO: Add GST_MATROSKA_CODEC_ID_SUBTITLE_BMP support
   * Check if we have to do something with codec_private */
  if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_UTF8)) {
    /* well, plain text simply does not have a lot of markup ... */
    caps = gst_caps_new_simple ("text/x-pango-markup", NULL);
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_SSA)) {
    caps = gst_caps_new_simple ("application/x-ssa", NULL);
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = FALSE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_ASS)) {
    caps = gst_caps_new_simple ("application/x-ass", NULL);
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = FALSE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_USF)) {
    caps = gst_caps_new_simple ("application/x-usf", NULL);
    context->postprocess_frame = gst_matroska_demux_check_subtitle_buffer;
    subtitlecontext->check_markup = FALSE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_VOBSUB)) {
    caps = gst_caps_new_simple ("video/x-dvd-subpicture", NULL);
    ((GstMatroskaTrackContext *) subtitlecontext)->send_dvd_event = TRUE;
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_HDMVPGS)) {
    caps = gst_caps_new_simple ("subpicture/x-pgs", NULL);
  } else if (!strcmp (codec_id, GST_MATROSKA_CODEC_ID_SUBTITLE_KATE)) {
    caps = gst_caps_new_simple ("subtitle/x-kate", NULL);
    context->send_xiph_headers = TRUE;
  } else {
    GST_DEBUG ("Unknown subtitle stream: codec_id='%s'", codec_id);
    caps = gst_caps_new_simple ("application/x-subtitle-unknown", NULL);
  }

  if (data != NULL && size > 0) {
    GstBuffer *buf;

    buf = gst_buffer_new_and_alloc (size);
    memcpy (GST_BUFFER_DATA (buf), data, size);
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER, buf, NULL);
    gst_buffer_unref (buf);
  }

  return caps;
}

static void
gst_matroska_demux_set_index (GstElement * element, GstIndex * index)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->common.element_index)
    gst_object_unref (demux->common.element_index);
  demux->common.element_index = index ? gst_object_ref (index) : NULL;
  GST_OBJECT_UNLOCK (demux);
  GST_DEBUG_OBJECT (demux, "Set index %" GST_PTR_FORMAT,
      demux->common.element_index);
}

static GstIndex *
gst_matroska_demux_get_index (GstElement * element)
{
  GstIndex *result = NULL;
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->common.element_index)
    result = gst_object_ref (demux->common.element_index);
  GST_OBJECT_UNLOCK (demux);

  GST_DEBUG_OBJECT (demux, "Returning index %" GST_PTR_FORMAT, result);

  return result;
}

static GstStateChangeReturn
gst_matroska_demux_change_state (GstElement * element,
    GstStateChange transition)
{
  GstMatroskaDemux *demux = GST_MATROSKA_DEMUX (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  /* handle upwards state changes here */
  switch (transition) {
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* handle downwards state changes */
  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_matroska_demux_reset (GST_ELEMENT (demux));
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_matroska_demux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstMatroskaDemux *demux;

  g_return_if_fail (GST_IS_MATROSKA_DEMUX (object));
  demux = GST_MATROSKA_DEMUX (object);

  switch (prop_id) {
    case ARG_MAX_GAP_TIME:
      GST_OBJECT_LOCK (demux);
      demux->max_gap_time = g_value_get_uint64 (value);
      GST_OBJECT_UNLOCK (demux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_matroska_demux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstMatroskaDemux *demux;

  g_return_if_fail (GST_IS_MATROSKA_DEMUX (object));
  demux = GST_MATROSKA_DEMUX (object);

  switch (prop_id) {
    case ARG_MAX_GAP_TIME:
      GST_OBJECT_LOCK (demux);
      g_value_set_uint64 (value, demux->max_gap_time);
      GST_OBJECT_UNLOCK (demux);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_matroska_demux_plugin_init (GstPlugin * plugin)
{
  gst_riff_init ();

  /* parser helper separate debug */
  GST_DEBUG_CATEGORY_INIT (ebmlread_debug, "ebmlread",
      0, "EBML stream helper class");

  /* create an elementfactory for the matroska_demux element */
  if (!gst_element_register (plugin, "matroskademux",
          GST_RANK_PRIMARY, GST_TYPE_MATROSKA_DEMUX))
    return FALSE;

  return TRUE;
}
