if LTIBBUILD

else
FBHEADER_PATH=../../../../inc/misc
endif

plugin_LTLIBRARIES = libmfw_gst_tvsrc.la 

libmfw_gst_tvsrc_la_SOURCES =  mfw_gst_tvsrc.c 
libmfw_gst_tvsrc_la_CFLAGS = $(GST_BASE_CFLAGS) -fno-omit-frame-pointer $(IPU_CFLAGS) -D$(PLATFORM) -I$(FBHEADER_PATH) -I. -I$(top_srcdir)/inc/plugin -I$(top_srcdir)/libs/gstbufmeta 
libmfw_gst_tvsrc_la_LIBADD = $(GST_BASE_LIBS) -lgstvideo-$(GST_MAJORMINOR) -lgstinterfaces-$(GST_MAJORMINOR) ../../../../libs/libgstfsl-@GST_MAJORMINOR@.la
libmfw_gst_tvsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = mfw_gst_tvsrc.h 
