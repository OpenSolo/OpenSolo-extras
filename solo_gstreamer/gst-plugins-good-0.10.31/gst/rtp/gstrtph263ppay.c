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
#include <stdio.h>

#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtph263ppay.h"

#define DEFAULT_FRAGMENTATION_MODE   GST_FRAGMENTATION_MODE_NORMAL

enum
{
  PROP_0,
  PROP_FRAGMENTATION_MODE
};

#define GST_TYPE_FRAGMENTATION_MODE (gst_fragmentation_mode_get_type())
static GType
gst_fragmentation_mode_get_type (void)
{
  static GType fragmentation_mode_type = 0;
  static const GEnumValue fragmentation_mode[] = {
    {GST_FRAGMENTATION_MODE_NORMAL, "Normal", "normal"},
    {GST_FRAGMENTATION_MODE_SYNC, "Fragment at sync points", "sync"},
    {0, NULL, NULL},
  };

  if (!fragmentation_mode_type) {
    fragmentation_mode_type =
        g_enum_register_static ("GstFragmentationMode", fragmentation_mode);
  }
  return fragmentation_mode_type;
}


GST_DEBUG_CATEGORY_STATIC (rtph263ppay_debug);
#define GST_CAT_DEFAULT rtph263ppay_debug

static GstStaticPadTemplate gst_rtp_h263p_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h263, " "variant = (string) \"itu\"")
    );

/*
 * We also return these in getcaps() as required by the SDP caps
 *
 * width = (int) [16, 4096]
 * height = (int) [16, 4096]
 * "annex-f = (boolean) {true, false},"
 * "annex-i = (boolean) {true, false},"
 * "annex-j = (boolean) {true, false},"
 * "annex-l = (boolean) {true, false},"
 * "annex-t = (boolean) {true, false},"
 * "annex-v = (boolean) {true, false}")
 */


static GstStaticPadTemplate gst_rtp_h263p_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H263-1998\"; "
        "application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H263-2000\"")
    );

static void gst_rtp_h263p_pay_finalize (GObject * object);

static void gst_rtp_h263p_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_h263p_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rtp_h263p_pay_setcaps (GstBaseRTPPayload * payload,
    GstCaps * caps);
static GstCaps *gst_rtp_h263p_pay_sink_getcaps (GstBaseRTPPayload * payload,
    GstPad * pad);
static GstFlowReturn gst_rtp_h263p_pay_handle_buffer (GstBaseRTPPayload *
    payload, GstBuffer * buffer);

GST_BOILERPLATE (GstRtpH263PPay, gst_rtp_h263p_pay, GstBaseRTPPayload,
    GST_TYPE_BASE_RTP_PAYLOAD);

static void
gst_rtp_h263p_pay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_h263p_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_h263p_pay_sink_template);

  gst_element_class_set_details_simple (element_class, "RTP H263 payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encodes H263/+/++ video in RTP packets (RFC 4629)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_h263p_pay_class_init (GstRtpH263PPayClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseRTPPayloadClass *gstbasertppayload_class;

  gobject_class = (GObjectClass *) klass;
  gstbasertppayload_class = (GstBaseRTPPayloadClass *) klass;

  gobject_class->finalize = gst_rtp_h263p_pay_finalize;
  gobject_class->set_property = gst_rtp_h263p_pay_set_property;
  gobject_class->get_property = gst_rtp_h263p_pay_get_property;

  gstbasertppayload_class->set_caps = gst_rtp_h263p_pay_setcaps;
  gstbasertppayload_class->get_caps = gst_rtp_h263p_pay_sink_getcaps;
  gstbasertppayload_class->handle_buffer = gst_rtp_h263p_pay_handle_buffer;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_FRAGMENTATION_MODE, g_param_spec_enum ("fragmentation-mode",
          "Fragmentation Mode",
          "Packet Fragmentation Mode", GST_TYPE_FRAGMENTATION_MODE,
          DEFAULT_FRAGMENTATION_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (rtph263ppay_debug, "rtph263ppay",
      0, "rtph263ppay (RFC 4629)");
}

static void
gst_rtp_h263p_pay_init (GstRtpH263PPay * rtph263ppay,
    GstRtpH263PPayClass * klass)
{
  rtph263ppay->adapter = gst_adapter_new ();

  rtph263ppay->fragmentation_mode = DEFAULT_FRAGMENTATION_MODE;
}

static void
gst_rtp_h263p_pay_finalize (GObject * object)
{
  GstRtpH263PPay *rtph263ppay;

  rtph263ppay = GST_RTP_H263P_PAY (object);

  g_object_unref (rtph263ppay->adapter);
  rtph263ppay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_h263p_pay_setcaps (GstBaseRTPPayload * payload, GstCaps * caps)
{
  gboolean res;
  GstCaps *peercaps;
  gchar *encoding_name = NULL;

  g_return_val_if_fail (gst_caps_is_fixed (caps), FALSE);

  peercaps = gst_pad_peer_get_caps (GST_BASE_RTP_PAYLOAD_SRCPAD (payload));
  if (peercaps) {
    GstCaps *intersect = gst_caps_intersect (peercaps,
        gst_pad_get_pad_template_caps (GST_BASE_RTP_PAYLOAD_SRCPAD (payload)));

    gst_caps_unref (peercaps);
    if (!gst_caps_is_empty (intersect)) {
      GstStructure *s = gst_caps_get_structure (intersect, 0);
      encoding_name = g_strdup (gst_structure_get_string (s, "encoding-name"));
    }
    gst_caps_unref (intersect);
  }

  if (!encoding_name)
    encoding_name = g_strdup ("H263-1998");

  gst_basertppayload_set_options (payload, "video", TRUE,
      (gchar *) encoding_name, 90000);
  res = gst_basertppayload_set_outcaps (payload, NULL);
  g_free (encoding_name);

  return res;
}

static void
caps_append (GstCaps * caps, GstStructure * in_s, guint x, guint y, guint mpi)
{
  GstStructure *s;

  if (!in_s)
    return;

  if (mpi < 1 || mpi > 32)
    return;

  s = gst_structure_copy (in_s);

  gst_structure_set (s,
      "width", GST_TYPE_INT_RANGE, 1, x,
      "height", GST_TYPE_INT_RANGE, 1, y,
      "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30000, 1001 * mpi, NULL);

  gst_caps_merge_structure (caps, s);
}


static GstCaps *
gst_rtp_h263p_pay_sink_getcaps (GstBaseRTPPayload * payload, GstPad * pad)
{
  GstRtpH263PPay *rtph263ppay;
  GstCaps *caps = NULL;
  GstCaps *peercaps = NULL;
  GstCaps *intersect = NULL;
  guint i;

  rtph263ppay = GST_RTP_H263P_PAY (payload);

  peercaps = gst_pad_peer_get_caps (GST_BASE_RTP_PAYLOAD_SRCPAD (payload));
  if (!peercaps)
    return
        gst_caps_copy (gst_pad_get_pad_template_caps
        (GST_BASE_RTP_PAYLOAD_SINKPAD (payload)));

  intersect = gst_caps_intersect (peercaps,
      gst_pad_get_pad_template_caps (GST_BASE_RTP_PAYLOAD_SRCPAD (payload)));
  gst_caps_unref (peercaps);

  if (gst_caps_is_empty (intersect))
    return intersect;

  caps = gst_caps_new_empty ();
  for (i = 0; i < gst_caps_get_size (intersect); i++) {
    GstStructure *s = gst_caps_get_structure (intersect, i);
    const gchar *encoding_name = gst_structure_get_string (s, "encoding-name");

    if (!strcmp (encoding_name, "H263-2000")) {
      const gchar *profile_str = gst_structure_get_string (s, "profile");
      const gchar *level_str = gst_structure_get_string (s, "level");
      int profile = 0;
      int level = 0;

      if (profile_str && level_str) {
        gboolean i = FALSE, j = FALSE, l = FALSE, t = FALSE, f = FALSE,
            v = FALSE;
        GstStructure *new_s = gst_structure_new ("video/x-h263",
            "variant", G_TYPE_STRING, "itu",
            NULL);

        profile = atoi (profile_str);
        level = atoi (level_str);

        /* These profiles are defined in the H.263 Annex X */
        switch (profile) {
          case 0:
            /* The Baseline Profile (Profile 0) */
            break;
          case 1:
            /* H.320 Coding Efficiency Version 2 Backward-Compatibility Profile
             * (Profile 1)
             * Baseline + Annexes I, J, L.4 and T
             */
            i = j = l = t = TRUE;
            break;
          case 2:
            /* Version 1 Backward-Compatibility Profile (Profile 2)
             * Baseline + Annex F
             */
            i = j = l = t = f = TRUE;
            break;
          case 3:
            /* Version 2 Interactive and Streaming Wireless Profile
             * Baseline + Annexes I, J, T
             */
            i = j = t = TRUE;
            break;
          case 4:
            /* Version 3 Interactive and Streaming Wireless Profile (Profile 4)
             * Baseline + Annexes I, J, T, V, W.6.3.8,
             */
            /* Missing W.6.3.8 */
            i = j = t = v = TRUE;
            break;
          case 5:
            /* Conversational High Compression Profile (Profile 5)
             * Baseline + Annexes F, I, J, L.4, T, D, U
             */
            /* Missing D, U */
            f = i = j = l = t = TRUE;
            break;
          case 6:
            /* Conversational Internet Profile (Profile 6)
             * Baseline + Annexes F, I, J, L.4, T, D, U and
             * K with arbitratry slice ordering
             */
            /* Missing D, U, K with arbitratry slice ordering */
            f = i = j = l = t = TRUE;
            break;
          case 7:
            /* Conversational Interlace Profile (Profile 7)
             * Baseline + Annexes F, I, J, L.4, T, D, U,  W.6.3.11
             */
            /* Missing D, U, W.6.3.11 */
            f = i = j = l = t = TRUE;
            break;
          case 8:
            /* High Latency Profile (Profile 8)
             * Baseline + Annexes F, I, J, L.4, T, D, U, P.5, O.1.1 and
             * K with arbitratry slice ordering
             */
            /* Missing D, U, P.5, O.1.1 */
            f = i = j = l = t = TRUE;
            break;
        }


        if (f || i || j || t || l || v) {
          GValue list = { 0 };
          GValue vstr = { 0 };

          g_value_init (&list, GST_TYPE_LIST);
          g_value_init (&vstr, G_TYPE_STRING);

          g_value_set_static_string (&vstr, "h263");
          gst_value_list_append_value (&list, &vstr);
          g_value_set_static_string (&vstr, "h263p");
          gst_value_list_append_value (&list, &vstr);

          if (l || v) {
            g_value_set_static_string (&vstr, "h263pp");
            gst_value_list_append_value (&list, &vstr);
          }
          g_value_unset (&vstr);

          gst_structure_set_value (new_s, "h263version", &list);
          g_value_unset (&list);
        } else {
          gst_structure_set (new_s, "h263version", G_TYPE_STRING, "h263", NULL);
        }


        if (!f)
          gst_structure_set (new_s, "annex-f", G_TYPE_BOOLEAN, FALSE, NULL);
        if (!i)
          gst_structure_set (new_s, "annex-i", G_TYPE_BOOLEAN, FALSE, NULL);
        if (!j)
          gst_structure_set (new_s, "annex-j", G_TYPE_BOOLEAN, FALSE, NULL);
        if (!t)
          gst_structure_set (new_s, "annex-t", G_TYPE_BOOLEAN, FALSE, NULL);
        if (!l)
          gst_structure_set (new_s, "annex-l", G_TYPE_BOOLEAN, FALSE, NULL);
        if (!v)
          gst_structure_set (new_s, "annex-v", G_TYPE_BOOLEAN, FALSE, NULL);


        if (level <= 10 || level == 45) {
          gst_structure_set (new_s,
              "width", GST_TYPE_INT_RANGE, 1, 176,
              "height", GST_TYPE_INT_RANGE, 1, 144,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30000, 2002, NULL);
          gst_caps_merge_structure (caps, new_s);
        } else if (level <= 20) {
          GstStructure *s_copy = gst_structure_copy (new_s);

          gst_structure_set (new_s,
              "width", GST_TYPE_INT_RANGE, 1, 352,
              "height", GST_TYPE_INT_RANGE, 1, 288,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30000, 2002, NULL);
          gst_caps_merge_structure (caps, new_s);

          gst_structure_set (s_copy,
              "width", GST_TYPE_INT_RANGE, 1, 176,
              "height", GST_TYPE_INT_RANGE, 1, 144,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30000, 1001, NULL);
          gst_caps_merge_structure (caps, s_copy);
        } else if (level <= 40) {

          gst_structure_set (new_s,
              "width", GST_TYPE_INT_RANGE, 1, 352,
              "height", GST_TYPE_INT_RANGE, 1, 288,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30000, 1001, NULL);
          gst_caps_merge_structure (caps, new_s);
        } else if (level <= 50) {
          GstStructure *s_copy = gst_structure_copy (new_s);

          gst_structure_set (new_s,
              "width", GST_TYPE_INT_RANGE, 1, 352,
              "height", GST_TYPE_INT_RANGE, 1, 288,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 50, 1, NULL);
          gst_caps_merge_structure (caps, new_s);

          gst_structure_set (s_copy,
              "width", GST_TYPE_INT_RANGE, 1, 352,
              "height", GST_TYPE_INT_RANGE, 1, 240,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 60000, 1001, NULL);
          gst_caps_merge_structure (caps, s_copy);
        } else if (level <= 60) {
          GstStructure *s_copy = gst_structure_copy (new_s);

          gst_structure_set (new_s,
              "width", GST_TYPE_INT_RANGE, 1, 720,
              "height", GST_TYPE_INT_RANGE, 1, 288,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 50, 1, NULL);
          gst_caps_merge_structure (caps, new_s);

          gst_structure_set (s_copy,
              "width", GST_TYPE_INT_RANGE, 1, 720,
              "height", GST_TYPE_INT_RANGE, 1, 240,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 60000, 1001, NULL);
          gst_caps_merge_structure (caps, s_copy);
        } else if (level <= 70) {
          GstStructure *s_copy = gst_structure_copy (new_s);

          gst_structure_set (new_s,
              "width", GST_TYPE_INT_RANGE, 1, 720,
              "height", GST_TYPE_INT_RANGE, 1, 576,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 50, 1, NULL);
          gst_caps_merge_structure (caps, new_s);

          gst_structure_set (s_copy,
              "width", GST_TYPE_INT_RANGE, 1, 720,
              "height", GST_TYPE_INT_RANGE, 1, 480,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 60000, 1001, NULL);
          gst_caps_merge_structure (caps, s_copy);
        } else {
          gst_caps_merge_structure (caps, new_s);
        }

      } else {
        GstStructure *new_s = gst_structure_new ("video/x-h263",
            "variant", G_TYPE_STRING, "itu",
            "h263version", G_TYPE_STRING, "h263",
            NULL);

        GST_DEBUG_OBJECT (rtph263ppay, "No profile or level specified"
            " for H263-2000, defaulting to baseline H263");

        gst_caps_merge_structure (caps, new_s);
      }
    } else {
      gboolean f = FALSE, i = FALSE, j = FALSE, t = FALSE;
      /* FIXME: ffmpeg support the Appendix K too, how do we express it ?
       *   guint k;
       */
      const gchar *str;
      GstStructure *new_s = gst_structure_new ("video/x-h263",
          "variant", G_TYPE_STRING, "itu",
          NULL);
      gboolean added = FALSE;

      str = gst_structure_get_string (s, "f");
      if (str && !strcmp (str, "1"))
        f = TRUE;

      str = gst_structure_get_string (s, "i");
      if (str && !strcmp (str, "1"))
        i = TRUE;

      str = gst_structure_get_string (s, "j");
      if (str && !strcmp (str, "1"))
        j = TRUE;

      str = gst_structure_get_string (s, "t");
      if (str && !strcmp (str, "1"))
        t = TRUE;

      if (f || i || j || t) {
        GValue list = { 0 };
        GValue vstr = { 0 };

        g_value_init (&list, GST_TYPE_LIST);
        g_value_init (&vstr, G_TYPE_STRING);

        g_value_set_static_string (&vstr, "h263");
        gst_value_list_append_value (&list, &vstr);
        g_value_set_static_string (&vstr, "h263p");
        gst_value_list_append_value (&list, &vstr);
        g_value_unset (&vstr);

        gst_structure_set_value (new_s, "h263version", &list);
        g_value_unset (&list);
      } else {
        gst_structure_set (new_s, "h263version", G_TYPE_STRING, "h263", NULL);
      }

      if (!f)
        gst_structure_set (new_s, "annex-f", G_TYPE_BOOLEAN, FALSE, NULL);
      if (!i)
        gst_structure_set (new_s, "annex-i", G_TYPE_BOOLEAN, FALSE, NULL);
      if (!j)
        gst_structure_set (new_s, "annex-j", G_TYPE_BOOLEAN, FALSE, NULL);
      if (!t)
        gst_structure_set (new_s, "annex-t", G_TYPE_BOOLEAN, FALSE, NULL);


      str = gst_structure_get_string (s, "custom");
      if (str) {
        unsigned int xmax, ymax, mpi;
        if (sscanf (str, "%u,%u,%u", &xmax, &ymax, &mpi) == 3) {
          if (xmax % 4 && ymax % 4 && mpi >= 1 && mpi <= 32) {
            caps_append (caps, new_s, xmax, ymax, mpi);
            added = TRUE;
          } else {
            GST_WARNING_OBJECT (rtph263ppay, "Invalid custom framesize/MPI"
                " %u x %u at %u, ignoring", xmax, ymax, mpi);
          }
        } else {
          GST_WARNING_OBJECT (rtph263ppay, "Invalid custom framesize/MPI: %s,"
              " ignoring", str);
        }
      }

      str = gst_structure_get_string (s, "16cif");
      if (str) {
        int mpi = atoi (str);
        caps_append (caps, new_s, 1408, 1152, mpi);
        added = TRUE;
      }

      str = gst_structure_get_string (s, "4cif");
      if (str) {
        int mpi = atoi (str);
        caps_append (caps, new_s, 704, 576, mpi);
        added = TRUE;
      }

      str = gst_structure_get_string (s, "cif");
      if (str) {
        int mpi = atoi (str);
        caps_append (caps, new_s, 352, 288, mpi);
        added = TRUE;
      }

      str = gst_structure_get_string (s, "qcif");
      if (str) {
        int mpi = atoi (str);
        caps_append (caps, new_s, 176, 144, mpi);
        added = TRUE;
      }

      str = gst_structure_get_string (s, "sqcif");
      if (str) {
        int mpi = atoi (str);
        caps_append (caps, new_s, 128, 96, mpi);
        added = TRUE;
      }

      if (added)
        gst_structure_free (new_s);
      else
        gst_caps_merge_structure (caps, new_s);
    }
  }

  gst_caps_unref (intersect);

  return caps;
}


static void
gst_rtp_h263p_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpH263PPay *rtph263ppay;

  rtph263ppay = GST_RTP_H263P_PAY (object);

  switch (prop_id) {
    case PROP_FRAGMENTATION_MODE:
      rtph263ppay->fragmentation_mode = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_h263p_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpH263PPay *rtph263ppay;

  rtph263ppay = GST_RTP_H263P_PAY (object);

  switch (prop_id) {
    case PROP_FRAGMENTATION_MODE:
      g_value_set_enum (value, rtph263ppay->fragmentation_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_rtp_h263p_pay_flush (GstRtpH263PPay * rtph263ppay)
{
  guint avail;
  GstBuffer *outbuf;
  GstFlowReturn ret;
  gboolean fragmented;

  avail = gst_adapter_available (rtph263ppay->adapter);
  if (avail == 0)
    return GST_FLOW_OK;

  fragmented = FALSE;
  /* This algorithm assumes the H263/+/++ encoder sends complete frames in each
   * buffer */
  /* With Fragmentation Mode at GST_FRAGMENTATION_MODE_NORMAL:
   *  This algorithm implements the Follow-on packets method for packetization.
   *  This assumes low packet loss network. 
   * With Fragmentation Mode at GST_FRAGMENTATION_MODE_SYNC:
   *  This algorithm separates large frames at synchronisation points (Segments)
   *  (See RFC 4629 section 6). It would be interesting to have a property such as network
   *  quality to select between both packetization methods */
  /* TODO Add VRC supprt (See RFC 4629 section 5.2) */

  while (avail > 0) {
    guint towrite;
    guint8 *payload;
    guint payload_len;
    gint header_len;
    guint next_gop = 0;
    gboolean found_gob = FALSE;

    if (rtph263ppay->fragmentation_mode == GST_FRAGMENTATION_MODE_SYNC) {
      /* start after 1st gop possible */
      guint parsed_len = 3;
      const guint8 *parse_data = NULL;

      parse_data = gst_adapter_peek (rtph263ppay->adapter, avail);

      /* Check if we have a gob or eos , eossbs */
      /* FIXME EOS and EOSSBS packets should never contain any gobs and vice-versa */
      if (avail >= 3 && *parse_data == 0 && *(parse_data + 1) == 0
          && *(parse_data + 2) >= 0x80) {
        GST_DEBUG_OBJECT (rtph263ppay, " Found GOB header");
        found_gob = TRUE;
      }
      /* Find next and cut the packet accordingly */
      /* TODO we should get as many gobs as possible until MTU is reached, this
       * code seems to just get one GOB per packet */
      while (parsed_len + 2 < avail) {
        if (parse_data[parsed_len] == 0 && parse_data[parsed_len + 1] == 0
            && parse_data[parsed_len + 2] >= 0x80) {
          next_gop = parsed_len;
          GST_DEBUG_OBJECT (rtph263ppay, " Next GOB Detected at :  %d",
              next_gop);
          break;
        }
        parsed_len++;
      }
    }

    /* for picture start frames (non-fragmented), we need to remove the first
     * two 0x00 bytes and set P=1 */
    header_len = (fragmented && !found_gob) ? 2 : 0;

    towrite = MIN (avail, gst_rtp_buffer_calc_payload_len
        (GST_BASE_RTP_PAYLOAD_MTU (rtph263ppay) - header_len, 0, 0));

    if (next_gop > 0)
      towrite = MIN (next_gop, towrite);

    payload_len = header_len + towrite;

    outbuf = gst_rtp_buffer_new_allocate (payload_len, 0, 0);
    /* last fragment gets the marker bit set */
    gst_rtp_buffer_set_marker (outbuf, avail > towrite ? 0 : 1);

    payload = gst_rtp_buffer_get_payload (outbuf);

    gst_adapter_copy (rtph263ppay->adapter, &payload[header_len], 0, towrite);

    /*  0                   1
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |   RR    |P|V|   PLEN    |PEBIT|
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    /* if fragmented or gop header , write p bit =1 */
    payload[0] = (fragmented && !found_gob) ? 0x00 : 0x04;
    payload[1] = 0;

    GST_BUFFER_TIMESTAMP (outbuf) = rtph263ppay->first_timestamp;
    GST_BUFFER_DURATION (outbuf) = rtph263ppay->first_duration;

    gst_adapter_flush (rtph263ppay->adapter, towrite);

    ret = gst_basertppayload_push (GST_BASE_RTP_PAYLOAD (rtph263ppay), outbuf);

    avail -= towrite;
    fragmented = TRUE;
  }

  return ret;
}

static GstFlowReturn
gst_rtp_h263p_pay_handle_buffer (GstBaseRTPPayload * payload,
    GstBuffer * buffer)
{
  GstRtpH263PPay *rtph263ppay;
  GstFlowReturn ret;

  rtph263ppay = GST_RTP_H263P_PAY (payload);

  rtph263ppay->first_timestamp = GST_BUFFER_TIMESTAMP (buffer);
  rtph263ppay->first_duration = GST_BUFFER_DURATION (buffer);

  /* we always encode and flush a full picture */
  gst_adapter_push (rtph263ppay->adapter, buffer);
  ret = gst_rtp_h263p_pay_flush (rtph263ppay);

  return ret;
}

gboolean
gst_rtp_h263p_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtph263ppay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H263P_PAY);
}
