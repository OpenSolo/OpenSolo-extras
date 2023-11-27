/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
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
 * SECTION:element-audiotestsrc
 *
 * AudioTestSrc can be used to generate basic audio signals. It support several
 * different waveforms and allows to set the base frequency and volume.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch audiotestsrc ! audioconvert ! alsasink
 * ]| This pipeline produces a sine with default frequency, 440 Hz, and the
 * default volume, 0.8 (relative to a maximum 1.0).
 * |[
 * gst-launch audiotestsrc wave=2 freq=200 ! audioconvert ! tee name=t ! queue ! alsasink t. ! queue ! libvisual_lv_scope ! ffmpegcolorspace ! xvimagesink
 * ]| In this example a saw wave is generated. The wave is shown using a
 * scope visualizer from libvisual, allowing you to visually verify that
 * the saw wave is correct.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gst/controller/gstcontroller.h>

#include "gstaudiotestsrc.h"


#define M_PI_M2 ( G_PI + G_PI )

GST_DEBUG_CATEGORY_STATIC (audio_test_src_debug);
#define GST_CAT_DEFAULT audio_test_src_debug

#define DEFAULT_SAMPLES_PER_BUFFER   1024
#define DEFAULT_WAVE                 GST_AUDIO_TEST_SRC_WAVE_SINE
#define DEFAULT_FREQ                 440.0
#define DEFAULT_VOLUME               0.8
#define DEFAULT_IS_LIVE              FALSE
#define DEFAULT_TIMESTAMP_OFFSET     G_GINT64_CONSTANT (0)
#define DEFAULT_CAN_ACTIVATE_PUSH    TRUE
#define DEFAULT_CAN_ACTIVATE_PULL    FALSE

enum
{
  PROP_0,
  PROP_SAMPLES_PER_BUFFER,
  PROP_WAVE,
  PROP_FREQ,
  PROP_VOLUME,
  PROP_IS_LIVE,
  PROP_TIMESTAMP_OFFSET,
  PROP_CAN_ACTIVATE_PUSH,
  PROP_CAN_ACTIVATE_PULL,
  PROP_LAST
};


static GstStaticPadTemplate gst_audio_test_src_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, 2 ]; "
        "audio/x-raw-int, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) true, "
        "width = (int) 32, "
        "depth = (int) 32,"
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, 2 ]; "
        "audio/x-raw-float, "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) { 32, 64 }, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, 2 ]")
    );


GST_BOILERPLATE (GstAudioTestSrc, gst_audio_test_src, GstBaseSrc,
    GST_TYPE_BASE_SRC);

#define GST_TYPE_AUDIO_TEST_SRC_WAVE (gst_audiostestsrc_wave_get_type())
static GType
gst_audiostestsrc_wave_get_type (void)
{
  static GType audiostestsrc_wave_type = 0;
  static const GEnumValue audiostestsrc_waves[] = {
    {GST_AUDIO_TEST_SRC_WAVE_SINE, "Sine", "sine"},
    {GST_AUDIO_TEST_SRC_WAVE_SQUARE, "Square", "square"},
    {GST_AUDIO_TEST_SRC_WAVE_SAW, "Saw", "saw"},
    {GST_AUDIO_TEST_SRC_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GST_AUDIO_TEST_SRC_WAVE_SILENCE, "Silence", "silence"},
    {GST_AUDIO_TEST_SRC_WAVE_WHITE_NOISE, "White uniform noise", "white-noise"},
    {GST_AUDIO_TEST_SRC_WAVE_PINK_NOISE, "Pink noise", "pink-noise"},
    {GST_AUDIO_TEST_SRC_WAVE_SINE_TAB, "Sine table", "sine-table"},
    {GST_AUDIO_TEST_SRC_WAVE_TICKS, "Periodic Ticks", "ticks"},
    {GST_AUDIO_TEST_SRC_WAVE_GAUSSIAN_WHITE_NOISE, "White Gaussian noise",
        "gaussian-noise"},
    {GST_AUDIO_TEST_SRC_WAVE_RED_NOISE, "Red (brownian) noise", "red-noise"},
    {GST_AUDIO_TEST_SRC_WAVE_BLUE_NOISE, "Blue noise", "blue-noise"},
    {GST_AUDIO_TEST_SRC_WAVE_VIOLET_NOISE, "Violet noise", "violet-noise"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (audiostestsrc_wave_type == 0)) {
    audiostestsrc_wave_type = g_enum_register_static ("GstAudioTestSrcWave",
        audiostestsrc_waves);
  }
  return audiostestsrc_wave_type;
}

static void gst_audio_test_src_finalize (GObject * object);

static void gst_audio_test_src_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_audio_test_src_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_audio_test_src_setcaps (GstBaseSrc * basesrc,
    GstCaps * caps);
static void gst_audio_test_src_src_fixate (GstPad * pad, GstCaps * caps);

static gboolean gst_audio_test_src_is_seekable (GstBaseSrc * basesrc);
static gboolean gst_audio_test_src_check_get_range (GstBaseSrc * basesrc);
static gboolean gst_audio_test_src_do_seek (GstBaseSrc * basesrc,
    GstSegment * segment);
static gboolean gst_audio_test_src_query (GstBaseSrc * basesrc,
    GstQuery * query);

static void gst_audio_test_src_change_wave (GstAudioTestSrc * src);

static void gst_audio_test_src_get_times (GstBaseSrc * basesrc,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);
static gboolean gst_audio_test_src_start (GstBaseSrc * basesrc);
static gboolean gst_audio_test_src_stop (GstBaseSrc * basesrc);
static GstFlowReturn gst_audio_test_src_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);


static void
gst_audio_test_src_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class,
      &gst_audio_test_src_src_template);
  gst_element_class_set_details_simple (element_class,
      "Audio test source", "Source/Audio",
      "Creates audio test signals of given frequency and volume",
      "Stefan Kost <ensonic@users.sf.net>");
}

static void
gst_audio_test_src_class_init (GstAudioTestSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_audio_test_src_set_property;
  gobject_class->get_property = gst_audio_test_src_get_property;
  gobject_class->finalize = gst_audio_test_src_finalize;

  g_object_class_install_property (gobject_class, PROP_SAMPLES_PER_BUFFER,
      g_param_spec_int ("samplesperbuffer", "Samples per buffer",
          "Number of samples in each outgoing buffer",
          1, G_MAXINT, DEFAULT_SAMPLES_PER_BUFFER,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GST_TYPE_AUDIO_TEST_SRC_WAVE, GST_AUDIO_TEST_SRC_WAVE_SINE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FREQ,
      g_param_spec_double ("freq", "Frequency", "Frequency of test signal",
          0.0, 20000.0, DEFAULT_FREQ,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of test signal", 0.0,
          1.0, DEFAULT_VOLUME,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "Is Live",
          "Whether to act as a live source", DEFAULT_IS_LIVE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_TIMESTAMP_OFFSET, g_param_spec_int64 ("timestamp-offset",
          "Timestamp offset",
          "An offset added to timestamps set on buffers (in ns)", G_MININT64,
          G_MAXINT64, DEFAULT_TIMESTAMP_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAN_ACTIVATE_PUSH,
      g_param_spec_boolean ("can-activate-push", "Can activate push",
          "Can activate in push mode", DEFAULT_CAN_ACTIVATE_PUSH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CAN_ACTIVATE_PULL,
      g_param_spec_boolean ("can-activate-pull", "Can activate pull",
          "Can activate in pull mode", DEFAULT_CAN_ACTIVATE_PULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_audio_test_src_setcaps);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gst_audio_test_src_is_seekable);
  gstbasesrc_class->check_get_range =
      GST_DEBUG_FUNCPTR (gst_audio_test_src_check_get_range);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_audio_test_src_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_audio_test_src_query);
  gstbasesrc_class->get_times =
      GST_DEBUG_FUNCPTR (gst_audio_test_src_get_times);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_audio_test_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_audio_test_src_stop);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_audio_test_src_create);
}

static void
gst_audio_test_src_init (GstAudioTestSrc * src, GstAudioTestSrcClass * g_class)
{
  GstPad *pad = GST_BASE_SRC_PAD (src);

  gst_pad_set_fixatecaps_function (pad, gst_audio_test_src_src_fixate);

  src->samplerate = 44100;
  src->format = GST_AUDIO_TEST_SRC_FORMAT_NONE;

  src->volume = DEFAULT_VOLUME;
  src->freq = DEFAULT_FREQ;

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), DEFAULT_IS_LIVE);

  src->samples_per_buffer = DEFAULT_SAMPLES_PER_BUFFER;
  src->generate_samples_per_buffer = src->samples_per_buffer;
  src->timestamp_offset = DEFAULT_TIMESTAMP_OFFSET;
  src->can_activate_pull = DEFAULT_CAN_ACTIVATE_PULL;

  src->gen = NULL;

  src->wave = DEFAULT_WAVE;
  gst_base_src_set_blocksize (GST_BASE_SRC (src), -1);
}

static void
gst_audio_test_src_finalize (GObject * object)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (object);

  if (src->gen)
    g_rand_free (src->gen);
  src->gen = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_audio_test_src_src_fixate (GstPad * pad, GstCaps * caps)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (GST_PAD_PARENT (pad));
  const gchar *name;
  GstStructure *structure;

  structure = gst_caps_get_structure (caps, 0);

  GST_DEBUG_OBJECT (src, "fixating samplerate to %d", src->samplerate);

  gst_structure_fixate_field_nearest_int (structure, "rate", src->samplerate);

  name = gst_structure_get_name (structure);
  if (strcmp (name, "audio/x-raw-int") == 0)
    gst_structure_fixate_field_nearest_int (structure, "width", 32);
  else if (strcmp (name, "audio/x-raw-float") == 0)
    gst_structure_fixate_field_nearest_int (structure, "width", 64);

  /* fixate to mono unless downstream requires stereo, for backwards compat */
  gst_structure_fixate_field_nearest_int (structure, "channels", 1);
}

static gboolean
gst_audio_test_src_setcaps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (basesrc);
  const GstStructure *structure;
  const gchar *name;
  gint width;
  gboolean ret;

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "rate", &src->samplerate);

  GST_DEBUG_OBJECT (src, "negotiated to samplerate %d", src->samplerate);

  name = gst_structure_get_name (structure);
  if (strcmp (name, "audio/x-raw-int") == 0) {
    ret &= gst_structure_get_int (structure, "width", &width);
    src->format = (width == 32) ? GST_AUDIO_TEST_SRC_FORMAT_S32 :
        GST_AUDIO_TEST_SRC_FORMAT_S16;
  } else {
    ret &= gst_structure_get_int (structure, "width", &width);
    src->format = (width == 32) ? GST_AUDIO_TEST_SRC_FORMAT_F32 :
        GST_AUDIO_TEST_SRC_FORMAT_F64;
  }

  /* allocate a new buffer suitable for this pad */
  switch (src->format) {
    case GST_AUDIO_TEST_SRC_FORMAT_S16:
      src->sample_size = sizeof (gint16);
      break;
    case GST_AUDIO_TEST_SRC_FORMAT_S32:
      src->sample_size = sizeof (gint32);
      break;
    case GST_AUDIO_TEST_SRC_FORMAT_F32:
      src->sample_size = sizeof (gfloat);
      break;
    case GST_AUDIO_TEST_SRC_FORMAT_F64:
      src->sample_size = sizeof (gdouble);
      break;
    default:
      /* can't really happen */
      ret = FALSE;
      break;
  }

  ret &= gst_structure_get_int (structure, "channels", &src->channels);
  GST_DEBUG_OBJECT (src, "negotiated to %d channels", src->channels);

  gst_audio_test_src_change_wave (src);

  return ret;
}

static gboolean
gst_audio_test_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (basesrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (src_fmt == dest_fmt) {
        dest_val = src_val;
        goto done;
      }

      switch (src_fmt) {
        case GST_FORMAT_DEFAULT:
          switch (dest_fmt) {
            case GST_FORMAT_TIME:
              /* samples to time */
              dest_val =
                  gst_util_uint64_scale_int (src_val, GST_SECOND,
                  src->samplerate);
              break;
            default:
              goto error;
          }
          break;
        case GST_FORMAT_TIME:
          switch (dest_fmt) {
            case GST_FORMAT_DEFAULT:
              /* time to samples */
              dest_val =
                  gst_util_uint64_scale_int (src_val, src->samplerate,
                  GST_SECOND);
              break;
            default:
              goto error;
          }
          break;
        default:
          goto error;
      }
    done:
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      res = TRUE;
      break;
    }
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
      break;
  }

  return res;
  /* ERROR */
error:
  {
    GST_DEBUG_OBJECT (src, "query failed");
    return FALSE;
  }
}

#define DEFINE_SINE(type,scale) \
static void \
gst_audio_test_src_create_sine_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble step, amp; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  amp = src->volume * scale; \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    for (c = 0; c < src->channels; ++c) { \
      samples[i++] = (g##type) (sin (src->accumulator) * amp); \
    } \
  } \
}

DEFINE_SINE (int16, 32767.0);
DEFINE_SINE (int32, 2147483647.0);
DEFINE_SINE (float, 1.0);
DEFINE_SINE (double, 1.0);

static const ProcessFunc sine_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_sine_int16,
  (ProcessFunc) gst_audio_test_src_create_sine_int32,
  (ProcessFunc) gst_audio_test_src_create_sine_float,
  (ProcessFunc) gst_audio_test_src_create_sine_double
};

#define DEFINE_SQUARE(type,scale) \
static void \
gst_audio_test_src_create_square_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble step, amp; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  amp = src->volume * scale; \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    for (c = 0; c < src->channels; ++c) { \
      samples[i++] = (g##type) ((src->accumulator < G_PI) ? amp : -amp); \
    } \
  } \
}

DEFINE_SQUARE (int16, 32767.0);
DEFINE_SQUARE (int32, 2147483647.0);
DEFINE_SQUARE (float, 1.0);
DEFINE_SQUARE (double, 1.0);

static const ProcessFunc square_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_square_int16,
  (ProcessFunc) gst_audio_test_src_create_square_int32,
  (ProcessFunc) gst_audio_test_src_create_square_float,
  (ProcessFunc) gst_audio_test_src_create_square_double
};

#define DEFINE_SAW(type,scale) \
static void \
gst_audio_test_src_create_saw_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble step, amp; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  amp = (src->volume * scale) / G_PI; \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    if (src->accumulator < G_PI) { \
      for (c = 0; c < src->channels; ++c) \
        samples[i++] = (g##type) (src->accumulator * amp); \
    } else { \
      for (c = 0; c < src->channels; ++c) \
        samples[i++] = (g##type) ((M_PI_M2 - src->accumulator) * -amp); \
    } \
  } \
}

DEFINE_SAW (int16, 32767.0);
DEFINE_SAW (int32, 2147483647.0);
DEFINE_SAW (float, 1.0);
DEFINE_SAW (double, 1.0);

static const ProcessFunc saw_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_saw_int16,
  (ProcessFunc) gst_audio_test_src_create_saw_int32,
  (ProcessFunc) gst_audio_test_src_create_saw_float,
  (ProcessFunc) gst_audio_test_src_create_saw_double
};

#define DEFINE_TRIANGLE(type,scale) \
static void \
gst_audio_test_src_create_triangle_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble step, amp; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  amp = (src->volume * scale) / G_PI_2; \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    if (src->accumulator < (G_PI_2)) { \
      for (c = 0; c < src->channels; ++c) \
        samples[i++] = (g##type) (src->accumulator * amp); \
    } else if (src->accumulator < (G_PI * 1.5)) { \
      for (c = 0; c < src->channels; ++c) \
        samples[i++] = (g##type) ((src->accumulator - G_PI) * -amp); \
    } else { \
      for (c = 0; c < src->channels; ++c) \
        samples[i++] = (g##type) ((M_PI_M2 - src->accumulator) * -amp); \
    } \
  } \
}

DEFINE_TRIANGLE (int16, 32767.0);
DEFINE_TRIANGLE (int32, 2147483647.0);
DEFINE_TRIANGLE (float, 1.0);
DEFINE_TRIANGLE (double, 1.0);

static const ProcessFunc triangle_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_triangle_int16,
  (ProcessFunc) gst_audio_test_src_create_triangle_int32,
  (ProcessFunc) gst_audio_test_src_create_triangle_float,
  (ProcessFunc) gst_audio_test_src_create_triangle_double
};

#define DEFINE_SILENCE(type) \
static void \
gst_audio_test_src_create_silence_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  memset (samples, 0, src->generate_samples_per_buffer * sizeof (g##type) * src->channels); \
}

DEFINE_SILENCE (int16);
DEFINE_SILENCE (int32);
DEFINE_SILENCE (float);
DEFINE_SILENCE (double);

static const ProcessFunc silence_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_silence_int16,
  (ProcessFunc) gst_audio_test_src_create_silence_int32,
  (ProcessFunc) gst_audio_test_src_create_silence_float,
  (ProcessFunc) gst_audio_test_src_create_silence_double
};

#define DEFINE_WHITE_NOISE(type,scale) \
static void \
gst_audio_test_src_create_white_noise_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble amp = (src->volume * scale); \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    for (c = 0; c < src->channels; ++c) \
      samples[i++] = (g##type) (amp * g_rand_double_range (src->gen, -1.0, 1.0)); \
  } \
}

DEFINE_WHITE_NOISE (int16, 32767.0);
DEFINE_WHITE_NOISE (int32, 2147483647.0);
DEFINE_WHITE_NOISE (float, 1.0);
DEFINE_WHITE_NOISE (double, 1.0);

static const ProcessFunc white_noise_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_white_noise_int16,
  (ProcessFunc) gst_audio_test_src_create_white_noise_int32,
  (ProcessFunc) gst_audio_test_src_create_white_noise_float,
  (ProcessFunc) gst_audio_test_src_create_white_noise_double
};

/* pink noise calculation is based on
 * http://www.firstpr.com.au/dsp/pink-noise/phil_burk_19990905_patest_pink.c
 * which has been released under public domain
 * Many thanks Phil!
 */
static void
gst_audio_test_src_init_pink_noise (GstAudioTestSrc * src)
{
  gint i;
  gint num_rows = 12;           /* arbitrary: 1 .. PINK_MAX_RANDOM_ROWS */
  glong pmax;

  src->pink.index = 0;
  src->pink.index_mask = (1 << num_rows) - 1;
  /* calculate maximum possible signed random value.
   * Extra 1 for white noise always added. */
  pmax = (num_rows + 1) * (1 << (PINK_RANDOM_BITS - 1));
  src->pink.scalar = 1.0f / pmax;
  /* Initialize rows. */
  for (i = 0; i < num_rows; i++)
    src->pink.rows[i] = 0;
  src->pink.running_sum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
static gdouble
gst_audio_test_src_generate_pink_noise_value (GstAudioTestSrc * src)
{
  GstPinkNoise *pink = &src->pink;
  glong new_random;
  glong sum;

  /* Increment and mask index. */
  pink->index = (pink->index + 1) & pink->index_mask;

  /* If index is zero, don't update any random values. */
  if (pink->index != 0) {
    /* Determine how many trailing zeros in PinkIndex. */
    /* This algorithm will hang if n==0 so test first. */
    gint num_zeros = 0;
    gint n = pink->index;

    while ((n & 1) == 0) {
      n = n >> 1;
      num_zeros++;
    }

    /* Replace the indexed ROWS random value.
     * Subtract and add back to RunningSum instead of adding all the random
     * values together. Only one changes each time.
     */
    pink->running_sum -= pink->rows[num_zeros];
    new_random = 32768.0 - (65536.0 * (gulong) g_rand_int (src->gen)
        / (G_MAXUINT32 + 1.0));
    pink->running_sum += new_random;
    pink->rows[num_zeros] = new_random;
  }

  /* Add extra white noise value. */
  new_random = 32768.0 - (65536.0 * (gulong) g_rand_int (src->gen)
      / (G_MAXUINT32 + 1.0));
  sum = pink->running_sum + new_random;

  /* Scale to range of -1.0 to 0.9999. */
  return (pink->scalar * sum);
}

#define DEFINE_PINK(type, scale) \
static void \
gst_audio_test_src_create_pink_noise_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble amp; \
  \
  amp = src->volume * scale; \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    for (c = 0; c < src->channels; ++c) { \
      samples[i++] = \
        (g##type) (gst_audio_test_src_generate_pink_noise_value (src) * \
        amp); \
    } \
  } \
}

DEFINE_PINK (int16, 32767.0);
DEFINE_PINK (int32, 2147483647.0);
DEFINE_PINK (float, 1.0);
DEFINE_PINK (double, 1.0);

static const ProcessFunc pink_noise_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_pink_noise_int16,
  (ProcessFunc) gst_audio_test_src_create_pink_noise_int32,
  (ProcessFunc) gst_audio_test_src_create_pink_noise_float,
  (ProcessFunc) gst_audio_test_src_create_pink_noise_double
};

static void
gst_audio_test_src_init_sine_table (GstAudioTestSrc * src)
{
  gint i;
  gdouble ang = 0.0;
  gdouble step = M_PI_M2 / 1024.0;
  gdouble amp = src->volume;

  for (i = 0; i < 1024; i++) {
    src->wave_table[i] = sin (ang) * amp;
    ang += step;
  }
}

#define DEFINE_SINE_TABLE(type,scale) \
static void \
gst_audio_test_src_create_sine_table_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble step, scl; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  scl = 1024.0 / M_PI_M2; \
  \
  i = 0; \
  while (i < (src->generate_samples_per_buffer * src->channels)) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    for (c = 0; c < src->channels; ++c) \
      samples[i++] = (g##type) scale * src->wave_table[(gint) (src->accumulator * scl)]; \
  } \
}

DEFINE_SINE_TABLE (int16, 32767.0);
DEFINE_SINE_TABLE (int32, 2147483647.0);
DEFINE_SINE_TABLE (float, 1.0);
DEFINE_SINE_TABLE (double, 1.0);

static const ProcessFunc sine_table_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_sine_table_int16,
  (ProcessFunc) gst_audio_test_src_create_sine_table_int32,
  (ProcessFunc) gst_audio_test_src_create_sine_table_float,
  (ProcessFunc) gst_audio_test_src_create_sine_table_double
};

#define DEFINE_TICKS(type,scale) \
static void \
gst_audio_test_src_create_tick_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble step, scl; \
  \
  step = M_PI_M2 * src->freq / src->samplerate; \
  scl = 1024.0 / M_PI_M2; \
  \
  for (i = 0; i < src->generate_samples_per_buffer; i++) { \
    src->accumulator += step; \
    if (src->accumulator >= M_PI_M2) \
      src->accumulator -= M_PI_M2; \
    \
    if ((src->next_sample + i)%src->samplerate < 1600) { \
      for (c = 0; c < src->channels; ++c) \
        samples[(i * src->channels) + c] = (g##type) scale * src->wave_table[(gint) (src->accumulator * scl)]; \
    } else { \
      for (c = 0; c < src->channels; ++c) \
        samples[(i * src->channels) + c] = 0; \
    } \
  } \
}

DEFINE_TICKS (int16, 32767.0);
DEFINE_TICKS (int32, 2147483647.0);
DEFINE_TICKS (float, 1.0);
DEFINE_TICKS (double, 1.0);

static const ProcessFunc tick_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_tick_int16,
  (ProcessFunc) gst_audio_test_src_create_tick_int32,
  (ProcessFunc) gst_audio_test_src_create_tick_float,
  (ProcessFunc) gst_audio_test_src_create_tick_double
};

/* Gaussian white noise using Box-Muller algorithm.  unit variance
 * normally-distributed random numbers are generated in pairs as the real
 * and imaginary parts of a compex random variable with
 * uniformly-distributed argument and \chi^{2}-distributed modulus.
 */

#define DEFINE_GAUSSIAN_WHITE_NOISE(type,scale) \
static void \
gst_audio_test_src_create_gaussian_white_noise_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble amp = (src->volume * scale); \
  \
  for (i = 0; i < src->generate_samples_per_buffer * src->channels; ) { \
    for (c = 0; c < src->channels; ++c) { \
      gdouble mag = sqrt (-2 * log (1.0 - g_rand_double (src->gen))); \
      gdouble phs = g_rand_double_range (src->gen, 0.0, M_PI_M2); \
      \
      samples[i++] = (g##type) (amp * mag * cos (phs)); \
      if (++c >= src->channels) \
        break; \
      samples[i++] = (g##type) (amp * mag * sin (phs)); \
    } \
  } \
}

DEFINE_GAUSSIAN_WHITE_NOISE (int16, 32767.0);
DEFINE_GAUSSIAN_WHITE_NOISE (int32, 2147483647.0);
DEFINE_GAUSSIAN_WHITE_NOISE (float, 1.0);
DEFINE_GAUSSIAN_WHITE_NOISE (double, 1.0);

static const ProcessFunc gaussian_white_noise_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_gaussian_white_noise_int16,
  (ProcessFunc) gst_audio_test_src_create_gaussian_white_noise_int32,
  (ProcessFunc) gst_audio_test_src_create_gaussian_white_noise_float,
  (ProcessFunc) gst_audio_test_src_create_gaussian_white_noise_double
};

/* Brownian (Red) Noise: noise where the power density decreases by 6 dB per
 * octave with increasing frequency
 *
 * taken from http://vellocet.com/dsp/noise/VRand.html
 * by Andrew Simper of Vellocet (andy@vellocet.com)
 */

#define DEFINE_RED_NOISE(type,scale) \
static void \
gst_audio_test_src_create_red_noise_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  gdouble amp = (src->volume * scale); \
  gdouble state = src->red.state; \
  \
  for (i = 0; i < src->generate_samples_per_buffer * src->channels; ) { \
    for (c = 0; c < src->channels; ++c) { \
      while (TRUE) { \
        gdouble r = g_rand_double_range (src->gen, -1.0, 1.0); \
        state += r; \
        if (state < -8.0f || state > 8.0f) state -= r; \
        else break; \
      } \
      samples[i++] = (g##type) (amp * state * 0.0625f); /* /16.0 */ \
    } \
  } \
  src->red.state = state; \
}

DEFINE_RED_NOISE (int16, 32767.0);
DEFINE_RED_NOISE (int32, 2147483647.0);
DEFINE_RED_NOISE (float, 1.0);
DEFINE_RED_NOISE (double, 1.0);

static const ProcessFunc red_noise_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_red_noise_int16,
  (ProcessFunc) gst_audio_test_src_create_red_noise_int32,
  (ProcessFunc) gst_audio_test_src_create_red_noise_float,
  (ProcessFunc) gst_audio_test_src_create_red_noise_double
};

/* Blue Noise: apply spectral inversion to pink noise */

#define DEFINE_BLUE_NOISE(type) \
static void \
gst_audio_test_src_create_blue_noise_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  static gdouble flip=1.0; \
  \
  gst_audio_test_src_create_pink_noise_##type (src, samples); \
  for (i = 0; i < src->generate_samples_per_buffer * src->channels; ) { \
    for (c = 0; c < src->channels; ++c) { \
      samples[i++] *= flip; \
    } \
    flip *= -1.0; \
  } \
}

DEFINE_BLUE_NOISE (int16);
DEFINE_BLUE_NOISE (int32);
DEFINE_BLUE_NOISE (float);
DEFINE_BLUE_NOISE (double);

static const ProcessFunc blue_noise_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_blue_noise_int16,
  (ProcessFunc) gst_audio_test_src_create_blue_noise_int32,
  (ProcessFunc) gst_audio_test_src_create_blue_noise_float,
  (ProcessFunc) gst_audio_test_src_create_blue_noise_double
};


/* Violet Noise: apply spectral inversion to red noise */

#define DEFINE_VIOLET_NOISE(type) \
static void \
gst_audio_test_src_create_violet_noise_##type (GstAudioTestSrc * src, g##type * samples) \
{ \
  gint i, c; \
  static gdouble flip=1.0; \
  \
  gst_audio_test_src_create_red_noise_##type (src, samples); \
  for (i = 0; i < src->generate_samples_per_buffer * src->channels; ) { \
    for (c = 0; c < src->channels; ++c) { \
      samples[i++] *= flip; \
    } \
    flip *= -1.0; \
  } \
}

DEFINE_VIOLET_NOISE (int16);
DEFINE_VIOLET_NOISE (int32);
DEFINE_VIOLET_NOISE (float);
DEFINE_VIOLET_NOISE (double);

static const ProcessFunc violet_noise_funcs[] = {
  (ProcessFunc) gst_audio_test_src_create_violet_noise_int16,
  (ProcessFunc) gst_audio_test_src_create_violet_noise_int32,
  (ProcessFunc) gst_audio_test_src_create_violet_noise_float,
  (ProcessFunc) gst_audio_test_src_create_violet_noise_double
};


/*
 * gst_audio_test_src_change_wave:
 * Assign function pointer of wave genrator.
 */
static void
gst_audio_test_src_change_wave (GstAudioTestSrc * src)
{
  if (src->format == -1) {
    src->process = NULL;
    return;
  }

  switch (src->wave) {
    case GST_AUDIO_TEST_SRC_WAVE_SINE:
      src->process = sine_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_SQUARE:
      src->process = square_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_SAW:
      src->process = saw_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_TRIANGLE:
      src->process = triangle_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_SILENCE:
      src->process = silence_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_WHITE_NOISE:
      if (!(src->gen))
        src->gen = g_rand_new ();
      src->process = white_noise_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_PINK_NOISE:
      if (!(src->gen))
        src->gen = g_rand_new ();
      gst_audio_test_src_init_pink_noise (src);
      src->process = pink_noise_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_SINE_TAB:
      gst_audio_test_src_init_sine_table (src);
      src->process = sine_table_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_TICKS:
      gst_audio_test_src_init_sine_table (src);
      src->process = tick_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_GAUSSIAN_WHITE_NOISE:
      if (!(src->gen))
        src->gen = g_rand_new ();
      src->process = gaussian_white_noise_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_RED_NOISE:
      if (!(src->gen))
        src->gen = g_rand_new ();
      src->red.state = 0.0;
      src->process = red_noise_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_BLUE_NOISE:
      if (!(src->gen))
        src->gen = g_rand_new ();
      gst_audio_test_src_init_pink_noise (src);
      src->process = blue_noise_funcs[src->format];
      break;
    case GST_AUDIO_TEST_SRC_WAVE_VIOLET_NOISE:
      if (!(src->gen))
        src->gen = g_rand_new ();
      src->red.state = 0.0;
      src->process = violet_noise_funcs[src->format];
      break;
    default:
      GST_ERROR ("invalid wave-form");
      break;
  }
}

/*
 * gst_audio_test_src_change_volume:
 * Recalc wave tables for precalculated waves.
 */
static void
gst_audio_test_src_change_volume (GstAudioTestSrc * src)
{
  switch (src->wave) {
    case GST_AUDIO_TEST_SRC_WAVE_SINE_TAB:
      gst_audio_test_src_init_sine_table (src);
      break;
    default:
      break;
  }
}

static void
gst_audio_test_src_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  /* for live sources, sync on the timestamp of the buffer */
  if (gst_base_src_is_live (basesrc)) {
    GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

    if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
      /* get duration to calculate end time */
      GstClockTime duration = GST_BUFFER_DURATION (buffer);

      if (GST_CLOCK_TIME_IS_VALID (duration)) {
        *end = timestamp + duration;
      }
      *start = timestamp;
    }
  } else {
    *start = -1;
    *end = -1;
  }
}

static gboolean
gst_audio_test_src_start (GstBaseSrc * basesrc)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (basesrc);

  src->next_sample = 0;
  src->next_byte = 0;
  src->next_time = 0;
  src->check_seek_stop = FALSE;
  src->eos_reached = FALSE;
  src->tags_pushed = FALSE;
  src->accumulator = 0;

  return TRUE;
}

static gboolean
gst_audio_test_src_stop (GstBaseSrc * basesrc)
{
  return TRUE;
}

/* seek to time, will be called when we operate in push mode. In pull mode we
 * get the requested byte offset. */
static gboolean
gst_audio_test_src_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (basesrc);
  GstClockTime time;

  GST_DEBUG_OBJECT (src, "seeking %" GST_SEGMENT_FORMAT, segment);

  time = segment->last_stop;
  src->reverse = (segment->rate < 0.0);

  /* now move to the time indicated */
  src->next_sample =
      gst_util_uint64_scale_int (time, src->samplerate, GST_SECOND);
  src->next_byte = src->next_sample * src->sample_size * src->channels;
  src->next_time =
      gst_util_uint64_scale_int (src->next_sample, GST_SECOND, src->samplerate);

  GST_DEBUG_OBJECT (src, "seeking next_sample=%" G_GINT64_FORMAT
      " next_time=%" GST_TIME_FORMAT, src->next_sample,
      GST_TIME_ARGS (src->next_time));

  g_assert (src->next_time <= time);

  if (!src->reverse) {
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      segment->time = segment->start;
    }
  } else {
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      segment->time = segment->stop;
    }
  }

  if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
    time = segment->stop;
    src->sample_stop = gst_util_uint64_scale_int (time, src->samplerate,
        GST_SECOND);
    src->check_seek_stop = TRUE;
  } else {
    src->check_seek_stop = FALSE;
  }
  src->eos_reached = FALSE;

  return TRUE;
}

static gboolean
gst_audio_test_src_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static gboolean
gst_audio_test_src_check_get_range (GstBaseSrc * basesrc)
{
  GstAudioTestSrc *src;

  src = GST_AUDIO_TEST_SRC (basesrc);

  /* if we can operate in pull mode */
  return src->can_activate_pull;
}

static GstFlowReturn
gst_audio_test_src_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstFlowReturn res;
  GstAudioTestSrc *src;
  GstBuffer *buf;
  GstClockTime next_time;
  gint64 next_sample, next_byte;
  gint bytes, samples;
  GstElementClass *eclass;

  src = GST_AUDIO_TEST_SRC (basesrc);

  /* example for tagging generated data */
  if (!src->tags_pushed) {
    GstTagList *taglist;

    taglist = gst_tag_list_new ();

    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND,
        GST_TAG_DESCRIPTION, "audiotest wave", NULL);

    eclass = GST_ELEMENT_CLASS (parent_class);
    if (eclass->send_event)
      eclass->send_event (GST_ELEMENT_CAST (basesrc),
          gst_event_new_tag (taglist));
    else
      gst_tag_list_free (taglist);
    src->tags_pushed = TRUE;
  }

  if (src->eos_reached) {
    GST_INFO_OBJECT (src, "eos");
    return GST_FLOW_UNEXPECTED;
  }

  /* if no length was given, use our default length in samples otherwise convert
   * the length in bytes to samples. */
  if (length == -1)
    samples = src->samples_per_buffer;
  else
    samples = length / (src->sample_size * src->channels);

  /* if no offset was given, use our next logical byte */
  if (offset == -1)
    offset = src->next_byte;

  /* now see if we are at the byteoffset we think we are */
  if (offset != src->next_byte) {
    GST_DEBUG_OBJECT (src, "seek to new offset %" G_GUINT64_FORMAT, offset);
    /* we have a discont in the expected sample offset, do a 'seek' */
    src->next_sample = offset / (src->sample_size * src->channels);
    src->next_time =
        gst_util_uint64_scale_int (src->next_sample, GST_SECOND,
        src->samplerate);
    src->next_byte = offset;
  }

  /* check for eos */
  if (src->check_seek_stop &&
      (src->sample_stop > src->next_sample) &&
      (src->sample_stop < src->next_sample + samples)
      ) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer = src->sample_stop - src->next_sample;
    next_sample = src->sample_stop;
    src->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples;
    next_sample = src->next_sample + (src->reverse ? (-samples) : samples);
  }

  bytes = src->generate_samples_per_buffer * src->sample_size * src->channels;

  if ((res = gst_pad_alloc_buffer (basesrc->srcpad, src->next_sample,
              bytes, GST_PAD_CAPS (basesrc->srcpad), &buf)) != GST_FLOW_OK) {
    return res;
  }

  next_byte = src->next_byte + (src->reverse ? (-bytes) : bytes);
  next_time = gst_util_uint64_scale_int (next_sample, GST_SECOND,
      src->samplerate);

  GST_LOG_OBJECT (src, "samplerate %d", src->samplerate);
  GST_LOG_OBJECT (src, "next_sample %" G_GINT64_FORMAT ", ts %" GST_TIME_FORMAT,
      next_sample, GST_TIME_ARGS (next_time));

  GST_BUFFER_OFFSET (buf) = src->next_sample;
  GST_BUFFER_OFFSET_END (buf) = next_sample;
  if (!src->reverse) {
    GST_BUFFER_TIMESTAMP (buf) = src->timestamp_offset + src->next_time;
    GST_BUFFER_DURATION (buf) = next_time - src->next_time;
  } else {
    GST_BUFFER_TIMESTAMP (buf) = src->timestamp_offset + next_time;
    GST_BUFFER_DURATION (buf) = src->next_time - next_time;
  }

  gst_object_sync_values (G_OBJECT (src), GST_BUFFER_TIMESTAMP (buf));

  src->next_time = next_time;
  src->next_sample = next_sample;
  src->next_byte = next_byte;

  GST_LOG_OBJECT (src, "generating %u samples at ts %" GST_TIME_FORMAT,
      src->generate_samples_per_buffer,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

  src->process (src, GST_BUFFER_DATA (buf));

  if (G_UNLIKELY ((src->wave == GST_AUDIO_TEST_SRC_WAVE_SILENCE)
          || (src->volume == 0.0))) {
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_GAP);
  }

  *buffer = buf;

  return GST_FLOW_OK;
}

static void
gst_audio_test_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (object);

  switch (prop_id) {
    case PROP_SAMPLES_PER_BUFFER:
      src->samples_per_buffer = g_value_get_int (value);
      break;
    case PROP_WAVE:
      src->wave = g_value_get_enum (value);
      gst_audio_test_src_change_wave (src);
      break;
    case PROP_FREQ:
      src->freq = g_value_get_double (value);
      break;
    case PROP_VOLUME:
      src->volume = g_value_get_double (value);
      gst_audio_test_src_change_volume (src);
      break;
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (src), g_value_get_boolean (value));
      break;
    case PROP_TIMESTAMP_OFFSET:
      src->timestamp_offset = g_value_get_int64 (value);
      break;
    case PROP_CAN_ACTIVATE_PUSH:
      GST_BASE_SRC (src)->can_activate_push = g_value_get_boolean (value);
      break;
    case PROP_CAN_ACTIVATE_PULL:
      src->can_activate_pull = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_test_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioTestSrc *src = GST_AUDIO_TEST_SRC (object);

  switch (prop_id) {
    case PROP_SAMPLES_PER_BUFFER:
      g_value_set_int (value, src->samples_per_buffer);
      break;
    case PROP_WAVE:
      g_value_set_enum (value, src->wave);
      break;
    case PROP_FREQ:
      g_value_set_double (value, src->freq);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, src->volume);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (src)));
      break;
    case PROP_TIMESTAMP_OFFSET:
      g_value_set_int64 (value, src->timestamp_offset);
      break;
    case PROP_CAN_ACTIVATE_PUSH:
      g_value_set_boolean (value, GST_BASE_SRC (src)->can_activate_push);
      break;
    case PROP_CAN_ACTIVATE_PULL:
      g_value_set_boolean (value, src->can_activate_pull);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  /* initialize gst controller library */
  gst_controller_init (NULL, NULL);

  GST_DEBUG_CATEGORY_INIT (audio_test_src_debug, "audiotestsrc", 0,
      "Audio Test Source");

  return gst_element_register (plugin, "audiotestsrc",
      GST_RANK_NONE, GST_TYPE_AUDIO_TEST_SRC);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "audiotestsrc",
    "Creates audio test signals of given frequency and volume",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
