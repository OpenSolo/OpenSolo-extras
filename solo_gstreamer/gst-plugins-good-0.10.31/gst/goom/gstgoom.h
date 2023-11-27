/* gstgoom.c: implementation of goom drawing element
 * Copyright (C) <2001> Richard Boulton <richard@tartarus.org>
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

#ifndef __GST_GOOM_H__
#define __GST_GOOM_H__

G_BEGIN_DECLS

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include "goom.h"

#define GOOM_SAMPLES 512

#define GST_TYPE_GOOM            (gst_goom_get_type())
#define GST_GOOM(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GOOM,GstGoom))
#define GST_GOOM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GOOM,GstGoomClass))
#define GST_IS_GOOM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GOOM))
#define GST_IS_GOOM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GOOM))

typedef struct _GstGoom GstGoom;
typedef struct _GstGoomClass GstGoomClass;

struct _GstGoom
{
  GstElement element;

  /* pads */
  GstPad *sinkpad, *srcpad;
  GstAdapter *adapter;

  /* input tracking */
  gint rate;
  gint channels;
  guint bps;

  /* video state */
  gint fps_n;
  gint fps_d;
  gint width;
  gint height;
  GstClockTime duration;
  guint outsize;

  /* samples per frame */
  guint spf;
  /* bytes per frame */
  guint bpf;

  /* goom stuff */
  gint16 datain[2][GOOM_SAMPLES];
  PluginInfo *plugin;

  /* segment state */
  GstSegment segment;

  /* QoS stuff *//* with LOCK */
  gdouble proportion;
  GstClockTime earliest_time;
};

struct _GstGoomClass
{
  GstElementClass parent_class;
};

GType gst_goom_get_type (void);

G_END_DECLS

#endif /* __GST_GOOM_H__ */

