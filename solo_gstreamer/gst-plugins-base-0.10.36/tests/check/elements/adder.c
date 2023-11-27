/* GStreamer
 *
 * unit test for adder
 *
 * Copyright (C) <2005> Thomas Vander Stichele <thomas at apestaart dot org>
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
# include <config.h>
#endif

#ifdef HAVE_VALGRIND
# include <valgrind/valgrind.h>
#endif

#include <unistd.h>

#include <gst/check/gstcheck.h>
#include <gst/check/gstconsistencychecker.h>

static GMainLoop *main_loop;

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * bin)
{
  GST_INFO ("bus message from \"%" GST_PTR_FORMAT "\": %" GST_PTR_FORMAT,
      GST_MESSAGE_SRC (message), message);

  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (main_loop);
      break;
    case GST_MESSAGE_WARNING:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_warning (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_error (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (main_loop);
      break;
    }
    default:
      break;
  }
}


static GstFormat format = GST_FORMAT_UNDEFINED;
static gint64 position = -1;

static void
test_event_message_received (GstBus * bus, GstMessage * message,
    GstPipeline * bin)
{
  GST_INFO ("bus message from \"%" GST_PTR_FORMAT "\": %" GST_PTR_FORMAT,
      GST_MESSAGE_SRC (message), message);

  switch (message->type) {
    case GST_MESSAGE_SEGMENT_DONE:
      gst_message_parse_segment_done (message, &format, &position);
      GST_INFO ("received segment_done : %" G_GINT64_FORMAT, position);
      g_main_loop_quit (main_loop);
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}


GST_START_TEST (test_event)
{
  GstElement *bin, *src1, *src2, *adder, *sink;
  GstBus *bus;
  GstEvent *seek_event;
  gboolean res;
  GstPad *srcpad;
  GstStreamConsistency *consist;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  /* FIXME, fakesrc with default setting will produce 0 sized
   * buffers and incompatible caps for adder that will make
   * adder EOS and error out */
  src1 = gst_element_factory_make ("audiotestsrc", "src1");
  g_object_set (src1, "wave", 4, NULL); /* silence */
  src2 = gst_element_factory_make ("audiotestsrc", "src2");
  g_object_set (src2, "wave", 4, NULL); /* silence */
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src1, src2, adder, sink, NULL);

  res = gst_element_link (src1, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (src2, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  srcpad = gst_element_get_static_pad (adder, "src");
  consist = gst_consistency_checker_new (srcpad);
  gst_object_unref (srcpad);

  seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) 2 * GST_SECOND);

  format = GST_FORMAT_UNDEFINED;
  position = -1;

  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::segment-done",
      (GCallback) test_event_message_received, bin);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  GST_INFO ("starting test");

  /* prepare playing */
  res = gst_element_set_state (bin, GST_STATE_PAUSED);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* wait for completion */
  res = gst_element_get_state (bin, NULL, NULL, GST_CLOCK_TIME_NONE);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  res = gst_element_send_event (bin, seek_event);
  fail_unless (res == TRUE, NULL);

  /* run pipeline */
  res = gst_element_set_state (bin, GST_STATE_PLAYING);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  g_main_loop_run (main_loop);

  res = gst_element_set_state (bin, GST_STATE_NULL);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  fail_unless (position == 2 * GST_SECOND, NULL);

  /* cleanup */
  g_main_loop_unref (main_loop);
  gst_consistency_checker_free (consist);
  gst_object_unref (G_OBJECT (bus));
  gst_object_unref (G_OBJECT (bin));
}

GST_END_TEST;

static guint play_count = 0;
static GstEvent *play_seek_event = NULL;

static void
test_play_twice_message_received (GstBus * bus, GstMessage * message,
    GstPipeline * bin)
{
  gboolean res;

  GST_INFO ("bus message from \"%" GST_PTR_FORMAT "\": %" GST_PTR_FORMAT,
      GST_MESSAGE_SRC (message), message);

  switch (message->type) {
    case GST_MESSAGE_SEGMENT_DONE:
      play_count++;
      if (play_count == 1) {
        res = gst_element_set_state (GST_ELEMENT (bin), GST_STATE_READY);
        fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

        /* prepare playing again */
        res = gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PAUSED);
        fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

        /* wait for completion */
        res =
            gst_element_get_state (GST_ELEMENT (bin), NULL, NULL,
            GST_CLOCK_TIME_NONE);
        fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

        res = gst_element_send_event (GST_ELEMENT (bin),
            gst_event_ref (play_seek_event));
        fail_unless (res == TRUE, NULL);

        res = gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PLAYING);
        fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);
      } else {
        g_main_loop_quit (main_loop);
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}


GST_START_TEST (test_play_twice)
{
  GstElement *bin, *src1, *src2, *adder, *sink;
  GstBus *bus;
  gboolean res;
  GstPad *srcpad;
  GstStreamConsistency *consist;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src1 = gst_element_factory_make ("audiotestsrc", "src1");
  g_object_set (src1, "wave", 4, NULL); /* silence */
  src2 = gst_element_factory_make ("audiotestsrc", "src2");
  g_object_set (src2, "wave", 4, NULL); /* silence */
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src1, src2, adder, sink, NULL);

  res = gst_element_link (src1, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (src2, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  srcpad = gst_element_get_static_pad (adder, "src");
  consist = gst_consistency_checker_new (srcpad);
  gst_object_unref (srcpad);

  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) 2 * GST_SECOND);

  play_count = 0;

  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::segment-done",
      (GCallback) test_play_twice_message_received, bin);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  GST_INFO ("starting test");

  /* prepare playing */
  res = gst_element_set_state (bin, GST_STATE_PAUSED);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* wait for completion */
  res =
      gst_element_get_state (GST_ELEMENT (bin), NULL, NULL,
      GST_CLOCK_TIME_NONE);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  res = gst_element_send_event (bin, gst_event_ref (play_seek_event));
  fail_unless (res == TRUE, NULL);

  GST_INFO ("seeked");

  /* run pipeline */
  res = gst_element_set_state (bin, GST_STATE_PLAYING);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  g_main_loop_run (main_loop);

  res = gst_element_set_state (bin, GST_STATE_NULL);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  fail_unless (play_count == 2, NULL);

  /* cleanup */
  g_main_loop_unref (main_loop);
  gst_consistency_checker_free (consist);
  gst_event_ref (play_seek_event);
  gst_object_unref (G_OBJECT (bus));
  gst_object_unref (G_OBJECT (bin));
}

GST_END_TEST;

GST_START_TEST (test_play_twice_then_add_and_play_again)
{
  GstElement *bin, *src1, *src2, *src3, *adder, *sink;
  GstBus *bus;
  gboolean res;
  gint i;
  GstPad *srcpad;
  GstStreamConsistency *consist;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src1 = gst_element_factory_make ("audiotestsrc", "src1");
  g_object_set (src1, "wave", 4, NULL); /* silence */
  src2 = gst_element_factory_make ("audiotestsrc", "src2");
  g_object_set (src2, "wave", 4, NULL); /* silence */
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src1, src2, adder, sink, NULL);

  srcpad = gst_element_get_static_pad (adder, "src");
  consist = gst_consistency_checker_new (srcpad);
  gst_object_unref (srcpad);

  res = gst_element_link (src1, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (src2, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) 2 * GST_SECOND);

  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::segment-done",
      (GCallback) test_play_twice_message_received, bin);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  /* run it twice */
  for (i = 0; i < 2; i++) {
    play_count = 0;

    GST_INFO ("starting test-loop %d", i);

    /* prepare playing */
    res = gst_element_set_state (bin, GST_STATE_PAUSED);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    /* wait for completion */
    res =
        gst_element_get_state (GST_ELEMENT (bin), NULL, NULL,
        GST_CLOCK_TIME_NONE);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    res = gst_element_send_event (bin, gst_event_ref (play_seek_event));
    fail_unless (res == TRUE, NULL);

    GST_INFO ("seeked");

    /* run pipeline */
    res = gst_element_set_state (bin, GST_STATE_PLAYING);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    g_main_loop_run (main_loop);

    res = gst_element_set_state (bin, GST_STATE_READY);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    fail_unless (play_count == 2, NULL);

    /* plug another source */
    if (i == 0) {
      src3 = gst_element_factory_make ("audiotestsrc", "src3");
      g_object_set (src3, "wave", 4, NULL);     /* silence */
      gst_bin_add (GST_BIN (bin), src3);

      res = gst_element_link (src3, adder);
      fail_unless (res == TRUE, NULL);
    }

    gst_consistency_checker_reset (consist);
  }

  res = gst_element_set_state (bin, GST_STATE_NULL);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* cleanup */
  g_main_loop_unref (main_loop);
  gst_event_ref (play_seek_event);
  gst_consistency_checker_free (consist);
  gst_object_unref (G_OBJECT (bus));
  gst_object_unref (G_OBJECT (bin));
}

GST_END_TEST;


static void
test_live_seeking_eos_message_received (GstBus * bus, GstMessage * message,
    GstPipeline * bin)
{
  GST_INFO ("bus message from \"%" GST_PTR_FORMAT "\": %" GST_PTR_FORMAT,
      GST_MESSAGE_SRC (message), message);

  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (main_loop);
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}


/* test failing seeks on live-sources */
GST_START_TEST (test_live_seeking)
{
  GstElement *bin, *src1, *src2, *ac1, *ac2, *adder, *sink;
  GstBus *bus;
  gboolean res;
  GstPad *srcpad;
  gint i;
  GstStreamConsistency *consist;

  GST_INFO ("preparing test");
  main_loop = NULL;
  play_seek_event = NULL;

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  /* normal audiosources behave differently than audiotestsrc */
#if 0
  src1 = gst_element_factory_make ("audiotestsrc", "src1");
  g_object_set (src1, "wave", 4, "is-live", TRUE, NULL);        /* silence */
#else
  src1 = gst_element_factory_make ("alsasrc", "src1");
  if (!src1) {
    GST_INFO ("no audiosrc, skipping");
    goto cleanup;
  }
  /* Test that the audio source can get to paused, else skip */
  res = gst_element_set_state (src1, GST_STATE_PAUSED);
  (void) gst_element_set_state (src1, GST_STATE_NULL);
  gst_object_unref (src1);

  if (res == GST_STATE_CHANGE_FAILURE)
    goto cleanup;
  src1 = gst_element_factory_make ("alsasrc", "src1");

  /* live sources ignore seeks, force eos after 2 sec (4 buffers half second
   * each) - don't use autoaudiosrc, as then we can't set anything here */
  g_object_set (src1, "num-buffers", 4, "blocksize", 44100, NULL);
#endif
  ac1 = gst_element_factory_make ("audioconvert", "ac1");
  src2 = gst_element_factory_make ("audiotestsrc", "src2");
  g_object_set (src2, "wave", 4, NULL); /* silence */
  ac2 = gst_element_factory_make ("audioconvert", "ac2");
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src1, ac1, src2, ac2, adder, sink, NULL);

  res = gst_element_link (src1, ac1);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (ac1, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (src2, ac2);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (ac2, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) 2 * GST_SECOND);

  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos",
      (GCallback) test_live_seeking_eos_message_received, bin);

  srcpad = gst_element_get_static_pad (adder, "src");
  consist = gst_consistency_checker_new (srcpad);
  gst_object_unref (srcpad);

  GST_INFO ("starting test");

  /* run it twice */
  for (i = 0; i < 2; i++) {

    GST_INFO ("starting test-loop %d", i);

    /* prepare playing */
    res = gst_element_set_state (bin, GST_STATE_PAUSED);
    fail_unless (res != GST_STATE_CHANGE_FAILURE);

    /* wait for completion */
    res =
        gst_element_get_state (GST_ELEMENT (bin), NULL, NULL,
        GST_CLOCK_TIME_NONE);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    res = gst_element_send_event (bin, gst_event_ref (play_seek_event));
#if 1
    fail_unless (res == TRUE, NULL);
#else
    /* adder is picky, if a single seek fails it totally fails */
    fail_unless (res == FALSE, NULL);
#endif

    GST_INFO ("seeked");

    /* run pipeline */
    res = gst_element_set_state (bin, GST_STATE_PLAYING);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    GST_INFO ("playing");

    g_main_loop_run (main_loop);

    res = gst_element_set_state (bin, GST_STATE_NULL);
    fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

    gst_consistency_checker_reset (consist);
  }

  /* cleanup */
cleanup:
  GST_INFO ("cleaning up");
  if (main_loop)
    g_main_loop_unref (main_loop);
  if (play_seek_event)
    gst_event_unref (play_seek_event);
  gst_object_unref (G_OBJECT (bus));
  gst_object_unref (G_OBJECT (bin));
}

GST_END_TEST;

/* check if adding pads work as expected */
GST_START_TEST (test_add_pad)
{
  GstElement *bin, *src1, *src2, *adder, *sink;
  GstBus *bus;
  GstPad *srcpad;
  gboolean res;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src1 = gst_element_factory_make ("audiotestsrc", "src1");
  g_object_set (src1, "num-buffers", 4, NULL);
  g_object_set (src1, "wave", 4, NULL); /* silence */
  src2 = gst_element_factory_make ("audiotestsrc", "src2");
  /* one buffer less, we connect with 1 buffer of delay */
  g_object_set (src2, "num-buffers", 3, NULL);
  g_object_set (src2, "wave", 4, NULL); /* silence */
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src1, adder, sink, NULL);

  res = gst_element_link (src1, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  srcpad = gst_element_get_static_pad (adder, "src");
  gst_object_unref (srcpad);

  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::segment-done", (GCallback) message_received,
      bin);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  GST_INFO ("starting test");

  /* prepare playing */
  res = gst_element_set_state (bin, GST_STATE_PAUSED);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* wait for completion */
  res =
      gst_element_get_state (GST_ELEMENT (bin), NULL, NULL,
      GST_CLOCK_TIME_NONE);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* add other element */
  gst_bin_add_many (GST_BIN (bin), src2, NULL);

  /* now link the second element */
  res = gst_element_link (src2, adder);
  fail_unless (res == TRUE, NULL);

  /* set to PAUSED as well */
  res = gst_element_set_state (src2, GST_STATE_PAUSED);

  /* now play all */
  res = gst_element_set_state (bin, GST_STATE_PLAYING);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  g_main_loop_run (main_loop);

  res = gst_element_set_state (bin, GST_STATE_NULL);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* cleanup */
  g_main_loop_unref (main_loop);
  gst_object_unref (G_OBJECT (bus));
  gst_object_unref (G_OBJECT (bin));
}

GST_END_TEST;

/* check if removing pads work as expected */
GST_START_TEST (test_remove_pad)
{
  GstElement *bin, *src, *adder, *sink;
  GstBus *bus;
  GstPad *pad, *srcpad;
  gboolean res;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  src = gst_element_factory_make ("audiotestsrc", "src");
  g_object_set (src, "num-buffers", 4, NULL);
  g_object_set (src, "wave", 4, NULL);
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (bin), src, adder, sink, NULL);

  res = gst_element_link (src, adder);
  fail_unless (res == TRUE, NULL);
  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  /* create an unconnected sinkpad in adder */
  pad = gst_element_get_request_pad (adder, "sink%d");
  fail_if (pad == NULL, NULL);

  srcpad = gst_element_get_static_pad (adder, "src");
  gst_object_unref (srcpad);

  main_loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message::segment-done", (GCallback) message_received,
      bin);
  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  GST_INFO ("starting test");

  /* prepare playing, this will not preroll as adder is waiting
   * on the unconnected sinkpad. */
  res = gst_element_set_state (bin, GST_STATE_PAUSED);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* wait for completion for one second, will return ASYNC */
  res = gst_element_get_state (GST_ELEMENT (bin), NULL, NULL, GST_SECOND);
  fail_unless (res == GST_STATE_CHANGE_ASYNC, NULL);

  /* get rid of the pad now, adder should stop waiting on it and
   * continue the preroll */
  gst_element_release_request_pad (adder, pad);
  gst_object_unref (pad);

  /* wait for completion, should work now */
  res =
      gst_element_get_state (GST_ELEMENT (bin), NULL, NULL,
      GST_CLOCK_TIME_NONE);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* now play all */
  res = gst_element_set_state (bin, GST_STATE_PLAYING);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  g_main_loop_run (main_loop);

  res = gst_element_set_state (bin, GST_STATE_NULL);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* cleanup */
  g_main_loop_unref (main_loop);
  gst_object_unref (G_OBJECT (bus));
  gst_object_unref (G_OBJECT (bin));
}

GST_END_TEST;


static GstBuffer *handoff_buffer = NULL;
static void
handoff_buffer_cb (GstElement * fakesink, GstBuffer * buffer, GstPad * pad,
    gpointer user_data)
{
  GST_DEBUG ("got buffer %p", buffer);
  gst_buffer_replace (&handoff_buffer, buffer);
}

/* check if clipping works as expected */
GST_START_TEST (test_clip)
{
  GstElement *bin, *adder, *sink;
  GstBus *bus;
  GstPad *sinkpad;
  gboolean res;
  GstFlowReturn ret;
  GstEvent *event;
  GstBuffer *buffer;
  GstCaps *caps;

  GST_INFO ("preparing test");

  /* build pipeline */
  bin = gst_pipeline_new ("pipeline");
  bus = gst_element_get_bus (bin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  g_signal_connect (bus, "message::error", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::warning", (GCallback) message_received, bin);
  g_signal_connect (bus, "message::eos", (GCallback) message_received, bin);

  /* just an adder and a fakesink */
  adder = gst_element_factory_make ("adder", "adder");
  sink = gst_element_factory_make ("fakesink", "sink");
  g_object_set (sink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (sink, "handoff", (GCallback) handoff_buffer_cb, NULL);
  gst_bin_add_many (GST_BIN (bin), adder, sink, NULL);

  res = gst_element_link (adder, sink);
  fail_unless (res == TRUE, NULL);

  /* set to playing */
  res = gst_element_set_state (bin, GST_STATE_PLAYING);
  fail_unless (res != GST_STATE_CHANGE_FAILURE, NULL);

  /* create an unconnected sinkpad in adder, should also automatically activate
   * the pad */
  sinkpad = gst_element_get_request_pad (adder, "sink%d");
  fail_if (sinkpad == NULL, NULL);

  /* send segment to adder */
  event = gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_TIME,
      GST_SECOND, 2 * GST_SECOND, 0);
  gst_pad_send_event (sinkpad, event);

  caps = gst_caps_new_simple ("audio/x-raw-int",
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 2,
      "endianness", G_TYPE_INT, G_BYTE_ORDER,
      "width", G_TYPE_INT, 16,
      "depth", G_TYPE_INT, 16, "signed", G_TYPE_BOOLEAN, TRUE, NULL);

  /* should be clipped and ok */
  buffer = gst_buffer_new_and_alloc (44100);
  GST_BUFFER_TIMESTAMP (buffer) = 0;
  GST_BUFFER_DURATION (buffer) = 250 * GST_MSECOND;
  gst_buffer_set_caps (buffer, caps);
  GST_DEBUG ("pushing buffer %p", buffer);
  ret = gst_pad_chain (sinkpad, buffer);
  fail_unless (ret == GST_FLOW_OK);
  fail_unless (handoff_buffer == NULL);

  /* should be partially clipped */
  buffer = gst_buffer_new_and_alloc (44100);
  GST_BUFFER_TIMESTAMP (buffer) = 900 * GST_MSECOND;
  GST_BUFFER_DURATION (buffer) = 250 * GST_MSECOND;
  gst_buffer_set_caps (buffer, caps);
  GST_DEBUG ("pushing buffer %p", buffer);
  ret = gst_pad_chain (sinkpad, buffer);
  fail_unless (ret == GST_FLOW_OK);
  fail_unless (handoff_buffer != NULL);
  gst_buffer_replace (&handoff_buffer, NULL);

  /* should not be clipped */
  buffer = gst_buffer_new_and_alloc (44100);
  GST_BUFFER_TIMESTAMP (buffer) = 1 * GST_SECOND;
  GST_BUFFER_DURATION (buffer) = 250 * GST_MSECOND;
  gst_buffer_set_caps (buffer, caps);
  GST_DEBUG ("pushing buffer %p", buffer);
  ret = gst_pad_chain (sinkpad, buffer);
  fail_unless (ret == GST_FLOW_OK);
  fail_unless (handoff_buffer != NULL);
  gst_buffer_replace (&handoff_buffer, NULL);

  /* should be clipped and ok */
  buffer = gst_buffer_new_and_alloc (44100);
  GST_BUFFER_TIMESTAMP (buffer) = 2 * GST_SECOND;
  GST_BUFFER_DURATION (buffer) = 250 * GST_MSECOND;
  gst_buffer_set_caps (buffer, caps);
  GST_DEBUG ("pushing buffer %p", buffer);
  ret = gst_pad_chain (sinkpad, buffer);
  fail_unless (ret == GST_FLOW_OK);
  fail_unless (handoff_buffer == NULL);


}

GST_END_TEST;

static Suite *
adder_suite (void)
{
  Suite *s = suite_create ("adder");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_event);
  tcase_add_test (tc_chain, test_play_twice);
  tcase_add_test (tc_chain, test_play_twice_then_add_and_play_again);
  tcase_add_test (tc_chain, test_live_seeking);
  tcase_add_test (tc_chain, test_add_pad);
  tcase_add_test (tc_chain, test_remove_pad);
  tcase_add_test (tc_chain, test_clip);

  /* Use a longer timeout */
#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    tcase_set_timeout (tc_chain, 5 * 60);
  } else
#endif
  {
    /* this is shorter than the default 60 seconds?! (tpm) */
    /* tcase_set_timeout (tc_chain, 6); */
  }

  return s;
}

GST_CHECK_MAIN (adder);
