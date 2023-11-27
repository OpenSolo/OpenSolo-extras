/* GStreamer
 * Copyright (C) 2009 Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:element-rtpj2kpay
 *
 * Payload encode JPEG 2000 pictures into RTP packets according to RFC 5371.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc5371.txt
 *
 * The payloader takes a JPEG 2000 picture, scans the header for packetization
 * units and constructs the RTP packet header followed by the actual JPEG 2000
 * codestream.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpj2kpay.h"

static GstStaticPadTemplate gst_rtp_j2k_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/x-jpc")
    );

static GstStaticPadTemplate gst_rtp_j2k_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "  media = (string) \"video\", "
        "  payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "  clock-rate = (int) 90000, "
        "  encoding-name = (string) \"JPEG2000\"")
    );

GST_DEBUG_CATEGORY_STATIC (rtpj2kpay_debug);
#define GST_CAT_DEFAULT (rtpj2kpay_debug)

/*
 * RtpJ2KMarker:
 * @J2K_MARKER: Prefix for JPEG 2000 marker
 * @J2K_MARKER_SOC: Start of Codestream
 * @J2K_MARKER_SOT: Start of tile
 * @J2K_MARKER_EOC: End of Codestream
 *
 * Identifers for markers in JPEG 2000 codestreams
 */
typedef enum
{
  J2K_MARKER = 0xFF,
  J2K_MARKER_SOC = 0x4F,
  J2K_MARKER_SOT = 0x90,
  J2K_MARKER_SOP = 0x91,
  J2K_MARKER_EPH = 0x92,
  J2K_MARKER_SOD = 0x93,
  J2K_MARKER_EOC = 0xD9
} RtpJ2KMarker;

#define DEFAULT_BUFFER_LIST             TRUE

enum
{
  PROP_0,
  PROP_BUFFER_LIST,
  PROP_LAST
};

typedef struct
{
  guint tp:2;
  guint MHF:2;
  guint mh_id:3;
  guint T:1;
  guint priority:8;
  guint tile:16;
  guint offset:24;
} RtpJ2KHeader;

#define HEADER_SIZE 8

static void gst_rtp_j2k_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_j2k_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rtp_j2k_pay_setcaps (GstBaseRTPPayload * basepayload,
    GstCaps * caps);

static GstFlowReturn gst_rtp_j2k_pay_handle_buffer (GstBaseRTPPayload * pad,
    GstBuffer * buffer);

GST_BOILERPLATE (GstRtpJ2KPay, gst_rtp_j2k_pay, GstBaseRTPPayload,
    GST_TYPE_BASE_RTP_PAYLOAD);

static void
gst_rtp_j2k_pay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_j2k_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_j2k_pay_sink_template);

  gst_element_class_set_details_simple (element_class,
      "RTP JPEG 2000 payloader", "Codec/Payloader/Network/RTP",
      "Payload-encodes JPEG 2000 pictures into RTP packets (RFC 5371)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_j2k_pay_class_init (GstRtpJ2KPayClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseRTPPayloadClass *gstbasertppayload_class;

  gobject_class = (GObjectClass *) klass;
  gstbasertppayload_class = (GstBaseRTPPayloadClass *) klass;

  gobject_class->set_property = gst_rtp_j2k_pay_set_property;
  gobject_class->get_property = gst_rtp_j2k_pay_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BUFFER_LIST,
      g_param_spec_boolean ("buffer-list", "Buffer List",
          "Use Buffer Lists",
          DEFAULT_BUFFER_LIST, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstbasertppayload_class->set_caps = gst_rtp_j2k_pay_setcaps;
  gstbasertppayload_class->handle_buffer = gst_rtp_j2k_pay_handle_buffer;

  GST_DEBUG_CATEGORY_INIT (rtpj2kpay_debug, "rtpj2kpay", 0,
      "JPEG 2000 RTP Payloader");
}

static void
gst_rtp_j2k_pay_init (GstRtpJ2KPay * pay, GstRtpJ2KPayClass * klass)
{
  pay->buffer_list = DEFAULT_BUFFER_LIST;
}

static gboolean
gst_rtp_j2k_pay_setcaps (GstBaseRTPPayload * basepayload, GstCaps * caps)
{
  GstStructure *caps_structure = gst_caps_get_structure (caps, 0);
  GstRtpJ2KPay *pay;
  gint width = 0, height = 0;
  gboolean res;

  pay = GST_RTP_J2K_PAY (basepayload);

  /* these properties are not mandatory, we can get them from the stream */
  if (gst_structure_get_int (caps_structure, "height", &height)) {
    pay->height = height;
  }
  if (gst_structure_get_int (caps_structure, "width", &width)) {
    pay->width = width;
  }

  gst_basertppayload_set_options (basepayload, "video", TRUE, "JPEG2000",
      90000);
  res = gst_basertppayload_set_outcaps (basepayload, NULL);

  return res;
}


static guint
gst_rtp_j2k_pay_header_size (const guint8 * data, guint offset)
{
  return data[offset] << 8 | data[offset + 1];
}

static RtpJ2KMarker
gst_rtp_j2k_pay_scan_marker (const guint8 * data, guint size, guint * offset)
{
  while ((data[(*offset)++] != J2K_MARKER) && ((*offset) < size));

  if (G_UNLIKELY ((*offset) >= size)) {
    return J2K_MARKER_EOC;
  } else {
    guint8 marker = data[(*offset)++];
    return marker;
  }
}

typedef struct
{
  RtpJ2KHeader header;
  gboolean bitstream;
  guint n_tiles;
  guint next_sot;
  gboolean force_packet;
} RtpJ2KState;

static guint
find_pu_end (GstRtpJ2KPay * pay, const guint8 * data, guint size,
    guint offset, RtpJ2KState * state)
{
  gboolean cut_sop = FALSE;
  RtpJ2KMarker marker;

  /* parse the j2k header for 'start of codestream' */
  GST_LOG_OBJECT (pay, "checking from offset %u", offset);
  while (offset < size) {
    marker = gst_rtp_j2k_pay_scan_marker (data, size, &offset);

    if (state->bitstream) {
      /* parsing bitstream, only look for SOP */
      switch (marker) {
        case J2K_MARKER_SOP:
          GST_LOG_OBJECT (pay, "found SOP at %u", offset);
          if (cut_sop)
            return offset - 2;
          cut_sop = TRUE;
          break;
        case J2K_MARKER_EPH:
          /* just skip over EPH */
          GST_LOG_OBJECT (pay, "found EPH at %u", offset);
          break;
        default:
          if (offset >= state->next_sot) {
            GST_LOG_OBJECT (pay, "reached next SOT at %u", offset);
            state->bitstream = FALSE;
            state->force_packet = TRUE;
            if (marker == J2K_MARKER_EOC && state->next_sot + 2 <= size)
              /* include EOC but never go past the max size */
              return state->next_sot + 2;
            else
              return state->next_sot;
          }
          break;
      }
    } else {
      switch (marker) {
        case J2K_MARKER_SOC:
          GST_LOG_OBJECT (pay, "found SOC at %u", offset);
          state->header.MHF = 1;
          break;
        case J2K_MARKER_SOT:
        {
          guint len, Psot;

          GST_LOG_OBJECT (pay, "found SOT at %u", offset);
          /* we found SOT but also had a header first */
          if (state->header.MHF) {
            state->force_packet = TRUE;
            return offset - 2;
          }

          /* parse SOT but do some sanity checks first */
          len = gst_rtp_j2k_pay_header_size (data, offset);
          GST_LOG_OBJECT (pay, "SOT length %u", len);
          if (len < 8)
            return size;
          if (offset + len >= size)
            return size;

          if (state->n_tiles == 0)
            /* first tile, T is valid */
            state->header.T = 0;
          else
            /* more tiles, T becomes invalid */
            state->header.T = 1;
          state->header.tile = GST_READ_UINT16_BE (&data[offset + 2]);
          state->n_tiles++;

          /* get offset of next tile, if it's 0, it goes all the way to the end of
           * the data */
          Psot = GST_READ_UINT32_BE (&data[offset + 4]);
          if (Psot == 0)
            state->next_sot = size;
          else
            state->next_sot = offset - 2 + Psot;

          offset += len;
          GST_LOG_OBJECT (pay, "Isot %u, Psot %u, next %u", state->header.tile,
              Psot, state->next_sot);
          break;
        }
        case J2K_MARKER_SOD:
          GST_LOG_OBJECT (pay, "found SOD at %u", offset);
          /* can't have more tiles now */
          state->n_tiles = 0;
          /* go to bitstream parsing */
          state->bitstream = TRUE;
          /* cut at the next SOP or else include all data */
          cut_sop = TRUE;
          /* force a new packet when we see SOP, this can be optional but the
           * spec recommends packing headers separately */
          state->force_packet = TRUE;
          break;
        case J2K_MARKER_EOC:
          GST_LOG_OBJECT (pay, "found EOC at %u", offset);
          return offset;
        default:
        {
          guint len = gst_rtp_j2k_pay_header_size (data, offset);
          GST_LOG_OBJECT (pay, "skip 0x%02x len %u", marker, len);
          offset += len;
          break;
        }
      }
    }
  }
  GST_DEBUG_OBJECT (pay, "reached end of data");
  return size;
}

static GstFlowReturn
gst_rtp_j2k_pay_handle_buffer (GstBaseRTPPayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpJ2KPay *pay;
  GstClockTime timestamp;
  GstFlowReturn ret = GST_FLOW_ERROR;
  RtpJ2KState state;
  GstBufferList *list = NULL;
  GstBufferListIterator *it = NULL;
  guint8 *data;
  guint size;
  guint mtu, max_size;
  guint offset;
  guint end, pos;

  pay = GST_RTP_J2K_PAY (basepayload);
  mtu = GST_BASE_RTP_PAYLOAD_MTU (pay);

  size = GST_BUFFER_SIZE (buffer);
  data = GST_BUFFER_DATA (buffer);
  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  offset = pos = end = 0;

  GST_LOG_OBJECT (pay, "got buffer size %u, timestamp %" GST_TIME_FORMAT, size,
      GST_TIME_ARGS (timestamp));

  /* do some header defaults first */
  state.header.tp = 0;          /* only progressive scan */
  state.header.MHF = 0;         /* no header */
  state.header.mh_id = 0;       /* always 0 for now */
  state.header.T = 1;           /* invalid tile */
  state.header.priority = 255;  /* always 255 for now */
  state.header.tile = 0;        /* no tile number */
  state.header.offset = 0;      /* offset of 0 */
  state.bitstream = FALSE;
  state.n_tiles = 0;
  state.next_sot = 0;
  state.force_packet = FALSE;

  if (pay->buffer_list) {
    list = gst_buffer_list_new ();
    it = gst_buffer_list_iterate (list);
  }

  /* get max packet length */
  max_size = gst_rtp_buffer_calc_payload_len (mtu - HEADER_SIZE, 0, 0);

  do {
    GstBuffer *outbuf;
    guint8 *header;
    guint payload_size;
    guint pu_size;

    /* try to pack as much as we can */
    do {
      /* see how much we have scanned already */
      pu_size = end - offset;
      GST_DEBUG_OBJECT (pay, "scanned pu size %u", pu_size);

      /* we need to make a new packet */
      if (state.force_packet) {
        GST_DEBUG_OBJECT (pay, "need to force a new packet");
        state.force_packet = FALSE;
        pos = end;
        break;
      }

      /* else see if we have enough */
      if (pu_size > max_size) {
        if (pos != offset)
          /* the packet became too large, use previous scanpos */
          pu_size = pos - offset;
        else
          /* the already scanned data was already too big, make sure we start
           * scanning from the last searched position */
          pos = end;

        GST_DEBUG_OBJECT (pay, "max size exceeded pu_size %u", pu_size);
        break;
      }

      pos = end;

      /* exit when finished */
      if (pos == size)
        break;

      /* scan next packetization unit and fill in the header */
      end = find_pu_end (pay, data, size, pos, &state);
    } while (TRUE);

    while (pu_size > 0) {
      guint packet_size, data_size;

      /* calculate the packet size */
      packet_size =
          gst_rtp_buffer_calc_packet_len (pu_size + HEADER_SIZE, 0, 0);

      if (packet_size > mtu) {
        GST_DEBUG_OBJECT (pay, "needed packet size %u clamped to MTU %u",
            packet_size, mtu);
        packet_size = mtu;
      } else {
        GST_DEBUG_OBJECT (pay, "needed packet size %u fits in MTU %u",
            packet_size, mtu);
      }

      /* get total payload size and data size */
      payload_size = gst_rtp_buffer_calc_payload_len (packet_size, 0, 0);
      data_size = payload_size - HEADER_SIZE;

      if (pay->buffer_list) {
        /* make buffer for header */
        outbuf = gst_rtp_buffer_new_allocate (HEADER_SIZE, 0, 0);
      } else {
        /* make buffer for header and data */
        outbuf = gst_rtp_buffer_new_allocate (payload_size, 0, 0);
      }
      GST_BUFFER_TIMESTAMP (outbuf) = timestamp;

      /* get pointer to header */
      header = gst_rtp_buffer_get_payload (outbuf);

      pu_size -= data_size;
      if (pu_size == 0) {
        /* reached the end of a packetization unit */
        if (state.header.MHF) {
          /* we were doing a header, see if all fit in one packet or if
           * we had to fragment it */
          if (offset == 0)
            state.header.MHF = 3;
          else
            state.header.MHF = 2;
        }
        if (end >= size)
          gst_rtp_buffer_set_marker (outbuf, TRUE);
      }

      /*
       * RtpJ2KHeader:
       * @tp: type (0 progressive, 1 odd field, 2 even field)
       * @MHF: Main Header Flag
       * @mh_id: Main Header Identification
       * @T: Tile field invalidation flag
       * @priority: priority
       * @tile number: the tile number of the payload
       * @reserved: set to 0
       * @fragment offset: the byte offset of the current payload
       *
       *  0                   1                   2                   3
       *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |tp |MHF|mh_id|T|     priority  |           tile number         |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |reserved       |             fragment offset                   |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
      header[0] = (state.header.tp << 6) | (state.header.MHF << 4) |
          (state.header.mh_id << 1) | state.header.T;
      header[1] = state.header.priority;
      header[2] = state.header.tile >> 8;
      header[3] = state.header.tile & 0xff;
      header[4] = 0;
      header[5] = state.header.offset >> 16;
      header[6] = (state.header.offset >> 8) & 0xff;
      header[7] = state.header.offset & 0xff;

      if (pay->buffer_list) {
        GstBuffer *paybuf;

        /* make subbuffer of j2k data */
        paybuf = gst_buffer_create_sub (buffer, offset, data_size);

        /* create a new group to hold the header and the payload */
        gst_buffer_list_iterator_add_group (it);

        /* add both buffers to the buffer list */
        gst_buffer_list_iterator_add (it, outbuf);
        gst_buffer_list_iterator_add (it, paybuf);
      } else {
        /* copy payload */
        memcpy (header + HEADER_SIZE, &data[offset], data_size);

        ret = gst_basertppayload_push (basepayload, outbuf);
        if (ret != GST_FLOW_OK)
          goto done;
      }

      /* reset header for next round */
      state.header.MHF = 0;
      state.header.T = 1;
      state.header.tile = 0;

      offset += data_size;
    }
    offset = pos;
  } while (offset < size);

done:
  gst_buffer_unref (buffer);

  if (pay->buffer_list) {
    /* free iterator and push the whole buffer list at once */
    gst_buffer_list_iterator_free (it);
    ret = gst_basertppayload_push_list (basepayload, list);
  }

  return ret;
}

static void
gst_rtp_j2k_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpJ2KPay *rtpj2kpay;

  rtpj2kpay = GST_RTP_J2K_PAY (object);

  switch (prop_id) {
    case PROP_BUFFER_LIST:
      rtpj2kpay->buffer_list = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_j2k_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpJ2KPay *rtpj2kpay;

  rtpj2kpay = GST_RTP_J2K_PAY (object);

  switch (prop_id) {
    case PROP_BUFFER_LIST:
      g_value_set_boolean (value, rtpj2kpay->buffer_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_rtp_j2k_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpj2kpay", GST_RANK_SECONDARY,
      GST_TYPE_RTP_J2K_PAY);
}
