/* GStreamer
 *
 * unit test for videoscale
 *
 * Copyright (C) <2009,2010> Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#include <gst/video/video.h>
#include <gst/base/gstbasesink.h>

#include <gst/check/gstcheck.h>
#include <string.h>

/* kids, don't do this at home, skipping checks is *BAD* */
#define LINK_CHECK_FLAGS GST_PAD_LINK_CHECK_NOTHING

static GstCaps **
videoscale_get_allowed_caps (void)
{
  GstElement *scale = gst_element_factory_make ("videoscale", "scale");
  GstPadTemplate *templ;
  GstCaps *caps, **ret;
  GstStructure *s;
  gint i, n;

  templ =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (scale),
      "sink");
  fail_unless (templ != NULL);

  caps = gst_pad_template_get_caps (templ);

  n = gst_caps_get_size (caps);
  ret = g_new0 (GstCaps *, n + 1);

  for (i = 0; i < n; i++) {
    s = gst_caps_get_structure (caps, i);
    ret[i] = gst_caps_new_empty ();
    gst_caps_append_structure (ret[i], gst_structure_copy (s));
  }

  gst_object_unref (scale);

  return ret;
}

static void
on_sink_handoff (GstElement * element, GstBuffer * buffer, GstPad * pad,
    gpointer user_data)
{
  guint *n_buffers = user_data;

  *n_buffers = *n_buffers + 1;
}

static gboolean
caps_are_64bpp (const GstCaps * caps)
{
  GstVideoFormat fmt;
  GstCaps *fmt_caps;

  /* need fixed caps for _parse_caps */
  fmt_caps = gst_caps_copy (caps);
  gst_structure_remove_field (gst_caps_get_structure (fmt_caps, 0), "width");
  gst_structure_remove_field (gst_caps_get_structure (fmt_caps, 0), "height");
  gst_structure_remove_field (gst_caps_get_structure (fmt_caps, 0),
      "framerate");

  fail_unless (gst_video_format_parse_caps (fmt_caps, &fmt, NULL, NULL));
  gst_caps_unref (fmt_caps);
  return (fmt == GST_VIDEO_FORMAT_ARGB64 || fmt == GST_VIDEO_FORMAT_AYUV64);
}

static void
run_test (const GstCaps * caps, gint src_width, gint src_height,
    gint dest_width, gint dest_height, gint method,
    GCallback src_handoff, gpointer src_handoff_user_data,
    GCallback sink_handoff, gpointer sink_handoff_user_data)
{
  GstElement *pipeline;
  GstElement *src, *ffmpegcolorspace, *capsfilter1, *identity, *scale,
      *capsfilter2, *sink;
  GstMessage *msg;
  GstBus *bus;
  GstCaps *copy;
  guint n_buffers = 0;

  /* skip formats that ffmpegcolorspace can't handle */
  if (caps_are_64bpp (caps))
    return;

  pipeline = gst_element_factory_make ("pipeline", "pipeline");
  fail_unless (pipeline != NULL);

  src = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (src != NULL);
  g_object_set (G_OBJECT (src), "num-buffers", 1, NULL);

  ffmpegcolorspace = gst_element_factory_make ("ffmpegcolorspace", "csp");
  fail_unless (ffmpegcolorspace != NULL);

  capsfilter1 = gst_element_factory_make ("capsfilter", "filter1");
  fail_unless (capsfilter1 != NULL);
  copy = gst_caps_copy (caps);
  gst_caps_set_simple (copy, "width", G_TYPE_INT, src_width, "height",
      G_TYPE_INT, src_height, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
  g_object_set (G_OBJECT (capsfilter1), "caps", copy, NULL);
  gst_caps_unref (copy);

  identity = gst_element_factory_make ("identity", "identity");
  fail_unless (identity != NULL);
  if (src_handoff) {
    g_object_set (G_OBJECT (identity), "signal-handoffs", TRUE, NULL);
    g_signal_connect (identity, "handoff", G_CALLBACK (src_handoff),
        src_handoff_user_data);
  }

  scale = gst_element_factory_make ("videoscale", "scale");
  fail_unless (scale != NULL);
  g_object_set (G_OBJECT (scale), "method", method, NULL);

  capsfilter2 = gst_element_factory_make ("capsfilter", "filter2");
  fail_unless (capsfilter2 != NULL);
  copy = gst_caps_copy (caps);
  gst_caps_set_simple (copy, "width", G_TYPE_INT, dest_width, "height",
      G_TYPE_INT, dest_height, NULL);
  g_object_set (G_OBJECT (capsfilter2), "caps", copy, NULL);
  gst_caps_unref (copy);

  sink = gst_element_factory_make ("fakesink", "sink");
  fail_unless (sink != NULL);
  g_object_set (G_OBJECT (sink), "signal-handoffs", TRUE, "async", FALSE, NULL);
  g_signal_connect (sink, "handoff", G_CALLBACK (on_sink_handoff), &n_buffers);
  if (sink_handoff) {
    g_signal_connect (sink, "handoff", G_CALLBACK (sink_handoff),
        sink_handoff_user_data);
  }

  gst_bin_add_many (GST_BIN (pipeline), src, ffmpegcolorspace, capsfilter1,
      identity, scale, capsfilter2, sink, NULL);

  fail_unless (gst_element_link_pads_full (src, "src", ffmpegcolorspace, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (ffmpegcolorspace, "src", capsfilter1,
          "sink", LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (capsfilter1, "src", identity, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (identity, "src", scale, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (scale, "src", capsfilter2, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (capsfilter2, "src", sink, "sink",
          LINK_CHECK_FLAGS));

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_WARNING);

  fail_unless_equals_int (GST_MESSAGE_TYPE (msg), GST_MESSAGE_EOS);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  fail_unless (n_buffers == 1);

  gst_object_unref (pipeline);
  gst_message_unref (msg);
  gst_object_unref (bus);
}

static void
on_sink_handoff_passthrough (GstElement * element, GstBuffer * buffer,
    GstPad * pad, gpointer user_data)
{
  GList **list = user_data;

  *list = g_list_prepend (*list, gst_buffer_ref (buffer));
}

static void
on_src_handoff_passthrough (GstElement * element, GstBuffer * buffer,
    gpointer user_data)
{
  GList **list = user_data;

  *list = g_list_prepend (*list, gst_buffer_ref (buffer));
}

GST_START_TEST (test_passthrough)
{
  GList *l1, *l2, *src_buffers = NULL, *sink_buffers = NULL;
  GstCaps **allowed_caps = NULL, **p;
  gint method;
  static const gint src_width = 640, src_height = 480;
  static const gint dest_width = 640, dest_height = 480;

  p = allowed_caps = videoscale_get_allowed_caps ();

  while (*p) {
    GstCaps *caps = *p;

    for (method = 0; method < 3; method++) {
      /* skip formats that ffmpegcolorspace can't handle */
      if (caps_are_64bpp (caps))
        continue;

      GST_DEBUG ("Running test for caps '%" GST_PTR_FORMAT "'"
          " from %dx%u to %dx%d with method %d", caps, src_width, src_height,
          dest_width, dest_height, method);
      run_test (caps, src_width, src_height,
          dest_width, dest_height, method,
          G_CALLBACK (on_src_handoff_passthrough), &src_buffers,
          G_CALLBACK (on_sink_handoff_passthrough), &sink_buffers);

      fail_unless (src_buffers && sink_buffers);
      fail_unless_equals_int (g_list_length (src_buffers),
          g_list_length (sink_buffers));

      for (l1 = src_buffers, l2 = sink_buffers; l1 && l2;
          l1 = l1->next, l2 = l2->next) {
        GstBuffer *a = l1->data;
        GstBuffer *b = l2->data;

        fail_unless_equals_int (GST_BUFFER_SIZE (a), GST_BUFFER_SIZE (b));
        fail_unless (GST_BUFFER_DATA (a) == GST_BUFFER_DATA (b));

        gst_buffer_unref (a);
        gst_buffer_unref (b);
      }
      g_list_free (src_buffers);
      src_buffers = NULL;
      g_list_free (sink_buffers);
      sink_buffers = NULL;
    }

    gst_caps_unref (caps);
    p++;
  }
  g_free (allowed_caps);
}

GST_END_TEST;

#define CREATE_TEST(name,method,src_width,src_height,dest_width,dest_height) \
GST_START_TEST (name) \
{ \
  GstCaps **allowed_caps = NULL, **p; \
  \
  p = allowed_caps = videoscale_get_allowed_caps (); \
  \
  while (*p) { \
    GstCaps *caps = *p; \
    \
    GST_DEBUG ("Running test for caps '%" GST_PTR_FORMAT "'" \
        " from %dx%u to %dx%d with method %d", caps, src_width, src_height, \
        dest_width, dest_height, method); \
    run_test (caps, src_width, src_height, \
        dest_width, dest_height, method, \
        NULL, NULL, NULL, NULL); \
    gst_caps_unref (caps); \
    p++; \
  } \
  g_free (allowed_caps); \
} \
\
GST_END_TEST;

CREATE_TEST (test_downscale_640x480_320x240_method_0, 0, 640, 480, 320, 240);
CREATE_TEST (test_downscale_640x480_320x240_method_1, 1, 640, 480, 320, 240);
CREATE_TEST (test_downscale_640x480_320x240_method_2, 2, 640, 480, 320, 240);
CREATE_TEST (test_upscale_320x240_640x480_method_0, 0, 320, 240, 640, 480);
CREATE_TEST (test_upscale_320x240_640x480_method_1, 1, 320, 240, 640, 480);
CREATE_TEST (test_upscale_320x240_640x480_method_2, 2, 320, 240, 640, 480);
CREATE_TEST (test_downscale_640x480_1x1_method_0, 0, 640, 480, 1, 1);
CREATE_TEST (test_downscale_640x480_1x1_method_1, 1, 640, 480, 1, 1);
CREATE_TEST (test_downscale_640x480_1x1_method_2, 2, 640, 480, 1, 1);
CREATE_TEST (test_upscale_1x1_640x480_method_0, 0, 1, 1, 640, 480);
CREATE_TEST (test_upscale_1x1_640x480_method_1, 1, 1, 1, 640, 480);
CREATE_TEST (test_upscale_1x1_640x480_method_2, 2, 1, 1, 640, 480);
CREATE_TEST (test_downscale_641x481_111x30_method_0, 0, 641, 481, 111, 30);
CREATE_TEST (test_downscale_641x481_111x30_method_1, 1, 641, 481, 111, 30);
CREATE_TEST (test_downscale_641x481_111x30_method_2, 2, 641, 481, 111, 30);
CREATE_TEST (test_upscale_111x30_641x481_method_0, 0, 111, 30, 641, 481);
CREATE_TEST (test_upscale_111x30_641x481_method_1, 1, 111, 30, 641, 481);
CREATE_TEST (test_upscale_111x30_641x481_method_2, 2, 111, 30, 641, 481);
CREATE_TEST (test_downscale_641x481_30x111_method_0, 0, 641, 481, 30, 111);
CREATE_TEST (test_downscale_641x481_30x111_method_1, 1, 641, 481, 30, 111);
CREATE_TEST (test_downscale_641x481_30x111_method_2, 2, 641, 481, 30, 111);
CREATE_TEST (test_upscale_30x111_641x481_method_0, 0, 30, 111, 641, 481);
CREATE_TEST (test_upscale_30x111_641x481_method_1, 1, 30, 111, 641, 481);
CREATE_TEST (test_upscale_30x111_641x481_method_2, 2, 30, 111, 641, 481);
CREATE_TEST (test_downscale_640x480_320x1_method_0, 0, 640, 480, 320, 1);
CREATE_TEST (test_downscale_640x480_320x1_method_1, 1, 640, 480, 320, 1);
CREATE_TEST (test_downscale_640x480_320x1_method_2, 2, 640, 480, 320, 1);
CREATE_TEST (test_upscale_320x1_640x480_method_0, 0, 320, 1, 640, 480);
CREATE_TEST (test_upscale_320x1_640x480_method_1, 1, 320, 1, 640, 480);
CREATE_TEST (test_upscale_320x1_640x480_method_2, 2, 320, 1, 640, 480);
CREATE_TEST (test_downscale_640x480_1x240_method_0, 0, 640, 480, 1, 240);
CREATE_TEST (test_downscale_640x480_1x240_method_1, 1, 640, 480, 1, 240);
CREATE_TEST (test_downscale_640x480_1x240_method_2, 2, 640, 480, 1, 240);
CREATE_TEST (test_upscale_1x240_640x480_method_0, 0, 1, 240, 640, 480);
CREATE_TEST (test_upscale_1x240_640x480_method_1, 1, 1, 240, 640, 480);
CREATE_TEST (test_upscale_1x240_640x480_method_2, 2, 1, 240, 640, 480);

typedef struct
{
  gint width, height;
  gint par_n, par_d;
  gboolean ok;
  GMainLoop *loop;
} TestNegotiationData;

static void
_test_negotiation_message (GstBus * bus, GstMessage * message,
    TestNegotiationData * data)
{
  GError *err = NULL;
  gchar *debug;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (message, &err, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), err, debug);
      g_error_free (err);
      g_free (debug);
      g_assert_not_reached ();
      break;
    case GST_MESSAGE_WARNING:
      gst_message_parse_warning (message, &err, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), err, debug);
      g_error_free (err);
      g_free (debug);
      g_assert_not_reached ();
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (data->loop);
      break;
    default:
      break;
  }
}

static void
_test_negotiation_notify_caps (GObject * src, GParamSpec * pspec,
    TestNegotiationData * data)
{
  GstCaps *caps;
  GstStructure *s;
  gint width, height;
  gint par_n = 0, par_d = 0;

  g_object_get (src, "caps", &caps, NULL);
  if (caps == NULL)
    return;

  s = gst_caps_get_structure (caps, 0);

  fail_unless (gst_structure_get_int (s, "width", &width));
  fail_unless (gst_structure_get_int (s, "height", &height));
  fail_unless (gst_structure_get_fraction (s, "pixel-aspect-ratio", &par_n,
          &par_d) || (data->par_n == 1 && data->par_d == 1));

  gst_caps_unref (caps);

  fail_unless_equals_int (width, data->width);
  fail_unless_equals_int (height, data->height);
  if (par_n != 0 || par_d != 0) {
    fail_unless_equals_int (par_n, data->par_n);
    fail_unless_equals_int (par_d, data->par_d);
  }

  data->ok = (width == data->width) && (height == data->height)
      && (par_n == data->par_n) && (par_d == data->par_d);

  g_main_loop_quit (data->loop);
}

static void
_test_negotiation (const gchar * src_templ, const gchar * sink_templ,
    gint width, gint height, gint par_n, gint par_d)
{
  GstElement *pipeline;
  GstElement *src, *capsfilter1, *scale, *capsfilter2, *sink;
  GstBus *bus;
  GMainLoop *loop;
  GstCaps *caps;
  TestNegotiationData data = { 0, 0, 0, 0, FALSE, NULL };
  GstPad *pad;

  GST_DEBUG ("Running test for src templ caps '%s' and sink templ caps '%s'",
      src_templ, sink_templ);

  pipeline = gst_element_factory_make ("pipeline", "pipeline");
  fail_unless (pipeline != NULL);

  src = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (src != NULL);
  g_object_set (G_OBJECT (src), "num-buffers", 1, NULL);

  capsfilter1 = gst_element_factory_make ("capsfilter", "filter1");
  fail_unless (capsfilter1 != NULL);
  caps = gst_caps_from_string (src_templ);
  fail_unless (caps != NULL);
  g_object_set (G_OBJECT (capsfilter1), "caps", caps, NULL);
  gst_caps_unref (caps);

  scale = gst_element_factory_make ("videoscale", "scale");
  fail_unless (scale != NULL);

  capsfilter2 = gst_element_factory_make ("capsfilter", "filter2");
  fail_unless (capsfilter2 != NULL);
  caps = gst_caps_from_string (sink_templ);
  fail_unless (caps != NULL);
  g_object_set (G_OBJECT (capsfilter2), "caps", caps, NULL);
  gst_caps_unref (caps);

  pad = gst_element_get_static_pad (capsfilter2, "sink");
  fail_unless (pad != NULL);
  g_signal_connect (pad, "notify::caps",
      G_CALLBACK (_test_negotiation_notify_caps), &data);
  gst_object_unref (pad);

  sink = gst_element_factory_make ("fakesink", "sink");
  fail_unless (sink != NULL);
  g_object_set (sink, "async", FALSE, NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, capsfilter1, scale, capsfilter2,
      sink, NULL);

  fail_unless (gst_element_link_pads_full (src, "src", capsfilter1, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (capsfilter1, "src", scale, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (scale, "src", capsfilter2, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (capsfilter2, "src", sink, "sink",
          LINK_CHECK_FLAGS));

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  gst_bus_add_signal_watch (bus);

  data.loop = loop;
  data.width = width;
  data.height = height;
  data.par_n = par_n;
  data.par_d = par_d;
  data.ok = FALSE;

  g_signal_connect (bus, "message", G_CALLBACK (_test_negotiation_message),
      &data);

  gst_object_unref (bus);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless (data.ok == TRUE);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
}

GST_START_TEST (test_negotiation)
{
  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=720,height=576,pixel-aspect-ratio=16/15",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=768,height=576",
      768, 576, 1, 1);

  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=320,height=240",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=640,height=320",
      640, 320, 2, 3);

  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=320,height=240",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=640,height=320,pixel-aspect-ratio=[0/1, 1/1]",
      640, 320, 2, 3);

  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=1920,height=2560,pixel-aspect-ratio=1/1",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=[1, 2048],height=[1, 2048],pixel-aspect-ratio=1/1",
      1536, 2048, 1, 1);

  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=1920,height=2560,pixel-aspect-ratio=1/1",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=[1, 2048],height=[1, 2048]",
      1920, 2048, 4, 5);

  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=1920,height=2560",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=[1, 2048],height=[1, 2048]",
      1920, 2048, 4, 5);

  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=1920,height=2560",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=1200,height=[1, 2048],pixel-aspect-ratio=1/1",
      1200, 1600, 1, 1);

  /* Doesn't keep DAR but must be possible! */
  _test_negotiation
      ("video/x-raw-yuv,format=(fourcc)AYUV,width=320,height=240,pixel-aspect-ratio=1/1",
      "video/x-raw-yuv,format=(fourcc)AYUV,width=200,height=200,pixel-aspect-ratio=1/2",
      200, 200, 1, 2);
}

GST_END_TEST;

#define GST_TYPE_TEST_REVERSE_NEGOTIATION_SINK \
  (gst_test_reverse_negotiation_sink_get_type())
#define GST_TEST_REVERSE_NEGOTIATION_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TEST_REVERSE_NEGOTIATION_SINK,GstTestReverseNegotiationSink))
#define GST_TEST_REVERSE_NEGOTIATION_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TEST_REVERSE_NEGOTIATION_SINK,GstTestReverseNegotiationSinkClass))
#define GST_IS_TEST_REVERSE_NEGOTIATION_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TEST_REVERSE_NEGOTIATION_SINK))
#define GST_IS_TEST_REVERSE_NEGOTIATION_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TEST_REVERSE_NEGOTIATION_SINK))
#define GST_TEST_REVERSE_NEGOTIATION_SINK_CAST(obj) ((GstTestReverseNegotiationSink *)obj)

typedef struct _GstTestReverseNegotiationSink GstTestReverseNegotiationSink;
typedef struct _GstTestReverseNegotiationSinkClass
    GstTestReverseNegotiationSinkClass;
struct _GstTestReverseNegotiationSink
{
  GstBaseSink element;
  gint nbuffers;
};

struct _GstTestReverseNegotiationSinkClass
{
  GstBaseSinkClass parent_class;
};

GType gst_test_reverse_negotiation_sink_get_type (void);

GST_BOILERPLATE (GstTestReverseNegotiationSink,
    gst_test_reverse_negotiation_sink, GstBaseSink, GST_TYPE_BASE_SINK);

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_xRGB));

static GstFlowReturn
gst_test_reverse_negotiation_sink_buffer_alloc (GstBaseSink * bsink,
    guint64 offset, guint size, GstCaps * caps, GstBuffer ** buf)
{
  GstTestReverseNegotiationSink *sink =
      GST_TEST_REVERSE_NEGOTIATION_SINK (bsink);
  GstVideoFormat fmt;
  gint width, height;

  fail_unless (gst_video_format_parse_caps (caps, &fmt, &width, &height));

  if (sink->nbuffers < 2) {
    *buf =
        gst_buffer_new_and_alloc (gst_video_format_get_size (fmt, width,
            height));
    gst_buffer_set_caps (*buf, caps);
  } else {
    gint fps_n, fps_d;

    fail_unless (gst_video_parse_caps_framerate (caps, &fps_n, &fps_d));

    width = 512;
    height = 128;
    *buf =
        gst_buffer_new_and_alloc (gst_video_format_get_size (fmt, width,
            height));
    caps = gst_video_format_new_caps (fmt, width, height, fps_n, fps_d, 1, 1);
    gst_buffer_set_caps (*buf, caps);
    gst_caps_unref (caps);
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_test_reverse_negotiation_sink_render (GstBaseSink * bsink,
    GstBuffer * buffer)
{
  GstTestReverseNegotiationSink *sink =
      GST_TEST_REVERSE_NEGOTIATION_SINK (bsink);
  GstCaps *caps = gst_buffer_get_caps (buffer);
  GstVideoFormat fmt;
  gint width, height;

  fail_unless (caps != NULL);
  fail_unless (gst_video_format_parse_caps (caps, &fmt, &width, &height));

  sink->nbuffers++;

  /* The third buffer is still in the old size
   * because the ffmpegcolorspaces can't convert
   * the frame sizes
   */
  if (sink->nbuffers > 3) {
    fail_unless_equals_int (width, 512);
    fail_unless_equals_int (height, 128);
  }

  gst_caps_unref (caps);

  return GST_FLOW_OK;
}

static void
gst_test_reverse_negotiation_sink_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (gstelement_class,
      "Test Reverse Negotiation Sink",
      "Sink",
      "Some test sink", "Sebastian Dröge <sebastian.droege@collabora.co.uk>");
  gst_element_class_add_static_pad_template (gstelement_class,
      &sinktemplate);
}

static void
gst_test_reverse_negotiation_sink_class_init (GstTestReverseNegotiationSinkClass
    * klass)
{
  GstBaseSinkClass *gstbase_sink_class;

  gstbase_sink_class = GST_BASE_SINK_CLASS (klass);

  gstbase_sink_class->buffer_alloc =
      GST_DEBUG_FUNCPTR (gst_test_reverse_negotiation_sink_buffer_alloc);
  gstbase_sink_class->render =
      GST_DEBUG_FUNCPTR (gst_test_reverse_negotiation_sink_render);
}

static void
gst_test_reverse_negotiation_sink_init (GstTestReverseNegotiationSink * sink,
    GstTestReverseNegotiationSinkClass * g_class)
{
  sink->nbuffers = 0;
}

static void
_test_reverse_negotiation_message (GstBus * bus, GstMessage * message,
    GMainLoop * loop)
{
  GError *err = NULL;
  gchar *debug;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (message, &err, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), err, debug);
      g_error_free (err);
      g_free (debug);
      g_assert_not_reached ();
      break;
    case GST_MESSAGE_WARNING:
      gst_message_parse_warning (message, &err, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), err, debug);
      g_error_free (err);
      g_free (debug);
      g_assert_not_reached ();
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }
}

GST_START_TEST (test_reverse_negotiation)
{
  GstElement *pipeline;
  GstElement *src, *csp1, *scale, *csp2, *sink;
  GstBus *bus;
  GMainLoop *loop;

  pipeline = gst_element_factory_make ("pipeline", "pipeline");
  fail_unless (pipeline != NULL);

  src = gst_element_factory_make ("videotestsrc", "src");
  fail_unless (src != NULL);
  g_object_set (G_OBJECT (src), "num-buffers", 8, NULL);

  csp1 = gst_element_factory_make ("ffmpegcolorspace", "csp1");
  fail_unless (csp1 != NULL);

  scale = gst_element_factory_make ("videoscale", "scale");
  fail_unless (scale != NULL);

  csp2 = gst_element_factory_make ("ffmpegcolorspace", "csp2");
  fail_unless (csp2 != NULL);

  sink = g_object_new (GST_TYPE_TEST_REVERSE_NEGOTIATION_SINK, NULL);
  fail_unless (sink != NULL);
  g_object_set (sink, "async", FALSE, NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, csp1, scale, csp2, sink, NULL);

  fail_unless (gst_element_link_pads_full (src, "src", csp1, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (csp1, "src", scale, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (scale, "src", csp2, "sink",
          LINK_CHECK_FLAGS));
  fail_unless (gst_element_link_pads_full (csp2, "src", sink, "sink",
          LINK_CHECK_FLAGS));

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  gst_bus_add_signal_watch (bus);

  g_signal_connect (bus, "message",
      G_CALLBACK (_test_reverse_negotiation_message), loop);

  gst_object_unref (bus);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  g_main_loop_run (loop);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
}

GST_END_TEST;

GST_START_TEST (test_basetransform_negotiation)
{
  GstElement *pipeline, *src, *sink, *scale, *capsfilter1, *capsfilter2;
  GstMessage *msg;
  GstCaps *caps;

  pipeline = gst_pipeline_new (NULL);
  src = gst_element_factory_make ("videotestsrc", NULL);
  capsfilter1 = gst_element_factory_make ("capsfilter", NULL);
  scale = gst_element_factory_make ("videoscale", NULL);
  capsfilter2 = gst_element_factory_make ("capsfilter", NULL);
  sink = gst_element_factory_make ("fakesink", NULL);
  fail_unless (pipeline && src && capsfilter1 && scale && capsfilter2 && sink);

  g_object_set (src, "num-buffers", 3, NULL);

  caps = gst_caps_new_simple ("video/x-raw-yuv", "format", GST_TYPE_FOURCC,
      GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'), "width", G_TYPE_INT, 352,
      "height", G_TYPE_INT, 288, "framerate", GST_TYPE_FRACTION, 30, 1,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, NULL);
  g_object_set (capsfilter1, "caps", caps, NULL);
  gst_caps_unref (caps);

  /* same caps, just different pixel-aspect-ratio */
  caps = gst_caps_new_simple ("video/x-raw-yuv", "format", GST_TYPE_FOURCC,
      GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'), "width", G_TYPE_INT, 352,
      "height", G_TYPE_INT, 288, "framerate", GST_TYPE_FRACTION, 30, 1,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 12, 11, NULL);
  g_object_set (capsfilter2, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_bin_add_many (GST_BIN (pipeline), src, capsfilter1, scale, capsfilter2,
      sink, NULL);
  fail_unless (gst_element_link_many (src, capsfilter1, scale, capsfilter2,
          sink, NULL));

  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_ASYNC);

  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipeline), -1,
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR);
  fail_unless_equals_int (GST_MESSAGE_TYPE (msg), GST_MESSAGE_EOS);
  gst_message_unref (msg);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

GST_END_TEST;

static Suite *
videoscale_suite (void)
{
  Suite *s = suite_create ("videoscale");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_set_timeout (tc_chain, 180);
  tcase_add_test (tc_chain, test_passthrough);
  tcase_add_test (tc_chain, test_downscale_640x480_320x240_method_0);
  tcase_add_test (tc_chain, test_downscale_640x480_320x240_method_1);
  tcase_add_test (tc_chain, test_downscale_640x480_320x240_method_2);
  tcase_add_test (tc_chain, test_upscale_320x240_640x480_method_0);
  tcase_add_test (tc_chain, test_upscale_320x240_640x480_method_1);
  tcase_add_test (tc_chain, test_upscale_320x240_640x480_method_2);
  tcase_add_test (tc_chain, test_downscale_640x480_1x1_method_0);
  tcase_add_test (tc_chain, test_downscale_640x480_1x1_method_1);
  tcase_add_test (tc_chain, test_downscale_640x480_1x1_method_2);
  tcase_add_test (tc_chain, test_upscale_1x1_640x480_method_0);
  tcase_add_test (tc_chain, test_upscale_1x1_640x480_method_1);
  tcase_add_test (tc_chain, test_upscale_1x1_640x480_method_2);
  tcase_add_test (tc_chain, test_downscale_641x481_111x30_method_0);
  tcase_add_test (tc_chain, test_downscale_641x481_111x30_method_1);
  tcase_add_test (tc_chain, test_downscale_641x481_111x30_method_2);
  tcase_add_test (tc_chain, test_upscale_111x30_641x481_method_0);
  tcase_add_test (tc_chain, test_upscale_111x30_641x481_method_1);
  tcase_add_test (tc_chain, test_upscale_111x30_641x481_method_2);
  tcase_add_test (tc_chain, test_downscale_641x481_30x111_method_0);
  tcase_add_test (tc_chain, test_downscale_641x481_30x111_method_1);
  tcase_add_test (tc_chain, test_downscale_641x481_30x111_method_2);
  tcase_add_test (tc_chain, test_upscale_30x111_641x481_method_0);
  tcase_add_test (tc_chain, test_upscale_30x111_641x481_method_1);
  tcase_add_test (tc_chain, test_upscale_30x111_641x481_method_2);
  tcase_add_test (tc_chain, test_downscale_640x480_320x1_method_0);
  tcase_add_test (tc_chain, test_downscale_640x480_320x1_method_1);
  tcase_add_test (tc_chain, test_downscale_640x480_320x1_method_2);
  tcase_add_test (tc_chain, test_upscale_320x1_640x480_method_0);
  tcase_add_test (tc_chain, test_upscale_320x1_640x480_method_1);
  tcase_add_test (tc_chain, test_upscale_320x1_640x480_method_2);
  tcase_add_test (tc_chain, test_downscale_640x480_1x240_method_0);
  tcase_add_test (tc_chain, test_downscale_640x480_1x240_method_1);
  tcase_add_test (tc_chain, test_downscale_640x480_1x240_method_2);
  tcase_add_test (tc_chain, test_upscale_1x240_640x480_method_0);
  tcase_add_test (tc_chain, test_upscale_1x240_640x480_method_1);
  tcase_add_test (tc_chain, test_upscale_1x240_640x480_method_2);
  tcase_add_test (tc_chain, test_negotiation);
  tcase_add_test (tc_chain, test_reverse_negotiation);
  tcase_add_test (tc_chain, test_basetransform_negotiation);

  GST_ERROR ("FIXME: test 64-bpp formats as well");
  return s;
}

GST_CHECK_MAIN (videoscale);
