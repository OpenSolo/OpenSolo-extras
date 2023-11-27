/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * SECTION:element-vorbisenc
 * @see_also: vorbisdec, oggmux
 *
 * This element encodes raw float audio into a Vorbis stream.
 * <ulink url="http://www.vorbis.com/">Vorbis</ulink> is a royalty-free
 * audio codec maintained by the <ulink url="http://www.xiph.org/">Xiph.org
 * Foundation</ulink>.
 *
 * <refsect2>
 * <title>Example pipelines</title>
 * |[
 * gst-launch -v audiotestsrc wave=sine num-buffers=100 ! audioconvert ! vorbisenc ! oggmux ! filesink location=sine.ogg
 * ]| Encode a test sine signal to Ogg/Vorbis.  Note that the resulting file
 * will be really small because a sine signal compresses very well.
 * |[
 * gst-launch -v alsasrc ! audioconvert ! vorbisenc ! oggmux ! filesink location=alsasrc.ogg
 * ]| Record from a sound card using ALSA and encode to Ogg/Vorbis.
 * </refsect2>
 *
 * Last reviewed on 2006-03-01 (0.10.4)
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vorbis/vorbisenc.h>

#include <gst/gsttagsetter.h>
#include <gst/tag/tag.h>
#include <gst/audio/multichannel.h>
#include <gst/audio/audio.h>
#include "gstvorbisenc.h"

#include "gstvorbiscommon.h"

GST_DEBUG_CATEGORY_EXTERN (vorbisenc_debug);
#define GST_CAT_DEFAULT vorbisenc_debug

static GstStaticPadTemplate vorbis_enc_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-float, "
        "rate = (int) [ 1, 200000 ], "
        "channels = (int) [ 1, 255 ], " "endianness = (int) BYTE_ORDER, "
        "width = (int) 32")
    );

static GstStaticPadTemplate vorbis_enc_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-vorbis")
    );

enum
{
  ARG_0,
  ARG_MAX_BITRATE,
  ARG_BITRATE,
  ARG_MIN_BITRATE,
  ARG_QUALITY,
  ARG_MANAGED,
  ARG_LAST_MESSAGE
};

static GstFlowReturn gst_vorbis_enc_output_buffers (GstVorbisEnc * vorbisenc);


#define MAX_BITRATE_DEFAULT     -1
#define BITRATE_DEFAULT         -1
#define MIN_BITRATE_DEFAULT     -1
#define QUALITY_DEFAULT         0.3
#define LOWEST_BITRATE          6000    /* lowest allowed for a 8 kHz stream */
#define HIGHEST_BITRATE         250001  /* highest allowed for a 44 kHz stream */

static gboolean gst_vorbis_enc_start (GstAudioEncoder * enc);
static gboolean gst_vorbis_enc_stop (GstAudioEncoder * enc);
static gboolean gst_vorbis_enc_set_format (GstAudioEncoder * enc,
    GstAudioInfo * info);
static GstFlowReturn gst_vorbis_enc_handle_frame (GstAudioEncoder * enc,
    GstBuffer * in_buf);
static GstCaps *gst_vorbis_enc_getcaps (GstAudioEncoder * enc);
static gboolean gst_vorbis_enc_sink_event (GstAudioEncoder * enc,
    GstEvent * event);
static GstFlowReturn gst_vorbis_enc_pre_push (GstAudioEncoder * enc,
    GstBuffer ** buffer);

static gboolean gst_vorbis_enc_setup (GstVorbisEnc * vorbisenc);

static void gst_vorbis_enc_dispose (GObject * object);
static void gst_vorbis_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_vorbis_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_vorbis_enc_add_interfaces (GType vorbisenc_type);

GST_BOILERPLATE_FULL (GstVorbisEnc, gst_vorbis_enc, GstAudioEncoder,
    GST_TYPE_AUDIO_ENCODER, gst_vorbis_enc_add_interfaces);

static void
gst_vorbis_enc_add_interfaces (GType vorbisenc_type)
{
  static const GInterfaceInfo tag_setter_info = { NULL, NULL, NULL };

  g_type_add_interface_static (vorbisenc_type, GST_TYPE_TAG_SETTER,
      &tag_setter_info);
}

static void
gst_vorbis_enc_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class,
      &vorbis_enc_src_factory);
  gst_element_class_add_static_pad_template (element_class,
      &vorbis_enc_sink_factory);

  gst_element_class_set_details_simple (element_class,
      "Vorbis audio encoder", "Codec/Encoder/Audio",
      "Encodes audio in Vorbis format",
      "Monty <monty@xiph.org>, " "Wim Taymans <wim@fluendo.com>");
}

static void
gst_vorbis_enc_class_init (GstVorbisEncClass * klass)
{
  GObjectClass *gobject_class;
  GstAudioEncoderClass *base_class;

  gobject_class = (GObjectClass *) klass;
  base_class = (GstAudioEncoderClass *) (klass);

  gobject_class->set_property = gst_vorbis_enc_set_property;
  gobject_class->get_property = gst_vorbis_enc_get_property;
  gobject_class->dispose = gst_vorbis_enc_dispose;

  base_class->start = GST_DEBUG_FUNCPTR (gst_vorbis_enc_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_vorbis_enc_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_vorbis_enc_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_vorbis_enc_handle_frame);
  base_class->getcaps = GST_DEBUG_FUNCPTR (gst_vorbis_enc_getcaps);
  base_class->event = GST_DEBUG_FUNCPTR (gst_vorbis_enc_sink_event);
  base_class->pre_push = GST_DEBUG_FUNCPTR (gst_vorbis_enc_pre_push);

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MAX_BITRATE,
      g_param_spec_int ("max-bitrate", "Maximum Bitrate",
          "Specify a maximum bitrate (in bps). Useful for streaming "
          "applications. (-1 == disabled)",
          -1, HIGHEST_BITRATE, MAX_BITRATE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_BITRATE,
      g_param_spec_int ("bitrate", "Target Bitrate",
          "Attempt to encode at a bitrate averaging this (in bps). "
          "This uses the bitrate management engine, and is not recommended for most users. "
          "Quality is a better alternative. (-1 == disabled)", -1,
          HIGHEST_BITRATE, BITRATE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MIN_BITRATE,
      g_param_spec_int ("min-bitrate", "Minimum Bitrate",
          "Specify a minimum bitrate (in bps). Useful for encoding for a "
          "fixed-size channel. (-1 == disabled)", -1, HIGHEST_BITRATE,
          MIN_BITRATE_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_QUALITY,
      g_param_spec_float ("quality", "Quality",
          "Specify quality instead of specifying a particular bitrate.", -0.1,
          1.0, QUALITY_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MANAGED,
      g_param_spec_boolean ("managed", "Managed",
          "Enable bitrate management engine", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_LAST_MESSAGE,
      g_param_spec_string ("last-message", "last-message",
          "The last status message", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gst_vorbis_enc_init (GstVorbisEnc * vorbisenc, GstVorbisEncClass * klass)
{
  GstAudioEncoder *enc = GST_AUDIO_ENCODER (vorbisenc);

  vorbisenc->channels = -1;
  vorbisenc->frequency = -1;

  vorbisenc->managed = FALSE;
  vorbisenc->max_bitrate = MAX_BITRATE_DEFAULT;
  vorbisenc->bitrate = BITRATE_DEFAULT;
  vorbisenc->min_bitrate = MIN_BITRATE_DEFAULT;
  vorbisenc->quality = QUALITY_DEFAULT;
  vorbisenc->quality_set = FALSE;
  vorbisenc->last_message = NULL;

  /* arrange granulepos marking (and required perfect ts) */
  gst_audio_encoder_set_mark_granule (enc, TRUE);
  gst_audio_encoder_set_perfect_timestamp (enc, TRUE);
}

static void
gst_vorbis_enc_dispose (GObject * object)
{
  GstVorbisEnc *vorbisenc = GST_VORBISENC (object);

  if (vorbisenc->sinkcaps) {
    gst_caps_unref (vorbisenc->sinkcaps);
    vorbisenc->sinkcaps = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gst_vorbis_enc_start (GstAudioEncoder * enc)
{
  GstVorbisEnc *vorbisenc = GST_VORBISENC (enc);

  GST_DEBUG_OBJECT (enc, "start");
  vorbisenc->tags = gst_tag_list_new ();
  vorbisenc->header_sent = FALSE;

  return TRUE;
}

static gboolean
gst_vorbis_enc_stop (GstAudioEncoder * enc)
{
  GstVorbisEnc *vorbisenc = GST_VORBISENC (enc);

  GST_DEBUG_OBJECT (enc, "stop");
  vorbis_block_clear (&vorbisenc->vb);
  vorbis_dsp_clear (&vorbisenc->vd);
  vorbis_info_clear (&vorbisenc->vi);
  g_free (vorbisenc->last_message);
  vorbisenc->last_message = NULL;
  gst_tag_list_free (vorbisenc->tags);
  vorbisenc->tags = NULL;
  g_slist_foreach (vorbisenc->headers, (GFunc) gst_buffer_unref, NULL);
  vorbisenc->headers = NULL;

  gst_tag_setter_reset_tags (GST_TAG_SETTER (enc));

  return TRUE;
}

static GstCaps *
gst_vorbis_enc_generate_sink_caps (void)
{
  GstCaps *caps = gst_caps_new_empty ();
  int i, c;

  gst_caps_append_structure (caps, gst_structure_new ("audio/x-raw-float",
          "rate", GST_TYPE_INT_RANGE, 1, 200000,
          "channels", G_TYPE_INT, 1,
          "endianness", G_TYPE_INT, G_BYTE_ORDER, "width", G_TYPE_INT, 32,
          NULL));

  gst_caps_append_structure (caps, gst_structure_new ("audio/x-raw-float",
          "rate", GST_TYPE_INT_RANGE, 1, 200000,
          "channels", G_TYPE_INT, 2,
          "endianness", G_TYPE_INT, G_BYTE_ORDER, "width", G_TYPE_INT, 32,
          NULL));

  for (i = 3; i <= 8; i++) {
    GValue chanpos = { 0 };
    GValue pos = { 0 };
    GstStructure *structure;

    g_value_init (&chanpos, GST_TYPE_ARRAY);
    g_value_init (&pos, GST_TYPE_AUDIO_CHANNEL_POSITION);

    for (c = 0; c < i; c++) {
      g_value_set_enum (&pos, gst_vorbis_channel_positions[i - 1][c]);
      gst_value_array_append_value (&chanpos, &pos);
    }
    g_value_unset (&pos);

    structure = gst_structure_new ("audio/x-raw-float",
        "rate", GST_TYPE_INT_RANGE, 1, 200000,
        "channels", G_TYPE_INT, i,
        "endianness", G_TYPE_INT, G_BYTE_ORDER, "width", G_TYPE_INT, 32, NULL);
    gst_structure_set_value (structure, "channel-positions", &chanpos);
    g_value_unset (&chanpos);

    gst_caps_append_structure (caps, structure);
  }

  gst_caps_append_structure (caps, gst_structure_new ("audio/x-raw-float",
          "rate", GST_TYPE_INT_RANGE, 1, 200000,
          "channels", GST_TYPE_INT_RANGE, 9, 255,
          "endianness", G_TYPE_INT, G_BYTE_ORDER, "width", G_TYPE_INT, 32,
          NULL));

  return caps;
}

static GstCaps *
gst_vorbis_enc_getcaps (GstAudioEncoder * enc)
{
  GstVorbisEnc *vorbisenc = GST_VORBISENC (enc);

  if (vorbisenc->sinkcaps == NULL)
    vorbisenc->sinkcaps = gst_vorbis_enc_generate_sink_caps ();

  return gst_audio_encoder_proxy_getcaps (enc, vorbisenc->sinkcaps);
}

static gint64
gst_vorbis_enc_get_latency (GstVorbisEnc * vorbisenc)
{
  /* FIXME, this probably depends on the bitrate and other setting but for now
   * we return this value, which was obtained by totally unscientific
   * measurements */
  return 58 * GST_MSECOND;
}

static gboolean
gst_vorbis_enc_set_format (GstAudioEncoder * enc, GstAudioInfo * info)
{
  GstVorbisEnc *vorbisenc;

  vorbisenc = GST_VORBISENC (enc);

  vorbisenc->channels = GST_AUDIO_INFO_CHANNELS (info);
  vorbisenc->frequency = GST_AUDIO_INFO_RATE (info);

  /* if re-configured, we were drained and cleared already */
  if (!gst_vorbis_enc_setup (vorbisenc))
    return FALSE;

  /* feedback to base class */
  gst_audio_encoder_set_latency (enc,
      gst_vorbis_enc_get_latency (vorbisenc),
      gst_vorbis_enc_get_latency (vorbisenc));

  return TRUE;
}

static void
gst_vorbis_enc_metadata_set1 (const GstTagList * list, const gchar * tag,
    gpointer vorbisenc)
{
  GstVorbisEnc *enc = GST_VORBISENC (vorbisenc);
  GList *vc_list, *l;

  vc_list = gst_tag_to_vorbis_comments (list, tag);

  for (l = vc_list; l != NULL; l = l->next) {
    const gchar *vc_string = (const gchar *) l->data;
    gchar *key = NULL, *val = NULL;

    GST_LOG_OBJECT (vorbisenc, "vorbis comment: %s", vc_string);
    if (gst_tag_parse_extended_comment (vc_string, &key, NULL, &val, TRUE)) {
      vorbis_comment_add_tag (&enc->vc, key, val);
      g_free (key);
      g_free (val);
    }
  }

  g_list_foreach (vc_list, (GFunc) g_free, NULL);
  g_list_free (vc_list);
}

static void
gst_vorbis_enc_set_metadata (GstVorbisEnc * enc)
{
  GstTagList *merged_tags;
  const GstTagList *user_tags;

  vorbis_comment_init (&enc->vc);

  user_tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (enc));

  GST_DEBUG_OBJECT (enc, "upstream tags = %" GST_PTR_FORMAT, enc->tags);
  GST_DEBUG_OBJECT (enc, "user-set tags = %" GST_PTR_FORMAT, user_tags);

  /* gst_tag_list_merge() will handle NULL for either or both lists fine */
  merged_tags = gst_tag_list_merge (user_tags, enc->tags,
      gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (enc)));

  if (merged_tags) {
    GST_DEBUG_OBJECT (enc, "merged   tags = %" GST_PTR_FORMAT, merged_tags);
    gst_tag_list_foreach (merged_tags, gst_vorbis_enc_metadata_set1, enc);
    gst_tag_list_free (merged_tags);
  }
}

static gchar *
get_constraints_string (GstVorbisEnc * vorbisenc)
{
  gint min = vorbisenc->min_bitrate;
  gint max = vorbisenc->max_bitrate;
  gchar *result;

  if (min > 0 && max > 0)
    result = g_strdup_printf ("(min %d bps, max %d bps)", min, max);
  else if (min > 0)
    result = g_strdup_printf ("(min %d bps, no max)", min);
  else if (max > 0)
    result = g_strdup_printf ("(no min, max %d bps)", max);
  else
    result = g_strdup_printf ("(no min or max)");

  return result;
}

static void
update_start_message (GstVorbisEnc * vorbisenc)
{
  gchar *constraints;

  g_free (vorbisenc->last_message);

  if (vorbisenc->bitrate > 0) {
    if (vorbisenc->managed) {
      constraints = get_constraints_string (vorbisenc);
      vorbisenc->last_message =
          g_strdup_printf ("encoding at average bitrate %d bps %s",
          vorbisenc->bitrate, constraints);
      g_free (constraints);
    } else {
      vorbisenc->last_message =
          g_strdup_printf
          ("encoding at approximate bitrate %d bps (VBR encoding enabled)",
          vorbisenc->bitrate);
    }
  } else {
    if (vorbisenc->quality_set) {
      if (vorbisenc->managed) {
        constraints = get_constraints_string (vorbisenc);
        vorbisenc->last_message =
            g_strdup_printf
            ("encoding at quality level %2.2f using constrained VBR %s",
            vorbisenc->quality, constraints);
        g_free (constraints);
      } else {
        vorbisenc->last_message =
            g_strdup_printf ("encoding at quality level %2.2f",
            vorbisenc->quality);
      }
    } else {
      constraints = get_constraints_string (vorbisenc);
      vorbisenc->last_message =
          g_strdup_printf ("encoding using bitrate management %s", constraints);
      g_free (constraints);
    }
  }

  g_object_notify (G_OBJECT (vorbisenc), "last_message");
}

static gboolean
gst_vorbis_enc_setup (GstVorbisEnc * vorbisenc)
{

  GST_LOG_OBJECT (vorbisenc, "setup");

  if (vorbisenc->bitrate < 0 && vorbisenc->min_bitrate < 0
      && vorbisenc->max_bitrate < 0) {
    vorbisenc->quality_set = TRUE;
  }

  update_start_message (vorbisenc);

  /* choose an encoding mode */
  /* (mode 0: 44kHz stereo uncoupled, roughly 128kbps VBR) */
  vorbis_info_init (&vorbisenc->vi);

  if (vorbisenc->quality_set) {
    if (vorbis_encode_setup_vbr (&vorbisenc->vi,
            vorbisenc->channels, vorbisenc->frequency,
            vorbisenc->quality) != 0) {
      GST_ERROR_OBJECT (vorbisenc,
          "vorbisenc: initialisation failed: invalid parameters for quality");
      vorbis_info_clear (&vorbisenc->vi);
      return FALSE;
    }

    /* do we have optional hard quality restrictions? */
    if (vorbisenc->max_bitrate > 0 || vorbisenc->min_bitrate > 0) {
      struct ovectl_ratemanage_arg ai;

      vorbis_encode_ctl (&vorbisenc->vi, OV_ECTL_RATEMANAGE_GET, &ai);

      ai.bitrate_hard_min = vorbisenc->min_bitrate;
      ai.bitrate_hard_max = vorbisenc->max_bitrate;
      ai.management_active = 1;

      vorbis_encode_ctl (&vorbisenc->vi, OV_ECTL_RATEMANAGE_SET, &ai);
    }
  } else {
    long min_bitrate, max_bitrate;

    min_bitrate = vorbisenc->min_bitrate > 0 ? vorbisenc->min_bitrate : -1;
    max_bitrate = vorbisenc->max_bitrate > 0 ? vorbisenc->max_bitrate : -1;

    if (vorbis_encode_setup_managed (&vorbisenc->vi,
            vorbisenc->channels,
            vorbisenc->frequency,
            max_bitrate, vorbisenc->bitrate, min_bitrate) != 0) {
      GST_ERROR_OBJECT (vorbisenc,
          "vorbis_encode_setup_managed "
          "(c %d, rate %d, max br %ld, br %d, min br %ld) failed",
          vorbisenc->channels, vorbisenc->frequency, max_bitrate,
          vorbisenc->bitrate, min_bitrate);
      vorbis_info_clear (&vorbisenc->vi);
      return FALSE;
    }
  }

  if (vorbisenc->managed && vorbisenc->bitrate < 0) {
    vorbis_encode_ctl (&vorbisenc->vi, OV_ECTL_RATEMANAGE_AVG, NULL);
  } else if (!vorbisenc->managed) {
    /* Turn off management entirely (if it was turned on). */
    vorbis_encode_ctl (&vorbisenc->vi, OV_ECTL_RATEMANAGE_SET, NULL);
  }
  vorbis_encode_setup_init (&vorbisenc->vi);

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init (&vorbisenc->vd, &vorbisenc->vi);
  vorbis_block_init (&vorbisenc->vd, &vorbisenc->vb);

  /* samples == granulepos start at 0 again */
  vorbisenc->samples_out = 0;

  /* fresh encoder available */
  vorbisenc->setup = TRUE;

  return TRUE;
}

static GstFlowReturn
gst_vorbis_enc_clear (GstVorbisEnc * vorbisenc)
{
  GstFlowReturn ret = GST_FLOW_OK;

  if (vorbisenc->setup) {
    vorbis_analysis_wrote (&vorbisenc->vd, 0);
    ret = gst_vorbis_enc_output_buffers (vorbisenc);

    /* marked EOS to encoder, recreate if needed */
    vorbisenc->setup = FALSE;
  }

  /* clean up and exit.  vorbis_info_clear() must be called last */
  vorbis_block_clear (&vorbisenc->vb);
  vorbis_dsp_clear (&vorbisenc->vd);
  vorbis_info_clear (&vorbisenc->vi);

  return ret;
}

static GstBuffer *
gst_vorbis_enc_buffer_from_header_packet (GstVorbisEnc * vorbisenc,
    ogg_packet * packet)
{
  GstBuffer *outbuf;

  outbuf = gst_buffer_new_and_alloc (packet->bytes);
  memcpy (GST_BUFFER_DATA (outbuf), packet->packet, packet->bytes);
  GST_BUFFER_OFFSET (outbuf) = vorbisenc->bytes_out;
  GST_BUFFER_OFFSET_END (outbuf) = 0;
  GST_BUFFER_TIMESTAMP (outbuf) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;

  GST_DEBUG ("created header packet buffer, %d bytes",
      GST_BUFFER_SIZE (outbuf));
  return outbuf;
}

static gboolean
gst_vorbis_enc_sink_event (GstAudioEncoder * enc, GstEvent * event)
{
  GstVorbisEnc *vorbisenc;

  vorbisenc = GST_VORBISENC (enc);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
      if (vorbisenc->tags) {
        GstTagList *list;

        gst_event_parse_tag (event, &list);
        gst_tag_list_insert (vorbisenc->tags, list,
            gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (vorbisenc)));
      } else {
        g_assert_not_reached ();
      }
      break;
      /* fall through */
    default:
      break;
  }

  /* we only peeked, let base class handle it */
  return FALSE;
}

/* push out the buffer and do internal bookkeeping */
static GstFlowReturn
gst_vorbis_enc_push_header (GstVorbisEnc * vorbisenc, GstBuffer * buffer)
{
  GST_DEBUG_OBJECT (vorbisenc,
      "Pushing buffer with GP %" G_GINT64_FORMAT ", ts %" GST_TIME_FORMAT,
      GST_BUFFER_OFFSET_END (buffer),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));
  gst_buffer_set_caps (buffer,
      GST_PAD_CAPS (GST_AUDIO_ENCODER_SRC_PAD (vorbisenc)));
  return gst_pad_push (GST_AUDIO_ENCODER_SRC_PAD (vorbisenc), buffer);
}

/*
 * (really really) FIXME: move into core (dixit tpm)
 */
/**
 * _gst_caps_set_buffer_array:
 * @caps: a #GstCaps
 * @field: field in caps to set
 * @buf: header buffers
 *
 * Adds given buffers to an array of buffers set as the given @field
 * on the given @caps.  List of buffer arguments must be NULL-terminated.
 *
 * Returns: input caps with a streamheader field added, or NULL if some error
 */
static GstCaps *
_gst_caps_set_buffer_array (GstCaps * caps, const gchar * field,
    GstBuffer * buf, ...)
{
  GstStructure *structure = NULL;
  va_list va;
  GValue array = { 0 };
  GValue value = { 0 };

  g_return_val_if_fail (caps != NULL, NULL);
  g_return_val_if_fail (gst_caps_is_fixed (caps), NULL);
  g_return_val_if_fail (field != NULL, NULL);

  caps = gst_caps_make_writable (caps);
  structure = gst_caps_get_structure (caps, 0);

  g_value_init (&array, GST_TYPE_ARRAY);

  va_start (va, buf);
  /* put buffers in a fixed list */
  while (buf) {
    g_assert (gst_buffer_is_metadata_writable (buf));

    /* mark buffer */
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_IN_CAPS);

    g_value_init (&value, GST_TYPE_BUFFER);
    buf = gst_buffer_copy (buf);
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_IN_CAPS);
    gst_value_set_buffer (&value, buf);
    gst_buffer_unref (buf);
    gst_value_array_append_value (&array, &value);
    g_value_unset (&value);

    buf = va_arg (va, GstBuffer *);
  }

  gst_structure_set_value (structure, field, &array);
  g_value_unset (&array);

  return caps;
}

static GstFlowReturn
gst_vorbis_enc_handle_frame (GstAudioEncoder * enc, GstBuffer * buffer)
{
  GstVorbisEnc *vorbisenc;
  GstFlowReturn ret = GST_FLOW_OK;
  gfloat *data;
  gulong size;
  gulong i, j;
  float **vorbis_buffer;
  GstBuffer *buf1, *buf2, *buf3;

  vorbisenc = GST_VORBISENC (enc);

  if (G_UNLIKELY (!vorbisenc->setup)) {
    if (buffer) {
      GST_DEBUG_OBJECT (vorbisenc, "forcing setup");
      /* should not fail, as setup before same way */
      if (!gst_vorbis_enc_setup (vorbisenc))
        return GST_FLOW_ERROR;
    } else {
      /* end draining */
      GST_LOG_OBJECT (vorbisenc, "already drained");
      return GST_FLOW_OK;
    }
  }

  if (!vorbisenc->header_sent) {
    /* Vorbis streams begin with three headers; the initial header (with
       most of the codec setup parameters) which is mandated by the Ogg
       bitstream spec.  The second header holds any comment fields.  The
       third header holds the bitstream codebook.  We merely need to
       make the headers, then pass them to libvorbis one at a time;
       libvorbis handles the additional Ogg bitstream constraints */
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    GstCaps *caps;

    GST_DEBUG_OBJECT (vorbisenc, "creating and sending header packets");
    gst_vorbis_enc_set_metadata (vorbisenc);
    vorbis_analysis_headerout (&vorbisenc->vd, &vorbisenc->vc, &header,
        &header_comm, &header_code);
    vorbis_comment_clear (&vorbisenc->vc);

    /* create header buffers */
    buf1 = gst_vorbis_enc_buffer_from_header_packet (vorbisenc, &header);
    buf2 = gst_vorbis_enc_buffer_from_header_packet (vorbisenc, &header_comm);
    buf3 = gst_vorbis_enc_buffer_from_header_packet (vorbisenc, &header_code);

    /* mark and put on caps */
    caps = gst_caps_new_simple ("audio/x-vorbis", NULL);
    caps = _gst_caps_set_buffer_array (caps, "streamheader",
        buf1, buf2, buf3, NULL);

    /* negotiate with these caps */
    GST_DEBUG_OBJECT (vorbisenc, "here are the caps: %" GST_PTR_FORMAT, caps);

    gst_buffer_set_caps (buf1, caps);
    gst_buffer_set_caps (buf2, caps);
    gst_buffer_set_caps (buf3, caps);
    gst_pad_set_caps (GST_AUDIO_ENCODER_SRC_PAD (vorbisenc), caps);
    gst_caps_unref (caps);

    /* store buffers for later pre_push sending */
    g_slist_foreach (vorbisenc->headers, (GFunc) gst_buffer_unref, NULL);
    vorbisenc->headers = NULL;
    GST_DEBUG_OBJECT (vorbisenc, "storing header buffers");
    vorbisenc->headers = g_slist_prepend (vorbisenc->headers, buf3);
    vorbisenc->headers = g_slist_prepend (vorbisenc->headers, buf2);
    vorbisenc->headers = g_slist_prepend (vorbisenc->headers, buf1);

    vorbisenc->header_sent = TRUE;
  }

  if (!buffer)
    return gst_vorbis_enc_clear (vorbisenc);

  /* data to encode */
  data = (gfloat *) GST_BUFFER_DATA (buffer);
  size = GST_BUFFER_SIZE (buffer) / (vorbisenc->channels * sizeof (float));

  /* expose the buffer to submit data */
  vorbis_buffer = vorbis_analysis_buffer (&vorbisenc->vd, size);

  /* deinterleave samples, write the buffer data */
  for (i = 0; i < size; i++) {
    for (j = 0; j < vorbisenc->channels; j++) {
      vorbis_buffer[j][i] = *data++;
    }
  }

  /* tell the library how much we actually submitted */
  vorbis_analysis_wrote (&vorbisenc->vd, size);

  GST_LOG_OBJECT (vorbisenc, "wrote %lu samples to vorbis", size);

  vorbisenc->samples_in += size;

  ret = gst_vorbis_enc_output_buffers (vorbisenc);

  return ret;
}

static GstFlowReturn
gst_vorbis_enc_output_buffers (GstVorbisEnc * vorbisenc)
{
  GstFlowReturn ret;

  /* vorbis does some data preanalysis, then divides up blocks for
     more involved (potentially parallel) processing.  Get a single
     block for encoding now */
  while (vorbis_analysis_blockout (&vorbisenc->vd, &vorbisenc->vb) == 1) {
    ogg_packet op;

    GST_LOG_OBJECT (vorbisenc, "analysed to a block");

    /* analysis */
    vorbis_analysis (&vorbisenc->vb, NULL);
    vorbis_bitrate_addblock (&vorbisenc->vb);

    while (vorbis_bitrate_flushpacket (&vorbisenc->vd, &op)) {
      GstBuffer *buf;

      GST_LOG_OBJECT (vorbisenc, "pushing out a data packet");
      buf = gst_buffer_new_and_alloc (op.bytes);
      memcpy (GST_BUFFER_DATA (buf), op.packet, op.bytes);
      /* tracking granulepos should tell us samples accounted for */
      ret =
          gst_audio_encoder_finish_frame (GST_AUDIO_ENCODER
          (vorbisenc), buf, op.granulepos - vorbisenc->samples_out);
      vorbisenc->samples_out = op.granulepos;

      if (ret != GST_FLOW_OK)
        return ret;
    }
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_vorbis_enc_pre_push (GstAudioEncoder * enc, GstBuffer ** buffer)
{
  GstVorbisEnc *vorbisenc;
  GstFlowReturn ret = GST_FLOW_OK;

  vorbisenc = GST_VORBISENC (enc);

  /* FIXME 0.11 ? get rid of this special ogg stuff and have it
   * put and use 'codec data' in caps like anything else,
   * with all the usual out-of-band advantage etc */
  if (G_UNLIKELY (vorbisenc->headers)) {
    GSList *header = vorbisenc->headers;

    /* try to push all of these, if we lose one, might as well lose all */
    while (header) {
      if (ret == GST_FLOW_OK)
        ret = gst_vorbis_enc_push_header (vorbisenc, header->data);
      else
        gst_vorbis_enc_push_header (vorbisenc, header->data);
      header = g_slist_next (header);
    }

    g_slist_free (vorbisenc->headers);
    vorbisenc->headers = NULL;
  }

  return ret;
}

static void
gst_vorbis_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstVorbisEnc *vorbisenc;

  g_return_if_fail (GST_IS_VORBISENC (object));

  vorbisenc = GST_VORBISENC (object);

  switch (prop_id) {
    case ARG_MAX_BITRATE:
      g_value_set_int (value, vorbisenc->max_bitrate);
      break;
    case ARG_BITRATE:
      g_value_set_int (value, vorbisenc->bitrate);
      break;
    case ARG_MIN_BITRATE:
      g_value_set_int (value, vorbisenc->min_bitrate);
      break;
    case ARG_QUALITY:
      g_value_set_float (value, vorbisenc->quality);
      break;
    case ARG_MANAGED:
      g_value_set_boolean (value, vorbisenc->managed);
      break;
    case ARG_LAST_MESSAGE:
      g_value_set_string (value, vorbisenc->last_message);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_vorbis_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVorbisEnc *vorbisenc;

  g_return_if_fail (GST_IS_VORBISENC (object));

  vorbisenc = GST_VORBISENC (object);

  switch (prop_id) {
    case ARG_MAX_BITRATE:
    {
      gboolean old_value = vorbisenc->managed;

      vorbisenc->max_bitrate = g_value_get_int (value);
      if (vorbisenc->max_bitrate >= 0
          && vorbisenc->max_bitrate < LOWEST_BITRATE) {
        g_warning ("Lowest allowed bitrate is %d", LOWEST_BITRATE);
        vorbisenc->max_bitrate = LOWEST_BITRATE;
      }
      if (vorbisenc->min_bitrate > 0 && vorbisenc->max_bitrate > 0)
        vorbisenc->managed = TRUE;
      else
        vorbisenc->managed = FALSE;

      if (old_value != vorbisenc->managed)
        g_object_notify (object, "managed");
      break;
    }
    case ARG_BITRATE:
      vorbisenc->bitrate = g_value_get_int (value);
      if (vorbisenc->bitrate >= 0 && vorbisenc->bitrate < LOWEST_BITRATE) {
        g_warning ("Lowest allowed bitrate is %d", LOWEST_BITRATE);
        vorbisenc->bitrate = LOWEST_BITRATE;
      }
      break;
    case ARG_MIN_BITRATE:
    {
      gboolean old_value = vorbisenc->managed;

      vorbisenc->min_bitrate = g_value_get_int (value);
      if (vorbisenc->min_bitrate >= 0
          && vorbisenc->min_bitrate < LOWEST_BITRATE) {
        g_warning ("Lowest allowed bitrate is %d", LOWEST_BITRATE);
        vorbisenc->min_bitrate = LOWEST_BITRATE;
      }
      if (vorbisenc->min_bitrate > 0 && vorbisenc->max_bitrate > 0)
        vorbisenc->managed = TRUE;
      else
        vorbisenc->managed = FALSE;

      if (old_value != vorbisenc->managed)
        g_object_notify (object, "managed");
      break;
    }
    case ARG_QUALITY:
      vorbisenc->quality = g_value_get_float (value);
      if (vorbisenc->quality >= 0.0)
        vorbisenc->quality_set = TRUE;
      else
        vorbisenc->quality_set = FALSE;
      break;
    case ARG_MANAGED:
      vorbisenc->managed = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
