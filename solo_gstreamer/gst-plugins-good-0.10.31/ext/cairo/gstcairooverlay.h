/* GStreamer
 * Copyright (C) <2011> Jon Nordby <jononor@gmail.com>
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

#ifndef __GST_CAIRO_OVERLAY_H__
#define __GST_CAIRO_OVERLAY_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#include <cairo.h>
#include <cairo-gobject.h>

G_BEGIN_DECLS

#define GST_TYPE_CAIRO_OVERLAY \
  (gst_cairo_overlay_get_type())
#define GST_CAIRO_OVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CAIRO_OVERLAY,GstCairoOverlay))
#define GST_CAIRO_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CAIRO_OVERLAY,GstCairoOverlayClass))
#define GST_IS_CAIRO_OVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CAIRO_OVERLAY))
#define GST_IS_CAIRO_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CAIRO_OVERLAY))

typedef struct _GstCairoOverlay GstCairoOverlay;
typedef struct _GstCairoOverlayClass GstCairoOverlayClass;

struct _GstCairoOverlay {
  GstVideoFilter parent_instance;

  /* < private > */
  GstVideoFormat format;
  gint width;
  gint height;
};

struct _GstCairoOverlayClass {
  GstVideoFilterClass parent_class;
};

GType gst_cairo_overlay_get_type(void);

G_END_DECLS

#endif /* __GST_CAIRO_OVERLAY_H__ */
