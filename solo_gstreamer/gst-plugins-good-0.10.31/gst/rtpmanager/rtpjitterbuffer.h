/* GStreamer
 * Copyright (C) <2007> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __RTP_JITTER_BUFFER_H__
#define __RTP_JITTER_BUFFER_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>

typedef struct _RTPJitterBuffer RTPJitterBuffer;
typedef struct _RTPJitterBufferClass RTPJitterBufferClass;

#define RTP_TYPE_JITTER_BUFFER             (rtp_jitter_buffer_get_type())
#define RTP_JITTER_BUFFER(src)             (G_TYPE_CHECK_INSTANCE_CAST((src),RTP_TYPE_JITTER_BUFFER,RTPJitterBuffer))
#define RTP_JITTER_BUFFER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),RTP_TYPE_JITTER_BUFFER,RTPJitterBufferClass))
#define RTP_IS_JITTER_BUFFER(src)          (G_TYPE_CHECK_INSTANCE_TYPE((src),RTP_TYPE_JITTER_BUFFER))
#define RTP_IS_JITTER_BUFFER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),RTP_TYPE_JITTER_BUFFER))
#define RTP_JITTER_BUFFER_CAST(src)        ((RTPJitterBuffer *)(src))

/**
 * RTPJitterBufferMode:
 *
 * RTP_JITTER_BUFFER_MODE_NONE: don't do any skew correction, outgoing
 *    timestamps are calculated directly from the RTP timestamps. This mode is
 *    good for recording but not for real-time applications.
 * RTP_JITTER_BUFFER_MODE_SLAVE: calculate the skew between sender and receiver
 *    and produce smoothed adjusted outgoing timestamps. This mode is good for
 *    low latency communications.
 * RTP_JITTER_BUFFER_MODE_BUFFER: buffer packets between low/high watermarks.
 *    This mode is good for streaming communication.
 * RTP_JITTER_BUFFER_MODE_LAST: last buffer mode.
 *
 * The different buffer modes for a jitterbuffer.
 */
typedef enum {
  RTP_JITTER_BUFFER_MODE_NONE    = 0,
  RTP_JITTER_BUFFER_MODE_SLAVE   = 1,
  RTP_JITTER_BUFFER_MODE_BUFFER  = 2,
  RTP_JITTER_BUFFER_MODE_LAST
} RTPJitterBufferMode;

#define RTP_TYPE_JITTER_BUFFER_MODE (rtp_jitter_buffer_mode_get_type())
GType rtp_jitter_buffer_mode_get_type (void);

#define RTP_JITTER_BUFFER_MAX_WINDOW 512
/**
 * RTPJitterBuffer:
 *
 * A JitterBuffer in the #RTPSession
 */
struct _RTPJitterBuffer {
  GObject        object;

  GQueue        *packets;

  RTPJitterBufferMode mode;

  GstClockTime   delay;

  /* for buffering */
  gboolean          buffering;
  guint64           low_level;
  guint64           high_level;

  /* for calculating skew */
  GstClockTime   base_time;
  GstClockTime   base_rtptime;
  guint32        clock_rate;
  GstClockTime   base_extrtp;
  GstClockTime   prev_out_time;
  guint64        ext_rtptime;
  guint64        last_rtptime;
  gint64         window[RTP_JITTER_BUFFER_MAX_WINDOW];
  guint          window_pos;
  guint          window_size;
  gboolean       window_filling;
  gint64         window_min;
  gint64         skew;
  gint64         prev_send_diff;
};

struct _RTPJitterBufferClass {
  GObjectClass   parent_class;
};

GType rtp_jitter_buffer_get_type (void);

/* managing lifetime */
RTPJitterBuffer*      rtp_jitter_buffer_new              (void);

RTPJitterBufferMode   rtp_jitter_buffer_get_mode         (RTPJitterBuffer *jbuf);
void                  rtp_jitter_buffer_set_mode         (RTPJitterBuffer *jbuf, RTPJitterBufferMode mode);

GstClockTime          rtp_jitter_buffer_get_delay        (RTPJitterBuffer *jbuf);
void                  rtp_jitter_buffer_set_delay        (RTPJitterBuffer *jbuf, GstClockTime delay);

void                  rtp_jitter_buffer_reset_skew       (RTPJitterBuffer *jbuf);

gboolean              rtp_jitter_buffer_insert           (RTPJitterBuffer *jbuf, GstBuffer *buf,
                                                          GstClockTime time,
                                                          guint32 clock_rate,
                                                          gboolean *tail, gint *percent);
GstBuffer *           rtp_jitter_buffer_peek             (RTPJitterBuffer *jbuf);
GstBuffer *           rtp_jitter_buffer_pop              (RTPJitterBuffer *jbuf, gint *percent);

void                  rtp_jitter_buffer_flush            (RTPJitterBuffer *jbuf);

gboolean              rtp_jitter_buffer_is_buffering     (RTPJitterBuffer * jbuf);
void                  rtp_jitter_buffer_set_buffering    (RTPJitterBuffer * jbuf, gboolean buffering);
gint                  rtp_jitter_buffer_get_percent      (RTPJitterBuffer * jbuf);

guint                 rtp_jitter_buffer_num_packets      (RTPJitterBuffer *jbuf);
guint32               rtp_jitter_buffer_get_ts_diff      (RTPJitterBuffer *jbuf);

void                  rtp_jitter_buffer_get_sync         (RTPJitterBuffer *jbuf, guint64 *rtptime,
                                                          guint64 *timestamp, guint32 *clock_rate,
                                                          guint64 *last_rtptime);

#endif /* __RTP_JITTER_BUFFER_H__ */
