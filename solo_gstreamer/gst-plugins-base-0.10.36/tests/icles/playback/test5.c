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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>             /* exit */
#endif
#include <gst/gst.h>

static GMainLoop *loop;

static void
new_pad (GstElement * element, GstPad * pad, gboolean last, GstElement * sink)
{
  g_print ("New pad...\n");
}

static void
no_more_pads (GstElement * element)
{
  g_print ("No more pads...\n");
  g_main_loop_quit (loop);
}

static gboolean
start_finding (GstElement * pipeline)
{
  GstStateChangeReturn res;

  g_print ("finding caps...\n");
  res = gst_element_set_state (pipeline, GST_STATE_PAUSED);
  if (res == GST_STATE_CHANGE_FAILURE) {
    g_print ("could not pause\n");
    exit (-1);
  }
  return FALSE;
}

static void
dump_element_stats (GstElement * element)
{
  GstIterator *it;
  gpointer data;

  it = gst_element_iterate_src_pads (element);
  while (gst_iterator_next (it, &data) == GST_ITERATOR_OK) {
    GstPad *pad = GST_PAD (data);
    GstCaps *caps;
    gchar *str;
    GstQuery *query;

    g_print ("stream %s:\n", GST_OBJECT_NAME (pad));

    caps = gst_pad_get_caps (pad);
    str = gst_caps_to_string (caps);
    g_print (" caps: %s\n", str);
    g_free (str);
    gst_caps_unref (caps);

    query = gst_query_new_duration (GST_FORMAT_TIME);
    if (gst_pad_query (pad, query)) {
      gint64 duration;

      gst_query_parse_duration (query, NULL, &duration);

      g_print (" duration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (duration));
    }
    gst_query_unref (query);

    gst_object_unref (pad);
  }
  gst_iterator_free (it);
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *pipeline, *filesrc, *decodebin;

  gst_init (&argc, &argv);

  pipeline = gst_pipeline_new ("pipeline");

  filesrc = gst_element_factory_make ("filesrc", "filesrc");
  g_assert (filesrc);

  decodebin = gst_element_factory_make ("decodebin", "decodebin");
  g_assert (decodebin);

  g_signal_connect (G_OBJECT (decodebin), "new-decoded-pad",
      G_CALLBACK (new_pad), NULL);
  g_signal_connect (G_OBJECT (decodebin), "no-more-pads",
      G_CALLBACK (no_more_pads), NULL);

  gst_bin_add_many (GST_BIN (pipeline), filesrc, decodebin, NULL);
  gst_element_link (filesrc, decodebin);

  if (argc < 2) {
    g_print ("usage: %s <uri>\n", argv[0]);
    exit (-1);
  }
  g_object_set (G_OBJECT (filesrc), "location", argv[1], NULL);

  /* event based programming approach */
  loop = g_main_loop_new (NULL, TRUE);
  g_idle_add ((GSourceFunc) start_finding, pipeline);
  g_main_loop_run (loop);

  dump_element_stats (decodebin);

  return 0;
}
