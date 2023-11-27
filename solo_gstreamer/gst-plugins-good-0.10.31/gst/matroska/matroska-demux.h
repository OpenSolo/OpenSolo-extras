/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-demux.h: matroska file/stream demuxer definition
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

#ifndef __GST_MATROSKA_DEMUX_H__
#define __GST_MATROSKA_DEMUX_H__

#include <gst/gst.h>

#include "ebml-read.h"
#include "matroska-ids.h"
#include "matroska-read-common.h"

G_BEGIN_DECLS

#define GST_TYPE_MATROSKA_DEMUX \
  (gst_matroska_demux_get_type ())
#define GST_MATROSKA_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_MATROSKA_DEMUX, GstMatroskaDemux))
#define GST_MATROSKA_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_MATROSKA_DEMUX, GstMatroskaDemuxClass))
#define GST_IS_MATROSKA_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_MATROSKA_DEMUX))
#define GST_IS_MATROSKA_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_MATROSKA_DEMUX))

typedef struct _GstMatroskaDemux {
  GstElement              parent;

  /* < private > */

  GstMatroskaReadCommon    common;

  /* pads */
  GstClock                *clock;
  guint                    num_v_streams;
  guint                    num_a_streams;
  guint                    num_t_streams;

  /* state */
  gboolean                 streaming;
  guint                    level_up;
  guint64                  seek_block;
  gboolean                 seek_first;

  /* did we parse cues/tracks/segmentinfo already? */
  gboolean                 tracks_parsed;
  GList                   *seek_parsed;

  /* cluster positions (optional) */
  GArray                  *clusters;

  /* keeping track of playback position */
  gboolean                 segment_running;
  GstClockTime             last_stop_end;
  GstClockTime             stream_start_time;

  GstEvent                *close_segment;
  GstEvent                *new_segment;

  /* some state saving */
  GstClockTime             cluster_time;
  guint64                  cluster_offset;
  guint64                  first_cluster_offset;
  guint64                  next_cluster_offset;

  /* index stuff */
  gboolean                 seekable;
  gboolean                 building_index;
  guint64                  index_offset;
  GstEvent                *seek_event;
  gboolean                 need_newsegment;

  /* reverse playback */
  GArray                  *seek_index;
  gint                     seek_entry;

  /* gap handling */
  guint64                  max_gap_time;

  /* for non-finalized files, with invalid segment duration */
  gboolean                 invalid_duration;
} GstMatroskaDemux;

typedef struct _GstMatroskaDemuxClass {
  GstElementClass parent;
} GstMatroskaDemuxClass;

gboolean gst_matroska_demux_plugin_init (GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_MATROSKA_DEMUX_H__ */
