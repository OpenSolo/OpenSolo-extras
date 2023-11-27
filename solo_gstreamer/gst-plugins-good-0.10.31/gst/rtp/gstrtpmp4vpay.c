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

#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpmp4vpay.h"

GST_DEBUG_CATEGORY_STATIC (rtpmp4vpay_debug);
#define GST_CAT_DEFAULT (rtpmp4vpay_debug)

static GstStaticPadTemplate gst_rtp_mp4v_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg,"
        "mpegversion=(int) 4," "systemstream=(boolean)false;" "video/x-xvid")
    );

static GstStaticPadTemplate gst_rtp_mp4v_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [1, MAX ], " "encoding-name = (string) \"MP4V-ES\""
        /* two string params
         *
         "profile-level-id = (string) [1,MAX]"
         "config = (string) [1,MAX]"
         */
    )
    );

#define DEFAULT_SEND_CONFIG     FALSE
#define DEFAULT_BUFFER_LIST     FALSE
#define DEFAULT_CONFIG_INTERVAL 0

enum
{
  ARG_0,
  ARG_SEND_CONFIG,
  ARG_BUFFER_LIST,
  ARG_CONFIG_INTERVAL
};


static void gst_rtp_mp4v_pay_finalize (GObject * object);

static void gst_rtp_mp4v_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_mp4v_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rtp_mp4v_pay_setcaps (GstBaseRTPPayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_mp4v_pay_handle_buffer (GstBaseRTPPayload *
    payload, GstBuffer * buffer);
static gboolean gst_rtp_mp4v_pay_handle_event (GstPad * pad, GstEvent * event);

GST_BOILERPLATE (GstRtpMP4VPay, gst_rtp_mp4v_pay, GstBaseRTPPayload,
    GST_TYPE_BASE_RTP_PAYLOAD)

     static void gst_rtp_mp4v_pay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_mp4v_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_mp4v_pay_sink_template);

  gst_element_class_set_details_simple (element_class,
      "RTP MPEG4 Video payloader", "Codec/Payloader/Network/RTP",
      "Payload MPEG-4 video as RTP packets (RFC 3016)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_mp4v_pay_class_init (GstRtpMP4VPayClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseRTPPayloadClass *gstbasertppayload_class;

  gobject_class = (GObjectClass *) klass;
  gstbasertppayload_class = (GstBaseRTPPayloadClass *) klass;

  gobject_class->set_property = gst_rtp_mp4v_pay_set_property;
  gobject_class->get_property = gst_rtp_mp4v_pay_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_SEND_CONFIG,
      g_param_spec_boolean ("send-config", "Send Config",
          "Send the config parameters in RTP packets as well(deprecated "
          "see config-interval)",
          DEFAULT_SEND_CONFIG, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_BUFFER_LIST,
      g_param_spec_boolean ("buffer-list", "Buffer Array",
          "Use Buffer Arrays",
          DEFAULT_BUFFER_LIST, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_CONFIG_INTERVAL,
      g_param_spec_uint ("config-interval", "Config Send Interval",
          "Send Config Insertion Interval in seconds (configuration headers "
          "will be multiplexed in the data stream when detected.) (0 = disabled)",
          0, 3600, DEFAULT_CONFIG_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );

  gobject_class->finalize = gst_rtp_mp4v_pay_finalize;

  gstbasertppayload_class->set_caps = gst_rtp_mp4v_pay_setcaps;
  gstbasertppayload_class->handle_buffer = gst_rtp_mp4v_pay_handle_buffer;
  gstbasertppayload_class->handle_event = gst_rtp_mp4v_pay_handle_event;

  GST_DEBUG_CATEGORY_INIT (rtpmp4vpay_debug, "rtpmp4vpay", 0,
      "MP4 video RTP Payloader");
}

static void
gst_rtp_mp4v_pay_init (GstRtpMP4VPay * rtpmp4vpay, GstRtpMP4VPayClass * klass)
{
  rtpmp4vpay->adapter = gst_adapter_new ();
  rtpmp4vpay->rate = 90000;
  rtpmp4vpay->profile = 1;
  rtpmp4vpay->buffer_list = DEFAULT_BUFFER_LIST;
  rtpmp4vpay->send_config = DEFAULT_SEND_CONFIG;
  rtpmp4vpay->need_config = TRUE;
  rtpmp4vpay->config_interval = DEFAULT_CONFIG_INTERVAL;
  rtpmp4vpay->last_config = -1;

  rtpmp4vpay->config = NULL;
}

static void
gst_rtp_mp4v_pay_finalize (GObject * object)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (object);

  if (rtpmp4vpay->config) {
    gst_buffer_unref (rtpmp4vpay->config);
    rtpmp4vpay->config = NULL;
  }
  g_object_unref (rtpmp4vpay->adapter);
  rtpmp4vpay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_mp4v_pay_new_caps (GstRtpMP4VPay * rtpmp4vpay)
{
  gchar *profile, *config;
  GValue v = { 0 };
  gboolean res;

  profile = g_strdup_printf ("%d", rtpmp4vpay->profile);
  g_value_init (&v, GST_TYPE_BUFFER);
  gst_value_set_buffer (&v, rtpmp4vpay->config);
  config = gst_value_serialize (&v);

  res = gst_basertppayload_set_outcaps (GST_BASE_RTP_PAYLOAD (rtpmp4vpay),
      "profile-level-id", G_TYPE_STRING, profile,
      "config", G_TYPE_STRING, config, NULL);

  g_value_unset (&v);

  g_free (profile);
  g_free (config);

  return res;
}

static gboolean
gst_rtp_mp4v_pay_setcaps (GstBaseRTPPayload * payload, GstCaps * caps)
{
  GstRtpMP4VPay *rtpmp4vpay;
  GstStructure *structure;
  const GValue *codec_data;
  gboolean res;

  rtpmp4vpay = GST_RTP_MP4V_PAY (payload);

  gst_basertppayload_set_options (payload, "video", TRUE, "MP4V-ES",
      rtpmp4vpay->rate);

  res = TRUE;

  structure = gst_caps_get_structure (caps, 0);
  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data) {
    GST_LOG_OBJECT (rtpmp4vpay, "got codec_data");
    if (G_VALUE_TYPE (codec_data) == GST_TYPE_BUFFER) {
      GstBuffer *buffer;
      guint8 *data;
      guint size;

      buffer = gst_value_get_buffer (codec_data);

      data = GST_BUFFER_DATA (buffer);
      size = GST_BUFFER_SIZE (buffer);

      if (size < 5)
        goto done;

      rtpmp4vpay->profile = data[4];
      GST_LOG_OBJECT (rtpmp4vpay, "configuring codec_data, profile %d",
          data[4]);

      if (rtpmp4vpay->config)
        gst_buffer_unref (rtpmp4vpay->config);
      rtpmp4vpay->config = gst_buffer_copy (buffer);
      res = gst_rtp_mp4v_pay_new_caps (rtpmp4vpay);
    }
  }

done:
  return res;
}

static void
gst_rtp_mp4v_pay_empty (GstRtpMP4VPay * rtpmp4vpay)
{
  gst_adapter_clear (rtpmp4vpay->adapter);
}

static GstFlowReturn
gst_rtp_mp4v_pay_flush (GstRtpMP4VPay * rtpmp4vpay)
{
  guint avail;
  GstBuffer *outbuf;
  GstBuffer *outbuf_data = NULL;
  GstFlowReturn ret;
  GstBufferList *list = NULL;
  GstBufferListIterator *it = NULL;

  /* the data available in the adapter is either smaller
   * than the MTU or bigger. In the case it is smaller, the complete
   * adapter contents can be put in one packet. In the case the
   * adapter has more than one MTU, we need to split the MP4V data
   * over multiple packets. */
  avail = gst_adapter_available (rtpmp4vpay->adapter);

  if (rtpmp4vpay->config == NULL && rtpmp4vpay->need_config) {
    /* when we don't have a config yet, flush things out */
    gst_adapter_flush (rtpmp4vpay->adapter, avail);
    avail = 0;
  }

  if (!avail)
    return GST_FLOW_OK;

  ret = GST_FLOW_OK;

  if (rtpmp4vpay->buffer_list) {
    /* Use buffer lists. Each frame will be put into a list
     * of buffers and the whole list will be pushed downstream
     * at once */
    list = gst_buffer_list_new ();
    it = gst_buffer_list_iterate (list);
  }

  while (avail > 0) {
    guint towrite;
    guint8 *payload;
    guint payload_len;
    guint packet_len;

    /* this will be the total lenght of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (avail, 0, 0);

    /* fill one MTU or all available bytes */
    towrite = MIN (packet_len, GST_BASE_RTP_PAYLOAD_MTU (rtpmp4vpay));

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    if (rtpmp4vpay->buffer_list) {
      /* create buffer without payload. The payload will be put
       * in next buffer instead. Both buffers will be then added
       * to the list */
      outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

      /* Take buffer with the payload from the adapter */
      outbuf_data = gst_adapter_take_buffer (rtpmp4vpay->adapter, payload_len);
    } else {
      /* create buffer to hold the payload */
      outbuf = gst_rtp_buffer_new_allocate (payload_len, 0, 0);

      /* copy payload */
      payload = gst_rtp_buffer_get_payload (outbuf);

      gst_adapter_copy (rtpmp4vpay->adapter, payload, 0, payload_len);
      gst_adapter_flush (rtpmp4vpay->adapter, payload_len);
    }

    avail -= payload_len;

    gst_rtp_buffer_set_marker (outbuf, avail == 0);

    GST_BUFFER_TIMESTAMP (outbuf) = rtpmp4vpay->first_timestamp;

    if (rtpmp4vpay->buffer_list) {
      /* create a new group to hold the rtp header and the payload */
      gst_buffer_list_iterator_add_group (it);
      gst_buffer_list_iterator_add (it, outbuf);
      gst_buffer_list_iterator_add (it, outbuf_data);
    } else {
      ret = gst_basertppayload_push (GST_BASE_RTP_PAYLOAD (rtpmp4vpay), outbuf);
    }
  }

  if (rtpmp4vpay->buffer_list) {
    gst_buffer_list_iterator_free (it);
    /* push the whole buffer list at once */
    ret =
        gst_basertppayload_push_list (GST_BASE_RTP_PAYLOAD (rtpmp4vpay), list);
  }

  return ret;
}

#define VOS_STARTCODE                   0x000001B0
#define VOS_ENDCODE                     0x000001B1
#define USER_DATA_STARTCODE             0x000001B2
#define GOP_STARTCODE                   0x000001B3
#define VISUAL_OBJECT_STARTCODE         0x000001B5
#define VOP_STARTCODE                   0x000001B6

static gboolean
gst_rtp_mp4v_pay_depay_data (GstRtpMP4VPay * enc, guint8 * data, guint size,
    gint * strip, gboolean * vopi)
{
  guint32 code;
  gboolean result;
  *vopi = FALSE;

  *strip = 0;

  if (size < 5)
    return FALSE;

  code = GST_READ_UINT32_BE (data);
  GST_DEBUG_OBJECT (enc, "start code 0x%08x", code);

  switch (code) {
    case VOS_STARTCODE:
    case 0x00000101:
    {
      gint i;
      guint8 profile;
      gboolean newprofile = FALSE;
      gboolean equal;

      if (code == VOS_STARTCODE) {
        /* profile_and_level_indication */
        profile = data[4];

        GST_DEBUG_OBJECT (enc, "VOS profile 0x%08x", profile);

        if (profile != enc->profile) {
          newprofile = TRUE;
          enc->profile = profile;
        }
      }

      /* up to the next GOP_STARTCODE or VOP_STARTCODE is
       * the config information */
      code = 0xffffffff;
      for (i = 5; i < size - 4; i++) {
        code = (code << 8) | data[i];
        if (code == GOP_STARTCODE || code == VOP_STARTCODE)
          break;
      }
      i -= 3;
      /* see if config changed */
      equal = FALSE;
      if (enc->config) {
        if (GST_BUFFER_SIZE (enc->config) == i) {
          equal = memcmp (GST_BUFFER_DATA (enc->config), data, i) == 0;
        }
      }
      /* if config string changed or new profile, make new caps */
      if (!equal || newprofile) {
        if (enc->config)
          gst_buffer_unref (enc->config);
        enc->config = gst_buffer_new_and_alloc (i);
        memcpy (GST_BUFFER_DATA (enc->config), data, i);
        gst_rtp_mp4v_pay_new_caps (enc);
      }
      *strip = i;
      /* we need to flush out the current packet. */
      result = TRUE;
      break;
    }
    case VOP_STARTCODE:
      GST_DEBUG_OBJECT (enc, "VOP");
      /* VOP startcode, we don't have to flush the packet */
      result = FALSE;
      /* vop-coding-type == I-frame */
      if (size > 4 && (data[4] >> 6 == 0)) {
        GST_DEBUG_OBJECT (enc, "VOP-I");
        *vopi = TRUE;
      }
      break;
    case GOP_STARTCODE:
      GST_DEBUG_OBJECT (enc, "GOP");
      *vopi = TRUE;
      result = TRUE;
      break;
    case 0x00000100:
      enc->need_config = FALSE;
      result = TRUE;
      break;
    default:
      if (code >= 0x20 && code <= 0x2f) {
        GST_DEBUG_OBJECT (enc, "short header");
        result = FALSE;
      } else {
        GST_DEBUG_OBJECT (enc, "other startcode");
        /* all other startcodes need a flush */
        result = TRUE;
      }
      break;
  }
  return result;
}

/* we expect buffers starting on startcodes. 
 */
static GstFlowReturn
gst_rtp_mp4v_pay_handle_buffer (GstBaseRTPPayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpMP4VPay *rtpmp4vpay;
  GstFlowReturn ret;
  guint size, avail;
  guint packet_len;
  guint8 *data;
  gboolean flush;
  gint strip;
  GstClockTime timestamp, duration;
  gboolean vopi;
  gboolean send_config;

  ret = GST_FLOW_OK;
  send_config = FALSE;

  rtpmp4vpay = GST_RTP_MP4V_PAY (basepayload);

  size = GST_BUFFER_SIZE (buffer);
  data = GST_BUFFER_DATA (buffer);
  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  duration = GST_BUFFER_DURATION (buffer);
  avail = gst_adapter_available (rtpmp4vpay->adapter);

  if (duration == -1)
    duration = 0;

  /* empty buffer, take timestamp */
  if (avail == 0) {
    rtpmp4vpay->first_timestamp = timestamp;
    rtpmp4vpay->duration = 0;
  }

  /* depay incomming data and see if we need to start a new RTP
   * packet */
  flush = gst_rtp_mp4v_pay_depay_data (rtpmp4vpay, data, size, &strip, &vopi);
  if (strip) {
    /* strip off config if requested */
    if (!(rtpmp4vpay->config_interval > 0)) {
      GstBuffer *subbuf;

      GST_LOG_OBJECT (rtpmp4vpay, "stripping config at %d, size %d", strip,
          size - strip);

      /* strip off header */
      subbuf = gst_buffer_create_sub (buffer, strip, size - strip);
      GST_BUFFER_TIMESTAMP (subbuf) = timestamp;
      gst_buffer_unref (buffer);
      buffer = subbuf;

      size = GST_BUFFER_SIZE (buffer);
    } else {
      GST_LOG_OBJECT (rtpmp4vpay, "found config in stream");
      rtpmp4vpay->last_config = timestamp;
    }
  }

  /* there is a config request, see if we need to insert it */
  if (vopi && (rtpmp4vpay->config_interval > 0) && rtpmp4vpay->config) {
    if (rtpmp4vpay->last_config != -1) {
      guint64 diff;

      GST_LOG_OBJECT (rtpmp4vpay,
          "now %" GST_TIME_FORMAT ", last VOP-I %" GST_TIME_FORMAT,
          GST_TIME_ARGS (timestamp), GST_TIME_ARGS (rtpmp4vpay->last_config));

      /* calculate diff between last config in milliseconds */
      if (timestamp > rtpmp4vpay->last_config) {
        diff = timestamp - rtpmp4vpay->last_config;
      } else {
        diff = 0;
      }

      GST_DEBUG_OBJECT (rtpmp4vpay,
          "interval since last config %" GST_TIME_FORMAT, GST_TIME_ARGS (diff));

      /* bigger than interval, queue config */
      /* FIXME should convert timestamps to running time */
      if (GST_TIME_AS_SECONDS (diff) >= rtpmp4vpay->config_interval) {
        GST_DEBUG_OBJECT (rtpmp4vpay, "time to send config");
        send_config = TRUE;
      }
    } else {
      /* no known previous config time, send now */
      GST_DEBUG_OBJECT (rtpmp4vpay, "no previous config time, send now");
      send_config = TRUE;
    }

    if (send_config) {
      /* we need to send config now first */
      GstBuffer *superbuf;

      GST_LOG_OBJECT (rtpmp4vpay, "inserting config in stream");

      /* insert header */
      superbuf = gst_buffer_merge (rtpmp4vpay->config, buffer);

      GST_BUFFER_TIMESTAMP (superbuf) = timestamp;
      gst_buffer_unref (buffer);
      buffer = superbuf;

      size = GST_BUFFER_SIZE (buffer);

      if (timestamp != -1) {
        rtpmp4vpay->last_config = timestamp;
      }
    }
  }

  /* if we need to flush, do so now */
  if (flush) {
    ret = gst_rtp_mp4v_pay_flush (rtpmp4vpay);
    rtpmp4vpay->first_timestamp = timestamp;
    rtpmp4vpay->duration = 0;
    avail = 0;
  }

  /* get packet length of data and see if we exceeded MTU. */
  packet_len = gst_rtp_buffer_calc_packet_len (avail + size, 0, 0);

  if (gst_basertppayload_is_filled (basepayload,
          packet_len, rtpmp4vpay->duration + duration)) {
    ret = gst_rtp_mp4v_pay_flush (rtpmp4vpay);
    rtpmp4vpay->first_timestamp = timestamp;
    rtpmp4vpay->duration = 0;
  }

  /* push new data */
  gst_adapter_push (rtpmp4vpay->adapter, buffer);

  rtpmp4vpay->duration += duration;

  return ret;
}

static gboolean
gst_rtp_mp4v_pay_handle_event (GstPad * pad, GstEvent * event)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (gst_pad_get_parent (pad));

  GST_DEBUG ("Got event: %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NEWSEGMENT:
    case GST_EVENT_EOS:
      /* This flush call makes sure that the last buffer is always pushed
       * to the base payloader */
      gst_rtp_mp4v_pay_flush (rtpmp4vpay);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_mp4v_pay_empty (rtpmp4vpay);
      break;
    default:
      break;
  }

  g_object_unref (rtpmp4vpay);

  /* let parent handle event too */
  return FALSE;
}

static void
gst_rtp_mp4v_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (object);

  switch (prop_id) {
    case ARG_SEND_CONFIG:
      rtpmp4vpay->send_config = g_value_get_boolean (value);
      /* send the configuration once every minute */
      if (rtpmp4vpay->send_config && !(rtpmp4vpay->config_interval > 0)) {
        rtpmp4vpay->config_interval = 60;
      }
      break;
    case ARG_BUFFER_LIST:
      rtpmp4vpay->buffer_list = g_value_get_boolean (value);
      break;
    case ARG_CONFIG_INTERVAL:
      rtpmp4vpay->config_interval = g_value_get_uint (value);
      break;
    default:
      break;
  }
}

static void
gst_rtp_mp4v_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (object);

  switch (prop_id) {
    case ARG_SEND_CONFIG:
      g_value_set_boolean (value, rtpmp4vpay->send_config);
      break;
    case ARG_BUFFER_LIST:
      g_value_set_boolean (value, rtpmp4vpay->buffer_list);
      break;
    case ARG_CONFIG_INTERVAL:
      g_value_set_uint (value, rtpmp4vpay->config_interval);
      break;
    default:
      break;
  }
}

gboolean
gst_rtp_mp4v_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmp4vpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MP4V_PAY);
}
