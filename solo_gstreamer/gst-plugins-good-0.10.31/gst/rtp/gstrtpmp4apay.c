/* GStreamer
 * Copyright (C) <2008> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpmp4apay.h"

GST_DEBUG_CATEGORY_STATIC (rtpmp4apay_debug);
#define GST_CAT_DEFAULT (rtpmp4apay_debug)

/* FIXME: add framed=(boolean)true once our encoders have this field set
 * on their output caps */
static GstStaticPadTemplate gst_rtp_mp4a_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, mpegversion=(int)4, "
        "stream-format=(string)raw")
    );

static GstStaticPadTemplate gst_rtp_mp4a_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [1, MAX ], "
        "encoding-name = (string) \"MP4A-LATM\""
        /* All optional parameters
         *
         * "cpresent = (string) \"0\""
         * "config=" 
         */
    )
    );

static void gst_rtp_mp4a_pay_finalize (GObject * object);

static gboolean gst_rtp_mp4a_pay_setcaps (GstBaseRTPPayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_mp4a_pay_handle_buffer (GstBaseRTPPayload *
    payload, GstBuffer * buffer);

GST_BOILERPLATE (GstRtpMP4APay, gst_rtp_mp4a_pay, GstBaseRTPPayload,
    GST_TYPE_BASE_RTP_PAYLOAD)

     static void gst_rtp_mp4a_pay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_mp4a_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_mp4a_pay_sink_template);

  gst_element_class_set_details_simple (element_class,
      "RTP MPEG4 audio payloader", "Codec/Payloader/Network/RTP",
      "Payload MPEG4 audio as RTP packets (RFC 3016)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_mp4a_pay_class_init (GstRtpMP4APayClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseRTPPayloadClass *gstbasertppayload_class;

  gobject_class = (GObjectClass *) klass;
  gstbasertppayload_class = (GstBaseRTPPayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mp4a_pay_finalize;

  gstbasertppayload_class->set_caps = gst_rtp_mp4a_pay_setcaps;
  gstbasertppayload_class->handle_buffer = gst_rtp_mp4a_pay_handle_buffer;

  GST_DEBUG_CATEGORY_INIT (rtpmp4apay_debug, "rtpmp4apay", 0,
      "MP4A-LATM RTP Payloader");
}

static void
gst_rtp_mp4a_pay_init (GstRtpMP4APay * rtpmp4apay, GstRtpMP4APayClass * klass)
{
  rtpmp4apay->rate = 90000;
  rtpmp4apay->profile = g_strdup ("1");
}

static void
gst_rtp_mp4a_pay_finalize (GObject * object)
{
  GstRtpMP4APay *rtpmp4apay;

  rtpmp4apay = GST_RTP_MP4A_PAY (object);

  g_free (rtpmp4apay->params);
  rtpmp4apay->params = NULL;

  if (rtpmp4apay->config)
    gst_buffer_unref (rtpmp4apay->config);
  rtpmp4apay->config = NULL;

  g_free (rtpmp4apay->profile);
  rtpmp4apay->profile = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static const unsigned int sampling_table[16] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

static gboolean
gst_rtp_mp4a_pay_parse_audio_config (GstRtpMP4APay * rtpmp4apay,
    GstBuffer * buffer)
{
  guint8 *data;
  guint size;
  guint8 objectType;
  guint8 samplingIdx;
  guint8 channelCfg;

  data = GST_BUFFER_DATA (buffer);
  size = GST_BUFFER_SIZE (buffer);

  if (size < 2)
    goto too_short;

  /* any object type is fine, we need to copy it to the profile-level-id field. */
  objectType = (data[0] & 0xf8) >> 3;
  if (objectType == 0)
    goto invalid_object;

  samplingIdx = ((data[0] & 0x07) << 1) | ((data[1] & 0x80) >> 7);
  /* only fixed values for now */
  if (samplingIdx > 12 && samplingIdx != 15)
    goto wrong_freq;

  channelCfg = ((data[1] & 0x78) >> 3);
  if (channelCfg > 7)
    goto wrong_channels;

  /* rtp rate depends on sampling rate of the audio */
  if (samplingIdx == 15) {
    if (size < 5)
      goto too_short;

    /* index of 15 means we get the rate in the next 24 bits */
    rtpmp4apay->rate = ((data[1] & 0x7f) << 17) |
        ((data[2]) << 9) | ((data[3]) << 1) | ((data[4] & 0x80) >> 7);
  } else {
    /* else use the rate from the table */
    rtpmp4apay->rate = sampling_table[samplingIdx];
  }
  /* extra rtp params contain the number of channels */
  g_free (rtpmp4apay->params);
  rtpmp4apay->params = g_strdup_printf ("%d", channelCfg);
  /* audio stream type */
  rtpmp4apay->streamtype = "5";
  /* profile */
  g_free (rtpmp4apay->profile);
  rtpmp4apay->profile = g_strdup_printf ("%d", objectType);

  GST_DEBUG_OBJECT (rtpmp4apay,
      "objectType: %d, samplingIdx: %d (%d), channelCfg: %d", objectType,
      samplingIdx, rtpmp4apay->rate, channelCfg);

  return TRUE;

  /* ERROR */
too_short:
  {
    GST_ELEMENT_ERROR (rtpmp4apay, STREAM, FORMAT,
        (NULL), ("config string too short, expected 2 bytes, got %d", size));
    return FALSE;
  }
invalid_object:
  {
    GST_ELEMENT_ERROR (rtpmp4apay, STREAM, FORMAT,
        (NULL), ("invalid object type 0"));
    return FALSE;
  }
wrong_freq:
  {
    GST_ELEMENT_ERROR (rtpmp4apay, STREAM, NOT_IMPLEMENTED,
        (NULL), ("unsupported frequency index %d", samplingIdx));
    return FALSE;
  }
wrong_channels:
  {
    GST_ELEMENT_ERROR (rtpmp4apay, STREAM, NOT_IMPLEMENTED,
        (NULL), ("unsupported number of channels %d, must < 8", channelCfg));
    return FALSE;
  }
}

static gboolean
gst_rtp_mp4a_pay_new_caps (GstRtpMP4APay * rtpmp4apay)
{
  gchar *config;
  GValue v = { 0 };
  gboolean res;

  g_value_init (&v, GST_TYPE_BUFFER);
  gst_value_set_buffer (&v, rtpmp4apay->config);
  config = gst_value_serialize (&v);

  res = gst_basertppayload_set_outcaps (GST_BASE_RTP_PAYLOAD (rtpmp4apay),
      "cpresent", G_TYPE_STRING, "0", "config", G_TYPE_STRING, config, NULL);

  g_value_unset (&v);
  g_free (config);

  return res;
}

static gboolean
gst_rtp_mp4a_pay_setcaps (GstBaseRTPPayload * payload, GstCaps * caps)
{
  GstRtpMP4APay *rtpmp4apay;
  GstStructure *structure;
  const GValue *codec_data;
  gboolean res, framed = TRUE;
  const gchar *stream_format;

  rtpmp4apay = GST_RTP_MP4A_PAY (payload);

  structure = gst_caps_get_structure (caps, 0);

  /* this is already handled by the template caps, but it is better
   * to leave here to have meaningful warning messages when linking
   * fails */
  stream_format = gst_structure_get_string (structure, "stream-format");
  if (stream_format) {
    if (strcmp (stream_format, "raw") != 0) {
      GST_WARNING_OBJECT (rtpmp4apay, "AAC's stream-format must be 'raw', "
          "%s is not supported", stream_format);
      return FALSE;
    }
  } else {
    GST_WARNING_OBJECT (rtpmp4apay, "AAC's stream-format not specified, "
        "assuming 'raw'");
  }

  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data) {
    GST_LOG_OBJECT (rtpmp4apay, "got codec_data");
    if (G_VALUE_TYPE (codec_data) == GST_TYPE_BUFFER) {
      GstBuffer *buffer, *cbuffer;
      guint8 *config;
      guint8 *data;
      guint size, i;

      buffer = gst_value_get_buffer (codec_data);
      GST_LOG_OBJECT (rtpmp4apay, "configuring codec_data");

      /* parse buffer */
      res = gst_rtp_mp4a_pay_parse_audio_config (rtpmp4apay, buffer);

      if (!res)
        goto config_failed;

      size = GST_BUFFER_SIZE (buffer);
      data = GST_BUFFER_DATA (buffer);

      /* make the StreamMuxConfig, we need 15 bits for the header */
      config = g_malloc0 (size + 2);

      /* Create StreamMuxConfig according to ISO/IEC 14496-3:
       *
       * audioMuxVersion           == 0 (1 bit)
       * allStreamsSameTimeFraming == 1 (1 bit)
       * numSubFrames              == numSubFrames (6 bits)
       * numProgram                == 0 (4 bits)
       * numLayer                  == 0 (3 bits)
       */
      config[0] = 0x40;
      config[1] = 0x00;

      /* append the config bits, shifting them 1 bit left */
      for (i = 0; i < size; i++) {
        config[i + 1] |= ((data[i] & 0x80) >> 7);
        config[i + 2] |= ((data[i] & 0x7f) << 1);
      }

      cbuffer = gst_buffer_new ();
      GST_BUFFER_DATA (cbuffer) = config;
      GST_BUFFER_MALLOCDATA (cbuffer) = config;
      GST_BUFFER_SIZE (cbuffer) = size + 2;

      /* now we can configure the buffer */
      if (rtpmp4apay->config)
        gst_buffer_unref (rtpmp4apay->config);
      rtpmp4apay->config = cbuffer;
    }
  }

  if (gst_structure_get_boolean (structure, "framed", &framed) && !framed) {
    GST_WARNING_OBJECT (payload, "Need framed AAC data as input!");
  }

  gst_basertppayload_set_options (payload, "audio", TRUE, "MP4A-LATM",
      rtpmp4apay->rate);

  res = gst_rtp_mp4a_pay_new_caps (rtpmp4apay);

  return res;

  /* ERRORS */
config_failed:
  {
    GST_DEBUG_OBJECT (rtpmp4apay, "failed to parse config");
    return FALSE;
  }
}

/* we expect buffers as exactly one complete AU
 */
static GstFlowReturn
gst_rtp_mp4a_pay_handle_buffer (GstBaseRTPPayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpMP4APay *rtpmp4apay;
  GstFlowReturn ret;
  GstBuffer *outbuf;
  guint count, mtu, size;
  guint8 *data;
  gboolean fragmented;
  GstClockTime timestamp;

  ret = GST_FLOW_OK;

  rtpmp4apay = GST_RTP_MP4A_PAY (basepayload);

  size = GST_BUFFER_SIZE (buffer);
  data = GST_BUFFER_DATA (buffer);
  timestamp = GST_BUFFER_TIMESTAMP (buffer);

  fragmented = FALSE;
  mtu = GST_BASE_RTP_PAYLOAD_MTU (rtpmp4apay);

  while (size > 0) {
    guint towrite;
    guint8 *payload;
    guint payload_len;
    guint packet_len;

    /* this will be the total lenght of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (size, 0, 0);

    if (!fragmented) {
      /* first packet calculate space for the packet including the header */
      count = size;
      while (count >= 0xff) {
        packet_len++;
        count -= 0xff;
      }
      packet_len++;
    }

    /* fill one MTU or all available bytes */
    towrite = MIN (packet_len, mtu);

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    GST_DEBUG_OBJECT (rtpmp4apay,
        "avail %d, towrite %d, packet_len %d, payload_len %d", size, towrite,
        packet_len, payload_len);

    /* create buffer to hold the payload. */
    outbuf = gst_rtp_buffer_new_allocate (payload_len, 0, 0);

    /* copy payload */
    payload = gst_rtp_buffer_get_payload (outbuf);

    if (!fragmented) {
      /* first packet write the header */
      count = size;
      while (count >= 0xff) {
        *payload++ = 0xff;
        payload_len--;
        count -= 0xff;
      }
      *payload++ = count;
      payload_len--;
    }

    /* copy data to payload */
    memcpy (payload, data, payload_len);
    data += payload_len;
    size -= payload_len;

    /* marker only if the packet is complete */
    gst_rtp_buffer_set_marker (outbuf, size == 0);

    /* copy incomming timestamp (if any) to outgoing buffers */
    GST_BUFFER_TIMESTAMP (outbuf) = timestamp;

    ret = gst_basertppayload_push (GST_BASE_RTP_PAYLOAD (rtpmp4apay), outbuf);

    fragmented = TRUE;
  }

  gst_buffer_unref (buffer);

  return ret;
}

gboolean
gst_rtp_mp4a_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmp4apay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MP4A_PAY);
}
