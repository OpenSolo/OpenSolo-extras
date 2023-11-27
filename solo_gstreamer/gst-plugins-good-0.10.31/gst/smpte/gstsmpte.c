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

/**
 * SECTION:element-smpte
 *
 * smpte can accept I420 video streams with the same width, height and
 * framerate. The two incomming buffers are blended together using an effect
 * specific alpha mask. 
 *
 * The #GstSmpte:depth property defines the presision in bits of the mask. A
 * higher presision will create a mask with smoother gradients in order to avoid
 * banding.
 *
 * <refsect2>
 * <title>Sample pipelines</title>
 * |[
 * gst-launch -v videotestsrc pattern=1 ! smpte name=s border=20000 type=234 duration=2000000000 ! ffmpegcolorspace ! ximagesink videotestsrc ! s.
 * ]| A pipeline to demonstrate the smpte transition.
 * It shows a pinwheel transition a from a snow videotestsrc to an smpte
 * pattern videotestsrc. The transition will take 2 seconds to complete. The
 * edges of the transition are smoothed with a 20000 big border.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include "gstsmpte.h"
#include <gst/video/video.h>
#include "paint.h"

GST_DEBUG_CATEGORY_STATIC (gst_smpte_debug);
#define GST_CAT_DEFAULT gst_smpte_debug

static GstStaticPadTemplate gst_smpte_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("I420")
    )
    );

static GstStaticPadTemplate gst_smpte_sink1_template =
GST_STATIC_PAD_TEMPLATE ("sink1",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("I420")
    )
    );

static GstStaticPadTemplate gst_smpte_sink2_template =
GST_STATIC_PAD_TEMPLATE ("sink2",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("I420")
    )
    );


/* SMPTE signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_PROP_TYPE	1
#define DEFAULT_PROP_BORDER	0
#define DEFAULT_PROP_DEPTH	16
#define DEFAULT_PROP_FPS	0.
#define DEFAULT_PROP_DURATION	GST_SECOND
#define DEFAULT_PROP_INVERT   FALSE

enum
{
  PROP_0,
  PROP_TYPE,
  PROP_BORDER,
  PROP_DEPTH,
  PROP_FPS,
  PROP_DURATION,
  PROP_INVERT,
  PROP_LAST,
};

#define I420_Y_ROWSTRIDE(width) (GST_ROUND_UP_4(width))
#define I420_U_ROWSTRIDE(width) (GST_ROUND_UP_8(width)/2)
#define I420_V_ROWSTRIDE(width) ((GST_ROUND_UP_8(I420_Y_ROWSTRIDE(width)))/2)

#define I420_Y_OFFSET(w,h) (0)
#define I420_U_OFFSET(w,h) (I420_Y_OFFSET(w,h)+(I420_Y_ROWSTRIDE(w)*GST_ROUND_UP_2(h)))
#define I420_V_OFFSET(w,h) (I420_U_OFFSET(w,h)+(I420_U_ROWSTRIDE(w)*GST_ROUND_UP_2(h)/2))

#define I420_SIZE(w,h)     (I420_V_OFFSET(w,h)+(I420_V_ROWSTRIDE(w)*GST_ROUND_UP_2(h)/2))


#define GST_TYPE_SMPTE_TRANSITION_TYPE (gst_smpte_transition_type_get_type())
static GType
gst_smpte_transition_type_get_type (void)
{
  static GType smpte_transition_type = 0;
  GEnumValue *smpte_transitions;

  if (!smpte_transition_type) {
    const GList *definitions;
    gint i = 0;

    definitions = gst_mask_get_definitions ();
    smpte_transitions =
        g_new0 (GEnumValue, g_list_length ((GList *) definitions) + 1);

    while (definitions) {
      GstMaskDefinition *definition = (GstMaskDefinition *) definitions->data;

      definitions = g_list_next (definitions);

      smpte_transitions[i].value = definition->type;
      /* older GLib versions have the two fields as non-const, hence the cast */
      smpte_transitions[i].value_nick = (gchar *) definition->short_name;
      smpte_transitions[i].value_name = (gchar *) definition->long_name;

      i++;
    }

    smpte_transition_type =
        g_enum_register_static ("GstSMPTETransitionType", smpte_transitions);
  }
  return smpte_transition_type;
}


static void gst_smpte_class_init (GstSMPTEClass * klass);
static void gst_smpte_base_init (GstSMPTEClass * klass);
static void gst_smpte_init (GstSMPTE * smpte);
static void gst_smpte_finalize (GstSMPTE * smpte);

static GstFlowReturn gst_smpte_collected (GstCollectPads * pads,
    GstSMPTE * smpte);

static void gst_smpte_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_smpte_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_smpte_change_state (GstElement * element,
    GstStateChange transition);

static GstElementClass *parent_class = NULL;

/*static guint gst_smpte_signals[LAST_SIGNAL] = { 0 }; */

static GType
gst_smpte_get_type (void)
{
  static GType smpte_type = 0;

  if (!smpte_type) {
    static const GTypeInfo smpte_info = {
      sizeof (GstSMPTEClass),
      (GBaseInitFunc) gst_smpte_base_init,
      NULL,
      (GClassInitFunc) gst_smpte_class_init,
      NULL,
      NULL,
      sizeof (GstSMPTE),
      0,
      (GInstanceInitFunc) gst_smpte_init,
    };

    smpte_type =
        g_type_register_static (GST_TYPE_ELEMENT, "GstSMPTE", &smpte_info, 0);
  }
  return smpte_type;
}

static void
gst_smpte_base_init (GstSMPTEClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_smpte_sink1_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_smpte_sink2_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_smpte_src_template);
  gst_element_class_set_details_simple (element_class, "SMPTE transitions",
      "Filter/Editor/Video",
      "Apply the standard SMPTE transitions on video images",
      "Wim Taymans <wim.taymans@chello.be>");
}

static void
gst_smpte_class_init (GstSMPTEClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_smpte_set_property;
  gobject_class->get_property = gst_smpte_get_property;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_smpte_finalize;

  _gst_mask_init ();

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TYPE,
      g_param_spec_enum ("type", "Type", "The type of transition to use",
          GST_TYPE_SMPTE_TRANSITION_TYPE, DEFAULT_PROP_TYPE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FPS,
      g_param_spec_float ("fps", "FPS",
          "Frames per second if no input files are given (deprecated)", 0.,
          G_MAXFLOAT, DEFAULT_PROP_FPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BORDER,
      g_param_spec_int ("border", "Border",
          "The border width of the transition", 0, G_MAXINT,
          DEFAULT_PROP_BORDER, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DEPTH,
      g_param_spec_int ("depth", "Depth", "Depth of the mask in bits", 1, 24,
          DEFAULT_PROP_DEPTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DURATION,
      g_param_spec_uint64 ("duration", "Duration",
          "Duration of the transition effect in nanoseconds", 0, G_MAXUINT64,
          DEFAULT_PROP_DURATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INVERT,
      g_param_spec_boolean ("invert", "Invert",
          "Invert transition mask", DEFAULT_PROP_INVERT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_smpte_change_state);
}

/*                              wht  yel  cya  grn  mag  red  blu  blk   -I    Q */
static const int y_colors[] = { 255, 226, 179, 150, 105, 76, 29, 16, 16, 0 };
static const int u_colors[] = { 128, 0, 170, 46, 212, 85, 255, 128, 0, 128 };
static const int v_colors[] = { 128, 155, 0, 21, 235, 255, 107, 128, 128, 255 };

static void
fill_i420 (guint8 * data, gint width, gint height, gint color)
{
  gint size = I420_Y_ROWSTRIDE (width) * GST_ROUND_UP_2 (height);
  gint size4 = size >> 2;
  guint8 *yp = data;
  guint8 *up = data + I420_U_OFFSET (width, height);
  guint8 *vp = data + I420_V_OFFSET (width, height);

  memset (yp, y_colors[color], size);
  memset (up, u_colors[color], size4);
  memset (vp, v_colors[color], size4);
}

static gboolean
gst_smpte_update_mask (GstSMPTE * smpte, gint type, gboolean invert,
    gint depth, gint width, gint height)
{
  GstMask *newmask;

  if (smpte->mask) {
    if (smpte->type == type &&
        smpte->invert == invert &&
        smpte->depth == depth &&
        smpte->width == width && smpte->height == height)
      return TRUE;
  }

  newmask = gst_mask_factory_new (type, invert, depth, width, height);
  if (newmask) {
    if (smpte->mask) {
      gst_mask_destroy (smpte->mask);
    }
    smpte->mask = newmask;
    smpte->type = type;
    smpte->invert = invert;
    smpte->depth = depth;
    smpte->width = width;
    smpte->height = height;

    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_smpte_setcaps (GstPad * pad, GstCaps * caps)
{
  GstSMPTE *smpte;
  GstStructure *structure;
  gboolean ret;

  smpte = GST_SMPTE (GST_PAD_PARENT (pad));

  structure = gst_caps_get_structure (caps, 0);

  ret = gst_structure_get_int (structure, "width", &smpte->width);
  ret &= gst_structure_get_int (structure, "height", &smpte->height);
  ret &= gst_structure_get_fraction (structure, "framerate",
      &smpte->fps_num, &smpte->fps_denom);
  if (!ret)
    return FALSE;

  /* for backward compat, we store these here */
  smpte->fps = ((gdouble) smpte->fps_num) / smpte->fps_denom;

  /* figure out the duration in frames */
  smpte->end_position = gst_util_uint64_scale (smpte->duration,
      smpte->fps_num, GST_SECOND * smpte->fps_denom);

  GST_DEBUG_OBJECT (smpte, "duration: %d frames", smpte->end_position);

  ret =
      gst_smpte_update_mask (smpte, smpte->type, smpte->invert, smpte->depth,
      smpte->width, smpte->height);

  return ret;
}

static void
gst_smpte_init (GstSMPTE * smpte)
{
  smpte->sinkpad1 =
      gst_pad_new_from_static_template (&gst_smpte_sink1_template, "sink1");
  gst_pad_set_setcaps_function (smpte->sinkpad1,
      GST_DEBUG_FUNCPTR (gst_smpte_setcaps));
  gst_pad_set_getcaps_function (smpte->sinkpad1,
      GST_DEBUG_FUNCPTR (gst_pad_proxy_getcaps));
  gst_element_add_pad (GST_ELEMENT (smpte), smpte->sinkpad1);

  smpte->sinkpad2 =
      gst_pad_new_from_static_template (&gst_smpte_sink2_template, "sink2");
  gst_pad_set_setcaps_function (smpte->sinkpad2,
      GST_DEBUG_FUNCPTR (gst_smpte_setcaps));
  gst_pad_set_getcaps_function (smpte->sinkpad2,
      GST_DEBUG_FUNCPTR (gst_pad_proxy_getcaps));
  gst_element_add_pad (GST_ELEMENT (smpte), smpte->sinkpad2);

  smpte->srcpad =
      gst_pad_new_from_static_template (&gst_smpte_src_template, "src");
  gst_element_add_pad (GST_ELEMENT (smpte), smpte->srcpad);

  smpte->collect = gst_collect_pads_new ();
  gst_collect_pads_set_function (smpte->collect,
      (GstCollectPadsFunction) GST_DEBUG_FUNCPTR (gst_smpte_collected), smpte);
  gst_collect_pads_start (smpte->collect);

  gst_collect_pads_add_pad (smpte->collect, smpte->sinkpad1,
      sizeof (GstCollectData));
  gst_collect_pads_add_pad (smpte->collect, smpte->sinkpad2,
      sizeof (GstCollectData));

  smpte->fps = DEFAULT_PROP_FPS;
  smpte->type = DEFAULT_PROP_TYPE;
  smpte->border = DEFAULT_PROP_BORDER;
  smpte->depth = DEFAULT_PROP_DEPTH;
  smpte->duration = DEFAULT_PROP_DURATION;
  smpte->invert = DEFAULT_PROP_INVERT;
  smpte->fps_num = 0;
  smpte->fps_denom = 1;
}

static void
gst_smpte_finalize (GstSMPTE * smpte)
{
  if (smpte->collect) {
    gst_object_unref (smpte->collect);
  }

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) smpte);
}

static void
gst_smpte_reset (GstSMPTE * smpte)
{
  smpte->width = -1;
  smpte->height = -1;
  smpte->position = 0;
  smpte->end_position = 0;
}

static void
gst_smpte_blend_i420 (guint8 * in1, guint8 * in2, guint8 * out, GstMask * mask,
    gint width, gint height, gint border, gint pos)
{
  guint32 *maskp;
  gint value;
  gint i, j;
  gint min, max;
  guint8 *in1u, *in1v, *in2u, *in2v, *outu, *outv;
  gint lumsize = width * height;
  gint chromsize = lumsize >> 2;

  if (border == 0)
    border++;

  min = pos - border;
  max = pos;

  in1u = in1 + lumsize;
  in1v = in1u + chromsize;
  in2u = in2 + lumsize;
  in2v = in2u + chromsize;
  outu = out + lumsize;
  outv = outu + chromsize;

  maskp = mask->data;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      value = *maskp++;
      value = ((CLAMP (value, min, max) - min) << 8) / border;

      *out++ = ((*in1++ * value) + (*in2++ * (256 - value))) >> 8;
      if (!(i & 1) && !(j & 1)) {
        *outu++ = ((*in1u++ * value) + (*in2u++ * (256 - value))) >> 8;
        *outv++ = ((*in1v++ * value) + (*in2v++ * (256 - value))) >> 8;
      }
    }
  }
}

static GstFlowReturn
gst_smpte_collected (GstCollectPads * pads, GstSMPTE * smpte)
{
  GstBuffer *outbuf;
  GstClockTime ts;
  GstBuffer *in1 = NULL, *in2 = NULL;
  GSList *collected;

  if (G_UNLIKELY (smpte->fps_num == 0))
    goto not_negotiated;

  if (!GST_PAD_CAPS (smpte->sinkpad1) || !GST_PAD_CAPS (smpte->sinkpad2))
    goto not_negotiated;

  ts = gst_util_uint64_scale_int (smpte->position * GST_SECOND,
      smpte->fps_denom, smpte->fps_num);

  for (collected = pads->data; collected; collected = g_slist_next (collected)) {
    GstCollectData *data;

    data = (GstCollectData *) collected->data;

    if (data->pad == smpte->sinkpad1)
      in1 = gst_collect_pads_pop (pads, data);
    else if (data->pad == smpte->sinkpad2)
      in2 = gst_collect_pads_pop (pads, data);
  }

  if (in1 == NULL) {
    /* if no input, make picture black */
    in1 = gst_buffer_new_and_alloc (I420_SIZE (smpte->width, smpte->height));
    fill_i420 (GST_BUFFER_DATA (in1), smpte->width, smpte->height, 7);
  }
  if (in2 == NULL) {
    /* if no input, make picture white */
    in2 = gst_buffer_new_and_alloc (I420_SIZE (smpte->width, smpte->height));
    fill_i420 (GST_BUFFER_DATA (in2), smpte->width, smpte->height, 0);
  }

  if (GST_BUFFER_SIZE (in1) != GST_BUFFER_SIZE (in2))
    goto input_formats_do_not_match;

  if (smpte->position < smpte->end_position) {
    outbuf = gst_buffer_new_and_alloc (I420_SIZE (smpte->width, smpte->height));

    /* set caps if not done yet */
    if (!GST_PAD_CAPS (smpte->srcpad)) {
      GstCaps *caps;

      caps =
          gst_caps_copy (gst_static_caps_get
          (&gst_smpte_src_template.static_caps));
      gst_caps_set_simple (caps, "width", G_TYPE_INT, smpte->width, "height",
          G_TYPE_INT, smpte->height, "framerate", GST_TYPE_FRACTION,
          smpte->fps_num, smpte->fps_denom, NULL);

      gst_pad_set_caps (smpte->srcpad, caps);

      gst_pad_push_event (smpte->srcpad,
          gst_event_new_new_segment_full (FALSE,
              1.0, 1.0, GST_FORMAT_TIME, 0, -1, 0));

    }
    gst_buffer_set_caps (outbuf, GST_PAD_CAPS (smpte->srcpad));

    gst_smpte_blend_i420 (GST_BUFFER_DATA (in1),
        GST_BUFFER_DATA (in2),
        GST_BUFFER_DATA (outbuf),
        smpte->mask, smpte->width, smpte->height,
        smpte->border,
        ((1 << smpte->depth) + smpte->border) *
        smpte->position / smpte->end_position);

  } else {
    outbuf = in2;
    gst_buffer_ref (in2);
  }

  smpte->position++;

  if (in1)
    gst_buffer_unref (in1);
  if (in2)
    gst_buffer_unref (in2);

  GST_BUFFER_TIMESTAMP (outbuf) = ts;

  return gst_pad_push (smpte->srcpad, outbuf);

  /* ERRORS */
not_negotiated:
  {
    GST_ELEMENT_ERROR (smpte, CORE, NEGOTIATION, (NULL),
        ("No input format negotiated"));
    return GST_FLOW_NOT_NEGOTIATED;
  }
input_formats_do_not_match:
  {
    GST_ELEMENT_ERROR (smpte, CORE, NEGOTIATION, (NULL),
        ("input formats don't match: %" GST_PTR_FORMAT " vs. %" GST_PTR_FORMAT,
            GST_PAD_CAPS (smpte->sinkpad1), GST_PAD_CAPS (smpte->sinkpad2)));
    return GST_FLOW_ERROR;
  }
}

static void
gst_smpte_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSMPTE *smpte;

  smpte = GST_SMPTE (object);

  switch (prop_id) {
    case PROP_TYPE:
      smpte->type = g_value_get_enum (value);
      break;
    case PROP_BORDER:
      smpte->border = g_value_get_int (value);
      break;
    case PROP_FPS:
      smpte->fps = g_value_get_float (value);
      break;
    case PROP_DEPTH:
      smpte->depth = g_value_get_int (value);
      break;
    case PROP_DURATION:
      smpte->duration = g_value_get_uint64 (value);
      break;
    case PROP_INVERT:
      smpte->invert = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_smpte_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSMPTE *smpte;

  smpte = GST_SMPTE (object);

  switch (prop_id) {
    case PROP_TYPE:
      g_value_set_enum (value, smpte->type);
      break;
    case PROP_FPS:
      g_value_set_float (value, smpte->fps);
      break;
    case PROP_BORDER:
      g_value_set_int (value, smpte->border);
      break;
    case PROP_DEPTH:
      g_value_set_int (value, smpte->depth);
      break;
    case PROP_DURATION:
      g_value_set_uint64 (value, smpte->duration);
      break;
    case PROP_INVERT:
      g_value_set_boolean (value, smpte->invert);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_smpte_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstSMPTE *smpte;

  smpte = GST_SMPTE (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_smpte_reset (smpte);
      GST_LOG_OBJECT (smpte, "starting collectpads");
      gst_collect_pads_start (smpte->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_LOG_OBJECT (smpte, "stopping collectpads");
      gst_collect_pads_stop (smpte->collect);
      break;
    default:
      break;
  }

  ret = parent_class->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_smpte_reset (smpte);
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_smpte_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_smpte_debug, "smpte", 0,
      "SMPTE transition effect");

  return gst_element_register (plugin, "smpte", GST_RANK_NONE, GST_TYPE_SMPTE);
}
