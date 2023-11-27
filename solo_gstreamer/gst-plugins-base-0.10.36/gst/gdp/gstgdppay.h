/* GStreamer
 * Copyright (C) 2006 Thomas Vander Stichele <thomas at apestaart dot org>
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

#ifndef __GST_GDP_PAY_H__
#define __GST_GDP_PAY_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_GDP_PAY \
  (gst_gdp_pay_get_type())
#define GST_GDP_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GDP_PAY,GstGDPPay))
#define GST_GDP_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GDP_PAY,GstGDPPayClass))
#define GST_IS_GDP_PAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GDP_PAY))
#define GST_IS_GDP_PAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GDP_PAY))

typedef struct _GstGDPPay GstGDPPay;
typedef struct _GstGDPPayClass GstGDPPayClass;

/**
 * GstGDPPay:
 *
 * Private gdppay element structure.
 */
struct _GstGDPPay
{
  GstElement element;

  GstPad *sinkpad;
  GstPad *srcpad;

  GstCaps *caps; /* incoming caps */

  GstBuffer *caps_buf;
  GstBuffer *new_segment_buf;
  GstBuffer *tag_buf;

  gboolean sent_streamheader; /* TRUE after the first streamheaders are sent */
  GList *queue; /* list of queued buffers before streamheaders are sent */
  guint64 offset;

  gboolean crc_header;
  gboolean crc_payload;
  GstDPHeaderFlag header_flag;
  GstDPVersion version;
  GstDPPacketizer *packetizer;
};

struct _GstGDPPayClass
{
  GstElementClass parent_class;
};

gboolean gst_gdp_pay_plugin_init (GstPlugin * plugin);

GType gst_gdp_pay_get_type (void);

G_END_DECLS

#endif /* __GST_GDP_PAY_H__ */
