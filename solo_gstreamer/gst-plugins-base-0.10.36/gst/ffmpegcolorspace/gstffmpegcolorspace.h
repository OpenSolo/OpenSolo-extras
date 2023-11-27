/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * This file:
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

#ifndef __GST_FFMPEGCOLORSPACE_H__
#define __GST_FFMPEGCOLORSPACE_H__

#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include "avcodec.h"

G_BEGIN_DECLS

#define GST_TYPE_FFMPEGCSP 	      (gst_ffmpegcsp_get_type())
#define GST_FFMPEGCSP(obj) 	      (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FFMPEGCSP,GstFFMpegCsp))
#define GST_FFMPEGCSP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FFMPEGCSP,GstFFMpegCspClass))
#define GST_IS_FFMPEGCSP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FFMPEGCSP))
#define GST_IS_FFMPEGCSP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FFMPEGCSP))

typedef struct _GstFFMpegCsp GstFFMpegCsp;
typedef struct _GstFFMpegCspClass GstFFMpegCspClass;

/**
 * GstFFMpegCsp:
 *
 * Opaque object data structure.
 */
struct _GstFFMpegCsp {
  GstVideoFilter element;

  gint width, height;
  gboolean interlaced;
  gfloat fps;
  enum PixelFormat from_pixfmt, to_pixfmt;
  AVPicture from_frame, to_frame;
  AVPaletteControl *palette;
};

struct _GstFFMpegCspClass
{
  GstVideoFilterClass parent_class;
};

G_END_DECLS

#endif /* __GST_FFMPEGCOLORSPACE_H__ */
