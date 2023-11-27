/* GStreamer
 * Copyright (C) <2005> Edgard Lima <edgard.lima@indt.org.br>
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

#include "gstrtpspeexdepay.h"

/* RtpSPEEXDepay signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0
};

static GstStaticPadTemplate gst_rtp_speex_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate =  (int) [6000, 48000], "
        "encoding-name = (string) \"SPEEX\", "
        "encoding-params = (string) \"1\"")
    );

static GstStaticPadTemplate gst_rtp_speex_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-speex")
    );

static GstBuffer *gst_rtp_speex_depay_process (GstBaseRTPDepayload * depayload,
    GstBuffer * buf);
static gboolean gst_rtp_speex_depay_setcaps (GstBaseRTPDepayload * depayload,
    GstCaps * caps);

GST_BOILERPLATE (GstRtpSPEEXDepay, gst_rtp_speex_depay, GstBaseRTPDepayload,
    GST_TYPE_BASE_RTP_DEPAYLOAD);

static void
gst_rtp_speex_depay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_speex_depay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_speex_depay_sink_template);
  gst_element_class_set_details_simple (element_class, "RTP Speex depayloader",
      "Codec/Depayloader/Network/RTP",
      "Extracts Speex audio from RTP packets",
      "Edgard Lima <edgard.lima@indt.org.br>");
}

static void
gst_rtp_speex_depay_class_init (GstRtpSPEEXDepayClass * klass)
{
  GstBaseRTPDepayloadClass *gstbasertpdepayload_class;

  gstbasertpdepayload_class = (GstBaseRTPDepayloadClass *) klass;

  gstbasertpdepayload_class->process = gst_rtp_speex_depay_process;
  gstbasertpdepayload_class->set_caps = gst_rtp_speex_depay_setcaps;
}

static void
gst_rtp_speex_depay_init (GstRtpSPEEXDepay * rtpspeexdepay,
    GstRtpSPEEXDepayClass * klass)
{
}

static gint
gst_rtp_speex_depay_get_mode (gint rate)
{
  if (rate > 25000)
    return 2;
  else if (rate > 12500)
    return 1;
  else
    return 0;
}

/* len 4 bytes LE,
 * vendor string (len bytes),
 * user_len 4 (0) bytes LE
 */
static const gchar gst_rtp_speex_comment[] =
    "\045\0\0\0Depayloaded with GStreamer speexdepay\0\0\0\0";

static gboolean
gst_rtp_speex_depay_setcaps (GstBaseRTPDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpSPEEXDepay *rtpspeexdepay;
  gint clock_rate, nb_channels;
  GstBuffer *buf;
  guint8 *data;
  const gchar *params;
  GstCaps *srccaps;
  gboolean res;

  rtpspeexdepay = GST_RTP_SPEEX_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    goto no_clockrate;
  depayload->clock_rate = clock_rate;

  if (!(params = gst_structure_get_string (structure, "encoding-params")))
    nb_channels = 1;
  else {
    nb_channels = atoi (params);
  }

  /* construct minimal header and comment packet for the decoder */
  buf = gst_buffer_new_and_alloc (80);
  data = GST_BUFFER_DATA (buf);
  memcpy (data, "Speex   ", 8);
  data += 8;
  memcpy (data, "1.1.12", 7);
  data += 20;
  GST_WRITE_UINT32_LE (data, 1);        /* version */
  data += 4;
  GST_WRITE_UINT32_LE (data, 80);       /* header_size */
  data += 4;
  GST_WRITE_UINT32_LE (data, clock_rate);       /* rate */
  data += 4;
  GST_WRITE_UINT32_LE (data, gst_rtp_speex_depay_get_mode (clock_rate));        /* mode */
  data += 4;
  GST_WRITE_UINT32_LE (data, 4);        /* mode_bitstream_version */
  data += 4;
  GST_WRITE_UINT32_LE (data, nb_channels);      /* nb_channels */
  data += 4;
  GST_WRITE_UINT32_LE (data, -1);       /* bitrate */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0xa0);     /* frame_size */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* VBR */
  data += 4;
  GST_WRITE_UINT32_LE (data, 1);        /* frames_per_packet */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* extra_headers */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* reserved1 */
  data += 4;
  GST_WRITE_UINT32_LE (data, 0);        /* reserved2 */

  srccaps = gst_caps_new_simple ("audio/x-speex", NULL);
  res = gst_pad_set_caps (depayload->srcpad, srccaps);
  gst_caps_unref (srccaps);

  gst_buffer_set_caps (buf, GST_PAD_CAPS (depayload->srcpad));
  gst_base_rtp_depayload_push (GST_BASE_RTP_DEPAYLOAD (rtpspeexdepay), buf);

  buf = gst_buffer_new_and_alloc (sizeof (gst_rtp_speex_comment));
  memcpy (GST_BUFFER_DATA (buf), gst_rtp_speex_comment,
      sizeof (gst_rtp_speex_comment));

  gst_buffer_set_caps (buf, GST_PAD_CAPS (depayload->srcpad));
  gst_base_rtp_depayload_push (GST_BASE_RTP_DEPAYLOAD (rtpspeexdepay), buf);

  return res;

  /* ERRORS */
no_clockrate:
  {
    GST_DEBUG_OBJECT (depayload, "no clock-rate specified");
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_speex_depay_process (GstBaseRTPDepayload * depayload, GstBuffer * buf)
{
  GstBuffer *outbuf = NULL;

  GST_DEBUG ("process : got %d bytes, mark %d ts %u seqn %d",
      GST_BUFFER_SIZE (buf),
      gst_rtp_buffer_get_marker (buf),
      gst_rtp_buffer_get_timestamp (buf), gst_rtp_buffer_get_seq (buf));

  /* nothing special to be done */
  outbuf = gst_rtp_buffer_get_payload_buffer (buf);

  if (outbuf)
    GST_BUFFER_DURATION (outbuf) = 20 * GST_MSECOND;

  return outbuf;
}

gboolean
gst_rtp_speex_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpspeexdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_SPEEX_DEPAY);
}
