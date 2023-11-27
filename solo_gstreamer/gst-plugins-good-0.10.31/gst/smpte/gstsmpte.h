/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_SMPTE_H__
#define __GST_SMPTE_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

G_BEGIN_DECLS

#include "gstmask.h"

#define GST_TYPE_SMPTE \
  (gst_smpte_get_type())
#define GST_SMPTE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SMPTE,GstSMPTE))
#define GST_SMPTE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SMPTE,GstSMPTEClass))
#define GST_IS_SMPTE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SMPTE))
#define GST_IS_SMPTE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SMPTE))

typedef struct _GstSMPTE GstSMPTE;
typedef struct _GstSMPTEClass GstSMPTEClass;

struct _GstSMPTE {
  GstElement     element;

  /* pads */
  GstPad        *srcpad,
                *sinkpad1,
                *sinkpad2;
  GstCollectPads *collect;

  /* properties */
  gint           type;
  gint           border;
  gint           depth;
  guint64        duration;
  gboolean       invert;

  /* negotiated format */
  gint           format;
  gint           width;
  gint           height;
  gdouble        fps;
  gint           fps_num;
  gint           fps_denom;

  /* state of the effect */
  gint           position;
  gint           end_position;
  GstMask       *mask;
};

struct _GstSMPTEClass {
  GstElementClass parent_class;
};

gboolean gst_smpte_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_SMPTE_H__ */
