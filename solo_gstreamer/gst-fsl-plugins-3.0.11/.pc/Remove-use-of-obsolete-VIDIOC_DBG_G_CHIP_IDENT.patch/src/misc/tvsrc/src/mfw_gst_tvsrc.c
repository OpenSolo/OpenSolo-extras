/*
 * Copyright (c) 2011-2013, Freescale Semiconductor, Inc. All rights reserved.
 *
 */

/*
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

/*
 * Module Name:    mfw_gst_tvsrc.c
 *
 * Description:    Implementation of TV-in module V4L2 Plugin for Gstreamer
 *
 * Portability:    This code is written for Linux OS and Gstreamer
 */

/*
 * Changelog: 
 *
 */



/*=============================================================================
                            INCLUDE FILES
=============================================================================*/
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/interfaces/propertyprobe.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include "mfw_gst_tvsrc.h"
#ifdef MX27
#include "mxcfb.h"
#else
#include "linux/mxcfb.h"
#endif

#include "mfw_gst_utils.h"
#include "gstbufmeta.h"


/*=============================================================================
                            LOCAL CONSTANTS
=============================================================================*/
#define DEFAULT_QUEUE_SIZE 6
#define NO_BUFFER_WAIT_COUNT   200
#define NO_BUFFER_WAIT_TIME_MS 5
#define NTSC_FRAMERATE 30
#define PAL_FRAMRRATE 25

/*=============================================================================
                LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
=============================================================================*/

enum
{
  MFW_V4L_SRC_0,
  MFW_V4L_SRC_FRAMERATE_NUM,
  MFW_V4L_SRC_FRAMERATE_DEN,
  MFW_V4L_SRC_DEVICE,
  MFW_V4L_SRC_QUEUE_SIZE,
};


/*=============================================================================
                              LOCAL MACROS
=============================================================================*/

/* used for debugging */
#define GST_CAT_DEFAULT mfw_gst_tvsrc_debug

#define ipu_fourcc(a,b,c,d)\
        (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define IPU_PIX_FMT_RGB332  ipu_fourcc('R','G','B','1') /*!<  8  RGB-3-3-2     */
#define IPU_PIX_FMT_RGB555  ipu_fourcc('R','G','B','O') /*!< 16  RGB-5-5-5     */
#define IPU_PIX_FMT_RGB565  ipu_fourcc('R','G','B','P') /*!< 16  RGB-5-6-5     */
#define IPU_PIX_FMT_RGB666  ipu_fourcc('R','G','B','6') /*!< 18  RGB-6-6-6     */
#define IPU_PIX_FMT_BGR24   ipu_fourcc('B','G','R','3') /*!< 24  BGR-8-8-8     */
#define IPU_PIX_FMT_RGB24   ipu_fourcc('R','G','B','3') /*!< 24  RGB-8-8-8     */
#define IPU_PIX_FMT_BGR32   ipu_fourcc('B','G','R','4') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_BGRA32  ipu_fourcc('B','G','R','A') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_RGB32   ipu_fourcc('R','G','B','4') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_RGBA32  ipu_fourcc('R','G','B','A') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_ABGR32  ipu_fourcc('A','B','G','R') /*!< 32  ABGR-8-8-8-8  */



/*=============================================================================
                             STATIC VARIABLES
=============================================================================*/


/*=============================================================================
                             GLOBAL VARIABLES
=============================================================================*/
/* None */

/*=============================================================================
                        LOCAL FUNCTION PROTOTYPES
=============================================================================*/

GST_DEBUG_CATEGORY_STATIC (mfw_gst_tvsrc_debug);
static void mfw_gst_tvsrc_buffer_class_init (gpointer g_class,
    gpointer class_data);
static void mfw_gst_tvsrc_buffer_init (GTypeInstance * instance,
    gpointer g_class);
static void mfw_gst_tvsrc_buffer_finalize (MFWGstTVSRCBuffer * TVSRC_buffer);
static void mfw_gst_tvsrc_fixate (GstPad * pad, GstCaps * caps);
static GstCaps *mfw_gst_tvsrc_get_caps (GstBaseSrc * src);
static GstFlowReturn mfw_gst_tvsrc_create (GstPushSrc * src, GstBuffer ** buf);
static GstBuffer *mfw_gst_tvsrc_buffer_new (MFWGstTVSRC * v4l_src);
static GstStateChangeReturn mfw_gst_tvsrc_change_state (GstElement * element,
    GstStateChange transition);
static gboolean unlock (GstBaseSrc *src);
static gboolean mfw_gst_tvsrc_stop (GstBaseSrc * src);
static gboolean mfw_gst_tvsrc_start (GstBaseSrc * src);
static gboolean mfw_gst_tvsrc_set_caps (GstBaseSrc * src, GstCaps * caps);
static void mfw_gst_tvsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void mfw_gst_tvsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static gint mfw_gst_tvsrc_capture_setup (MFWGstTVSRC * v4l_src);
static gint mfw_gst_tvsrc_stop_capturing (MFWGstTVSRC * v4l_src);
static gint mfw_gst_tvsrc_start_capturing (MFWGstTVSRC * v4l_src);


GST_BOILERPLATE (MFWGstTVSRC, mfw_gst_tvsrc, GstPushSrc, GST_TYPE_PUSH_SRC);


/*=============================================================================
FUNCTION:           mfw_gst_tvsrc_buffer_get_type    

DESCRIPTION:        This funtion registers the  buffer type on to the V4L Source plugin
             
ARGUMENTS PASSED:   void   

RETURN VALUE:       Return the registered buffer type
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
GType
mfw_gst_tvsrc_buffer_get_type (void)
{
  static GType TVSRC_buffer_type;

  if (G_UNLIKELY (TVSRC_buffer_type == 0)) {
    static const GTypeInfo TVSRC_buffer_info = {
      sizeof (GstBufferClass),
      NULL,
      NULL,
      mfw_gst_tvsrc_buffer_class_init,
      NULL,
      NULL,
      sizeof (MFWGstTVSRCBuffer),
      0,
      mfw_gst_tvsrc_buffer_init,
      NULL
    };
    TVSRC_buffer_type = g_type_register_static (GST_TYPE_BUFFER,
        "MFWGstTVSRCBuffer", &TVSRC_buffer_info, 0);
  }
  return TVSRC_buffer_type;
}


/*=============================================================================
FUNCTION:           mfw_gst_tvsrc_buffer_class_init    

DESCRIPTION:   This funtion registers the  funtions used by the 
                buffer class of the V4l source plug-in
             
ARGUMENTS PASSED:
        g_class        -   class from which the mini objext is derived
        class_data     -   global class data

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static void
mfw_gst_tvsrc_buffer_class_init (gpointer g_class, gpointer class_data)
{
  GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS (g_class);

  mini_object_class->finalize = (GstMiniObjectFinalizeFunction)
      mfw_gst_tvsrc_buffer_finalize;
}


/*=============================================================================
FUNCTION:      mfw_gst_tvsrc_buffer_init    

DESCRIPTION:   This funtion initialises the buffer class of the V4l source plug-in
             
ARGUMENTS PASSED:
        instance       -   pointer to buffer instance
        g_class        -   global pointer

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static void
mfw_gst_tvsrc_buffer_init (GTypeInstance * instance, gpointer g_class)
{

}


/*=============================================================================
FUNCTION:      mfw_gst_tvsrc_buffer_finalize    

DESCRIPTION:   This function is invoked whenever the buffer object belonging 
               to the V4L Source buffer glass is tried to un-refrenced. Here 
               only the refernce count of the buffer object is increased without 
               freeing the memory allocated.

ARGUMENTS PASSED:
        TVSRC_buffer -   pointer to V4L sou4rce buffer class

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static void
mfw_gst_tvsrc_buffer_finalize (MFWGstTVSRCBuffer * TVSRC_buffer)
{
  MFWGstTVSRC *v4l_src;
  gint num;
  GstBuffer *buf;
  struct v4l2_buffer v4lbuf;


  v4l_src = TVSRC_buffer->TVSRCcontext;
  if (v4l_src->start) {
    num = TVSRC_buffer->num;


    buf = (GstBuffer *) (v4l_src->buffers[num]);
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_LAST);

    g_mutex_lock (v4l_src->pool_lock);
    if (g_list_find (v4l_src->free_pool, (gpointer) (num)))
      GST_WARNING ("something wrong here, v4l buffer index:%d already in queue",
          num);
    else
      GST_LOG ("v4l buffer index:%d will be push in pool", num);
    g_mutex_unlock (v4l_src->pool_lock);

    memset (&v4lbuf, 0, sizeof (v4lbuf));
    v4lbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4lbuf.memory = V4L2_MEMORY_MMAP;
    v4lbuf.index = num;

    if (ioctl (v4l_src->fd_v4l, VIDIOC_QBUF, &v4lbuf) < 0) {
      GST_ERROR (">>V4L_SRC: VIDIOC_QBUF failed");
      return;
    }

    g_mutex_lock (v4l_src->pool_lock);
    v4l_src->free_pool = g_list_append (v4l_src->free_pool, (gpointer) num);
    g_mutex_unlock (v4l_src->pool_lock);
    GST_LOG_OBJECT (v4l_src, "freeing buffer %p for frame %d", TVSRC_buffer,
        num);
    gst_buffer_ref (GST_BUFFER_CAST (TVSRC_buffer));
  } else {
    GST_LOG ("free buffer %d\n", TVSRC_buffer->num);
  }
}


/*=============================================================================
FUNCTION:      mfw_gst_tvsrc_start_capturing    
        
DESCRIPTION:   This function triggers the V4L Driver to start Capturing

ARGUMENTS PASSED:
        v4l_src -   The V4L Souce plug-in context.

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gint
mfw_gst_tvsrc_start_capturing (MFWGstTVSRC * v4l_src)
{
  guint i;
  struct v4l2_buffer *buf;
  MFWGstTVSRCBuffer *tvsrc_buf = NULL;
  enum v4l2_buf_type type;

  v4l_src->buffers = g_malloc (v4l_src->queue_size * sizeof (GstBuffer *));
  // query for v4l_src->queue_size number of buffers to store the captured data 
  for (i = 0; i < v4l_src->queue_size; i++) {
    tvsrc_buf =
        (MFWGstTVSRCBuffer *) gst_mini_object_new (MFW_GST_TYPE_TVSRC_BUFFER);
    tvsrc_buf->num = i;
    tvsrc_buf->TVSRCcontext = v4l_src;
    /* v4l2_buffer initialization */
    buf = &tvsrc_buf->v4l2_buf;
    // memset (&buf, 0, sizeof (buf));
    buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;
    buf->index = i;
    if (ioctl (v4l_src->fd_v4l, VIDIOC_QUERYBUF, buf) < 0) {
      GST_ERROR (">>V4L_SRC: VIDIOC_QUERYBUF error");
      return -1;
    }
    // v4l_src->buffers[i] = gst_buffer_new ();
    /* GstBuffer initialization */
    v4l_src->buffers[i] = (GstBuffer *) tvsrc_buf;
    GST_BUFFER_SIZE (v4l_src->buffers[i]) = buf->length;
    GST_BUFFER_OFFSET (v4l_src->buffers[i]) = (size_t) buf->m.offset;
    GST_BUFFER_DATA (v4l_src->buffers[i]) = mmap (NULL,
        GST_BUFFER_SIZE (v4l_src->buffers[i]),
        PROT_READ | PROT_WRITE, MAP_SHARED,
        v4l_src->fd_v4l, GST_BUFFER_OFFSET (v4l_src->buffers[i]));
    memset (GST_BUFFER_DATA (v4l_src->buffers[i]), 0xFF,
        GST_BUFFER_SIZE (v4l_src->buffers[i]));
    {
      gint index;
      GstBufferMeta *meta;
      index = G_N_ELEMENTS (v4l_src->buffers[i]->_gst_reserved) - 1;
      meta = gst_buffer_meta_new ();
      meta->physical_data = (gpointer) (buf->m.offset);
      v4l_src->buffers[i]->_gst_reserved[index] = meta;
    }


    buf->m.offset = GST_BUFFER_OFFSET (v4l_src->buffers[i]);
    if (v4l_src->crop_pixel) {
      buf->m.offset += v4l_src->crop_pixel *
          (v4l_src->capture_width) + v4l_src->crop_pixel;
    }

    if (ioctl (v4l_src->fd_v4l, VIDIOC_QBUF, buf) < 0) {
      GST_ERROR (">>V4L_SRC: VIDIOC_QBUF error");
      return -1;
    }
    v4l_src->free_pool =
        g_list_append (v4l_src->free_pool, (gpointer) buf->index);

  }

  v4l_src->pool_lock = g_mutex_new ();

  /* Switch ON the capture device */
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl (v4l_src->fd_v4l, VIDIOC_STREAMON, &type) < 0) {
    GST_ERROR (">>V4L_SRC: VIDIOC_STREAMON error");
    return -1;
  }

  v4l_src->start = TRUE;
  v4l_src->time_per_frame =
      gst_util_uint64_scale_int (GST_SECOND, v4l_src->fps_d, v4l_src->fps_n);
  GST_DEBUG (">>V4L_SRC: time per frame %d", (guint32) v4l_src->time_per_frame);
  v4l_src->last_ts = 0;
  return 0;
}


/*=============================================================================
FUNCTION:      mfw_gst_tvsrc_stop_capturing    
        
DESCRIPTION:   This function triggers the V4L Driver to stop Capturing

ARGUMENTS PASSED:
        v4l_src -   The V4L Souce plug-in context.

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gint
mfw_gst_tvsrc_stop_capturing (MFWGstTVSRC * v4l_src)
{
  enum v4l2_buf_type type;
  guint i;
  gint index;

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl (v4l_src->fd_v4l, VIDIOC_STREAMOFF, &type) < 0) {
    GST_ERROR (">>V4L_SRC: error in VIDIOC_STREAMOFF");
    return -1;
  }
  v4l_src->start = FALSE;
  for (i = 0; i < v4l_src->queue_size; i++) {
    munmap (GST_BUFFER_DATA (v4l_src->buffers[i]),
        GST_BUFFER_SIZE (v4l_src->buffers[i]));

    index = G_N_ELEMENTS (v4l_src->buffers[i]->_gst_reserved) - 1;
    gst_buffer_meta_free (v4l_src->buffers[i]->_gst_reserved[index]);
    gst_buffer_unref (v4l_src->buffers[i]);
  }
  if (v4l_src->buffers)
    g_free (v4l_src->buffers);

  g_mutex_free (v4l_src->pool_lock);
  v4l_src->pool_lock = NULL;

  return 0;
}



/*=============================================================================
FUNCTION:      mfw_gst_tvsrc_capture_setup    
        
DESCRIPTION:   This function does the necessay initialistions for the V4L capture
               device driver.

ARGUMENTS PASSED:
        v4l_src -   The V4L Souce plug-in context.

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gint
mfw_gst_tvsrc_capture_setup (MFWGstTVSRC * v4l_src)
{
  struct v4l2_format fmt;
  struct v4l2_control ctrl;
  struct v4l2_streamparm parm;
  struct v4l2_dbg_chip_ident chip;
  gint fd_v4l = 0;
  struct v4l2_mxc_offset off;
  gint in_width = 0, in_height = 0;
  v4l2_std_id id;

  if ((fd_v4l = open (v4l_src->devicename, O_RDWR, 0)) < 0) {
    GST_ERROR (">>V4L_SRC: Unable to open %s", v4l_src->devicename);
    return 0;
  }

  if (ioctl (fd_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip)) {
    g_print ("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
  } else
    g_print ("sensor chip is %s\n", chip.match.name);



  if (ioctl (fd_v4l, VIDIOC_G_STD, &id) < 0) {
    g_print ("VIDIOC_G_STD failed\n");
    close (fd_v4l);
    return 0;
  }
  v4l_src->id = id;

  if (ioctl (fd_v4l, VIDIOC_S_STD, &id) < 0) {
    g_print ("VIDIOC_S_STD failed\n");
    close (fd_v4l);
    return 0;
  }

  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  parm.parm.capture.timeperframe.numerator = v4l_src->fps_d;
  parm.parm.capture.timeperframe.denominator = v4l_src->fps_n;
  parm.parm.capture.capturemode = v4l_src->capture_mode;
  if ((v4l_src->tv_in) || (parm.parm.capture.capturemode >= 4)) {
    gint input = 1;
    g_print ("should set the input to 1\n");
    if (ioctl (fd_v4l, VIDIOC_S_INPUT, &input) < 0) {
      GST_ERROR (">>V4L_SRC: VIDIOC_S_INPUT failed");
      return -1;
    }
  }


  if (ioctl (fd_v4l, VIDIOC_S_PARM, &parm) < 0) {
    GST_ERROR (">>V4L_SRC: VIDIOC_S_PARM failed");
    return -1;
  }

  memset (&fmt, 0, sizeof (fmt));

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if (defined (MX51) || (defined(MX6)))
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
#else
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
#endif
  fmt.fmt.pix.width = 0;
  fmt.fmt.pix.height = 0;


  if (ioctl (fd_v4l, VIDIOC_S_FMT, &fmt) < 0) {
    GST_ERROR (">>V4L_SRC: set format failed");
    return 0;
  }

  if (ioctl (fd_v4l, VIDIOC_G_FMT, &fmt) < 0) {
    g_print ("VIDIOC_G_FMT failed\n");
    close (fd_v4l);
    return 0;
  }

  v4l_src->capture_width = fmt.fmt.pix.width;
  v4l_src->capture_height = fmt.fmt.pix.height;
  if (v4l_src->id == V4L2_STD_NTSC) {
    v4l_src->fps_n = NTSC_FRAMERATE;
  } else {
    v4l_src->fps_n = PAL_FRAMRRATE;
  }


  struct v4l2_requestbuffers req;
  memset (&req, 0, sizeof (req));
  req.count = v4l_src->queue_size;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl (fd_v4l, VIDIOC_REQBUFS, &req) < 0) {
    GST_ERROR
        (">>V4L_SRC: v4l_mfw_gst_tvsrc_capture_setup: VIDIOC_REQBUFS failed");
    return 0;
  }

  return fd_v4l;
}


/*=============================================================================
FUNCTION:           mfw_gst_tvsrc_set_property   
        
DESCRIPTION:        This function is notified if application changes the values of 
                    a property.            

ARGUMENTS PASSED:
        object  -   pointer to GObject   
        prop_id -   id of element
        value   -   pointer to Gvalue
        pspec   -   pointer to GParamSpec

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static void
mfw_gst_tvsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (object);
  switch (prop_id) {
    case MFW_V4L_SRC_FRAMERATE_NUM:
      v4l_src->fps_n = g_value_get_int (value);
      GST_DEBUG ("framerate numerator =%d", v4l_src->fps_n);
      break;
    case MFW_V4L_SRC_FRAMERATE_DEN:
      v4l_src->fps_d = g_value_get_int (value);
      GST_DEBUG ("framerate denominator=%d", v4l_src->fps_d);
      break;
    case MFW_V4L_SRC_DEVICE:
      if (v4l_src->devicename)
        g_free (v4l_src->devicename);
      v4l_src->devicename = g_strdup (g_value_get_string (value));
      break;
    case MFW_V4L_SRC_QUEUE_SIZE:
      v4l_src->queue_size = g_value_get_int (value);
      GST_DEBUG ("queue size=%d", v4l_src->queue_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}


/*=============================================================================
FUNCTION:   mfw_gst_tvsrc_get_property    
        
DESCRIPTION:    This function is notified if application requests the values of 
                a property.                  

ARGUMENTS PASSED:
        object  -   pointer to GObject   
        prop_id -   id of element
        value   -   pointer to Gvalue
        pspec   -   pointer to GParamSpec

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static void
mfw_gst_tvsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{

  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (object);
  switch (prop_id) {
    case MFW_V4L_SRC_FRAMERATE_NUM:
      g_value_set_int (value, v4l_src->fps_n);
      break;
    case MFW_V4L_SRC_FRAMERATE_DEN:
      g_value_set_int (value, v4l_src->fps_d);
      break;

    case MFW_V4L_SRC_DEVICE:
      g_value_set_string (value, v4l_src->devicename);
      break;

    case MFW_V4L_SRC_QUEUE_SIZE:
      g_value_set_int (value, v4l_src->queue_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}


/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_set_caps
         
DESCRIPTION:         this function does the capability negotiation between adjacent pad  

ARGUMENTS PASSED:    
        src       -   pointer to base source 
        caps      -   pointer to GstCaps
        
  
RETURN VALUE:       TRUE or FALSE depending on capability is negotiated or not.
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gboolean
mfw_gst_tvsrc_set_caps (GstBaseSrc * src, GstCaps * caps)
{
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (src);
  return TRUE;
}


/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_overlay_setup
         
DESCRIPTION:         This function performs the initialisations required for preview

ARGUMENTS PASSED:    
        fd_v4l    -   capture device ID
        fmt       -   pointer to the V4L format structure.
        
  
RETURN VALUE:       TRUE - preview setup initialised successfully
                    FALSE - Error in initialising the preview set up.
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
gboolean
mfw_gst_tvsrc_overlay_setup (MFWGstTVSRC * v4l_src, struct v4l2_format * fmt)
{
  struct v4l2_streamparm parm;
  v4l2_std_id id;
  struct v4l2_control ctl;
  struct v4l2_crop crop;
  int g_sensor_top = 0;
  int g_sensor_left = 0;
  int g_camera_color = 0;
  int fd_v4l = v4l_src->fd_v4l;
  struct v4l2_framebuffer fb;
  struct v4l2_cropcap cropcap;

  GST_INFO ("display lcd:%d\n", v4l_src->g_display_lcd);
  /* this ioctl sets up the LCD display for preview */
  if (ioctl (v4l_src->fd_v4l_out, VIDIOC_S_OUTPUT, &v4l_src->g_display_lcd) < 0) {
    GST_ERROR (">>V4L_SRC: VIDIOC_S_OUTPUT failed");
    return FALSE;
  }

  memset (&cropcap, 0, sizeof (cropcap));
  cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  if (ioctl (v4l_src->fd_v4l_out, VIDIOC_CROPCAP, &cropcap) < 0) {
    g_print ("get crop capability failed\n");
    return FALSE;
  }

  crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
  crop.c.left = v4l_src->crop_pixel;
  crop.c.top = v4l_src->crop_pixel;
  crop.c.width = v4l_src->capture_width;
  crop.c.height = v4l_src->capture_height;

  /* this ioctl sets capture rectangle */
  if (ioctl (v4l_src->fd_v4l_out, VIDIOC_S_CROP, &crop) < 0) {
    GST_ERROR (">>V4L_SRC: set capture rectangle for cropping failed");
    return FALSE;
  }

  ctl.id = V4L2_CID_PRIVATE_BASE;
  ctl.value = 0;
  if (ioctl (v4l_src->fd_v4l_out, VIDIOC_S_CTRL, &ctl) < 0) {
    g_print ("set ctrl failed\n");
    return FALSE;
  }

  ctl.id = V4L2_CID_PRIVATE_BASE + 3;
  ctl.value = 0;
  if (ioctl (v4l_src->fd_v4l_out, VIDIOC_S_CTRL, &ctl) < 0) {
    g_print ("set ctrl failed\n");
    return FALSE;
  }
  g_print ("set fb overlay\n");
  fb.flags = V4L2_FBUF_FLAG_OVERLAY;
  ioctl (v4l_src->fd_v4l_out, VIDIOC_S_FBUF, &fb);

  g_print ("display lcd:%d\n", v4l_src->g_display_lcd);

  if (ioctl (fd_v4l, VIDIOC_S_FMT, fmt) < 0) {
    GST_ERROR (">>V4L_SRC: set format failed");
    return FALSE;
  }

  if (ioctl (fd_v4l, VIDIOC_G_FMT, fmt) < 0) {
    GST_ERROR (">>V4L_SRC: get format failed");
    return FALSE;
  }

  if (ioctl (fd_v4l, VIDIOC_G_STD, &id) < 0) {
    GST_ERROR (">>V4L_SRC: VIDIOC_G_STD failed");
    return FALSE;
  }

  struct v4l2_requestbuffers req;
  memset (&req, 0, sizeof (req));
  req.count = v4l_src->queue_size;
  req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  req.memory = V4L2_MEMORY_MMAP;
  if (ioctl (v4l_src->fd_v4l_out, VIDIOC_REQBUFS, &req) < 0) {
    g_print ("request buffers failed\n");
    return FALSE;
  }


  return TRUE;
}



/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_start
         
DESCRIPTION:         this function is registered  with the Base Source Class of
                     the gstreamer to start the video capturing process 
                     from this function

ARGUMENTS PASSED:    
        src       -   pointer to base source 
        
RETURN VALUE:        TRUE or FALSE depending on the sate of capture initiation
        
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gboolean
mfw_gst_tvsrc_start (GstBaseSrc * src)
{
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (src);
  struct v4l2_format fmt;
  struct v4l2_framebuffer fb_v4l2;
  char fb_device[100] = "/dev/fb0";
  int fd_fb = 0;
  struct fb_fix_screeninfo fix;
  struct fb_var_screeninfo var;
  struct mxcfb_color_key color_key;
  struct mxcfb_gbl_alpha alpha;
  unsigned short *fb0;
  unsigned char *cur_fb8;
  unsigned short *cur_fb16;
  unsigned int *cur_fb32;
  __u32 screen_size;
  int h, w;
  int ret = 0;
  int g_display_width = 0;
  int g_display_height = 0;
  int g_display_top = 0;
  int g_display_left = 0;


  if (v4l_src->tv_in) {
    v4l_src->capture_width = 0;
    v4l_src->capture_height = 0;
    v4l_src->fps_n = 1;
  }

  v4l_src->fd_v4l = mfw_gst_tvsrc_capture_setup (v4l_src);
  if (v4l_src->fd_v4l <= 0) {
    GST_ERROR ("TVSRC:error in opening the device");
    return FALSE;
  }

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl (v4l_src->fd_v4l, VIDIOC_G_FMT, &fmt) < 0) {
    GST_ERROR (">>V4L_SRC: get format failed");
    return FALSE;
  } else {
    v4l_src->buffer_size = fmt.fmt.pix.sizeimage;
    GST_DEBUG ("Width = %d", fmt.fmt.pix.width);
    GST_DEBUG ("Height = %d", fmt.fmt.pix.height);
    GST_DEBUG ("Image size = %d", fmt.fmt.pix.sizeimage);
    GST_DEBUG ("pixelformat = %d", fmt.fmt.pix.pixelformat);
  }


  if (mfw_gst_tvsrc_start_capturing (v4l_src) < 0) {
    GST_ERROR ("start_capturing failed");
    return FALSE;
  }

  v4l_src->offset = 0;
  return TRUE;
}


/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_stop
         
DESCRIPTION:         this function is registered  with the Base Source Class of
                     the gstreamer to stop the video capturing process 
                     by this function

ARGUMENTS PASSED:    
        src       -   pointer to base source 
        
  
RETURN VALUE:        TRUE or FALSE depending on the sate of capture initiation
        
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gboolean
mfw_gst_tvsrc_stop (GstBaseSrc * src)
{
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (src);
  gint overlay = 0;


  if (mfw_gst_tvsrc_stop_capturing (v4l_src) < 0) {
    GST_ERROR (">>V4L_SRC: stop_capturing failed");
    return FALSE;
  }

  if (TRUE == v4l_src->preview) {

    if (ioctl (v4l_src->fd_v4l, VIDIOC_OVERLAY, &overlay) < 0) {
      g_print ("VIDIOC_OVERLAY stop failed\n");
      return FALSE;
    }
  }
  close (v4l_src->fd_v4l);
  v4l_src->fd_v4l = -1;
  close (v4l_src->fd_v4l_out);
  v4l_src->fd_v4l_out = -1;
  return TRUE;
}

static void
mfw_gst_tvsrc_set_field (MFWGstTVSRC * v4l_src, guint v4l_field, GstBuffer * buf)
{
  if (!buf)
    return;

  if (v4l_field == V4L2_FIELD_NONE)
    return;

  GstCaps *caps = GST_BUFFER_CAPS (buf);
  GstCaps *newcaps;
  GstStructure *stru;
  gint field = 0, gst_field = 0;
  stru = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (stru, "field", &field);

  if ( v4l_field == V4L2_FIELD_INTERLACED) {
    if (v4l_src->id == V4L2_STD_NTSC) {
      v4l_field = V4L2_FIELD_INTERLACED_BT;
    } else {
      v4l_field = V4L2_FIELD_INTERLACED_TB;
    }
  }

  switch (v4l_field) {
    case V4L2_FIELD_TOP:
      gst_field = FIELD_TOP;
      break;
    case V4L2_FIELD_BOTTOM:
      gst_field = FIELD_BOTTOM;
      break;
    case V4L2_FIELD_INTERLACED_TB:
      gst_field = FIELD_INTERLACED_TB;
      break;
    case V4L2_FIELD_INTERLACED_BT:
      gst_field = FIELD_INTERLACED_BT;
      break;
    default:
      gst_field = V4L2_FIELD_NONE;
      GST_WARNING("Field is not supported");
      return;
  }

  if (field != gst_field) {
    newcaps = gst_caps_copy (caps);
    gst_caps_set_simple (newcaps, "field", G_TYPE_INT, gst_field, NULL);
    gst_buffer_set_caps (buf, newcaps);
    gst_caps_unref (newcaps);
  }

}

/*=============================================================================
FUNCTION:           mfw_gst_tvsrc_buffer_new
         
DESCRIPTION:        This function is used to store the frames captured by the
                    V4L capture driver

ARGUMENTS PASSED:   v4l_src     - 
        
RETURN VALUE:       TRUE or FALSE depending on the sate of capture initiation
        
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static GstBuffer *
mfw_gst_tvsrc_buffer_new (MFWGstTVSRC * v4l_src)
{
  GstBuffer *buf;
  gint fps_n, fps_d;
  struct v4l2_buffer v4lbuf;
  GstClockTime ts, res;
  gint wait_cnt = 0;

  v4l_src->count++;
  memset (&v4lbuf, 0, sizeof (v4lbuf));
  v4lbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4lbuf.memory = V4L2_MEMORY_MMAP;

  while (g_list_length (v4l_src->free_pool) == 0) {
      GST_WARNING ("no buffer available in pool");
      g_usleep(NO_BUFFER_WAIT_TIME_MS*1000);
      wait_cnt ++;
      if (wait_cnt >= NO_BUFFER_WAIT_COUNT) {
          GST_ERROR ("no buffer available in %d ms", NO_BUFFER_WAIT_TIME_MS*NO_BUFFER_WAIT_COUNT);
          return NULL;
      }

      if (v4l_src->stop == TRUE) {
          GST_WARNING("v4l src stopped.");
          return NULL;
      }
  }

  if (ioctl (v4l_src->fd_v4l, VIDIOC_DQBUF, &v4lbuf) < 0) {
    GST_ERROR (">>V4L_SRC: VIDIOC_DQBUF failed.");
    return NULL;
  }

  g_mutex_lock (v4l_src->pool_lock);
  if (g_list_find (v4l_src->free_pool, (gpointer) (v4lbuf.index)))
    GST_LOG ("v4l buffer index:%d will be used outside", v4lbuf.index);
  else
    GST_WARNING ("v4l buffer index:%d can not be found in pool", v4lbuf.index);

  v4l_src->free_pool =
      g_list_remove (v4l_src->free_pool, (gpointer) (v4lbuf.index));
  g_mutex_unlock (v4l_src->pool_lock);


  buf = (GstBuffer *) (v4l_src->buffers[v4lbuf.index]);
  GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_LAST);

  GST_LOG ("v4l dequeued buffer index:%d(ref %d), num in pool:%d", v4lbuf.index,
      buf->mini_object.refcount, g_list_length (v4l_src->free_pool));

  if (v4l_src->preview) {

  }

  GST_BUFFER_SIZE (buf) = v4l_src->buffer_size;

  ts = gst_clock_get_time (GST_ELEMENT (v4l_src)->clock);
  if (ts != GST_CLOCK_TIME_NONE)
    ts -= gst_element_get_base_time (GST_ELEMENT (v4l_src));
  else
    ts = v4l_src->count * v4l_src->time_per_frame;
  GST_BUFFER_TIMESTAMP (buf) = ts;
  GST_BUFFER_DURATION (buf) = v4l_src->time_per_frame;

  if (v4l_src->last_ts) {
    guint num_frame_delay = 0;
    GstClockTimeDiff diff = ts - v4l_src->last_ts;
    if (ts < v4l_src->last_ts)
      diff = v4l_src->last_ts + ts;
    while (diff > v4l_src->time_per_frame) {
      diff -= v4l_src->time_per_frame;
      num_frame_delay++;
    }
    if (num_frame_delay > 1)
      GST_DEBUG (">>V4L_SRC: Camera ts late by %d frames", num_frame_delay);
  }
  v4l_src->last_ts = ts;

  gst_buffer_set_caps (buf, GST_PAD_CAPS (GST_BASE_SRC_PAD (v4l_src)));
  mfw_gst_tvsrc_set_field (v4l_src, v4lbuf.field, buf);

  return buf;
}


/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_create
         
DESCRIPTION:         This function is registered with the Base Source Class 
                     This function updates the the buffer to be pushed to the
                     next element with the frame captured.
                     
ARGUMENTS PASSED:    v4l_src     - 
        
  
RETURN VALUE:        
              GST_FLOW_OK       -    buffer create successfull.
              GST_FLOW_ERROR    -    Error in buffer creation.
        
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static GstFlowReturn
mfw_gst_tvsrc_create (GstPushSrc * src, GstBuffer ** buf)
{
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (src);
  *buf = mfw_gst_tvsrc_buffer_new (v4l_src);
  if (*buf == NULL)
    return GST_FLOW_ERROR;
  else
    return GST_FLOW_OK;

}

/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_unlock
         
DESCRIPTION:         this function is registered  with the Base Source Class of
                     the gstreamer to unlock any block in mfw_gst_tvsrc_create
                     by this function

ARGUMENTS PASSED:    
        src       -   pointer to base source 
        
  
RETURN VALUE:        TRUE
        
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static gboolean
mfw_gst_tvsrc_unlock (GstBaseSrc * src)
{
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (src);
  v4l_src->stop = TRUE;

  return TRUE;
}

/*=============================================================================
FUNCTION:   mfw_gst_tvsrc_change_state

DESCRIPTION: this function keeps track of different states of pipeline.

ARGUMENTS PASSED:
        element     -   pointer to element
        transition  -   state of the pipeline

RETURN VALUE:
        GST_STATE_CHANGE_FAILURE    - the state change failed
        GST_STATE_CHANGE_SUCCESS    - the state change succeeded
        GST_STATE_CHANGE_ASYNC      - the state change will happen
                                        asynchronously
        GST_STATE_CHANGE_NO_PREROLL - the state change cannot be prerolled

PRE-CONDITIONS:
        None

POST-CONDITIONS:
        None

IMPORTANT NOTES:
        None
=============================================================================*/
static GstStateChangeReturn
mfw_gst_tvsrc_change_state(GstElement* element, GstStateChange transition)
{
    
    GstStateChangeReturn retstate = GST_STATE_CHANGE_FAILURE;
    MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (element);
       
    switch (transition)
    {
        case GST_STATE_CHANGE_NULL_TO_READY:
            GST_DEBUG(" in NULL to READY state \n");
               break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            GST_DEBUG(" in READY_TO_PAUSED state \n");
            break;
        case  GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            GST_DEBUG(" in  to  PAUSED_TO_PLAYING state \n");
            v4l_src->stop = FALSE;
            break;
        default:
            break;
    }

    retstate = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    switch (transition)
    {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            GST_DEBUG(" in  to PLAYING_TO_PAUSED state \n");
            v4l_src->stop = TRUE;
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_DEBUG(" in  to PAUSED_TO_READY state \n");
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            GST_DEBUG(" in  to READY_TO_NULL state \n");
            break;
        default:
            break;
    }
    
    return retstate;
}

/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_get_caps
         
DESCRIPTION:         This function gets the caps to be set on the source pad.
                     

ARGUMENTS PASSED:    
        v4l_src     - 
         
RETURN VALUE:       Returns the caps to be set.
        
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static GstCaps *
mfw_gst_tvsrc_get_caps (GstBaseSrc * src)
{
  GstCaps *list;
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (src);
  GstCaps *capslist;
  GstPadTemplate *src_template = NULL;
  gint i;
#if (defined (MX51) || (defined(MX6)))
  guint32 format = GST_MAKE_FOURCC ('N', 'V', '1', '2');
#else
  guint32 format = GST_MAKE_FOURCC ('I', '4', '2', '0');
#endif

  capslist = gst_caps_new_empty ();

  gst_caps_append_structure (capslist,
      gst_structure_new ("video/x-raw-yuv",
          "format",
          GST_TYPE_FOURCC,
          format, "width",
          GST_TYPE_INT_RANGE, 16,
          G_MAXINT, "height",
          GST_TYPE_INT_RANGE, 16,
          G_MAXINT, "framerate",
          GST_TYPE_FRACTION_RANGE,
          0, 1, 100, 1, "pixel-aspect-ratio",
          GST_TYPE_FRACTION_RANGE, 0, 1, 100, 1, NULL));



  return capslist;
}


/*=============================================================================
FUNCTION:            mfw_gst_tvsrc_fixate
         
DESCRIPTION:         Fixes the Caps on the source pad
                     

ARGUMENTS PASSED:    v4l_src     - 
        
RETURN VALUE:        None
PRE-CONDITIONS:      None
POST-CONDITIONS:     None
IMPORTANT NOTES:     None
=============================================================================*/
static void
mfw_gst_tvsrc_fixate (GstPad * pad, GstCaps * caps)
{

  gint i = 0;
  GstStructure *structure = NULL;
  MFWGstTVSRC *v4l_src = MFW_GST_TVSRC (gst_pad_get_parent (pad));
#if (defined (MX51) || (defined(MX6)))
  guint32 fourcc = GST_MAKE_FOURCC ('N', 'V', '1', '2');
#else
  guint32 fourcc = GST_MAKE_FOURCC ('I', '4', '2', '0');
#endif

  const GValue *v = NULL;
  for (i = 0; i < gst_caps_get_size (caps); ++i) {
    structure = gst_caps_get_structure (caps, i);
    gst_structure_fixate_field_nearest_int (structure, "width",
        v4l_src->capture_width);
    gst_structure_fixate_field_nearest_int (structure, "height",
        v4l_src->capture_height);
    gst_structure_fixate_field_nearest_fraction (structure, "framerate",
        v4l_src->fps_n, v4l_src->fps_d);
    gst_structure_fixate_field_nearest_fraction (structure,
        "pixel-aspect-ratio", 1, 1);

    gst_structure_set (structure, "format", GST_TYPE_FOURCC, fourcc, NULL);
  }
  GST_INFO ("capture: %d, %d, fixrate :%s\n", v4l_src->capture_width,
      v4l_src->capture_height, gst_caps_to_string (caps));
  gst_object_unref (v4l_src);

}

/*=============================================================================
FUNCTION:   mfw_gst_tvsrc_init   
        
DESCRIPTION:     create the pad template that has been registered with the 
                element class in the _base_init and do library table 
                initialization      

ARGUMENTS PASSED:
        context  -    pointer to TVSRC element structure      
  
RETURN VALUE:       None
      
PRE-CONDITIONS:     _base_init and _class_init are called 
 
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=============================================================================*/
static void
mfw_gst_tvsrc_init (MFWGstTVSRC * v4l_src, MFWGstTVSRCClass * klass)
{
  v4l_src->capture_width = 176;
  v4l_src->capture_height = 144;
  v4l_src->fps_n = 30;
  v4l_src->fps_d = 1;
  v4l_src->fd_v4l = -1;
  v4l_src->count = 0;
  v4l_src->buffer_size = 0;
  v4l_src->offset = 0;
  v4l_src->crop_pixel = 0;
  v4l_src->rotate = 0;
  v4l_src->preview = FALSE;
  v4l_src->preview_width = 160;
  v4l_src->preview_height = 128;
  v4l_src->preview_top = 0;
  v4l_src->preview_left = 0;
  v4l_src->sensor_width = 1280;
  v4l_src->sensor_height = 1024;
  v4l_src->capture_mode = 0;
  v4l_src->bg = FALSE;
  v4l_src->g_display_lcd = 0;
  v4l_src->queue_size = DEFAULT_QUEUE_SIZE;
  v4l_src->start = FALSE;
  v4l_src->stop = FALSE;
  v4l_src->tv_in = TRUE;        /* */
  if (v4l_src->tv_in) {
    v4l_src->capture_width = 0;
    v4l_src->capture_height = 0;
    v4l_src->fps_n = 1;
    v4l_src->g_display_lcd = 3;
  }
#ifdef MX27
  v4l_src->devicename = g_strdup ("/dev/v4l/video0");
#else
  v4l_src->devicename = g_strdup ("/dev/video0");
#endif
  v4l_src->buf_pools = g_malloc (sizeof (GstBuffer *) * v4l_src->queue_size);

  gst_pad_set_fixatecaps_function (GST_BASE_SRC_PAD (v4l_src),
      mfw_gst_tvsrc_fixate);
  gst_base_src_set_live (GST_BASE_SRC (v4l_src), TRUE);

#define MFW_GST_tvsrc_PLUGIN VERSION
  PRINT_PLUGIN_VERSION (MFW_GST_tvsrc_PLUGIN);
  return;
}

/*=============================================================================
FUNCTION:   mfw_gst_tvsrc_class_init    
        
DESCRIPTION:     Initialise the class only once (specifying what signals,
                arguments and virtual functions the class has and setting up 
                global state)    
     

ARGUMENTS PASSED:
       klass   -   pointer to mp3decoder element class
        
RETURN VALUE:        None
PRE-CONDITIONS:      None
POST-CONDITIONS:     None
IMPORTANT NOTES:     None
=============================================================================*/
static void
mfw_gst_tvsrc_class_init (MFWGstTVSRCClass * klass)
{

  GObjectClass *gobject_class;
  GstBaseSrcClass *basesrc_class;
  GstPushSrcClass *pushsrc_class;
  GstElementClass *element_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  basesrc_class = (GstBaseSrcClass *) klass;
  pushsrc_class = (GstPushSrcClass *) klass;


  gobject_class->set_property = mfw_gst_tvsrc_set_property;
  gobject_class->get_property = mfw_gst_tvsrc_get_property;
  element_class->change_state = mfw_gst_tvsrc_change_state;

  g_object_class_install_property (gobject_class, MFW_V4L_SRC_FRAMERATE_NUM,
      g_param_spec_int ("fps-n",
          "fps_n",
          "gets the numerator of the framerate at which"
          "the input stream is to be captured",
          0, G_MAXINT, 0, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, MFW_V4L_SRC_FRAMERATE_DEN,
      g_param_spec_int ("fps-d",
          "fps_d",
          "gets the denominator of the framerate at which"
          "the input stream is to be captured",
          1, G_MAXINT, 1, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, MFW_V4L_SRC_DEVICE,
      g_param_spec_string ("device", "Device", "Device location",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MFW_V4L_SRC_QUEUE_SIZE,
      g_param_spec_int ("queue-size",
          "queue-size",
          "v4l2 request buffer number", 0, G_MAXINT, 5, G_PARAM_READWRITE));

  basesrc_class->get_caps = mfw_gst_tvsrc_get_caps;
  basesrc_class->set_caps = mfw_gst_tvsrc_set_caps;
  basesrc_class->start = mfw_gst_tvsrc_start;
  basesrc_class->stop = mfw_gst_tvsrc_stop;
  basesrc_class->unlock = mfw_gst_tvsrc_unlock;
  pushsrc_class->create = mfw_gst_tvsrc_create;
  return;
}


/*=============================================================================
FUNCTION:   mfw_gst_tvsrc_base_init   
        
DESCRIPTION:     v4l source element details are registered with the plugin during
                _base_init ,This function will initialise the class and child 
                class properties during each new child class creation       

ARGUMENTS PASSED:
        Klass   -   void pointer
  
RETURN VALUE:        None
PRE-CONDITIONS:      None
POST-CONDITIONS:     None
IMPORTANT NOTES:     None
=============================================================================*/
static void
mfw_gst_tvsrc_base_init (gpointer g_class)
{

  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  FSL_GST_ELEMENT_SET_DETAIL_SIMPLE (element_class, "v4l2 based tv src",
      "Src/Video", "Capture by using tv-in");

  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_new_any ()));

  GST_DEBUG_CATEGORY_INIT (mfw_gst_tvsrc_debug, "tvsrc", 0,
      "V4L2 tv src element");

  return;
}

/*=============================================================================
FUNCTION:   plugin_init

DESCRIPTION:    special function , which is called as soon as the plugin or 
                element is loaded and information returned by this function 
                will be cached in central registry

ARGUMENTS PASSED:
        plugin     -    pointer to container that contains features loaded 
                        from shared object module

RETURN VALUE:
        return TRUE or FALSE depending on whether it loaded initialized any 
        dependency correctly

PRE-CONDITIONS:      None
POST-CONDITIONS:     None
IMPORTANT NOTES:     None
=============================================================================*/
static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "tvsrc", GST_RANK_PRIMARY,
          MFW_GST_TYPE_TVSRC))
    return FALSE;

  return TRUE;
}

/*****************************************************************************/
/*    This is used to define the entry point and meta data of plugin         */
/*****************************************************************************/

FSL_GST_PLUGIN_DEFINE ("tvsrc", "v4l2-based tv src", plugin_init);
