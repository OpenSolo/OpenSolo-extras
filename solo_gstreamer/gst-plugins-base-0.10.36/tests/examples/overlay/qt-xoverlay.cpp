/* GStreamer
 * Copyright (C) <2010> Stefan Kost <ensonic@users.sf.net>
 *
 * qt-xoverlay: demonstrate overlay handling using qt
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
#include "config.h"
#endif

#include <glib.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include <QApplication>
#include <QTimer>
#include <QWidget>

/* slightly convoluted way to find a working video sink that's not a bin,
 * one could use autovideosink from gst-plugins-good instead
 */
static GstElement *
find_video_sink (void)
{
  GstStateChangeReturn sret;
  GstElement *sink;

  if ((sink = gst_element_factory_make ("xvimagesink", NULL))) {
    sret = gst_element_set_state (sink, GST_STATE_READY);
    if (sret == GST_STATE_CHANGE_SUCCESS)
      return sink;
  
    gst_element_set_state (sink, GST_STATE_NULL);
  }
  gst_object_unref (sink);

  if ((sink = gst_element_factory_make ("ximagesink", NULL))) {
    sret = gst_element_set_state (sink, GST_STATE_READY);
    if (sret == GST_STATE_CHANGE_SUCCESS)
      return sink;
  
    gst_element_set_state (sink, GST_STATE_NULL);
  }
  gst_object_unref (sink);

  if (strcmp (DEFAULT_VIDEOSINK, "xvimagesink") == 0 ||
      strcmp (DEFAULT_VIDEOSINK, "ximagesink") == 0)
    return NULL;

  if ((sink = gst_element_factory_make (DEFAULT_VIDEOSINK, NULL))) {
    if (GST_IS_BIN (sink)) {
      gst_object_unref (sink);
      return NULL;
    }
  
    sret = gst_element_set_state (sink, GST_STATE_READY);
    if (sret == GST_STATE_CHANGE_SUCCESS)
      return sink;
  
    gst_element_set_state (sink, GST_STATE_NULL);
  }
  gst_object_unref (sink);
  return NULL;
}

int main(int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION (2, 31, 0)
  if (!g_thread_supported ())
    g_thread_init (NULL);
#endif

  gst_init (&argc, &argv);
  QApplication app(argc, argv);
  app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit ()));

  /* prepare the pipeline */

  GstElement *pipeline = gst_pipeline_new ("xvoverlay");
  GstElement *src = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *sink = find_video_sink ();

  if (sink == NULL)
    g_error ("Couldn't find a working video sink.");

  gst_bin_add_many (GST_BIN (pipeline), src, sink, NULL);
  gst_element_link (src, sink);
  
  /* prepare the ui */

  QWidget window;
  window.resize(320, 240);
  window.setWindowTitle("GstXOverlay Qt demo");
  window.show();
  
  WId xwinid = window.winId();
  gst_x_overlay_set_window_handle (GST_X_OVERLAY (sink), xwinid);

  /* run the pipeline */

  GstStateChangeReturn sret = gst_element_set_state (pipeline,
      GST_STATE_PLAYING);
  if (sret == GST_STATE_CHANGE_FAILURE) {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    /* Exit application */
    QTimer::singleShot(0, QApplication::activeWindow(), SLOT(quit()));
  }

  int ret = app.exec();
  
  window.hide();
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return ret;
}
