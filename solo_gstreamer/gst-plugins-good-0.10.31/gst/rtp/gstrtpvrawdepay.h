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

#ifndef __GST_RTP_VRAW_DEPAY_H__
#define __GST_RTP_VRAW_DEPAY_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/rtp/gstbasertpdepayload.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_VRAW_DEPAY \
  (gst_rtp_vraw_depay_get_type())
#define GST_RTP_VRAW_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_VRAW_DEPAY,GstRtpVRawDepay))
#define GST_RTP_VRAW_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_VRAW_DEPAY,GstRtpVRawDepayClass))
#define GST_IS_RTP_VRAW_DEPAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_VRAW_DEPAY))
#define GST_IS_RTP_VRAW_DEPAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_VRAW_DEPAY))

typedef struct _GstRtpVRawDepay GstRtpVRawDepay;
typedef struct _GstRtpVRawDepayClass GstRtpVRawDepayClass;

struct _GstRtpVRawDepay
{
  GstBaseRTPDepayload payload;

  gint width, height;
  GstVideoFormat format;

  GstBuffer *outbuf;
  guint32 timestamp;
  guint outsize;

  gint pgroup;
  gint xinc, yinc;
  guint yp, up, vp;
  gint ystride;
  gint uvstride;
};

struct _GstRtpVRawDepayClass
{
  GstBaseRTPDepayloadClass parent_class;
};

GType gst_rtp_vraw_depay_get_type (void);

gboolean gst_rtp_vraw_depay_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_RTP_VRAW_DEPAY_H__ */
