/* GStreamer
 *
 * Copyright (c) 2008 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#ifndef __GST_FLV_MUX_H__
#define __GST_FLV_MUX_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

G_BEGIN_DECLS

#define GST_TYPE_FLV_MUX \
  (gst_flv_mux_get_type ())
#define GST_FLV_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_FLV_MUX, GstFlvMux))
#define GST_FLV_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_FLV_MUX, GstFlvMuxClass))
#define GST_IS_FLV_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_FLV_MUX))
#define GST_IS_FLV_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_FLV_MUX))

typedef struct
{
  GstCollectData collect;

  gboolean video;

  guint audio_codec;
  guint rate;
  guint width;
  guint channels;
  GstBuffer *audio_codec_data;

  guint video_codec;
  GstBuffer *video_codec_data;

  GstClockTime last_timestamp;
} GstFlvPad;

typedef enum
{
  GST_FLV_MUX_STATE_HEADER,
  GST_FLV_MUX_STATE_DATA
} GstFlvMuxState;

typedef struct _GstFlvMux {
  GstElement     element;

  GstPad         *srcpad;
  GstCollectPads *collect;

  /* <private> */
  GstPadEventFunction collect_event;

  GstFlvMuxState state;
  gboolean have_audio;
  gboolean have_video;
  gboolean streamable;

  GstTagList *tags;
  gboolean new_tags;
  GList *index;
  guint64 byte_count;
  guint64 duration;
} GstFlvMux;

typedef struct _GstFlvMuxClass {
  GstElementClass parent;
} GstFlvMuxClass;

GType    gst_flv_mux_get_type    (void);

G_END_DECLS

#endif /* __GST_FLV_MUX_H__ */
