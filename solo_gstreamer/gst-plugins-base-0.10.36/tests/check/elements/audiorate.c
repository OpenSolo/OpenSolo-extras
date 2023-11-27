/* GStreamer unit tests for audiorate
 *
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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

#include <gst/check/gstcheck.h>

/* helper element to insert additional buffers overlapping with previous ones */
static gdouble injector_inject_probability = 0.0;

typedef GstElement TestInjector;
typedef GstElementClass TestInjectorClass;

GType test_injector_get_type (void);
GST_BOILERPLATE (TestInjector, test_injector, GstElement, GST_TYPE_ELEMENT);

#define INJECTOR_CAPS \
  "audio/x-raw-float, "                                  \
    "rate = (int) [ 1, MAX ], "                          \
    "channels = (int) [ 1, 8 ], "                        \
    "endianness = (int) BYTE_ORDER, "                    \
    "width = (int) 32;"                                  \
  "audio/x-raw-int, "                                    \
    "rate = (int) [ 1, MAX ], "                          \
    "channels = (int) [ 1, 8 ], "                        \
    "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }, " \
    "width = (int) { 8, 16, 32 }, "                      \
    "depth = (int) [ 1, 32 ], "                          \
    "signed = (boolean) { true, false }"

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (INJECTOR_CAPS));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (INJECTOR_CAPS));

static void
test_injector_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class, &src_template);
  gst_element_class_add_static_pad_template (element_class,
      &sink_template);
}

static void
test_injector_class_init (TestInjectorClass * klass)
{
  /* nothing to do here */
}

static GstFlowReturn
test_injector_chain (GstPad * pad, GstBuffer * buf)
{
  GstFlowReturn ret;
  GstPad *srcpad;

  srcpad =
      gst_element_get_static_pad (GST_ELEMENT (GST_PAD_PARENT (pad)), "src");

  /* since we're increasing timestamp/offsets, push this one first */
  GST_LOG (" passing buffer   [t=%" GST_TIME_FORMAT "-%" GST_TIME_FORMAT
      "], offset=%" G_GINT64_FORMAT ", offset_end=%" G_GINT64_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf) + GST_BUFFER_DURATION (buf)),
      GST_BUFFER_OFFSET (buf), GST_BUFFER_OFFSET_END (buf));

  gst_buffer_ref (buf);

  ret = gst_pad_push (srcpad, buf);

  if (g_random_double () < injector_inject_probability) {
    GstBuffer *ibuf;

    ibuf = gst_buffer_copy (buf);

    if (GST_BUFFER_OFFSET_IS_VALID (buf) &&
        GST_BUFFER_OFFSET_END_IS_VALID (buf)) {
      guint64 delta;

      delta = GST_BUFFER_OFFSET_END (buf) - GST_BUFFER_OFFSET (buf);
      GST_BUFFER_OFFSET (ibuf) += delta / 4;
      GST_BUFFER_OFFSET_END (ibuf) += delta / 4;
    } else {
      GST_BUFFER_OFFSET (ibuf) = GST_BUFFER_OFFSET_NONE;
      GST_BUFFER_OFFSET_END (ibuf) = GST_BUFFER_OFFSET_NONE;
    }

    if (GST_BUFFER_TIMESTAMP_IS_VALID (buf) &&
        GST_BUFFER_DURATION_IS_VALID (buf)) {
      GstClockTime delta;

      delta = GST_BUFFER_DURATION (buf);
      GST_BUFFER_TIMESTAMP (ibuf) += delta / 4;
    } else {
      GST_BUFFER_TIMESTAMP (ibuf) = GST_CLOCK_TIME_NONE;
      GST_BUFFER_DURATION (ibuf) = GST_CLOCK_TIME_NONE;
    }

    if (GST_BUFFER_TIMESTAMP_IS_VALID (ibuf) ||
        GST_BUFFER_OFFSET_IS_VALID (ibuf)) {
      GST_LOG ("injecting buffer [t=%" GST_TIME_FORMAT "-%" GST_TIME_FORMAT
          "], offset=%" G_GINT64_FORMAT ", offset_end=%" G_GINT64_FORMAT,
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (ibuf)),
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (ibuf) +
              GST_BUFFER_DURATION (ibuf)), GST_BUFFER_OFFSET (ibuf),
          GST_BUFFER_OFFSET_END (ibuf));

      if (gst_pad_push (srcpad, ibuf) != GST_FLOW_OK) {
        /* ignore return value */
      }
    } else {
      GST_WARNING ("couldn't inject buffer, no incoming timestamps or offsets");
      gst_buffer_unref (ibuf);
    }
  }

  gst_buffer_unref (buf);

  return ret;
}

static void
test_injector_init (TestInjector * injector, TestInjectorClass * klass)
{
  GstPad *pad;

  pad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_pad_set_chain_function (pad, test_injector_chain);
  gst_pad_set_getcaps_function (pad, gst_pad_proxy_getcaps);
  gst_pad_set_setcaps_function (pad, gst_pad_proxy_setcaps);
  gst_element_add_pad (GST_ELEMENT (injector), pad);

  pad = gst_pad_new_from_static_template (&src_template, "src");
  gst_pad_set_getcaps_function (pad, gst_pad_proxy_getcaps);
  gst_pad_set_setcaps_function (pad, gst_pad_proxy_setcaps);
  gst_element_add_pad (GST_ELEMENT (injector), pad);
}

static gboolean
probe_cb (GstPad * pad, GstBuffer * buf, gdouble * drop_probability)
{
  if (g_random_double () < *drop_probability) {
    GST_LOG ("dropping buffer [t=%" GST_TIME_FORMAT "-%" GST_TIME_FORMAT "], "
        "offset=%" G_GINT64_FORMAT ", offset_end=%" G_GINT64_FORMAT,
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf) + GST_BUFFER_DURATION (buf)),
        GST_BUFFER_OFFSET (buf), GST_BUFFER_OFFSET_END (buf));
    return FALSE;               /* drop buffer */
  }

  return TRUE;                  /* don't drop buffer */
}

static void
got_buf (GstElement * fakesink, GstBuffer * buf, GstPad * pad, GList ** p_bufs)
{
  *p_bufs = g_list_append (*p_bufs, gst_buffer_ref (buf));
}

static void
do_perfect_stream_test (guint rate, guint width, gdouble drop_probability,
    gdouble inject_probability)
{
  GstElement *pipe, *src, *conv, *filter, *injector, *audiorate, *sink;
  GstMessage *msg;
  GstCaps *caps;
  GstPad *srcpad;
  GList *l, *bufs = NULL;
  GstClockTime next_time = GST_CLOCK_TIME_NONE;
  guint64 next_offset = GST_BUFFER_OFFSET_NONE;

  caps = gst_caps_new_simple ("audio/x-raw-int", "rate", G_TYPE_INT,
      rate, "width", G_TYPE_INT, width, NULL);

  GST_INFO ("-------- drop=%.0f%% caps = %" GST_PTR_FORMAT " ---------- ",
      drop_probability * 100.0, caps);

  g_assert (drop_probability >= 0.0 && drop_probability <= 1.0);
  g_assert (inject_probability >= 0.0 && inject_probability <= 1.0);
  g_assert (width > 0 && (width % 8) == 0);

  pipe = gst_pipeline_new ("pipeline");
  fail_unless (pipe != NULL);

  src = gst_element_factory_make ("audiotestsrc", "audiotestsrc");
  fail_unless (src != NULL);

  g_object_set (src, "num-buffers", 100, NULL);

  conv = gst_element_factory_make ("audioconvert", "audioconvert");
  fail_unless (conv != NULL);

  filter = gst_element_factory_make ("capsfilter", "capsfilter");
  fail_unless (filter != NULL);

  g_object_set (filter, "caps", caps, NULL);

  injector_inject_probability = inject_probability;

  injector = GST_ELEMENT (g_object_new (test_injector_get_type (), NULL));

  srcpad = gst_element_get_static_pad (injector, "src");
  fail_unless (srcpad != NULL);
  gst_pad_add_buffer_probe (srcpad, G_CALLBACK (probe_cb), &drop_probability);
  gst_object_unref (srcpad);

  audiorate = gst_element_factory_make ("audiorate", "audiorate");
  fail_unless (audiorate != NULL);

  sink = gst_element_factory_make ("fakesink", "fakesink");
  fail_unless (sink != NULL);

  g_object_set (sink, "signal-handoffs", TRUE, NULL);

  g_signal_connect (sink, "handoff", G_CALLBACK (got_buf), &bufs);

  gst_bin_add_many (GST_BIN (pipe), src, conv, filter, injector, audiorate,
      sink, NULL);
  gst_element_link_many (src, conv, filter, injector, audiorate, sink, NULL);

  fail_unless_equals_int (gst_element_set_state (pipe, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);

  fail_unless_equals_int (gst_element_get_state (pipe, NULL, NULL, -1),
      GST_STATE_CHANGE_SUCCESS);

  msg = gst_bus_poll (GST_ELEMENT_BUS (pipe),
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR, -1);
  fail_unless_equals_string (GST_MESSAGE_TYPE_NAME (msg), "eos");

  for (l = bufs; l != NULL; l = l->next) {
    GstBuffer *buf = GST_BUFFER (l->data);
    guint num_samples;

    fail_unless (GST_BUFFER_TIMESTAMP_IS_VALID (buf));
    fail_unless (GST_BUFFER_DURATION_IS_VALID (buf));
    fail_unless (GST_BUFFER_OFFSET_IS_VALID (buf));
    fail_unless (GST_BUFFER_OFFSET_END_IS_VALID (buf));

    GST_LOG ("buffer: ts=%" GST_TIME_FORMAT ", end_ts=%" GST_TIME_FORMAT
        " off=%" G_GINT64_FORMAT ", end_off=%" G_GINT64_FORMAT,
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf) + GST_BUFFER_DURATION (buf)),
        GST_BUFFER_OFFSET (buf), GST_BUFFER_OFFSET_END (buf));

    if (GST_CLOCK_TIME_IS_VALID (next_time)) {
      fail_unless_equals_uint64 (next_time, GST_BUFFER_TIMESTAMP (buf));
    }
    if (next_offset != GST_BUFFER_OFFSET_NONE) {
      fail_unless_equals_uint64 (next_offset, GST_BUFFER_OFFSET (buf));
    }

    /* check buffer size for sanity */
    fail_unless_equals_int (GST_BUFFER_SIZE (buf) % (width / 8), 0);

    /* check there is actually as much data as there should be */
    num_samples = GST_BUFFER_OFFSET_END (buf) - GST_BUFFER_OFFSET (buf);
    fail_unless_equals_int (GST_BUFFER_SIZE (buf), num_samples * (width / 8));

    next_time = GST_BUFFER_TIMESTAMP (buf) + GST_BUFFER_DURATION (buf);
    next_offset = GST_BUFFER_OFFSET_END (buf);
  }

  gst_message_unref (msg);
  gst_element_set_state (pipe, GST_STATE_NULL);
  gst_object_unref (pipe);

  g_list_foreach (bufs, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (bufs);

  gst_caps_unref (caps);
}

static const guint rates[] = { 8000, 11025, 16000, 22050, 32000, 44100,
  48000, 3333, 33333, 66666, 9999
};

GST_START_TEST (test_perfect_stream_drop0)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.0, 0.0);
    do_perfect_stream_test (rates[i], 16, 0.0, 0.0);
  }
}

GST_END_TEST;

GST_START_TEST (test_perfect_stream_drop10)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.10, 0.0);
    do_perfect_stream_test (rates[i], 16, 0.10, 0.0);
  }
}

GST_END_TEST;

GST_START_TEST (test_perfect_stream_drop50)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.50, 0.0);
    do_perfect_stream_test (rates[i], 16, 0.50, 0.0);
  }
}

GST_END_TEST;

GST_START_TEST (test_perfect_stream_drop90)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.90, 0.0);
    do_perfect_stream_test (rates[i], 16, 0.90, 0.0);
  }
}

GST_END_TEST;

GST_START_TEST (test_perfect_stream_inject10)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.0, 0.10);
    do_perfect_stream_test (rates[i], 16, 0.0, 0.10);
  }
}

GST_END_TEST;

GST_START_TEST (test_perfect_stream_inject90)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.0, 0.90);
    do_perfect_stream_test (rates[i], 16, 0.0, 0.90);
  }
}

GST_END_TEST;

GST_START_TEST (test_perfect_stream_drop45_inject25)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (rates); ++i) {
    do_perfect_stream_test (rates[i], 8, 0.45, 0.25);
    do_perfect_stream_test (rates[i], 16, 0.45, 0.25);
  }
}

GST_END_TEST;

/* TODO: also do all tests with channels=1 and channels=2 */

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-float,channels=1,rate=44100,width=32")
    );

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-float,channels=1,rate=44100,width=32")
    );

GST_START_TEST (test_large_discont)
{
  GstElement *audiorate;
  GstCaps *caps;
  GstPad *srcpad, *sinkpad;
  GstBuffer *buf;

  audiorate = gst_check_setup_element ("audiorate");
  caps = gst_caps_new_simple ("audio/x-raw-float",
      "channels", G_TYPE_INT, 1,
      "rate", G_TYPE_INT, 44100, "width", G_TYPE_INT, 32,
      "endianness", G_TYPE_INT, G_BYTE_ORDER, NULL);

  srcpad = gst_check_setup_src_pad (audiorate, &srctemplate, caps);
  sinkpad = gst_check_setup_sink_pad (audiorate, &sinktemplate, caps);

  gst_pad_set_active (srcpad, TRUE);
  gst_pad_set_active (sinkpad, TRUE);

  fail_unless (gst_element_set_state (audiorate,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "failed to set audiorate playing");

  buf = gst_buffer_new_and_alloc (4);
  gst_buffer_set_caps (buf, caps);
  GST_BUFFER_TIMESTAMP (buf) = 0;
  gst_pad_push (srcpad, buf);

  fail_unless_equals_int (g_list_length (buffers), 1);

  buf = gst_buffer_new_and_alloc (4);
  gst_buffer_set_caps (buf, caps);
  GST_BUFFER_TIMESTAMP (buf) = 2 * GST_SECOND;
  gst_pad_push (srcpad, buf);
  /* Now we should have 3 more buffers: the one we injected, plus _two_ filler
   * buffers, because the gap is > 1 second (but less than 2 seconds) */
  fail_unless_equals_int (g_list_length (buffers), 4);

  gst_element_set_state (audiorate, GST_STATE_NULL);
  gst_caps_unref (caps);

  gst_check_teardown_sink_pad (audiorate);
  gst_check_teardown_src_pad (audiorate);

  gst_object_unref (audiorate);
}

GST_END_TEST;

static Suite *
audiorate_suite (void)
{
  Suite *s = suite_create ("audiorate");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_perfect_stream_drop0);
  tcase_add_test (tc_chain, test_perfect_stream_drop10);
  tcase_add_test (tc_chain, test_perfect_stream_drop50);
  tcase_add_test (tc_chain, test_perfect_stream_drop90);
  tcase_add_test (tc_chain, test_perfect_stream_inject10);
  tcase_add_test (tc_chain, test_perfect_stream_inject90);
  tcase_add_test (tc_chain, test_perfect_stream_drop45_inject25);
  tcase_add_test (tc_chain, test_large_discont);

  return s;
}

GST_CHECK_MAIN (audiorate);
