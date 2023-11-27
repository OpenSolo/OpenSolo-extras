/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2005> Tim-Philipp Müller <tim@centricular.net>
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


#ifndef __GST_TIME_OVERLAY_H__
#define __GST_TIME_OVERLAY_H__

#include "gsttextoverlay.h"

G_BEGIN_DECLS

#define GST_TYPE_TIME_OVERLAY \
  (gst_time_overlay_get_type())
#define GST_TIME_OVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TIME_OVERLAY,GstTimeOverlay))
#define GST_TIME_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TIME_OVERLAY,GstTimeOverlayClass))
#define GST_IS_TIME_OVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TIME_OVERLAY))
#define GST_IS_TIME_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TIME_OVERLAY))

typedef struct _GstTimeOverlay GstTimeOverlay;
typedef struct _GstTimeOverlayClass GstTimeOverlayClass;

/**
 * GstTimeOverlay:
 *
 * Opaque timeoverlay data structure.
 */
struct _GstTimeOverlay {
  GstTextOverlay textoverlay;
};

struct _GstTimeOverlayClass {
  GstTextOverlayClass parent_class;
};

GType gst_time_overlay_get_type (void);

G_END_DECLS

#endif /* __GST_TIME_OVERLAY_H__ */

