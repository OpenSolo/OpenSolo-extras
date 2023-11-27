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

#ifndef __GST_RTP_MP4V_PAY_H__
#define __GST_RTP_MP4V_PAY_H__

#include <gst/gst.h>
#include <gst/rtp/gstbasertppayload.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_MP4V_PAY \
  (gst_rtp_mp4v_pay_get_type())
#define GST_RTP_MP4V_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_MP4V_PAY,GstRtpMP4VPay))
#define GST_RTP_MP4V_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_MP4V_PAY,GstRtpMP4VPayClass))
#define GST_IS_RTP_MP4V_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_MP4V_PAY))
#define GST_IS_RTP_MP4V_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_MP4V_PAY))

typedef struct _GstRtpMP4VPay GstRtpMP4VPay;
typedef struct _GstRtpMP4VPayClass GstRtpMP4VPayClass;

struct _GstRtpMP4VPay
{
  GstBaseRTPPayload    payload;

  GstAdapter   *adapter;
  GstClockTime  first_timestamp;
  GstClockTime  duration;

  gint          rate;
  gint          profile;
  GstBuffer    *config;
  gboolean      send_config;
  gboolean      need_config;

  gboolean      buffer_list;

  /* naming might be confusing with send_config; but naming matches h264
   * payloader */
  guint         config_interval;
  GstClockTime  last_config;
};

struct _GstRtpMP4VPayClass
{
  GstBaseRTPPayloadClass parent_class;
};

GType gst_rtp_mp4v_pay_get_type (void);

gboolean gst_rtp_mp4v_pay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_MP4V_PAY_H__ */
