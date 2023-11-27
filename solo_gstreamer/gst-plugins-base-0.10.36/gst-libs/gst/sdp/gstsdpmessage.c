/* GStreamer
 * Copyright (C) <2005,2006> Wim Taymans <wim@fluendo.com>
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
 * SECTION:gstsdpmessage
 * @short_description: Helper methods for dealing with SDP messages
 *
 * <refsect2>
 * <para>
 * The GstSDPMessage helper functions makes it easy to parse and create SDP
 * messages.
 * </para>
 * </refsect2>
 *
 * Last reviewed on 2007-07-24 (0.10.14)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <glib.h>               /* for G_OS_WIN32 */
#include <gst/gstinfo.h>        /* For GST_STR_NULL */

#ifdef G_OS_WIN32
/* ws2_32.dll has getaddrinfo and freeaddrinfo on Windows XP and later.
 * minwg32 headers check WINVER before allowing the use of these */
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifdef _MSC_VER
#include <Winsock2.h>
#endif
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "gstsdpmessage.h"

/* FIXME, is currently allocated on the stack */
#define MAX_LINE_LEN    1024 * 16

#define FREE_STRING(field)              g_free (field); (field) = NULL
#define REPLACE_STRING(field, val)      FREE_STRING(field); (field) = g_strdup (val)

#define INIT_ARRAY(field, type, init_func)              \
G_STMT_START {                                          \
  if (field) {                                          \
    guint i;                                            \
    for(i = 0; i < (field)->len; i++)                   \
      init_func (&g_array_index ((field), type, i));    \
    g_array_set_size ((field), 0);                      \
  }                                                     \
  else                                                  \
    (field) = g_array_new (FALSE, TRUE, sizeof (type)); \
} G_STMT_END

#define FREE_ARRAY(field)         \
G_STMT_START {                    \
  if (field)                      \
    g_array_free ((field), TRUE); \
  (field) = NULL;                 \
} G_STMT_END

#define INIT_PTR_ARRAY(field, type, init_func)          \
G_STMT_START {                                          \
  if (field) {                                          \
    guint i;                                            \
    for(i = 0; i < (field)->len; i++)                   \
      init_func (g_array_index ((field), type, i));     \
    g_array_set_size ((field), 0);                      \
  }                                                     \
  else                                                  \
    (field) = g_array_new (FALSE, TRUE, sizeof (type)); \
} G_STMT_END

#define FREE_PTR_ARRAY(field) FREE_ARRAY(field)

#define DEFINE_STRING_SETTER(field)                                     \
GstSDPResult gst_sdp_message_set_##field (GstSDPMessage *msg, const gchar *val) { \
  g_free (msg->field);                                                  \
  msg->field = g_strdup (val);                                          \
  return GST_SDP_OK;                                                    \
}
#define DEFINE_STRING_GETTER(field)                                     \
const gchar* gst_sdp_message_get_##field (const GstSDPMessage *msg) {   \
  return msg->field;                                                    \
}

#define DEFINE_ARRAY_LEN(field)                                         \
guint gst_sdp_message_##field##_len (const GstSDPMessage *msg) {        \
  return msg->field->len;                                               \
}
#define DEFINE_ARRAY_GETTER(method, field, type)                        \
type * gst_sdp_message_get_##method (const GstSDPMessage *msg, guint idx) {  \
  return &g_array_index (msg->field, type, idx);                        \
}

#define DEFINE_PTR_ARRAY_LEN(field) DEFINE_ARRAY_LEN(field)
#define DEFINE_PTR_ARRAY_GETTER(method, field, type)                    \
type gst_sdp_message_get_##method (const GstSDPMessage *msg, guint idx) {    \
  return g_array_index (msg->field, type, idx);                         \
}
#define DEFINE_PTR_ARRAY_ADDER(method, field, type, dup_method)         \
GstSDPResult gst_sdp_message_add_##method (GstSDPMessage *msg, type val) {   \
  type v = dup_method (val);                                            \
  g_array_append_val (msg->field, v);                                   \
  return GST_SDP_OK;                                                    \
}

static void
gst_sdp_origin_init (GstSDPOrigin * origin)
{
  FREE_STRING (origin->username);
  FREE_STRING (origin->sess_id);
  FREE_STRING (origin->sess_version);
  FREE_STRING (origin->nettype);
  FREE_STRING (origin->addrtype);
  FREE_STRING (origin->addr);
}

static void
gst_sdp_connection_init (GstSDPConnection * connection)
{
  FREE_STRING (connection->nettype);
  FREE_STRING (connection->addrtype);
  FREE_STRING (connection->address);
  connection->ttl = 0;
  connection->addr_number = 0;
}

static void
gst_sdp_bandwidth_init (GstSDPBandwidth * bandwidth)
{
  FREE_STRING (bandwidth->bwtype);
  bandwidth->bandwidth = 0;
}

static void
gst_sdp_time_init (GstSDPTime * t)
{
  FREE_STRING (t->start);
  FREE_STRING (t->stop);
  INIT_PTR_ARRAY (t->repeat, gchar *, g_free);
  FREE_PTR_ARRAY (t->repeat);
}

static void
gst_sdp_zone_init (GstSDPZone * zone)
{
  FREE_STRING (zone->time);
  FREE_STRING (zone->typed_time);
}

static void
gst_sdp_key_init (GstSDPKey * key)
{
  FREE_STRING (key->type);
  FREE_STRING (key->data);
}

static void
gst_sdp_attribute_init (GstSDPAttribute * attr)
{
  FREE_STRING (attr->key);
  FREE_STRING (attr->value);
}

/**
 * gst_sdp_message_new:
 * @msg: pointer to new #GstSDPMessage
 *
 * Allocate a new GstSDPMessage and store the result in @msg.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_new (GstSDPMessage ** msg)
{
  GstSDPMessage *newmsg;

  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);

  newmsg = g_new0 (GstSDPMessage, 1);

  *msg = newmsg;

  return gst_sdp_message_init (newmsg);
}

/**
 * gst_sdp_message_init:
 * @msg: a #GstSDPMessage
 *
 * Initialize @msg so that its contents are as if it was freshly allocated
 * with gst_sdp_message_new(). This function is mostly used to initialize a message
 * allocated on the stack. gst_sdp_message_uninit() undoes this operation.
 *
 * When this function is invoked on newly allocated data (with malloc or on the
 * stack), its contents should be set to 0 before calling this function.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_init (GstSDPMessage * msg)
{
  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);

  FREE_STRING (msg->version);
  gst_sdp_origin_init (&msg->origin);
  FREE_STRING (msg->session_name);
  FREE_STRING (msg->information);
  FREE_STRING (msg->uri);
  INIT_PTR_ARRAY (msg->emails, gchar *, g_free);
  INIT_PTR_ARRAY (msg->phones, gchar *, g_free);
  gst_sdp_connection_init (&msg->connection);
  INIT_ARRAY (msg->bandwidths, GstSDPBandwidth, gst_sdp_bandwidth_init);
  INIT_ARRAY (msg->times, GstSDPTime, gst_sdp_time_init);
  INIT_ARRAY (msg->zones, GstSDPZone, gst_sdp_zone_init);
  gst_sdp_key_init (&msg->key);
  INIT_ARRAY (msg->attributes, GstSDPAttribute, gst_sdp_attribute_init);
  INIT_ARRAY (msg->medias, GstSDPMedia, gst_sdp_media_uninit);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_uninit:
 * @msg: a #GstSDPMessage
 *
 * Free all resources allocated in @msg. @msg should not be used anymore after
 * this function. This function should be used when @msg was allocated on the
 * stack and initialized with gst_sdp_message_init().
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_uninit (GstSDPMessage * msg)
{
  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);

  gst_sdp_message_init (msg);

  FREE_PTR_ARRAY (msg->emails);
  FREE_PTR_ARRAY (msg->phones);
  FREE_ARRAY (msg->bandwidths);
  FREE_ARRAY (msg->times);
  FREE_ARRAY (msg->zones);
  FREE_ARRAY (msg->attributes);
  FREE_ARRAY (msg->medias);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_free:
 * @msg: a #GstSDPMessage
 *
 * Free all resources allocated by @msg. @msg should not be used anymore after
 * this function. This function should be used when @msg was dynamically
 * allocated with gst_sdp_message_new().
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_free (GstSDPMessage * msg)
{
  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);

  gst_sdp_message_uninit (msg);
  g_free (msg);

  return GST_SDP_OK;
}

/**
 * gst_sdp_address_is_multicast:
 * @nettype: a network type
 * @addrtype: an address type
 * @addr: an address
 *
 * Check if the given @addr is a multicast address.
 *
 * Returns: TRUE when @addr is multicast.
 *
 * Since: 0.10.32
 */
gboolean
gst_sdp_address_is_multicast (const gchar * nettype, const gchar * addrtype,
    const gchar * addr)
{
  struct addrinfo hints;
  struct addrinfo *ai;
  struct addrinfo *res;
  gboolean ret = FALSE;

  g_return_val_if_fail (addr, FALSE);

  /* we only support IN */
  if (nettype && strcmp (nettype, "IN") != 0)
    return FALSE;

  memset (&hints, 0, sizeof (hints));
  hints.ai_socktype = SOCK_DGRAM;

  /* set the address type as a hint */
  if (addrtype) {
    if (!strcmp (addrtype, "IP4"))
      hints.ai_family = AF_INET;
    else if (!strcmp (addrtype, "IP6"))
      hints.ai_family = AF_INET6;
  }

  if (getaddrinfo (addr, NULL, &hints, &res) < 0)
    return FALSE;

  for (ai = res; !ret && ai; ai = ai->ai_next) {
    if (ai->ai_family == AF_INET)
      ret =
          IN_MULTICAST (ntohl (((struct sockaddr_in *) ai->ai_addr)->
              sin_addr.s_addr));
    else
      ret =
          IN6_IS_ADDR_MULTICAST (&((struct sockaddr_in6 *) ai->
              ai_addr)->sin6_addr);
  }

  freeaddrinfo (res);

  return ret;
}

/**
 * gst_sdp_message_as_text:
 * @msg: a #GstSDPMessage
 *
 * Convert the contents of @msg to a text string.
 *
 * Returns: A dynamically allocated string representing the SDP description.
 */
gchar *
gst_sdp_message_as_text (const GstSDPMessage * msg)
{
  /* change all vars so they match rfc? */
  GString *lines;
  guint i;

  g_return_val_if_fail (msg != NULL, NULL);

  lines = g_string_new ("");

  if (msg->version)
    g_string_append_printf (lines, "v=%s\r\n", msg->version);

  if (msg->origin.sess_id && msg->origin.sess_version && msg->origin.nettype &&
      msg->origin.addrtype && msg->origin.addr)
    g_string_append_printf (lines, "o=%s %s %s %s %s %s\r\n",
        msg->origin.username ? msg->origin.username : "-", msg->origin.sess_id,
        msg->origin.sess_version, msg->origin.nettype, msg->origin.addrtype,
        msg->origin.addr);

  if (msg->session_name)
    g_string_append_printf (lines, "s=%s\r\n", msg->session_name);

  if (msg->information)
    g_string_append_printf (lines, "i=%s\r\n", msg->information);

  if (msg->uri)
    g_string_append_printf (lines, "u=%s\r\n", msg->uri);

  for (i = 0; i < gst_sdp_message_emails_len (msg); i++)
    g_string_append_printf (lines, "e=%s\r\n",
        gst_sdp_message_get_email (msg, i));

  for (i = 0; i < gst_sdp_message_phones_len (msg); i++)
    g_string_append_printf (lines, "p=%s\r\n",
        gst_sdp_message_get_phone (msg, i));

  if (gst_sdp_message_emails_len (msg) == 0 &&
      gst_sdp_message_phones_len (msg) == 0)
    g_string_append_printf (lines, "e=NONE\r\n");

  if (msg->connection.nettype && msg->connection.addrtype &&
      msg->connection.address) {
    g_string_append_printf (lines, "c=%s %s %s", msg->connection.nettype,
        msg->connection.addrtype, msg->connection.address);
    if (gst_sdp_address_is_multicast (msg->connection.nettype,
            msg->connection.addrtype, msg->connection.address)) {
      /* only add ttl for IP4 */
      if (strcmp (msg->connection.addrtype, "IP4") == 0)
        g_string_append_printf (lines, "/%u", msg->connection.ttl);
      if (msg->connection.addr_number > 1)
        g_string_append_printf (lines, "/%u", msg->connection.addr_number);
    }
    g_string_append_printf (lines, "\r\n");
  }

  for (i = 0; i < gst_sdp_message_bandwidths_len (msg); i++) {
    const GstSDPBandwidth *bandwidth = gst_sdp_message_get_bandwidth (msg, i);

    g_string_append_printf (lines, "b=%s:%u\r\n", bandwidth->bwtype,
        bandwidth->bandwidth);
  }

  for (i = 0; i < gst_sdp_message_times_len (msg); i++) {
    const GstSDPTime *times = gst_sdp_message_get_time (msg, i);

    g_string_append_printf (lines, "t=%s %s\r\n", times->start, times->stop);

    if (times->repeat != NULL) {
      guint j;

      g_string_append_printf (lines, "r=%s",
          g_array_index (times->repeat, gchar *, 0));
      for (j = 1; j < times->repeat->len; j++)
        g_string_append_printf (lines, " %s",
            g_array_index (times->repeat, gchar *, j));
      g_string_append_printf (lines, "\r\n");
    }
  }

  if (gst_sdp_message_zones_len (msg) > 0) {
    const GstSDPZone *zone = gst_sdp_message_get_zone (msg, 0);

    g_string_append_printf (lines, "z=%s %s", zone->time, zone->typed_time);
    for (i = 1; i < gst_sdp_message_zones_len (msg); i++) {
      zone = gst_sdp_message_get_zone (msg, i);
      g_string_append_printf (lines, " %s %s", zone->time, zone->typed_time);
    }
    g_string_append_printf (lines, "\r\n");
  }

  if (msg->key.type) {
    g_string_append_printf (lines, "k=%s", msg->key.type);
    if (msg->key.data)
      g_string_append_printf (lines, ":%s", msg->key.data);
    g_string_append_printf (lines, "\r\n");
  }

  for (i = 0; i < gst_sdp_message_attributes_len (msg); i++) {
    const GstSDPAttribute *attr = gst_sdp_message_get_attribute (msg, i);

    if (attr->key) {
      g_string_append_printf (lines, "a=%s", attr->key);
      if (attr->value)
        g_string_append_printf (lines, ":%s", attr->value);
      g_string_append_printf (lines, "\r\n");
    }
  }

  for (i = 0; i < gst_sdp_message_medias_len (msg); i++) {
    const GstSDPMedia *media = gst_sdp_message_get_media (msg, i);
    gchar *sdp_media_str;

    sdp_media_str = gst_sdp_media_as_text (media);
    g_string_append_printf (lines, "%s", sdp_media_str);
    g_free (sdp_media_str);
  }

  return g_string_free (lines, FALSE);
}

static int
hex_to_int (gchar c)
{
  return c >= '0' && c <= '9' ? c - '0'
      : c >= 'A' && c <= 'F' ? c - 'A' + 10
      : c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0;
}

/**
 * gst_sdp_message_parse_uri:
 * @uri: the start of the uri
 * @msg: the result #GstSDPMessage
 *
 * Parse the null-terminated @uri and store the result in @msg.
 *
 * The uri should be of the form:
 *
 *  scheme://[address[:ttl=ttl][:noa=noa]]/[sessionname]
 *               [#type=value *[&type=value]]
 *
 *  where value is url encoded. This looslely resembles
 *  http://tools.ietf.org/html/draft-fujikawa-sdp-url-01
 *
 * Returns: #GST_SDP_OK on success.
 *
 * Since: 0.10.31
 */
GstSDPResult
gst_sdp_message_parse_uri (const gchar * uri, GstSDPMessage * msg)
{
  GstSDPResult res;
  gchar *message;
  const gchar *colon, *slash, *hash, *p;
  GString *lines;

  g_return_val_if_fail (uri != NULL, GST_SDP_EINVAL);
  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);

  colon = strstr (uri, "://");
  if (!colon)
    goto no_colon;

  /* FIXME connection info goes here */

  slash = strstr (colon + 3, "/");
  if (!slash)
    goto no_slash;

  /* FIXME session name goes here */

  hash = strstr (slash + 1, "#");
  if (!hash)
    goto no_hash;

  lines = g_string_new ("");

  /* unescape */
  for (p = hash + 1; *p; p++) {
    if (*p == '&')
      g_string_append_printf (lines, "\r\n");
    else if (*p == '+')
      g_string_append_c (lines, ' ');
    else if (*p == '%') {
      gchar a, b;

      if ((a = p[1])) {
        if ((b = p[2])) {
          g_string_append_c (lines, (hex_to_int (a) << 4) | hex_to_int (b));
          p += 2;
        }
      } else {
        p++;
      }
    } else
      g_string_append_c (lines, *p);
  }

  message = g_string_free (lines, FALSE);
  res =
      gst_sdp_message_parse_buffer ((const guint8 *) message, strlen (message),
      msg);
  g_free (message);

  return res;

  /* ERRORS */
no_colon:
  {
    return GST_SDP_EINVAL;
  }
no_slash:
  {
    return GST_SDP_EINVAL;
  }
no_hash:
  {
    return GST_SDP_EINVAL;
  }
}

static const guchar acceptable[96] = {
  /* X0   X1    X2    X3    X4    X5    X6    X7    X8    X9    XA    XB    XC    XD    XE    XF */
  0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00,       /* 2X  !"#$%&'()*+,-./   */
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       /* 3X 0123456789:;<=>?   */
  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,       /* 4X @ABCDEFGHIJKLMNO   */
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,       /* 5X PQRSTUVWXYZ[\]^_   */
  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,       /* 6X `abcdefghijklmno   */
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00        /* 7X pqrstuvwxyz{|}~DEL */
};

static const gchar hex[16] = "0123456789ABCDEF";

#define ACCEPTABLE_CHAR(a) (((guchar)(a))>=32 && ((guchar)(a))<128 && acceptable[(((guchar)a))-32])

/**
 * gst_sdp_message_as_uri:
 * @scheme: the uri scheme
 * @msg: the #GstSDPMessage
 *
 * Creates a uri from @msg with the given @scheme. The uri has the format:
 *
 *  @scheme:///[#type=value *[&type=value]]
 *
 *  Where each value is url encoded.
 *
 * Returns: a uri for @msg.
 *
 * Since: 0.10.31
 */
gchar *
gst_sdp_message_as_uri (const gchar * scheme, const GstSDPMessage * msg)
{
  gchar *serialized, *p;
  gchar *res;
  GString *lines;
  gboolean first;

  g_return_val_if_fail (scheme != NULL, NULL);
  g_return_val_if_fail (msg != NULL, NULL);

  p = serialized = gst_sdp_message_as_text (msg);

  lines = g_string_new ("");
  g_string_append_printf (lines, "%s:///#", scheme);

  /* now escape */
  first = TRUE;
  for (p = serialized; *p; p++) {
    if (first) {
      g_string_append_printf (lines, "%c=", *p);
      if (*(p + 1))
        p++;
      first = FALSE;
      continue;
    }
    if (*p == '\r')
      continue;
    else if (*p == '\n') {
      if (*(p + 1))
        g_string_append_c (lines, '&');
      first = TRUE;
    } else if (*p == ' ')
      g_string_append_c (lines, '+');
    else if (ACCEPTABLE_CHAR (*p))
      g_string_append_c (lines, *p);
    else {
      /* escape */
      g_string_append_printf (lines, "%%%c%c", hex[*p >> 4], hex[*p & 0xf]);
    }
  }

  res = g_string_free (lines, FALSE);
  g_free (serialized);

  return res;
}

/**
 * gst_sdp_message_set_version:
 * @msg: a #GstSDPMessage
 * @version: the version
 *
 * Set the version in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_SETTER (version);
/**
 * gst_sdp_message_get_version:
 * @msg: a #GstSDPMessage
 *
 * Get the version in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_GETTER (version);

/**
 * gst_sdp_message_set_origin:
 * @msg: a #GstSDPMessage
 * @username: the user name
 * @sess_id: a session id
 * @sess_version: a session version
 * @nettype: a network type
 * @addrtype: an address type
 * @addr: an address
 *
 * Configure the SDP origin in @msg with the given parameters.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_message_set_origin (GstSDPMessage * msg, const gchar * username,
    const gchar * sess_id, const gchar * sess_version, const gchar * nettype,
    const gchar * addrtype, const gchar * addr)
{
  REPLACE_STRING (msg->origin.username, username);
  REPLACE_STRING (msg->origin.sess_id, sess_id);
  REPLACE_STRING (msg->origin.sess_version, sess_version);
  REPLACE_STRING (msg->origin.nettype, nettype);
  REPLACE_STRING (msg->origin.addrtype, addrtype);
  REPLACE_STRING (msg->origin.addr, addr);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_get_origin:
 * @msg: a #GstSDPMessage
 *
 * Get the origin of @msg.
 *
 * Returns: a #GstSDPOrigin. The result remains valid as long as @msg is valid.
 */
const GstSDPOrigin *
gst_sdp_message_get_origin (const GstSDPMessage * msg)
{
  return &msg->origin;
}

/**
 * gst_sdp_message_set_session_name:
 * @msg: a #GstSDPMessage
 * @session_name: the session name
 *
 * Set the session name in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_SETTER (session_name);
/**
 * gst_sdp_message_get_session_name:
 * @msg: a #GstSDPMessage
 *
 * Get the session name in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_GETTER (session_name);
/**
 * gst_sdp_message_set_information:
 * @msg: a #GstSDPMessage
 * @information: the information
 *
 * Set the information in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_SETTER (information);
/**
 * gst_sdp_message_get_information:
 * @msg: a #GstSDPMessage
 *
 * Get the information in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_GETTER (information);
/**
 * gst_sdp_message_set_uri:
 * @msg: a #GstSDPMessage
 * @uri: the URI
 *
 * Set the URI in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_SETTER (uri);
/**
 * gst_sdp_message_get_uri:
 * @msg: a #GstSDPMessage
 *
 * Get the URI in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_STRING_GETTER (uri);

/**
 * gst_sdp_message_emails_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of emails in @msg.
 *
 * Returns: the number of emails in @msg.
 */
DEFINE_ARRAY_LEN (emails);
/**
 * gst_sdp_message_get_email:
 * @msg: a #GstSDPMessage
 * @idx: an email index
 *
 * Get the email with number @idx from @msg.
 *
 * Returns: the email at position @idx.
 */
DEFINE_PTR_ARRAY_GETTER (email, emails, const gchar *);

/**
 * gst_sdp_message_add_email:
 * @msg: a #GstSDPMessage
 * @email: an email
 *
 * Add @email to the list of emails in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_PTR_ARRAY_ADDER (email, emails, const gchar *, g_strdup);

/**
 * gst_sdp_message_phones_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of phones in @msg.
 *
 * Returns: the number of phones in @msg.
 */
DEFINE_ARRAY_LEN (phones);
/**
 * gst_sdp_message_get_phone:
 * @msg: a #GstSDPMessage
 * @idx: a phone index
 *
 * Get the phone with number @idx from @msg.
 *
 * Returns: the phone at position @idx.
 */
DEFINE_PTR_ARRAY_GETTER (phone, phones, const gchar *);

/**
 * gst_sdp_message_add_phone:
 * @msg: a #GstSDPMessage
 * @phone: a phone
 *
 * Add @phone to the list of phones in @msg.
 *
 * Returns: a #GstSDPResult.
 */
DEFINE_PTR_ARRAY_ADDER (phone, phones, const gchar *, g_strdup);

/**
 * gst_sdp_message_set_connection:
 * @msg: a #GstSDPMessage
 * @nettype: the type of network. "IN" is defined to have the meaning
 * "Internet".
 * @addrtype: the type of address.
 * @address: the address
 * @ttl: the time to live of the address
 * @addr_number: the number of layers
 *
 * Configure the SDP connection in @msg with the given parameters.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_set_connection (GstSDPMessage * msg, const gchar * nettype,
    const gchar * addrtype, const gchar * address, guint ttl, guint addr_number)
{
  REPLACE_STRING (msg->connection.nettype, nettype);
  REPLACE_STRING (msg->connection.addrtype, addrtype);
  REPLACE_STRING (msg->connection.address, address);
  msg->connection.ttl = ttl;
  msg->connection.addr_number = addr_number;

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_get_connection:
 * @msg: a #GstSDPMessage
 *
 * Get the connection of @msg.
 *
 * Returns: a #GstSDPConnection. The result remains valid as long as @msg is valid.
 */
const GstSDPConnection *
gst_sdp_message_get_connection (const GstSDPMessage * msg)
{
  return &msg->connection;
}

/**
 * gst_sdp_message_bandwidths_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of bandwidth information in @msg.
 *
 * Returns: the number of bandwidth information in @msg.
 */
DEFINE_ARRAY_LEN (bandwidths);
/**
 * gst_sdp_message_get_bandwidth:
 * @msg: a #GstSDPMessage
 * @idx: the bandwidth index
 *
 * Get the bandwidth at index @idx from @msg.
 *
 * Returns: a #GstSDPBandwidth.
 */
DEFINE_ARRAY_GETTER (bandwidth, bandwidths, const GstSDPBandwidth);

/**
 * gst_sdp_message_add_bandwidth:
 * @msg: a #GstSDPMessage
 * @bwtype: the bandwidth modifier type
 * @bandwidth: the bandwidth in kilobits per second
 *
 * Add the specified bandwidth information to @msg.
 *
 * Returns: a #GstSDPResult.
 */

GstSDPResult
gst_sdp_message_add_bandwidth (GstSDPMessage * msg, const gchar * bwtype,
    guint bandwidth)
{
  GstSDPBandwidth bw;

  bw.bwtype = g_strdup (bwtype);
  bw.bandwidth = bandwidth;

  g_array_append_val (msg->bandwidths, bw);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_times_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of time information entries in @msg.
 *
 * Returns: the number of time information entries in @msg.
 */
DEFINE_ARRAY_LEN (times);

/**
 * gst_sdp_message_get_time:
 * @msg: a #GstSDPMessage
 * @idx: the time index
 *
 * Get time information with index @idx from @msg.
 *
 * Returns: a #GstSDPTime.
 */
DEFINE_ARRAY_GETTER (time, times, const GstSDPTime);

/**
 * gst_sdp_message_add_time:
 * @msg: a #GstSDPMessage
 * @start: the start time
 * @stop: the stop time
 * @repeat: the repeat times
 *
 * Add time information @start and @stop to @msg.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_add_time (GstSDPMessage * msg, const gchar * start,
    const gchar * stop, const gchar ** repeat)
{
  GstSDPTime times;

  times.start = g_strdup (start);
  times.stop = g_strdup (stop);
  if (repeat) {
    times.repeat = g_array_new (FALSE, TRUE, sizeof (gchar *));
    for (; *repeat; repeat++) {
      gchar *r = g_strdup (*repeat);

      g_array_append_val (times.repeat, r);
    }
  } else
    times.repeat = NULL;
  g_array_append_val (msg->times, times);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_zones_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of time zone information entries in @msg.
 *
 * Returns: the number of time zone information entries in @msg.
 */
DEFINE_ARRAY_LEN (zones);
/**
 * gst_sdp_message_get_zone:
 * @msg: a #GstSDPMessage
 * @idx: the zone index
 *
 * Get time zone information with index @idx from @msg.
 *
 * Returns: a #GstSDPZone.
 */
DEFINE_ARRAY_GETTER (zone, zones, const GstSDPZone);

/**
 * gst_sdp_message_add_zone:
 * @msg: a #GstSDPMessage
 * @adj_time: the NTP time that a time zone adjustment happens
 * @typed_time: the offset from the time when the session was first scheduled
 *
 * Add time zone information to @msg.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_add_zone (GstSDPMessage * msg, const gchar * adj_time,
    const gchar * typed_time)
{
  GstSDPZone zone;

  zone.time = g_strdup (adj_time);
  zone.typed_time = g_strdup (typed_time);

  g_array_append_val (msg->zones, zone);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_set_key:
 * @msg: a #GstSDPMessage
 * @type: the encryption type
 * @data: the encryption data
 *
 * Adds the encryption information to @msg.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_set_key (GstSDPMessage * msg, const gchar * type,
    const gchar * data)
{
  REPLACE_STRING (msg->key.type, type);
  REPLACE_STRING (msg->key.data, data);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_get_key:
 * @msg: a #GstSDPMessage
 *
 * Get the encryption information from @msg.
 *
 * Returns: a #GstSDPKey.
 */
const GstSDPKey *
gst_sdp_message_get_key (const GstSDPMessage * msg)
{
  return &msg->key;
}

/**
 * gst_sdp_message_attributes_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of attributes in @msg.
 *
 * Returns: the number of attributes in @msg.
 */
DEFINE_ARRAY_LEN (attributes);

/**
 * gst_sdp_message_get_attribute:
 * @msg: a #GstSDPMessage
 * @idx: the index
 *
 * Get the attribute at position @idx in @msg.
 *
 * Returns: the #GstSDPAttribute at position @idx.
 */
DEFINE_ARRAY_GETTER (attribute, attributes, const GstSDPAttribute);

/**
 * gst_sdp_message_get_attribute_val_n:
 * @msg: a #GstSDPMessage
 * @key: the key
 * @nth: the index
 *
 * Get the @nth attribute with key @key in @msg.
 *
 * Returns: the attribute value of the @nth attribute with @key.
 */
const gchar *
gst_sdp_message_get_attribute_val_n (const GstSDPMessage * msg,
    const gchar * key, guint nth)
{
  guint i;

  for (i = 0; i < msg->attributes->len; i++) {
    GstSDPAttribute *attr;

    attr = &g_array_index (msg->attributes, GstSDPAttribute, i);
    if (!strcmp (attr->key, key)) {
      if (nth == 0)
        return attr->value;
      else
        nth--;
    }
  }
  return NULL;
}

/**
 * gst_sdp_message_get_attribute_val:
 * @msg: a #GstSDPMessage
 * @key: the key
 *
 * Get the first attribute with key @key in @msg.
 *
 * Returns: the attribute value of the first attribute with @key.
 */
const gchar *
gst_sdp_message_get_attribute_val (const GstSDPMessage * msg, const gchar * key)
{
  return gst_sdp_message_get_attribute_val_n (msg, key, 0);
}

/**
 * gst_sdp_message_add_attribute:
 * @msg: a #GstSDPMessage
 * @key: the key
 * @value: the value
 *
 * Add the attribute with @key and @value to @msg.
 *
 * Returns: @GST_SDP_OK.
 */
GstSDPResult
gst_sdp_message_add_attribute (GstSDPMessage * msg, const gchar * key,
    const gchar * value)
{
  GstSDPAttribute attr;

  attr.key = g_strdup (key);
  attr.value = g_strdup (value);

  g_array_append_val (msg->attributes, attr);

  return GST_SDP_OK;
}

/**
 * gst_sdp_message_medias_len:
 * @msg: a #GstSDPMessage
 *
 * Get the number of media descriptions in @msg.
 *
 * Returns: the number of media descriptions in @msg.
 */
DEFINE_ARRAY_LEN (medias);
/**
 * gst_sdp_message_get_media:
 * @msg: a #GstSDPMessage
 * @idx: the index
 *
 * Get the media description at index @idx in @msg.
 *
 * Returns: a #GstSDPMedia.
 */
DEFINE_ARRAY_GETTER (media, medias, const GstSDPMedia);

/**
 * gst_sdp_message_add_media:
 * @msg: a #GstSDPMessage
 * @media: a #GstSDPMedia to add
 *
 * Adds @media to the array of medias in @msg. This function takes ownership of
 * the contents of @media so that @media will have to be reinitialized with
 * gst_sdp_media_init() before it can be used again.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_add_media (GstSDPMessage * msg, GstSDPMedia * media)
{
  guint len;
  GstSDPMedia *nmedia;

  len = msg->medias->len;
  g_array_set_size (msg->medias, len + 1);
  nmedia = &g_array_index (msg->medias, GstSDPMedia, len);

  memcpy (nmedia, media, sizeof (GstSDPMedia));
  memset (media, 0, sizeof (GstSDPMedia));

  return GST_SDP_OK;
}

/* media access */

/**
 * gst_sdp_media_new:
 * @media: pointer to new #GstSDPMedia
 *
 * Allocate a new GstSDPMedia and store the result in @media.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_media_new (GstSDPMedia ** media)
{
  GstSDPMedia *newmedia;

  g_return_val_if_fail (media != NULL, GST_SDP_EINVAL);

  newmedia = g_new0 (GstSDPMedia, 1);

  *media = newmedia;

  return gst_sdp_media_init (newmedia);
}

/**
 * gst_sdp_media_init:
 * @media: a #GstSDPMedia
 *
 * Initialize @media so that its contents are as if it was freshly allocated
 * with gst_sdp_media_new(). This function is mostly used to initialize a media
 * allocated on the stack. gst_sdp_media_uninit() undoes this operation.
 *
 * When this function is invoked on newly allocated data (with malloc or on the
 * stack), its contents should be set to 0 before calling this function.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_media_init (GstSDPMedia * media)
{
  g_return_val_if_fail (media != NULL, GST_SDP_EINVAL);

  FREE_STRING (media->media);
  media->port = 0;
  media->num_ports = 0;
  FREE_STRING (media->proto);
  INIT_PTR_ARRAY (media->fmts, gchar *, g_free);
  FREE_STRING (media->information);
  INIT_ARRAY (media->connections, GstSDPConnection, gst_sdp_connection_init);
  INIT_ARRAY (media->bandwidths, GstSDPBandwidth, gst_sdp_bandwidth_init);
  gst_sdp_key_init (&media->key);
  INIT_ARRAY (media->attributes, GstSDPAttribute, gst_sdp_attribute_init);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_uninit:
 * @media: a #GstSDPMedia
 *
 * Free all resources allocated in @media. @media should not be used anymore after
 * this function. This function should be used when @media was allocated on the
 * stack and initialized with gst_sdp_media_init().
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_media_uninit (GstSDPMedia * media)
{
  g_return_val_if_fail (media != NULL, GST_SDP_EINVAL);

  gst_sdp_media_init (media);
  FREE_PTR_ARRAY (media->fmts);
  FREE_ARRAY (media->connections);
  FREE_ARRAY (media->bandwidths);
  FREE_ARRAY (media->attributes);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_free:
 * @media: a #GstSDPMedia
 *
 * Free all resources allocated by @media. @media should not be used anymore after
 * this function. This function should be used when @media was dynamically
 * allocated with gst_sdp_media_new().
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_media_free (GstSDPMedia * media)
{
  g_return_val_if_fail (media != NULL, GST_SDP_EINVAL);

  gst_sdp_media_uninit (media);
  g_free (media);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_as_text:
 * @media: a #GstSDPMedia
 *
 * Convert the contents of @media to a text string.
 *
 * Returns: A dynamically allocated string representing the media.
 */
gchar *
gst_sdp_media_as_text (const GstSDPMedia * media)
{
  GString *lines;
  guint i;

  g_return_val_if_fail (media != NULL, NULL);

  lines = g_string_new ("");

  if (media->media)
    g_string_append_printf (lines, "m=%s", media->media);

  g_string_append_printf (lines, " %u", media->port);

  if (media->num_ports > 1)
    g_string_append_printf (lines, "/%u", media->num_ports);

  g_string_append_printf (lines, " %s", media->proto);

  for (i = 0; i < gst_sdp_media_formats_len (media); i++)
    g_string_append_printf (lines, " %s", gst_sdp_media_get_format (media, i));
  g_string_append_printf (lines, "\r\n");

  if (media->information)
    g_string_append_printf (lines, "i=%s", media->information);

  for (i = 0; i < gst_sdp_media_connections_len (media); i++) {
    const GstSDPConnection *conn = gst_sdp_media_get_connection (media, i);

    if (conn->nettype && conn->addrtype && conn->address) {
      g_string_append_printf (lines, "c=%s %s %s", conn->nettype,
          conn->addrtype, conn->address);
      if (gst_sdp_address_is_multicast (conn->nettype, conn->addrtype,
              conn->address)) {
        /* only add TTL for IP4 multicast */
        if (strcmp (conn->addrtype, "IP4") == 0)
          g_string_append_printf (lines, "/%u", conn->ttl);
        if (conn->addr_number > 1)
          g_string_append_printf (lines, "/%u", conn->addr_number);
      }
      g_string_append_printf (lines, "\r\n");
    }
  }

  for (i = 0; i < gst_sdp_media_bandwidths_len (media); i++) {
    const GstSDPBandwidth *bandwidth = gst_sdp_media_get_bandwidth (media, i);

    g_string_append_printf (lines, "b=%s:%u\r\n", bandwidth->bwtype,
        bandwidth->bandwidth);
  }

  if (media->key.type) {
    g_string_append_printf (lines, "k=%s", media->key.type);
    if (media->key.data)
      g_string_append_printf (lines, ":%s", media->key.data);
    g_string_append_printf (lines, "\r\n");
  }

  for (i = 0; i < gst_sdp_media_attributes_len (media); i++) {
    const GstSDPAttribute *attr = gst_sdp_media_get_attribute (media, i);

    if (attr->key) {
      g_string_append_printf (lines, "a=%s", attr->key);
      if (attr->value)
        g_string_append_printf (lines, ":%s", attr->value);
      g_string_append_printf (lines, "\r\n");
    }
  }

  return g_string_free (lines, FALSE);
}

/**
 * gst_sdp_media_get_media:
 * @media: a #GstSDPMedia
 *
 * Get the media description of @media.
 *
 * Returns: the media description.
 */
const gchar *
gst_sdp_media_get_media (const GstSDPMedia * media)
{
  return media->media;
}

/**
 * gst_sdp_media_set_media:
 * @media: a #GstSDPMedia
 * @med: the media description
 *
 * Set the media description of @media to @med.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_set_media (GstSDPMedia * media, const gchar * med)
{
  g_free (media->media);
  media->media = g_strdup (med);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_get_port:
 * @media: a #GstSDPMedia
 *
 * Get the port number for @media.
 *
 * Returns: the port number of @media.
 */
guint
gst_sdp_media_get_port (const GstSDPMedia * media)
{
  return media->port;
}

/**
 * gst_sdp_media_get_num_ports:
 * @media: a #GstSDPMedia
 *
 * Get the number of ports for @media.
 *
 * Returns: the number of ports for @media.
 */
guint
gst_sdp_media_get_num_ports (const GstSDPMedia * media)
{
  return media->num_ports;
}

/**
 * gst_sdp_media_set_port_info:
 * @media: a #GstSDPMedia
 * @port: the port number
 * @num_ports: the number of ports
 *
 * Set the port information in @media.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_set_port_info (GstSDPMedia * media, guint port, guint num_ports)
{
  media->port = port;
  media->num_ports = num_ports;

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_get_proto:
 * @media: a #GstSDPMedia
 *
 * Get the transport protocol of @media
 *
 * Returns: the transport protocol of @media.
 */
const gchar *
gst_sdp_media_get_proto (const GstSDPMedia * media)
{
  return media->proto;
}

/**
 * gst_sdp_media_set_proto:
 * @media: a #GstSDPMedia
 * @proto: the media transport protocol
 *
 * Set the media transport protocol of @media to @proto.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_set_proto (GstSDPMedia * media, const gchar * proto)
{
  g_free (media->proto);
  media->proto = g_strdup (proto);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_formats_len:
 * @media: a #GstSDPMedia
 *
 * Get the number of formats in @media.
 *
 * Returns: the number of formats in @media.
 */
guint
gst_sdp_media_formats_len (const GstSDPMedia * media)
{
  return media->fmts->len;
}

/**
 * gst_sdp_media_get_format:
 * @media: a #GstSDPMedia
 * @idx: an index
 *
 * Get the format information at position @idx in @media.
 *
 * Returns: the format at position @idx.
 */
const gchar *
gst_sdp_media_get_format (const GstSDPMedia * media, guint idx)
{
  if (idx >= media->fmts->len)
    return NULL;
  return g_array_index (media->fmts, gchar *, idx);
}

/**
 * gst_sdp_media_add_format:
 * @media: a #GstSDPMedia
 * @format: the format
 *
 * Add the format information to @media.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_add_format (GstSDPMedia * media, const gchar * format)
{
  gchar *fmt;

  fmt = g_strdup (format);

  g_array_append_val (media->fmts, fmt);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_get_information:
 * @media: a #GstSDPMedia
 *
 * Get the information of @media
 *
 * Returns: the information of @media.
 */
const gchar *
gst_sdp_media_get_information (const GstSDPMedia * media)
{
  return media->information;
}

/**
 * gst_sdp_media_set_information:
 * @media: a #GstSDPMedia
 * @information: the media information
 *
 * Set the media information of @media to @information.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_set_information (GstSDPMedia * media, const gchar * information)
{
  g_free (media->information);
  media->information = g_strdup (information);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_connections_len:
 * @media: a #GstSDPMedia
 *
 * Get the number of connection fields in @media.
 *
 * Returns: the number of connections in @media.
 */
guint
gst_sdp_media_connections_len (const GstSDPMedia * media)
{
  return media->connections->len;
}

/**
 * gst_sdp_media_get_connection:
 * @media: a #GstSDPMedia
 * @idx: an index
 *
 * Get the connection at position @idx in @media.
 *
 * Returns: the #GstSDPConnection at position @idx.
 */
const GstSDPConnection *
gst_sdp_media_get_connection (const GstSDPMedia * media, guint idx)
{
  return &g_array_index (media->connections, GstSDPConnection, idx);
}

/**
 * gst_sdp_media_add_connection:
 * @media: a #GstSDPMedia
 * @nettype: the type of network. "IN" is defined to have the meaning
 * "Internet".
 * @addrtype: the type of address.
 * @address: the address
 * @ttl: the time to live of the address
 * @addr_number: the number of layers
 *
 * Add the given connection parameters to @media.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_media_add_connection (GstSDPMedia * media, const gchar * nettype,
    const gchar * addrtype, const gchar * address, guint ttl, guint addr_number)
{
  GstSDPConnection conn;

  conn.nettype = g_strdup (nettype);
  conn.addrtype = g_strdup (addrtype);
  conn.address = g_strdup (address);
  conn.ttl = ttl;
  conn.addr_number = addr_number;

  g_array_append_val (media->connections, conn);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_bandwidths_len:
 * @media: a #GstSDPMedia
 *
 * Get the number of bandwidth fields in @media.
 *
 * Returns: the number of bandwidths in @media.
 */
guint
gst_sdp_media_bandwidths_len (const GstSDPMedia * media)
{
  return media->bandwidths->len;
}

/**
 * gst_sdp_media_get_bandwidth:
 * @media: a #GstSDPMedia
 * @idx: an index
 *
 * Get the bandwidth at position @idx in @media.
 *
 * Returns: the #GstSDPBandwidth at position @idx.
 */
const GstSDPBandwidth *
gst_sdp_media_get_bandwidth (const GstSDPMedia * media, guint idx)
{
  return &g_array_index (media->bandwidths, GstSDPBandwidth, idx);
}

/**
 * gst_sdp_media_add_bandwidth:
 * @media: a #GstSDPMedia
 * @bwtype: the bandwidth modifier type
 * @bandwidth: the bandwidth in kilobits per second
 *
 * Add the bandwidth information with @bwtype and @bandwidth to @media.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_add_bandwidth (GstSDPMedia * media, const gchar * bwtype,
    guint bandwidth)
{
  GstSDPBandwidth bw;

  bw.bwtype = g_strdup (bwtype);
  bw.bandwidth = bandwidth;

  g_array_append_val (media->bandwidths, bw);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_set_key:
 * @media: a #GstSDPMedia
 * @type: the encryption type
 * @data: the encryption data
 *
 * Adds the encryption information to @media.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_media_set_key (GstSDPMedia * media, const gchar * type,
    const gchar * data)
{
  g_free (media->key.type);
  media->key.type = g_strdup (type);
  g_free (media->key.data);
  media->key.data = g_strdup (data);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_get_key:
 * @media: a #GstSDPMedia
 *
 * Get the encryption information from @media.
 *
 * Returns: a #GstSDPKey.
 */
const GstSDPKey *
gst_sdp_media_get_key (const GstSDPMedia * media)
{
  return &media->key;
}

/**
 * gst_sdp_media_attributes_len:
 * @media: a #GstSDPMedia
 *
 * Get the number of attribute fields in @media.
 *
 * Returns: the number of attributes in @media.
 */
guint
gst_sdp_media_attributes_len (const GstSDPMedia * media)
{
  return media->attributes->len;
}

/**
 * gst_sdp_media_add_attribute:
 * @media: a #GstSDPMedia
 * @key: a key
 * @value: a value
 *
 * Add the attribute with @key and @value to @media.
 *
 * Returns: #GST_SDP_OK.
 */
GstSDPResult
gst_sdp_media_add_attribute (GstSDPMedia * media, const gchar * key,
    const gchar * value)
{
  GstSDPAttribute attr;

  attr.key = g_strdup (key);
  attr.value = g_strdup (value);

  g_array_append_val (media->attributes, attr);

  return GST_SDP_OK;
}

/**
 * gst_sdp_media_get_attribute:
 * @media: a #GstSDPMedia
 * @idx: an index
 *
 * Get the attribute at position @idx in @media.
 *
 * Returns: the #GstSDPAttribute at position @idx.
 */
const GstSDPAttribute *
gst_sdp_media_get_attribute (const GstSDPMedia * media, guint idx)
{
  return &g_array_index (media->attributes, GstSDPAttribute, idx);
}

/**
 * gst_sdp_media_get_attribute_val_n:
 * @media: a #GstSDPMedia
 * @key: a key
 * @nth: an index
 *
 * Get the @nth attribute value for @key in @media.
 *
 * Returns: the @nth attribute value.
 */
const gchar *
gst_sdp_media_get_attribute_val_n (const GstSDPMedia * media, const gchar * key,
    guint nth)
{
  guint i;

  for (i = 0; i < media->attributes->len; i++) {
    GstSDPAttribute *attr;

    attr = &g_array_index (media->attributes, GstSDPAttribute, i);
    if (!strcmp (attr->key, key)) {
      if (nth == 0)
        return attr->value;
      else
        nth--;
    }
  }
  return NULL;
}

/**
 * gst_sdp_media_get_attribute_val:
 * @media: a #GstSDPMedia
 * @key: a key
 *
 * Get the first attribute value for @key in @media.
 *
 * Returns: the first attribute value for @key.
 */
const gchar *
gst_sdp_media_get_attribute_val (const GstSDPMedia * media, const gchar * key)
{
  return gst_sdp_media_get_attribute_val_n (media, key, 0);
}

static void
read_string (gchar * dest, guint size, gchar ** src)
{
  guint idx;

  idx = 0;
  /* skip spaces */
  while (g_ascii_isspace (**src))
    (*src)++;

  while (!g_ascii_isspace (**src) && **src != '\0') {
    if (idx < size - 1)
      dest[idx++] = **src;
    (*src)++;
  }
  if (size > 0)
    dest[idx] = '\0';
}

static void
read_string_del (gchar * dest, guint size, gchar del, gchar ** src)
{
  guint idx;

  idx = 0;
  /* skip spaces */
  while (g_ascii_isspace (**src))
    (*src)++;

  while (**src != del && **src != '\0') {
    if (idx < size - 1)
      dest[idx++] = **src;
    (*src)++;
  }
  if (size > 0)
    dest[idx] = '\0';
}

enum
{
  SDP_SESSION,
  SDP_MEDIA,
};

typedef struct
{
  guint state;
  GstSDPMessage *msg;
  GstSDPMedia *media;
} SDPContext;

static gboolean
gst_sdp_parse_line (SDPContext * c, gchar type, gchar * buffer)
{
  gchar str[8192];
  gchar *p = buffer;

#define READ_STRING(field) read_string (str, sizeof (str), &p); REPLACE_STRING (field, str)
#define READ_UINT(field) read_string (str, sizeof (str), &p); field = strtoul (str, NULL, 10)

  switch (type) {
    case 'v':
      if (buffer[0] != '0')
        g_warning ("wrong SDP version");
      gst_sdp_message_set_version (c->msg, buffer);
      break;
    case 'o':
      READ_STRING (c->msg->origin.username);
      READ_STRING (c->msg->origin.sess_id);
      READ_STRING (c->msg->origin.sess_version);
      READ_STRING (c->msg->origin.nettype);
      READ_STRING (c->msg->origin.addrtype);
      READ_STRING (c->msg->origin.addr);
      break;
    case 's':
      REPLACE_STRING (c->msg->session_name, buffer);
      break;
    case 'i':
      if (c->state == SDP_SESSION) {
        REPLACE_STRING (c->msg->information, buffer);
      } else {
        REPLACE_STRING (c->media->information, buffer);
      }
      break;
    case 'u':
      REPLACE_STRING (c->msg->uri, buffer);
      break;
    case 'e':
      gst_sdp_message_add_email (c->msg, buffer);
      break;
    case 'p':
      gst_sdp_message_add_phone (c->msg, buffer);
      break;
    case 'c':
    {
      GstSDPConnection conn;
      gchar *str2;

      memset (&conn, 0, sizeof (conn));

      str2 = p;
      while ((str2 = strchr (str2, '/')))
        *str2++ = ' ';
      READ_STRING (conn.nettype);
      READ_STRING (conn.addrtype);
      READ_STRING (conn.address);
      /* only read TTL for IP4 */
      if (strcmp (conn.addrtype, "IP4") == 0)
        READ_UINT (conn.ttl);
      READ_UINT (conn.addr_number);

      if (c->state == SDP_SESSION) {
        gst_sdp_message_set_connection (c->msg, conn.nettype, conn.addrtype,
            conn.address, conn.ttl, conn.addr_number);
      } else {
        gst_sdp_media_add_connection (c->media, conn.nettype, conn.addrtype,
            conn.address, conn.ttl, conn.addr_number);
      }
      gst_sdp_connection_init (&conn);
      break;
    }
    case 'b':
    {
      gchar str2[MAX_LINE_LEN];

      read_string_del (str, sizeof (str), ':', &p);
      if (*p != '\0')
        p++;
      read_string (str2, sizeof (str2), &p);
      if (c->state == SDP_SESSION)
        gst_sdp_message_add_bandwidth (c->msg, str, atoi (str2));
      else
        gst_sdp_media_add_bandwidth (c->media, str, atoi (str2));
      break;
    }
    case 't':
      break;
    case 'k':
      break;
    case 'a':
      read_string_del (str, sizeof (str), ':', &p);
      if (*p != '\0')
        p++;
      if (c->state == SDP_SESSION)
        gst_sdp_message_add_attribute (c->msg, str, p);
      else
        gst_sdp_media_add_attribute (c->media, str, p);
      break;
    case 'm':
    {
      gchar *slash;
      GstSDPMedia nmedia;

      c->state = SDP_MEDIA;
      memset (&nmedia, 0, sizeof (nmedia));
      gst_sdp_media_init (&nmedia);

      /* m=<media> <port>/<number of ports> <proto> <fmt> ... */
      READ_STRING (nmedia.media);
      read_string (str, sizeof (str), &p);
      slash = g_strrstr (str, "/");
      if (slash) {
        *slash = '\0';
        nmedia.port = atoi (str);
        nmedia.num_ports = atoi (slash + 1);
      } else {
        nmedia.port = atoi (str);
        nmedia.num_ports = -1;
      }
      READ_STRING (nmedia.proto);
      do {
        read_string (str, sizeof (str), &p);
        gst_sdp_media_add_format (&nmedia, str);
      } while (*p != '\0');

      gst_sdp_message_add_media (c->msg, &nmedia);
      c->media =
          &g_array_index (c->msg->medias, GstSDPMedia, c->msg->medias->len - 1);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

/**
 * gst_sdp_message_parse_buffer:
 * @data: the start of the buffer
 * @size: the size of the buffer
 * @msg: the result #GstSDPMessage
 *
 * Parse the contents of @size bytes pointed to by @data and store the result in
 * @msg.
 *
 * Returns: #GST_SDP_OK on success.
 */
GstSDPResult
gst_sdp_message_parse_buffer (const guint8 * data, guint size,
    GstSDPMessage * msg)
{
  gchar *p;
  SDPContext c;
  gchar type;
  gchar buffer[MAX_LINE_LEN];
  guint idx = 0;

  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);
  g_return_val_if_fail (data != NULL, GST_SDP_EINVAL);
  g_return_val_if_fail (size != 0, GST_SDP_EINVAL);

  c.state = SDP_SESSION;
  c.msg = msg;
  c.media = NULL;

  p = (gchar *) data;
  while (TRUE) {
    while (g_ascii_isspace (*p))
      p++;

    type = *p++;
    if (type == '\0')
      break;

    if (*p != '=')
      goto line_done;
    p++;

    idx = 0;
    while (*p != '\n' && *p != '\r' && *p != '\0') {
      if (idx < sizeof (buffer) - 1)
        buffer[idx++] = *p;
      p++;
    }
    buffer[idx] = '\0';
    gst_sdp_parse_line (&c, type, buffer);

  line_done:
    while (*p != '\n' && *p != '\0')
      p++;
    if (*p == '\n')
      p++;
  }

  return GST_SDP_OK;
}

static void
print_media (GstSDPMedia * media)
{
  g_print ("   media:       '%s'\n", GST_STR_NULL (media->media));
  g_print ("   port:        '%u'\n", media->port);
  g_print ("   num_ports:   '%u'\n", media->num_ports);
  g_print ("   proto:       '%s'\n", GST_STR_NULL (media->proto));
  if (media->fmts->len > 0) {
    guint i;

    g_print ("   formats:\n");
    for (i = 0; i < media->fmts->len; i++) {
      g_print ("    format  '%s'\n", g_array_index (media->fmts, gchar *, i));
    }
  }
  g_print ("   information: '%s'\n", GST_STR_NULL (media->information));
  if (media->connections->len > 0) {
    guint i;

    g_print ("   connections:\n");
    for (i = 0; i < media->connections->len; i++) {
      GstSDPConnection *conn =
          &g_array_index (media->connections, GstSDPConnection, i);

      g_print ("    nettype:      '%s'\n", GST_STR_NULL (conn->nettype));
      g_print ("    addrtype:     '%s'\n", GST_STR_NULL (conn->addrtype));
      g_print ("    address:      '%s'\n", GST_STR_NULL (conn->address));
      g_print ("    ttl:          '%u'\n", conn->ttl);
      g_print ("    addr_number:  '%u'\n", conn->addr_number);
    }
  }
  if (media->bandwidths->len > 0) {
    guint i;

    g_print ("   bandwidths:\n");
    for (i = 0; i < media->bandwidths->len; i++) {
      GstSDPBandwidth *bw =
          &g_array_index (media->bandwidths, GstSDPBandwidth, i);

      g_print ("    type:         '%s'\n", GST_STR_NULL (bw->bwtype));
      g_print ("    bandwidth:    '%u'\n", bw->bandwidth);
    }
  }
  g_print ("   key:\n");
  g_print ("    type:       '%s'\n", GST_STR_NULL (media->key.type));
  g_print ("    data:       '%s'\n", GST_STR_NULL (media->key.data));
  if (media->attributes->len > 0) {
    guint i;

    g_print ("   attributes:\n");
    for (i = 0; i < media->attributes->len; i++) {
      GstSDPAttribute *attr =
          &g_array_index (media->attributes, GstSDPAttribute, i);

      g_print ("    attribute '%s' : '%s'\n", attr->key, attr->value);
    }
  }
}

/**
 * gst_sdp_message_dump:
 * @msg: a #GstSDPMessage
 *
 * Dump the parsed contents of @msg to stdout.
 *
 * Returns: a #GstSDPResult.
 */
GstSDPResult
gst_sdp_message_dump (const GstSDPMessage * msg)
{
  g_return_val_if_fail (msg != NULL, GST_SDP_EINVAL);

  g_print ("sdp packet %p:\n", msg);
  g_print (" version:       '%s'\n", GST_STR_NULL (msg->version));
  g_print (" origin:\n");
  g_print ("  username:     '%s'\n", GST_STR_NULL (msg->origin.username));
  g_print ("  sess_id:      '%s'\n", GST_STR_NULL (msg->origin.sess_id));
  g_print ("  sess_version: '%s'\n", GST_STR_NULL (msg->origin.sess_version));
  g_print ("  nettype:      '%s'\n", GST_STR_NULL (msg->origin.nettype));
  g_print ("  addrtype:     '%s'\n", GST_STR_NULL (msg->origin.addrtype));
  g_print ("  addr:         '%s'\n", GST_STR_NULL (msg->origin.addr));
  g_print (" session_name:  '%s'\n", GST_STR_NULL (msg->session_name));
  g_print (" information:   '%s'\n", GST_STR_NULL (msg->information));
  g_print (" uri:           '%s'\n", GST_STR_NULL (msg->uri));

  if (msg->emails->len > 0) {
    guint i;

    g_print (" emails:\n");
    for (i = 0; i < msg->emails->len; i++) {
      g_print ("  email '%s'\n", g_array_index (msg->emails, gchar *, i));
    }
  }
  if (msg->phones->len > 0) {
    guint i;

    g_print (" phones:\n");
    for (i = 0; i < msg->phones->len; i++) {
      g_print ("  phone '%s'\n", g_array_index (msg->phones, gchar *, i));
    }
  }
  g_print (" connection:\n");
  g_print ("  nettype:      '%s'\n", GST_STR_NULL (msg->connection.nettype));
  g_print ("  addrtype:     '%s'\n", GST_STR_NULL (msg->connection.addrtype));
  g_print ("  address:      '%s'\n", GST_STR_NULL (msg->connection.address));
  g_print ("  ttl:          '%u'\n", msg->connection.ttl);
  g_print ("  addr_number:  '%u'\n", msg->connection.addr_number);
  if (msg->bandwidths->len > 0) {
    guint i;

    g_print (" bandwidths:\n");
    for (i = 0; i < msg->bandwidths->len; i++) {
      GstSDPBandwidth *bw =
          &g_array_index (msg->bandwidths, GstSDPBandwidth, i);

      g_print ("  type:         '%s'\n", GST_STR_NULL (bw->bwtype));
      g_print ("  bandwidth:    '%u'\n", bw->bandwidth);
    }
  }
  g_print (" key:\n");
  g_print ("  type:         '%s'\n", GST_STR_NULL (msg->key.type));
  g_print ("  data:         '%s'\n", GST_STR_NULL (msg->key.data));
  if (msg->attributes->len > 0) {
    guint i;

    g_print (" attributes:\n");
    for (i = 0; i < msg->attributes->len; i++) {
      GstSDPAttribute *attr =
          &g_array_index (msg->attributes, GstSDPAttribute, i);

      g_print ("  attribute '%s' : '%s'\n", attr->key, attr->value);
    }
  }
  if (msg->medias->len > 0) {
    guint i;

    g_print (" medias:\n");
    for (i = 0; i < msg->medias->len; i++) {
      g_print ("  media %u:\n", i);
      print_media (&g_array_index (msg->medias, GstSDPMedia, i));
    }
  }
  return GST_SDP_OK;
}
