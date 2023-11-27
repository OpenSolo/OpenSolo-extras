/* GStreamer
 * Copyright (C) <2009> Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * OpTV - Optical art meets real-time video effect.
 * Copyright (C) 2004-2005 FUKUCHI Kentaro
 *
 * EffecTV is free software. This library is free software;
 * you can redistribute it and/or
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
 * SECTION:element-optv
 *
 * Traditional black-white optical animation is now resurrected as a
 * real-time video effect. Input images are binarized and combined with
 * various optical pattern.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v videotestsrc ! optv ! ffmpegcolorspace ! autovideosink
 * ]| This pipeline shows the effect of optv on a test stream.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "gstop.h"
#include "gsteffectv.h"

#include <gst/video/video.h>
#include <gst/controller/gstcontroller.h>

enum
{
  OP_SPIRAL1 = 0,
  OP_SPIRAL2,
  OP_PARABOLA,
  OP_HSTRIPE
};

#define GST_TYPE_OPTV_MODE (gst_optv_mode_get_type())
static GType
gst_optv_mode_get_type (void)
{
  static GType type = 0;

  static const GEnumValue enumvalue[] = {
    {OP_SPIRAL1, "Maelstrom", "maelstrom"},
    {OP_SPIRAL2, "Radiation", "radiation"},
    {OP_PARABOLA, "Horizontal Stripes",
        "horizontal-stripes"},
    {OP_HSTRIPE, "Vertical Stripes", "vertical-stripes"},
    {0, NULL, NULL},
  };

  if (!type) {
    type = g_enum_register_static ("GstOpTVMode", enumvalue);
  }
  return type;
}

#define DEFAULT_MODE OP_SPIRAL1
#define DEFAULT_SPEED 16
#define DEFAULT_THRESHOLD 60

enum
{
  PROP_0,
  PROP_MODE,
  PROP_SPEED,
  PROP_THRESHOLD
};

static guint32 palette[256];

GST_BOILERPLATE (GstOpTV, gst_optv, GstVideoFilter, GST_TYPE_VIDEO_FILTER);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS_STR GST_VIDEO_CAPS_BGRx ";" GST_VIDEO_CAPS_RGBx
#else
#define CAPS_STR GST_VIDEO_CAPS_xBGR ";" GST_VIDEO_CAPS_xRGB
#endif

static GstStaticPadTemplate gst_optv_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstStaticPadTemplate gst_optv_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static void
initPalette (void)
{
  gint i;
  guint8 v;

  for (i = 0; i < 112; i++) {
    palette[i] = 0;
    palette[i + 128] = 0xffffff;
  }
  for (i = 0; i < 16; i++) {
    v = 16 * (i + 1) - 1;
    palette[i + 112] = (v << 16) | (v << 8) | v;
    v = 255 - v;
    palette[i + 240] = (v << 16) | (v << 8) | v;
  }
}

static void
setOpmap (gint8 * opmap[4], gint width, gint height)
{
  gint i, j, x, y;
#ifndef PS2
  gdouble xx, yy, r, at, rr;
#else
  gfloat xx, yy, r, at, rr;
#endif
  gint sci;

  sci = 640 / width;
  i = 0;
  for (y = 0; y < height; y++) {
    yy = (gdouble) (y - height / 2) / width;
    for (x = 0; x < width; x++) {
      xx = (gdouble) x / width - 0.5;
#ifndef PS2
      r = sqrt (xx * xx + yy * yy);
      at = atan2 (xx, yy);
#else
      r = sqrtf (xx * xx + yy * yy);
      at = atan2f (xx, yy);
#endif

      opmap[OP_SPIRAL1][i] = ((guint)
          ((at / G_PI * 256) + (r * 4000))) & 255;

      j = r * 300 / 32;
      rr = r * 300 - j * 32;
      j *= 64;
      j += (rr > 28) ? (rr - 28) * 16 : 0;
      opmap[OP_SPIRAL2][i] = ((guint)
          ((at / G_PI * 4096) + (r * 1600) - j)) & 255;

      opmap[OP_PARABOLA][i] =
          ((guint) (yy / (xx * xx * 0.3 + 0.1) * 400)) & 255;
      opmap[OP_HSTRIPE][i] = x * 8 * sci;
      i++;
    }
  }
}

/* Taken from effectv/image.c */
/* Y value filters */
static void
image_y_over (guint32 * src, guint8 * diff, gint y_threshold, gint video_area)
{
  gint i;
  gint R, G, B, v;
  guint8 *p = diff;

  for (i = video_area; i > 0; i--) {
    R = ((*src) & 0xff0000) >> (16 - 1);
    G = ((*src) & 0xff00) >> (8 - 2);
    B = (*src) & 0xff;
    v = y_threshold * 7 - (R + G + B);
    *p = (guint8) (v >> 24);
    src++;
    p++;
  }
}

static GstFlowReturn
gst_optv_transform (GstBaseTransform * trans, GstBuffer * in, GstBuffer * out)
{
  GstOpTV *filter = GST_OPTV (trans);
  guint32 *src, *dest;
  GstFlowReturn ret = GST_FLOW_OK;
  gint8 *p;
  guint8 *diff;
  gint x, y, width, height;
  GstClockTime timestamp, stream_time;
  guint8 phase;

  timestamp = GST_BUFFER_TIMESTAMP (in);
  stream_time =
      gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (G_OBJECT (filter), stream_time);

  src = (guint32 *) GST_BUFFER_DATA (in);
  dest = (guint32 *) GST_BUFFER_DATA (out);

  if (G_UNLIKELY (filter->opmap[0] == NULL))
    return GST_FLOW_NOT_NEGOTIATED;

  GST_OBJECT_LOCK (filter);
  switch (filter->mode) {
    default:
    case 0:
      p = filter->opmap[OP_SPIRAL1];
      break;
    case 1:
      p = filter->opmap[OP_SPIRAL2];
      break;
    case 2:
      p = filter->opmap[OP_PARABOLA];
      break;
    case 3:
      p = filter->opmap[OP_HSTRIPE];
      break;
  }

  filter->phase -= filter->speed;

  diff = filter->diff;
  image_y_over (src, diff, filter->threshold, filter->width * filter->height);
  height = filter->height;
  width = filter->width;
  phase = filter->phase;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      *dest++ = palette[(((guint8) (*p + phase)) ^ *diff++) & 255];
      p++;
    }
  }
  GST_OBJECT_UNLOCK (filter);

  return ret;
}

static gboolean
gst_optv_set_caps (GstBaseTransform * btrans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstOpTV *filter = GST_OPTV (btrans);
  GstStructure *structure;
  gboolean ret = FALSE;

  structure = gst_caps_get_structure (incaps, 0);

  GST_OBJECT_LOCK (filter);
  if (gst_structure_get_int (structure, "width", &filter->width) &&
      gst_structure_get_int (structure, "height", &filter->height)) {
    gint i;

    for (i = 0; i < 4; i++) {
      if (filter->opmap[i])
        g_free (filter->opmap[i]);
      filter->opmap[i] = g_new (gint8, filter->width * filter->height);
    }
    setOpmap (filter->opmap, filter->width, filter->height);

    if (filter->diff)
      g_free (filter->diff);
    filter->diff = g_new (guint8, filter->width * filter->height);

    ret = TRUE;
  }
  GST_OBJECT_UNLOCK (filter);

  return ret;
}

static gboolean
gst_optv_start (GstBaseTransform * trans)
{
  GstOpTV *filter = GST_OPTV (trans);

  filter->phase = 0;

  return TRUE;
}

static void
gst_optv_finalize (GObject * object)
{
  GstOpTV *filter = GST_OPTV (object);

  if (filter->opmap[0]) {
    gint i;

    for (i = 0; i < 4; i++) {
      if (filter->opmap[i])
        g_free (filter->opmap[i]);
      filter->opmap[i] = NULL;
    }
  }

  if (filter->diff)
    g_free (filter->diff);
  filter->diff = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_optv_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstOpTV *filter = GST_OPTV (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_MODE:
      filter->mode = g_value_get_enum (value);
      break;
    case PROP_SPEED:
      filter->speed = g_value_get_int (value);
      break;
    case PROP_THRESHOLD:
      filter->threshold = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_optv_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstOpTV *filter = GST_OPTV (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, filter->mode);
      break;
    case PROP_SPEED:
      g_value_set_int (value, filter->speed);
      break;
    case PROP_THRESHOLD:
      g_value_set_uint (value, filter->threshold);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_optv_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class, "OpTV effect",
      "Filter/Effect/Video",
      "Optical art meets real-time video effect",
      "FUKUCHI, Kentarou <fukuchi@users.sourceforge.net>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  gst_element_class_add_static_pad_template (element_class,
      &gst_optv_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_optv_src_template);
}

static void
gst_optv_class_init (GstOpTVClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;

  gobject_class->set_property = gst_optv_set_property;
  gobject_class->get_property = gst_optv_get_property;

  gobject_class->finalize = gst_optv_finalize;

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Mode", GST_TYPE_OPTV_MODE, DEFAULT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SPEED,
      g_param_spec_int ("speed", "Speed",
          "Effect speed", G_MININT, G_MAXINT, DEFAULT_SPEED,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_THRESHOLD,
      g_param_spec_uint ("threshold", "Threshold",
          "Luma threshold", 0, G_MAXINT, DEFAULT_THRESHOLD,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  trans_class->set_caps = GST_DEBUG_FUNCPTR (gst_optv_set_caps);
  trans_class->transform = GST_DEBUG_FUNCPTR (gst_optv_transform);
  trans_class->start = GST_DEBUG_FUNCPTR (gst_optv_start);

  initPalette ();
}

static void
gst_optv_init (GstOpTV * filter, GstOpTVClass * klass)
{
  filter->speed = DEFAULT_SPEED;
  filter->mode = DEFAULT_MODE;
  filter->threshold = DEFAULT_THRESHOLD;
}
