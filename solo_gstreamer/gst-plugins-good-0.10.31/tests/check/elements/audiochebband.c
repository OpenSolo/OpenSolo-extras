/* GStreamer
 *
 * Copyright (C) 2007 Sebastian Dröge <slomo@circular-chaos.org>
 *
 * audiochebband.c: Unit test for the audiochebband element
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/check/gstcheck.h>

#include <math.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
GstPad *mysrcpad, *mysinkpad;

#define BUFFER_CAPS_STRING_32           \
    "audio/x-raw-float, "               \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "endianness = (int) BYTE_ORDER, "   \
    "width = (int) 32"                  \

#define BUFFER_CAPS_STRING_64           \
    "audio/x-raw-float, "               \
    "channels = (int) 1, "              \
    "rate = (int) 44100, "              \
    "endianness = (int) BYTE_ORDER, "   \
    "width = (int) 64"                  \

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-float, "
        "channels = (int) 1, "
        "rate = (int) 44100, "
        "endianness = (int) BYTE_ORDER, " "width = (int) { 32, 64 }")
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-float, "
        "channels = (int) 1, "
        "rate = (int) 44100, "
        "endianness = (int) BYTE_ORDER, " "width = (int) { 32, 64 }")
    );

static GstElement *
setup_audiochebband (void)
{
  GstElement *audiochebband;

  GST_DEBUG ("setup_audiochebband");
  audiochebband = gst_check_setup_element ("audiochebband");
  mysrcpad = gst_check_setup_src_pad (audiochebband, &srctemplate, NULL);
  mysinkpad = gst_check_setup_sink_pad (audiochebband, &sinktemplate, NULL);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return audiochebband;
}

static void
cleanup_audiochebband (GstElement * audiochebband)
{
  GST_DEBUG ("cleanup_audiochebband");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (audiochebband);
  gst_check_teardown_sink_pad (audiochebband);
  gst_check_teardown_element (audiochebband);
}

/* Test if data containing only one frequency component
 * at 0 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_32_bp_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is preserved with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_32_bp_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.6);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_32_bp_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_32_br_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is erased with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_32_br_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_32_br_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_64_bp_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is preserved with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_64_bp_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.6);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_64_bp_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_64_br_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is erased with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_64_br_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type1_64_br_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 0.25, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_32_bp_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is preserved with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_32_bp_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.6);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_32_bp_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_32_br_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is erased with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_32_br_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_32_br_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gfloat *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gfloat));
  in = (gfloat *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_32);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gfloat *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_64_bp_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is preserved with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_64_bp_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.6);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is erased with bandpass mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_64_bp_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandpass */
  g_object_set (G_OBJECT (audiochebband), "mode", 0, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at 0 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_64_br_0hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i++)
    in[i] = 1.0;

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at band center is erased with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_64_br_11025hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 4) {
    in[i] = 0.0;
    in[i + 1] = 1.0;
    in[i + 2] = 0.0;
    in[i + 3] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms <= 0.1);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

/* Test if data containing only one frequency component
 * at rate/2 is preserved with bandreject mode and a 
 * 2000Hz frequency band around rate/4 */
GST_START_TEST (test_type2_64_br_22050hz)
{
  GstElement *audiochebband;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  gdouble *in, *res, rms;
  gint i;

  audiochebband = setup_audiochebband ();
  /* Set to bandreject */
  g_object_set (G_OBJECT (audiochebband), "mode", 1, NULL);
  g_object_set (G_OBJECT (audiochebband), "poles", 8, NULL);
  g_object_set (G_OBJECT (audiochebband), "type", 2, NULL);
  g_object_set (G_OBJECT (audiochebband), "ripple", 40.0, NULL);

  fail_unless (gst_element_set_state (audiochebband,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  g_object_set (G_OBJECT (audiochebband), "lower-frequency",
      44100 / 4.0 - 1000, NULL);
  g_object_set (G_OBJECT (audiochebband), "upper-frequency",
      44100 / 4.0 + 1000, NULL);
  inbuffer = gst_buffer_new_and_alloc (1024 * sizeof (gdouble));
  in = (gdouble *) GST_BUFFER_DATA (inbuffer);
  for (i = 0; i < 1024; i += 2) {
    in[i] = 1.0;
    in[i + 1] = -1.0;
  }

  caps = gst_caps_from_string (BUFFER_CAPS_STRING_64);
  gst_buffer_set_caps (inbuffer, caps);
  gst_caps_unref (caps);
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  /* pushing gives away my reference ... */
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  /* ... and puts a new buffer on the global list */
  fail_unless_equals_int (g_list_length (buffers), 1);
  fail_if ((outbuffer = (GstBuffer *) buffers->data) == NULL);

  res = (gdouble *) GST_BUFFER_DATA (outbuffer);

  rms = 0.0;
  for (i = 0; i < 1024; i++)
    rms += res[i] * res[i];
  rms = sqrt (rms / 1024.0);
  fail_unless (rms >= 0.9);

  /* cleanup */
  cleanup_audiochebband (audiochebband);
}

GST_END_TEST;

static Suite *
audiochebband_suite (void)
{
  Suite *s = suite_create ("audiochebband");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_type1_32_bp_0hz);
  tcase_add_test (tc_chain, test_type1_32_bp_11025hz);
  tcase_add_test (tc_chain, test_type1_32_bp_22050hz);
  tcase_add_test (tc_chain, test_type1_32_br_0hz);
  tcase_add_test (tc_chain, test_type1_32_br_11025hz);
  tcase_add_test (tc_chain, test_type1_32_br_22050hz);
  tcase_add_test (tc_chain, test_type1_64_bp_0hz);
  tcase_add_test (tc_chain, test_type1_64_bp_11025hz);
  tcase_add_test (tc_chain, test_type1_64_bp_22050hz);
  tcase_add_test (tc_chain, test_type1_64_br_0hz);
  tcase_add_test (tc_chain, test_type1_64_br_11025hz);
  tcase_add_test (tc_chain, test_type1_64_br_22050hz);
  tcase_add_test (tc_chain, test_type2_32_bp_0hz);
  tcase_add_test (tc_chain, test_type2_32_bp_11025hz);
  tcase_add_test (tc_chain, test_type2_32_bp_22050hz);
  tcase_add_test (tc_chain, test_type2_32_br_0hz);
  tcase_add_test (tc_chain, test_type2_32_br_11025hz);
  tcase_add_test (tc_chain, test_type2_32_br_22050hz);
  tcase_add_test (tc_chain, test_type2_64_bp_0hz);
  tcase_add_test (tc_chain, test_type2_64_bp_11025hz);
  tcase_add_test (tc_chain, test_type2_64_bp_22050hz);
  tcase_add_test (tc_chain, test_type2_64_br_0hz);
  tcase_add_test (tc_chain, test_type2_64_br_11025hz);
  tcase_add_test (tc_chain, test_type2_64_br_22050hz);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = audiochebband_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
