/* gstgoom.c: implementation of goom drawing element
 * Copyright (C) <2001> Richard Boulton <richard@tartarus.org>
 *           (C) <2006> Wim Taymans <wim at fluendo dot com>
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

/**
 * SECTION:element-goom2k1
 * @see_also: goom, synaesthesia
 *
 * Goom2k1 is an audio visualisation element. It creates warping structures
 * based on the incomming audio signal. Goom2k1 is the older version of the
 * visualisation. Also available is goom2k4, with a different look.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v audiotestsrc ! goom2k1 ! ffmpegcolorspace ! xvimagesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>
#include "gstgoom.h"
#include <gst/video/video.h>
#include "goom_core.h"

GST_DEBUG_CATEGORY_STATIC (goom_debug);
#define GST_CAT_DEFAULT goom_debug

#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 240
#define DEFAULT_FPS_N  25
#define DEFAULT_FPS_D  1

/* signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0
      /* FILL ME */
};

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_xRGB_HOST_ENDIAN)
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",    /* the name of the pads */
    GST_PAD_SINK,               /* type of the pad */
    GST_PAD_ALWAYS,             /* ALWAYS/SOMETIMES */
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) TRUE, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) [ 8000, 96000 ], " "channels = (int) { 1, 2 }")
    );


static void gst_goom_class_init (GstGoomClass * klass);
static void gst_goom_base_init (GstGoomClass * klass);
static void gst_goom_init (GstGoom * goom);
static void gst_goom_finalize (GObject * object);

static GstStateChangeReturn gst_goom_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn gst_goom_chain (GstPad * pad, GstBuffer * buffer);
static gboolean gst_goom_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_goom_sink_event (GstPad * pad, GstEvent * event);

static gboolean gst_goom_src_query (GstPad * pad, GstQuery * query);

static gboolean gst_goom_sink_setcaps (GstPad * pad, GstCaps * caps);
static gboolean gst_goom_src_setcaps (GstPad * pad, GstCaps * caps);

static GstElementClass *parent_class = NULL;

GType
gst_goom_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo info = {
      sizeof (GstGoomClass),
      (GBaseInitFunc) gst_goom_base_init,
      NULL,
      (GClassInitFunc) gst_goom_class_init,
      NULL,
      NULL,
      sizeof (GstGoom),
      0,
      (GInstanceInitFunc) gst_goom_init,
    };

    type = g_type_register_static (GST_TYPE_ELEMENT, "GstGoom2k1", &info, 0);
  }
  return type;
}

static void
gst_goom_base_init (GstGoomClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_set_details_simple (element_class,
      "GOOM: what a GOOM! 2k1 edition", "Visualization",
      "Takes frames of data and outputs video frames using the GOOM 2k1 filter",
      "Wim Taymans <wim@fluendo.com>");
  gst_element_class_add_static_pad_template (element_class,
      &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);
}

static void
gst_goom_class_init (GstGoomClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_goom_finalize;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_goom_change_state);

  GST_DEBUG_CATEGORY_INIT (goom_debug, "goom", 0, "goom visualisation element");
}

static void
gst_goom_init (GstGoom * goom)
{
  /* create the sink and src pads */
  goom->sinkpad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_pad_set_chain_function (goom->sinkpad,
      GST_DEBUG_FUNCPTR (gst_goom_chain));
  gst_pad_set_event_function (goom->sinkpad,
      GST_DEBUG_FUNCPTR (gst_goom_sink_event));
  gst_pad_set_setcaps_function (goom->sinkpad,
      GST_DEBUG_FUNCPTR (gst_goom_sink_setcaps));
  gst_element_add_pad (GST_ELEMENT (goom), goom->sinkpad);

  goom->srcpad = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_set_setcaps_function (goom->srcpad,
      GST_DEBUG_FUNCPTR (gst_goom_src_setcaps));
  gst_pad_set_event_function (goom->srcpad,
      GST_DEBUG_FUNCPTR (gst_goom_src_event));
  gst_pad_set_query_function (goom->srcpad,
      GST_DEBUG_FUNCPTR (gst_goom_src_query));
  gst_element_add_pad (GST_ELEMENT (goom), goom->srcpad);

  goom->adapter = gst_adapter_new ();

  goom->width = DEFAULT_WIDTH;
  goom->height = DEFAULT_HEIGHT;
  goom->fps_n = DEFAULT_FPS_N;  /* desired frame rate */
  goom->fps_d = DEFAULT_FPS_D;  /* desired frame rate */
  goom->channels = 0;
  goom->rate = 0;
  goom->duration = 0;

  goom_init (&(goom->goomdata), goom->width, goom->height);
}

static void
gst_goom_finalize (GObject * object)
{
  GstGoom *goom = GST_GOOM (object);

  goom_close (&(goom->goomdata));

  g_object_unref (goom->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_goom_reset (GstGoom * goom)
{
  gst_adapter_clear (goom->adapter);
  gst_segment_init (&goom->segment, GST_FORMAT_UNDEFINED);

  GST_OBJECT_LOCK (goom);
  goom->proportion = 1.0;
  goom->earliest_time = -1;
  GST_OBJECT_UNLOCK (goom);
}

static gboolean
gst_goom_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstGoom *goom;
  GstStructure *structure;
  gboolean res;

  goom = GST_GOOM (GST_PAD_PARENT (pad));

  structure = gst_caps_get_structure (caps, 0);

  res = gst_structure_get_int (structure, "channels", &goom->channels);
  res &= gst_structure_get_int (structure, "rate", &goom->rate);

  goom->bps = goom->channels * sizeof (gint16);

  return res;
}

static gboolean
gst_goom_src_setcaps (GstPad * pad, GstCaps * caps)
{
  GstGoom *goom;
  GstStructure *structure;

  goom = GST_GOOM (GST_PAD_PARENT (pad));

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "width", &goom->width) ||
      !gst_structure_get_int (structure, "height", &goom->height) ||
      !gst_structure_get_fraction (structure, "framerate", &goom->fps_n,
          &goom->fps_d))
    return FALSE;

  goom_set_resolution (&(goom->goomdata), goom->width, goom->height);

  /* size of the output buffer in bytes, depth is always 4 bytes */
  goom->outsize = goom->width * goom->height * 4;
  goom->duration =
      gst_util_uint64_scale_int (GST_SECOND, goom->fps_d, goom->fps_n);
  goom->spf = gst_util_uint64_scale_int (goom->rate, goom->fps_d, goom->fps_n);
  goom->bpf = goom->spf * goom->bps;

  GST_DEBUG_OBJECT (goom, "dimension %dx%d, framerate %d/%d, spf %d",
      goom->width, goom->height, goom->fps_n, goom->fps_d, goom->spf);

  return TRUE;
}

static gboolean
gst_goom_src_negotiate (GstGoom * goom)
{
  GstCaps *othercaps, *target;
  GstStructure *structure;
  const GstCaps *templ;

  templ = gst_pad_get_pad_template_caps (goom->srcpad);

  GST_DEBUG_OBJECT (goom, "performing negotiation");

  /* see what the peer can do */
  othercaps = gst_pad_peer_get_caps (goom->srcpad);
  if (othercaps) {
    target = gst_caps_intersect (othercaps, templ);
    gst_caps_unref (othercaps);

    if (gst_caps_is_empty (target))
      goto no_format;

    gst_caps_truncate (target);
  } else {
    target = gst_caps_ref ((GstCaps *) templ);
  }

  structure = gst_caps_get_structure (target, 0);
  gst_structure_fixate_field_nearest_int (structure, "width", DEFAULT_WIDTH);
  gst_structure_fixate_field_nearest_int (structure, "height", DEFAULT_HEIGHT);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate",
      DEFAULT_FPS_N, DEFAULT_FPS_D);

  gst_pad_set_caps (goom->srcpad, target);
  gst_caps_unref (target);

  return TRUE;

no_format:
  {
    gst_caps_unref (target);
    return FALSE;
  }
}

static gboolean
gst_goom_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstGoom *goom;

  goom = GST_GOOM (gst_pad_get_parent (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:
    {
      gdouble proportion;
      GstClockTimeDiff diff;
      GstClockTime timestamp;

      gst_event_parse_qos (event, &proportion, &diff, &timestamp);

      /* save stuff for the _chain() function */
      GST_OBJECT_LOCK (goom);
      goom->proportion = proportion;
      if (diff >= 0)
        /* we're late, this is a good estimate for next displayable
         * frame (see part-qos.txt) */
        goom->earliest_time = timestamp + 2 * diff + goom->duration;
      else
        goom->earliest_time = timestamp + diff;
      GST_OBJECT_UNLOCK (goom);

      res = gst_pad_push_event (goom->sinkpad, event);
      break;
    }
    default:
      res = gst_pad_push_event (goom->sinkpad, event);
      break;
  }
  gst_object_unref (goom);

  return res;
}

static gboolean
gst_goom_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstGoom *goom;

  goom = GST_GOOM (gst_pad_get_parent (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      res = gst_pad_push_event (goom->srcpad, event);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_goom_reset (goom);
      res = gst_pad_push_event (goom->srcpad, event);
      break;
    case GST_EVENT_NEWSEGMENT:
    {
      GstFormat format;
      gdouble rate, arate;
      gint64 start, stop, time;
      gboolean update;

      /* the newsegment values are used to clip the input samples
       * and to convert the incomming timestamps to running time so
       * we can do QoS */
      gst_event_parse_new_segment_full (event, &update, &rate, &arate, &format,
          &start, &stop, &time);

      /* now configure the values */
      gst_segment_set_newsegment_full (&goom->segment, update,
          rate, arate, format, start, stop, time);

      res = gst_pad_push_event (goom->srcpad, event);
      break;
    }
    default:
      res = gst_pad_push_event (goom->srcpad, event);
      break;
  }
  gst_object_unref (goom);

  return res;
}

static gboolean
gst_goom_src_query (GstPad * pad, GstQuery * query)
{
  gboolean res;
  GstGoom *goom;

  goom = GST_GOOM (gst_pad_get_parent (pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
    {
      /* We need to send the query upstream and add the returned latency to our
       * own */
      GstClockTime min_latency, max_latency;
      gboolean us_live;
      GstClockTime our_latency;
      guint max_samples;

      if ((res = gst_pad_peer_query (goom->sinkpad, query))) {
        gst_query_parse_latency (query, &us_live, &min_latency, &max_latency);

        GST_DEBUG_OBJECT (goom, "Peer latency: min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

        /* the max samples we must buffer buffer */
        max_samples = MAX (GOOM_SAMPLES, goom->spf);
        our_latency =
            gst_util_uint64_scale_int (max_samples, GST_SECOND, goom->rate);

        GST_DEBUG_OBJECT (goom, "Our latency: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (our_latency));

        /* we add some latency but only if we need to buffer more than what
         * upstream gives us */
        min_latency += our_latency;
        if (max_latency != -1)
          max_latency += our_latency;

        GST_DEBUG_OBJECT (goom, "Calculated total latency : min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

        gst_query_set_latency (query, TRUE, min_latency, max_latency);
      }
      break;
    }
    default:
      res = gst_pad_peer_query (goom->sinkpad, query);
      break;
  }

  gst_object_unref (goom);

  return res;
}

static GstFlowReturn
get_buffer (GstGoom * goom, GstBuffer ** outbuf)
{
  GstFlowReturn ret;

  if (GST_PAD_CAPS (goom->srcpad) == NULL) {
    if (!gst_goom_src_negotiate (goom))
      return GST_FLOW_NOT_NEGOTIATED;
  }

  GST_DEBUG_OBJECT (goom, "allocating output buffer with caps %"
      GST_PTR_FORMAT, GST_PAD_CAPS (goom->srcpad));

  ret =
      gst_pad_alloc_buffer_and_set_caps (goom->srcpad,
      GST_BUFFER_OFFSET_NONE, goom->outsize,
      GST_PAD_CAPS (goom->srcpad), outbuf);
  if (ret != GST_FLOW_OK)
    return ret;

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_goom_chain (GstPad * pad, GstBuffer * buffer)
{
  GstGoom *goom;
  GstFlowReturn ret;
  GstBuffer *outbuf = NULL;

  goom = GST_GOOM (gst_pad_get_parent (pad));

  /* If we don't have an output format yet, preallocate a buffer to try and
   * set one */
  if (GST_PAD_CAPS (goom->srcpad) == NULL) {
    ret = get_buffer (goom, &outbuf);
    if (ret != GST_FLOW_OK) {
      gst_buffer_unref (buffer);
      goto beach;
    }
  }

  /* don't try to combine samples from discont buffer */
  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT)) {
    gst_adapter_clear (goom->adapter);
  }

  GST_DEBUG_OBJECT (goom,
      "Input buffer has %d samples, time=%" G_GUINT64_FORMAT,
      GST_BUFFER_SIZE (buffer) / goom->bps, GST_BUFFER_TIMESTAMP (buffer));

  /* Collect samples until we have enough for an output frame */
  gst_adapter_push (goom->adapter, buffer);

  ret = GST_FLOW_OK;

  while (TRUE) {
    const guint16 *data;
    gboolean need_skip;
    guchar *out_frame;
    gint i;
    guint avail, to_flush;
    guint64 dist, timestamp;

    avail = gst_adapter_available (goom->adapter);
    GST_DEBUG_OBJECT (goom, "avail now %u", avail);

    /* we need GOOM_SAMPLES to get a meaningful result from goom. */
    if (avail < (GOOM_SAMPLES * goom->bps))
      break;

    /* we also need enough samples to produce one frame at least */
    if (avail < goom->bpf)
      break;

    GST_DEBUG_OBJECT (goom, "processing buffer");

    /* get timestamp of the current adapter byte */
    timestamp = gst_adapter_prev_timestamp (goom->adapter, &dist);
    if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
      /* convert bytes to time */
      dist /= goom->bps;
      timestamp += gst_util_uint64_scale_int (dist, GST_SECOND, goom->rate);
    }

    if (timestamp != -1) {
      gint64 qostime;

      qostime = gst_segment_to_running_time (&goom->segment, GST_FORMAT_TIME,
          timestamp);
      qostime += goom->duration;

      GST_OBJECT_LOCK (goom);
      /* check for QoS, don't compute buffers that are known to be late */
      need_skip = goom->earliest_time != -1 && qostime <= goom->earliest_time;
      GST_OBJECT_UNLOCK (goom);

      if (need_skip) {
        GST_WARNING_OBJECT (goom,
            "QoS: skip ts: %" GST_TIME_FORMAT ", earliest: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (qostime), GST_TIME_ARGS (goom->earliest_time));
        goto skip;
      }
    }

    /* get next GOOM_SAMPLES, we have at least this amount of samples */
    data =
        (const guint16 *) gst_adapter_peek (goom->adapter,
        GOOM_SAMPLES * goom->bps);

    if (goom->channels == 2) {
      for (i = 0; i < GOOM_SAMPLES; i++) {
        goom->datain[0][i] = *data++;
        goom->datain[1][i] = *data++;
      }
    } else {
      for (i = 0; i < GOOM_SAMPLES; i++) {
        goom->datain[0][i] = *data;
        goom->datain[1][i] = *data++;
      }
    }

    /* alloc a buffer if we don't have one yet, this happens
     * when we pushed a buffer in this while loop before */
    if (outbuf == NULL) {
      ret = get_buffer (goom, &outbuf);
      if (ret != GST_FLOW_OK) {
        goto beach;
      }
    }

    GST_BUFFER_TIMESTAMP (outbuf) = timestamp;
    GST_BUFFER_DURATION (outbuf) = goom->duration;
    GST_BUFFER_SIZE (outbuf) = goom->outsize;

    out_frame = (guchar *) goom_update (&(goom->goomdata), goom->datain);
    memcpy (GST_BUFFER_DATA (outbuf), out_frame, goom->outsize);

    GST_DEBUG ("Pushing frame with time=%" GST_TIME_FORMAT ", duration=%"
        GST_TIME_FORMAT, GST_TIME_ARGS (timestamp),
        GST_TIME_ARGS (goom->duration));

    ret = gst_pad_push (goom->srcpad, outbuf);
    outbuf = NULL;

  skip:
    /* Now flush the samples we needed for this frame, which might be more than
     * the samples we used (GOOM_SAMPLES). */
    to_flush = goom->bpf;

    GST_DEBUG_OBJECT (goom, "finished frame, flushing %u bytes from input",
        to_flush);
    gst_adapter_flush (goom->adapter, to_flush);

    if (ret != GST_FLOW_OK)
      break;
  }

  if (outbuf != NULL)
    gst_buffer_unref (outbuf);

beach:
  gst_object_unref (goom);

  return ret;
}

static GstStateChangeReturn
gst_goom_change_state (GstElement * element, GstStateChange transition)
{
  GstGoom *goom = GST_GOOM (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_goom_reset (goom);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "goom2k1", GST_RANK_NONE, GST_TYPE_GOOM);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "goom2k1",
    "GOOM 2k1 visualization filter",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
