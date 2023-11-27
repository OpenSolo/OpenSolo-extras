/* GStreamer
 *
 * unit test for audiotestsrc basetime handling
 *
 * Copyright (C) 2009 Maemo Multimedia <multimedia at maemo dot org>
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

#include <gst/check/gstcheck.h>

#ifndef GST_DISABLE_PARSE

static GstClockTime old_ts = GST_CLOCK_TIME_NONE;

static gboolean
break_mainloop (gpointer data)
{
  GMainLoop *loop;

  loop = (GMainLoop *) data;
  g_main_loop_quit (loop);

  return FALSE;
}

static gboolean
buffer_probe_cb (GstPad * pad, GstBuffer * buffer)
{
  GstClockTime new_ts = GST_BUFFER_TIMESTAMP (buffer);

  GST_LOG ("ts = %" GST_TIME_FORMAT, GST_TIME_ARGS (new_ts));
  if (old_ts != GST_CLOCK_TIME_NONE) {
    fail_unless (new_ts != old_ts,
        "Two buffers had same timestamp: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (old_ts));
  }
  old_ts = new_ts;

  return TRUE;
}

GST_START_TEST (test_basetime_calculation)
{
  GstElement *p1, *bin;
  GstElement *asrc, *asink;
  GstPad *pad;
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  /* The "main" pipeline */
  p1 = gst_parse_launch ("fakesrc ! fakesink", NULL);
  fail_if (p1 == NULL);

  /* Create a sub-bin that is activated only in "certain situations" */
  asrc = gst_element_factory_make ("audiotestsrc", NULL);
  if (asrc == NULL) {
    GST_WARNING ("Cannot run test. 'audiotestsrc' not available");
    gst_element_set_state (p1, GST_STATE_NULL);
    gst_object_unref (p1);
    return;
  }
  asink = gst_element_factory_make ("fakesink", NULL);

  bin = gst_bin_new ("audiobin");
  gst_bin_add_many (GST_BIN (bin), asrc, asink, NULL);
  gst_element_link (asrc, asink);

  gst_bin_add (GST_BIN (p1), bin);
  gst_element_set_state (p1, GST_STATE_READY);

  pad = gst_element_get_static_pad (asink, "sink");
  fail_unless (pad != NULL, "Could not get pad out of sink");

  gst_pad_add_buffer_probe (pad, G_CALLBACK (buffer_probe_cb), NULL);
  gst_element_set_locked_state (bin, TRUE);

  /* Run main pipeline first */
  gst_element_set_state (p1, GST_STATE_PLAYING);
  g_timeout_add (2 * 1000, break_mainloop, loop);
  g_main_loop_run (loop);

  /* Now activate the audio pipeline */
  gst_element_set_locked_state (bin, FALSE);
  gst_element_set_state (p1, GST_STATE_PAUSED);

  /* Normally our custom audiobin would send this message */
  gst_element_post_message (asrc,
      gst_message_new_clock_provide (GST_OBJECT (asrc), NULL, TRUE));

  /* At this point a new clock is selected */
  gst_element_set_state (p1, GST_STATE_PLAYING);

  g_timeout_add (2 * 1000, break_mainloop, loop);
  g_main_loop_run (loop);

  gst_object_unref (pad);
  gst_element_set_state (p1, GST_STATE_NULL);
  gst_object_unref (p1);

  g_main_loop_unref (loop);
}

GST_END_TEST;

#endif /* #ifndef GST_DISABLE_PARSE */

static Suite *
baseaudiosrc_suite (void)
{
  Suite *s = suite_create ("baseaudiosrc");
  TCase *tc_chain = tcase_create ("general");

  /* timeout 6 sec */
  tcase_set_timeout (tc_chain, 6);
  suite_add_tcase (s, tc_chain);

#ifndef GST_DISABLE_PARSE
  tcase_add_test (tc_chain, test_basetime_calculation);
#endif

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = baseaudiosrc_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
