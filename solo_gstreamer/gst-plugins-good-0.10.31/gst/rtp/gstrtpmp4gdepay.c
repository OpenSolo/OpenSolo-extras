/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpmp4gdepay.h"

GST_DEBUG_CATEGORY_STATIC (rtpmp4gdepay_debug);
#define GST_CAT_DEFAULT (rtpmp4gdepay_debug)

static GstStaticPadTemplate gst_rtp_mp4g_depay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg,"
        "mpegversion=(int) 4,"
        "systemstream=(boolean)false;"
        "audio/mpeg," "mpegversion=(int) 4, " "stream-format=(string)raw")
    );

static GstStaticPadTemplate gst_rtp_mp4g_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) { \"video\", \"audio\", \"application\" }, "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [1, MAX ], "
        "encoding-name = (string) \"MPEG4-GENERIC\", "
        /* required string params */
        "streamtype = (string) { \"4\", \"5\" }, "      /* 4 = video, 5 = audio */
        /* "profile-level-id = (string) [1,MAX], " */
        /* "config = (string) [1,MAX]" */
        "mode = (string) { \"generic\", \"CELP-cbr\", \"CELP-vbr\", \"AAC-lbr\", \"AAC-hbr\" } "
        /* Optional general parameters */
        /* "objecttype = (string) [1,MAX], " */
        /* "constantsize = (string) [1,MAX], " *//* constant size of each AU */
        /* "constantduration = (string) [1,MAX], " *//* constant duration of each AU */
        /* "maxdisplacement = (string) [1,MAX], " */
        /* "de-interleavebuffersize = (string) [1,MAX], " */
        /* Optional configuration parameters */
        /* "sizelength = (string) [1, 32], " */
        /* "indexlength = (string) [1, 32], " */
        /* "indexdeltalength = (string) [1, 32], " */
        /* "ctsdeltalength = (string) [1, 32], " */
        /* "dtsdeltalength = (string) [1, 32], " */
        /* "randomaccessindication = (string) {0, 1}, " */
        /* "streamstateindication = (string) [0, 32], " */
        /* "auxiliarydatasizelength = (string) [0, 32]" */ )
    );

/* simple bitstream parser */
typedef struct
{
  const guint8 *data;
  const guint8 *end;
  gint head;                    /* bitpos in the cache of next bit */
  guint64 cache;                /* cached bytes */
} GstBsParse;

static void
gst_bs_parse_init (GstBsParse * bs, const guint8 * data, guint size)
{
  bs->data = data;
  bs->end = data + size;
  bs->head = 0;
  bs->cache = 0xffffffff;
}

static guint32
gst_bs_parse_read (GstBsParse * bs, guint n)
{
  guint32 res = 0;
  gint shift;

  if (n == 0)
    return res;

  /* fill up the cache if we need to */
  while (bs->head < n) {
    if (bs->data >= bs->end) {
      /* we're at the end, can't produce more than head number of bits */
      n = bs->head;
      break;
    }
    /* shift bytes in cache, moving the head bits of the cache left */
    bs->cache = (bs->cache << 8) | *bs->data++;
    bs->head += 8;
  }

  /* bring the required bits down and truncate */
  if ((shift = bs->head - n) > 0)
    res = bs->cache >> shift;
  else
    res = bs->cache;

  /* mask out required bits */
  if (n < 32)
    res &= (1 << n) - 1;

  bs->head = shift;

  return res;
}


GST_BOILERPLATE (GstRtpMP4GDepay, gst_rtp_mp4g_depay, GstBaseRTPDepayload,
    GST_TYPE_BASE_RTP_DEPAYLOAD);

static void gst_rtp_mp4g_depay_finalize (GObject * object);

static gboolean gst_rtp_mp4g_depay_setcaps (GstBaseRTPDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_mp4g_depay_process (GstBaseRTPDepayload * depayload,
    GstBuffer * buf);
static gboolean gst_rtp_mp4g_depay_handle_event (GstBaseRTPDepayload * filter,
    GstEvent * event);

static GstStateChangeReturn gst_rtp_mp4g_depay_change_state (GstElement *
    element, GstStateChange transition);


static void
gst_rtp_mp4g_depay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_mp4g_depay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_mp4g_depay_sink_template);

  gst_element_class_set_details_simple (element_class,
      "RTP MPEG4 ES depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts MPEG4 elementary streams from RTP packets (RFC 3640)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_mp4g_depay_class_init (GstRtpMP4GDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseRTPDepayloadClass *gstbasertpdepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasertpdepayload_class = (GstBaseRTPDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mp4g_depay_finalize;

  gstelement_class->change_state = gst_rtp_mp4g_depay_change_state;

  gstbasertpdepayload_class->process = gst_rtp_mp4g_depay_process;
  gstbasertpdepayload_class->set_caps = gst_rtp_mp4g_depay_setcaps;
  gstbasertpdepayload_class->handle_event = gst_rtp_mp4g_depay_handle_event;

  GST_DEBUG_CATEGORY_INIT (rtpmp4gdepay_debug, "rtpmp4gdepay", 0,
      "MP4-generic RTP Depayloader");
}

static void
gst_rtp_mp4g_depay_init (GstRtpMP4GDepay * rtpmp4gdepay,
    GstRtpMP4GDepayClass * klass)
{
  rtpmp4gdepay->adapter = gst_adapter_new ();
  rtpmp4gdepay->packets = g_queue_new ();
}

static void
gst_rtp_mp4g_depay_finalize (GObject * object)
{
  GstRtpMP4GDepay *rtpmp4gdepay;

  rtpmp4gdepay = GST_RTP_MP4G_DEPAY (object);

  g_object_unref (rtpmp4gdepay->adapter);
  rtpmp4gdepay->adapter = NULL;
  g_queue_free (rtpmp4gdepay->packets);
  rtpmp4gdepay->packets = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
gst_rtp_mp4g_depay_parse_int (GstStructure * structure, const gchar * field,
    gint def)
{
  const gchar *str;
  gint res;

  if ((str = gst_structure_get_string (structure, field)))
    return atoi (str);

  if (gst_structure_get_int (structure, field, &res))
    return res;

  return def;
}

static gboolean
gst_rtp_mp4g_depay_setcaps (GstBaseRTPDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpMP4GDepay *rtpmp4gdepay;
  GstCaps *srccaps = NULL;
  const gchar *str;
  gint clock_rate;
  gint someint;
  gboolean res;

  rtpmp4gdepay = GST_RTP_MP4G_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;         /* default */
  depayload->clock_rate = clock_rate;

  if ((str = gst_structure_get_string (structure, "media"))) {
    if (strcmp (str, "audio") == 0) {
      srccaps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, 4, "stream-format", G_TYPE_STRING, "raw",
          NULL);
    } else if (strcmp (str, "video") == 0) {
      srccaps = gst_caps_new_simple ("video/mpeg",
          "mpegversion", G_TYPE_INT, 4,
          "systemstream", G_TYPE_BOOLEAN, FALSE, NULL);
    }
  }
  if (srccaps == NULL)
    goto unknown_media;

  /* these values are optional and have a default value of 0 (no header) */
  rtpmp4gdepay->sizelength =
      gst_rtp_mp4g_depay_parse_int (structure, "sizelength", 0);
  rtpmp4gdepay->indexlength =
      gst_rtp_mp4g_depay_parse_int (structure, "indexlength", 0);
  rtpmp4gdepay->indexdeltalength =
      gst_rtp_mp4g_depay_parse_int (structure, "indexdeltalength", 0);
  rtpmp4gdepay->ctsdeltalength =
      gst_rtp_mp4g_depay_parse_int (structure, "ctsdeltalength", 0);
  rtpmp4gdepay->dtsdeltalength =
      gst_rtp_mp4g_depay_parse_int (structure, "dtsdeltalength", 0);
  someint =
      gst_rtp_mp4g_depay_parse_int (structure, "randomaccessindication", 0);
  rtpmp4gdepay->randomaccessindication = someint > 0 ? 1 : 0;
  rtpmp4gdepay->streamstateindication =
      gst_rtp_mp4g_depay_parse_int (structure, "streamstateindication", 0);
  rtpmp4gdepay->auxiliarydatasizelength =
      gst_rtp_mp4g_depay_parse_int (structure, "auxiliarydatasizelength", 0);
  rtpmp4gdepay->constantSize =
      gst_rtp_mp4g_depay_parse_int (structure, "constantsize", 0);
  rtpmp4gdepay->constantDuration =
      gst_rtp_mp4g_depay_parse_int (structure, "constantduration", 0);
  rtpmp4gdepay->maxDisplacement =
      gst_rtp_mp4g_depay_parse_int (structure, "maxdisplacement", 0);


  /* get config string */
  if ((str = gst_structure_get_string (structure, "config"))) {
    GValue v = { 0 };

    g_value_init (&v, GST_TYPE_BUFFER);
    if (gst_value_deserialize (&v, str)) {
      GstBuffer *buffer;

      buffer = gst_value_get_buffer (&v);
      gst_caps_set_simple (srccaps,
          "codec_data", GST_TYPE_BUFFER, buffer, NULL);
      g_value_unset (&v);
    } else {
      g_warning ("cannot convert config to buffer");
    }
  }

  res = gst_pad_set_caps (depayload->srcpad, srccaps);
  gst_caps_unref (srccaps);

  return res;

  /* ERRORS */
unknown_media:
  {
    GST_DEBUG_OBJECT (rtpmp4gdepay, "Unknown media type");
    return FALSE;
  }
}

static void
gst_rtp_mp4g_depay_clear_queue (GstRtpMP4GDepay * rtpmp4gdepay)
{
  GstBuffer *outbuf;

  while ((outbuf = g_queue_pop_head (rtpmp4gdepay->packets)))
    gst_buffer_unref (outbuf);
}

static void
gst_rtp_mp4g_depay_reset (GstRtpMP4GDepay * rtpmp4gdepay)
{
  gst_adapter_clear (rtpmp4gdepay->adapter);
  rtpmp4gdepay->max_AU_index = -1;
  rtpmp4gdepay->next_AU_index = -1;
  rtpmp4gdepay->prev_AU_index = -1;
  rtpmp4gdepay->prev_rtptime = -1;
  rtpmp4gdepay->last_AU_index = -1;
  gst_rtp_mp4g_depay_clear_queue (rtpmp4gdepay);
}

static void
gst_rtp_mp4g_depay_flush_queue (GstRtpMP4GDepay * rtpmp4gdepay)
{
  GstBuffer *outbuf;
  gboolean discont = FALSE;
  guint AU_index;

  while ((outbuf = g_queue_pop_head (rtpmp4gdepay->packets))) {
    AU_index = GST_BUFFER_OFFSET (outbuf);

    GST_DEBUG_OBJECT (rtpmp4gdepay, "next available AU_index %u", AU_index);

    if (rtpmp4gdepay->next_AU_index != AU_index) {
      GST_DEBUG_OBJECT (rtpmp4gdepay, "discont, expected AU_index %u",
          rtpmp4gdepay->next_AU_index);
      discont = TRUE;
    }

    if (discont) {
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
      discont = FALSE;
    }

    GST_DEBUG_OBJECT (rtpmp4gdepay, "pushing AU_index %u", AU_index);
    gst_base_rtp_depayload_push (GST_BASE_RTP_DEPAYLOAD (rtpmp4gdepay), outbuf);
    rtpmp4gdepay->next_AU_index = AU_index + 1;
  }
}

static void
gst_rtp_mp4g_depay_queue (GstRtpMP4GDepay * rtpmp4gdepay, GstBuffer * outbuf)
{
  guint AU_index = GST_BUFFER_OFFSET (outbuf);

  if (rtpmp4gdepay->next_AU_index == -1) {
    GST_DEBUG_OBJECT (rtpmp4gdepay, "Init AU counter %u", AU_index);
    rtpmp4gdepay->next_AU_index = AU_index;
  }

  if (rtpmp4gdepay->next_AU_index == AU_index) {
    GST_DEBUG_OBJECT (rtpmp4gdepay, "pushing expected AU_index %u", AU_index);

    /* we received the expected packet, push it and flush as much as we can from
     * the queue */
    gst_base_rtp_depayload_push (GST_BASE_RTP_DEPAYLOAD (rtpmp4gdepay), outbuf);
    rtpmp4gdepay->next_AU_index++;

    while ((outbuf = g_queue_peek_head (rtpmp4gdepay->packets))) {
      AU_index = GST_BUFFER_OFFSET (outbuf);

      GST_DEBUG_OBJECT (rtpmp4gdepay, "next available AU_index %u", AU_index);

      if (rtpmp4gdepay->next_AU_index == AU_index) {
        GST_DEBUG_OBJECT (rtpmp4gdepay, "pushing expected AU_index %u",
            AU_index);
        outbuf = g_queue_pop_head (rtpmp4gdepay->packets);
        gst_base_rtp_depayload_push (GST_BASE_RTP_DEPAYLOAD (rtpmp4gdepay),
            outbuf);
        rtpmp4gdepay->next_AU_index++;
      } else {
        GST_DEBUG_OBJECT (rtpmp4gdepay, "waiting for next AU_index %u",
            rtpmp4gdepay->next_AU_index);
        break;
      }
    }
  } else {
    GList *list;

    GST_DEBUG_OBJECT (rtpmp4gdepay, "queueing AU_index %u", AU_index);

    /* loop the list to skip strictly smaller AU_index buffers */
    for (list = rtpmp4gdepay->packets->head; list; list = g_list_next (list)) {
      guint idx;
      gint gap;

      idx = GST_BUFFER_OFFSET (GST_BUFFER_CAST (list->data));

      /* compare the new seqnum to the one in the buffer */
      gap = (gint) (idx - AU_index);

      GST_DEBUG_OBJECT (rtpmp4gdepay, "compare with AU_index %u, gap %d", idx,
          gap);

      /* AU_index <= idx, we can stop looking */
      if (G_LIKELY (gap > 0))
        break;
    }
    if (G_LIKELY (list))
      g_queue_insert_before (rtpmp4gdepay->packets, list, outbuf);
    else
      g_queue_push_tail (rtpmp4gdepay->packets, outbuf);
  }
}

static GstBuffer *
gst_rtp_mp4g_depay_process (GstBaseRTPDepayload * depayload, GstBuffer * buf)
{
  GstRtpMP4GDepay *rtpmp4gdepay;
  GstBuffer *outbuf;
  GstClockTime timestamp;

  rtpmp4gdepay = GST_RTP_MP4G_DEPAY (depayload);

  /* flush remaining data on discont */
  if (GST_BUFFER_IS_DISCONT (buf)) {
    GST_DEBUG_OBJECT (rtpmp4gdepay, "received DISCONT");
    gst_adapter_clear (rtpmp4gdepay->adapter);
  }

  timestamp = GST_BUFFER_TIMESTAMP (buf);

  {
    gint payload_len, payload_AU;
    guint8 *payload;
    guint32 rtptime;
    guint AU_headers_len;
    guint AU_size, AU_index, AU_index_delta, payload_AU_size;
    gboolean M;

    payload_len = gst_rtp_buffer_get_payload_len (buf);
    payload = gst_rtp_buffer_get_payload (buf);

    GST_DEBUG_OBJECT (rtpmp4gdepay, "received payload of %d", payload_len);

    rtptime = gst_rtp_buffer_get_timestamp (buf);
    M = gst_rtp_buffer_get_marker (buf);

    if (rtpmp4gdepay->sizelength > 0) {
      gint num_AU_headers, AU_headers_bytes, i;
      GstBsParse bs;

      if (payload_len < 2)
        goto short_payload;

      /* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
       * |AU-headers-length|AU-header|AU-header|      |AU-header|padding|
       * |                 |   (1)   |   (2)   |      |   (n) * | bits  |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
       *
       * The length is 2 bytes and contains the length of the following
       * AU-headers in bits.
       */
      AU_headers_len = (payload[0] << 8) | payload[1];
      AU_headers_bytes = (AU_headers_len + 7) / 8;
      num_AU_headers = AU_headers_len / 16;

      GST_DEBUG_OBJECT (rtpmp4gdepay, "AU headers len %d, bytes %d, num %d",
          AU_headers_len, AU_headers_bytes, num_AU_headers);

      /* skip header */
      payload += 2;
      payload_len -= 2;

      if (payload_len < AU_headers_bytes)
        goto short_payload;

      /* skip special headers, point to first payload AU */
      payload_AU = 2 + AU_headers_bytes;
      payload_AU_size = payload_len - AU_headers_bytes;

      if (G_UNLIKELY (rtpmp4gdepay->auxiliarydatasizelength)) {
        gint aux_size;

        /* point the bitstream parser to the first auxiliary data bit */
        gst_bs_parse_init (&bs, payload + AU_headers_bytes,
            payload_len - AU_headers_bytes);
        aux_size =
            gst_bs_parse_read (&bs, rtpmp4gdepay->auxiliarydatasizelength);
        /* convert to bytes */
        aux_size = (aux_size + 7) / 8;
        /* AU data then follows auxiliary data */
        if (payload_AU_size < aux_size)
          goto short_payload;
        payload_AU += aux_size;
        payload_AU_size -= aux_size;
      }

      /* point the bitstream parser to the first AU header bit */
      gst_bs_parse_init (&bs, payload, payload_len);
      AU_index = AU_index_delta = 0;

      for (i = 0; i < num_AU_headers && payload_AU_size > 0; i++) {
        /* parse AU header
         *  +---------------------------------------+
         *  |     AU-size                           |
         *  +---------------------------------------+
         *  |     AU-Index / AU-Index-delta         |
         *  +---------------------------------------+
         *  |     CTS-flag                          |
         *  +---------------------------------------+
         *  |     CTS-delta                         |
         *  +---------------------------------------+
         *  |     DTS-flag                          |
         *  +---------------------------------------+
         *  |     DTS-delta                         |
         *  +---------------------------------------+
         *  |     RAP-flag                          |
         *  +---------------------------------------+
         *  |     Stream-state                      |
         *  +---------------------------------------+
         */
        AU_size = gst_bs_parse_read (&bs, rtpmp4gdepay->sizelength);

        /* calculate the AU_index, which is only on the first AU of the packet
         * and the AU_index_delta on the other AUs. This will be used to
         * reconstruct the AU ordering when interleaving. */
        if (i == 0) {
          AU_index = gst_bs_parse_read (&bs, rtpmp4gdepay->indexlength);

          GST_DEBUG_OBJECT (rtpmp4gdepay, "AU index %u", AU_index);

          if (AU_index == 0 && rtpmp4gdepay->prev_AU_index == 0) {
            gint diff;
            gint cd;

            /* if we see two consecutive packets with AU_index of 0, we can
             * assume we have constantDuration packets. Since we don't have
             * the index we must use the AU duration to calculate the
             * index. Get the diff between the timestamps first, this can be
             * positive or negative. */
            if (rtpmp4gdepay->prev_rtptime <= rtptime)
              diff = rtptime - rtpmp4gdepay->prev_rtptime;
            else
              diff = -(rtpmp4gdepay->prev_rtptime - rtptime);

            /* if no constantDuration was given, make one */
            if (rtpmp4gdepay->constantDuration != 0) {
              cd = rtpmp4gdepay->constantDuration;
              GST_DEBUG_OBJECT (depayload, "using constantDuration %d", cd);
            } else if (rtpmp4gdepay->prev_AU_num > 0) {
              /* use number of packets and of previous frame */
              cd = diff / rtpmp4gdepay->prev_AU_num;
              GST_DEBUG_OBJECT (depayload, "guessing constantDuration %d", cd);
            } else {
              /* assume this frame has the same number of packets as the
               * previous one */
              cd = diff / num_AU_headers;
              GST_DEBUG_OBJECT (depayload, "guessing constantDuration %d", cd);
            }

            if (cd > 0) {
              /* get the number of packets by dividing with the duration */
              diff /= cd;
            } else {
              diff = 0;
            }

            rtpmp4gdepay->last_AU_index += diff;
            rtpmp4gdepay->prev_AU_index = AU_index;

            AU_index = rtpmp4gdepay->last_AU_index;

            GST_DEBUG_OBJECT (rtpmp4gdepay, "diff %d, AU index %u", diff,
                AU_index);
          } else {
            rtpmp4gdepay->prev_AU_index = AU_index;
            rtpmp4gdepay->last_AU_index = AU_index;
          }

          /* keep track of the higest AU_index */
          if (rtpmp4gdepay->max_AU_index != -1
              && rtpmp4gdepay->max_AU_index <= AU_index) {
            GST_DEBUG_OBJECT (rtpmp4gdepay, "new interleave group, flushing");
            /* a new interleave group started, flush */
            gst_rtp_mp4g_depay_flush_queue (rtpmp4gdepay);
          }
          if (G_UNLIKELY (!rtpmp4gdepay->maxDisplacement &&
                  rtpmp4gdepay->max_AU_index != -1
                  && rtpmp4gdepay->max_AU_index >= AU_index)) {
            GstBuffer *outbuf;

            /* some broken non-interleaved streams have AU-index jumping around
             * all over the place, apparently assuming receiver disregards */
            GST_DEBUG_OBJECT (rtpmp4gdepay, "non-interleaved broken AU indices;"
                " forcing continuous flush");
            /* reset AU to avoid repeated DISCONT in such case */
            outbuf = g_queue_peek_head (rtpmp4gdepay->packets);
            if (G_LIKELY (outbuf)) {
              rtpmp4gdepay->next_AU_index = GST_BUFFER_OFFSET (outbuf);
              gst_rtp_mp4g_depay_flush_queue (rtpmp4gdepay);
            }
            /* rebase next_AU_index to current rtp's first AU_index */
            rtpmp4gdepay->next_AU_index = AU_index;
          }
          rtpmp4gdepay->prev_rtptime = rtptime;
          rtpmp4gdepay->prev_AU_num = num_AU_headers;
        } else {
          AU_index_delta =
              gst_bs_parse_read (&bs, rtpmp4gdepay->indexdeltalength);
          AU_index += AU_index_delta + 1;
        }
        /* keep track of highest AU_index */
        if (rtpmp4gdepay->max_AU_index == -1
            || AU_index > rtpmp4gdepay->max_AU_index)
          rtpmp4gdepay->max_AU_index = AU_index;

        /* the presentation time offset, a 2s-complement value, we need this to
         * calculate the timestamp on the output packet. */
        if (rtpmp4gdepay->ctsdeltalength > 0) {
          if (gst_bs_parse_read (&bs, 1))
            gst_bs_parse_read (&bs, rtpmp4gdepay->ctsdeltalength);
        }
        /* the decoding time offset, a 2s-complement value */
        if (rtpmp4gdepay->dtsdeltalength > 0) {
          if (gst_bs_parse_read (&bs, 1))
            gst_bs_parse_read (&bs, rtpmp4gdepay->dtsdeltalength);
        }
        /* RAP-flag to indicate that the AU contains a keyframe */
        if (rtpmp4gdepay->randomaccessindication)
          gst_bs_parse_read (&bs, 1);
        /* stream-state */
        if (rtpmp4gdepay->streamstateindication > 0)
          gst_bs_parse_read (&bs, rtpmp4gdepay->streamstateindication);

        GST_DEBUG_OBJECT (rtpmp4gdepay, "size %d, index %d, delta %d", AU_size,
            AU_index, AU_index_delta);

        /* fragmented pakets have the AU_size set to the size of the
         * unfragmented AU. */
        if (AU_size > payload_AU_size)
          AU_size = payload_AU_size;

        /* collect stuff in the adapter, strip header from payload and push in
         * the adapter */
        outbuf =
            gst_rtp_buffer_get_payload_subbuffer (buf, payload_AU, AU_size);
        gst_adapter_push (rtpmp4gdepay->adapter, outbuf);

        if (M) {
          guint avail;

          /* packet is complete, flush */
          avail = gst_adapter_available (rtpmp4gdepay->adapter);

          outbuf = gst_adapter_take_buffer (rtpmp4gdepay->adapter, avail);
          gst_buffer_set_caps (outbuf, GST_PAD_CAPS (depayload->srcpad));

          /* copy some of the fields we calculated above on the buffer. We also
           * copy the AU_index so that we can sort the packets in our queue. */
          GST_BUFFER_TIMESTAMP (outbuf) = timestamp;
          GST_BUFFER_OFFSET (outbuf) = AU_index;

          /* make sure we don't use the timestamp again for other AUs in this
           * RTP packet. */
          timestamp = -1;

          GST_DEBUG_OBJECT (depayload, "pushing buffer of size %d",
              GST_BUFFER_SIZE (outbuf));

          gst_rtp_mp4g_depay_queue (rtpmp4gdepay, outbuf);

        }
        payload_AU += AU_size;
        payload_AU_size -= AU_size;
      }
    } else {
      /* push complete buffer in adapter */
      outbuf = gst_rtp_buffer_get_payload_subbuffer (buf, 0, payload_len);
      gst_adapter_push (rtpmp4gdepay->adapter, outbuf);

      /* if this was the last packet of the VOP, create and push a buffer */
      if (M) {
        guint avail;

        avail = gst_adapter_available (rtpmp4gdepay->adapter);

        outbuf = gst_adapter_take_buffer (rtpmp4gdepay->adapter, avail);

        GST_DEBUG ("gst_rtp_mp4g_depay_chain: pushing buffer of size %d",
            GST_BUFFER_SIZE (outbuf));

        return outbuf;
      }
    }
  }
  return NULL;

  /* ERRORS */
short_payload:
  {
    GST_ELEMENT_WARNING (rtpmp4gdepay, STREAM, DECODE,
        ("Packet payload was too short."), (NULL));
    return NULL;
  }
}

static gboolean
gst_rtp_mp4g_depay_handle_event (GstBaseRTPDepayload * filter, GstEvent * event)
{
  gboolean ret;
  GstRtpMP4GDepay *rtpmp4gdepay;

  rtpmp4gdepay = GST_RTP_MP4G_DEPAY (filter);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_mp4g_depay_reset (rtpmp4gdepay);
      break;
    default:
      break;
  }

  ret =
      GST_BASE_RTP_DEPAYLOAD_CLASS (parent_class)->handle_event (filter, event);

  return ret;
}

static GstStateChangeReturn
gst_rtp_mp4g_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpMP4GDepay *rtpmp4gdepay;
  GstStateChangeReturn ret;

  rtpmp4gdepay = GST_RTP_MP4G_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_mp4g_depay_reset (rtpmp4gdepay);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_mp4g_depay_reset (rtpmp4gdepay);
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_mp4g_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmp4gdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MP4G_DEPAY);
}
