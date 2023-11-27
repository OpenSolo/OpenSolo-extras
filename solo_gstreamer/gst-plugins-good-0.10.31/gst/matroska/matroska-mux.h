/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2005 Michal Benes <michal.benes@xeris.cz>
 *
 * matroska-mux.h: matroska file/stream muxer object types
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

#ifndef __GST_MATROSKA_MUX_H__
#define __GST_MATROSKA_MUX_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

#include "ebml-write.h"
#include "matroska-ids.h"

G_BEGIN_DECLS

#define GST_TYPE_MATROSKA_MUX \
  (gst_matroska_mux_get_type ())
#define GST_MATROSKA_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_MATROSKA_MUX, GstMatroskaMux))
#define GST_MATROSKA_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_MATROSKA_MUX, GstMatroskaMuxClass))
#define GST_IS_MATROSKA_MUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_MATROSKA_MUX))
#define GST_IS_MATROSKA_MUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_MATROSKA_MUX))

typedef enum {
  GST_MATROSKA_MUX_STATE_START,
  GST_MATROSKA_MUX_STATE_HEADER,
  GST_MATROSKA_MUX_STATE_DATA,
} GstMatroskaMuxState;

typedef struct _GstMatroskaMetaSeekIndex {
  guint32  id;
  guint64  pos;
} GstMatroskaMetaSeekIndex;

/* all information needed for one matroska stream */
typedef struct
{
  GstCollectData collect;       /* we extend the CollectData */
  GstMatroskaTrackContext *track;

  GstBuffer *buffer;            /* the queued buffer for this pad */

  guint64 duration;
  GstClockTime start_ts;
  GstClockTime end_ts;    /* last timestamp + (if available) duration */
  guint64 default_duration_scaled;
}
GstMatroskaPad;


typedef struct _GstMatroskaMux {
  GstElement     element;
  
  /* < private > */

  /* pads */
  GstPad        *srcpad;
  GstCollectPads *collect;
  GstPadEventFunction collect_event;
  GstEbmlWrite *ebml_write;

  guint          num_streams,
                 num_v_streams, num_a_streams, num_t_streams;

  /* Application name (for the writing application header element) */
  gchar          *writing_app;

  /* EBML DocType. */
  const gchar    *doctype;

  /* DocType version. */
  guint          doctype_version;

  /* state */
  GstMatroskaMuxState state;

  /* a cue (index) table */
  GstMatroskaIndex *index;
  guint          num_indexes;
  GstClockTimeDiff min_index_interval;
  gboolean       streamable;
 
  /* timescale in the file */
  guint64        time_scale;
  /* based on timescale, limit of nanoseconds you can have in a cluster */ 
  guint64        max_cluster_duration;

  /* length, position (time, ns) */
  guint64        duration;

  /* byte-positions of master-elements (for replacing contents) */
  guint64        segment_pos,
                 seekhead_pos,
                 cues_pos,
                 tags_pos,
                 info_pos,
                 tracks_pos,
                 duration_pos,
                 meta_pos;
  guint64        segment_master;

  /* current cluster */
  guint64        cluster,
                 cluster_time,
                 cluster_pos,
		 prev_cluster_size;

  /* GstForceKeyUnit event */
  GstEvent       *force_key_unit_event;

} GstMatroskaMux;

typedef struct _GstMatroskaMuxClass {
  GstElementClass parent;
} GstMatroskaMuxClass;

GType   gst_matroska_mux_get_type (void);

G_END_DECLS

#endif /* __GST_MATROSKA_MUX_H__ */
