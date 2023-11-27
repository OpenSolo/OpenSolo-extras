/* GStreamer AC3 parser
 * Copyright (C) 2009 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2009 Mark Nauwelaerts <mnauw users sf net>
 * Copyright (C) 2009 Nokia Corporation. All rights reserved.
 *   Contact: Stefan Kost <stefan.kost@nokia.com>
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
 * SECTION:element-ac3parse
 * @short_description: AC3 parser
 * @see_also: #GstAmrParse, #GstAACParse
 *
 * This is an AC3 parser.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch filesrc location=abc.ac3 ! ac3parse ! a52dec ! audioresample ! audioconvert ! autoaudiosink
 * ]|
 * </refsect2>
 */

/* TODO:
 *  - add support for audio/x-private1-ac3 as well
 *  - should accept framed and unframed input (needs decodebin fixes first)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstac3parse.h"
#include <gst/base/gstbytereader.h>
#include <gst/base/gstbitreader.h>

GST_DEBUG_CATEGORY_STATIC (ac3_parse_debug);
#define GST_CAT_DEFAULT ac3_parse_debug

static const struct
{
  const guint bit_rate;         /* nominal bit rate */
  const guint frame_size[3];    /* frame size for 32kHz, 44kHz, and 48kHz */
} frmsizcod_table[38] = {
  {
    32, {
  64, 69, 96}}, {
    32, {
  64, 70, 96}}, {
    40, {
  80, 87, 120}}, {
    40, {
  80, 88, 120}}, {
    48, {
  96, 104, 144}}, {
    48, {
  96, 105, 144}}, {
    56, {
  112, 121, 168}}, {
    56, {
  112, 122, 168}}, {
    64, {
  128, 139, 192}}, {
    64, {
  128, 140, 192}}, {
    80, {
  160, 174, 240}}, {
    80, {
  160, 175, 240}}, {
    96, {
  192, 208, 288}}, {
    96, {
  192, 209, 288}}, {
    112, {
  224, 243, 336}}, {
    112, {
  224, 244, 336}}, {
    128, {
  256, 278, 384}}, {
    128, {
  256, 279, 384}}, {
    160, {
  320, 348, 480}}, {
    160, {
  320, 349, 480}}, {
    192, {
  384, 417, 576}}, {
    192, {
  384, 418, 576}}, {
    224, {
  448, 487, 672}}, {
    224, {
  448, 488, 672}}, {
    256, {
  512, 557, 768}}, {
    256, {
  512, 558, 768}}, {
    320, {
  640, 696, 960}}, {
    320, {
  640, 697, 960}}, {
    384, {
  768, 835, 1152}}, {
    384, {
  768, 836, 1152}}, {
    448, {
  896, 975, 1344}}, {
    448, {
  896, 976, 1344}}, {
    512, {
  1024, 1114, 1536}}, {
    512, {
  1024, 1115, 1536}}, {
    576, {
  1152, 1253, 1728}}, {
    576, {
  1152, 1254, 1728}}, {
    640, {
  1280, 1393, 1920}}, {
    640, {
  1280, 1394, 1920}}
};

static const guint fscod_rates[4] = { 48000, 44100, 32000, 0 };
static const guint acmod_chans[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
static const guint numblks[4] = { 1, 2, 3, 6 };

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-ac3, framed = (boolean) true, "
        " channels = (int) [ 1, 6 ], rate = (int) [ 8000, 48000 ], "
        " alignment = (string) { iec61937, frame}; "
        "audio/x-eac3, framed = (boolean) true, "
        " channels = (int) [ 1, 6 ], rate = (int) [ 8000, 48000 ], "
        " alignment = (string) { iec61937, frame}; "));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-ac3; " "audio/x-eac3; " "audio/ac3"));

static void gst_ac3_parse_finalize (GObject * object);

static gboolean gst_ac3_parse_start (GstBaseParse * parse);
static gboolean gst_ac3_parse_stop (GstBaseParse * parse);
static gboolean gst_ac3_parse_check_valid_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, guint * size, gint * skipsize);
static GstFlowReturn gst_ac3_parse_parse_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame);
static gboolean gst_ac3_parse_src_event (GstBaseParse * parse,
    GstEvent * event);
static GstCaps *gst_ac3_parse_get_sink_caps (GstBaseParse * parse);

GST_BOILERPLATE (GstAc3Parse, gst_ac3_parse, GstBaseParse, GST_TYPE_BASE_PARSE);

static void
gst_ac3_parse_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_details_simple (element_class,
      "AC3 audio stream parser", "Codec/Parser/Converter/Audio",
      "AC3 parser", "Tim-Philipp Müller <tim centricular net>");
}

static void
gst_ac3_parse_class_init (GstAc3ParseClass * klass)
{
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (ac3_parse_debug, "ac3parse", 0,
      "AC3 audio stream parser");

  object_class->finalize = gst_ac3_parse_finalize;

  parse_class->start = GST_DEBUG_FUNCPTR (gst_ac3_parse_start);
  parse_class->stop = GST_DEBUG_FUNCPTR (gst_ac3_parse_stop);
  parse_class->check_valid_frame =
      GST_DEBUG_FUNCPTR (gst_ac3_parse_check_valid_frame);
  parse_class->parse_frame = GST_DEBUG_FUNCPTR (gst_ac3_parse_parse_frame);
  parse_class->src_event = GST_DEBUG_FUNCPTR (gst_ac3_parse_src_event);
  parse_class->get_sink_caps = GST_DEBUG_FUNCPTR (gst_ac3_parse_get_sink_caps);
}

static void
gst_ac3_parse_reset (GstAc3Parse * ac3parse)
{
  ac3parse->channels = -1;
  ac3parse->sample_rate = -1;
  ac3parse->blocks = -1;
  ac3parse->eac = FALSE;
  g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_NONE);
}

static void
gst_ac3_parse_init (GstAc3Parse * ac3parse, GstAc3ParseClass * klass)
{
  gst_base_parse_set_min_frame_size (GST_BASE_PARSE (ac3parse), 6);
  gst_ac3_parse_reset (ac3parse);
}

static void
gst_ac3_parse_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_ac3_parse_start (GstBaseParse * parse)
{
  GstAc3Parse *ac3parse = GST_AC3_PARSE (parse);

  GST_DEBUG_OBJECT (parse, "starting");

  gst_ac3_parse_reset (ac3parse);

  return TRUE;
}

static gboolean
gst_ac3_parse_stop (GstBaseParse * parse)
{
  GST_DEBUG_OBJECT (parse, "stopping");

  return TRUE;
}

static void
gst_ac3_parse_set_alignment (GstAc3Parse * ac3parse, gboolean eac)
{
  GstCaps *caps;
  GstStructure *st;
  const gchar *str = NULL;
  int i;

  if (G_LIKELY (!eac))
    goto done;

  caps = gst_pad_get_allowed_caps (GST_BASE_PARSE_SRC_PAD (ac3parse));

  if (!caps)
    goto done;

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    st = gst_caps_get_structure (caps, i);

    if (!g_str_equal (gst_structure_get_name (st), "audio/x-eac3"))
      continue;

    if ((str = gst_structure_get_string (st, "alignment"))) {
      if (g_str_equal (str, "iec61937")) {
        g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_IEC61937);
        GST_DEBUG_OBJECT (ac3parse, "picked iec61937 alignment");
      } else if (g_str_equal (str, "frame") == 0) {
        g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_FRAME);
        GST_DEBUG_OBJECT (ac3parse, "picked frame alignment");
      } else {
        g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_FRAME);
        GST_WARNING_OBJECT (ac3parse, "unknown alignment: %s", str);
      }
      break;
    }
  }

  if (caps)
    gst_caps_unref (caps);

done:
  /* default */
  if (ac3parse->align == GST_AC3_PARSE_ALIGN_NONE) {
    g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_FRAME);
    GST_DEBUG_OBJECT (ac3parse, "picked syncframe alignment");
  }
}

static gboolean
gst_ac3_parse_frame_header_ac3 (GstAc3Parse * ac3parse, GstBuffer * buf,
    gint skip, guint * frame_size, guint * rate, guint * chans, guint * blks,
    guint * sid)
{
  GstBitReader bits = GST_BIT_READER_INIT_FROM_BUFFER (buf);
  guint8 fscod, frmsizcod, bsid, acmod, lfe_on, rate_scale;

  GST_LOG_OBJECT (ac3parse, "parsing ac3");

  gst_bit_reader_skip_unchecked (&bits, skip * 8);

  gst_bit_reader_skip_unchecked (&bits, 16 + 16);
  fscod = gst_bit_reader_get_bits_uint8_unchecked (&bits, 2);
  frmsizcod = gst_bit_reader_get_bits_uint8_unchecked (&bits, 6);

  if (G_UNLIKELY (fscod == 3 || frmsizcod >= G_N_ELEMENTS (frmsizcod_table))) {
    GST_DEBUG_OBJECT (ac3parse, "bad fscod=%d frmsizcod=%d", fscod, frmsizcod);
    return FALSE;
  }

  bsid = gst_bit_reader_get_bits_uint8_unchecked (&bits, 5);
  gst_bit_reader_skip_unchecked (&bits, 3);     /* bsmod */
  acmod = gst_bit_reader_get_bits_uint8_unchecked (&bits, 3);

  /* spec not quite clear here: decoder should decode if less than 8,
   * but seemingly only defines 6 and 8 cases */
  /* Files with 9 and 10 happen, and seem to comply with the <= 8
     format, so let them through. The spec says nothing about 9 and 10 */
  if (bsid > 10) {
    GST_DEBUG_OBJECT (ac3parse, "unexpected bsid=%d", bsid);
    return FALSE;
  } else if (bsid != 8 && bsid != 6) {
    GST_DEBUG_OBJECT (ac3parse, "undefined bsid=%d", bsid);
  }

  if ((acmod & 0x1) && (acmod != 0x1))  /* 3 front channels */
    gst_bit_reader_skip_unchecked (&bits, 2);
  if ((acmod & 0x4))            /* if a surround channel exists */
    gst_bit_reader_skip_unchecked (&bits, 2);
  if (acmod == 0x2)             /* if in 2/0 mode */
    gst_bit_reader_skip_unchecked (&bits, 2);

  lfe_on = gst_bit_reader_get_bits_uint8_unchecked (&bits, 1);

  /* 6/8->0, 9->1, 10->2,
     see http://matroska.org/technical/specs/codecid/index.html */
  rate_scale = (CLAMP (bsid, 8, 10) - 8);

  if (frame_size)
    *frame_size = frmsizcod_table[frmsizcod].frame_size[fscod] * 2;
  if (rate)
    *rate = fscod_rates[fscod] >> rate_scale;
  if (chans)
    *chans = acmod_chans[acmod] + lfe_on;
  if (blks)
    *blks = 6;
  if (sid)
    *sid = 0;

  return TRUE;
}

static gboolean
gst_ac3_parse_frame_header_eac3 (GstAc3Parse * ac3parse, GstBuffer * buf,
    gint skip, guint * frame_size, guint * rate, guint * chans, guint * blks,
    guint * sid)
{
  GstBitReader bits = GST_BIT_READER_INIT_FROM_BUFFER (buf);
  guint16 frmsiz, sample_rate, blocks;
  guint8 strmtyp, fscod, fscod2, acmod, lfe_on, strmid, numblkscod;

  GST_LOG_OBJECT (ac3parse, "parsing e-ac3");

  gst_bit_reader_skip_unchecked (&bits, skip * 8);

  gst_bit_reader_skip_unchecked (&bits, 16);
  strmtyp = gst_bit_reader_get_bits_uint8_unchecked (&bits, 2); /* strmtyp     */
  if (G_UNLIKELY (strmtyp == 3)) {
    GST_DEBUG_OBJECT (ac3parse, "bad strmtyp %d", strmtyp);
    return FALSE;
  }

  strmid = gst_bit_reader_get_bits_uint8_unchecked (&bits, 3);  /* substreamid */
  frmsiz = gst_bit_reader_get_bits_uint16_unchecked (&bits, 11);        /* frmsiz      */
  fscod = gst_bit_reader_get_bits_uint8_unchecked (&bits, 2);   /* fscod       */
  if (fscod == 3) {
    fscod2 = gst_bit_reader_get_bits_uint8_unchecked (&bits, 2);        /* fscod2      */
    if (G_UNLIKELY (fscod2 == 3)) {
      GST_DEBUG_OBJECT (ac3parse, "invalid fscod2");
      return FALSE;
    }
    sample_rate = fscod_rates[fscod2] / 2;
    blocks = 6;
  } else {
    numblkscod = gst_bit_reader_get_bits_uint8_unchecked (&bits, 2);    /* numblkscod  */
    sample_rate = fscod_rates[fscod];
    blocks = numblks[numblkscod];
  }

  acmod = gst_bit_reader_get_bits_uint8_unchecked (&bits, 3);   /* acmod       */
  lfe_on = gst_bit_reader_get_bits_uint8_unchecked (&bits, 1);  /* lfeon       */

  gst_bit_reader_skip_unchecked (&bits, 5);     /* bsid        */

  if (frame_size)
    *frame_size = (frmsiz + 1) * 2;
  if (rate)
    *rate = sample_rate;
  if (chans)
    *chans = acmod_chans[acmod] + lfe_on;
  if (blks)
    *blks = blocks;
  if (sid)
    *sid = (strmtyp & 0x1) << 3 | strmid;

  return TRUE;
}

static gboolean
gst_ac3_parse_frame_header (GstAc3Parse * parse, GstBuffer * buf, gint skip,
    guint * framesize, guint * rate, guint * chans, guint * blocks,
    guint * sid, gboolean * eac)
{
  GstBitReader bits = GST_BIT_READER_INIT_FROM_BUFFER (buf);
  guint16 sync;
  guint8 bsid;

  GST_MEMDUMP_OBJECT (parse, "AC3 frame sync", GST_BUFFER_DATA (buf), 16);

  gst_bit_reader_skip_unchecked (&bits, skip * 8);

  sync = gst_bit_reader_get_bits_uint16_unchecked (&bits, 16);
  gst_bit_reader_skip_unchecked (&bits, 16 + 8);
  bsid = gst_bit_reader_peek_bits_uint8_unchecked (&bits, 5);

  if (G_UNLIKELY (sync != 0x0b77))
    return FALSE;

  GST_LOG_OBJECT (parse, "bsid = %d", bsid);

  if (bsid <= 10) {
    if (eac)
      *eac = FALSE;
    return gst_ac3_parse_frame_header_ac3 (parse, buf, skip, framesize, rate,
        chans, blocks, sid);
  } else if (bsid <= 16) {
    if (eac)
      *eac = TRUE;
    return gst_ac3_parse_frame_header_eac3 (parse, buf, skip, framesize, rate,
        chans, blocks, sid);
  } else {
    GST_DEBUG_OBJECT (parse, "unexpected bsid %d", bsid);
    return FALSE;
  }
}

static gboolean
gst_ac3_parse_check_valid_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, guint * framesize, gint * skipsize)
{
  GstAc3Parse *ac3parse = GST_AC3_PARSE (parse);
  GstBuffer *buf = frame->buffer;
  GstByteReader reader = GST_BYTE_READER_INIT_FROM_BUFFER (buf);
  gint off;
  gboolean lost_sync, draining, eac, more = FALSE;
  guint frmsiz, blocks, sid;
  gint have_blocks = 0;

  if (G_UNLIKELY (GST_BUFFER_SIZE (buf) < 6))
    return FALSE;

  off = gst_byte_reader_masked_scan_uint32 (&reader, 0xffff0000, 0x0b770000,
      0, GST_BUFFER_SIZE (buf));

  GST_LOG_OBJECT (parse, "possible sync at buffer offset %d", off);

  /* didn't find anything that looks like a sync word, skip */
  if (off < 0) {
    *skipsize = GST_BUFFER_SIZE (buf) - 3;
    return FALSE;
  }

  /* possible frame header, but not at offset 0? skip bytes before sync */
  if (off > 0) {
    *skipsize = off;
    return FALSE;
  }

  /* make sure the values in the frame header look sane */
  if (!gst_ac3_parse_frame_header (ac3parse, buf, 0, &frmsiz, NULL, NULL,
          &blocks, &sid, &eac)) {
    *skipsize = off + 2;
    return FALSE;
  }

  *framesize = frmsiz;

  if (G_UNLIKELY (g_atomic_int_get (&ac3parse->align) ==
          GST_AC3_PARSE_ALIGN_NONE))
    gst_ac3_parse_set_alignment (ac3parse, eac);

  GST_LOG_OBJECT (parse, "got frame");

  lost_sync = GST_BASE_PARSE_LOST_SYNC (parse);
  draining = GST_BASE_PARSE_DRAINING (parse);

  if (g_atomic_int_get (&ac3parse->align) == GST_AC3_PARSE_ALIGN_IEC61937) {
    /* We need 6 audio blocks from each substream, so we keep going forwards
     * till we have it */

    g_assert (blocks > 0);
    GST_LOG_OBJECT (ac3parse, "Need %d frames before pushing", 6 / blocks);

    if (sid != 0) {
      /* We need the first substream to be the one with id 0 */
      GST_LOG_OBJECT (ac3parse, "Skipping till we find sid 0");
      *skipsize = off + 2;
      return FALSE;
    }

    *framesize = 0;

    /* Loop till we have 6 blocks per substream */
    for (have_blocks = 0; !more && have_blocks < 6; have_blocks += blocks) {
      /* Loop till we get one frame from each substream */
      do {
        *framesize += frmsiz;

        if (!gst_byte_reader_skip (&reader, frmsiz) ||
            GST_BUFFER_SIZE (buf) < (*framesize + 6)) {
          more = TRUE;
          break;
        }

        if (!gst_ac3_parse_frame_header (ac3parse, buf, *framesize, &frmsiz,
                NULL, NULL, NULL, &sid, &eac)) {
          *skipsize = off + 2;
          return FALSE;
        }
      } while (sid);
    }

    /* We're now at the next frame, so no need to skip if resyncing */
    frmsiz = 0;
  }

  if (lost_sync && !draining) {
    guint16 word = 0;

    GST_DEBUG_OBJECT (ac3parse, "resyncing; checking next frame syncword");

    if (more || !gst_byte_reader_skip (&reader, frmsiz) ||
        !gst_byte_reader_get_uint16_be (&reader, &word)) {
      GST_DEBUG_OBJECT (ac3parse, "... but not sufficient data");
      gst_base_parse_set_min_frame_size (parse, *framesize + 6);
      *skipsize = 0;
      return FALSE;
    } else {
      if (word != 0x0b77) {
        GST_DEBUG_OBJECT (ac3parse, "0x%x not OK", word);
        *skipsize = off + 2;
        return FALSE;
      } else {
        /* ok, got sync now, let's assume constant frame size */
        gst_base_parse_set_min_frame_size (parse, *framesize);
      }
    }
  }

  return TRUE;
}

static GstFlowReturn
gst_ac3_parse_parse_frame (GstBaseParse * parse, GstBaseParseFrame * frame)
{
  GstAc3Parse *ac3parse = GST_AC3_PARSE (parse);
  GstBuffer *buf = frame->buffer;
  guint fsize, rate, chans, blocks, sid;
  gboolean eac, update_rate = FALSE;

  if (!gst_ac3_parse_frame_header (ac3parse, buf, 0, &fsize, &rate, &chans,
          &blocks, &sid, &eac))
    goto broken_header;

  GST_LOG_OBJECT (parse, "size: %u, blocks: %u, rate: %u, chans: %u", fsize,
      blocks, rate, chans);

  if (G_UNLIKELY (sid)) {
    /* dependent frame, no need to (ac)count for or consider further */
    GST_LOG_OBJECT (parse, "sid: %d", sid);
    frame->flags |= GST_BASE_PARSE_FRAME_FLAG_NO_FRAME;
    /* TODO maybe also mark as DELTA_UNIT,
     * if that does not surprise baseparse elsewhere */
    /* occupies same time space as previous base frame */
    if (G_LIKELY (GST_BUFFER_TIMESTAMP (buf) >= GST_BUFFER_DURATION (buf)))
      GST_BUFFER_TIMESTAMP (buf) -= GST_BUFFER_DURATION (buf);
    /* only return if we already arranged for caps */
    if (G_LIKELY (ac3parse->sample_rate > 0))
      return GST_FLOW_OK;
  }

  if (G_UNLIKELY (ac3parse->sample_rate != rate || ac3parse->channels != chans
          || ac3parse->eac != eac)) {
    GstCaps *caps = gst_caps_new_simple (eac ? "audio/x-eac3" : "audio/x-ac3",
        "framed", G_TYPE_BOOLEAN, TRUE, "rate", G_TYPE_INT, rate,
        "channels", G_TYPE_INT, chans, NULL);
    gst_caps_set_simple (caps, "alignment", G_TYPE_STRING,
        g_atomic_int_get (&ac3parse->align) == GST_AC3_PARSE_ALIGN_IEC61937 ?
        "iec61937" : "frame", NULL);
    gst_buffer_set_caps (buf, caps);
    gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (parse), caps);
    gst_caps_unref (caps);

    ac3parse->sample_rate = rate;
    ac3parse->channels = chans;
    ac3parse->eac = eac;

    update_rate = TRUE;
  }

  if (G_UNLIKELY (ac3parse->blocks != blocks)) {
    ac3parse->blocks = blocks;

    update_rate = TRUE;
  }

  if (G_UNLIKELY (update_rate))
    gst_base_parse_set_frame_rate (parse, rate, 256 * blocks, 2, 2);

  return GST_FLOW_OK;

/* ERRORS */
broken_header:
  {
    /* this really shouldn't ever happen */
    GST_ELEMENT_ERROR (parse, STREAM, DECODE, (NULL), (NULL));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_ac3_parse_src_event (GstBaseParse * parse, GstEvent * event)
{
  GstAc3Parse *ac3parse = GST_AC3_PARSE (parse);

  if (G_UNLIKELY (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_UPSTREAM) &&
      gst_event_has_name (event, "ac3parse-set-alignment")) {
    const GstStructure *st = gst_event_get_structure (event);
    const gchar *align = gst_structure_get_string (st, "alignment");

    if (g_str_equal (align, "iec61937")) {
      GST_DEBUG_OBJECT (ac3parse, "Switching to iec61937 alignment");
      g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_IEC61937);
    } else if (g_str_equal (align, "frame")) {
      GST_DEBUG_OBJECT (ac3parse, "Switching to frame alignment");
      g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_FRAME);
    } else {
      g_atomic_int_set (&ac3parse->align, GST_AC3_PARSE_ALIGN_FRAME);
      GST_WARNING_OBJECT (ac3parse, "Got unknown alignment request (%s) "
          "reverting to frame alignment.",
          gst_structure_get_string (st, "alignment"));
    }

    gst_event_unref (event);
    return TRUE;
  }

  return GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);
}

static GstCaps *
gst_ac3_parse_get_sink_caps (GstBaseParse * parse)
{
  GstCaps *peercaps;
  GstCaps *res;

  peercaps = gst_pad_get_allowed_caps (GST_BASE_PARSE_SRC_PAD (parse));
  if (peercaps) {
    guint i, n;

    /* Remove the framed and alignment field. We can convert
     * between different alignments. */
    peercaps = gst_caps_make_writable (peercaps);
    n = gst_caps_get_size (peercaps);
    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (peercaps, i);

      gst_structure_remove_field (s, "framed");
      gst_structure_remove_field (s, "alignment");
    }

    res =
        gst_caps_intersect_full (peercaps,
        gst_pad_get_pad_template_caps (GST_BASE_PARSE_SRC_PAD (parse)),
        GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (peercaps);
  } else {
    res =
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_PARSE_SINK_PAD
            (parse)));
  }

  return res;
}
