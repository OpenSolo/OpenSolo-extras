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

#ifndef __GST_MULAWDECODE_H__
#define __GST_MULAWDECODE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_MULAWDEC \
  (gst_mulawdec_get_type())
#define GST_MULAWDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULAWDEC,GstMuLawDec))
#define GST_MULAWDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULAWDEC,GstMuLawDecClass))
#define GST_IS_MULAWDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULAWDEC))
#define GST_IS_MULAWDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULAWDEC))

typedef struct _GstMuLawDec GstMuLawDec;
typedef struct _GstMuLawDecClass GstMuLawDecClass;

struct _GstMuLawDec {
  GstElement element;

  GstPad *sinkpad,*srcpad;

  gint rate;
  gint channels;
};

struct _GstMuLawDecClass {
  GstElementClass parent_class;
};

GType gst_mulawdec_get_type(void);

G_END_DECLS

#endif /* __GST_STEREO_H__ */
