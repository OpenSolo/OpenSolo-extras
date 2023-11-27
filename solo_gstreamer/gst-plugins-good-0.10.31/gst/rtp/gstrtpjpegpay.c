/* GStreamer
 * Copyright (C) 2008 Axis Communications <dev-gstreamer@axis.com>
 * @author Bjorn Ostby <bjorn.ostby@axis.com>
 * @author Peter Kjellerstedt <peter.kjellerstedt@axis.com>
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
 * SECTION:element-rtpjpegpay
 *
 * Payload encode JPEG pictures into RTP packets according to RFC 2435.
 * For detailed information see: http://www.rfc-editor.org/rfc/rfc2435.txt
 *
 * The payloader takes a JPEG picture, scans the header for quantization
 * tables (if needed) and constructs the RTP packet header followed by
 * the actual JPEG entropy scan.
 *
 * The payloader assumes that correct width and height is found in the caps.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpjpegpay.h"

static GstStaticPadTemplate gst_rtp_jpeg_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg; " "video/x-jpeg")
    );

static GstStaticPadTemplate gst_rtp_jpeg_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "  media = (string) \"video\", "
        "  payload = (int) 26 ,        "
        "  clock-rate = (int) 90000,   " "  encoding-name = (string) \"JPEG\"")
    );

GST_DEBUG_CATEGORY_STATIC (rtpjpegpay_debug);
#define GST_CAT_DEFAULT (rtpjpegpay_debug)

/*
 * QUANT_PREFIX_LEN:
 *
 * Prefix length in the header before the quantization tables:
 * Two size bytes and one byte for precision
 */
#define QUANT_PREFIX_LEN     3

/*
 * DEFAULT_BUFFER_LIST:
 *
 */
#define DEFAULT_BUFFER_LIST            FALSE

typedef enum _RtpJpegMarker RtpJpegMarker;

/*
 * RtpJpegMarker:
 * @JPEG_MARKER: Prefix for JPEG marker
 * @JPEG_MARKER_SOI: Start of Image marker
 * @JPEG_MARKER_JFIF: JFIF marker
 * @JPEG_MARKER_CMT: Comment marker
 * @JPEG_MARKER_DQT: Define Quantization Table marker
 * @JPEG_MARKER_SOF: Start of Frame marker
 * @JPEG_MARKER_DHT: Define Huffman Table marker
 * @JPEG_MARKER_SOS: Start of Scan marker
 * @JPEG_MARKER_EOI: End of Image marker
 * @JPEG_MARKER_DRI: Define Restart Interval marker
 * @JPEG_MARKER_H264: H264 marker
 *
 * Identifers for markers in JPEG header
 */
enum _RtpJpegMarker
{
  JPEG_MARKER = 0xFF,
  JPEG_MARKER_SOI = 0xD8,
  JPEG_MARKER_JFIF = 0xE0,
  JPEG_MARKER_CMT = 0xFE,
  JPEG_MARKER_DQT = 0xDB,
  JPEG_MARKER_SOF = 0xC0,
  JPEG_MARKER_DHT = 0xC4,
  JPEG_MARKER_SOS = 0xDA,
  JPEG_MARKER_EOI = 0xD9,
  JPEG_MARKER_DRI = 0xDD,
  JPEG_MARKER_H264 = 0xE4
};

#define DEFAULT_JPEG_QUANT    255

#define DEFAULT_JPEG_QUALITY  255
#define DEFAULT_JPEG_TYPE     1

enum
{
  PROP_0,
  PROP_JPEG_QUALITY,
  PROP_JPEG_TYPE,
  PROP_BUFFER_LIST,
  PROP_LAST
};

enum
{
  Q_TABLE_0 = 0,
  Q_TABLE_1,
  Q_TABLE_MAX                   /* only support for two tables at the moment */
};

typedef struct _RtpJpegHeader RtpJpegHeader;

/*
 * RtpJpegHeader:
 * @type_spec: type specific
 * @offset: fragment offset
 * @type: type field
 * @q: quantization table for this frame
 * @width: width of image in 8-pixel multiples
 * @height: height of image in 8-pixel multiples
 *
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Type-specific |              Fragment Offset                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |       Q       |     Width     |     Height    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct _RtpJpegHeader
{
  guint type_spec:8;
  guint offset:24;
  guint8 type;
  guint8 q;
  guint8 width;
  guint8 height;
};

/*
 * RtpQuantHeader
 * @mbz: must be zero
 * @precision: specify size of quantization tables
 * @length: length of quantization data
 *
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      MBZ      |   Precision   |             Length            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Quantization Table Data                    |
 * |                              ...                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct
{
  guint8 mbz;
  guint8 precision;
  guint16 length;
} RtpQuantHeader;

typedef struct
{
  guint8 size;
  const guint8 *data;
} RtpQuantTable;

/*
 * RtpRestartMarkerHeader:
 * @restartInterval: number of MCUs that appear between restart markers
 * @restartFirstLastCount: a combination of the first packet mark in the chunk
 *                         last packet mark in the chunk and the position of the
 *                         first restart interval in the current "chunk"
 *
 *    0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       Restart Interval        |F|L|       Restart Count       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  The restart marker header is implemented according to the following
 *  methodology specified in section 3.1.7 of rfc2435.txt.
 *
 *  "If the restart intervals in a frame are not guaranteed to be aligned
 *  with packet boundaries, the F (first) and L (last) bits MUST be set
 *  to 1 and the Restart Count MUST be set to 0x3FFF.  This indicates
 *  that a receiver MUST reassemble the entire frame before decoding it."
 *
 */

typedef struct
{
  guint16 restart_interval;
  guint16 restart_count;
} RtpRestartMarkerHeader;

typedef struct
{
  guint8 id;
  guint8 samp;
  guint8 qt;
} CompInfo;

/* FIXME: restart marker header currently unsupported */

static void gst_rtp_jpeg_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_rtp_jpeg_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rtp_jpeg_pay_setcaps (GstBaseRTPPayload * basepayload,
    GstCaps * caps);

static GstFlowReturn gst_rtp_jpeg_pay_handle_buffer (GstBaseRTPPayload * pad,
    GstBuffer * buffer);

GST_BOILERPLATE (GstRtpJPEGPay, gst_rtp_jpeg_pay, GstBaseRTPPayload,
    GST_TYPE_BASE_RTP_PAYLOAD);

static void
gst_rtp_jpeg_pay_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_jpeg_pay_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_jpeg_pay_sink_template);

  gst_element_class_set_details_simple (element_class, "RTP JPEG payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encodes JPEG pictures into RTP packets (RFC 2435)",
      "Axis Communications <dev-gstreamer@axis.com>");
}

static void
gst_rtp_jpeg_pay_class_init (GstRtpJPEGPayClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseRTPPayloadClass *gstbasertppayload_class;

  gobject_class = (GObjectClass *) klass;
  gstbasertppayload_class = (GstBaseRTPPayloadClass *) klass;

  gobject_class->set_property = gst_rtp_jpeg_pay_set_property;
  gobject_class->get_property = gst_rtp_jpeg_pay_get_property;

  gstbasertppayload_class->set_caps = gst_rtp_jpeg_pay_setcaps;
  gstbasertppayload_class->handle_buffer = gst_rtp_jpeg_pay_handle_buffer;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_JPEG_QUALITY,
      g_param_spec_int ("quality", "Quality",
          "Quality factor on JPEG data (unused)", 0, 255, 255,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_JPEG_TYPE,
      g_param_spec_int ("type", "Type",
          "Default JPEG Type, overwritten by SOF when present", 0, 255,
          DEFAULT_JPEG_TYPE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BUFFER_LIST,
      g_param_spec_boolean ("buffer-list", "Buffer List",
          "Use Buffer Lists",
          DEFAULT_BUFFER_LIST, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (rtpjpegpay_debug, "rtpjpegpay", 0,
      "Motion JPEG RTP Payloader");
}

static void
gst_rtp_jpeg_pay_init (GstRtpJPEGPay * pay, GstRtpJPEGPayClass * klass)
{
  pay->quality = DEFAULT_JPEG_QUALITY;
  pay->quant = DEFAULT_JPEG_QUANT;
  pay->type = DEFAULT_JPEG_TYPE;
  pay->buffer_list = DEFAULT_BUFFER_LIST;
}

static gboolean
gst_rtp_jpeg_pay_setcaps (GstBaseRTPPayload * basepayload, GstCaps * caps)
{
  GstStructure *caps_structure = gst_caps_get_structure (caps, 0);
  GstRtpJPEGPay *pay;
  gboolean res;
  gint width = 0, height = 0;

  pay = GST_RTP_JPEG_PAY (basepayload);

  /* these properties are not mandatory, we can get them from the SOF, if there
   * is one. */
  if (gst_structure_get_int (caps_structure, "height", &height)) {
    if (height <= 0 || height > 2040)
      goto invalid_dimension;
  }
  pay->height = GST_ROUND_UP_8 (height) / 8;

  if (gst_structure_get_int (caps_structure, "width", &width)) {
    if (width <= 0 || width > 2040)
      goto invalid_dimension;
  }
  pay->width = GST_ROUND_UP_8 (width) / 8;

  gst_basertppayload_set_options (basepayload, "video", TRUE, "JPEG", 90000);
  res = gst_basertppayload_set_outcaps (basepayload, NULL);

  return res;

  /* ERRORS */
invalid_dimension:
  {
    GST_ERROR_OBJECT (pay, "Invalid width/height from caps");
    return FALSE;
  }
}

static guint
gst_rtp_jpeg_pay_header_size (const guint8 * data, guint offset)
{
  return data[offset] << 8 | data[offset + 1];
}

static guint
gst_rtp_jpeg_pay_read_quant_table (const guint8 * data, guint size,
    guint offset, RtpQuantTable tables[])
{
  guint quant_size, tab_size;
  guint8 prec;
  guint8 id;

  if (offset + 2 > size)
    goto too_small;

  quant_size = gst_rtp_jpeg_pay_header_size (data, offset);
  if (quant_size < 2)
    goto small_quant_size;

  /* clamp to available data */
  if (offset + quant_size > size)
    quant_size = size - offset;

  offset += 2;
  quant_size -= 2;

  while (quant_size > 0) {
    /* not enough to read the id */
    if (offset + 1 > size)
      break;

    id = data[offset] & 0x0f;
    if (id == 15)
      /* invalid id received - corrupt data */
      goto invalid_id;

    prec = (data[offset] & 0xf0) >> 4;
    if (prec)
      tab_size = 128;
    else
      tab_size = 64;

    /* there is not enough for the table */
    if (quant_size < tab_size + 1)
      goto no_table;

    GST_LOG ("read quant table %d, tab_size %d, prec %02x", id, tab_size, prec);

    tables[id].size = tab_size;
    tables[id].data = &data[offset + 1];

    tab_size += 1;
    quant_size -= tab_size;
    offset += tab_size;
  }
done:
  return offset + quant_size;

  /* ERRORS */
too_small:
  {
    GST_WARNING ("not enough data");
    return size;
  }
small_quant_size:
  {
    GST_WARNING ("quant_size too small (%u < 2)", quant_size);
    return size;
  }
invalid_id:
  {
    GST_WARNING ("invalid id");
    goto done;
  }
no_table:
  {
    GST_WARNING ("not enough data for table (%u < %u)", quant_size,
        tab_size + 1);
    goto done;
  }
}

static gboolean
gst_rtp_jpeg_pay_read_sof (GstRtpJPEGPay * pay, const guint8 * data,
    guint size, guint * offset, CompInfo info[])
{
  guint sof_size, off;
  guint width, height, infolen;
  CompInfo elem;
  gint i, j;

  off = *offset;

  /* we need at least 17 bytes for the SOF */
  if (off + 17 > size)
    goto wrong_size;

  sof_size = gst_rtp_jpeg_pay_header_size (data, off);
  if (sof_size < 17)
    goto wrong_length;

  *offset += sof_size;

  /* skip size */
  off += 2;

  /* precision should be 8 */
  if (data[off++] != 8)
    goto bad_precision;

  /* read dimensions */
  height = data[off] << 8 | data[off + 1];
  width = data[off + 2] << 8 | data[off + 3];
  off += 4;

  GST_LOG_OBJECT (pay, "got dimensions %ux%u", height, width);

  if (height == 0 || height > 2040)
    goto invalid_dimension;
  if (width == 0 || width > 2040)
    goto invalid_dimension;

  pay->height = GST_ROUND_UP_8 (height) / 8;
  pay->width = GST_ROUND_UP_8 (width) / 8;

  /* we only support 3 components */
  if (data[off++] != 3)
    goto bad_components;

  infolen = 0;
  for (i = 0; i < 3; i++) {
    elem.id = data[off++];
    elem.samp = data[off++];
    elem.qt = data[off++];
    GST_LOG_OBJECT (pay, "got comp %d, samp %02x, qt %d", elem.id, elem.samp,
        elem.qt);
    /* insertion sort from the last element to the first */
    for (j = infolen; j > 1; j--) {
      if (G_LIKELY (info[j - 1].id < elem.id))
        break;
      info[j] = info[j - 1];
    }
    info[j] = elem;
    infolen++;
  }

  /* see that the components are supported */
  if (info[0].samp == 0x21)
    pay->type = 0;
  else if (info[0].samp == 0x22)
    pay->type = 1;
  else
    goto invalid_comp;

  if (!(info[1].samp == 0x11))
    goto invalid_comp;

  if (!(info[2].samp == 0x11))
    goto invalid_comp;

  /* the other components are free to use any quant table but they have to
   * have the same table id */
  if (info[1].qt != info[2].qt)
    goto invalid_comp;

  return TRUE;

  /* ERRORS */
wrong_size:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT,
        ("Wrong size %u (needed %u).", size, off + 17), (NULL));
    return FALSE;
  }
wrong_length:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT,
        ("Wrong SOF length %u.", sof_size), (NULL));
    return FALSE;
  }
bad_precision:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT,
        ("Wrong precision, expecting 8."), (NULL));
    return FALSE;
  }
invalid_dimension:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT,
        ("Wrong dimension, size %ux%u", width, height), (NULL));
    return FALSE;
  }
bad_components:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT,
        ("Wrong number of components"), (NULL));
    return FALSE;
  }
invalid_comp:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT, ("Invalid component"), (NULL));
    return FALSE;
  }
}

static gboolean
gst_rtp_jpeg_pay_read_dri (GstRtpJPEGPay * pay, const guint8 * data,
    guint size, guint * offset, RtpRestartMarkerHeader * dri)
{
  guint dri_size, off;

  off = *offset;

  /* we need at least 4 bytes for the DRI */
  if (off + 4 > size)
    goto wrong_size;

  dri_size = gst_rtp_jpeg_pay_header_size (data, off);
  if (dri_size < 4)
    goto wrong_length;

  *offset += dri_size;
  off += 2;

  dri->restart_interval = g_htons ((data[off] << 8) | (data[off + 1]));
  dri->restart_count = g_htons (0xFFFF);

  return dri->restart_interval > 0;

wrong_size:
  {
    GST_WARNING ("not enough data for DRI");
    *offset = size;
    return FALSE;
  }
wrong_length:
  {
    GST_WARNING ("DRI size too small (%u)", dri_size);
    *offset += dri_size;
    return FALSE;
  }
}

static RtpJpegMarker
gst_rtp_jpeg_pay_scan_marker (const guint8 * data, guint size, guint * offset)
{
  while ((data[(*offset)++] != JPEG_MARKER) && ((*offset) < size));

  if (G_UNLIKELY ((*offset) >= size)) {
    GST_LOG ("found EOI marker");
    return JPEG_MARKER_EOI;
  } else {
    guint8 marker;

    marker = data[*offset];
    GST_LOG ("found 0x%02x marker at offset %u", marker, *offset);
    (*offset)++;
    return marker;
  }
}

static GstFlowReturn
gst_rtp_jpeg_pay_handle_buffer (GstBaseRTPPayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpJPEGPay *pay;
  GstClockTime timestamp;
  GstFlowReturn ret = GST_FLOW_ERROR;
  RtpJpegHeader jpeg_header;
  RtpQuantHeader quant_header;
  RtpRestartMarkerHeader restart_marker_header;
  RtpQuantTable tables[15] = { {0, NULL}, };
  CompInfo info[3] = { {0,}, };
  guint quant_data_size;
  guint8 *data;
  guint size;
  guint mtu;
  guint bytes_left;
  guint jpeg_header_size = 0;
  guint offset;
  gboolean frame_done;
  gboolean sos_found, sof_found, dqt_found, dri_found;
  gint i;
  GstBufferList *list = NULL;
  GstBufferListIterator *it = NULL;

  pay = GST_RTP_JPEG_PAY (basepayload);
  mtu = GST_BASE_RTP_PAYLOAD_MTU (pay);

  size = GST_BUFFER_SIZE (buffer);
  data = GST_BUFFER_DATA (buffer);
  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  offset = 0;

  GST_LOG_OBJECT (pay, "got buffer size %u, timestamp %" GST_TIME_FORMAT, size,
      GST_TIME_ARGS (timestamp));

  /* parse the jpeg header for 'start of scan' and read quant tables if needed */
  sos_found = FALSE;
  dqt_found = FALSE;
  sof_found = FALSE;
  dri_found = FALSE;

  while (!sos_found && (offset < size)) {
    GST_LOG_OBJECT (pay, "checking from offset %u", offset);
    switch (gst_rtp_jpeg_pay_scan_marker (data, size, &offset)) {
      case JPEG_MARKER_JFIF:
      case JPEG_MARKER_CMT:
      case JPEG_MARKER_DHT:
      case JPEG_MARKER_H264:
        GST_LOG_OBJECT (pay, "skipping marker");
        offset += gst_rtp_jpeg_pay_header_size (data, offset);
        break;
      case JPEG_MARKER_SOF:
        if (!gst_rtp_jpeg_pay_read_sof (pay, data, size, &offset, info))
          goto invalid_format;
        sof_found = TRUE;
        break;
      case JPEG_MARKER_DQT:
        GST_LOG ("DQT found");
        offset = gst_rtp_jpeg_pay_read_quant_table (data, size, offset, tables);
        dqt_found = TRUE;
        break;
      case JPEG_MARKER_SOS:
        sos_found = TRUE;
        GST_LOG_OBJECT (pay, "SOS found");
        jpeg_header_size = offset + gst_rtp_jpeg_pay_header_size (data, offset);
        break;
      case JPEG_MARKER_EOI:
        GST_WARNING_OBJECT (pay, "EOI reached before SOS!");
        break;
      case JPEG_MARKER_SOI:
        GST_LOG_OBJECT (pay, "SOI found");
        break;
      case JPEG_MARKER_DRI:
        GST_LOG_OBJECT (pay, "DRI found");
        if (gst_rtp_jpeg_pay_read_dri (pay, data, size, &offset,
                &restart_marker_header))
          dri_found = TRUE;
        break;
      default:
        break;
    }
  }
  if (!dqt_found || !sof_found)
    goto unsupported_jpeg;

  /* by now we should either have negotiated the width/height or the SOF header
   * should have filled us in */
  if (pay->width == 0 || pay->height == 0)
    goto no_dimension;

  GST_LOG_OBJECT (pay, "header size %u", jpeg_header_size);

  size -= jpeg_header_size;
  data += jpeg_header_size;
  offset = 0;

  if (dri_found)
    pay->type += 64;

  /* prepare stuff for the jpeg header */
  jpeg_header.type_spec = 0;
  jpeg_header.type = pay->type;
  jpeg_header.q = pay->quant;
  jpeg_header.width = pay->width;
  jpeg_header.height = pay->height;

  /* collect the quant headers sizes */
  quant_header.mbz = 0;
  quant_header.precision = 0;
  quant_header.length = 0;
  quant_data_size = 0;

  if (pay->quant > 127) {
    /* for the Y and U component, look up the quant table and its size. quant
     * tables for U and V should be the same */
    for (i = 0; i < 2; i++) {
      guint qsize;
      guint qt;

      qt = info[i].qt;
      if (qt >= G_N_ELEMENTS (tables))
        goto invalid_quant;

      qsize = tables[qt].size;
      if (qsize == 0)
        goto invalid_quant;

      quant_header.precision |= (qsize == 64 ? 0 : (1 << i));
      quant_data_size += qsize;
    }
    quant_header.length = g_htons (quant_data_size);
    quant_data_size += sizeof (quant_header);
  }

  GST_LOG_OBJECT (pay, "quant_data size %u", quant_data_size);

  if (pay->buffer_list) {
    list = gst_buffer_list_new ();
    it = gst_buffer_list_iterate (list);
  }

  bytes_left = sizeof (jpeg_header) + quant_data_size + size;

  if (dri_found)
    bytes_left += sizeof (restart_marker_header);

  frame_done = FALSE;
  do {
    GstBuffer *outbuf;
    guint8 *payload;
    guint payload_size = (bytes_left < mtu ? bytes_left : mtu);

    if (pay->buffer_list) {
      guint header_size;

      header_size = sizeof (jpeg_header) + quant_data_size;
      if (dri_found)
        header_size += sizeof (restart_marker_header);

      outbuf = gst_rtp_buffer_new_allocate (header_size, 0, 0);
    } else {
      outbuf = gst_rtp_buffer_new_allocate (payload_size, 0, 0);
    }
    GST_BUFFER_TIMESTAMP (outbuf) = timestamp;

    if (payload_size == bytes_left) {
      GST_LOG_OBJECT (pay, "last packet of frame");
      frame_done = TRUE;
      gst_rtp_buffer_set_marker (outbuf, 1);
    }

    payload = gst_rtp_buffer_get_payload (outbuf);

    /* update offset */
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    jpeg_header.offset = ((offset & 0x0000FF) << 16) |
        ((offset & 0xFF0000) >> 16) | (offset & 0x00FF00);
#else
    jpeg_header.offset = offset;
#endif
    memcpy (payload, &jpeg_header, sizeof (jpeg_header));
    payload += sizeof (jpeg_header);
    payload_size -= sizeof (jpeg_header);

    if (dri_found) {
      memcpy (payload, &restart_marker_header, sizeof (restart_marker_header));
      payload += sizeof (restart_marker_header);
      payload_size -= sizeof (restart_marker_header);
    }

    /* only send quant table with first packet */
    if (G_UNLIKELY (quant_data_size > 0)) {
      memcpy (payload, &quant_header, sizeof (quant_header));
      payload += sizeof (quant_header);

      /* copy the quant tables for luma and chrominance */
      for (i = 0; i < 2; i++) {
        guint qsize;
        guint qt;

        qt = info[i].qt;
        qsize = tables[qt].size;
        memcpy (payload, tables[qt].data, qsize);

        GST_LOG_OBJECT (pay, "component %d using quant %d, size %d", i, qt,
            qsize);

        payload += qsize;
      }
      payload_size -= quant_data_size;
      bytes_left -= quant_data_size;
      quant_data_size = 0;
    }
    GST_LOG_OBJECT (pay, "sending payload size %d", payload_size);

    if (pay->buffer_list) {
      GstBuffer *paybuf;

      /* create a new buf to hold the payload */
      paybuf = gst_buffer_create_sub (buffer, jpeg_header_size + offset,
          payload_size);

      /* create a new group to hold the rtp header and the payload */
      gst_buffer_list_iterator_add_group (it);
      gst_buffer_list_iterator_add (it, outbuf);
      gst_buffer_list_iterator_add (it, paybuf);
    } else {
      memcpy (payload, data, payload_size);
      ret = gst_basertppayload_push (basepayload, outbuf);
      if (ret != GST_FLOW_OK)
        break;
    }

    bytes_left -= payload_size;
    offset += payload_size;
    data += payload_size;
  }
  while (!frame_done);

  if (pay->buffer_list) {
    gst_buffer_list_iterator_free (it);
    /* push the whole buffer list at once */
    ret = gst_basertppayload_push_list (basepayload, list);
  }

  gst_buffer_unref (buffer);

  return ret;

  /* ERRORS */
unsupported_jpeg:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT, ("Unsupported JPEG"), (NULL));
    gst_buffer_unref (buffer);
    return GST_FLOW_NOT_SUPPORTED;
  }
no_dimension:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT, ("No size given"), (NULL));
    gst_buffer_unref (buffer);
    return GST_FLOW_NOT_NEGOTIATED;
  }
invalid_format:
  {
    /* error was posted */
    gst_buffer_unref (buffer);
    return GST_FLOW_ERROR;
  }
invalid_quant:
  {
    GST_ELEMENT_ERROR (pay, STREAM, FORMAT, ("Invalid quant tables"), (NULL));
    gst_buffer_unref (buffer);
    return GST_FLOW_ERROR;
  }
}

static void
gst_rtp_jpeg_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpJPEGPay *rtpjpegpay;

  rtpjpegpay = GST_RTP_JPEG_PAY (object);

  switch (prop_id) {
    case PROP_JPEG_QUALITY:
      rtpjpegpay->quality = g_value_get_int (value);
      GST_DEBUG_OBJECT (object, "quality = %d", rtpjpegpay->quality);
      break;
    case PROP_JPEG_TYPE:
      rtpjpegpay->type = g_value_get_int (value);
      GST_DEBUG_OBJECT (object, "type = %d", rtpjpegpay->type);
      break;
    case PROP_BUFFER_LIST:
      rtpjpegpay->buffer_list = g_value_get_boolean (value);
      GST_DEBUG_OBJECT (object, "buffer_list = %d", rtpjpegpay->buffer_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_jpeg_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpJPEGPay *rtpjpegpay;

  rtpjpegpay = GST_RTP_JPEG_PAY (object);

  switch (prop_id) {
    case PROP_JPEG_QUALITY:
      g_value_set_int (value, rtpjpegpay->quality);
      break;
    case PROP_JPEG_TYPE:
      g_value_set_int (value, rtpjpegpay->type);
      break;
    case PROP_BUFFER_LIST:
      g_value_set_boolean (value, rtpjpegpay->buffer_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_rtp_jpeg_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpjpegpay", GST_RANK_SECONDARY,
      GST_TYPE_RTP_JPEG_PAY);
}
