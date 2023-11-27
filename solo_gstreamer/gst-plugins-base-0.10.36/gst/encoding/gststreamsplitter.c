/* GStreamer Stream Splitter
 * Copyright (C) 2010 Edward Hervey <edward.hervey@collabora.co.uk>
 *           (C) 2009 Nokia Corporation
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gststreamsplitter.h"
#include "gst/glib-compat-private.h"

static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE ("src_%d", GST_PAD_SRC, GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_stream_splitter_debug);
#define GST_CAT_DEFAULT gst_stream_splitter_debug

G_DEFINE_TYPE (GstStreamSplitter, gst_stream_splitter, GST_TYPE_ELEMENT);

#define STREAMS_LOCK(obj) (g_mutex_lock(obj->lock))
#define STREAMS_UNLOCK(obj) (g_mutex_unlock(obj->lock))

static void gst_stream_splitter_dispose (GObject * object);

static GstPad *gst_stream_splitter_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name);
static void gst_stream_splitter_release_pad (GstElement * element,
    GstPad * pad);

static void
gst_stream_splitter_class_init (GstStreamSplitterClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *gstelement_klass;

  gobject_klass = (GObjectClass *) klass;
  gstelement_klass = (GstElementClass *) klass;

  gobject_klass->dispose = gst_stream_splitter_dispose;

  GST_DEBUG_CATEGORY_INIT (gst_stream_splitter_debug, "streamsplitter", 0,
      "Stream Splitter");

  gst_element_class_add_static_pad_template (gstelement_klass, &src_template);
  gst_element_class_add_static_pad_template (gstelement_klass, &sink_template);

  gstelement_klass->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_stream_splitter_request_new_pad);
  gstelement_klass->release_pad =
      GST_DEBUG_FUNCPTR (gst_stream_splitter_release_pad);

  gst_element_class_set_details_simple (gstelement_klass,
      "streamsplitter", "Generic",
      "Splits streams based on their media type",
      "Edward Hervey <edward.hervey@collabora.co.uk>");
}

static void
gst_stream_splitter_dispose (GObject * object)
{
  GstStreamSplitter *stream_splitter = (GstStreamSplitter *) object;

  if (stream_splitter->lock) {
    g_mutex_free (stream_splitter->lock);
    stream_splitter->lock = NULL;
  }

  g_list_foreach (stream_splitter->pending_events, (GFunc) gst_event_unref,
      NULL);
  g_list_free (stream_splitter->pending_events);
  stream_splitter->pending_events = NULL;

  G_OBJECT_CLASS (gst_stream_splitter_parent_class)->dispose (object);
}

static GstFlowReturn
gst_stream_splitter_chain (GstPad * pad, GstBuffer * buf)
{
  GstStreamSplitter *stream_splitter =
      (GstStreamSplitter *) GST_PAD_PARENT (pad);
  GstFlowReturn res;
  GstPad *srcpad = NULL;

  STREAMS_LOCK (stream_splitter);
  if (stream_splitter->current)
    srcpad = gst_object_ref (stream_splitter->current);
  STREAMS_UNLOCK (stream_splitter);

  if (G_UNLIKELY (srcpad == NULL))
    goto nopad;

  if (G_UNLIKELY (stream_splitter->pending_events)) {
    GList *tmp;
    GST_DEBUG_OBJECT (srcpad, "Pushing out pending events");

    for (tmp = stream_splitter->pending_events; tmp; tmp = tmp->next) {
      GstEvent *event = (GstEvent *) tmp->data;
      gst_pad_push_event (srcpad, event);
    }
    g_list_free (stream_splitter->pending_events);
    stream_splitter->pending_events = NULL;
  }

  /* Forward to currently activated stream */
  res = gst_pad_push (srcpad, buf);
  gst_object_unref (srcpad);

  return res;

nopad:
  GST_WARNING_OBJECT (stream_splitter, "No output pad was configured");
  return GST_FLOW_ERROR;
}

static gboolean
gst_stream_splitter_sink_event (GstPad * pad, GstEvent * event)
{
  GstStreamSplitter *stream_splitter =
      (GstStreamSplitter *) GST_PAD_PARENT (pad);
  gboolean res = TRUE;
  gboolean toall = FALSE;
  gboolean store = FALSE;
  gboolean eos = FALSE;
  gboolean flushpending = FALSE;

  /* FLUSH_START/STOP : forward to all
   * EOS : transform to CUSTOM_REAL_EOS and forward to all
   * INBAND events : store to send in chain function to selected chain
   * OUT_OF_BAND events : send to all
   */

  GST_DEBUG_OBJECT (stream_splitter, "Got event %s",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      flushpending = TRUE;
      toall = TRUE;
      break;
    case GST_EVENT_FLUSH_START:
      toall = TRUE;
      break;
    case GST_EVENT_EOS:
      /* Replace with our custom eos event */
      gst_event_unref (event);
      event =
          gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
          gst_structure_empty_new ("stream-switching-eos"));
      toall = TRUE;
      eos = TRUE;
      break;
    default:
      if (GST_EVENT_TYPE (event) & GST_EVENT_TYPE_SERIALIZED)
        store = TRUE;
  }

  if (flushpending) {
    g_list_foreach (stream_splitter->pending_events, (GFunc) gst_event_unref,
        NULL);
    g_list_free (stream_splitter->pending_events);
    stream_splitter->pending_events = NULL;
  }

  if (store) {
    stream_splitter->pending_events =
        g_list_append (stream_splitter->pending_events, event);
  } else if (toall || eos) {
    GList *tmp;
    guint32 cookie;

    /* Send to all pads */
    STREAMS_LOCK (stream_splitter);
  resync:
    if (G_UNLIKELY (stream_splitter->srcpads == NULL)) {
      STREAMS_UNLOCK (stream_splitter);
      /* No source pads */
      gst_event_unref (event);
      res = FALSE;
      goto beach;
    }
    tmp = stream_splitter->srcpads;
    cookie = stream_splitter->cookie;
    while (tmp) {
      GstPad *srcpad = (GstPad *) tmp->data;
      STREAMS_UNLOCK (stream_splitter);
      /* In case of EOS, we first push out the real one to flush out
       * each streams (but which will be discarded in the streamcombiner)
       * before our custom one (which will be converted back to and EOS
       * in the streamcombiner) */
      if (eos)
        gst_pad_push_event (srcpad, gst_event_new_eos ());
      gst_event_ref (event);
      res = gst_pad_push_event (srcpad, event);
      STREAMS_LOCK (stream_splitter);
      if (G_UNLIKELY (cookie != stream_splitter->cookie))
        goto resync;
      tmp = tmp->next;
    }
    STREAMS_UNLOCK (stream_splitter);
    gst_event_unref (event);
  } else {
    GstPad *pad;

    /* Only send to current pad */

    STREAMS_LOCK (stream_splitter);
    pad = stream_splitter->current;
    STREAMS_UNLOCK (stream_splitter);
    if (pad)
      res = gst_pad_push_event (pad, event);
    else {
      gst_event_unref (event);
      res = FALSE;
    }
  }

beach:
  return res;
}

static GstCaps *
gst_stream_splitter_sink_getcaps (GstPad * pad)
{
  GstStreamSplitter *stream_splitter =
      (GstStreamSplitter *) GST_PAD_PARENT (pad);
  guint32 cookie;
  GList *tmp;
  GstCaps *res = NULL;

  /* Return the combination of all downstream caps */

  STREAMS_LOCK (stream_splitter);

resync:
  if (G_UNLIKELY (stream_splitter->srcpads == NULL)) {
    res = gst_caps_new_any ();
    goto beach;
  }

  res = NULL;
  cookie = stream_splitter->cookie;
  tmp = stream_splitter->srcpads;

  while (tmp) {
    GstPad *srcpad = (GstPad *) tmp->data;

    STREAMS_UNLOCK (stream_splitter);
    if (res) {
      GstCaps *peercaps = gst_pad_peer_get_caps_reffed (srcpad);
      if (peercaps)
        gst_caps_merge (res, gst_caps_make_writable (peercaps));
    } else {
      res = gst_pad_peer_get_caps (srcpad);
    }
    STREAMS_LOCK (stream_splitter);

    if (G_UNLIKELY (cookie != stream_splitter->cookie)) {
      if (res)
        gst_caps_unref (res);
      goto resync;
    }
    tmp = tmp->next;
  }

beach:
  STREAMS_UNLOCK (stream_splitter);
  return res;
}

static gboolean
gst_stream_splitter_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstStreamSplitter *stream_splitter =
      (GstStreamSplitter *) GST_PAD_PARENT (pad);
  guint32 cookie;
  GList *tmp;
  gboolean res;

  GST_DEBUG_OBJECT (stream_splitter, "caps %" GST_PTR_FORMAT, caps);

  /* Try on all pads, choose the one that succeeds as the current stream */
  STREAMS_LOCK (stream_splitter);

resync:
  if (G_UNLIKELY (stream_splitter->srcpads == NULL)) {
    res = FALSE;
    goto beach;
  }

  res = FALSE;
  tmp = stream_splitter->srcpads;
  cookie = stream_splitter->cookie;

  while (tmp) {
    GstPad *srcpad = (GstPad *) tmp->data;
    GstCaps *peercaps;

    STREAMS_UNLOCK (stream_splitter);
    peercaps = gst_pad_peer_get_caps_reffed (srcpad);
    if (peercaps) {
      res = gst_caps_can_intersect (caps, peercaps);
      gst_caps_unref (peercaps);
    }
    STREAMS_LOCK (stream_splitter);

    if (G_UNLIKELY (cookie != stream_splitter->cookie))
      goto resync;

    if (res) {
      /* FIXME : we need to switch properly */
      GST_DEBUG_OBJECT (srcpad, "Setting caps on this pad was successful");
      stream_splitter->current = srcpad;
      goto beach;
    }
    tmp = tmp->next;
  }

beach:
  STREAMS_UNLOCK (stream_splitter);
  return res;
}

static gboolean
gst_stream_splitter_src_event (GstPad * pad, GstEvent * event)
{
  GstStreamSplitter *stream_splitter =
      (GstStreamSplitter *) GST_PAD_PARENT (pad);

  GST_DEBUG_OBJECT (pad, "%s", GST_EVENT_TYPE_NAME (event));

  /* Forward upstream as is */
  return gst_pad_push_event (stream_splitter->sinkpad, event);
}

static gboolean
gst_stream_splitter_src_query (GstPad * pad, GstQuery * query)
{
  GstStreamSplitter *stream_splitter =
      (GstStreamSplitter *) GST_PAD_PARENT (pad);

  GST_DEBUG_OBJECT (pad, "%s", GST_QUERY_TYPE_NAME (query));

  /* Forward upstream as is */
  return gst_pad_peer_query (stream_splitter->sinkpad, query);
}

static void
gst_stream_splitter_init (GstStreamSplitter * stream_splitter)
{
  stream_splitter->sinkpad =
      gst_pad_new_from_static_template (&sink_template, "sink");
  /* FIXME : No buffer alloc for the time being, it will resort to the fallback */
  /* gst_pad_set_bufferalloc_function (stream_splitter->sinkpad, */
  /*     gst_stream_splitter_buffer_alloc); */
  gst_pad_set_chain_function (stream_splitter->sinkpad,
      gst_stream_splitter_chain);
  gst_pad_set_event_function (stream_splitter->sinkpad,
      gst_stream_splitter_sink_event);
  gst_pad_set_getcaps_function (stream_splitter->sinkpad,
      gst_stream_splitter_sink_getcaps);
  gst_pad_set_setcaps_function (stream_splitter->sinkpad,
      gst_stream_splitter_sink_setcaps);
  gst_element_add_pad (GST_ELEMENT (stream_splitter), stream_splitter->sinkpad);

  stream_splitter->lock = g_mutex_new ();
}

static GstPad *
gst_stream_splitter_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name)
{
  GstStreamSplitter *stream_splitter = (GstStreamSplitter *) element;
  GstPad *srcpad;

  srcpad = gst_pad_new_from_static_template (&src_template, name);
  gst_pad_set_event_function (srcpad, gst_stream_splitter_src_event);
  gst_pad_set_query_function (srcpad, gst_stream_splitter_src_query);

  STREAMS_LOCK (stream_splitter);
  stream_splitter->srcpads = g_list_append (stream_splitter->srcpads, srcpad);
  gst_pad_set_active (srcpad, TRUE);
  gst_element_add_pad (element, srcpad);
  stream_splitter->cookie++;
  STREAMS_UNLOCK (stream_splitter);

  return srcpad;
}

static void
gst_stream_splitter_release_pad (GstElement * element, GstPad * pad)
{
  GstStreamSplitter *stream_splitter = (GstStreamSplitter *) element;
  GList *tmp;

  STREAMS_LOCK (stream_splitter);
  tmp = g_list_find (stream_splitter->srcpads, pad);
  if (tmp) {
    GstPad *pad = (GstPad *) tmp->data;

    stream_splitter->srcpads =
        g_list_delete_link (stream_splitter->srcpads, tmp);
    stream_splitter->cookie++;

    if (pad == stream_splitter->current) {
      /* Deactivate current flow */
      GST_DEBUG_OBJECT (element, "Removed pad was the current one");
      stream_splitter->current = NULL;
    }

    gst_element_remove_pad (element, pad);
  }
  STREAMS_UNLOCK (stream_splitter);

  return;
}
