/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright 2005 Jan Schmidt <thaytan@mad.scientist.com>
 *           2006 Michael Smith <msmith@fluendo.com>
 * Copyright (C) 2003-2004 Benjamin Otte <otte@gnome.org>
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
 * SECTION:element-icydemux
 *
 * icydemux accepts data streams with ICY metadata at known intervals, as
 * transmitted from an upstream element (usually read as response headers from
 * an HTTP stream). The mime type of the data between the tag blocks is
 * detected using typefind functions, and the appropriate output mime type set
 * on outgoing buffers. 
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch souphttpsrc location=http://some.server/ iradio-mode=true ! icydemux ! fakesink -t
 * ]| This pipeline should read any available ICY tag information and output it.
 * The contents of the stream should be detected, and the appropriate mime
 * type set on buffers produced from icydemux. (Using gnomevfssrc, neonhttpsrc
 * or giosrc instead of souphttpsrc should also work.)
 * </refsect2>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/gst.h>
#include <gst/gst-i18n-plugin.h>
#include <gst/tag/tag.h>

#include "gsticydemux.h"

#include <string.h>

#define ICY_TYPE_FIND_MAX_SIZE (40*1024)

GST_DEBUG_CATEGORY_STATIC (icydemux_debug);
#define GST_CAT_DEFAULT (icydemux_debug)

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-icy, metadata-interval = (int)[0, MAX]")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("ANY")
    );

static void gst_icydemux_class_init (GstICYDemuxClass * klass);
static void gst_icydemux_base_init (GstICYDemuxClass * klass);
static void gst_icydemux_init (GstICYDemux * icydemux);
static void gst_icydemux_dispose (GObject * object);

static GstFlowReturn gst_icydemux_chain (GstPad * pad, GstBuffer * buf);
static gboolean gst_icydemux_handle_event (GstPad * pad, GstEvent * event);

static gboolean gst_icydemux_add_srcpad (GstICYDemux * icydemux,
    GstCaps * new_caps);
static gboolean gst_icydemux_remove_srcpad (GstICYDemux * icydemux);

static GstStateChangeReturn gst_icydemux_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_icydemux_sink_setcaps (GstPad * pad, GstCaps * caps);

static gboolean gst_icydemux_send_tag_event (GstICYDemux * icydemux,
    GstTagList * taglist);

static GstElementClass *parent_class = NULL;

GType
gst_icydemux_get_type (void)
{
  static GType plugin_type = 0;

  if (!plugin_type) {
    static const GTypeInfo plugin_info = {
      sizeof (GstICYDemuxClass),
      (GBaseInitFunc) gst_icydemux_base_init,
      NULL,
      (GClassInitFunc) gst_icydemux_class_init,
      NULL,
      NULL,
      sizeof (GstICYDemux),
      0,
      (GInstanceInitFunc) gst_icydemux_init,
    };
    plugin_type = g_type_register_static (GST_TYPE_ELEMENT,
        "GstICYDemux", &plugin_info, 0);
  }
  return plugin_type;
}

static void
gst_icydemux_base_init (GstICYDemuxClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class, &src_factory);
  gst_element_class_add_static_pad_template (element_class, &sink_factory);
  gst_element_class_set_details_simple (element_class, "ICY tag demuxer",
      "Codec/Demuxer/Metadata",
      "Read and output ICY tags while demuxing the contents",
      "Jan Schmidt <thaytan@mad.scientist.com>, "
      "Michael Smith <msmith@fluendo.com>");
}

static void
gst_icydemux_class_init (GstICYDemuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

  gobject_class->dispose = gst_icydemux_dispose;

  gstelement_class->change_state = gst_icydemux_change_state;

}

static void
gst_icydemux_reset (GstICYDemux * icydemux)
{
  /* Unknown at the moment (this is a fatal error if don't have a value by the
   * time we get to our chain function)
   */
  icydemux->meta_interval = -1;
  icydemux->remaining = 0;

  icydemux->typefinding = TRUE;

  gst_caps_replace (&(icydemux->src_caps), NULL);

  gst_icydemux_remove_srcpad (icydemux);

  if (icydemux->cached_tags) {
    gst_tag_list_free (icydemux->cached_tags);
    icydemux->cached_tags = NULL;
  }

  if (icydemux->cached_events) {
    g_list_foreach (icydemux->cached_events,
        (GFunc) gst_mini_object_unref, NULL);
    g_list_free (icydemux->cached_events);
    icydemux->cached_events = NULL;
  }

  if (icydemux->meta_adapter) {
    gst_adapter_clear (icydemux->meta_adapter);
    g_object_unref (icydemux->meta_adapter);
    icydemux->meta_adapter = NULL;
  }

  if (icydemux->typefind_buf) {
    gst_buffer_unref (icydemux->typefind_buf);
    icydemux->typefind_buf = NULL;
  }

  if (icydemux->content_type) {
    g_free (icydemux->content_type);
    icydemux->content_type = NULL;
  }
}

static void
gst_icydemux_init (GstICYDemux * icydemux)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (icydemux);

  icydemux->sinkpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  gst_pad_set_chain_function (icydemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_icydemux_chain));
  gst_pad_set_event_function (icydemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_icydemux_handle_event));
  gst_pad_set_setcaps_function (icydemux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_icydemux_sink_setcaps));
  gst_element_add_pad (GST_ELEMENT (icydemux), icydemux->sinkpad);

  gst_icydemux_reset (icydemux);
}

static gboolean
gst_icydemux_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstICYDemux *icydemux = GST_ICYDEMUX (GST_PAD_PARENT (pad));
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  const gchar *tmp;

  if (!gst_structure_get_int (structure, "metadata-interval",
          &icydemux->meta_interval))
    return FALSE;

  /* If incoming caps have the HTTP Content-Type, copy that over */
  if ((tmp = gst_structure_get_string (structure, "content-type")))
    icydemux->content_type = g_strdup (tmp);

  /* We have a meta interval, so initialise the rest */
  icydemux->remaining = icydemux->meta_interval;
  icydemux->meta_remaining = 0;
  return TRUE;
}

static void
gst_icydemux_dispose (GObject * object)
{
  GstICYDemux *icydemux = GST_ICYDEMUX (object);

  gst_icydemux_reset (icydemux);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gst_icydemux_add_srcpad (GstICYDemux * icydemux, GstCaps * new_caps)
{
  if (icydemux->src_caps == NULL ||
      !gst_caps_is_equal (new_caps, icydemux->src_caps)) {
    gst_caps_replace (&(icydemux->src_caps), new_caps);
    if (icydemux->srcpad != NULL) {
      GST_DEBUG_OBJECT (icydemux, "Changing src pad caps to %" GST_PTR_FORMAT,
          icydemux->src_caps);

      gst_pad_set_caps (icydemux->srcpad, icydemux->src_caps);
    }
  } else {
    /* Caps never changed */
    gst_caps_unref (new_caps);
  }

  if (icydemux->srcpad == NULL) {
    icydemux->srcpad =
        gst_pad_new_from_template (gst_element_class_get_pad_template
        (GST_ELEMENT_GET_CLASS (icydemux), "src"), "src");
    g_return_val_if_fail (icydemux->srcpad != NULL, FALSE);

    gst_pad_use_fixed_caps (icydemux->srcpad);

    if (icydemux->src_caps)
      gst_pad_set_caps (icydemux->srcpad, icydemux->src_caps);

    GST_DEBUG_OBJECT (icydemux, "Adding src pad with caps %" GST_PTR_FORMAT,
        icydemux->src_caps);

    gst_pad_set_active (icydemux->srcpad, TRUE);
    if (!(gst_element_add_pad (GST_ELEMENT (icydemux), icydemux->srcpad)))
      return FALSE;
    gst_element_no_more_pads (GST_ELEMENT (icydemux));
  }

  return TRUE;
}

static gboolean
gst_icydemux_remove_srcpad (GstICYDemux * icydemux)
{
  gboolean res = TRUE;

  if (icydemux->srcpad != NULL) {
    res = gst_element_remove_pad (GST_ELEMENT (icydemux), icydemux->srcpad);
    g_return_val_if_fail (res != FALSE, FALSE);
    icydemux->srcpad = NULL;
  }

  return res;
};

static gchar *
gst_icydemux_unicodify (const gchar * str)
{
  const gchar *env_vars[] = { "GST_ICY_TAG_ENCODING",
    "GST_TAG_ENCODING", NULL
  };

  return gst_tag_freeform_string_to_utf8 (str, -1, env_vars);
}

/* takes ownership of tag list */
static gboolean
gst_icydemux_tag_found (GstICYDemux * icydemux, GstTagList * tags)
{
  /* send the tag event if we have finished typefinding and have a src pad */
  if (icydemux->srcpad)
    return gst_icydemux_send_tag_event (icydemux, tags);

  /* if we haven't a source pad yet, cache the tags */
  if (!icydemux->cached_tags) {
    icydemux->cached_tags = tags;
  } else {
    gst_tag_list_insert (icydemux->cached_tags, tags,
        GST_TAG_MERGE_REPLACE_ALL);
    gst_tag_list_free (tags);
  }

  return TRUE;
}

static void
gst_icydemux_parse_and_send_tags (GstICYDemux * icydemux)
{
  GstTagList *tags;
  const guint8 *data;
  int length, i;
  gchar *buffer;
  gchar **strings;

  length = gst_adapter_available (icydemux->meta_adapter);

  data = gst_adapter_peek (icydemux->meta_adapter, length);

  /* Now, copy this to a buffer where we can NULL-terminate it to make things
   * a bit easier, then do that parsing. */
  buffer = g_strndup ((const gchar *) data, length);

  tags = gst_tag_list_new ();
  strings = g_strsplit (buffer, "';", 0);

  for (i = 0; strings[i]; i++) {
    if (!g_ascii_strncasecmp (strings[i], "StreamTitle=", 12)) {
      char *title = gst_icydemux_unicodify (strings[i] + 13);

      if (title && *title) {
        gst_tag_list_add (tags, GST_TAG_MERGE_REPLACE, GST_TAG_TITLE,
            title, NULL);
        g_free (title);
      }
    } else if (!g_ascii_strncasecmp (strings[i], "StreamUrl=", 10)) {
      char *url = gst_icydemux_unicodify (strings[i] + 11);

      if (url && *url) {
        gst_tag_list_add (tags, GST_TAG_MERGE_REPLACE, GST_TAG_HOMEPAGE,
            url, NULL);
        g_free (url);
      }
    }
  }

  g_strfreev (strings);
  g_free (buffer);
  gst_adapter_clear (icydemux->meta_adapter);

  if (!gst_tag_list_is_empty (tags))
    gst_icydemux_tag_found (icydemux, tags);
  else
    gst_tag_list_free (tags);
}

static gboolean
gst_icydemux_handle_event (GstPad * pad, GstEvent * event)
{
  GstICYDemux *icydemux = GST_ICYDEMUX (GST_PAD_PARENT (pad));
  gboolean result;

  if (GST_EVENT_TYPE (event) == GST_EVENT_TAG) {
    GstTagList *tags;

    gst_event_parse_tag (event, &tags);
    result = gst_icydemux_tag_found (icydemux, gst_tag_list_copy (tags));
    gst_event_unref (event);
    return result;
  }

  if (icydemux->typefinding) {
    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_FLUSH_STOP:
        g_list_foreach (icydemux->cached_events,
            (GFunc) gst_mini_object_unref, NULL);
        g_list_free (icydemux->cached_events);
        icydemux->cached_events = NULL;

        return gst_pad_event_default (pad, event);
      default:
        icydemux->cached_events = g_list_append (icydemux->cached_events,
            event);
        return TRUE;
    }
  } else {
    return gst_pad_event_default (pad, event);
  }
}

static void
gst_icydemux_send_cached_events (GstICYDemux * icydemux)
{
  GList *l;

  for (l = icydemux->cached_events; l != NULL; l = l->next) {
    GstEvent *event = GST_EVENT (l->data);

    gst_pad_push_event (icydemux->srcpad, event);
  }
  g_list_free (icydemux->cached_events);
  icydemux->cached_events = NULL;
}

static GstFlowReturn
gst_icydemux_typefind_or_forward (GstICYDemux * icydemux, GstBuffer * buf)
{
  if (icydemux->typefinding) {
    GstBuffer *tf_buf;
    GstCaps *caps = NULL;
    GstTypeFindProbability prob;

    /* If we have a content-type from upstream, let's see if we can shortcut
     * typefinding */
    if (G_UNLIKELY (icydemux->content_type)) {
      if (!g_ascii_strcasecmp (icydemux->content_type, "video/nsv")) {
        GST_DEBUG ("We have a NSV stream");
        caps = gst_caps_new_simple ("video/x-nsv", NULL);
      } else {
        GST_DEBUG ("Upstream Content-Type isn't supported");
        g_free (icydemux->content_type);
        icydemux->content_type = NULL;
      }
    }

    if (icydemux->typefind_buf) {
      icydemux->typefind_buf = gst_buffer_join (icydemux->typefind_buf, buf);
    } else {
      icydemux->typefind_buf = buf;
    }

    /* Only typefind if we haven't already got some caps */
    if (caps == NULL) {
      caps = gst_type_find_helper_for_buffer (GST_OBJECT (icydemux),
          icydemux->typefind_buf, &prob);

      if (caps == NULL) {
        if (GST_BUFFER_SIZE (icydemux->typefind_buf) < ICY_TYPE_FIND_MAX_SIZE) {
          /* Just break for more data */
          return GST_FLOW_OK;
        }

        /* We failed typefind */
        GST_ELEMENT_ERROR (icydemux, STREAM, TYPE_NOT_FOUND, (NULL),
            ("No caps found for contents within an ICY stream"));
        gst_buffer_unref (icydemux->typefind_buf);
        icydemux->typefind_buf = NULL;
        return GST_FLOW_ERROR;
      }
    }

    if (!gst_icydemux_add_srcpad (icydemux, caps)) {
      GST_DEBUG_OBJECT (icydemux, "Failed to add srcpad");
      gst_caps_unref (caps);
      gst_buffer_unref (icydemux->typefind_buf);
      icydemux->typefind_buf = NULL;
      return GST_FLOW_ERROR;
    }
    gst_caps_unref (caps);

    if (icydemux->cached_events) {
      gst_icydemux_send_cached_events (icydemux);
    }

    if (icydemux->cached_tags) {
      gst_icydemux_send_tag_event (icydemux, icydemux->cached_tags);
      icydemux->cached_tags = NULL;
    }

    /* Move onto streaming: call ourselves recursively with the typefind buffer
     * to get that forwarded. */
    icydemux->typefinding = FALSE;

    tf_buf = icydemux->typefind_buf;
    icydemux->typefind_buf = NULL;
    return gst_icydemux_typefind_or_forward (icydemux, tf_buf);
  } else {
    if (G_UNLIKELY (icydemux->srcpad == NULL)) {
      gst_buffer_unref (buf);
      return GST_FLOW_ERROR;
    }

    buf = gst_buffer_make_metadata_writable (buf);
    gst_buffer_set_caps (buf, icydemux->src_caps);

    /* Most things don't care, and it's a pain to track (we should preserve a
     * 0 offset on the first buffer though if it's there, for id3demux etc.) */
    if (GST_BUFFER_OFFSET (buf) != 0) {
      GST_BUFFER_OFFSET (buf) = GST_BUFFER_OFFSET_NONE;
    }

    return gst_pad_push (icydemux->srcpad, buf);
  }
}

static void
gst_icydemux_add_meta (GstICYDemux * icydemux, GstBuffer * buf)
{
  if (!icydemux->meta_adapter)
    icydemux->meta_adapter = gst_adapter_new ();

  gst_adapter_push (icydemux->meta_adapter, buf);
}

static GstFlowReturn
gst_icydemux_chain (GstPad * pad, GstBuffer * buf)
{
  GstICYDemux *icydemux;
  guint size, chunk, offset;
  GstBuffer *sub;
  GstFlowReturn ret = GST_FLOW_OK;

  icydemux = GST_ICYDEMUX (GST_PAD_PARENT (pad));

  if (G_UNLIKELY (icydemux->meta_interval < 0))
    goto not_negotiated;

  if (icydemux->meta_interval == 0) {
    ret = gst_icydemux_typefind_or_forward (icydemux, buf);
    goto done;
  }

  /* Go through the buffer, chopping it into appropriate chunks. Forward as
   * tags or buffers, as appropriate
   */
  size = GST_BUFFER_SIZE (buf);
  offset = 0;
  while (size) {
    if (icydemux->remaining) {
      chunk = (size <= icydemux->remaining) ? size : icydemux->remaining;
      sub = gst_buffer_create_sub (buf, offset, chunk);
      offset += chunk;
      icydemux->remaining -= chunk;
      size -= chunk;

      /* This buffer goes onto typefinding, and/or directly pushed out */
      ret = gst_icydemux_typefind_or_forward (icydemux, sub);
      if (ret != GST_FLOW_OK)
        goto done;
    } else if (icydemux->meta_remaining) {
      chunk = (size <= icydemux->meta_remaining) ?
          size : icydemux->meta_remaining;
      sub = gst_buffer_create_sub (buf, offset, chunk);
      gst_icydemux_add_meta (icydemux, sub);

      offset += chunk;
      icydemux->meta_remaining -= chunk;
      size -= chunk;

      if (icydemux->meta_remaining == 0) {
        /* Parse tags from meta_adapter, send off as tag messages */
        GST_DEBUG_OBJECT (icydemux, "No remaining metadata, parsing for tags");
        gst_icydemux_parse_and_send_tags (icydemux);

        icydemux->remaining = icydemux->meta_interval;
      }
    } else {
      /* We need to read a single byte (always safe at this point in the loop)
       * to figure out how many bytes of metadata exist. 
       * The 'spec' tells us to read 16 * (byte_value) bytes of metadata after
       * this (zero is common, and means the metadata hasn't changed).
       */
      icydemux->meta_remaining = 16 * GST_BUFFER_DATA (buf)[offset];
      if (icydemux->meta_remaining == 0)
        icydemux->remaining = icydemux->meta_interval;

      offset += 1;
      size -= 1;
    }
  }

done:
  gst_buffer_unref (buf);

  return ret;

  /* ERRORS */
not_negotiated:
  {
    GST_WARNING_OBJECT (icydemux, "meta_interval not set, buffer probably had "
        "no caps set. Try enabling iradio-mode on the http source element");
    gst_buffer_unref (buf);
    return GST_FLOW_NOT_NEGOTIATED;
  }
}

static GstStateChangeReturn
gst_icydemux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstICYDemux *icydemux = GST_ICYDEMUX (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_icydemux_reset (icydemux);
      break;
    default:
      break;
  }
  return ret;
}

/* takes ownership of tag list */
static gboolean
gst_icydemux_send_tag_event (GstICYDemux * icydemux, GstTagList * tags)
{
  GstEvent *event;

  gst_element_post_message (GST_ELEMENT (icydemux),
      gst_message_new_tag (GST_OBJECT (icydemux), gst_tag_list_copy (tags)));

  event = gst_event_new_tag (tags);
  GST_EVENT_TIMESTAMP (event) = 0;

  GST_DEBUG_OBJECT (icydemux, "Sending tag event on src pad");
  return gst_pad_push_event (icydemux->srcpad, event);

}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (icydemux_debug, "icydemux", 0,
      "GStreamer ICY tag demuxer");

  return gst_element_register (plugin, "icydemux",
      GST_RANK_PRIMARY, GST_TYPE_ICYDEMUX);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "icydemux",
    "Demux ICY tags from a stream",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
