/* GStreamer
 *
 * Copyright (C) 2009, Axis Communications AB, LUND, SWEDEN
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

#include <gst/check/gstcheck.h>
#include <gst/app/gstappsink.h>

gint global_testdata;

static GstPad *mysrcpad;

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-gst-check")
    );

static GstElement *
setup_appsink (void)
{
  GstElement *appsink;

  GST_DEBUG ("setup_appsink");
  appsink = gst_check_setup_element ("appsink");
  mysrcpad = gst_check_setup_src_pad (appsink, &srctemplate, NULL);

  return appsink;
}

static void
cleanup_appsink (GstElement * appsink)
{
  GST_DEBUG ("cleanup_appsink");

  gst_check_teardown_src_pad (appsink);
  gst_check_teardown_element (appsink);
}

/* This function does an operation to it's indata argument and returns it.
 * The exact operation performed doesn't matter. Currently it multiplies with
 * two, but it could do anything. The idea is to use the function to verify
 * that the code calling it gets run. */
static gint
operate_on_data (gint indata)
{
  return indata * 2;
}

static GstFlowReturn
callback_function (GstAppSink * appsink, gpointer callback_data)
{
  global_testdata = operate_on_data (*((gint *) callback_data));

  return GST_FLOW_OK;
}

static void
notify_function (gpointer callback_data)
{
  global_testdata = operate_on_data (*((gint *) callback_data));
}

GST_START_TEST (test_non_clients)
{
  GstElement *sink;
  GstBuffer *buffer;
  GstCaps *caps;

  sink = setup_appsink ();

  ASSERT_SET_STATE (sink, GST_STATE_PLAYING, GST_STATE_CHANGE_ASYNC);

  caps = gst_caps_from_string ("application/x-gst-check");
  buffer = gst_buffer_new_and_alloc (4);
  gst_buffer_set_caps (buffer, caps);
  gst_caps_unref (caps);
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  GST_DEBUG ("cleaning up appsink");
  ASSERT_SET_STATE (sink, GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);
  cleanup_appsink (sink);
}

GST_END_TEST;

/* Verifies that the handoff callback gets run one time when passing a buffer */
GST_START_TEST (test_handoff_callback)
{
  GstElement *sink;
  GstBuffer *buffer;
  GstCaps *caps;
  gint testdata;
  GstAppSinkCallbacks callbacks = { NULL };

  sink = setup_appsink ();

  global_testdata = 0;
  testdata = 5;                 /* Arbitrary value */

  callbacks.new_buffer = callback_function;

  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks, &testdata, NULL);

  ASSERT_SET_STATE (sink, GST_STATE_PLAYING, GST_STATE_CHANGE_ASYNC);

  caps = gst_caps_from_string ("application/x-gst-check");
  buffer = gst_buffer_new_and_alloc (4);
  gst_buffer_set_caps (buffer, caps);
  gst_caps_unref (caps);
  /* Pushing a buffer should run our callback */
  fail_unless (gst_pad_push (mysrcpad, buffer) == GST_FLOW_OK);

  testdata = operate_on_data (testdata);

  /* If both test_data & global_testdata have been operated on, we're happy. */
  fail_unless (testdata == global_testdata);

  GST_DEBUG ("cleaning up appsink");
  ASSERT_SET_STATE (sink, GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);
  cleanup_appsink (sink);
}

GST_END_TEST;

/* Verifies that the notify function gets executed when the sink is destroyed */
GST_START_TEST (test_notify0)
{
  GstElement *sink;
  gint testdata;
  GstAppSinkCallbacks callbacks = { NULL };

  sink = gst_element_factory_make ("appsink", NULL);

  global_testdata = 0;
  testdata = 17;                /* Arbitrary value */

  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks,
      &testdata, (*notify_function));

  GST_DEBUG ("cleaning up appsink");
  /* Destroying sink should call our notify_function */
  gst_object_unref (sink);

  testdata = operate_on_data (testdata);

  /* If both test_data & global_testdata have been operated on, we're happy. */
  fail_unless (testdata == global_testdata);
}

GST_END_TEST;


/* Verifies that the notify function gets executed when
 * gst_app_sink_set_callbacks () gets called */
GST_START_TEST (test_notify1)
{
  GstElement *sink;
  gint testdata;
  GstAppSinkCallbacks callbacks = { NULL };

  sink = gst_element_factory_make ("appsink", NULL);

  global_testdata = 0;
  testdata = 42;                /* Arbitrary value */

  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks,
      &testdata, (*notify_function));
  /* Setting new callbacks should trigger the destroy of the old data */
  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks, &testdata, NULL);

  testdata = operate_on_data (testdata);

  /* If both test_data & global_testdata have been operated on, we're happy. */
  fail_unless (testdata == global_testdata);

  GST_DEBUG ("cleaning up appsink");
  gst_object_unref (sink);
}

GST_END_TEST;

static GstBufferList *mylist;
static GstCaps *mycaps;

static GstBufferList *
create_buffer_list (void)
{
  GstBufferListIterator *it;
  GstBuffer *buffer;

  mylist = gst_buffer_list_new ();
  fail_if (mylist == NULL);

  mycaps = gst_caps_from_string ("application/x-gst-check");
  fail_if (mycaps == NULL);

  it = gst_buffer_list_iterate (mylist);
  fail_if (it == NULL);

  gst_buffer_list_iterator_add_group (it);

  buffer = gst_buffer_new_and_alloc (sizeof (gint));
  *(gint *) GST_BUFFER_DATA (buffer) = 1;
  gst_buffer_set_caps (buffer, mycaps);
  gst_buffer_list_iterator_add (it, buffer);

  gst_buffer_list_iterator_add_group (it);

  buffer = gst_buffer_new_and_alloc (sizeof (gint));
  *(gint *) GST_BUFFER_DATA (buffer) = 2;
  gst_buffer_set_caps (buffer, mycaps);
  gst_buffer_list_iterator_add (it, buffer);

  buffer = gst_buffer_new_and_alloc (sizeof (gint));
  *(gint *) GST_BUFFER_DATA (buffer) = 4;
  gst_buffer_set_caps (buffer, mycaps);
  gst_buffer_list_iterator_add (it, buffer);

  gst_buffer_list_iterator_free (it);

  return mylist;
}

static void
check_buffer_list (GstBufferList * list)
{
  GstBufferListIterator *it;
  GstBuffer *buf;
  GstCaps *caps;

  fail_unless (list == mylist);
  fail_unless (gst_buffer_list_n_groups (list) == 2);

  it = gst_buffer_list_iterate (list);
  fail_if (it == NULL);

  fail_unless (gst_buffer_list_iterator_next_group (it));
  fail_unless (gst_buffer_list_iterator_n_buffers (it) == 1);
  buf = gst_buffer_list_iterator_next (it);
  fail_if (buf == NULL);
  fail_unless (*(gint *) GST_BUFFER_DATA (buf) == 1);
  caps = gst_buffer_get_caps (buf);
  fail_unless (caps == mycaps);
  fail_unless (gst_caps_is_equal (caps, mycaps));
  gst_caps_unref (caps);

  fail_unless (gst_buffer_list_iterator_next_group (it));
  fail_unless (gst_buffer_list_iterator_n_buffers (it) == 2);
  buf = gst_buffer_list_iterator_next (it);
  fail_if (buf == NULL);
  fail_unless (*(gint *) GST_BUFFER_DATA (buf) == 2);
  caps = gst_buffer_get_caps (buf);
  fail_unless (caps == mycaps);
  gst_caps_unref (caps);

  buf = gst_buffer_list_iterator_next (it);
  fail_if (buf == NULL);
  fail_unless (*(gint *) GST_BUFFER_DATA (buf) == 4);
  caps = gst_buffer_get_caps (buf);
  fail_unless (caps == mycaps);
  gst_caps_unref (caps);

  gst_buffer_list_iterator_free (it);
}

static GstFlowReturn
callback_function_buffer_list (GstAppSink * appsink, gpointer callback_data)
{
  GstBufferList *list;

  list = gst_app_sink_pull_buffer_list (appsink);
  fail_unless (GST_IS_BUFFER_LIST (list));

  check_buffer_list (list);

  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}

GST_START_TEST (test_buffer_list)
{
  GstElement *sink;
  GstBufferList *list;
  GstAppSinkCallbacks callbacks = { NULL };

  sink = setup_appsink ();

  callbacks.new_buffer_list = callback_function_buffer_list;

  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks, NULL, NULL);

  ASSERT_SET_STATE (sink, GST_STATE_PLAYING, GST_STATE_CHANGE_ASYNC);

  list = create_buffer_list ();
  fail_unless (gst_pad_push_list (mysrcpad, list) == GST_FLOW_OK);

  ASSERT_SET_STATE (sink, GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);
  cleanup_appsink (sink);
}

GST_END_TEST;

static GstFlowReturn
callback_function_buffer (GstAppSink * appsink, gpointer p_counter)
{
  GstBuffer *buf;
  gint *p_int_counter = p_counter;

  buf = gst_app_sink_pull_buffer (appsink);
  fail_unless (GST_IS_BUFFER (buf));

  /* buffer list has 3 buffers in two groups */
  switch (*p_int_counter) {
    case 0:
      fail_unless_equals_int (GST_BUFFER_SIZE (buf), sizeof (gint));
      fail_unless_equals_int ((((gint *) GST_BUFFER_DATA (buf))[0]), 1);
      break;
    case 1:
      fail_unless_equals_int (GST_BUFFER_SIZE (buf), 2 * sizeof (gint));
      fail_unless_equals_int ((((gint *) GST_BUFFER_DATA (buf))[0]), 2);
      fail_unless_equals_int ((((gint *) GST_BUFFER_DATA (buf))[1]), 4);
      break;
    default:
      g_warn_if_reached ();
      break;
  }

  gst_buffer_unref (buf);

  *p_int_counter += 1;

  return GST_FLOW_OK;
}

GST_START_TEST (test_buffer_list_fallback)
{
  GstElement *sink;
  GstBufferList *list;
  GstAppSinkCallbacks callbacks = { NULL };
  gint counter = 0;

  sink = setup_appsink ();

  callbacks.new_buffer = callback_function_buffer;

  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks, &counter, NULL);

  ASSERT_SET_STATE (sink, GST_STATE_PLAYING, GST_STATE_CHANGE_ASYNC);

  list = create_buffer_list ();
  fail_unless (gst_pad_push_list (mysrcpad, list) == GST_FLOW_OK);

  fail_unless_equals_int (counter, 2);

  ASSERT_SET_STATE (sink, GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);
  cleanup_appsink (sink);
}

GST_END_TEST;

GST_START_TEST (test_buffer_list_fallback_signal)
{
  GstElement *sink;
  GstBufferList *list;
  gint counter = 0;

  sink = setup_appsink ();

  /* C calling convention to the rescue.. */
  g_signal_connect (sink, "new-buffer", G_CALLBACK (callback_function_buffer),
      &counter);

  g_object_set (sink, "emit-signals", TRUE, NULL);

  ASSERT_SET_STATE (sink, GST_STATE_PLAYING, GST_STATE_CHANGE_ASYNC);

  list = create_buffer_list ();
  fail_unless (gst_pad_push_list (mysrcpad, list) == GST_FLOW_OK);

  fail_unless_equals_int (counter, 2);

  ASSERT_SET_STATE (sink, GST_STATE_NULL, GST_STATE_CHANGE_SUCCESS);
  cleanup_appsink (sink);
}

GST_END_TEST;

static Suite *
appsink_suite (void)
{
  Suite *s = suite_create ("appsink");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_non_clients);
  tcase_add_test (tc_chain, test_handoff_callback);
  tcase_add_test (tc_chain, test_notify0);
  tcase_add_test (tc_chain, test_notify1);
  tcase_add_test (tc_chain, test_buffer_list);
  tcase_add_test (tc_chain, test_buffer_list_fallback);
  tcase_add_test (tc_chain, test_buffer_list_fallback_signal);

  return s;
}

GST_CHECK_MAIN (appsink);
