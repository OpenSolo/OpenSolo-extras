/* GStreamer
 *
 * gstv4lxoverlay.h: tv mixer interface implementation for V4L
 *
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
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

#ifndef __GST_V4L_X_OVERLAY_H__
#define __GST_V4L_X_OVERLAY_H__

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "gstv4lelement.h"

G_BEGIN_DECLS

void    gst_v4l_xoverlay_interface_init (GstXOverlayClass *klass);

void    gst_v4l_xoverlay_start          (GstV4lElement * v4lelement);
void    gst_v4l_xoverlay_stop           (GstV4lElement * v4lelement);

G_END_DECLS

#endif /* __GST_V4L_X_OVERLAY_H__ */
