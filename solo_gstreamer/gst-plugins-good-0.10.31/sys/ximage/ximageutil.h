/* GStreamer
 * Copyright (C) <2005> Luca Ognibene <luogni@tin.it>
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

#ifndef __GST_XIMAGEUTIL_H__
#define __GST_XIMAGEUTIL_H__

#include <gst/gst.h>

#ifdef HAVE_XSHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* HAVE_XSHM */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_XSHM
#include <X11/extensions/XShm.h>
#endif /* HAVE_XSHM */

#include <string.h>
#include <math.h>

G_BEGIN_DECLS

typedef struct _GstXContext GstXContext;
typedef struct _GstXWindow GstXWindow;
typedef struct _GstXImage GstXImage;
typedef struct _GstXImageSrcBuffer GstXImageSrcBuffer;

/* Global X Context stuff */
/**
 * GstXContext:
 * @disp: the X11 Display of this context
 * @screen: the default Screen of Display @disp
 * @screen_num: the Screen number of @screen
 * @visual: the default Visual of Screen @screen
 * @root: the root Window of Display @disp
 * @white: the value of a white pixel on Screen @screen
 * @black: the value of a black pixel on Screen @screen
 * @depth: the color depth of Display @disp
 * @bpp: the number of bits per pixel on Display @disp
 * @endianness: the endianness of image bytes on Display @disp
 * @width: the width in pixels of Display @disp
 * @height: the height in pixels of Display @disp
 * @widthmm: the width in millimeters of Display @disp
 * @heightmm: the height in millimeters of Display @disp
 * @par: the pixel aspect ratio calculated from @width, @widthmm and @height,
 * @heightmm ratio
 * @use_xshm: used to known wether of not XShm extension is usable or not even
 * if the Extension is present
 * @caps: the #GstCaps that Display @disp can accept
 *
 * Structure used to store various informations collected/calculated for a
 * Display.
 */
struct _GstXContext {
  Display *disp;

  Screen *screen;
  gint screen_num;

  Visual *visual;

  Window root;

  gulong white, black;

  gint depth;
  gint bpp;
  gint endianness;

  gint width, height;
  gint widthmm, heightmm;

  /* these are the output masks 
   * for buffers from ximagesrc
   * and are in big endian */
  guint32 r_mask_output, g_mask_output, b_mask_output;
  
  GValue *par;                  /* calculated pixel aspect ratio */

  gboolean use_xshm;

  GstCaps *caps;
};

/**
 * GstXWindow:
 * @win: the Window ID of this X11 window
 * @width: the width in pixels of Window @win
 * @height: the height in pixels of Window @win
 * @internal: used to remember if Window @win was created internally or passed
 * through the #GstXOverlay interface
 * @gc: the Graphical Context of Window @win
 *
 * Structure used to store informations about a Window.
 */
struct _GstXWindow {
  Window win;
  gint width, height;
  gboolean internal;
  GC gc;
};

gboolean ximageutil_check_xshm_calls (GstXContext * xcontext);

GstXContext *ximageutil_xcontext_get (GstElement *parent, 
    const gchar *display_name);
void ximageutil_xcontext_clear (GstXContext *xcontext);
void ximageutil_calculate_pixel_aspect_ratio (GstXContext * xcontext);

/* custom ximagesrc buffer, copied from ximagesink */

/* BufferReturnFunc is called when a buffer is finalised */
typedef void (*BufferReturnFunc) (GstElement *parent, GstXImageSrcBuffer *buf);

/**
 * GstXImageSrcBuffer:
 * @parent: a reference to the element we belong to
 * @ximage: the XImage of this buffer
 * @width: the width in pixels of XImage @ximage
 * @height: the height in pixels of XImage @ximage
 * @size: the size in bytes of XImage @ximage
 *
 * Subclass of #GstBuffer containing additional information about an XImage.
 */
struct _GstXImageSrcBuffer {
  GstBuffer buffer;

  /* Reference to the ximagesrc we belong to */
  GstElement *parent;

  XImage *ximage;

#ifdef HAVE_XSHM
  XShmSegmentInfo SHMInfo;
#endif /* HAVE_XSHM */

  gint width, height;
  size_t size;
  
  BufferReturnFunc return_func;
};


GstXImageSrcBuffer *gst_ximageutil_ximage_new (GstXContext *xcontext,
  GstElement *parent, int width, int height, BufferReturnFunc return_func);

void gst_ximageutil_ximage_destroy (GstXContext *xcontext, 
  GstXImageSrcBuffer * ximage);
  
/* Call to manually release a buffer */
void gst_ximage_buffer_free (GstXImageSrcBuffer *ximage);

#define GST_TYPE_XIMAGESRC_BUFFER            (gst_ximagesrc_buffer_get_type())
#define GST_IS_XIMAGESRC_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_XIMAGESRC_BUFFER))
#define GST_IS_XIMAGESRC_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_XIMAGESRC_BUFFER))
#define GST_XIMAGESRC_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_XIMAGESRC_BUFFER, GstXImageSrcBuffer))
#define GST_XIMAGESRC_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_XIMAGESRC_BUFFER, GstXImageSrcBufferClass))
#define GST_XIMAGESRC_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_XIMAGESRC_BUFFER, GstXImageSrcBufferClass))

G_END_DECLS 

#endif /* __GST_XIMAGEUTIL_H__ */
