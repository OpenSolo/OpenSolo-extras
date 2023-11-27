/* GStreamer
 * Copyright (C) <2005-2009> Wim Taymans <wim.taymans@gmail.com>
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
/*
 * Unless otherwise indicated, Source Code is licensed under MIT license.
 * See further explanation attached in License Statement (distributed in the file
 * LICENSE).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * SECTION:gstrtspconnection
 * @short_description: manage RTSP connections
 * @see_also: gstrtspurl
 *  
 * This object manages the RTSP connection to the server. It provides function
 * to receive and send bytes and messages.
 *  
 * Last reviewed on 2007-07-24 (0.10.14)
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* we include this here to get the G_OS_* defines */
#include <glib.h>
#include <gst/gst.h>

#ifdef G_OS_WIN32
/* ws2_32.dll has getaddrinfo and freeaddrinfo on Windows XP and later.
 * minwg32 headers check WINVER before allowing the use of these */
#ifndef WINVER
#define WINVER 0x0501
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#define EINPROGRESS WSAEINPROGRESS
#else
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#endif

#ifdef HAVE_FIONREAD_IN_SYS_FILIO
#include <sys/filio.h>
#endif

#include "gstrtspconnection.h"
#include "gstrtspbase64.h"

#include "gst/glib-compat-private.h"

union gst_sockaddr
{
  struct sockaddr sa;
  struct sockaddr_in sa_in;
  struct sockaddr_in6 sa_in6;
  struct sockaddr_storage sa_stor;
};

typedef struct
{
  gint state;
  guint save;
  guchar out[3];                /* the size must be evenly divisible by 3 */
  guint cout;
  guint coutl;
} DecodeCtx;

#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif

#ifdef G_OS_WIN32
#define READ_SOCKET(fd, buf, len) recv (fd, (char *)buf, len, 0)
#define WRITE_SOCKET(fd, buf, len) send (fd, (const char *)buf, len, SEND_FLAGS)
#define SETSOCKOPT(sock, level, name, val, len) setsockopt (sock, level, name, (const char *)val, len)
#define CLOSE_SOCKET(sock) closesocket (sock)
#define ERRNO_IS_EAGAIN (WSAGetLastError () == WSAEWOULDBLOCK)
#define ERRNO_IS_EINTR (WSAGetLastError () == WSAEINTR)
/* According to Microsoft's connect() documentation this one returns
 * WSAEWOULDBLOCK and not WSAEINPROGRESS. */
#define ERRNO_IS_EINPROGRESS (WSAGetLastError () == WSAEWOULDBLOCK)
#else
#define READ_SOCKET(fd, buf, len) read (fd, buf, len)
#define WRITE_SOCKET(fd, buf, len) send (fd, buf, len, SEND_FLAGS)
#define SETSOCKOPT(sock, level, name, val, len) setsockopt (sock, level, name, val, len)
#define CLOSE_SOCKET(sock) close (sock)
#define ERRNO_IS_EAGAIN (errno == EAGAIN)
#define ERRNO_IS_EINTR (errno == EINTR)
#define ERRNO_IS_EINPROGRESS (errno == EINPROGRESS)
#endif

#define ADD_POLLFD(fdset, pfd, fd)        \
G_STMT_START {                            \
  (pfd)->fd = fd;                         \
  gst_poll_add_fd (fdset, pfd);           \
} G_STMT_END

#define REMOVE_POLLFD(fdset, pfd)          \
G_STMT_START {                             \
  if ((pfd)->fd != -1) {                   \
    GST_DEBUG ("remove fd %d", (pfd)->fd); \
    gst_poll_remove_fd (fdset, pfd);       \
    CLOSE_SOCKET ((pfd)->fd);              \
    (pfd)->fd = -1;                        \
  }                                        \
} G_STMT_END

typedef enum
{
  TUNNEL_STATE_NONE,
  TUNNEL_STATE_GET,
  TUNNEL_STATE_POST,
  TUNNEL_STATE_COMPLETE
} GstRTSPTunnelState;

#define TUNNELID_LEN   24

struct _GstRTSPConnection
{
  /*< private > */
  /* URL for the connection */
  GstRTSPUrl *url;

  /* connection state */
  GstPollFD fd0;
  GstPollFD fd1;

  GstPollFD *readfd;
  GstPollFD *writefd;

  gboolean manual_http;

  gchar tunnelid[TUNNELID_LEN];
  gboolean tunneled;
  GstRTSPTunnelState tstate;

  GstPoll *fdset;
  gchar *ip;

  gint read_ahead;

  gchar *initial_buffer;
  gsize initial_buffer_offset;

  /* Session state */
  gint cseq;                    /* sequence number */
  gchar session_id[512];        /* session id */
  gint timeout;                 /* session timeout in seconds */
  GTimer *timer;                /* timeout timer */

  /* Authentication */
  GstRTSPAuthMethod auth_method;
  gchar *username;
  gchar *passwd;
  GHashTable *auth_params;

  DecodeCtx ctx;
  DecodeCtx *ctxp;

  gchar *proxy_host;
  guint proxy_port;
};

enum
{
  STATE_START = 0,
  STATE_DATA_HEADER,
  STATE_DATA_BODY,
  STATE_READ_LINES,
  STATE_END,
  STATE_LAST
};

enum
{
  READ_AHEAD_EOH = -1,          /* end of headers */
  READ_AHEAD_CRLF = -2,
  READ_AHEAD_CRLFCR = -3
};

/* a structure for constructing RTSPMessages */
typedef struct
{
  gint state;
  GstRTSPResult status;
  guint8 buffer[4096];
  guint offset;

  guint line;
  guint8 *body_data;
  glong body_len;
} GstRTSPBuilder;

static void
build_reset (GstRTSPBuilder * builder)
{
  g_free (builder->body_data);
  memset (builder, 0, sizeof (GstRTSPBuilder));
}

/**
 * gst_rtsp_connection_create:
 * @url: a #GstRTSPUrl 
 * @conn: storage for a #GstRTSPConnection
 *
 * Create a newly allocated #GstRTSPConnection from @url and store it in @conn.
 * The connection will not yet attempt to connect to @url, use
 * gst_rtsp_connection_connect().
 *
 * A copy of @url will be made.
 *
 * Returns: #GST_RTSP_OK when @conn contains a valid connection.
 */
GstRTSPResult
gst_rtsp_connection_create (const GstRTSPUrl * url, GstRTSPConnection ** conn)
{
  GstRTSPConnection *newconn;
#ifdef G_OS_WIN32
  WSADATA w;
  int error;
#endif

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

#ifdef G_OS_WIN32
  error = WSAStartup (0x0202, &w);

  if (error)
    goto startup_error;

  if (w.wVersion != 0x0202)
    goto version_error;
#endif

  newconn = g_new0 (GstRTSPConnection, 1);

  if ((newconn->fdset = gst_poll_new (TRUE)) == NULL)
    goto no_fdset;

  newconn->url = gst_rtsp_url_copy (url);
  newconn->fd0.fd = -1;
  newconn->fd1.fd = -1;
  newconn->timer = g_timer_new ();
  newconn->timeout = 60;
  newconn->cseq = 1;

  newconn->auth_method = GST_RTSP_AUTH_NONE;
  newconn->username = NULL;
  newconn->passwd = NULL;
  newconn->auth_params = NULL;

  *conn = newconn;

  return GST_RTSP_OK;

  /* ERRORS */
#ifdef G_OS_WIN32
startup_error:
  {
    g_warning ("Error %d on WSAStartup", error);
    return GST_RTSP_EWSASTART;
  }
version_error:
  {
    g_warning ("Windows sockets are not version 0x202 (current 0x%x)",
        w.wVersion);
    WSACleanup ();
    return GST_RTSP_EWSAVERSION;
  }
#endif
no_fdset:
  {
    g_free (newconn);
#ifdef G_OS_WIN32
    WSACleanup ();
#endif
    return GST_RTSP_ESYS;
  }
}

/**
 * gst_rtsp_connection_create_from_fd:
 * @fd: a file descriptor
 * @ip: the IP address of the other end
 * @port: the port used by the other end
 * @initial_buffer: data already read from @fd
 * @conn: storage for a #GstRTSPConnection
 *
 * Create a new #GstRTSPConnection for handling communication on the existing
 * file descriptor @fd. The @initial_buffer contains any data already read from
 * @fd which should be used before starting to read new data.
 *
 * Returns: #GST_RTSP_OK when @conn contains a valid connection.
 *
 * Since: 0.10.25
 */
GstRTSPResult
gst_rtsp_connection_create_from_fd (gint fd, const gchar * ip, guint16 port,
    const gchar * initial_buffer, GstRTSPConnection ** conn)
{
  GstRTSPConnection *newconn = NULL;
  GstRTSPUrl *url;
#ifdef G_OS_WIN32
  gulong flags = 1;
#endif
  GstRTSPResult res;

  g_return_val_if_fail (fd >= 0, GST_RTSP_EINVAL);
  g_return_val_if_fail (ip != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  /* set to non-blocking mode so that we can cancel the communication */
#ifndef G_OS_WIN32
  fcntl (fd, F_SETFL, O_NONBLOCK);
#else
  ioctlsocket (fd, FIONBIO, &flags);
#endif /* G_OS_WIN32 */

  /* create a url for the client address */
  url = g_new0 (GstRTSPUrl, 1);
  url->host = g_strdup (ip);
  url->port = port;

  /* now create the connection object */
  GST_RTSP_CHECK (gst_rtsp_connection_create (url, &newconn), newconn_failed);
  gst_rtsp_url_free (url);

  ADD_POLLFD (newconn->fdset, &newconn->fd0, fd);

  /* both read and write initially */
  newconn->readfd = &newconn->fd0;
  newconn->writefd = &newconn->fd0;

  newconn->ip = g_strdup (ip);

  newconn->initial_buffer = g_strdup (initial_buffer);

  *conn = newconn;

  return GST_RTSP_OK;

  /* ERRORS */
newconn_failed:
  {
    gst_rtsp_url_free (url);
    return res;
  }
}

/**
 * gst_rtsp_connection_accept:
 * @sock: a socket
 * @conn: storage for a #GstRTSPConnection
 *
 * Accept a new connection on @sock and create a new #GstRTSPConnection for
 * handling communication on new socket.
 *
 * Returns: #GST_RTSP_OK when @conn contains a valid connection.
 *
 * Since: 0.10.23
 */
GstRTSPResult
gst_rtsp_connection_accept (gint sock, GstRTSPConnection ** conn)
{
  int fd;
  union gst_sockaddr sa;
  socklen_t slen = sizeof (sa);
  gchar ip[INET6_ADDRSTRLEN];
  guint16 port;

  g_return_val_if_fail (sock >= 0, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  memset (&sa, 0, slen);

#ifndef G_OS_WIN32
  fd = accept (sock, &sa.sa, &slen);
#else
  fd = accept (sock, &sa.sa, (gint *) & slen);
#endif /* G_OS_WIN32 */
  if (fd == -1)
    goto accept_failed;

  if (getnameinfo (&sa.sa, slen, ip, sizeof (ip), NULL, 0, NI_NUMERICHOST) != 0)
    goto getnameinfo_failed;

  if (sa.sa.sa_family == AF_INET)
    port = sa.sa_in.sin_port;
  else if (sa.sa.sa_family == AF_INET6)
    port = sa.sa_in6.sin6_port;
  else
    goto wrong_family;

  return gst_rtsp_connection_create_from_fd (fd, ip, port, NULL, conn);

  /* ERRORS */
accept_failed:
  {
    return GST_RTSP_ESYS;
  }
getnameinfo_failed:
wrong_family:
  {
    CLOSE_SOCKET (fd);
    return GST_RTSP_ERROR;
  }
}

static gchar *
do_resolve (const gchar * host)
{
  gchar ip[INET6_ADDRSTRLEN];
  struct addrinfo *aires, hints;
  struct addrinfo *ai;
  gint aierr;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;  /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM;       /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;  /* For wildcard IP address */
  hints.ai_protocol = 0;        /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  aierr = getaddrinfo (host, NULL, &hints, &aires);
  if (aierr != 0)
    goto no_addrinfo;

  for (ai = aires; ai; ai = ai->ai_next) {
    if (ai->ai_family == AF_INET || ai->ai_family == AF_INET6) {
      break;
    }
  }
  if (ai == NULL)
    goto no_family;

  aierr = getnameinfo (ai->ai_addr, ai->ai_addrlen, ip, sizeof (ip), NULL, 0,
      NI_NUMERICHOST | NI_NUMERICSERV);
  if (aierr != 0)
    goto no_address;

  freeaddrinfo (aires);

  return g_strdup (ip);

  /* ERRORS */
no_addrinfo:
  {
    GST_ERROR ("no addrinfo found for %s: %s", host, gai_strerror (aierr));
    return NULL;
  }
no_family:
  {
    GST_ERROR ("no family found for %s", host);
    freeaddrinfo (aires);
    return NULL;
  }
no_address:
  {
    GST_ERROR ("no address found for %s: %s", host, gai_strerror (aierr));
    freeaddrinfo (aires);
    return NULL;
  }
}

static GstRTSPResult
do_connect (const gchar * ip, guint16 port, GstPollFD * fdout,
    GstPoll * fdset, GTimeVal * timeout)
{
  gint fd;
  struct addrinfo hints;
  struct addrinfo *aires;
  struct addrinfo *ai;
  gint aierr;
  gchar service[NI_MAXSERV];
  gint ret;
#ifdef G_OS_WIN32
  unsigned long flags = 1;
#endif /* G_OS_WIN32 */
  GstClockTime to;
  gint retval;

  memset (&hints, 0, sizeof hints);
  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  g_snprintf (service, sizeof (service) - 1, "%hu", port);
  service[sizeof (service) - 1] = '\0';

  aierr = getaddrinfo (ip, service, &hints, &aires);
  if (aierr != 0)
    goto no_addrinfo;

  for (ai = aires; ai; ai = ai->ai_next) {
    if (ai->ai_family == AF_INET || ai->ai_family == AF_INET6) {
      break;
    }
  }
  if (ai == NULL)
    goto no_family;

  fd = socket (ai->ai_family, SOCK_STREAM, 0);
  if (fd == -1)
    goto no_socket;

  /* set to non-blocking mode so that we can cancel the connect */
#ifndef G_OS_WIN32
  fcntl (fd, F_SETFL, O_NONBLOCK);
#else
  ioctlsocket (fd, FIONBIO, &flags);
#endif /* G_OS_WIN32 */

  /* add the socket to our fdset */
  ADD_POLLFD (fdset, fdout, fd);

  /* we are going to connect ASYNC now */
  ret = connect (fd, ai->ai_addr, ai->ai_addrlen);
  if (ret == 0)
    goto done;
  if (!ERRNO_IS_EINPROGRESS)
    goto sys_error;

  /* wait for connect to complete up to the specified timeout or until we got
   * interrupted. */
  gst_poll_fd_ctl_write (fdset, fdout, TRUE);

  to = timeout ? GST_TIMEVAL_TO_TIME (*timeout) : GST_CLOCK_TIME_NONE;

  do {
    retval = gst_poll_wait (fdset, to);
  } while (retval == -1 && (errno == EINTR || errno == EAGAIN));

  if (retval == 0)
    goto timeout;
  else if (retval == -1)
    goto sys_error;

  /* we can still have an error connecting on windows */
  if (gst_poll_fd_has_error (fdset, fdout)) {
    socklen_t len = sizeof (errno);
#ifndef G_OS_WIN32
    getsockopt (fd, SOL_SOCKET, SO_ERROR, &errno, &len);
#else
    getsockopt (fd, SOL_SOCKET, SO_ERROR, (char *) &errno, &len);
#endif
    goto sys_error;
  } else {
#ifdef __APPLE__
    /* osx wakes up select with POLLOUT if the connection is refused... */
    socklen_t len = sizeof (errno);
    getsockopt (fd, SOL_SOCKET, SO_ERROR, (char *) &errno, &len);
    if (errno != 0)
      goto sys_error;
#endif
  }

  gst_poll_fd_ignored (fdset, fdout);

done:
  freeaddrinfo (aires);

  return GST_RTSP_OK;

  /* ERRORS */
no_addrinfo:
  {
    GST_ERROR ("no addrinfo found for %s: %s", ip, gai_strerror (aierr));
    return GST_RTSP_ERROR;
  }
no_family:
  {
    GST_ERROR ("no family found for %s", ip);
    freeaddrinfo (aires);
    return GST_RTSP_ERROR;
  }
no_socket:
  {
    GST_ERROR ("no socket %d (%s)", errno, g_strerror (errno));
    freeaddrinfo (aires);
    return GST_RTSP_ESYS;
  }
sys_error:
  {
    GST_ERROR ("system error %d (%s)", errno, g_strerror (errno));
    REMOVE_POLLFD (fdset, fdout);
    freeaddrinfo (aires);
    return GST_RTSP_ESYS;
  }
timeout:
  {
    GST_ERROR ("timeout");
    REMOVE_POLLFD (fdset, fdout);
    freeaddrinfo (aires);
    return GST_RTSP_ETIMEOUT;
  }
}

static GstRTSPResult
setup_tunneling (GstRTSPConnection * conn, GTimeVal * timeout)
{
  gint i;
  GstRTSPResult res;
  gchar *ip;
  gchar *uri;
  gchar *value;
  guint16 port, url_port;
  GstRTSPUrl *url;
  gchar *hostparam;
  GstRTSPMessage *msg;
  GstRTSPMessage response;
  gboolean old_http;

  memset (&response, 0, sizeof (response));
  gst_rtsp_message_init (&response);

  /* create a random sessionid */
  for (i = 0; i < TUNNELID_LEN; i++)
    conn->tunnelid[i] = g_random_int_range ('a', 'z');
  conn->tunnelid[TUNNELID_LEN - 1] = '\0';

  url = conn->url;
  /* get the port from the url */
  gst_rtsp_url_get_port (url, &url_port);

  if (conn->proxy_host) {
    uri = g_strdup_printf ("http://%s:%d%s%s%s", url->host, url_port,
        url->abspath, url->query ? "?" : "", url->query ? url->query : "");
    hostparam = g_strdup_printf ("%s:%d", url->host, url_port);
    ip = conn->proxy_host;
    port = conn->proxy_port;
  } else {
    uri = g_strdup_printf ("%s%s%s", url->abspath, url->query ? "?" : "",
        url->query ? url->query : "");
    hostparam = NULL;
    ip = conn->ip;
    port = url_port;
  }

  /* create the GET request for the read connection */
  GST_RTSP_CHECK (gst_rtsp_message_new_request (&msg, GST_RTSP_GET, uri),
      no_message);
  msg->type = GST_RTSP_MESSAGE_HTTP_REQUEST;

  if (hostparam != NULL)
    gst_rtsp_message_add_header (msg, GST_RTSP_HDR_HOST, hostparam);
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_X_SESSIONCOOKIE,
      conn->tunnelid);
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_ACCEPT,
      "application/x-rtsp-tunnelled");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_CACHE_CONTROL, "no-cache");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_PRAGMA, "no-cache");

  /* we start by writing to this fd */
  conn->writefd = &conn->fd0;

  /* we need to temporarily set conn->tunneled to FALSE to prevent the HTTP
   * request from being base64 encoded */
  conn->tunneled = FALSE;
  GST_RTSP_CHECK (gst_rtsp_connection_send (conn, msg, timeout), write_failed);
  gst_rtsp_message_free (msg);
  conn->tunneled = TRUE;

  /* receive the response to the GET request */
  /* we need to temporarily set manual_http to TRUE since
   * gst_rtsp_connection_receive() will treat the HTTP response as a parsing
   * failure otherwise */
  old_http = conn->manual_http;
  conn->manual_http = TRUE;
  GST_RTSP_CHECK (gst_rtsp_connection_receive (conn, &response, timeout),
      read_failed);
  conn->manual_http = old_http;

  if (response.type != GST_RTSP_MESSAGE_HTTP_RESPONSE ||
      response.type_data.response.code != GST_RTSP_STS_OK)
    goto wrong_result;

  if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_X_SERVER_IP_ADDRESS,
          &value, 0) == GST_RTSP_OK) {
    if (conn->proxy_host) {
      /* if we use a proxy we need to change the destination url */
      g_free (url->host);
      url->host = g_strdup (value);
      g_free (hostparam);
      hostparam = g_strdup_printf ("%s:%d", url->host, url_port);
    } else {
      /* and resolve the new ip address */
      if (!(ip = do_resolve (value)))
        goto not_resolved;
      g_free (conn->ip);
      conn->ip = ip;
    }
  }

  /* connect to the host/port */
  res = do_connect (ip, port, &conn->fd1, conn->fdset, timeout);
  if (res != GST_RTSP_OK)
    goto connect_failed;

  /* this is now our writing socket */
  conn->writefd = &conn->fd1;

  /* create the POST request for the write connection */
  GST_RTSP_CHECK (gst_rtsp_message_new_request (&msg, GST_RTSP_POST, uri),
      no_message);
  msg->type = GST_RTSP_MESSAGE_HTTP_REQUEST;

  if (hostparam != NULL)
    gst_rtsp_message_add_header (msg, GST_RTSP_HDR_HOST, hostparam);
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_X_SESSIONCOOKIE,
      conn->tunnelid);
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_ACCEPT,
      "application/x-rtsp-tunnelled");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_CACHE_CONTROL, "no-cache");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_PRAGMA, "no-cache");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_EXPIRES,
      "Sun, 9 Jan 1972 00:00:00 GMT");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_CONTENT_LENGTH, "32767");

  /* we need to temporarily set conn->tunneled to FALSE to prevent the HTTP
   * request from being base64 encoded */
  conn->tunneled = FALSE;
  GST_RTSP_CHECK (gst_rtsp_connection_send (conn, msg, timeout), write_failed);
  gst_rtsp_message_free (msg);
  conn->tunneled = TRUE;

exit:
  gst_rtsp_message_unset (&response);
  g_free (hostparam);
  g_free (uri);

  return res;

  /* ERRORS */
no_message:
  {
    GST_ERROR ("failed to create request (%d)", res);
    goto exit;
  }
write_failed:
  {
    GST_ERROR ("write failed (%d)", res);
    gst_rtsp_message_free (msg);
    conn->tunneled = TRUE;
    goto exit;
  }
read_failed:
  {
    GST_ERROR ("read failed (%d)", res);
    conn->manual_http = FALSE;
    goto exit;
  }
wrong_result:
  {
    GST_ERROR ("got failure response %d %s", response.type_data.response.code,
        response.type_data.response.reason);
    res = GST_RTSP_ERROR;
    goto exit;
  }
not_resolved:
  {
    GST_ERROR ("could not resolve %s", conn->ip);
    res = GST_RTSP_ENET;
    goto exit;
  }
connect_failed:
  {
    GST_ERROR ("failed to connect");
    goto exit;
  }
}

/**
 * gst_rtsp_connection_connect:
 * @conn: a #GstRTSPConnection 
 * @timeout: a #GTimeVal timeout
 *
 * Attempt to connect to the url of @conn made with
 * gst_rtsp_connection_create(). If @timeout is #NULL this function can block
 * forever. If @timeout contains a valid timeout, this function will return
 * #GST_RTSP_ETIMEOUT after the timeout expired.
 *
 * This function can be cancelled with gst_rtsp_connection_flush().
 *
 * Returns: #GST_RTSP_OK when a connection could be made.
 */
GstRTSPResult
gst_rtsp_connection_connect (GstRTSPConnection * conn, GTimeVal * timeout)
{
  GstRTSPResult res;
  gchar *ip;
  guint16 port;
  GstRTSPUrl *url;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->url != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->fd0.fd < 0, GST_RTSP_EINVAL);

  url = conn->url;

  if (conn->proxy_host && conn->tunneled) {
    if (!(ip = do_resolve (conn->proxy_host))) {
      GST_ERROR ("could not resolve %s", conn->proxy_host);
      goto not_resolved;
    }
    port = conn->proxy_port;
    g_free (conn->proxy_host);
    conn->proxy_host = ip;
  } else {
    if (!(ip = do_resolve (url->host))) {
      GST_ERROR ("could not resolve %s", url->host);
      goto not_resolved;
    }
    /* get the port from the url */
    gst_rtsp_url_get_port (url, &port);

    g_free (conn->ip);
    conn->ip = ip;
  }

  /* connect to the host/port */
  res = do_connect (ip, port, &conn->fd0, conn->fdset, timeout);
  if (res != GST_RTSP_OK)
    goto connect_failed;

  /* this is our read URL */
  conn->readfd = &conn->fd0;

  if (conn->tunneled) {
    res = setup_tunneling (conn, timeout);
    if (res != GST_RTSP_OK)
      goto tunneling_failed;
  } else {
    conn->writefd = &conn->fd0;
  }

  return GST_RTSP_OK;

not_resolved:
  {
    return GST_RTSP_ENET;
  }
connect_failed:
  {
    GST_ERROR ("failed to connect");
    return res;
  }
tunneling_failed:
  {
    GST_ERROR ("failed to setup tunneling");
    return res;
  }
}

static void
auth_digest_compute_hex_urp (const gchar * username,
    const gchar * realm, const gchar * password, gchar hex_urp[33])
{
  GChecksum *md5_context = g_checksum_new (G_CHECKSUM_MD5);
  const gchar *digest_string;

  g_checksum_update (md5_context, (const guchar *) username, strlen (username));
  g_checksum_update (md5_context, (const guchar *) ":", 1);
  g_checksum_update (md5_context, (const guchar *) realm, strlen (realm));
  g_checksum_update (md5_context, (const guchar *) ":", 1);
  g_checksum_update (md5_context, (const guchar *) password, strlen (password));
  digest_string = g_checksum_get_string (md5_context);

  memset (hex_urp, 0, 33);
  memcpy (hex_urp, digest_string, strlen (digest_string));

  g_checksum_free (md5_context);
}

static void
auth_digest_compute_response (const gchar * method,
    const gchar * uri, const gchar * hex_a1, const gchar * nonce,
    gchar response[33])
{
  char hex_a2[33] = { 0, };
  GChecksum *md5_context = g_checksum_new (G_CHECKSUM_MD5);
  const gchar *digest_string;

  /* compute A2 */
  g_checksum_update (md5_context, (const guchar *) method, strlen (method));
  g_checksum_update (md5_context, (const guchar *) ":", 1);
  g_checksum_update (md5_context, (const guchar *) uri, strlen (uri));
  digest_string = g_checksum_get_string (md5_context);
  memcpy (hex_a2, digest_string, strlen (digest_string));

  /* compute KD */
  g_checksum_reset (md5_context);
  g_checksum_update (md5_context, (const guchar *) hex_a1, strlen (hex_a1));
  g_checksum_update (md5_context, (const guchar *) ":", 1);
  g_checksum_update (md5_context, (const guchar *) nonce, strlen (nonce));
  g_checksum_update (md5_context, (const guchar *) ":", 1);

  g_checksum_update (md5_context, (const guchar *) hex_a2, 32);
  digest_string = g_checksum_get_string (md5_context);
  memset (response, 0, 33);
  memcpy (response, digest_string, strlen (digest_string));

  g_checksum_free (md5_context);
}

static void
add_auth_header (GstRTSPConnection * conn, GstRTSPMessage * message)
{
  switch (conn->auth_method) {
    case GST_RTSP_AUTH_BASIC:{
      gchar *user_pass;
      gchar *user_pass64;
      gchar *auth_string;

      if (conn->username == NULL || conn->passwd == NULL)
        break;

      user_pass = g_strdup_printf ("%s:%s", conn->username, conn->passwd);
      user_pass64 = g_base64_encode ((guchar *) user_pass, strlen (user_pass));
      auth_string = g_strdup_printf ("Basic %s", user_pass64);

      gst_rtsp_message_take_header (message, GST_RTSP_HDR_AUTHORIZATION,
          auth_string);

      g_free (user_pass);
      g_free (user_pass64);
      break;
    }
    case GST_RTSP_AUTH_DIGEST:{
      gchar response[33], hex_urp[33];
      gchar *auth_string, *auth_string2;
      gchar *realm;
      gchar *nonce;
      gchar *opaque;
      const gchar *uri;
      const gchar *method;

      /* we need to have some params set */
      if (conn->auth_params == NULL || conn->username == NULL ||
          conn->passwd == NULL)
        break;

      /* we need the realm and nonce */
      realm = (gchar *) g_hash_table_lookup (conn->auth_params, "realm");
      nonce = (gchar *) g_hash_table_lookup (conn->auth_params, "nonce");
      if (realm == NULL || nonce == NULL)
        break;

      auth_digest_compute_hex_urp (conn->username, realm, conn->passwd,
          hex_urp);

      method = gst_rtsp_method_as_text (message->type_data.request.method);
      uri = message->type_data.request.uri;

      /* Assume no qop, algorithm=md5, stale=false */
      /* For algorithm MD5, a1 = urp. */
      auth_digest_compute_response (method, uri, hex_urp, nonce, response);
      auth_string = g_strdup_printf ("Digest username=\"%s\", "
          "realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"",
          conn->username, realm, nonce, uri, response);

      opaque = (gchar *) g_hash_table_lookup (conn->auth_params, "opaque");
      if (opaque) {
        auth_string2 = g_strdup_printf ("%s, opaque=\"%s\"", auth_string,
            opaque);
        g_free (auth_string);
        auth_string = auth_string2;
      }
      gst_rtsp_message_take_header (message, GST_RTSP_HDR_AUTHORIZATION,
          auth_string);
      break;
    }
    default:
      /* Nothing to do */
      break;
  }
}

static void
gen_date_string (gchar * date_string, guint len)
{
  static const char wkdays[7][4] =
      { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char months[12][4] =
      { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
    "Nov", "Dec"
  };
  struct tm tm;
  time_t t;

  time (&t);

#ifdef HAVE_GMTIME_R
  gmtime_r (&t, &tm);
#else
  tm = *gmtime (&t);
#endif

  g_snprintf (date_string, len, "%s, %02d %s %04d %02d:%02d:%02d GMT",
      wkdays[tm.tm_wday], tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900,
      tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static GstRTSPResult
write_bytes (gint fd, const guint8 * buffer, guint * idx, guint size)
{
  guint left;

  if (G_UNLIKELY (*idx > size))
    return GST_RTSP_ERROR;

  left = size - *idx;

  while (left) {
    gint r;

    r = WRITE_SOCKET (fd, &buffer[*idx], left);
    if (G_UNLIKELY (r == 0)) {
      return GST_RTSP_EINTR;
    } else if (G_UNLIKELY (r < 0)) {
      if (ERRNO_IS_EAGAIN)
        return GST_RTSP_EINTR;
      if (!ERRNO_IS_EINTR)
        return GST_RTSP_ESYS;
    } else {
      left -= r;
      *idx += r;
    }
  }
  return GST_RTSP_OK;
}

static gint
fill_raw_bytes (GstRTSPConnection * conn, guint8 * buffer, guint size)
{
  gint out = 0;

  if (G_UNLIKELY (conn->initial_buffer != NULL)) {
    gsize left = strlen (&conn->initial_buffer[conn->initial_buffer_offset]);

    out = MIN (left, size);
    memcpy (buffer, &conn->initial_buffer[conn->initial_buffer_offset], out);

    if (left == (gsize) out) {
      g_free (conn->initial_buffer);
      conn->initial_buffer = NULL;
      conn->initial_buffer_offset = 0;
    } else
      conn->initial_buffer_offset += out;
  }

  if (G_LIKELY (size > (guint) out)) {
    gint r;

    r = READ_SOCKET (conn->readfd->fd, &buffer[out], size - out);
    if (r <= 0) {
      if (out == 0)
        out = r;
    } else
      out += r;
  }

  return out;
}

static gint
fill_bytes (GstRTSPConnection * conn, guint8 * buffer, guint size)
{
  DecodeCtx *ctx = conn->ctxp;
  gint out = 0;

  if (ctx) {
    while (size > 0) {
      guint8 in[sizeof (ctx->out) * 4 / 3];
      gint r;

      while (size > 0 && ctx->cout < ctx->coutl) {
        /* we have some leftover bytes */
        *buffer++ = ctx->out[ctx->cout++];
        size--;
        out++;
      }

      /* got what we needed? */
      if (size == 0)
        break;

      /* try to read more bytes */
      r = fill_raw_bytes (conn, in, sizeof (in));
      if (r <= 0) {
        if (out == 0)
          out = r;
        break;
      }

      ctx->cout = 0;
      ctx->coutl =
          g_base64_decode_step ((gchar *) in, r, ctx->out, &ctx->state,
          &ctx->save);
    }
  } else {
    out = fill_raw_bytes (conn, buffer, size);
  }

  return out;
}

static GstRTSPResult
read_bytes (GstRTSPConnection * conn, guint8 * buffer, guint * idx, guint size)
{
  guint left;

  if (G_UNLIKELY (*idx > size))
    return GST_RTSP_ERROR;

  left = size - *idx;

  while (left) {
    gint r;

    r = fill_bytes (conn, &buffer[*idx], left);
    if (G_UNLIKELY (r == 0)) {
      return GST_RTSP_EEOF;
    } else if (G_UNLIKELY (r < 0)) {
      if (ERRNO_IS_EAGAIN)
        return GST_RTSP_EINTR;
      if (!ERRNO_IS_EINTR)
        return GST_RTSP_ESYS;
    } else {
      left -= r;
      *idx += r;
    }
  }
  return GST_RTSP_OK;
}

/* The code below tries to handle clients using \r, \n or \r\n to indicate the
 * end of a line. It even does its best to handle clients which mix them (even
 * though this is a really stupid idea (tm).) It also handles Line White Space
 * (LWS), where a line end followed by whitespace is considered LWS. This is
 * the method used in RTSP (and HTTP) to break long lines.
 */
static GstRTSPResult
read_line (GstRTSPConnection * conn, guint8 * buffer, guint * idx, guint size)
{
  while (TRUE) {
    guint8 c;
    gint r;

    if (conn->read_ahead == READ_AHEAD_EOH) {
      /* the last call to read_line() already determined that we have reached
       * the end of the headers, so convey that information now */
      conn->read_ahead = 0;
      break;
    } else if (conn->read_ahead == READ_AHEAD_CRLF) {
      /* the last call to read_line() left off after having read \r\n */
      c = '\n';
    } else if (conn->read_ahead == READ_AHEAD_CRLFCR) {
      /* the last call to read_line() left off after having read \r\n\r */
      c = '\r';
    } else if (conn->read_ahead != 0) {
      /* the last call to read_line() left us with a character to start with */
      c = (guint8) conn->read_ahead;
      conn->read_ahead = 0;
    } else {
      /* read the next character */
      r = fill_bytes (conn, &c, 1);
      if (G_UNLIKELY (r == 0)) {
        return GST_RTSP_EEOF;
      } else if (G_UNLIKELY (r < 0)) {
        if (ERRNO_IS_EAGAIN)
          return GST_RTSP_EINTR;
        if (!ERRNO_IS_EINTR)
          return GST_RTSP_ESYS;
        continue;
      }
    }

    /* special treatment of line endings */
    if (c == '\r' || c == '\n') {
      guint8 read_ahead;

    retry:
      /* need to read ahead one more character to know what to do... */
      r = fill_bytes (conn, &read_ahead, 1);
      if (G_UNLIKELY (r == 0)) {
        return GST_RTSP_EEOF;
      } else if (G_UNLIKELY (r < 0)) {
        if (ERRNO_IS_EAGAIN) {
          /* remember the original character we read and try again next time */
          if (conn->read_ahead == 0)
            conn->read_ahead = c;
          return GST_RTSP_EINTR;
        }
        if (!ERRNO_IS_EINTR)
          return GST_RTSP_ESYS;
        goto retry;
      }

      if (read_ahead == ' ' || read_ahead == '\t') {
        if (conn->read_ahead == READ_AHEAD_CRLFCR) {
          /* got \r\n\r followed by whitespace, treat it as a normal line
           * followed by one starting with LWS */
          conn->read_ahead = read_ahead;
          break;
        } else {
          /* got LWS, change the line ending to a space and continue */
          c = ' ';
          conn->read_ahead = read_ahead;
        }
      } else if (conn->read_ahead == READ_AHEAD_CRLFCR) {
        if (read_ahead == '\r' || read_ahead == '\n') {
          /* got \r\n\r\r or \r\n\r\n, treat it as the end of the headers */
          conn->read_ahead = READ_AHEAD_EOH;
          break;
        } else {
          /* got \r\n\r followed by something else, this is not really
           * supported since we have probably just eaten the first character
           * of the body or the next message, so just ignore the second \r
           * and live with it... */
          conn->read_ahead = read_ahead;
          break;
        }
      } else if (conn->read_ahead == READ_AHEAD_CRLF) {
        if (read_ahead == '\r') {
          /* got \r\n\r so far, need one more character... */
          conn->read_ahead = READ_AHEAD_CRLFCR;
          goto retry;
        } else if (read_ahead == '\n') {
          /* got \r\n\n, treat it as the end of the headers */
          conn->read_ahead = READ_AHEAD_EOH;
          break;
        } else {
          /* found the end of a line, keep read_ahead for the next line */
          conn->read_ahead = read_ahead;
          break;
        }
      } else if (c == read_ahead) {
        /* got double \r or \n, treat it as the end of the headers */
        conn->read_ahead = READ_AHEAD_EOH;
        break;
      } else if (c == '\r' && read_ahead == '\n') {
        /* got \r\n so far, still need more to know what to do... */
        conn->read_ahead = READ_AHEAD_CRLF;
        goto retry;
      } else {
        /* found the end of a line, keep read_ahead for the next line */
        conn->read_ahead = read_ahead;
        break;
      }
    }

    if (G_LIKELY (*idx < size - 1))
      buffer[(*idx)++] = c;
  }
  buffer[*idx] = '\0';

  return GST_RTSP_OK;
}

/**
 * gst_rtsp_connection_write:
 * @conn: a #GstRTSPConnection
 * @data: the data to write
 * @size: the size of @data
 * @timeout: a timeout value or #NULL
 *
 * Attempt to write @size bytes of @data to the connected @conn, blocking up to
 * the specified @timeout. @timeout can be #NULL, in which case this function
 * might block forever.
 * 
 * This function can be cancelled with gst_rtsp_connection_flush().
 *
 * Returns: #GST_RTSP_OK on success.
 */
GstRTSPResult
gst_rtsp_connection_write (GstRTSPConnection * conn, const guint8 * data,
    guint size, GTimeVal * timeout)
{
  guint offset;
  gint retval;
  GstClockTime to;
  GstRTSPResult res;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (data != NULL || size == 0, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->writefd != NULL, GST_RTSP_EINVAL);

  gst_poll_set_controllable (conn->fdset, TRUE);
  gst_poll_fd_ctl_write (conn->fdset, conn->writefd, TRUE);
  gst_poll_fd_ctl_read (conn->fdset, conn->readfd, FALSE);
  /* clear all previous poll results */
  gst_poll_fd_ignored (conn->fdset, conn->writefd);
  gst_poll_fd_ignored (conn->fdset, conn->readfd);

  to = timeout ? GST_TIMEVAL_TO_TIME (*timeout) : GST_CLOCK_TIME_NONE;

  offset = 0;

  while (TRUE) {
    /* try to write */
    res = write_bytes (conn->writefd->fd, data, &offset, size);
    if (G_LIKELY (res == GST_RTSP_OK))
      break;
    if (G_UNLIKELY (res != GST_RTSP_EINTR))
      goto write_error;

    /* not all is written, wait until we can write more */
    do {
      retval = gst_poll_wait (conn->fdset, to);
    } while (retval == -1 && (errno == EINTR || errno == EAGAIN));

    if (G_UNLIKELY (retval == 0))
      goto timeout;

    if (G_UNLIKELY (retval == -1)) {
      if (errno == EBUSY)
        goto stopped;
      else
        goto select_error;
    }

    /* could also be an error with read socket */
    if (gst_poll_fd_has_error (conn->fdset, conn->readfd))
      goto socket_error;
  }
  return GST_RTSP_OK;

  /* ERRORS */
timeout:
  {
    return GST_RTSP_ETIMEOUT;
  }
select_error:
  {
    return GST_RTSP_ESYS;
  }
stopped:
  {
    return GST_RTSP_EINTR;
  }
socket_error:
  {
    return GST_RTSP_ENET;
  }
write_error:
  {
    return res;
  }
}

static GString *
message_to_string (GstRTSPConnection * conn, GstRTSPMessage * message)
{
  GString *str = NULL;

  str = g_string_new ("");

  switch (message->type) {
    case GST_RTSP_MESSAGE_REQUEST:
      /* create request string, add CSeq */
      g_string_append_printf (str, "%s %s RTSP/1.0\r\n"
          "CSeq: %d\r\n",
          gst_rtsp_method_as_text (message->type_data.request.method),
          message->type_data.request.uri, conn->cseq++);
      /* add session id if we have one */
      if (conn->session_id[0] != '\0') {
        gst_rtsp_message_remove_header (message, GST_RTSP_HDR_SESSION, -1);
        gst_rtsp_message_add_header (message, GST_RTSP_HDR_SESSION,
            conn->session_id);
      }
      /* add any authentication headers */
      add_auth_header (conn, message);
      break;
    case GST_RTSP_MESSAGE_RESPONSE:
      /* create response string */
      g_string_append_printf (str, "RTSP/1.0 %d %s\r\n",
          message->type_data.response.code, message->type_data.response.reason);
      break;
    case GST_RTSP_MESSAGE_HTTP_REQUEST:
      /* create request string */
      g_string_append_printf (str, "%s %s HTTP/%s\r\n",
          gst_rtsp_method_as_text (message->type_data.request.method),
          message->type_data.request.uri,
          gst_rtsp_version_as_text (message->type_data.request.version));
      /* add any authentication headers */
      add_auth_header (conn, message);
      break;
    case GST_RTSP_MESSAGE_HTTP_RESPONSE:
      /* create response string */
      g_string_append_printf (str, "HTTP/%s %d %s\r\n",
          gst_rtsp_version_as_text (message->type_data.request.version),
          message->type_data.response.code, message->type_data.response.reason);
      break;
    case GST_RTSP_MESSAGE_DATA:
    {
      guint8 data_header[4];

      /* prepare data header */
      data_header[0] = '$';
      data_header[1] = message->type_data.data.channel;
      data_header[2] = (message->body_size >> 8) & 0xff;
      data_header[3] = message->body_size & 0xff;

      /* create string with header and data */
      str = g_string_append_len (str, (gchar *) data_header, 4);
      str =
          g_string_append_len (str, (gchar *) message->body,
          message->body_size);
      break;
    }
    default:
      g_string_free (str, TRUE);
      g_return_val_if_reached (NULL);
      break;
  }

  /* append headers and body */
  if (message->type != GST_RTSP_MESSAGE_DATA) {
    gchar date_string[100];

    gen_date_string (date_string, sizeof (date_string));

    /* add date header */
    gst_rtsp_message_remove_header (message, GST_RTSP_HDR_DATE, -1);
    gst_rtsp_message_add_header (message, GST_RTSP_HDR_DATE, date_string);

    /* append headers */
    gst_rtsp_message_append_headers (message, str);

    /* append Content-Length and body if needed */
    if (message->body != NULL && message->body_size > 0) {
      gchar *len;

      len = g_strdup_printf ("%d", message->body_size);
      g_string_append_printf (str, "%s: %s\r\n",
          gst_rtsp_header_as_text (GST_RTSP_HDR_CONTENT_LENGTH), len);
      g_free (len);
      /* header ends here */
      g_string_append (str, "\r\n");
      str =
          g_string_append_len (str, (gchar *) message->body,
          message->body_size);
    } else {
      /* just end headers */
      g_string_append (str, "\r\n");
    }
  }

  return str;
}

/**
 * gst_rtsp_connection_send:
 * @conn: a #GstRTSPConnection
 * @message: the message to send
 * @timeout: a timeout value or #NULL
 *
 * Attempt to send @message to the connected @conn, blocking up to
 * the specified @timeout. @timeout can be #NULL, in which case this function
 * might block forever.
 * 
 * This function can be cancelled with gst_rtsp_connection_flush().
 *
 * Returns: #GST_RTSP_OK on success.
 */
GstRTSPResult
gst_rtsp_connection_send (GstRTSPConnection * conn, GstRTSPMessage * message,
    GTimeVal * timeout)
{
  GString *string = NULL;
  GstRTSPResult res;
  gchar *str;
  gsize len;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (message != NULL, GST_RTSP_EINVAL);

  if (G_UNLIKELY (!(string = message_to_string (conn, message))))
    goto no_message;

  if (conn->tunneled) {
    str = g_base64_encode ((const guchar *) string->str, string->len);
    g_string_free (string, TRUE);
    len = strlen (str);
  } else {
    str = string->str;
    len = string->len;
    g_string_free (string, FALSE);
  }

  /* write request */
  res = gst_rtsp_connection_write (conn, (guint8 *) str, len, timeout);

  g_free (str);

  return res;

no_message:
  {
    g_warning ("Wrong message");
    return GST_RTSP_EINVAL;
  }
}

static GstRTSPResult
parse_string (gchar * dest, gint size, gchar ** src)
{
  GstRTSPResult res = GST_RTSP_OK;
  gint idx;

  idx = 0;
  /* skip spaces */
  while (g_ascii_isspace (**src))
    (*src)++;

  while (!g_ascii_isspace (**src) && **src != '\0') {
    if (idx < size - 1)
      dest[idx++] = **src;
    else
      res = GST_RTSP_EPARSE;
    (*src)++;
  }
  if (size > 0)
    dest[idx] = '\0';

  return res;
}

static GstRTSPResult
parse_protocol_version (gchar * protocol, GstRTSPMsgType * type,
    GstRTSPVersion * version)
{
  GstRTSPResult res = GST_RTSP_OK;
  gchar *ver;

  if (G_LIKELY ((ver = strchr (protocol, '/')) != NULL)) {
    guint major;
    guint minor;
    gchar dummychar;

    *ver++ = '\0';

    /* the version number must be formatted as X.Y with nothing following */
    if (sscanf (ver, "%u.%u%c", &major, &minor, &dummychar) != 2)
      res = GST_RTSP_EPARSE;

    if (g_ascii_strcasecmp (protocol, "RTSP") == 0) {
      if (major != 1 || minor != 0) {
        *version = GST_RTSP_VERSION_INVALID;
        res = GST_RTSP_ERROR;
      }
    } else if (g_ascii_strcasecmp (protocol, "HTTP") == 0) {
      if (*type == GST_RTSP_MESSAGE_REQUEST)
        *type = GST_RTSP_MESSAGE_HTTP_REQUEST;
      else if (*type == GST_RTSP_MESSAGE_RESPONSE)
        *type = GST_RTSP_MESSAGE_HTTP_RESPONSE;

      if (major == 1 && minor == 1) {
        *version = GST_RTSP_VERSION_1_1;
      } else if (major != 1 || minor != 0) {
        *version = GST_RTSP_VERSION_INVALID;
        res = GST_RTSP_ERROR;
      }
    } else
      res = GST_RTSP_EPARSE;
  } else
    res = GST_RTSP_EPARSE;

  return res;
}

static GstRTSPResult
parse_response_status (guint8 * buffer, GstRTSPMessage * msg)
{
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPResult res2;
  gchar versionstr[20];
  gchar codestr[4];
  gint code;
  gchar *bptr;

  bptr = (gchar *) buffer;

  if (parse_string (versionstr, sizeof (versionstr), &bptr) != GST_RTSP_OK)
    res = GST_RTSP_EPARSE;

  if (parse_string (codestr, sizeof (codestr), &bptr) != GST_RTSP_OK)
    res = GST_RTSP_EPARSE;
  code = atoi (codestr);
  if (G_UNLIKELY (*codestr == '\0' || code < 0 || code >= 600))
    res = GST_RTSP_EPARSE;

  while (g_ascii_isspace (*bptr))
    bptr++;

  if (G_UNLIKELY (gst_rtsp_message_init_response (msg, code, bptr,
              NULL) != GST_RTSP_OK))
    res = GST_RTSP_EPARSE;

  res2 = parse_protocol_version (versionstr, &msg->type,
      &msg->type_data.response.version);
  if (G_LIKELY (res == GST_RTSP_OK))
    res = res2;

  return res;
}

static GstRTSPResult
parse_request_line (guint8 * buffer, GstRTSPMessage * msg)
{
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPResult res2;
  gchar versionstr[20];
  gchar methodstr[20];
  gchar urlstr[4096];
  gchar *bptr;
  GstRTSPMethod method;

  bptr = (gchar *) buffer;

  if (parse_string (methodstr, sizeof (methodstr), &bptr) != GST_RTSP_OK)
    res = GST_RTSP_EPARSE;
  method = gst_rtsp_find_method (methodstr);

  if (parse_string (urlstr, sizeof (urlstr), &bptr) != GST_RTSP_OK)
    res = GST_RTSP_EPARSE;
  if (G_UNLIKELY (*urlstr == '\0'))
    res = GST_RTSP_EPARSE;

  if (parse_string (versionstr, sizeof (versionstr), &bptr) != GST_RTSP_OK)
    res = GST_RTSP_EPARSE;

  if (G_UNLIKELY (*bptr != '\0'))
    res = GST_RTSP_EPARSE;

  if (G_UNLIKELY (gst_rtsp_message_init_request (msg, method,
              urlstr) != GST_RTSP_OK))
    res = GST_RTSP_EPARSE;

  res2 = parse_protocol_version (versionstr, &msg->type,
      &msg->type_data.request.version);
  if (G_LIKELY (res == GST_RTSP_OK))
    res = res2;

  if (G_LIKELY (msg->type == GST_RTSP_MESSAGE_REQUEST)) {
    /* GET and POST are not allowed as RTSP methods */
    if (msg->type_data.request.method == GST_RTSP_GET ||
        msg->type_data.request.method == GST_RTSP_POST) {
      msg->type_data.request.method = GST_RTSP_INVALID;
      if (res == GST_RTSP_OK)
        res = GST_RTSP_ERROR;
    }
  } else if (msg->type == GST_RTSP_MESSAGE_HTTP_REQUEST) {
    /* only GET and POST are allowed as HTTP methods */
    if (msg->type_data.request.method != GST_RTSP_GET &&
        msg->type_data.request.method != GST_RTSP_POST) {
      msg->type_data.request.method = GST_RTSP_INVALID;
      if (res == GST_RTSP_OK)
        res = GST_RTSP_ERROR;
    }
  }

  return res;
}

/* parsing lines means reading a Key: Value pair */
static GstRTSPResult
parse_line (guint8 * buffer, GstRTSPMessage * msg)
{
  GstRTSPHeaderField field;
  gchar *line = (gchar *) buffer;
  gchar *value;

  if ((value = strchr (line, ':')) == NULL || value == line)
    goto parse_error;

  /* trim space before the colon */
  if (value[-1] == ' ')
    value[-1] = '\0';

  /* replace the colon with a NUL */
  *value++ = '\0';

  /* find the header */
  field = gst_rtsp_find_header_field (line);
  if (field == GST_RTSP_HDR_INVALID)
    goto done;

  /* split up the value in multiple key:value pairs if it contains comma(s) */
  while (*value != '\0') {
    gchar *next_value;
    gchar *comma = NULL;
    gboolean quoted = FALSE;
    guint comment = 0;

    /* trim leading space */
    if (*value == ' ')
      value++;

    /* for headers which may not appear multiple times, and thus may not
     * contain multiple values on the same line, we can short-circuit the loop
     * below and the entire value results in just one key:value pair*/
    if (!gst_rtsp_header_allow_multiple (field))
      next_value = value + strlen (value);
    else
      next_value = value;

    /* find the next value, taking special care of quotes and comments */
    while (*next_value != '\0') {
      if ((quoted || comment != 0) && *next_value == '\\' &&
          next_value[1] != '\0')
        next_value++;
      else if (comment == 0 && *next_value == '"')
        quoted = !quoted;
      else if (!quoted && *next_value == '(')
        comment++;
      else if (comment != 0 && *next_value == ')')
        comment--;
      else if (!quoted && comment == 0) {
        /* To quote RFC 2068: "User agents MUST take special care in parsing
         * the WWW-Authenticate field value if it contains more than one
         * challenge, or if more than one WWW-Authenticate header field is
         * provided, since the contents of a challenge may itself contain a
         * comma-separated list of authentication parameters."
         *
         * What this means is that we cannot just look for an unquoted comma
         * when looking for multiple values in Proxy-Authenticate and
         * WWW-Authenticate headers. Instead we need to look for the sequence
         * "comma [space] token space token" before we can split after the
         * comma...
         */
        if (field == GST_RTSP_HDR_PROXY_AUTHENTICATE ||
            field == GST_RTSP_HDR_WWW_AUTHENTICATE) {
          if (*next_value == ',') {
            if (next_value[1] == ' ') {
              /* skip any space following the comma so we do not mistake it for
               * separating between two tokens */
              next_value++;
            }
            comma = next_value;
          } else if (*next_value == ' ' && next_value[1] != ',' &&
              next_value[1] != '=' && comma != NULL) {
            next_value = comma;
            comma = NULL;
            break;
          }
        } else if (*next_value == ',')
          break;
      }

      next_value++;
    }

    /* trim space */
    if (value != next_value && next_value[-1] == ' ')
      next_value[-1] = '\0';

    if (*next_value != '\0')
      *next_value++ = '\0';

    /* add the key:value pair */
    if (*value != '\0')
      gst_rtsp_message_add_header (msg, field, value);

    value = next_value;
  }

done:
  return GST_RTSP_OK;

  /* ERRORS */
parse_error:
  {
    return GST_RTSP_EPARSE;
  }
}

/* convert all consecutive whitespace to a single space */
static void
normalize_line (guint8 * buffer)
{
  while (*buffer) {
    if (g_ascii_isspace (*buffer)) {
      guint8 *tmp;

      *buffer++ = ' ';
      for (tmp = buffer; g_ascii_isspace (*tmp); tmp++) {
      }
      if (buffer != tmp)
        memmove (buffer, tmp, strlen ((gchar *) tmp) + 1);
    } else {
      buffer++;
    }
  }
}

/* returns:
 *  GST_RTSP_OK when a complete message was read.
 *  GST_RTSP_EEOF: when the read socket is closed
 *  GST_RTSP_EINTR: when more data is needed.
 *  GST_RTSP_..: some other error occured.
 */
static GstRTSPResult
build_next (GstRTSPBuilder * builder, GstRTSPMessage * message,
    GstRTSPConnection * conn)
{
  GstRTSPResult res;

  while (TRUE) {
    switch (builder->state) {
      case STATE_START:
      {
        guint8 c;

        builder->offset = 0;
        res =
            read_bytes (conn, (guint8 *) builder->buffer, &builder->offset, 1);
        if (res != GST_RTSP_OK)
          goto done;

        c = builder->buffer[0];

        /* we have 1 bytes now and we can see if this is a data message or
         * not */
        if (c == '$') {
          /* data message, prepare for the header */
          builder->state = STATE_DATA_HEADER;
        } else if (c == '\n' || c == '\r') {
          /* skip \n and \r */
          builder->offset = 0;
        } else {
          builder->line = 0;
          builder->state = STATE_READ_LINES;
        }
        break;
      }
      case STATE_DATA_HEADER:
      {
        res =
            read_bytes (conn, (guint8 *) builder->buffer, &builder->offset, 4);
        if (res != GST_RTSP_OK)
          goto done;

        gst_rtsp_message_init_data (message, builder->buffer[1]);

        builder->body_len = (builder->buffer[2] << 8) | builder->buffer[3];
        builder->body_data = g_malloc (builder->body_len + 1);
        builder->body_data[builder->body_len] = '\0';
        builder->offset = 0;
        builder->state = STATE_DATA_BODY;
        break;
      }
      case STATE_DATA_BODY:
      {
        res =
            read_bytes (conn, builder->body_data, &builder->offset,
            builder->body_len);
        if (res != GST_RTSP_OK)
          goto done;

        /* we have the complete body now, store in the message adjusting the
         * length to include the trailing '\0' */
        gst_rtsp_message_take_body (message,
            (guint8 *) builder->body_data, builder->body_len + 1);
        builder->body_data = NULL;
        builder->body_len = 0;

        builder->state = STATE_END;
        break;
      }
      case STATE_READ_LINES:
      {
        res = read_line (conn, builder->buffer, &builder->offset,
            sizeof (builder->buffer));
        if (res != GST_RTSP_OK)
          goto done;

        /* we have a regular response */
        if (builder->buffer[0] == '\0') {
          gchar *hdrval;

          /* empty line, end of message header */
          /* see if there is a Content-Length header, but ignore it if this
           * is a POST request with an x-sessioncookie header */
          if (gst_rtsp_message_get_header (message,
                  GST_RTSP_HDR_CONTENT_LENGTH, &hdrval, 0) == GST_RTSP_OK &&
              (message->type != GST_RTSP_MESSAGE_HTTP_REQUEST ||
                  message->type_data.request.method != GST_RTSP_POST ||
                  gst_rtsp_message_get_header (message,
                      GST_RTSP_HDR_X_SESSIONCOOKIE, NULL, 0) != GST_RTSP_OK)) {
            /* there is, prepare to read the body */
            builder->body_len = atol (hdrval);
            builder->body_data = g_try_malloc (builder->body_len + 1);
            /* we can't do much here, we need the length to know how many bytes
             * we need to read next and when allocation fails, something is
             * probably wrong with the length. */
            if (builder->body_data == NULL)
              goto invalid_body_len;

            builder->body_data[builder->body_len] = '\0';
            builder->offset = 0;
            builder->state = STATE_DATA_BODY;
          } else {
            builder->state = STATE_END;
          }
          break;
        }

        /* we have a line */
        normalize_line (builder->buffer);
        if (builder->line == 0) {
          /* first line, check for response status */
          if (memcmp (builder->buffer, "RTSP", 4) == 0 ||
              memcmp (builder->buffer, "HTTP", 4) == 0) {
            builder->status = parse_response_status (builder->buffer, message);
          } else {
            builder->status = parse_request_line (builder->buffer, message);
          }
        } else {
          /* else just parse the line */
          res = parse_line (builder->buffer, message);
          if (res != GST_RTSP_OK)
            builder->status = res;
        }
        builder->line++;
        builder->offset = 0;
        break;
      }
      case STATE_END:
      {
        gchar *session_cookie;
        gchar *session_id;

        if (message->type == GST_RTSP_MESSAGE_DATA) {
          /* data messages don't have headers */
          res = GST_RTSP_OK;
          goto done;
        }

        /* save the tunnel session in the connection */
        if (message->type == GST_RTSP_MESSAGE_HTTP_REQUEST &&
            !conn->manual_http &&
            conn->tstate == TUNNEL_STATE_NONE &&
            gst_rtsp_message_get_header (message, GST_RTSP_HDR_X_SESSIONCOOKIE,
                &session_cookie, 0) == GST_RTSP_OK) {
          strncpy (conn->tunnelid, session_cookie, TUNNELID_LEN);
          conn->tunnelid[TUNNELID_LEN - 1] = '\0';
          conn->tunneled = TRUE;
        }

        /* save session id in the connection for further use */
        if (message->type == GST_RTSP_MESSAGE_RESPONSE &&
            gst_rtsp_message_get_header (message, GST_RTSP_HDR_SESSION,
                &session_id, 0) == GST_RTSP_OK) {
          gint maxlen, i;

          maxlen = sizeof (conn->session_id) - 1;
          /* the sessionid can have attributes marked with ;
           * Make sure we strip them */
          for (i = 0; session_id[i] != '\0'; i++) {
            if (session_id[i] == ';') {
              maxlen = i;
              /* parse timeout */
              do {
                i++;
              } while (g_ascii_isspace (session_id[i]));
              if (g_str_has_prefix (&session_id[i], "timeout=")) {
                gint to;

                /* if we parsed something valid, configure */
                if ((to = atoi (&session_id[i + 8])) > 0)
                  conn->timeout = to;
              }
              break;
            }
          }

          /* make sure to not overflow */
          strncpy (conn->session_id, session_id, maxlen);
          conn->session_id[maxlen] = '\0';
        }
        res = builder->status;
        goto done;
      }
      default:
        res = GST_RTSP_ERROR;
        break;
    }
  }
done:
  return res;

  /* ERRORS */
invalid_body_len:
  {
    GST_DEBUG ("could not allocate body");
    return GST_RTSP_ERROR;
  }
}

/**
 * gst_rtsp_connection_read:
 * @conn: a #GstRTSPConnection
 * @data: the data to read
 * @size: the size of @data
 * @timeout: a timeout value or #NULL
 *
 * Attempt to read @size bytes into @data from the connected @conn, blocking up to
 * the specified @timeout. @timeout can be #NULL, in which case this function
 * might block forever.
 *
 * This function can be cancelled with gst_rtsp_connection_flush().
 *
 * Returns: #GST_RTSP_OK on success.
 */
GstRTSPResult
gst_rtsp_connection_read (GstRTSPConnection * conn, guint8 * data, guint size,
    GTimeVal * timeout)
{
  guint offset;
  gint retval;
  GstClockTime to;
  GstRTSPResult res;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (data != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->readfd != NULL, GST_RTSP_EINVAL);

  if (G_UNLIKELY (size == 0))
    return GST_RTSP_OK;

  offset = 0;

  /* configure timeout if any */
  to = timeout ? GST_TIMEVAL_TO_TIME (*timeout) : GST_CLOCK_TIME_NONE;

  gst_poll_set_controllable (conn->fdset, TRUE);
  gst_poll_fd_ctl_write (conn->fdset, conn->writefd, FALSE);
  gst_poll_fd_ctl_read (conn->fdset, conn->readfd, TRUE);

  while (TRUE) {
    res = read_bytes (conn, data, &offset, size);
    if (G_UNLIKELY (res == GST_RTSP_EEOF))
      goto eof;
    if (G_LIKELY (res == GST_RTSP_OK))
      break;
    if (G_UNLIKELY (res != GST_RTSP_EINTR))
      goto read_error;

    do {
      retval = gst_poll_wait (conn->fdset, to);
    } while (retval == -1 && (errno == EINTR || errno == EAGAIN));

    /* check for timeout */
    if (G_UNLIKELY (retval == 0))
      goto select_timeout;

    if (G_UNLIKELY (retval == -1)) {
      if (errno == EBUSY)
        goto stopped;
      else
        goto select_error;
    }

    /* could also be an error with write socket */
    if (gst_poll_fd_has_error (conn->fdset, conn->writefd))
      goto socket_error;

    gst_poll_set_controllable (conn->fdset, FALSE);
  }
  return GST_RTSP_OK;

  /* ERRORS */
select_error:
  {
    return GST_RTSP_ESYS;
  }
select_timeout:
  {
    return GST_RTSP_ETIMEOUT;
  }
stopped:
  {
    return GST_RTSP_EINTR;
  }
eof:
  {
    return GST_RTSP_EEOF;
  }
socket_error:
  {
    res = GST_RTSP_ENET;
  }
read_error:
  {
    return res;
  }
}

static GstRTSPMessage *
gen_tunnel_reply (GstRTSPConnection * conn, GstRTSPStatusCode code,
    const GstRTSPMessage * request)
{
  GstRTSPMessage *msg;
  GstRTSPResult res;

  if (gst_rtsp_status_as_text (code) == NULL)
    code = GST_RTSP_STS_INTERNAL_SERVER_ERROR;

  GST_RTSP_CHECK (gst_rtsp_message_new_response (&msg, code, NULL, request),
      no_message);

  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_SERVER,
      "GStreamer RTSP Server");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_CONNECTION, "close");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_CACHE_CONTROL, "no-store");
  gst_rtsp_message_add_header (msg, GST_RTSP_HDR_PRAGMA, "no-cache");

  if (code == GST_RTSP_STS_OK) {
    if (conn->ip)
      gst_rtsp_message_add_header (msg, GST_RTSP_HDR_X_SERVER_IP_ADDRESS,
          conn->ip);
    gst_rtsp_message_add_header (msg, GST_RTSP_HDR_CONTENT_TYPE,
        "application/x-rtsp-tunnelled");
  }

  return msg;

  /* ERRORS */
no_message:
  {
    return NULL;
  }
}

/**
 * gst_rtsp_connection_receive:
 * @conn: a #GstRTSPConnection
 * @message: the message to read
 * @timeout: a timeout value or #NULL
 *
 * Attempt to read into @message from the connected @conn, blocking up to
 * the specified @timeout. @timeout can be #NULL, in which case this function
 * might block forever.
 * 
 * This function can be cancelled with gst_rtsp_connection_flush().
 *
 * Returns: #GST_RTSP_OK on success.
 */
GstRTSPResult
gst_rtsp_connection_receive (GstRTSPConnection * conn, GstRTSPMessage * message,
    GTimeVal * timeout)
{
  GstRTSPResult res;
  GstRTSPBuilder builder;
  gint retval;
  GstClockTime to;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (message != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->readfd != NULL, GST_RTSP_EINVAL);

  /* configure timeout if any */
  to = timeout ? GST_TIMEVAL_TO_TIME (*timeout) : GST_CLOCK_TIME_NONE;

  gst_poll_set_controllable (conn->fdset, TRUE);
  gst_poll_fd_ctl_write (conn->fdset, conn->writefd, FALSE);
  gst_poll_fd_ctl_read (conn->fdset, conn->readfd, TRUE);

  memset (&builder, 0, sizeof (GstRTSPBuilder));
  while (TRUE) {
    res = build_next (&builder, message, conn);
    if (G_UNLIKELY (res == GST_RTSP_EEOF))
      goto eof;
    else if (G_LIKELY (res == GST_RTSP_OK)) {
      if (!conn->manual_http) {
        if (message->type == GST_RTSP_MESSAGE_HTTP_REQUEST) {
          if (conn->tstate == TUNNEL_STATE_NONE &&
              message->type_data.request.method == GST_RTSP_GET) {
            GstRTSPMessage *response;

            conn->tstate = TUNNEL_STATE_GET;

            /* tunnel GET request, we can reply now */
            response = gen_tunnel_reply (conn, GST_RTSP_STS_OK, message);
            res = gst_rtsp_connection_send (conn, response, timeout);
            gst_rtsp_message_free (response);
            if (res == GST_RTSP_OK)
              res = GST_RTSP_ETGET;
            goto cleanup;
          } else if (conn->tstate == TUNNEL_STATE_NONE &&
              message->type_data.request.method == GST_RTSP_POST) {
            conn->tstate = TUNNEL_STATE_POST;

            /* tunnel POST request, the caller now has to link the two
             * connections. */
            res = GST_RTSP_ETPOST;
            goto cleanup;
          } else {
            res = GST_RTSP_EPARSE;
            goto cleanup;
          }
        } else if (message->type == GST_RTSP_MESSAGE_HTTP_RESPONSE) {
          res = GST_RTSP_EPARSE;
          goto cleanup;
        }
      }

      break;
    } else if (G_UNLIKELY (res != GST_RTSP_EINTR))
      goto read_error;

    do {
      retval = gst_poll_wait (conn->fdset, to);
    } while (retval == -1 && (errno == EINTR || errno == EAGAIN));

    /* check for timeout */
    if (G_UNLIKELY (retval == 0))
      goto select_timeout;

    if (G_UNLIKELY (retval == -1)) {
      if (errno == EBUSY)
        goto stopped;
      else
        goto select_error;
    }

    /* could also be an error with write socket */
    if (gst_poll_fd_has_error (conn->fdset, conn->writefd))
      goto socket_error;

    /* once we start reading the wait cannot be controlled */
    if (builder.state != STATE_START)
      gst_poll_set_controllable (conn->fdset, FALSE);
  }

  /* we have a message here */
  build_reset (&builder);

  return GST_RTSP_OK;

  /* ERRORS */
select_error:
  {
    res = GST_RTSP_ESYS;
    goto cleanup;
  }
select_timeout:
  {
    res = GST_RTSP_ETIMEOUT;
    goto cleanup;
  }
stopped:
  {
    res = GST_RTSP_EINTR;
    goto cleanup;
  }
eof:
  {
    res = GST_RTSP_EEOF;
    goto cleanup;
  }
socket_error:
  {
    res = GST_RTSP_ENET;
    goto cleanup;
  }
read_error:
cleanup:
  {
    build_reset (&builder);
    gst_rtsp_message_unset (message);
    return res;
  }
}

/**
 * gst_rtsp_connection_close:
 * @conn: a #GstRTSPConnection
 *
 * Close the connected @conn. After this call, the connection is in the same
 * state as when it was first created.
 * 
 * Returns: #GST_RTSP_OK on success.
 */
GstRTSPResult
gst_rtsp_connection_close (GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  g_free (conn->ip);
  conn->ip = NULL;

  conn->read_ahead = 0;

  g_free (conn->initial_buffer);
  conn->initial_buffer = NULL;
  conn->initial_buffer_offset = 0;

  REMOVE_POLLFD (conn->fdset, &conn->fd0);
  REMOVE_POLLFD (conn->fdset, &conn->fd1);
  conn->writefd = NULL;
  conn->readfd = NULL;
  conn->tunneled = FALSE;
  conn->tstate = TUNNEL_STATE_NONE;
  conn->ctxp = NULL;
  g_free (conn->username);
  conn->username = NULL;
  g_free (conn->passwd);
  conn->passwd = NULL;
  gst_rtsp_connection_clear_auth_params (conn);
  conn->timeout = 60;
  conn->cseq = 0;
  conn->session_id[0] = '\0';

  return GST_RTSP_OK;
}

/**
 * gst_rtsp_connection_free:
 * @conn: a #GstRTSPConnection
 *
 * Close and free @conn.
 * 
 * Returns: #GST_RTSP_OK on success.
 */
GstRTSPResult
gst_rtsp_connection_free (GstRTSPConnection * conn)
{
  GstRTSPResult res;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  res = gst_rtsp_connection_close (conn);
  gst_poll_free (conn->fdset);
  g_timer_destroy (conn->timer);
  gst_rtsp_url_free (conn->url);
  g_free (conn->proxy_host);
  g_free (conn);
#ifdef G_OS_WIN32
  WSACleanup ();
#endif

  return res;
}

/**
 * gst_rtsp_connection_poll:
 * @conn: a #GstRTSPConnection
 * @events: a bitmask of #GstRTSPEvent flags to check
 * @revents: location for result flags 
 * @timeout: a timeout
 *
 * Wait up to the specified @timeout for the connection to become available for
 * at least one of the operations specified in @events. When the function returns
 * with #GST_RTSP_OK, @revents will contain a bitmask of available operations on
 * @conn.
 *
 * @timeout can be #NULL, in which case this function might block forever.
 *
 * This function can be cancelled with gst_rtsp_connection_flush().
 * 
 * Returns: #GST_RTSP_OK on success.
 *
 * Since: 0.10.15
 */
GstRTSPResult
gst_rtsp_connection_poll (GstRTSPConnection * conn, GstRTSPEvent events,
    GstRTSPEvent * revents, GTimeVal * timeout)
{
  GstClockTime to;
  gint retval;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (events != 0, GST_RTSP_EINVAL);
  g_return_val_if_fail (revents != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->readfd != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->writefd != NULL, GST_RTSP_EINVAL);

  gst_poll_set_controllable (conn->fdset, TRUE);

  /* add fd to writer set when asked to */
  gst_poll_fd_ctl_write (conn->fdset, conn->writefd,
      events & GST_RTSP_EV_WRITE);

  /* add fd to reader set when asked to */
  gst_poll_fd_ctl_read (conn->fdset, conn->readfd, events & GST_RTSP_EV_READ);

  /* configure timeout if any */
  to = timeout ? GST_TIMEVAL_TO_TIME (*timeout) : GST_CLOCK_TIME_NONE;

  do {
    retval = gst_poll_wait (conn->fdset, to);
  } while (retval == -1 && (errno == EINTR || errno == EAGAIN));

  if (G_UNLIKELY (retval == 0))
    goto select_timeout;

  if (G_UNLIKELY (retval == -1)) {
    if (errno == EBUSY)
      goto stopped;
    else
      goto select_error;
  }

  *revents = 0;
  if (events & GST_RTSP_EV_READ) {
    if (gst_poll_fd_can_read (conn->fdset, conn->readfd))
      *revents |= GST_RTSP_EV_READ;
  }
  if (events & GST_RTSP_EV_WRITE) {
    if (gst_poll_fd_can_write (conn->fdset, conn->writefd))
      *revents |= GST_RTSP_EV_WRITE;
  }
  return GST_RTSP_OK;

  /* ERRORS */
select_timeout:
  {
    return GST_RTSP_ETIMEOUT;
  }
select_error:
  {
    return GST_RTSP_ESYS;
  }
stopped:
  {
    return GST_RTSP_EINTR;
  }
}

/**
 * gst_rtsp_connection_next_timeout:
 * @conn: a #GstRTSPConnection
 * @timeout: a timeout
 *
 * Calculate the next timeout for @conn, storing the result in @timeout.
 *
 * Returns: #GST_RTSP_OK.
 */
GstRTSPResult
gst_rtsp_connection_next_timeout (GstRTSPConnection * conn, GTimeVal * timeout)
{
  gdouble elapsed;
  glong sec;
  gulong usec;
  gint ctimeout;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (timeout != NULL, GST_RTSP_EINVAL);

  ctimeout = conn->timeout;
  if (ctimeout >= 20) {
    /* Because we should act before the timeout we timeout 5
     * seconds in advance. */
    ctimeout -= 5;
  } else if (ctimeout >= 5) {
    /* else timeout 20% earlier */
    ctimeout -= ctimeout / 5;
  } else if (ctimeout >= 1) {
    /* else timeout 1 second earlier */
    ctimeout -= 1;
  }

  elapsed = g_timer_elapsed (conn->timer, &usec);
  if (elapsed >= ctimeout) {
    sec = 0;
    usec = 0;
  } else {
    sec = ctimeout - elapsed;
    if (usec <= G_USEC_PER_SEC)
      usec = G_USEC_PER_SEC - usec;
    else
      usec = 0;
  }

  timeout->tv_sec = sec;
  timeout->tv_usec = usec;

  return GST_RTSP_OK;
}

/**
 * gst_rtsp_connection_reset_timeout:
 * @conn: a #GstRTSPConnection
 *
 * Reset the timeout of @conn.
 *
 * Returns: #GST_RTSP_OK.
 */
GstRTSPResult
gst_rtsp_connection_reset_timeout (GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  g_timer_start (conn->timer);

  return GST_RTSP_OK;
}

/**
 * gst_rtsp_connection_flush:
 * @conn: a #GstRTSPConnection
 * @flush: start or stop the flush
 *
 * Start or stop the flushing action on @conn. When flushing, all current
 * and future actions on @conn will return #GST_RTSP_EINTR until the connection
 * is set to non-flushing mode again.
 * 
 * Returns: #GST_RTSP_OK.
 */
GstRTSPResult
gst_rtsp_connection_flush (GstRTSPConnection * conn, gboolean flush)
{
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  gst_poll_set_flushing (conn->fdset, flush);

  return GST_RTSP_OK;
}

/**
 * gst_rtsp_connection_set_proxy:
 * @conn: a #GstRTSPConnection
 * @host: the proxy host
 * @port: the proxy port
 *
 * Set the proxy host and port.
 * 
 * Returns: #GST_RTSP_OK.
 *
 * Since: 0.10.23
 */
GstRTSPResult
gst_rtsp_connection_set_proxy (GstRTSPConnection * conn,
    const gchar * host, guint port)
{
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  g_free (conn->proxy_host);
  conn->proxy_host = g_strdup (host);
  conn->proxy_port = port;

  return GST_RTSP_OK;
}

/**
 * gst_rtsp_connection_set_auth:
 * @conn: a #GstRTSPConnection
 * @method: authentication method
 * @user: the user
 * @pass: the password
 *
 * Configure @conn for authentication mode @method with @user and @pass as the
 * user and password respectively.
 * 
 * Returns: #GST_RTSP_OK.
 */
GstRTSPResult
gst_rtsp_connection_set_auth (GstRTSPConnection * conn,
    GstRTSPAuthMethod method, const gchar * user, const gchar * pass)
{
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  if (method == GST_RTSP_AUTH_DIGEST && ((user == NULL || pass == NULL)
          || g_strrstr (user, ":") != NULL))
    return GST_RTSP_EINVAL;

  /* Make sure the username and passwd are being set for authentication */
  if (method == GST_RTSP_AUTH_NONE && (user == NULL || pass == NULL))
    return GST_RTSP_EINVAL;

  /* ":" chars are not allowed in usernames for basic auth */
  if (method == GST_RTSP_AUTH_BASIC && g_strrstr (user, ":") != NULL)
    return GST_RTSP_EINVAL;

  g_free (conn->username);
  g_free (conn->passwd);

  conn->auth_method = method;
  conn->username = g_strdup (user);
  conn->passwd = g_strdup (pass);

  return GST_RTSP_OK;
}

/**
 * str_case_hash:
 * @key: ASCII string to hash
 *
 * Hashes @key in a case-insensitive manner.
 *
 * Returns: the hash code.
 **/
static guint
str_case_hash (gconstpointer key)
{
  const char *p = key;
  guint h = g_ascii_toupper (*p);

  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + g_ascii_toupper (*p);

  return h;
}

/**
 * str_case_equal:
 * @v1: an ASCII string
 * @v2: another ASCII string
 *
 * Compares @v1 and @v2 in a case-insensitive manner
 *
 * Returns: %TRUE if they are equal (modulo case)
 **/
static gboolean
str_case_equal (gconstpointer v1, gconstpointer v2)
{
  const char *string1 = v1;
  const char *string2 = v2;

  return g_ascii_strcasecmp (string1, string2) == 0;
}

/**
 * gst_rtsp_connection_set_auth_param:
 * @conn: a #GstRTSPConnection
 * @param: authentication directive
 * @value: value
 *
 * Setup @conn with authentication directives. This is not necesary for
 * methods #GST_RTSP_AUTH_NONE and #GST_RTSP_AUTH_BASIC. For
 * #GST_RTSP_AUTH_DIGEST, directives should be taken from the digest challenge
 * in the WWW-Authenticate response header and can include realm, domain,
 * nonce, opaque, stale, algorithm, qop as per RFC2617.
 * 
 * Since: 0.10.20
 */
void
gst_rtsp_connection_set_auth_param (GstRTSPConnection * conn,
    const gchar * param, const gchar * value)
{
  g_return_if_fail (conn != NULL);
  g_return_if_fail (param != NULL);

  if (conn->auth_params == NULL) {
    conn->auth_params =
        g_hash_table_new_full (str_case_hash, str_case_equal, g_free, g_free);
  }
  g_hash_table_insert (conn->auth_params, g_strdup (param), g_strdup (value));
}

/**
 * gst_rtsp_connection_clear_auth_params:
 * @conn: a #GstRTSPConnection
 *
 * Clear the list of authentication directives stored in @conn.
 *
 * Since: 0.10.20
 */
void
gst_rtsp_connection_clear_auth_params (GstRTSPConnection * conn)
{
  g_return_if_fail (conn != NULL);

  if (conn->auth_params != NULL) {
    g_hash_table_destroy (conn->auth_params);
    conn->auth_params = NULL;
  }
}

static GstRTSPResult
set_qos_dscp (gint fd, guint qos_dscp)
{
  union gst_sockaddr sa;
  socklen_t slen = sizeof (sa);
  gint af;
  gint tos;

  if (fd == -1)
    return GST_RTSP_OK;

  if (getsockname (fd, &sa.sa, &slen) < 0)
    goto no_getsockname;

  af = sa.sa.sa_family;

  /* if this is an IPv4-mapped address then do IPv4 QoS */
  if (af == AF_INET6) {
    if (IN6_IS_ADDR_V4MAPPED (&sa.sa_in6.sin6_addr))
      af = AF_INET;
  }

  /* extract and shift 6 bits of the DSCP */
  tos = (qos_dscp & 0x3f) << 2;

  switch (af) {
    case AF_INET:
      if (SETSOCKOPT (fd, IPPROTO_IP, IP_TOS, &tos, sizeof (tos)) < 0)
        goto no_setsockopt;
      break;
    case AF_INET6:
#ifdef IPV6_TCLASS
      if (SETSOCKOPT (fd, IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof (tos)) < 0)
        goto no_setsockopt;
      break;
#endif
    default:
      goto wrong_family;
  }

  return GST_RTSP_OK;

  /* ERRORS */
no_getsockname:
no_setsockopt:
  {
    return GST_RTSP_ESYS;
  }

wrong_family:
  {
    return GST_RTSP_ERROR;
  }
}

/**
 * gst_rtsp_connection_set_qos_dscp:
 * @conn: a #GstRTSPConnection
 * @qos_dscp: DSCP value
 *
 * Configure @conn to use the specified DSCP value.
 *
 * Returns: #GST_RTSP_OK on success.
 *
 * Since: 0.10.20
 */
GstRTSPResult
gst_rtsp_connection_set_qos_dscp (GstRTSPConnection * conn, guint qos_dscp)
{
  GstRTSPResult res;

  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->readfd != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (conn->writefd != NULL, GST_RTSP_EINVAL);

  res = set_qos_dscp (conn->fd0.fd, qos_dscp);
  if (res == GST_RTSP_OK)
    res = set_qos_dscp (conn->fd1.fd, qos_dscp);

  return res;
}


/**
 * gst_rtsp_connection_get_url:
 * @conn: a #GstRTSPConnection
 *
 * Retrieve the URL of the other end of @conn.
 *
 * Returns: The URL. This value remains valid until the
 * connection is freed.
 *
 * Since: 0.10.23
 */
GstRTSPUrl *
gst_rtsp_connection_get_url (const GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, NULL);

  return conn->url;
}

/**
 * gst_rtsp_connection_get_ip:
 * @conn: a #GstRTSPConnection
 *
 * Retrieve the IP address of the other end of @conn.
 *
 * Returns: The IP address as a string. this value remains valid until the
 * connection is closed.
 *
 * Since: 0.10.20
 */
const gchar *
gst_rtsp_connection_get_ip (const GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, NULL);

  return conn->ip;
}

/**
 * gst_rtsp_connection_set_ip:
 * @conn: a #GstRTSPConnection
 * @ip: an ip address
 *
 * Set the IP address of the server.
 *
 * Since: 0.10.23
 */
void
gst_rtsp_connection_set_ip (GstRTSPConnection * conn, const gchar * ip)
{
  g_return_if_fail (conn != NULL);

  g_free (conn->ip);
  conn->ip = g_strdup (ip);
}

/**
 * gst_rtsp_connection_get_readfd:
 * @conn: a #GstRTSPConnection
 *
 * Get the file descriptor for reading.
 *
 * Returns: the file descriptor used for reading or -1 on error. The file
 * descriptor remains valid until the connection is closed.
 *
 * Since: 0.10.23
 */
gint
gst_rtsp_connection_get_readfd (const GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, -1);
  g_return_val_if_fail (conn->readfd != NULL, -1);

  return conn->readfd->fd;
}

/**
 * gst_rtsp_connection_get_writefd:
 * @conn: a #GstRTSPConnection
 *
 * Get the file descriptor for writing.
 *
 * Returns: the file descriptor used for writing or -1 on error. The file
 * descriptor remains valid until the connection is closed.
 *
 * Since: 0.10.23
 */
gint
gst_rtsp_connection_get_writefd (const GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, -1);
  g_return_val_if_fail (conn->writefd != NULL, -1);

  return conn->writefd->fd;
}

/**
 * gst_rtsp_connection_set_http_mode:
 * @conn: a #GstRTSPConnection
 * @enable: %TRUE to enable manual HTTP mode
 *
 * By setting the HTTP mode to %TRUE the message parsing will support HTTP
 * messages in addition to the RTSP messages. It will also disable the
 * automatic handling of setting up an HTTP tunnel.
 *
 * Since: 0.10.25
 */
void
gst_rtsp_connection_set_http_mode (GstRTSPConnection * conn, gboolean enable)
{
  g_return_if_fail (conn != NULL);

  conn->manual_http = enable;
}

/**
 * gst_rtsp_connection_set_tunneled:
 * @conn: a #GstRTSPConnection
 * @tunneled: the new state
 *
 * Set the HTTP tunneling state of the connection. This must be configured before
 * the @conn is connected.
 *
 * Since: 0.10.23
 */
void
gst_rtsp_connection_set_tunneled (GstRTSPConnection * conn, gboolean tunneled)
{
  g_return_if_fail (conn != NULL);
  g_return_if_fail (conn->readfd == NULL);
  g_return_if_fail (conn->writefd == NULL);

  conn->tunneled = tunneled;
}

/**
 * gst_rtsp_connection_is_tunneled:
 * @conn: a #GstRTSPConnection
 *
 * Get the tunneling state of the connection. 
 *
 * Returns: if @conn is using HTTP tunneling.
 *
 * Since: 0.10.23
 */
gboolean
gst_rtsp_connection_is_tunneled (const GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, FALSE);

  return conn->tunneled;
}

/**
 * gst_rtsp_connection_get_tunnelid:
 * @conn: a #GstRTSPConnection
 *
 * Get the tunnel session id the connection. 
 *
 * Returns: returns a non-empty string if @conn is being tunneled over HTTP.
 *
 * Since: 0.10.23
 */
const gchar *
gst_rtsp_connection_get_tunnelid (const GstRTSPConnection * conn)
{
  g_return_val_if_fail (conn != NULL, NULL);

  if (!conn->tunneled)
    return NULL;

  return conn->tunnelid;
}

/**
 * gst_rtsp_connection_do_tunnel:
 * @conn: a #GstRTSPConnection
 * @conn2: a #GstRTSPConnection or %NULL
 *
 * If @conn received the first tunnel connection and @conn2 received
 * the second tunnel connection, link the two connections together so that
 * @conn manages the tunneled connection.
 *
 * After this call, @conn2 cannot be used anymore and must be freed with
 * gst_rtsp_connection_free().
 *
 * If @conn2 is %NULL then only the base64 decoding context will be setup for
 * @conn.
 *
 * Returns: return GST_RTSP_OK on success.
 *
 * Since: 0.10.23
 */
GstRTSPResult
gst_rtsp_connection_do_tunnel (GstRTSPConnection * conn,
    GstRTSPConnection * conn2)
{
  g_return_val_if_fail (conn != NULL, GST_RTSP_EINVAL);

  if (conn2 != NULL) {
    g_return_val_if_fail (conn->tstate == TUNNEL_STATE_GET, GST_RTSP_EINVAL);
    g_return_val_if_fail (conn2->tstate == TUNNEL_STATE_POST, GST_RTSP_EINVAL);
    g_return_val_if_fail (!memcmp (conn2->tunnelid, conn->tunnelid,
            TUNNELID_LEN), GST_RTSP_EINVAL);

    /* both connections have fd0 as the read/write socket. start by taking the
     * socket from conn2 and set it as the socket in conn */
    conn->fd1 = conn2->fd0;

    /* clean up some of the state of conn2 */
    gst_poll_remove_fd (conn2->fdset, &conn2->fd0);
    conn2->fd0.fd = -1;
    conn2->readfd = conn2->writefd = NULL;

    /* We make fd0 the write socket and fd1 the read socket. */
    conn->writefd = &conn->fd0;
    conn->readfd = &conn->fd1;

    conn->tstate = TUNNEL_STATE_COMPLETE;
  }

  /* we need base64 decoding for the readfd */
  conn->ctx.state = 0;
  conn->ctx.save = 0;
  conn->ctx.cout = 0;
  conn->ctx.coutl = 0;
  conn->ctxp = &conn->ctx;

  return GST_RTSP_OK;
}

#define READ_ERR    (G_IO_HUP | G_IO_ERR | G_IO_NVAL)
#define READ_COND   (G_IO_IN | READ_ERR)
#define WRITE_ERR   (G_IO_HUP | G_IO_ERR | G_IO_NVAL)
#define WRITE_COND  (G_IO_OUT | WRITE_ERR)

typedef struct
{
  guint8 *data;
  guint size;
  guint id;
} GstRTSPRec;

/* async functions */
struct _GstRTSPWatch
{
  GSource source;

  GstRTSPConnection *conn;

  GstRTSPBuilder builder;
  GstRTSPMessage message;

  GPollFD readfd;
  GPollFD writefd;

  /* queued message for transmission */
  guint id;
  GMutex *mutex;
  GQueue *messages;
  guint8 *write_data;
  guint write_off;
  guint write_size;
  guint write_id;

  GstRTSPWatchFuncs funcs;

  gpointer user_data;
  GDestroyNotify notify;
};

static gboolean
gst_rtsp_source_prepare (GSource * source, gint * timeout)
{
  GstRTSPWatch *watch = (GstRTSPWatch *) source;

  if (watch->conn->initial_buffer != NULL)
    return TRUE;

  *timeout = (watch->conn->timeout * 1000);

  return FALSE;
}

static gboolean
gst_rtsp_source_check (GSource * source)
{
  GstRTSPWatch *watch = (GstRTSPWatch *) source;

  if (watch->readfd.revents & READ_COND)
    return TRUE;

  if (watch->writefd.revents & WRITE_COND)
    return TRUE;

  return FALSE;
}

static gboolean
gst_rtsp_source_dispatch (GSource * source, GSourceFunc callback G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED)
{
  GstRTSPWatch *watch = (GstRTSPWatch *) source;
  GstRTSPResult res = GST_RTSP_ERROR;
  gboolean keep_running = TRUE;

  /* first read as much as we can */
  if (watch->readfd.revents & READ_COND || watch->conn->initial_buffer != NULL) {
    do {
      if (watch->readfd.revents & READ_ERR)
        goto read_error;

      res = build_next (&watch->builder, &watch->message, watch->conn);
      if (res == GST_RTSP_EINTR)
        break;
      else if (G_UNLIKELY (res == GST_RTSP_EEOF)) {
        watch->readfd.events = 0;
        watch->readfd.revents = 0;
        g_source_remove_poll ((GSource *) watch, &watch->readfd);
        /* When we are in tunnelled mode, the read socket can be closed and we
         * should be prepared for a new POST method to reopen it */
        if (watch->conn->tstate == TUNNEL_STATE_COMPLETE) {
          /* remove the read connection for the tunnel */
          /* we accept a new POST request */
          watch->conn->tstate = TUNNEL_STATE_GET;
          /* and signal that we lost our tunnel */
          if (watch->funcs.tunnel_lost)
            res = watch->funcs.tunnel_lost (watch, watch->user_data);
          goto read_done;
        } else
          goto eof;
      } else if (G_LIKELY (res == GST_RTSP_OK)) {
        if (!watch->conn->manual_http &&
            watch->message.type == GST_RTSP_MESSAGE_HTTP_REQUEST) {
          if (watch->conn->tstate == TUNNEL_STATE_NONE &&
              watch->message.type_data.request.method == GST_RTSP_GET) {
            GstRTSPMessage *response;
            GstRTSPStatusCode code;

            watch->conn->tstate = TUNNEL_STATE_GET;

            if (watch->funcs.tunnel_start)
              code = watch->funcs.tunnel_start (watch, watch->user_data);
            else
              code = GST_RTSP_STS_OK;

            /* queue the response */
            response = gen_tunnel_reply (watch->conn, code, &watch->message);
            gst_rtsp_watch_send_message (watch, response, NULL);
            gst_rtsp_message_free (response);
            goto read_done;
          } else if (watch->conn->tstate == TUNNEL_STATE_NONE &&
              watch->message.type_data.request.method == GST_RTSP_POST) {
            watch->conn->tstate = TUNNEL_STATE_POST;

            /* in the callback the connection should be tunneled with the
             * GET connection */
            if (watch->funcs.tunnel_complete)
              watch->funcs.tunnel_complete (watch, watch->user_data);
            goto read_done;
          }
        }
      }

      if (!watch->conn->manual_http) {
        /* if manual HTTP support is not enabled, then restore the message to
         * what it would have looked like without the support for parsing HTTP
         * messages being present */
        if (watch->message.type == GST_RTSP_MESSAGE_HTTP_REQUEST) {
          watch->message.type = GST_RTSP_MESSAGE_REQUEST;
          watch->message.type_data.request.method = GST_RTSP_INVALID;
          if (watch->message.type_data.request.version != GST_RTSP_VERSION_1_0)
            watch->message.type_data.request.version = GST_RTSP_VERSION_INVALID;
          res = GST_RTSP_EPARSE;
        } else if (watch->message.type == GST_RTSP_MESSAGE_HTTP_RESPONSE) {
          watch->message.type = GST_RTSP_MESSAGE_RESPONSE;
          if (watch->message.type_data.response.version != GST_RTSP_VERSION_1_0)
            watch->message.type_data.response.version =
                GST_RTSP_VERSION_INVALID;
          res = GST_RTSP_EPARSE;
        }
      }

      if (G_LIKELY (res == GST_RTSP_OK)) {
        if (watch->funcs.message_received)
          watch->funcs.message_received (watch, &watch->message,
              watch->user_data);
      } else {
        goto read_error;
      }

    read_done:
      gst_rtsp_message_unset (&watch->message);
      build_reset (&watch->builder);
    } while (FALSE);
  }

  if (watch->writefd.revents & WRITE_COND) {
    if (watch->writefd.revents & WRITE_ERR)
      goto write_error;

    g_mutex_lock (watch->mutex);
    do {
      if (watch->write_data == NULL) {
        GstRTSPRec *rec;

        /* get a new message from the queue */
        rec = g_queue_pop_tail (watch->messages);
        if (rec == NULL)
          break;

        watch->write_off = 0;
        watch->write_data = rec->data;
        watch->write_size = rec->size;
        watch->write_id = rec->id;

        g_slice_free (GstRTSPRec, rec);
      }

      res = write_bytes (watch->writefd.fd, watch->write_data,
          &watch->write_off, watch->write_size);
      g_mutex_unlock (watch->mutex);

      if (res == GST_RTSP_EINTR)
        goto write_blocked;
      else if (G_LIKELY (res == GST_RTSP_OK)) {
        if (watch->funcs.message_sent)
          watch->funcs.message_sent (watch, watch->write_id, watch->user_data);
      } else {
        goto write_error;
      }
      g_mutex_lock (watch->mutex);

      g_free (watch->write_data);
      watch->write_data = NULL;
    } while (TRUE);

    watch->writefd.events = WRITE_ERR;

    g_mutex_unlock (watch->mutex);
  }

write_blocked:
  return keep_running;

  /* ERRORS */
eof:
  {
    if (watch->funcs.closed)
      watch->funcs.closed (watch, watch->user_data);

    /* always stop when the readfd returns EOF in non-tunneled mode */
    return FALSE;
  }
read_error:
  {
    watch->readfd.events = 0;
    watch->readfd.revents = 0;
    g_source_remove_poll ((GSource *) watch, &watch->readfd);
    keep_running = (watch->writefd.events != 0);

    if (keep_running) {
      if (watch->funcs.error_full)
        GST_RTSP_CHECK (watch->funcs.error_full (watch, res, &watch->message,
                0, watch->user_data), error);
      else
        goto error;
    } else
      goto eof;
  }
write_error:
  {
    watch->writefd.events = 0;
    watch->writefd.revents = 0;
    g_source_remove_poll ((GSource *) watch, &watch->writefd);
    keep_running = (watch->readfd.events != 0);

    if (keep_running) {
      if (watch->funcs.error_full)
        GST_RTSP_CHECK (watch->funcs.error_full (watch, res, NULL,
                watch->write_id, watch->user_data), error);
      else
        goto error;
    } else
      goto eof;
  }
error:
  {
    if (watch->funcs.error)
      watch->funcs.error (watch, res, watch->user_data);

    return keep_running;
  }
}

static void
gst_rtsp_rec_free (gpointer data)
{
  GstRTSPRec *rec = data;

  g_free (rec->data);
  g_slice_free (GstRTSPRec, rec);
}

static void
gst_rtsp_source_finalize (GSource * source)
{
  GstRTSPWatch *watch = (GstRTSPWatch *) source;

  build_reset (&watch->builder);
  gst_rtsp_message_unset (&watch->message);

  g_queue_foreach (watch->messages, (GFunc) gst_rtsp_rec_free, NULL);
  g_queue_free (watch->messages);
  watch->messages = NULL;
  g_free (watch->write_data);

  g_mutex_free (watch->mutex);

  if (watch->notify)
    watch->notify (watch->user_data);
}

static GSourceFuncs gst_rtsp_source_funcs = {
  gst_rtsp_source_prepare,
  gst_rtsp_source_check,
  gst_rtsp_source_dispatch,
  gst_rtsp_source_finalize,
  NULL,
  NULL
};

/**
 * gst_rtsp_watch_new:
 * @conn: a #GstRTSPConnection
 * @funcs: watch functions
 * @user_data: user data to pass to @funcs
 * @notify: notify when @user_data is not referenced anymore
 *
 * Create a watch object for @conn. The functions provided in @funcs will be
 * called with @user_data when activity happened on the watch.
 *
 * The new watch is usually created so that it can be attached to a
 * maincontext with gst_rtsp_watch_attach(). 
 *
 * @conn must exist for the entire lifetime of the watch.
 *
 * Returns: a #GstRTSPWatch that can be used for asynchronous RTSP
 * communication. Free with gst_rtsp_watch_unref () after usage.
 *
 * Since: 0.10.23
 */
GstRTSPWatch *
gst_rtsp_watch_new (GstRTSPConnection * conn,
    GstRTSPWatchFuncs * funcs, gpointer user_data, GDestroyNotify notify)
{
  GstRTSPWatch *result;

  g_return_val_if_fail (conn != NULL, NULL);
  g_return_val_if_fail (funcs != NULL, NULL);
  g_return_val_if_fail (conn->readfd != NULL, NULL);
  g_return_val_if_fail (conn->writefd != NULL, NULL);

  result = (GstRTSPWatch *) g_source_new (&gst_rtsp_source_funcs,
      sizeof (GstRTSPWatch));

  result->conn = conn;
  result->builder.state = STATE_START;

  result->mutex = g_mutex_new ();
  result->messages = g_queue_new ();

  result->readfd.fd = -1;
  result->writefd.fd = -1;

  gst_rtsp_watch_reset (result);

  result->funcs = *funcs;
  result->user_data = user_data;
  result->notify = notify;

  return result;
}

/**
 * gst_rtsp_watch_reset:
 * @watch: a #GstRTSPWatch
 *
 * Reset @watch, this is usually called after gst_rtsp_connection_do_tunnel()
 * when the file descriptors of the connection might have changed.
 *
 * Since: 0.10.23
 */
void
gst_rtsp_watch_reset (GstRTSPWatch * watch)
{
  if (watch->readfd.fd != -1)
    g_source_remove_poll ((GSource *) watch, &watch->readfd);
  if (watch->writefd.fd != -1)
    g_source_remove_poll ((GSource *) watch, &watch->writefd);

  watch->readfd.fd = watch->conn->readfd->fd;
  watch->readfd.events = READ_COND;
  watch->readfd.revents = 0;

  watch->writefd.fd = watch->conn->writefd->fd;
  watch->writefd.events = WRITE_ERR;
  watch->writefd.revents = 0;

  if (watch->readfd.fd != -1)
    g_source_add_poll ((GSource *) watch, &watch->readfd);
  if (watch->writefd.fd != -1)
    g_source_add_poll ((GSource *) watch, &watch->writefd);
}

/**
 * gst_rtsp_watch_attach:
 * @watch: a #GstRTSPWatch
 * @context: a GMainContext (if NULL, the default context will be used)
 *
 * Adds a #GstRTSPWatch to a context so that it will be executed within that context.
 *
 * Returns: the ID (greater than 0) for the watch within the GMainContext. 
 *
 * Since: 0.10.23
 */
guint
gst_rtsp_watch_attach (GstRTSPWatch * watch, GMainContext * context)
{
  g_return_val_if_fail (watch != NULL, 0);

  return g_source_attach ((GSource *) watch, context);
}

/**
 * gst_rtsp_watch_unref:
 * @watch: a #GstRTSPWatch
 *
 * Decreases the reference count of @watch by one. If the resulting reference
 * count is zero the watch and associated memory will be destroyed.
 *
 * Since: 0.10.23
 */
void
gst_rtsp_watch_unref (GstRTSPWatch * watch)
{
  g_return_if_fail (watch != NULL);

  g_source_unref ((GSource *) watch);
}

/**
 * gst_rtsp_watch_write_data:
 * @watch: a #GstRTSPWatch
 * @data: the data to queue
 * @size: the size of @data
 * @id: location for a message ID or %NULL
 *
 * Write @data using the connection of the @watch. If it cannot be sent
 * immediately, it will be queued for transmission in @watch. The contents of
 * @message will then be serialized and transmitted when the connection of the
 * @watch becomes writable. In case the @message is queued, the ID returned in
 * @id will be non-zero and used as the ID argument in the message_sent
 * callback.
 *
 * This function will take ownership of @data and g_free() it after use.
 *
 * Returns: #GST_RTSP_OK on success.
 *
 * Since: 0.10.25
 */
GstRTSPResult
gst_rtsp_watch_write_data (GstRTSPWatch * watch, const guint8 * data,
    guint size, guint * id)
{
  GstRTSPResult res;
  GstRTSPRec *rec;
  guint off = 0;
  GMainContext *context = NULL;

  g_return_val_if_fail (watch != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (data != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (size != 0, GST_RTSP_EINVAL);

  g_mutex_lock (watch->mutex);

  /* try to send the message synchronously first */
  if (watch->messages->length == 0 && watch->write_data == NULL) {
    res = write_bytes (watch->writefd.fd, data, &off, size);
    if (res != GST_RTSP_EINTR) {
      if (id != NULL)
        *id = 0;
      g_free ((gpointer) data);
      goto done;
    }
  }

  /* make a record with the data and id for sending async */
  rec = g_slice_new (GstRTSPRec);
  if (off == 0) {
    rec->data = (guint8 *) data;
    rec->size = size;
  } else {
    rec->data = g_memdup (data + off, size - off);
    rec->size = size - off;
    g_free ((gpointer) data);
  }

  do {
    /* make sure rec->id is never 0 */
    rec->id = ++watch->id;
  } while (G_UNLIKELY (rec->id == 0));

  /* add the record to a queue. FIXME we would like to have an upper limit here */
  g_queue_push_head (watch->messages, rec);

  /* make sure the main context will now also check for writability on the
   * socket */
  if (watch->writefd.events != WRITE_COND) {
    watch->writefd.events = WRITE_COND;
    context = ((GSource *) watch)->context;
  }

  if (id != NULL)
    *id = rec->id;
  res = GST_RTSP_OK;

done:
  g_mutex_unlock (watch->mutex);

  if (context)
    g_main_context_wakeup (context);

  return res;
}

/**
 * gst_rtsp_watch_send_message:
 * @watch: a #GstRTSPWatch
 * @message: a #GstRTSPMessage
 * @id: location for a message ID or %NULL
 *
 * Send a @message using the connection of the @watch. If it cannot be sent
 * immediately, it will be queued for transmission in @watch. The contents of
 * @message will then be serialized and transmitted when the connection of the
 * @watch becomes writable. In case the @message is queued, the ID returned in
 * @id will be non-zero and used as the ID argument in the message_sent
 * callback.
 *
 * Returns: #GST_RTSP_OK on success.
 *
 * Since: 0.10.25
 */
GstRTSPResult
gst_rtsp_watch_send_message (GstRTSPWatch * watch, GstRTSPMessage * message,
    guint * id)
{
  GString *str;
  guint size;

  g_return_val_if_fail (watch != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (message != NULL, GST_RTSP_EINVAL);

  /* make a record with the message as a string and id */
  str = message_to_string (watch->conn, message);
  size = str->len;
  return gst_rtsp_watch_write_data (watch,
      (guint8 *) g_string_free (str, FALSE), size, id);
}

/**
 * gst_rtsp_watch_queue_data:
 * @watch: a #GstRTSPWatch
 * @data: the data to queue
 * @size: the size of @data
 *
 * Queue @data for transmission in @watch. It will be transmitted when the
 * connection of the @watch becomes writable.
 *
 * This function will take ownership of @data and g_free() it after use.
 *
 * The return value of this function will be used as the id argument in the
 * message_sent callback.
 *
 * Deprecated: Use gst_rtsp_watch_write_data()
 *
 * Returns: an id.
 *
 * Since: 0.10.24
 */
#ifndef GST_REMOVE_DEPRECATED
#ifdef GST_DISABLE_DEPRECATED
guint
gst_rtsp_watch_queue_data (GstRTSPWatch * watch, const guint8 * data,
    guint size);
#endif
guint
gst_rtsp_watch_queue_data (GstRTSPWatch * watch, const guint8 * data,
    guint size)
{
  GstRTSPRec *rec;
  GMainContext *context = NULL;

  g_return_val_if_fail (watch != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (data != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (size != 0, GST_RTSP_EINVAL);

  g_mutex_lock (watch->mutex);

  /* make a record with the data and id */
  rec = g_slice_new (GstRTSPRec);
  rec->data = (guint8 *) data;
  rec->size = size;
  do {
    /* make sure rec->id is never 0 */
    rec->id = ++watch->id;
  } while (G_UNLIKELY (rec->id == 0));

  /* add the record to a queue. FIXME we would like to have an upper limit here */
  g_queue_push_head (watch->messages, rec);

  /* make sure the main context will now also check for writability on the
   * socket */
  if (watch->writefd.events != WRITE_COND) {
    watch->writefd.events = WRITE_COND;
    context = ((GSource *) watch)->context;
  }
  g_mutex_unlock (watch->mutex);

  if (context)
    g_main_context_wakeup (context);

  return rec->id;
}
#endif /* GST_REMOVE_DEPRECATED */

/**
 * gst_rtsp_watch_queue_message:
 * @watch: a #GstRTSPWatch
 * @message: a #GstRTSPMessage
 *
 * Queue a @message for transmission in @watch. The contents of this
 * message will be serialized and transmitted when the connection of the
 * @watch becomes writable.
 *
 * The return value of this function will be used as the id argument in the
 * message_sent callback.
 *
 * Deprecated: Use gst_rtsp_watch_send_message()
 *
 * Returns: an id.
 *
 * Since: 0.10.23
 */
#ifndef GST_REMOVE_DEPRECATED
#ifdef GST_DISABLE_DEPRECATED
guint
gst_rtsp_watch_queue_message (GstRTSPWatch * watch, GstRTSPMessage * message);
#endif
guint
gst_rtsp_watch_queue_message (GstRTSPWatch * watch, GstRTSPMessage * message)
{
  GString *str;
  guint size;

  g_return_val_if_fail (watch != NULL, GST_RTSP_EINVAL);
  g_return_val_if_fail (message != NULL, GST_RTSP_EINVAL);

  /* make a record with the message as a string and id */
  str = message_to_string (watch->conn, message);
  size = str->len;
  return gst_rtsp_watch_queue_data (watch,
      (guint8 *) g_string_free (str, FALSE), size);
}
#endif /* GST_REMOVE_DEPRECATED */
