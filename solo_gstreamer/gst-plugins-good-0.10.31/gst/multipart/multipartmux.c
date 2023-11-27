/* multipart muxer plugin for GStreamer
 * Copyright (C) 2004 Wim Taymans <wim@fluendo.com>
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

/**
 * SECTION:element-multipartmux
 *
 * MultipartMux uses the #GstCaps of the sink pad as the Content-type field for
 * incoming buffers when muxing them to a multipart stream. Most of the time 
 * multipart streams are sequential JPEG frames.
 *
 * <refsect2>
 * <title>Sample pipelines</title>
 * |[
 * gst-launch videotestsrc ! video/x-raw-yuv, framerate='(fraction)'5/1 ! jpegenc ! multipartmux ! filesink location=/tmp/test.multipart
 * ]| a pipeline to mux 5 JPEG frames per second into a multipart stream
 * stored to a file.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "multipartmux.h"

GST_DEBUG_CATEGORY_STATIC (gst_multipart_mux_debug);
#define GST_CAT_DEFAULT gst_multipart_mux_debug

#define DEFAULT_BOUNDARY        "ThisRandomString"

enum
{
  ARG_0,
  ARG_BOUNDARY
      /* FILL ME */
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("multipart/x-mixed-replace")
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink_%d",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY         /* we can take anything, really */
    );

typedef struct
{
  const gchar *key;
  const gchar *val;
} MimeTypeMap;

/* convert from gst structure names to mime types. Add more when needed. */
static const MimeTypeMap mimetypes[] = {
  {"audio/x-mulaw", "audio/basic"},
  {NULL, NULL}
};

static void gst_multipart_mux_base_init (gpointer g_class);
static void gst_multipart_mux_class_init (GstMultipartMuxClass * klass);
static void gst_multipart_mux_init (GstMultipartMux * multipart_mux);

static void gst_multipart_mux_finalize (GObject * object);

static gboolean gst_multipart_mux_handle_src_event (GstPad * pad,
    GstEvent * event);
static GstPad *gst_multipart_mux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name);
static GstStateChangeReturn gst_multipart_mux_change_state (GstElement *
    element, GstStateChange transition);

static GstFlowReturn gst_multipart_mux_collected (GstCollectPads * pads,
    GstMultipartMux * mux);

static void gst_multipart_mux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_multipart_mux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstElementClass *parent_class = NULL;

GType
gst_multipart_mux_get_type (void)
{
  static GType multipart_mux_type = 0;

  if (!multipart_mux_type) {
    static const GTypeInfo multipart_mux_info = {
      sizeof (GstMultipartMuxClass),
      gst_multipart_mux_base_init,
      NULL,
      (GClassInitFunc) gst_multipart_mux_class_init,
      NULL,
      NULL,
      sizeof (GstMultipartMux),
      0,
      (GInstanceInitFunc) gst_multipart_mux_init,
    };

    multipart_mux_type =
        g_type_register_static (GST_TYPE_ELEMENT, "GstMultipartMux",
        &multipart_mux_info, 0);
  }
  return multipart_mux_type;
}

static void
gst_multipart_mux_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class, &src_factory);
  gst_element_class_add_static_pad_template (element_class, &sink_factory);

  gst_element_class_set_details_simple (element_class, "Multipart muxer",
      "Codec/Muxer", "mux multipart streams", "Wim Taymans <wim@fluendo.com>");
}

static void
gst_multipart_mux_class_init (GstMultipartMuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  gint i;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_multipart_mux_finalize;
  gobject_class->get_property = gst_multipart_mux_get_property;
  gobject_class->set_property = gst_multipart_mux_set_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_BOUNDARY,
      g_param_spec_string ("boundary", "Boundary", "Boundary string",
          DEFAULT_BOUNDARY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->request_new_pad = gst_multipart_mux_request_new_pad;
  gstelement_class->change_state = gst_multipart_mux_change_state;

  /* populate mime types */
  klass->mimetypes = g_hash_table_new (g_str_hash, g_str_equal);
  for (i = 0; mimetypes[i].key; i++) {
    g_hash_table_insert (klass->mimetypes, (gpointer) mimetypes[i].key,
        (gpointer) mimetypes[i].val);
  }
}

static void
gst_multipart_mux_init (GstMultipartMux * multipart_mux)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (multipart_mux);

  multipart_mux->srcpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  gst_pad_set_event_function (multipart_mux->srcpad,
      gst_multipart_mux_handle_src_event);
  gst_element_add_pad (GST_ELEMENT (multipart_mux), multipart_mux->srcpad);

  multipart_mux->boundary = g_strdup (DEFAULT_BOUNDARY);

  multipart_mux->collect = gst_collect_pads_new ();
  gst_collect_pads_set_function (multipart_mux->collect,
      (GstCollectPadsFunction) GST_DEBUG_FUNCPTR (gst_multipart_mux_collected),
      multipart_mux);
}

static void
gst_multipart_mux_finalize (GObject * object)
{
  GstMultipartMux *multipart_mux;

  multipart_mux = GST_MULTIPART_MUX (object);

  g_free (multipart_mux->boundary);

  if (multipart_mux->collect)
    gst_object_unref (multipart_mux->collect);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstPad *
gst_multipart_mux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name)
{
  GstMultipartMux *multipart_mux;
  GstPad *newpad;
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
  gchar *name;

  if (templ != gst_element_class_get_pad_template (klass, "sink_%d"))
    goto wrong_template;

  multipart_mux = GST_MULTIPART_MUX (element);

  /* create new pad with the name */
  name = g_strdup_printf ("sink_%02d", multipart_mux->numpads);
  newpad = gst_pad_new_from_template (templ, name);
  g_free (name);

  /* construct our own wrapper data structure for the pad to
   * keep track of its status */
  {
    GstMultipartPadData *multipartpad;

    multipartpad = (GstMultipartPadData *)
        gst_collect_pads_add_pad (multipart_mux->collect, newpad,
        sizeof (GstMultipartPadData));

    /* save a pointer to our data in the pad */
    gst_pad_set_element_private (newpad, multipartpad);
    multipart_mux->numpads++;
  }

  /* add the pad to the element */
  gst_element_add_pad (element, newpad);

  return newpad;

  /* ERRORS */
wrong_template:
  {
    g_warning ("multipart_mux: this is not our template!");
    return NULL;
  }
}

/* handle events */
static gboolean
gst_multipart_mux_handle_src_event (GstPad * pad, GstEvent * event)
{
  GstMultipartMux *multipart_mux;
  GstEventType type;

  multipart_mux = GST_MULTIPART_MUX (gst_pad_get_parent (pad));

  type = event ? GST_EVENT_TYPE (event) : GST_EVENT_UNKNOWN;

  switch (type) {
    case GST_EVENT_SEEK:
      /* disable seeking for now */
      return FALSE;
    default:
      break;
  }

  gst_object_unref (multipart_mux);

  return gst_pad_event_default (pad, event);
}

static const gchar *
gst_multipart_mux_get_mime (GstMultipartMux * mux, GstStructure * s)
{
  GstMultipartMuxClass *klass;
  const gchar *mime;
  const gchar *name;
  gint rate;
  gint channels;
  gint bitrate = 0;

  klass = GST_MULTIPART_MUX_GET_CLASS (mux);

  name = gst_structure_get_name (s);

  /* use hashtable to convert to mime type */
  mime = g_hash_table_lookup (klass->mimetypes, name);
  if (mime == NULL) {
    if (!strcmp (name, "audio/x-adpcm"))
      gst_structure_get_int (s, "bitrate", &bitrate);

    switch (bitrate) {
      case 16000:
        mime = "audio/G726-16";
        break;
      case 24000:
        mime = "audio/G726-24";
        break;
      case 32000:
        mime = "audio/G726-32";
        break;
      case 40000:
        mime = "audio/G726-40";
        break;
      default:
        /* no mime type mapping, use name */
        mime = name;
        break;
    }
  }
  /* RFC2046 requires audio/basic to be mulaw 8000Hz mono */
  if (g_ascii_strcasecmp (mime, "audio/basic") == 0) {
    if (gst_structure_get_int (s, "rate", &rate) &&
        gst_structure_get_int (s, "channels", &channels)) {
      if (rate != 8000 || channels != 1) {
        mime = name;
      }
    } else {
      mime = name;
    }
  }
  return mime;
}

/*
 * Given two pads, compare the buffers queued on it and return 0 if they have
 * an equal priority, 1 if the new pad is better, -1 if the old pad is better 
 */
static gint
gst_multipart_mux_compare_pads (GstMultipartMux * multipart_mux,
    GstMultipartPadData * old, GstMultipartPadData * new)
{
  guint64 oldtime, newtime;

  /* if the old pad doesn't contain anything or is even NULL, return 
   * the new pad as best candidate and vice versa */
  if (old == NULL || old->buffer == NULL)
    return 1;
  if (new == NULL || new->buffer == NULL)
    return -1;

  /* no timestamp on old buffer, it must go first */
  oldtime = old->timestamp;
  if (oldtime == GST_CLOCK_TIME_NONE)
    return -1;

  /* no timestamp on new buffer, it must go first */
  newtime = new->timestamp;
  if (newtime == GST_CLOCK_TIME_NONE)
    return 1;

  /* old buffer has higher timestamp, new one should go first */
  if (newtime < oldtime)
    return 1;
  /* new buffer has higher timestamp, old one should go first */
  else if (newtime > oldtime)
    return -1;

  /* same priority if all of the above failed */
  return 0;
}

/* make sure a buffer is queued on all pads, returns a pointer to an multipartpad
 * that holds the best buffer or NULL when no pad was usable */
static GstMultipartPadData *
gst_multipart_mux_queue_pads (GstMultipartMux * mux)
{
  GSList *walk = NULL;
  GstMultipartPadData *bestpad = NULL;

  g_return_val_if_fail (GST_IS_MULTIPART_MUX (mux), NULL);

  /* try to make sure we have a buffer from each usable pad first */
  walk = mux->collect->data;
  while (walk) {
    GstCollectData *data = (GstCollectData *) walk->data;
    GstMultipartPadData *pad = (GstMultipartPadData *) data;

    walk = g_slist_next (walk);

    /* try to get a new buffer for this pad if needed and possible */
    if (pad->buffer == NULL) {
      GstBuffer *buf = NULL;

      buf = gst_collect_pads_pop (mux->collect, data);

      /* Store timestamp with segment_start and preroll */
      if (buf && GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
        pad->timestamp =
            gst_segment_to_running_time (&data->segment, GST_FORMAT_TIME,
            GST_BUFFER_TIMESTAMP (buf));
      } else {
        pad->timestamp = GST_CLOCK_TIME_NONE;
      }

      pad->buffer = buf;
    }

    /* we should have a buffer now, see if it is the best stream to
     * pull on */
    if (pad->buffer != NULL) {
      if (gst_multipart_mux_compare_pads (mux, bestpad, pad) > 0) {
        bestpad = pad;
      }
    }
  }

  return bestpad;
}

/* basic idea:
 *
 * 1) find a pad to pull on, this is done by pulling on all pads and
 *    looking at the buffers to decide which one should be muxed first.
 * 2) create a new buffer for the header
 * 3) push both buffers on best pad, go to 1
 */
static GstFlowReturn
gst_multipart_mux_collected (GstCollectPads * pads, GstMultipartMux * mux)
{
  GstMultipartPadData *best;
  GstFlowReturn ret = GST_FLOW_OK;
  gchar *header = NULL;
  size_t headerlen;
  GstBuffer *headerbuf = NULL;
  GstBuffer *footerbuf = NULL;
  GstBuffer *databuf = NULL;
  GstStructure *structure = NULL;
  const gchar *mime;

  GST_DEBUG_OBJECT (mux, "all pads are collected");

  /* queue buffers on all pads; find a buffer with the lowest timestamp */
  best = gst_multipart_mux_queue_pads (mux);
  if (!best)
    /* EOS */
    goto eos;
  else if (!best->buffer)
    goto buffer_error;

  /* If not negotiated yet set caps on src pad */
  if (!mux->negotiated) {
    GstCaps *newcaps;

    newcaps = gst_caps_new_simple ("multipart/x-mixed-replace",
        "boundary", G_TYPE_STRING, mux->boundary, NULL);

    if (!gst_pad_set_caps (mux->srcpad, newcaps)) {
      gst_caps_unref (newcaps);
      goto nego_error;
    }

    gst_caps_unref (newcaps);
    mux->negotiated = TRUE;
  }

  /* see if we need to push a segment */
  if (mux->need_segment) {
    GstEvent *event;
    GstClockTime time;

    if (best->timestamp != -1)
      time = best->timestamp;
    else
      time = 0;

    /* for the segment, we take the first timestamp we see, we don't know the
     * length and the position is 0 */
    event = gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_TIME,
        time, -1, 0);

    gst_pad_push_event (mux->srcpad, event);

    mux->need_segment = FALSE;
  }

  structure = gst_caps_get_structure (GST_BUFFER_CAPS (best->buffer), 0);
  if (!structure)
    goto no_caps;

  /* get the mime type for the structure */
  mime = gst_multipart_mux_get_mime (mux, structure);

  header = g_strdup_printf ("--%s\r\nContent-Type: %s\r\n"
      "Content-Length: %u\r\n\r\n",
      mux->boundary, mime, GST_BUFFER_SIZE (best->buffer));
  headerlen = strlen (header);

  ret = gst_pad_alloc_buffer_and_set_caps (mux->srcpad, GST_BUFFER_OFFSET_NONE,
      headerlen, GST_PAD_CAPS (mux->srcpad), &headerbuf);
  if (ret != GST_FLOW_OK)
    goto alloc_failed;

  memcpy (GST_BUFFER_DATA (headerbuf), header, headerlen);
  g_free (header);

  /* the header has the same timestamp as the data buffer (which we will push
   * below) and has a duration of 0 */
  GST_BUFFER_TIMESTAMP (headerbuf) = best->timestamp;
  GST_BUFFER_DURATION (headerbuf) = 0;
  GST_BUFFER_OFFSET (headerbuf) = mux->offset;
  mux->offset += headerlen;
  GST_BUFFER_OFFSET_END (headerbuf) = mux->offset;

  GST_DEBUG_OBJECT (mux, "pushing %" G_GSIZE_FORMAT " bytes header buffer",
      headerlen);
  ret = gst_pad_push (mux->srcpad, headerbuf);
  if (ret != GST_FLOW_OK)
    /* push always takes ownership of the buffer, even after an error, so we
     * don't need to unref headerbuf here. */
    goto beach;

  /* take best->buffer, we don't need to unref it later as we will push it
   * now. */
  databuf = gst_buffer_make_metadata_writable (best->buffer);
  best->buffer = NULL;

  gst_buffer_set_caps (databuf, GST_PAD_CAPS (mux->srcpad));
  /* we need to updated the timestamp to match the running_time */
  GST_BUFFER_TIMESTAMP (databuf) = best->timestamp;
  GST_BUFFER_OFFSET (databuf) = mux->offset;
  mux->offset += GST_BUFFER_SIZE (databuf);
  GST_BUFFER_OFFSET_END (databuf) = mux->offset;
  GST_BUFFER_FLAG_SET (databuf, GST_BUFFER_FLAG_DELTA_UNIT);

  GST_DEBUG_OBJECT (mux, "pushing %u bytes data buffer",
      GST_BUFFER_SIZE (databuf));
  ret = gst_pad_push (mux->srcpad, databuf);
  if (ret != GST_FLOW_OK)
    /* push always takes ownership of the buffer, even after an error, so we
     * don't need to unref headerbuf here. */
    goto beach;

  ret = gst_pad_alloc_buffer_and_set_caps (mux->srcpad, GST_BUFFER_OFFSET_NONE,
      2, GST_PAD_CAPS (mux->srcpad), &footerbuf);
  if (ret != GST_FLOW_OK)
    goto alloc_failed;

  memcpy (GST_BUFFER_DATA (footerbuf), "\r\n", 2);

  /* the footer has the same timestamp as the data buffer and has a
   * duration of 0 */
  GST_BUFFER_TIMESTAMP (footerbuf) = best->timestamp;
  GST_BUFFER_DURATION (footerbuf) = 0;
  GST_BUFFER_OFFSET (footerbuf) = mux->offset;
  mux->offset += 2;
  GST_BUFFER_OFFSET_END (footerbuf) = mux->offset;
  GST_BUFFER_FLAG_SET (footerbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  GST_DEBUG_OBJECT (mux, "pushing 2 bytes footer buffer");
  ret = gst_pad_push (mux->srcpad, footerbuf);

beach:
  if (best && best->buffer) {
    gst_buffer_unref (best->buffer);
    best->buffer = NULL;
  }
  return ret;

  /* ERRORS */
buffer_error:
  {
    /* There is a best but no buffer, this is not quite right.. */
    GST_ELEMENT_ERROR (mux, STREAM, FAILED, (NULL), ("internal muxing error"));
    ret = GST_FLOW_ERROR;
    goto beach;
  }
eos:
  {
    GST_DEBUG_OBJECT (mux, "Pushing EOS");
    gst_pad_push_event (mux->srcpad, gst_event_new_eos ());
    ret = GST_FLOW_UNEXPECTED;
    goto beach;
  }
nego_error:
  {
    GST_WARNING_OBJECT (mux, "failed to set caps");
    GST_ELEMENT_ERROR (mux, CORE, NEGOTIATION, (NULL), (NULL));
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto beach;
  }
no_caps:
  {
    GST_WARNING_OBJECT (mux, "no caps on the incoming buffer %p", best->buffer);
    GST_ELEMENT_ERROR (mux, CORE, NEGOTIATION, (NULL), (NULL));
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto beach;
  }
alloc_failed:
  {
    GST_WARNING_OBJECT (mux,
        "failed allocating a %" G_GSIZE_FORMAT " bytes buffer", headerlen);
    g_free (header);
    goto beach;
  }
}

static void
gst_multipart_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstMultipartMux *mux;

  mux = GST_MULTIPART_MUX (object);

  switch (prop_id) {
    case ARG_BOUNDARY:
      g_value_set_string (value, mux->boundary);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_multipart_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstMultipartMux *mux;

  mux = GST_MULTIPART_MUX (object);

  switch (prop_id) {
    case ARG_BOUNDARY:
      g_free (mux->boundary);
      mux->boundary = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_multipart_mux_change_state (GstElement * element, GstStateChange transition)
{
  GstMultipartMux *multipart_mux;
  GstStateChangeReturn ret;

  multipart_mux = GST_MULTIPART_MUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      multipart_mux->offset = 0;
      multipart_mux->negotiated = FALSE;
      multipart_mux->need_segment = TRUE;
      GST_DEBUG_OBJECT (multipart_mux, "starting collect pads");
      gst_collect_pads_start (multipart_mux->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG_OBJECT (multipart_mux, "stopping collect pads");
      gst_collect_pads_stop (multipart_mux->collect);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    default:
      break;
  }

  return ret;
}

gboolean
gst_multipart_mux_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_multipart_mux_debug, "multipartmux", 0,
      "multipart muxer");

  return gst_element_register (plugin, "multipartmux", GST_RANK_NONE,
      GST_TYPE_MULTIPART_MUX);
}
