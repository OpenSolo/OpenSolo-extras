/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2004> Thomas Vander Stichele <thomas at apestaart dot org>
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


#ifndef __GST_TCP_SERVER_SRC_H__
#define __GST_TCP_SERVER_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_END_DECLS

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gsttcp.h"

#include <fcntl.h>

#define GST_TYPE_TCP_SERVER_SRC \
  (gst_tcp_server_src_get_type())
#define GST_TCP_SERVER_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TCP_SERVER_SRC,GstTCPServerSrc))
#define GST_TCP_SERVER_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TCP_SERVER_SRC,GstTCPServerSrcClass))
#define GST_IS_TCP_SERVER_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TCP_SERVER_SRC))
#define GST_IS_TCP_SERVER_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TCP_SERVER_SRC))

typedef struct _GstTCPServerSrc GstTCPServerSrc;
typedef struct _GstTCPServerSrcClass GstTCPServerSrcClass;

typedef enum {
  GST_TCP_SERVER_SRC_OPEN       = (GST_ELEMENT_FLAG_LAST << 0),

  GST_TCP_SERVER_SRC_FLAG_LAST  = (GST_ELEMENT_FLAG_LAST << 2)
} GstTCPServerSrcFlags;

struct _GstTCPServerSrc {
  GstPushSrc element;

  /* server information */
  int server_port;
  gchar *host;
  struct sockaddr_in server_sin;
  GstPollFD server_sock_fd;

  /* client information */
  struct sockaddr_in client_sin;
  socklen_t client_sin_len;
  GstPollFD client_sock_fd;

  GstPoll *fdset;

  GstTCPProtocol protocol; /* protocol used for reading data */
  gboolean caps_received;      /* if we have received caps yet */
};

struct _GstTCPServerSrcClass {
  GstPushSrcClass parent_class;
};

GType gst_tcp_server_src_get_type (void);

G_BEGIN_DECLS

#endif /* __GST_TCP_SERVER_SRC_H__ */
