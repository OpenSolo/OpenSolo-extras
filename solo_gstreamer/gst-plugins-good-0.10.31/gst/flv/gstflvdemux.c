/* GStreamer
 * Copyright (C) <2007> Julien Moutte <julien@moutte.net>
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
 * SECTION:element-flvdemux
 *
 * flvdemux demuxes an FLV file into the different contained streams.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v filesrc location=/path/to/flv ! flvdemux ! audioconvert ! autoaudiosink
 * ]| This pipeline demuxes an FLV file and outputs the contained raw audio streams.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* FIXME 0.11: suppress warnings for deprecated API such as GStaticRecMutex
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include "gstflvdemux.h"
#include "gstflvmux.h"

#include <string.h>
#include <gst/base/gstbytereader.h>
#include <gst/pbutils/descriptions.h>
#include <gst/pbutils/pbutils.h>

static GstStaticPadTemplate flv_sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-flv")
    );

static GstStaticPadTemplate audio_src_template =
    GST_STATIC_PAD_TEMPLATE ("audio",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS
    ("audio/x-adpcm, layout = (string) swf, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/mpeg, mpegversion = (int) 1, layer = (int) 3, channels = (int) { 1, 2 }, rate = (int) { 5512, 8000, 11025, 22050, 44100 }, parsed = (boolean) TRUE; "
        "audio/mpeg, mpegversion = (int) 4, stream-format = (string) raw, framed = (boolean) TRUE; "
        "audio/x-nellymoser, channels = (int) { 1, 2 }, rate = (int) { 5512, 8000, 11025, 16000, 22050, 44100 }; "
        "audio/x-raw-int, endianness = (int) LITTLE_ENDIAN, channels = (int) { 1, 2 }, width = (int) 8, depth = (int) 8, rate = (int) { 5512, 11025, 22050, 44100 }, signed = (boolean) FALSE; "
        "audio/x-raw-int, endianness = (int) LITTLE_ENDIAN, channels = (int) { 1, 2 }, width = (int) 16, depth = (int) 16, rate = (int) { 5512, 11025, 22050, 44100 }, signed = (boolean) TRUE; "
        "audio/x-alaw, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/x-mulaw, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/x-speex, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 };")
    );

static GstStaticPadTemplate video_src_template =
    GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("video/x-flash-video; "
        "video/x-flash-screen; "
        "video/x-vp6-flash; " "video/x-vp6-alpha; "
        "video/x-h264, stream-format=avc;")
    );

GST_DEBUG_CATEGORY_STATIC (flvdemux_debug);
#define GST_CAT_DEFAULT flvdemux_debug

GST_BOILERPLATE (GstFlvDemux, gst_flv_demux, GstElement, GST_TYPE_ELEMENT);

/* 9 bytes of header + 4 bytes of first previous tag size */
#define FLV_HEADER_SIZE 13
/* 1 byte of tag type + 3 bytes of tag data size */
#define FLV_TAG_TYPE_SIZE 4

/* two seconds - consider pts are resynced to another base if this different */
#define RESYNC_THRESHOLD 2000

static gboolean flv_demux_handle_seek_push (GstFlvDemux * demux,
    GstEvent * event);
static gboolean gst_flv_demux_handle_seek_pull (GstFlvDemux * demux,
    GstEvent * event, gboolean seeking);

static gboolean gst_flv_demux_query (GstPad * pad, GstQuery * query);
static gboolean gst_flv_demux_src_event (GstPad * pad, GstEvent * event);


static void
gst_flv_demux_parse_and_add_index_entry (GstFlvDemux * demux, GstClockTime ts,
    guint64 pos, gboolean keyframe)
{
  static GstIndexAssociation associations[2];
  static GstIndexEntry *entry;

  GST_LOG_OBJECT (demux,
      "adding key=%d association %" GST_TIME_FORMAT "-> %" G_GUINT64_FORMAT,
      keyframe, GST_TIME_ARGS (ts), pos);

  /* if upstream is not seekable there is no point in building an index */
  if (!demux->upstream_seekable)
    return;

  /* entry may already have been added before, avoid adding indefinitely */
  entry = gst_index_get_assoc_entry (demux->index, demux->index_id,
      GST_INDEX_LOOKUP_EXACT, GST_ASSOCIATION_FLAG_NONE, GST_FORMAT_BYTES, pos);

  if (entry) {
#ifndef GST_DISABLE_GST_DEBUG
    gint64 time;
    gboolean key;

    gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &time);
    key = ! !(GST_INDEX_ASSOC_FLAGS (entry) & GST_ASSOCIATION_FLAG_KEY_UNIT);
    GST_LOG_OBJECT (demux, "position already mapped to time %" GST_TIME_FORMAT
        ", keyframe %d", GST_TIME_ARGS (time), key);
    /* there is not really a way to delete the existing one */
    if (time != ts || key != ! !keyframe)
      GST_DEBUG_OBJECT (demux, "metadata mismatch");
#endif
    return;
  }

  associations[0].format = GST_FORMAT_TIME;
  associations[0].value = ts;
  associations[1].format = GST_FORMAT_BYTES;
  associations[1].value = pos;

  gst_index_add_associationv (demux->index, demux->index_id,
      (keyframe) ? GST_ASSOCIATION_FLAG_KEY_UNIT :
      GST_ASSOCIATION_FLAG_DELTA_UNIT, 2,
      (const GstIndexAssociation *) &associations);

  if (pos > demux->index_max_pos)
    demux->index_max_pos = pos;
  if (ts > demux->index_max_time)
    demux->index_max_time = ts;
}

static gchar *
FLV_GET_STRING (GstByteReader * reader)
{
  guint16 string_size = 0;
  gchar *string = NULL;
  const guint8 *str = NULL;

  g_return_val_if_fail (reader != NULL, NULL);

  if (G_UNLIKELY (!gst_byte_reader_get_uint16_be (reader, &string_size)))
    return NULL;

  if (G_UNLIKELY (string_size > gst_byte_reader_get_remaining (reader)))
    return NULL;

  string = g_try_malloc0 (string_size + 1);
  if (G_UNLIKELY (!string)) {
    return NULL;
  }

  if (G_UNLIKELY (!gst_byte_reader_get_data (reader, string_size, &str))) {
    g_free (string);
    return NULL;
  }

  memcpy (string, str, string_size);
  if (!g_utf8_validate (string, string_size, NULL)) {
    g_free (string);
    return NULL;
  }

  return string;
}

static const GstQueryType *
gst_flv_demux_query_types (GstPad * pad)
{
  static const GstQueryType query_types[] = {
    GST_QUERY_DURATION,
    GST_QUERY_POSITION,
    GST_QUERY_SEEKING,
    0
  };

  return query_types;
}

static void
gst_flv_demux_check_seekability (GstFlvDemux * demux)
{
  GstQuery *query;
  gint64 start = -1, stop = -1;

  demux->upstream_seekable = FALSE;

  query = gst_query_new_seeking (GST_FORMAT_BYTES);
  if (!gst_pad_peer_query (demux->sinkpad, query)) {
    GST_DEBUG_OBJECT (demux, "seeking query failed");
    gst_query_unref (query);
    return;
  }

  gst_query_parse_seeking (query, NULL, &demux->upstream_seekable,
      &start, &stop);

  gst_query_unref (query);

  /* try harder to query upstream size if we didn't get it the first time */
  if (demux->upstream_seekable && stop == -1) {
    GstFormat fmt = GST_FORMAT_BYTES;

    GST_DEBUG_OBJECT (demux, "doing duration query to fix up unset stop");
    gst_pad_query_peer_duration (demux->sinkpad, &fmt, &stop);
  }

  /* if upstream doesn't know the size, it's likely that it's not seekable in
   * practice even if it technically may be seekable */
  if (demux->upstream_seekable && (start != 0 || stop <= start)) {
    GST_DEBUG_OBJECT (demux, "seekable but unknown start/stop -> disable");
    demux->upstream_seekable = FALSE;
  }

  GST_DEBUG_OBJECT (demux, "upstream seekable: %d", demux->upstream_seekable);
}

static void
parse_flv_demux_parse_date_string (GDate * date, const gchar * s)
{
  g_date_set_parse (date, s);
  if (g_date_valid (date))
    return;

  /* "Fri Oct 15 15:13:16 2004" needs to be parsed */
  {
    static const gchar *months[] = {
      "Jan", "Feb", "Mar", "Apr",
      "May", "Jun", "Jul", "Aug",
      "Sep", "Oct", "Nov", "Dec"
    };
    gchar **tokens = g_strsplit (s, " ", -1);
    guint64 d;
    gchar *endptr;
    gint i;

    if (g_strv_length (tokens) != 5)
      goto out;

    if (strlen (tokens[1]) != 3)
      goto out;
    for (i = 0; i < 12; i++) {
      if (!strcmp (tokens[1], months[i])) {
        break;
      }
    }
    if (i == 12)
      goto out;
    g_date_set_month (date, i + 1);

    d = g_ascii_strtoull (tokens[2], &endptr, 10);
    if (d == 0 && *endptr != '\0')
      goto out;

    g_date_set_day (date, d);

    d = g_ascii_strtoull (tokens[4], &endptr, 10);
    if (d == 0 && *endptr != '\0')
      goto out;

    g_date_set_year (date, d);

  out:
    if (tokens)
      g_strfreev (tokens);
  }
}

static gboolean
gst_flv_demux_parse_metadata_item (GstFlvDemux * demux, GstByteReader * reader,
    gboolean * end_marker)
{
  gchar *tag_name = NULL;
  guint8 tag_type = 0;

  /* Initialize the end_marker flag to FALSE */
  *end_marker = FALSE;

  /* Name of the tag */
  tag_name = FLV_GET_STRING (reader);
  if (G_UNLIKELY (!tag_name)) {
    GST_WARNING_OBJECT (demux, "failed reading tag name");
    return FALSE;
  }

  /* What kind of object is that */
  if (!gst_byte_reader_get_uint8 (reader, &tag_type))
    goto error;

  GST_DEBUG_OBJECT (demux, "tag name %s, tag type %d", tag_name, tag_type);

  switch (tag_type) {
    case 0:                    // Double
    {                           /* Use a union to read the uint64 and then as a double */
      gdouble d = 0;

      if (!gst_byte_reader_get_float64_be (reader, &d))
        goto error;

      GST_DEBUG_OBJECT (demux, "%s => (double) %f", tag_name, d);

      if (!strcmp (tag_name, "duration")) {
        demux->duration = d * GST_SECOND;

        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_DURATION, demux->duration, NULL);
      } else if (!strcmp (tag_name, "AspectRatioX")) {
        demux->par_x = d;
        demux->got_par = TRUE;
      } else if (!strcmp (tag_name, "AspectRatioY")) {
        demux->par_y = d;
        demux->got_par = TRUE;
      } else if (!strcmp (tag_name, "width")) {
        demux->w = d;
      } else if (!strcmp (tag_name, "height")) {
        demux->h = d;
      } else if (!strcmp (tag_name, "framerate")) {
        demux->framerate = d;
      } else {
        GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);
      }

      break;
    }
    case 1:                    // Boolean
    {
      guint8 b = 0;

      if (!gst_byte_reader_get_uint8 (reader, &b))
        goto error;

      GST_DEBUG_OBJECT (demux, "%s => (boolean) %d", tag_name, b);

      GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);

      break;
    }
    case 2:                    // String
    {
      gchar *s = NULL;

      s = FLV_GET_STRING (reader);
      if (s == NULL)
        goto error;

      GST_DEBUG_OBJECT (demux, "%s => (string) %s", tag_name, s);

      if (!strcmp (tag_name, "creationdate")) {
        GDate *date = g_date_new ();

        parse_flv_demux_parse_date_string (date, s);
        if (!g_date_valid (date)) {
          GST_DEBUG_OBJECT (demux, "Failed to parse string as date");
        } else {
          gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
              GST_TAG_DATE, date, NULL);
        }
        g_date_free (date);
      } else if (!strcmp (tag_name, "creator")) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_ARTIST, s, NULL);
      } else if (!strcmp (tag_name, "title")) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_TITLE, s, NULL);
      } else if (!strcmp (tag_name, "metadatacreator")) {
        gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
            GST_TAG_ENCODER, s, NULL);
      } else {
        GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);
      }

      g_free (s);

      break;
    }
    case 3:                    // Object
    {
      gboolean end_of_object_marker = FALSE;

      while (!end_of_object_marker) {
        gboolean ok = gst_flv_demux_parse_metadata_item (demux, reader,
            &end_of_object_marker);

        if (G_UNLIKELY (!ok)) {
          GST_WARNING_OBJECT (demux, "failed reading a tag, skipping");
          goto error;
        }
      }

      break;
    }
    case 8:                    // ECMA array
    {
      guint32 nb_elems = 0;
      gboolean end_of_object_marker = FALSE;

      if (!gst_byte_reader_get_uint32_be (reader, &nb_elems))
        goto error;

      GST_DEBUG_OBJECT (demux, "there are approx. %d elements in the array",
          nb_elems);

      while (!end_of_object_marker) {
        gboolean ok = gst_flv_demux_parse_metadata_item (demux, reader,
            &end_of_object_marker);

        if (G_UNLIKELY (!ok)) {
          GST_WARNING_OBJECT (demux, "failed reading a tag, skipping");
          goto error;
        }
      }

      break;
    }
    case 9:                    // End marker
    {
      GST_DEBUG_OBJECT (demux, "end marker ?");
      if (tag_name[0] == '\0') {

        GST_DEBUG_OBJECT (demux, "end marker detected");

        *end_marker = TRUE;
      }

      break;
    }
    case 10:                   // Array
    {
      guint32 nb_elems = 0;

      if (!gst_byte_reader_get_uint32_be (reader, &nb_elems))
        goto error;

      GST_DEBUG_OBJECT (demux, "array has %d elements", nb_elems);

      if (!strcmp (tag_name, "times")) {
        if (demux->times) {
          g_array_free (demux->times, TRUE);
        }
        demux->times = g_array_new (FALSE, TRUE, sizeof (gdouble));
      } else if (!strcmp (tag_name, "filepositions")) {
        if (demux->filepositions) {
          g_array_free (demux->filepositions, TRUE);
        }
        demux->filepositions = g_array_new (FALSE, TRUE, sizeof (gdouble));
      }

      while (nb_elems--) {
        guint8 elem_type = 0;

        if (!gst_byte_reader_get_uint8 (reader, &elem_type))
          goto error;

        switch (elem_type) {
          case 0:
          {
            gdouble d;

            if (!gst_byte_reader_get_float64_be (reader, &d))
              goto error;

            GST_DEBUG_OBJECT (demux, "element is a double %f", d);

            if (!strcmp (tag_name, "times") && demux->times) {
              g_array_append_val (demux->times, d);
            } else if (!strcmp (tag_name, "filepositions") &&
                demux->filepositions) {
              g_array_append_val (demux->filepositions, d);
            }
            break;
          }
          default:
            GST_WARNING_OBJECT (demux, "unsupported array element type %d",
                elem_type);
        }
      }

      break;
    }
    case 11:                   // Date
    {
      gdouble d = 0;
      gint16 i = 0;

      if (!gst_byte_reader_get_float64_be (reader, &d))
        goto error;

      if (!gst_byte_reader_get_int16_be (reader, &i))
        goto error;

      GST_DEBUG_OBJECT (demux,
          "%s => (date as a double) %f, timezone offset %d", tag_name, d, i);

      GST_INFO_OBJECT (demux, "Tag \'%s\' not handled", tag_name);

      break;
    }
    default:
      GST_WARNING_OBJECT (demux, "unsupported tag type %d", tag_type);
  }

  g_free (tag_name);

  return TRUE;

error:
  g_free (tag_name);

  return FALSE;
}

static GstFlowReturn
gst_flv_demux_parse_tag_script (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstByteReader reader = GST_BYTE_READER_INIT_FROM_BUFFER (buffer);
  guint8 type = 0;

  g_return_val_if_fail (GST_BUFFER_SIZE (buffer) >= 7, GST_FLOW_ERROR);

  gst_byte_reader_skip (&reader, 7);

  GST_LOG_OBJECT (demux, "parsing a script tag");

  if (!gst_byte_reader_get_uint8 (&reader, &type))
    return GST_FLOW_OK;

  /* Must be string */
  if (type == 2) {
    gchar *function_name;
    guint i;

    function_name = FLV_GET_STRING (&reader);

    GST_LOG_OBJECT (demux, "function name is %s", GST_STR_NULL (function_name));

    if (function_name != NULL && strcmp (function_name, "onMetaData") == 0) {
      gboolean end_marker = FALSE;
      GST_DEBUG_OBJECT (demux, "we have a metadata script object");

      if (!gst_byte_reader_get_uint8 (&reader, &type)) {
        g_free (function_name);
        return GST_FLOW_OK;
      }

      switch (type) {
        case 8:
        {
          guint32 nb_elems = 0;

          /* ECMA array */
          if (!gst_byte_reader_get_uint32_be (&reader, &nb_elems)) {
            g_free (function_name);
            return GST_FLOW_OK;
          }

          /* The number of elements is just a hint, some files have
             nb_elements == 0 and actually contain items. */
          GST_DEBUG_OBJECT (demux, "there are approx. %d elements in the array",
              nb_elems);
        }
          /* fallthrough to read data */
        case 3:
        {
          /* Object */
          while (!end_marker) {
            gboolean ok =
                gst_flv_demux_parse_metadata_item (demux, &reader, &end_marker);

            if (G_UNLIKELY (!ok)) {
              GST_WARNING_OBJECT (demux, "failed reading a tag, skipping");
              break;
            }
          }
        }
          break;
        default:
          GST_DEBUG_OBJECT (demux, "Unhandled script data type : %d", type);
          g_free (function_name);
          return GST_FLOW_OK;
      }

      demux->push_tags = TRUE;
    }

    g_free (function_name);

    if (demux->index && demux->times && demux->filepositions) {
      guint num;

      /* If an index was found, insert associations */
      num = MIN (demux->times->len, demux->filepositions->len);
      for (i = 0; i < num; i++) {
        guint64 time, fileposition;

        time = g_array_index (demux->times, gdouble, i) * GST_SECOND;
        fileposition = g_array_index (demux->filepositions, gdouble, i);
        gst_flv_demux_parse_and_add_index_entry (demux, time, fileposition,
            TRUE);
      }
      demux->indexed = TRUE;
    }
  }

  return ret;
}

static gboolean
gst_flv_demux_audio_negotiate (GstFlvDemux * demux, guint32 codec_tag,
    guint32 rate, guint32 channels, guint32 width)
{
  GstCaps *caps = NULL;
  gchar *codec_name = NULL;
  gboolean ret = FALSE;
  guint adjusted_rate = rate;

  switch (codec_tag) {
    case 1:
      caps = gst_caps_new_simple ("audio/x-adpcm", "layout", G_TYPE_STRING,
          "swf", NULL);
      break;
    case 2:
    case 14:
      caps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, 1, "layer", G_TYPE_INT, 3,
          "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
      break;
    case 0:
    case 3:
      /* Assuming little endian for 0 (aka endianness of the
       * system on which the file was created) as most people
       * are probably using little endian machines */
      caps = gst_caps_new_simple ("audio/x-raw-int",
          "endianness", G_TYPE_INT, G_LITTLE_ENDIAN,
          "signed", G_TYPE_BOOLEAN, (width == 8) ? FALSE : TRUE,
          "width", G_TYPE_INT, width, "depth", G_TYPE_INT, width, NULL);
      break;
    case 4:
    case 5:
    case 6:
      caps = gst_caps_new_simple ("audio/x-nellymoser", NULL);
      break;
    case 10:
    {
      /* use codec-data to extract and verify samplerate */
      if (demux->audio_codec_data &&
          GST_BUFFER_SIZE (demux->audio_codec_data) >= 2) {
        gint freq_index;

        freq_index =
            ((GST_READ_UINT16_BE (GST_BUFFER_DATA (demux->audio_codec_data))));
        freq_index = (freq_index & 0x0780) >> 7;
        adjusted_rate =
            gst_codec_utils_aac_get_sample_rate_from_index (freq_index);

        if (adjusted_rate && (rate != adjusted_rate)) {
          GST_LOG_OBJECT (demux, "Ajusting AAC sample rate %d -> %d", rate,
              adjusted_rate);
        } else {
          adjusted_rate = rate;
        }
      }
      caps = gst_caps_new_simple ("audio/mpeg",
          "mpegversion", G_TYPE_INT, 4, "framed", G_TYPE_BOOLEAN, TRUE,
          "stream-format", G_TYPE_STRING, "raw", NULL);
      break;
    }
    case 7:
      caps = gst_caps_new_simple ("audio/x-alaw", NULL);
      break;
    case 8:
      caps = gst_caps_new_simple ("audio/x-mulaw", NULL);
      break;
    case 11:
      caps = gst_caps_new_simple ("audio/x-speex", NULL);
      break;
    default:
      GST_WARNING_OBJECT (demux, "unsupported audio codec tag %u", codec_tag);
  }

  if (G_UNLIKELY (!caps)) {
    GST_WARNING_OBJECT (demux, "failed creating caps for audio pad");
    goto beach;
  }

  gst_caps_set_simple (caps, "rate", G_TYPE_INT, adjusted_rate,
      "channels", G_TYPE_INT, channels, NULL);

  if (demux->audio_codec_data) {
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER,
        demux->audio_codec_data, NULL);
  }

  ret = gst_pad_set_caps (demux->audio_pad, caps);

  if (G_LIKELY (ret)) {
    /* Store the caps we got from tags */
    demux->audio_codec_tag = codec_tag;
    demux->rate = rate;
    demux->channels = channels;
    demux->width = width;

    codec_name = gst_pb_utils_get_codec_description (caps);

    if (codec_name) {
      if (demux->taglist == NULL)
        demux->taglist = gst_tag_list_new ();
      gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
          GST_TAG_AUDIO_CODEC, codec_name, NULL);
      g_free (codec_name);
    }

    GST_DEBUG_OBJECT (demux->audio_pad, "successfully negotiated caps %"
        GST_PTR_FORMAT, caps);
  } else {
    GST_WARNING_OBJECT (demux->audio_pad, "failed negotiating caps %"
        GST_PTR_FORMAT, caps);
  }

  gst_caps_unref (caps);

beach:
  return ret;
}

static void
gst_flv_demux_push_tags (GstFlvDemux * demux)
{
  if (demux->has_audio && !demux->audio_pad) {
    GST_DEBUG_OBJECT (demux,
        "Waiting for audio stream pad to come up before we can push tags");
    return;
  }
  if (demux->has_video && !demux->video_pad) {
    GST_DEBUG_OBJECT (demux,
        "Waiting for video stream pad to come up before we can push tags");
    return;
  }
  if (demux->taglist) {
    GST_DEBUG_OBJECT (demux, "pushing tags out %" GST_PTR_FORMAT,
        demux->taglist);
    gst_element_found_tags (GST_ELEMENT (demux), demux->taglist);
    demux->taglist = gst_tag_list_new ();
    demux->push_tags = FALSE;
  }
}

static void
gst_flv_demux_update_resync (GstFlvDemux * demux, guint32 pts, gboolean discont,
    guint32 * last, GstClockTime * offset)
{
  gint32 dpts = pts - *last;
  if (!discont && ABS (dpts) >= RESYNC_THRESHOLD) {
    /* Theoretically, we should use substract the duration of the last buffer,
       but this demuxer sends no durations on buffers, not sure if it cannot
       know, or just does not care to calculate. */
    *offset -= dpts * GST_MSECOND;
    GST_WARNING_OBJECT (demux,
        "Large pts gap (%" G_GINT32_FORMAT " ms), assuming resync, offset now %"
        GST_TIME_FORMAT "", dpts, GST_TIME_ARGS (*offset));
  }
  *last = pts;
}

static GstFlowReturn
gst_flv_demux_parse_tag_audio (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 pts = 0, codec_tag = 0, rate = 5512, width = 8, channels = 1;
  guint32 codec_data = 0, pts_ext = 0;
  guint8 flags = 0;
  guint8 *data = GST_BUFFER_DATA (buffer);
  GstBuffer *outbuf;

  GST_LOG_OBJECT (demux, "parsing an audio tag");

  if (demux->no_more_pads && !demux->audio_pad) {
    GST_WARNING_OBJECT (demux,
        "Signaled no-more-pads already but had no audio pad -- ignoring");
    goto beach;
  }

  g_return_val_if_fail (GST_BUFFER_SIZE (buffer) == demux->tag_size,
      GST_FLOW_ERROR);

  /* Grab information about audio tag */
  pts = GST_READ_UINT24_BE (data);
  /* read the pts extension to 32 bits integer */
  pts_ext = GST_READ_UINT8 (data + 3);
  /* Combine them */
  pts |= pts_ext << 24;

  GST_LOG_OBJECT (demux, "pts bytes %02X %02X %02X %02X (%d)", data[0], data[1],
      data[2], data[3], pts);

  /* Error out on tags with too small headers */
  if (GST_BUFFER_SIZE (buffer) < 11) {
    GST_ERROR_OBJECT (demux, "Too small tag size (%d)",
        GST_BUFFER_SIZE (buffer));
    return GST_FLOW_ERROR;
  }

  /* Silently skip buffers with no data */
  if (GST_BUFFER_SIZE (buffer) == 11)
    return GST_FLOW_OK;

  /* Skip the stream id and go directly to the flags */
  flags = GST_READ_UINT8 (data + 7);

  /* Channels */
  if (flags & 0x01) {
    channels = 2;
  }
  /* Width */
  if (flags & 0x02) {
    width = 16;
  }
  /* Sampling rate */
  if ((flags & 0x0C) == 0x0C) {
    rate = 44100;
  } else if ((flags & 0x0C) == 0x08) {
    rate = 22050;
  } else if ((flags & 0x0C) == 0x04) {
    rate = 11025;
  }
  /* Codec tag */
  codec_tag = flags >> 4;
  if (codec_tag == 10) {        /* AAC has an extra byte for packet type */
    codec_data = 2;
  } else {
    codec_data = 1;
  }

  /* codec tags with special rates */
  if (codec_tag == 5 || codec_tag == 14)
    rate = 8000;
  else if (codec_tag == 4)
    rate = 16000;

  GST_LOG_OBJECT (demux, "audio tag with %d channels, %dHz sampling rate, "
      "%d bits width, codec tag %u (flags %02X)", channels, rate, width,
      codec_tag, flags);

  /* If we don't have our audio pad created, then create it. */
  if (G_UNLIKELY (!demux->audio_pad)) {

    demux->audio_pad =
        gst_pad_new_from_template (gst_element_class_get_pad_template
        (GST_ELEMENT_GET_CLASS (demux), "audio"), "audio");
    if (G_UNLIKELY (!demux->audio_pad)) {
      GST_WARNING_OBJECT (demux, "failed creating audio pad");
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* Negotiate caps */
    if (!gst_flv_demux_audio_negotiate (demux, codec_tag, rate, channels,
            width)) {
      gst_object_unref (demux->audio_pad);
      demux->audio_pad = NULL;
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    GST_DEBUG_OBJECT (demux, "created audio pad with caps %" GST_PTR_FORMAT,
        GST_PAD_CAPS (demux->audio_pad));

    /* Set functions on the pad */
    gst_pad_set_query_type_function (demux->audio_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_query_types));
    gst_pad_set_query_function (demux->audio_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_query));
    gst_pad_set_event_function (demux->audio_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_src_event));

    gst_pad_use_fixed_caps (demux->audio_pad);

    /* Make it active */
    gst_pad_set_active (demux->audio_pad, TRUE);

    /* We need to set caps before adding */
    gst_element_add_pad (GST_ELEMENT (demux),
        gst_object_ref (demux->audio_pad));

    /* We only emit no more pads when we have audio and video. Indeed we can
     * not trust the FLV header to tell us if there will be only audio or
     * only video and we would just break discovery of some files */
    if (demux->audio_pad && demux->video_pad) {
      GST_DEBUG_OBJECT (demux, "emitting no more pads");
      gst_element_no_more_pads (GST_ELEMENT (demux));
      demux->no_more_pads = TRUE;
      demux->push_tags = TRUE;
    }
  }

  /* Check if caps have changed */
  if (G_UNLIKELY (rate != demux->rate || channels != demux->channels ||
          codec_tag != demux->audio_codec_tag || width != demux->width)) {
    GST_DEBUG_OBJECT (demux, "audio settings have changed, changing caps");

    /* Negotiate caps */
    if (!gst_flv_demux_audio_negotiate (demux, codec_tag, rate, channels,
            width)) {
      ret = GST_FLOW_ERROR;
      goto beach;
    }
  }

  /* Push taglist if present */
  if (G_UNLIKELY (demux->push_tags))
    gst_flv_demux_push_tags (demux);

  /* Check if we have anything to push */
  if (demux->tag_data_size <= codec_data) {
    GST_LOG_OBJECT (demux, "Nothing left in this tag, returning");
    goto beach;
  }

  /* Create buffer from pad */
  outbuf =
      gst_buffer_create_sub (buffer, 7 + codec_data,
      demux->tag_data_size - codec_data);

  if (demux->audio_codec_tag == 10) {
    guint8 aac_packet_type = GST_READ_UINT8 (data + 8);

    switch (aac_packet_type) {
      case 0:
      {
        /* AudioSpecificConfig data */
        GST_LOG_OBJECT (demux, "got an AAC codec data packet");
        if (demux->audio_codec_data) {
          gst_buffer_unref (demux->audio_codec_data);
        }
        demux->audio_codec_data = outbuf;
        /* Use that buffer data in the caps */
        gst_flv_demux_audio_negotiate (demux, codec_tag, rate, channels, width);
        goto beach;
        break;
      }
      case 1:
        /* AAC raw packet */
        GST_LOG_OBJECT (demux, "got a raw AAC audio packet");
        break;
      default:
        GST_WARNING_OBJECT (demux, "invalid AAC packet type %u",
            aac_packet_type);
    }
  }

  /* detect (and deem to be resyncs)  large pts gaps */
  gst_flv_demux_update_resync (demux, pts, demux->audio_need_discont,
      &demux->last_audio_pts, &demux->audio_time_offset);

  /* Fill buffer with data */
  GST_BUFFER_TIMESTAMP (outbuf) = pts * GST_MSECOND + demux->audio_time_offset;
  GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_OFFSET (outbuf) = demux->audio_offset++;
  GST_BUFFER_OFFSET_END (outbuf) = demux->audio_offset;
  gst_buffer_set_caps (outbuf, GST_PAD_CAPS (demux->audio_pad));

  if (demux->duration == GST_CLOCK_TIME_NONE ||
      demux->duration < GST_BUFFER_TIMESTAMP (outbuf))
    demux->duration = GST_BUFFER_TIMESTAMP (outbuf);

  /* Only add audio frames to the index if we have no video,
   * and if the index is not yet complete */
  if (!demux->has_video && demux->index && !demux->indexed) {
    gst_flv_demux_parse_and_add_index_entry (demux,
        GST_BUFFER_TIMESTAMP (outbuf), demux->cur_tag_offset, TRUE);
  }

  if (G_UNLIKELY (demux->audio_need_discont)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    demux->audio_need_discont = FALSE;
  }

  gst_segment_set_last_stop (&demux->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (outbuf));

  /* Do we need a newsegment event ? */
  if (G_UNLIKELY (demux->audio_need_segment)) {
    if (demux->close_seg_event)
      gst_pad_push_event (demux->audio_pad,
          gst_event_ref (demux->close_seg_event));

    if (!demux->new_seg_event) {
      GST_DEBUG_OBJECT (demux, "pushing newsegment from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.last_stop),
          GST_TIME_ARGS (demux->segment.stop));
      demux->new_seg_event =
          gst_event_new_new_segment (FALSE, demux->segment.rate,
          demux->segment.format, demux->segment.last_stop,
          demux->segment.stop, demux->segment.last_stop);
    } else {
      GST_DEBUG_OBJECT (demux, "pushing pre-generated newsegment event");
    }

    gst_pad_push_event (demux->audio_pad, gst_event_ref (demux->new_seg_event));

    demux->audio_need_segment = FALSE;
  }

  GST_LOG_OBJECT (demux, "pushing %d bytes buffer at pts %" GST_TIME_FORMAT
      " with duration %" GST_TIME_FORMAT ", offset %" G_GUINT64_FORMAT,
      GST_BUFFER_SIZE (outbuf), GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)), GST_BUFFER_OFFSET (outbuf));

  if (!GST_CLOCK_TIME_IS_VALID (demux->audio_start)) {
    demux->audio_start = GST_BUFFER_TIMESTAMP (outbuf);
  }
  if (!GST_CLOCK_TIME_IS_VALID (demux->audio_first_ts)) {
    demux->audio_first_ts = GST_BUFFER_TIMESTAMP (outbuf);
  }

  if (G_UNLIKELY (!demux->no_more_pads
          && (GST_CLOCK_DIFF (demux->audio_start,
                  GST_BUFFER_TIMESTAMP (outbuf)) > 6 * GST_SECOND))) {
    GST_DEBUG_OBJECT (demux,
        "Signalling no-more-pads because no video stream was found"
        " after 6 seconds of audio");
    gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
    demux->no_more_pads = TRUE;
    demux->push_tags = TRUE;
  }

  /* Push downstream */
  ret = gst_pad_push (demux->audio_pad, outbuf);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    if (demux->segment.rate < 0.0 && ret == GST_FLOW_UNEXPECTED &&
        demux->segment.last_stop > demux->segment.stop) {
      /* In reverse playback we can get a GST_FLOW_UNEXPECTED when
       * we are at the end of the segment, so we just need to jump
       * back to the previous section. */
      GST_DEBUG_OBJECT (demux, "downstream has reached end of segment");
      demux->audio_done = TRUE;
      ret = GST_FLOW_OK;
    } else {
      GST_WARNING_OBJECT (demux, "failed pushing a %" G_GUINT64_FORMAT
          " bytes audio buffer: %s", demux->tag_data_size,
          gst_flow_get_name (ret));
      if (ret == GST_FLOW_NOT_LINKED) {
        demux->audio_linked = FALSE;
      }
      goto beach;
    }
  }

  demux->audio_linked = TRUE;

beach:
  return ret;
}

static gboolean
gst_flv_demux_video_negotiate (GstFlvDemux * demux, guint32 codec_tag)
{
  gboolean ret = FALSE;
  GstCaps *caps = NULL;
  gchar *codec_name = NULL;

  /* Generate caps for that pad */
  switch (codec_tag) {
    case 2:
      caps = gst_caps_new_simple ("video/x-flash-video", NULL);
      break;
    case 3:
      caps = gst_caps_new_simple ("video/x-flash-screen", NULL);
      break;
    case 4:
      caps = gst_caps_new_simple ("video/x-vp6-flash", NULL);
      break;
    case 5:
      caps = gst_caps_new_simple ("video/x-vp6-alpha", NULL);
      break;
    case 7:
      caps =
          gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
          "avc", NULL);
      break;
    default:
      GST_WARNING_OBJECT (demux, "unsupported video codec tag %u", codec_tag);
  }

  if (G_UNLIKELY (!caps)) {
    GST_WARNING_OBJECT (demux, "failed creating caps for video pad");
    goto beach;
  }

  gst_caps_set_simple (caps, "pixel-aspect-ratio", GST_TYPE_FRACTION,
      demux->par_x, demux->par_y, NULL);

  if (G_LIKELY (demux->w)) {
    gst_caps_set_simple (caps, "width", G_TYPE_INT, demux->w, NULL);
  }

  if (G_LIKELY (demux->h)) {
    gst_caps_set_simple (caps, "height", G_TYPE_INT, demux->h, NULL);
  }

  if (G_LIKELY (demux->framerate)) {
    gint num = 0, den = 0;

    gst_util_double_to_fraction (demux->framerate, &num, &den);
    GST_DEBUG_OBJECT (demux->video_pad,
        "fps to be used on caps %f (as a fraction = %d/%d)", demux->framerate,
        num, den);

    gst_caps_set_simple (caps, "framerate", GST_TYPE_FRACTION, num, den, NULL);
  }

  if (demux->video_codec_data) {
    gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER,
        demux->video_codec_data, NULL);
  }

  ret = gst_pad_set_caps (demux->video_pad, caps);

  if (G_LIKELY (ret)) {
    /* Store the caps we have set */
    demux->video_codec_tag = codec_tag;

    codec_name = gst_pb_utils_get_codec_description (caps);

    if (codec_name) {
      if (demux->taglist == NULL)
        demux->taglist = gst_tag_list_new ();
      gst_tag_list_add (demux->taglist, GST_TAG_MERGE_REPLACE,
          GST_TAG_VIDEO_CODEC, codec_name, NULL);
      g_free (codec_name);
    }

    GST_DEBUG_OBJECT (demux->video_pad, "successfully negotiated caps %"
        GST_PTR_FORMAT, caps);
  } else {
    GST_WARNING_OBJECT (demux->video_pad, "failed negotiating caps %"
        GST_PTR_FORMAT, caps);
  }

  gst_caps_unref (caps);

beach:
  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_tag_video (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 pts = 0, codec_data = 1, pts_ext = 0;
  gboolean keyframe = FALSE;
  guint8 flags = 0, codec_tag = 0;
  guint8 *data = GST_BUFFER_DATA (buffer);
  GstBuffer *outbuf;

  g_return_val_if_fail (GST_BUFFER_SIZE (buffer) == demux->tag_size,
      GST_FLOW_ERROR);

  GST_LOG_OBJECT (demux, "parsing a video tag");


  if (demux->no_more_pads && !demux->video_pad) {
    GST_WARNING_OBJECT (demux,
        "Signaled no-more-pads already but had no audio pad -- ignoring");
    goto beach;
  }

  /* Grab information about video tag */
  pts = GST_READ_UINT24_BE (data);
  /* read the pts extension to 32 bits integer */
  pts_ext = GST_READ_UINT8 (data + 3);
  /* Combine them */
  pts |= pts_ext << 24;

  GST_LOG_OBJECT (demux, "pts bytes %02X %02X %02X %02X (%d)", data[0], data[1],
      data[2], data[3], pts);

  if (GST_BUFFER_SIZE (buffer) < 12) {
    GST_ERROR_OBJECT (demux, "Too small tag size");
    return GST_FLOW_ERROR;
  }

  /* Skip the stream id and go directly to the flags */
  flags = GST_READ_UINT8 (data + 7);

  /* Keyframe */
  if ((flags >> 4) == 1) {
    keyframe = TRUE;
  }
  /* Codec tag */
  codec_tag = flags & 0x0F;
  if (codec_tag == 4 || codec_tag == 5) {
    codec_data = 2;
  } else if (codec_tag == 7) {
    gint32 cts;

    codec_data = 5;

    cts = GST_READ_UINT24_BE (data + 9);
    cts = (cts + 0xff800000) ^ 0xff800000;

    GST_LOG_OBJECT (demux, "got cts %d", cts);

    /* avoid negative overflow */
    if (cts >= 0 || pts >= -cts)
      pts += cts;
  }

  GST_LOG_OBJECT (demux, "video tag with codec tag %u, keyframe (%d) "
      "(flags %02X)", codec_tag, keyframe, flags);

  /* If we don't have our video pad created, then create it. */
  if (G_UNLIKELY (!demux->video_pad)) {
    demux->video_pad =
        gst_pad_new_from_template (gst_element_class_get_pad_template
        (GST_ELEMENT_GET_CLASS (demux), "video"), "video");
    if (G_UNLIKELY (!demux->video_pad)) {
      GST_WARNING_OBJECT (demux, "failed creating video pad");
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    if (!gst_flv_demux_video_negotiate (demux, codec_tag)) {
      gst_object_unref (demux->video_pad);
      demux->video_pad = NULL;
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* When we ve set pixel-aspect-ratio we use that boolean to detect a
     * metadata tag that would come later and trigger a caps change */
    demux->got_par = FALSE;

    GST_DEBUG_OBJECT (demux, "created video pad with caps %" GST_PTR_FORMAT,
        GST_PAD_CAPS (demux->video_pad));

    /* Set functions on the pad */
    gst_pad_set_query_type_function (demux->video_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_query_types));
    gst_pad_set_query_function (demux->video_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_query));
    gst_pad_set_event_function (demux->video_pad,
        GST_DEBUG_FUNCPTR (gst_flv_demux_src_event));

    gst_pad_use_fixed_caps (demux->video_pad);

    /* Make it active */
    gst_pad_set_active (demux->video_pad, TRUE);

    /* We need to set caps before adding */
    gst_element_add_pad (GST_ELEMENT (demux),
        gst_object_ref (demux->video_pad));

    /* We only emit no more pads when we have audio and video. Indeed we can
     * not trust the FLV header to tell us if there will be only audio or
     * only video and we would just break discovery of some files */
    if (demux->audio_pad && demux->video_pad) {
      GST_DEBUG_OBJECT (demux, "emitting no more pads");
      gst_element_no_more_pads (GST_ELEMENT (demux));
      demux->no_more_pads = TRUE;
      demux->push_tags = TRUE;
    }
  }

  /* Check if caps have changed */
  if (G_UNLIKELY (codec_tag != demux->video_codec_tag || demux->got_par)) {

    GST_DEBUG_OBJECT (demux, "video settings have changed, changing caps");

    if (!gst_flv_demux_video_negotiate (demux, codec_tag)) {
      ret = GST_FLOW_ERROR;
      goto beach;
    }

    /* When we ve set pixel-aspect-ratio we use that boolean to detect a
     * metadata tag that would come later and trigger a caps change */
    demux->got_par = FALSE;
  }

  /* Push taglist if present */
  if (G_UNLIKELY (demux->push_tags))
    gst_flv_demux_push_tags (demux);

  /* Check if we have anything to push */
  if (demux->tag_data_size <= codec_data) {
    GST_LOG_OBJECT (demux, "Nothing left in this tag, returning");
    goto beach;
  }

  /* Create buffer from pad */
  outbuf =
      gst_buffer_create_sub (buffer, 7 + codec_data,
      demux->tag_data_size - codec_data);

  if (demux->video_codec_tag == 7) {
    guint8 avc_packet_type = GST_READ_UINT8 (data + 8);

    switch (avc_packet_type) {
      case 0:
      {
        /* AVCDecoderConfigurationRecord data */
        GST_LOG_OBJECT (demux, "got an H.264 codec data packet");
        if (demux->video_codec_data) {
          gst_buffer_unref (demux->video_codec_data);
        }
        demux->video_codec_data = outbuf;
        /* Use that buffer data in the caps */
        gst_flv_demux_video_negotiate (demux, codec_tag);
        goto beach;
        break;
      }
      case 1:
        /* H.264 NALU packet */
        GST_LOG_OBJECT (demux, "got a H.264 NALU video packet");
        break;
      default:
        GST_WARNING_OBJECT (demux, "invalid video packet type %u",
            avc_packet_type);
    }
  }

  /* detect (and deem to be resyncs)  large pts gaps */
  gst_flv_demux_update_resync (demux, pts, demux->video_need_discont,
      &demux->last_video_pts, &demux->video_time_offset);

  /* Fill buffer with data */
  GST_BUFFER_TIMESTAMP (outbuf) = pts * GST_MSECOND + demux->video_time_offset;
  GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_OFFSET (outbuf) = demux->video_offset++;
  GST_BUFFER_OFFSET_END (outbuf) = demux->video_offset;
  gst_buffer_set_caps (outbuf, GST_PAD_CAPS (demux->video_pad));

  if (demux->duration == GST_CLOCK_TIME_NONE ||
      demux->duration < GST_BUFFER_TIMESTAMP (outbuf))
    demux->duration = GST_BUFFER_TIMESTAMP (outbuf);

  if (!keyframe)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  if (!demux->indexed && demux->index) {
    gst_flv_demux_parse_and_add_index_entry (demux,
        GST_BUFFER_TIMESTAMP (outbuf), demux->cur_tag_offset, keyframe);
  }

  if (G_UNLIKELY (demux->video_need_discont)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    demux->video_need_discont = FALSE;
  }

  gst_segment_set_last_stop (&demux->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (outbuf));

  /* Do we need a newsegment event ? */
  if (G_UNLIKELY (demux->video_need_segment)) {
    if (demux->close_seg_event)
      gst_pad_push_event (demux->video_pad,
          gst_event_ref (demux->close_seg_event));

    if (!demux->new_seg_event) {
      GST_DEBUG_OBJECT (demux, "pushing newsegment from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.last_stop),
          GST_TIME_ARGS (demux->segment.stop));
      demux->new_seg_event =
          gst_event_new_new_segment (FALSE, demux->segment.rate,
          demux->segment.format, demux->segment.last_stop,
          demux->segment.stop, demux->segment.last_stop);
    } else {
      GST_DEBUG_OBJECT (demux, "pushing pre-generated newsegment event");
    }

    gst_pad_push_event (demux->video_pad, gst_event_ref (demux->new_seg_event));

    demux->video_need_segment = FALSE;
  }

  GST_LOG_OBJECT (demux, "pushing %d bytes buffer at pts %" GST_TIME_FORMAT
      " with duration %" GST_TIME_FORMAT ", offset %" G_GUINT64_FORMAT
      ", keyframe (%d)", GST_BUFFER_SIZE (outbuf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)), GST_BUFFER_OFFSET (outbuf),
      keyframe);

  if (!GST_CLOCK_TIME_IS_VALID (demux->video_start)) {
    demux->video_start = GST_BUFFER_TIMESTAMP (outbuf);
  }
  if (!GST_CLOCK_TIME_IS_VALID (demux->audio_first_ts)) {
    demux->video_first_ts = GST_BUFFER_TIMESTAMP (outbuf);
  }

  if (G_UNLIKELY (!demux->no_more_pads
          && (GST_CLOCK_DIFF (demux->video_start,
                  GST_BUFFER_TIMESTAMP (outbuf)) > 6 * GST_SECOND))) {
    GST_DEBUG_OBJECT (demux,
        "Signalling no-more-pads because no audio stream was found"
        " after 6 seconds of video");
    gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
    demux->no_more_pads = TRUE;
    demux->push_tags = TRUE;
  }

  /* Push downstream */
  ret = gst_pad_push (demux->video_pad, outbuf);

  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    if (demux->segment.rate < 0.0 && ret == GST_FLOW_UNEXPECTED &&
        demux->segment.last_stop > demux->segment.stop) {
      /* In reverse playback we can get a GST_FLOW_UNEXPECTED when
       * we are at the end of the segment, so we just need to jump
       * back to the previous section. */
      GST_DEBUG_OBJECT (demux, "downstream has reached end of segment");
      demux->video_done = TRUE;
      ret = GST_FLOW_OK;
    } else {
      GST_WARNING_OBJECT (demux, "failed pushing a %" G_GUINT64_FORMAT
          " bytes video buffer: %s", demux->tag_data_size,
          gst_flow_get_name (ret));
      if (ret == GST_FLOW_NOT_LINKED) {
        demux->video_linked = FALSE;
      }
      goto beach;
    }
  }

  demux->video_linked = TRUE;

beach:
  return ret;
}

static GstClockTime
gst_flv_demux_parse_tag_timestamp (GstFlvDemux * demux, gboolean index,
    GstBuffer * buffer, size_t * tag_size)
{
  guint32 pts = 0, pts_ext = 0;
  guint32 tag_data_size;
  guint8 type;
  gboolean keyframe = TRUE;
  GstClockTime ret;
  guint8 *data = GST_BUFFER_DATA (buffer);

  g_return_val_if_fail (GST_BUFFER_SIZE (buffer) >= 12, GST_CLOCK_TIME_NONE);

  type = data[0];

  if (type != 9 && type != 8 && type != 18) {
    GST_WARNING_OBJECT (demux, "Unsupported tag type %u", data[0]);
    return GST_CLOCK_TIME_NONE;
  }

  if (type == 9)
    demux->has_video = TRUE;
  else if (type == 8)
    demux->has_audio = TRUE;

  tag_data_size = GST_READ_UINT24_BE (data + 1);

  if (GST_BUFFER_SIZE (buffer) >= tag_data_size + 11 + 4) {
    if (GST_READ_UINT32_BE (data + tag_data_size + 11) != tag_data_size + 11) {
      GST_WARNING_OBJECT (demux, "Invalid tag size");
      return GST_CLOCK_TIME_NONE;
    }
  }

  if (tag_size)
    *tag_size = tag_data_size + 11 + 4;

  data += 4;

  GST_LOG_OBJECT (demux, "pts bytes %02X %02X %02X %02X", data[0], data[1],
      data[2], data[3]);

  /* Grab timestamp of tag tag */
  pts = GST_READ_UINT24_BE (data);
  /* read the pts extension to 32 bits integer */
  pts_ext = GST_READ_UINT8 (data + 3);
  /* Combine them */
  pts |= pts_ext << 24;

  if (type == 9) {
    data += 7;

    keyframe = ((data[0] >> 4) == 1);
  }

  ret = pts * GST_MSECOND;
  GST_LOG_OBJECT (demux, "pts: %" GST_TIME_FORMAT, GST_TIME_ARGS (ret));

  if (index && demux->index && !demux->indexed && (type == 9 || (type == 8
              && !demux->has_video))) {
    gst_flv_demux_parse_and_add_index_entry (demux, ret, demux->offset,
        keyframe);
  }

  if (demux->duration == GST_CLOCK_TIME_NONE || demux->duration < ret)
    demux->duration = ret;

  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_tag_type (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint8 tag_type = 0;
  guint8 *data = GST_BUFFER_DATA (buffer);

  g_return_val_if_fail (GST_BUFFER_SIZE (buffer) >= 4, GST_FLOW_ERROR);

  tag_type = data[0];

  switch (tag_type) {
    case 9:
      demux->state = FLV_STATE_TAG_VIDEO;
      demux->has_video = TRUE;
      break;
    case 8:
      demux->state = FLV_STATE_TAG_AUDIO;
      demux->has_audio = TRUE;
      break;
    case 18:
      demux->state = FLV_STATE_TAG_SCRIPT;
      break;
    default:
      GST_WARNING_OBJECT (demux, "unsupported tag type %u", tag_type);
  }

  /* Tag size is 1 byte of type + 3 bytes of size + 7 bytes + tag data size +
   * 4 bytes of previous tag size */
  demux->tag_data_size = GST_READ_UINT24_BE (data + 1);
  demux->tag_size = demux->tag_data_size + 11;

  GST_LOG_OBJECT (demux, "tag data size is %" G_GUINT64_FORMAT,
      demux->tag_data_size);

  return ret;
}

static GstFlowReturn
gst_flv_demux_parse_header (GstFlvDemux * demux, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint8 *data = GST_BUFFER_DATA (buffer);

  g_return_val_if_fail (GST_BUFFER_SIZE (buffer) >= 9, GST_FLOW_ERROR);

  /* Check for the FLV tag */
  if (data[0] == 'F' && data[1] == 'L' && data[2] == 'V') {
    GST_DEBUG_OBJECT (demux, "FLV header detected");
  } else {
    if (G_UNLIKELY (demux->strict)) {
      GST_WARNING_OBJECT (demux, "invalid header tag detected");
      ret = GST_FLOW_UNEXPECTED;
      goto beach;
    }
  }

  /* Jump over the 4 first bytes */
  data += 4;

  /* Now look at audio/video flags */
  {
    guint8 flags = data[0];

    demux->has_video = demux->has_audio = FALSE;

    if (flags & 1) {
      GST_DEBUG_OBJECT (demux, "there is a video stream");
      demux->has_video = TRUE;
    }
    if (flags & 4) {
      GST_DEBUG_OBJECT (demux, "there is an audio stream");
      demux->has_audio = TRUE;
    }
  }

  /* do a one-time seekability check */
  gst_flv_demux_check_seekability (demux);

  /* We don't care about the rest */
  demux->need_header = FALSE;

beach:
  return ret;
}


static void
gst_flv_demux_flush (GstFlvDemux * demux, gboolean discont)
{
  GST_DEBUG_OBJECT (demux, "flushing queued data in the FLV demuxer");

  gst_adapter_clear (demux->adapter);

  demux->audio_need_discont = TRUE;
  demux->video_need_discont = TRUE;

  demux->flushing = FALSE;

  /* Only in push mode and if we're not during a seek */
  if (!demux->random_access && demux->state != FLV_STATE_SEEK) {
    /* After a flush we expect a tag_type */
    demux->state = FLV_STATE_TAG_TYPE;
    /* We reset the offset and will get one from first push */
    demux->offset = 0;
  }
}

static void
gst_flv_demux_cleanup (GstFlvDemux * demux)
{
  GST_DEBUG_OBJECT (demux, "cleaning up FLV demuxer");

  demux->state = FLV_STATE_HEADER;

  demux->flushing = FALSE;
  demux->need_header = TRUE;
  demux->audio_need_segment = TRUE;
  demux->video_need_segment = TRUE;
  demux->audio_need_discont = TRUE;
  demux->video_need_discont = TRUE;

  /* By default we consider them as linked */
  demux->audio_linked = TRUE;
  demux->video_linked = TRUE;

  demux->has_audio = FALSE;
  demux->has_video = FALSE;
  demux->push_tags = FALSE;
  demux->got_par = FALSE;

  demux->indexed = FALSE;
  demux->upstream_seekable = FALSE;
  demux->file_size = 0;

  demux->index_max_pos = 0;
  demux->index_max_time = 0;

  demux->audio_start = demux->video_start = GST_CLOCK_TIME_NONE;
  demux->last_audio_pts = demux->last_video_pts = 0;
  demux->audio_time_offset = demux->video_time_offset = 0;

  demux->no_more_pads = FALSE;

  gst_segment_init (&demux->segment, GST_FORMAT_TIME);

  demux->w = demux->h = 0;
  demux->framerate = 0.0;
  demux->par_x = demux->par_y = 1;
  demux->video_offset = 0;
  demux->audio_offset = 0;
  demux->offset = demux->cur_tag_offset = 0;
  demux->tag_size = demux->tag_data_size = 0;
  demux->duration = GST_CLOCK_TIME_NONE;

  if (demux->new_seg_event) {
    gst_event_unref (demux->new_seg_event);
    demux->new_seg_event = NULL;
  }

  if (demux->close_seg_event) {
    gst_event_unref (demux->close_seg_event);
    demux->close_seg_event = NULL;
  }

  gst_adapter_clear (demux->adapter);

  if (demux->audio_codec_data) {
    gst_buffer_unref (demux->audio_codec_data);
    demux->audio_codec_data = NULL;
  }

  if (demux->video_codec_data) {
    gst_buffer_unref (demux->video_codec_data);
    demux->video_codec_data = NULL;
  }

  if (demux->audio_pad) {
    gst_element_remove_pad (GST_ELEMENT (demux), demux->audio_pad);
    gst_object_unref (demux->audio_pad);
    demux->audio_pad = NULL;
  }

  if (demux->video_pad) {
    gst_element_remove_pad (GST_ELEMENT (demux), demux->video_pad);
    gst_object_unref (demux->video_pad);
    demux->video_pad = NULL;
  }

  if (demux->times) {
    g_array_free (demux->times, TRUE);
    demux->times = NULL;
  }

  if (demux->filepositions) {
    g_array_free (demux->filepositions, TRUE);
    demux->filepositions = NULL;
  }
}

/*
 * Create and push a flushing seek event upstream
 */
static gboolean
flv_demux_seek_to_offset (GstFlvDemux * demux, guint64 offset)
{
  GstEvent *event;
  gboolean res = 0;

  GST_DEBUG_OBJECT (demux, "Seeking to %" G_GUINT64_FORMAT, offset);

  event =
      gst_event_new_seek (1.0, GST_FORMAT_BYTES,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, offset,
      GST_SEEK_TYPE_NONE, -1);

  res = gst_pad_push_event (demux->sinkpad, event);

  if (res)
    demux->offset = offset;
  return res;
}

static GstFlowReturn
gst_flv_demux_chain (GstPad * pad, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstFlvDemux *demux = NULL;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (pad));

  GST_LOG_OBJECT (demux, "received buffer of %d bytes at offset %"
      G_GUINT64_FORMAT, GST_BUFFER_SIZE (buffer), GST_BUFFER_OFFSET (buffer));

  if (G_UNLIKELY (GST_BUFFER_OFFSET (buffer) == 0)) {
    GST_DEBUG_OBJECT (demux, "beginning of file, expect header");
    demux->state = FLV_STATE_HEADER;
    demux->offset = 0;
  }

  if (G_UNLIKELY (demux->offset == 0 && GST_BUFFER_OFFSET (buffer) != 0)) {
    GST_DEBUG_OBJECT (demux, "offset was zero, synchronizing with buffer's");
    demux->offset = GST_BUFFER_OFFSET (buffer);
  }

  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT)) {
    GST_DEBUG_OBJECT (demux, "Discontinuity");
    gst_adapter_clear (demux->adapter);
  }

  gst_adapter_push (demux->adapter, buffer);

  if (demux->seeking) {
    demux->state = FLV_STATE_SEEK;
    GST_OBJECT_LOCK (demux);
    demux->seeking = FALSE;
    GST_OBJECT_UNLOCK (demux);
  }

parse:
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    if (ret == GST_FLOW_NOT_LINKED && (demux->audio_linked
            || demux->video_linked)) {
      ret = GST_FLOW_OK;
    } else {
      GST_DEBUG_OBJECT (demux, "got flow return %s", gst_flow_get_name (ret));
      goto beach;
    }
  }

  if (G_UNLIKELY (demux->flushing)) {
    GST_DEBUG_OBJECT (demux, "we are now flushing, exiting parser loop");
    ret = GST_FLOW_WRONG_STATE;
    goto beach;
  }

  switch (demux->state) {
    case FLV_STATE_HEADER:
    {
      if (gst_adapter_available (demux->adapter) >= FLV_HEADER_SIZE) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, FLV_HEADER_SIZE);

        ret = gst_flv_demux_parse_header (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += FLV_HEADER_SIZE;

        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_TYPE:
    {
      if (gst_adapter_available (demux->adapter) >= FLV_TAG_TYPE_SIZE) {
        GstBuffer *buffer;

        /* Remember the tag offset in bytes */
        demux->cur_tag_offset = demux->offset;

        buffer = gst_adapter_take_buffer (demux->adapter, FLV_TAG_TYPE_SIZE);

        ret = gst_flv_demux_parse_tag_type (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += FLV_TAG_TYPE_SIZE;

        /* last tag is not an index => no index/don't know where the index is
         * seek back to the beginning */
        if (demux->seek_event && demux->state != FLV_STATE_TAG_SCRIPT)
          goto no_index;

        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_VIDEO:
    {
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, demux->tag_size);

        ret = gst_flv_demux_parse_tag_video (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += demux->tag_size;

        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_AUDIO:
    {
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, demux->tag_size);

        ret = gst_flv_demux_parse_tag_audio (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += demux->tag_size;

        demux->state = FLV_STATE_TAG_TYPE;
        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_TAG_SCRIPT:
    {
      if (gst_adapter_available (demux->adapter) >= demux->tag_size) {
        GstBuffer *buffer;

        buffer = gst_adapter_take_buffer (demux->adapter, demux->tag_size);

        ret = gst_flv_demux_parse_tag_script (demux, buffer);

        gst_buffer_unref (buffer);
        demux->offset += demux->tag_size;

        demux->state = FLV_STATE_TAG_TYPE;

        /* if there's a seek event we're here for the index so if we don't have it
         * we seek back to the beginning */
        if (demux->seek_event) {
          if (demux->indexed)
            demux->state = FLV_STATE_SEEK;
          else
            goto no_index;
        }

        goto parse;
      } else {
        goto beach;
      }
    }
    case FLV_STATE_SEEK:
    {
      GstEvent *event;

      ret = GST_FLOW_OK;

      if (!demux->indexed) {
        if (demux->offset == demux->file_size - sizeof (guint32)) {
          GstBuffer *buffer =
              gst_adapter_take_buffer (demux->adapter, sizeof (guint32));
          GstByteReader *reader = gst_byte_reader_new_from_buffer (buffer);
          guint64 seek_offset;

          if (!gst_adapter_available (demux->adapter) >= sizeof (guint32)) {
            /* error */
          }

          seek_offset =
              demux->file_size - sizeof (guint32) -
              gst_byte_reader_peek_uint32_be_unchecked (reader);
          gst_byte_reader_free (reader);
          gst_buffer_unref (buffer);

          GST_INFO_OBJECT (demux,
              "Seeking to beginning of last tag at %" G_GUINT64_FORMAT,
              seek_offset);
          demux->state = FLV_STATE_TAG_TYPE;
          flv_demux_seek_to_offset (demux, seek_offset);
          goto beach;
        } else
          goto no_index;
      }

      GST_OBJECT_LOCK (demux);
      event = demux->seek_event;
      demux->seek_event = NULL;
      GST_OBJECT_UNLOCK (demux);

      /* calculate and perform seek */
      if (!flv_demux_handle_seek_push (demux, event))
        goto seek_failed;

      gst_event_unref (event);
      demux->state = FLV_STATE_TAG_TYPE;
      goto beach;
    }
    default:
      GST_DEBUG_OBJECT (demux, "unexpected demuxer state");
  }

beach:
  if (G_UNLIKELY (ret == GST_FLOW_NOT_LINKED)) {
    /* If either audio or video is linked we return GST_FLOW_OK */
    if (demux->audio_linked || demux->video_linked) {
      ret = GST_FLOW_OK;
    }
  }

  gst_object_unref (demux);

  return ret;

/* ERRORS */
no_index:
  {
    GST_OBJECT_LOCK (demux);
    demux->seeking = FALSE;
    gst_event_unref (demux->seek_event);
    demux->seek_event = NULL;
    GST_OBJECT_UNLOCK (demux);
    GST_WARNING_OBJECT (demux,
        "failed to find an index, seeking back to beginning");
    flv_demux_seek_to_offset (demux, 0);
    return GST_FLOW_OK;
  }
seek_failed:
  {
    GST_ELEMENT_ERROR (demux, STREAM, DEMUX, (NULL), ("seek failed"));
    return GST_FLOW_ERROR;
  }

}

static GstFlowReturn
gst_flv_demux_pull_range (GstFlvDemux * demux, GstPad * pad, guint64 offset,
    guint size, GstBuffer ** buffer)
{
  GstFlowReturn ret;

  ret = gst_pad_pull_range (pad, offset, size, buffer);
  if (G_UNLIKELY (ret != GST_FLOW_OK)) {
    GST_WARNING_OBJECT (demux,
        "failed when pulling %d bytes from offset %" G_GUINT64_FORMAT ": %s",
        size, offset, gst_flow_get_name (ret));
    *buffer = NULL;
    return ret;
  }

  if (G_UNLIKELY (*buffer && GST_BUFFER_SIZE (*buffer) != size)) {
    GST_WARNING_OBJECT (demux,
        "partial pull got %d when expecting %d from offset %" G_GUINT64_FORMAT,
        GST_BUFFER_SIZE (*buffer), size, offset);
    gst_buffer_unref (*buffer);
    ret = GST_FLOW_UNEXPECTED;
    *buffer = NULL;
    return ret;
  }

  return ret;
}

static GstFlowReturn
gst_flv_demux_pull_tag (GstPad * pad, GstFlvDemux * demux)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  /* Store tag offset */
  demux->cur_tag_offset = demux->offset;

  /* Get the first 4 bytes to identify tag type and size */
  if (G_UNLIKELY ((ret = gst_flv_demux_pull_range (demux, pad, demux->offset,
                  FLV_TAG_TYPE_SIZE, &buffer)) != GST_FLOW_OK))
    goto beach;

  /* Identify tag type */
  ret = gst_flv_demux_parse_tag_type (demux, buffer);

  gst_buffer_unref (buffer);

  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto beach;

  /* Jump over tag type + size */
  demux->offset += FLV_TAG_TYPE_SIZE;

  /* Pull the whole tag */
  if (G_UNLIKELY ((ret = gst_flv_demux_pull_range (demux, pad, demux->offset,
                  demux->tag_size, &buffer)) != GST_FLOW_OK))
    goto beach;

  switch (demux->state) {
    case FLV_STATE_TAG_VIDEO:
      ret = gst_flv_demux_parse_tag_video (demux, buffer);
      break;
    case FLV_STATE_TAG_AUDIO:
      ret = gst_flv_demux_parse_tag_audio (demux, buffer);
      break;
    case FLV_STATE_TAG_SCRIPT:
      ret = gst_flv_demux_parse_tag_script (demux, buffer);
      break;
    default:
      GST_WARNING_OBJECT (demux, "unexpected state %d", demux->state);
  }

  gst_buffer_unref (buffer);

  /* Jump over that part we've just parsed */
  demux->offset += demux->tag_size;

  /* Make sure we reinitialize the tag size */
  demux->tag_size = 0;

  /* Ready for the next tag */
  demux->state = FLV_STATE_TAG_TYPE;

  if (G_UNLIKELY (ret == GST_FLOW_NOT_LINKED)) {
    /* If either audio or video is linked we return GST_FLOW_OK */
    if (demux->audio_linked || demux->video_linked) {
      ret = GST_FLOW_OK;
    } else {
      GST_WARNING_OBJECT (demux, "parsing this tag returned not-linked and "
          "neither video nor audio are linked");
    }
  }

beach:
  return ret;
}

static GstFlowReturn
gst_flv_demux_pull_header (GstPad * pad, GstFlvDemux * demux)
{
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  /* Get the first 9 bytes */
  if (G_UNLIKELY ((ret = gst_flv_demux_pull_range (demux, pad, demux->offset,
                  FLV_HEADER_SIZE, &buffer)) != GST_FLOW_OK))
    goto beach;

  ret = gst_flv_demux_parse_header (demux, buffer);

  gst_buffer_unref (buffer);

  /* Jump over the header now */
  demux->offset += FLV_HEADER_SIZE;
  demux->state = FLV_STATE_TAG_TYPE;

beach:
  return ret;
}

static void
gst_flv_demux_move_to_offset (GstFlvDemux * demux, gint64 offset,
    gboolean reset)
{
  demux->offset = offset;

  /* Tell all the stream we moved to a different position (discont) */
  demux->audio_need_discont = TRUE;
  demux->video_need_discont = TRUE;

  /* next section setup */
  demux->from_offset = -1;
  demux->audio_done = demux->video_done = FALSE;
  demux->audio_first_ts = demux->video_first_ts = GST_CLOCK_TIME_NONE;

  if (reset) {
    demux->from_offset = -1;
    demux->to_offset = G_MAXINT64;
  }

  /* If we seeked at the beginning of the file parse the header again */
  if (G_UNLIKELY (!demux->offset)) {
    demux->state = FLV_STATE_HEADER;
  } else {                      /* or parse a tag */
    demux->state = FLV_STATE_TAG_TYPE;
  }
}

static GstFlowReturn
gst_flv_demux_seek_to_prev_keyframe (GstFlvDemux * demux)
{
  GstFlowReturn ret = GST_FLOW_UNEXPECTED;
  GstIndexEntry *entry = NULL;

  GST_DEBUG_OBJECT (demux,
      "terminated section started at offset %" G_GINT64_FORMAT,
      demux->from_offset);

  /* we are done if we got all audio and video */
  if ((!GST_CLOCK_TIME_IS_VALID (demux->audio_first_ts) ||
          demux->audio_first_ts < demux->segment.start) &&
      (!GST_CLOCK_TIME_IS_VALID (demux->video_first_ts) ||
          demux->video_first_ts < demux->segment.start))
    goto done;

  if (demux->from_offset <= 0)
    goto done;

  GST_DEBUG_OBJECT (demux, "locating previous position");

  /* locate index entry before previous start position */
  if (demux->index)
    entry = gst_index_get_assoc_entry (demux->index, demux->index_id,
        GST_INDEX_LOOKUP_BEFORE, GST_ASSOCIATION_FLAG_KEY_UNIT,
        GST_FORMAT_BYTES, demux->from_offset - 1);

  if (entry) {
    gint64 bytes, time;

    gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &bytes);
    gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &time);

    GST_DEBUG_OBJECT (demux, "found index entry for %" G_GINT64_FORMAT
        " at %" GST_TIME_FORMAT ", seeking to %" G_GINT64_FORMAT,
        demux->offset - 1, GST_TIME_ARGS (time), bytes);

    /* setup for next section */
    demux->to_offset = demux->from_offset;
    gst_flv_demux_move_to_offset (demux, bytes, FALSE);
    ret = GST_FLOW_OK;
  }

done:
  return ret;
}

static gboolean
gst_flv_demux_push_src_event (GstFlvDemux * demux, GstEvent * event)
{
  gboolean ret = TRUE;

  if (demux->audio_pad)
    ret |= gst_pad_push_event (demux->audio_pad, gst_event_ref (event));

  if (demux->video_pad)
    ret |= gst_pad_push_event (demux->video_pad, gst_event_ref (event));

  gst_event_unref (event);

  return ret;
}

static GstFlowReturn
gst_flv_demux_create_index (GstFlvDemux * demux, gint64 pos, GstClockTime ts)
{
  gint64 size;
  GstFormat fmt = GST_FORMAT_BYTES;
  size_t tag_size;
  guint64 old_offset;
  GstBuffer *buffer;
  GstClockTime tag_time;
  GstFlowReturn ret = GST_FLOW_OK;

  if (G_UNLIKELY (!gst_pad_query_peer_duration (demux->sinkpad, &fmt, &size) ||
          fmt != GST_FORMAT_BYTES))
    return GST_FLOW_OK;

  GST_DEBUG_OBJECT (demux, "building index at %" G_GINT64_FORMAT
      " looking for time %" GST_TIME_FORMAT, pos, GST_TIME_ARGS (ts));

  old_offset = demux->offset;
  demux->offset = pos;

  while ((ret = gst_flv_demux_pull_range (demux, demux->sinkpad, demux->offset,
              12, &buffer)) == GST_FLOW_OK) {
    tag_time =
        gst_flv_demux_parse_tag_timestamp (demux, TRUE, buffer, &tag_size);

    gst_buffer_unref (buffer);

    if (G_UNLIKELY (tag_time == GST_CLOCK_TIME_NONE || tag_time > ts))
      goto exit;

    demux->offset += tag_size;
  }

  if (ret == GST_FLOW_UNEXPECTED) {
    /* file ran out, so mark we have complete index */
    demux->indexed = TRUE;
    ret = GST_FLOW_OK;
  }

exit:
  demux->offset = old_offset;

  return ret;
}

static gint64
gst_flv_demux_get_metadata (GstFlvDemux * demux)
{
  gint64 ret = 0, offset;
  GstFormat fmt = GST_FORMAT_BYTES;
  size_t tag_size, size;
  GstBuffer *buffer = NULL;

  if (G_UNLIKELY (!gst_pad_query_peer_duration (demux->sinkpad, &fmt, &offset)
          || fmt != GST_FORMAT_BYTES))
    goto exit;

  ret = offset;
  GST_DEBUG_OBJECT (demux, "upstream size: %" G_GINT64_FORMAT, offset);
  if (G_UNLIKELY (offset < 4))
    goto exit;

  offset -= 4;
  if (GST_FLOW_OK != gst_flv_demux_pull_range (demux, demux->sinkpad, offset,
          4, &buffer))
    goto exit;

  tag_size = GST_READ_UINT32_BE (GST_BUFFER_DATA (buffer));
  GST_DEBUG_OBJECT (demux, "last tag size: %" G_GSIZE_FORMAT, tag_size);
  gst_buffer_unref (buffer);
  buffer = NULL;

  offset -= tag_size;
  if (GST_FLOW_OK != gst_flv_demux_pull_range (demux, demux->sinkpad, offset,
          12, &buffer))
    goto exit;

  /* a consistency check */
  size = GST_READ_UINT24_BE (GST_BUFFER_DATA (buffer) + 1);
  if (size != tag_size - 11) {
    GST_DEBUG_OBJECT (demux,
        "tag size %" G_GSIZE_FORMAT ", expected %" G_GSIZE_FORMAT
        ", corrupt or truncated file", size, tag_size - 11);
    goto exit;
  }

  /* try to update duration with timestamp in any case */
  gst_flv_demux_parse_tag_timestamp (demux, FALSE, buffer, &size);

  /* maybe get some more metadata */
  if (GST_BUFFER_DATA (buffer)[0] == 18) {
    gst_buffer_unref (buffer);
    buffer = NULL;
    GST_DEBUG_OBJECT (demux, "script tag, pulling it to parse");
    offset += 4;
    if (GST_FLOW_OK == gst_flv_demux_pull_range (demux, demux->sinkpad, offset,
            tag_size, &buffer))
      gst_flv_demux_parse_tag_script (demux, buffer);
  }

exit:
  if (buffer)
    gst_buffer_unref (buffer);

  return ret;
}

static void
gst_flv_demux_loop (GstPad * pad)
{
  GstFlvDemux *demux = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (pad));

  /* pull in data */
  switch (demux->state) {
    case FLV_STATE_TAG_TYPE:
      if (demux->from_offset == -1)
        demux->from_offset = demux->offset;
      ret = gst_flv_demux_pull_tag (pad, demux);
      /* if we have seen real data, we probably passed a possible metadata
       * header located at start.  So if we do not yet have an index,
       * try to pick up metadata (index, duration) at the end */
      if (G_UNLIKELY (!demux->file_size && !demux->indexed &&
              (demux->has_video || demux->has_audio)))
        demux->file_size = gst_flv_demux_get_metadata (demux);
      break;
    case FLV_STATE_DONE:
      ret = GST_FLOW_UNEXPECTED;
      break;
    case FLV_STATE_SEEK:
      /* seek issued with insufficient index;
       * scan for index in task thread from current maximum offset to
       * desired time and then perform seek */
      /* TODO maybe some buffering message or so to indicate scan progress */
      ret = gst_flv_demux_create_index (demux, demux->index_max_pos,
          demux->seek_time);
      if (ret != GST_FLOW_OK)
        goto pause;
      /* position and state arranged by seek,
       * also unrefs event */
      gst_flv_demux_handle_seek_pull (demux, demux->seek_event, FALSE);
      demux->seek_event = NULL;
      break;
    default:
      ret = gst_flv_demux_pull_header (pad, demux);
      /* index scans start after header */
      demux->index_max_pos = demux->offset;
      break;
  }

  if (demux->segment.rate < 0.0) {
    /* check end of section */
    if ((gint64) demux->offset >= demux->to_offset ||
        demux->segment.last_stop >= demux->segment.stop + 2 * GST_SECOND ||
        (demux->audio_done && demux->video_done))
      ret = gst_flv_demux_seek_to_prev_keyframe (demux);
  } else {
    /* check EOS condition */
    if ((demux->segment.stop != -1) &&
        (demux->segment.last_stop >= demux->segment.stop)) {
      ret = GST_FLOW_UNEXPECTED;
    }
  }

  /* pause if something went wrong or at end */
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto pause;

  gst_object_unref (demux);

  return;

pause:
  {
    const gchar *reason = gst_flow_get_name (ret);

    GST_LOG_OBJECT (demux, "pausing task, reason %s", reason);
    gst_pad_pause_task (pad);

    if (ret == GST_FLOW_UNEXPECTED) {
      /* perform EOS logic */
      if (!demux->no_more_pads) {
        gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
        demux->no_more_pads = TRUE;
      }

      if (demux->segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gint64 stop;

        /* for segment playback we need to post when (in stream time)
         * we stopped, this is either stop (when set) or the duration. */
        if ((stop = demux->segment.stop) == -1)
          stop = demux->segment.duration;

        if (demux->segment.rate >= 0) {
          GST_LOG_OBJECT (demux, "Sending segment done, at end of segment");
          gst_element_post_message (GST_ELEMENT_CAST (demux),
              gst_message_new_segment_done (GST_OBJECT_CAST (demux),
                  GST_FORMAT_TIME, stop));
        } else {                /* Reverse playback */
          GST_LOG_OBJECT (demux, "Sending segment done, at beginning of "
              "segment");
          gst_element_post_message (GST_ELEMENT_CAST (demux),
              gst_message_new_segment_done (GST_OBJECT_CAST (demux),
                  GST_FORMAT_TIME, demux->segment.start));
        }
      } else {
        /* normal playback, send EOS to all linked pads */
        if (!demux->no_more_pads) {
          gst_element_no_more_pads (GST_ELEMENT (demux));
          demux->no_more_pads = TRUE;
        }

        GST_LOG_OBJECT (demux, "Sending EOS, at end of stream");
        if (!gst_flv_demux_push_src_event (demux, gst_event_new_eos ()))
          GST_WARNING_OBJECT (demux, "failed pushing EOS on streams");
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_UNEXPECTED) {
      GST_ELEMENT_ERROR (demux, STREAM, FAILED,
          ("Internal data stream error."),
          ("stream stopped, reason %s", reason));
      gst_flv_demux_push_src_event (demux, gst_event_new_eos ());
    }
    gst_object_unref (demux);
    return;
  }
}

static guint64
gst_flv_demux_find_offset (GstFlvDemux * demux, GstSegment * segment)
{
  gint64 bytes = 0;
  gint64 time = 0;
  GstIndexEntry *entry;

  g_return_val_if_fail (segment != NULL, 0);

  time = segment->last_stop;

  if (demux->index) {
    /* Let's check if we have an index entry for that seek time */
    entry = gst_index_get_assoc_entry (demux->index, demux->index_id,
        GST_INDEX_LOOKUP_BEFORE, GST_ASSOCIATION_FLAG_KEY_UNIT,
        GST_FORMAT_TIME, time);

    if (entry) {
      gst_index_entry_assoc_map (entry, GST_FORMAT_BYTES, &bytes);
      gst_index_entry_assoc_map (entry, GST_FORMAT_TIME, &time);

      GST_DEBUG_OBJECT (demux, "found index entry for %" GST_TIME_FORMAT
          " at %" GST_TIME_FORMAT ", seeking to %" G_GINT64_FORMAT,
          GST_TIME_ARGS (segment->last_stop), GST_TIME_ARGS (time), bytes);

      /* Key frame seeking */
      if (segment->flags & GST_SEEK_FLAG_KEY_UNIT) {
        /* Adjust the segment so that the keyframe fits in */
        if (time < segment->start) {
          segment->start = segment->time = time;
        }
        segment->last_stop = time;
      }
    } else {
      GST_DEBUG_OBJECT (demux, "no index entry found for %" GST_TIME_FORMAT,
          GST_TIME_ARGS (segment->start));
    }
  }

  return bytes;
}

static gboolean
flv_demux_handle_seek_push (GstFlvDemux * demux, GstEvent * event)
{
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType start_type, stop_type;
  gint64 start, stop;
  gdouble rate;
  gboolean update, flush, ret;
  GstSegment seeksegment;

  gst_event_parse_seek (event, &rate, &format, &flags,
      &start_type, &start, &stop_type, &stop);

  if (format != GST_FORMAT_TIME)
    goto wrong_format;

  flush = ! !(flags & GST_SEEK_FLAG_FLUSH);
  /* FIXME : the keyframe flag is never used ! */

  /* Work on a copy until we are sure the seek succeeded. */
  memcpy (&seeksegment, &demux->segment, sizeof (GstSegment));

  GST_DEBUG_OBJECT (demux, "segment before configure %" GST_SEGMENT_FORMAT,
      &demux->segment);

  /* Apply the seek to our segment */
  gst_segment_set_seek (&seeksegment, rate, format, flags,
      start_type, start, stop_type, stop, &update);

  GST_DEBUG_OBJECT (demux, "segment configured %" GST_SEGMENT_FORMAT,
      &seeksegment);

  if (flush || seeksegment.last_stop != demux->segment.last_stop) {
    /* Do the actual seeking */
    guint64 offset = gst_flv_demux_find_offset (demux, &seeksegment);

    GST_DEBUG_OBJECT (demux, "generating an upstream seek at position %"
        G_GUINT64_FORMAT, offset);
    ret = gst_pad_push_event (demux->sinkpad,
        gst_event_new_seek (seeksegment.rate, GST_FORMAT_BYTES,
            seeksegment.flags | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET,
            offset, GST_SEEK_TYPE_NONE, 0));
    if (G_UNLIKELY (!ret)) {
      GST_WARNING_OBJECT (demux, "upstream seek failed");
    }

    /* Tell all the stream we moved to a different position (discont) */
    demux->audio_need_discont = TRUE;
    demux->video_need_discont = TRUE;
  } else {
    ret = TRUE;
  }

  if (ret) {
    /* Ok seek succeeded, take the newly configured segment */
    memcpy (&demux->segment, &seeksegment, sizeof (GstSegment));

    /* Tell all the stream a new segment is needed */
    demux->audio_need_segment = TRUE;
    demux->video_need_segment = TRUE;
    /* Clean any potential newsegment event kept for the streams. The first
     * stream needing a new segment will create a new one. */
    if (G_UNLIKELY (demux->new_seg_event)) {
      gst_event_unref (demux->new_seg_event);
      demux->new_seg_event = NULL;
    }
    gst_event_unref (event);
  } else {
    ret = gst_pad_push_event (demux->sinkpad, event);
  }

  return ret;

/* ERRORS */
wrong_format:
  {
    GST_WARNING_OBJECT (demux, "we only support seeking in TIME format");
    gst_event_unref (event);
    return FALSE;
  }
}

static gboolean
gst_flv_demux_handle_seek_push (GstFlvDemux * demux, GstEvent * event)
{
  GstFormat format;

  gst_event_parse_seek (event, NULL, &format, NULL, NULL, NULL, NULL, NULL);

  if (format != GST_FORMAT_TIME) {
    GST_WARNING_OBJECT (demux, "we only support seeking in TIME format");
    gst_event_unref (event);
    return FALSE;
  }

  /* First try upstream */
  if (gst_pad_push_event (demux->sinkpad, gst_event_ref (event))) {
    GST_DEBUG_OBJECT (demux, "Upstream successfully seeked");
    gst_event_unref (event);
    return TRUE;
  }

  if (!demux->indexed) {
    guint64 seek_offset = 0;
    gboolean building_index;
    GstFormat fmt;

    GST_OBJECT_LOCK (demux);
    /* handle the seek in the chain function */
    demux->seeking = TRUE;
    demux->state = FLV_STATE_SEEK;

    /* copy the event */
    if (demux->seek_event)
      gst_event_unref (demux->seek_event);
    demux->seek_event = gst_event_ref (event);

    /* set the building_index flag so that only one thread can setup the
     * structures for index seeking. */
    building_index = demux->building_index;
    if (!building_index) {
      demux->building_index = TRUE;
      fmt = GST_FORMAT_BYTES;
      if (!demux->file_size
          && !gst_pad_query_peer_duration (demux->sinkpad, &fmt,
              &demux->file_size)) {
        GST_WARNING_OBJECT (demux,
            "Cannot obtain file size - %" G_GINT64_FORMAT ", format %u",
            demux->file_size, fmt);
        GST_OBJECT_UNLOCK (demux);
        return FALSE;
      }

      /* we hope the last tag is a scriptdataobject containing an index
       * the size of the last tag is given in the last guint32 bits
       * then we seek to the beginning of the tag, parse it and hopefully obtain an index */
      seek_offset = demux->file_size - sizeof (guint32);
      GST_DEBUG_OBJECT (demux,
          "File size obtained, seeking to %" G_GUINT64_FORMAT, seek_offset);
    }
    GST_OBJECT_UNLOCK (demux);

    if (!building_index) {
      GST_INFO_OBJECT (demux, "Seeking to last 4 bytes at %" G_GUINT64_FORMAT,
          seek_offset);
      return flv_demux_seek_to_offset (demux, seek_offset);
    }

    /* FIXME: we have to always return true so that we don't block the seek
     * thread.
     * Note: maybe it is OK to return true if we're still building the index */
    return TRUE;
  }

  return flv_demux_handle_seek_push (demux, event);
}

static gboolean
gst_flv_demux_handle_seek_pull (GstFlvDemux * demux, GstEvent * event,
    gboolean seeking)
{
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType start_type, stop_type;
  gint64 start, stop;
  gdouble rate;
  gboolean update, flush, ret = FALSE;
  GstSegment seeksegment;

  gst_event_parse_seek (event, &rate, &format, &flags,
      &start_type, &start, &stop_type, &stop);

  if (format != GST_FORMAT_TIME)
    goto wrong_format;

  /* mark seeking thread entering flushing/pausing */
  GST_OBJECT_LOCK (demux);
  if (seeking)
    demux->seeking = seeking;
  GST_OBJECT_UNLOCK (demux);

  flush = ! !(flags & GST_SEEK_FLAG_FLUSH);
  /* FIXME : the keyframe flag is never used */

  if (flush) {
    /* Flush start up and downstream to make sure data flow and loops are
       idle */
    gst_flv_demux_push_src_event (demux, gst_event_new_flush_start ());
    gst_pad_push_event (demux->sinkpad, gst_event_new_flush_start ());
  } else {
    /* Pause the pulling task */
    gst_pad_pause_task (demux->sinkpad);
  }

  /* Take the stream lock */
  GST_PAD_STREAM_LOCK (demux->sinkpad);

  if (flush) {
    /* Stop flushing upstream we need to pull */
    gst_pad_push_event (demux->sinkpad, gst_event_new_flush_stop ());
  }

  /* Work on a copy until we are sure the seek succeeded. */
  memcpy (&seeksegment, &demux->segment, sizeof (GstSegment));

  GST_DEBUG_OBJECT (demux, "segment before configure %" GST_SEGMENT_FORMAT,
      &demux->segment);

  /* Apply the seek to our segment */
  gst_segment_set_seek (&seeksegment, rate, format, flags,
      start_type, start, stop_type, stop, &update);

  GST_DEBUG_OBJECT (demux, "segment configured %" GST_SEGMENT_FORMAT,
      &seeksegment);

  if (flush || seeksegment.last_stop != demux->segment.last_stop) {
    /* Do the actual seeking */
    /* index is reliable if it is complete or we do not go to far ahead */
    if (seeking && !demux->indexed &&
        seeksegment.last_stop > demux->index_max_time + 10 * GST_SECOND) {
      GST_DEBUG_OBJECT (demux, "delaying seek to post-scan; "
          " index only up to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->index_max_time));
      /* stop flushing for now */
      if (flush)
        gst_flv_demux_push_src_event (demux, gst_event_new_flush_stop ());
      /* delegate scanning and index building to task thread to avoid
       * occupying main (UI) loop */
      if (demux->seek_event)
        gst_event_unref (demux->seek_event);
      demux->seek_event = gst_event_ref (event);
      demux->seek_time = seeksegment.last_stop;
      demux->state = FLV_STATE_SEEK;
      /* do not know about succes yet, but we did care and handled it */
      ret = TRUE;
      goto exit;
    }
    /* now index should be as reliable as it can be for current purpose */
    gst_flv_demux_move_to_offset (demux,
        gst_flv_demux_find_offset (demux, &seeksegment), TRUE);
    ret = TRUE;
  } else {
    ret = TRUE;
  }

  if (G_UNLIKELY (demux->close_seg_event)) {
    gst_event_unref (demux->close_seg_event);
    demux->close_seg_event = NULL;
  }

  if (flush) {
    /* Stop flushing, the sinks are at time 0 now */
    gst_flv_demux_push_src_event (demux, gst_event_new_flush_stop ());
  } else {
    GST_DEBUG_OBJECT (demux, "closing running segment %" GST_SEGMENT_FORMAT,
        &demux->segment);

    /* Close the current segment for a linear playback */
    if (demux->segment.rate >= 0) {
      /* for forward playback, we played from start to last_stop */
      demux->close_seg_event = gst_event_new_new_segment (TRUE,
          demux->segment.rate, demux->segment.format,
          demux->segment.start, demux->segment.last_stop, demux->segment.time);
    } else {
      gint64 stop;

      if ((stop = demux->segment.stop) == -1)
        stop = demux->segment.duration;

      /* for reverse playback, we played from stop to last_stop. */
      demux->close_seg_event = gst_event_new_new_segment (TRUE,
          demux->segment.rate, demux->segment.format,
          demux->segment.last_stop, stop, demux->segment.last_stop);
    }
  }

  if (ret) {
    /* Ok seek succeeded, take the newly configured segment */
    memcpy (&demux->segment, &seeksegment, sizeof (GstSegment));

    /* Notify about the start of a new segment */
    if (demux->segment.flags & GST_SEEK_FLAG_SEGMENT) {
      gst_element_post_message (GST_ELEMENT (demux),
          gst_message_new_segment_start (GST_OBJECT (demux),
              demux->segment.format, demux->segment.last_stop));
    }

    /* Tell all the stream a new segment is needed */
    demux->audio_need_segment = TRUE;
    demux->video_need_segment = TRUE;
    /* Clean any potential newsegment event kept for the streams. The first
     * stream needing a new segment will create a new one. */
    if (G_UNLIKELY (demux->new_seg_event)) {
      gst_event_unref (demux->new_seg_event);
      demux->new_seg_event = NULL;
    }
    if (demux->segment.rate < 0.0) {
      /* we can't generate a segment by locking on
       * to the first timestamp we see */
      GST_DEBUG_OBJECT (demux, "preparing newsegment from %"
          GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.start),
          GST_TIME_ARGS (demux->segment.stop));
      demux->new_seg_event =
          gst_event_new_new_segment (FALSE, demux->segment.rate,
          demux->segment.format, demux->segment.start,
          demux->segment.stop, demux->segment.start);
    }
  }

exit:
  GST_OBJECT_LOCK (demux);
  seeking = demux->seeking && !seeking;
  demux->seeking = FALSE;
  GST_OBJECT_UNLOCK (demux);

  /* if we detect an external seek having started (and possibly already having
   * flushed), do not restart task to give it a chance.
   * Otherwise external one's flushing will take care to pause task */
  if (seeking) {
    gst_pad_pause_task (demux->sinkpad);
  } else {
    gst_pad_start_task (demux->sinkpad,
        (GstTaskFunction) gst_flv_demux_loop, demux->sinkpad);
  }

  GST_PAD_STREAM_UNLOCK (demux->sinkpad);

  gst_event_unref (event);
  return ret;

  /* ERRORS */
wrong_format:
  {
    GST_WARNING_OBJECT (demux, "we only support seeking in TIME format");
    gst_event_unref (event);
    return ret;
  }
}

/* If we can pull that's prefered */
static gboolean
gst_flv_demux_sink_activate (GstPad * sinkpad)
{
  if (gst_pad_check_pull_range (sinkpad)) {
    return gst_pad_activate_pull (sinkpad, TRUE);
  } else {
    return gst_pad_activate_push (sinkpad, TRUE);
  }
}

/* This function gets called when we activate ourselves in push mode.
 * We cannot seek (ourselves) in the stream */
static gboolean
gst_flv_demux_sink_activate_push (GstPad * sinkpad, gboolean active)
{
  GstFlvDemux *demux;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (sinkpad));

  demux->random_access = FALSE;

  gst_object_unref (demux);

  return TRUE;
}

/* this function gets called when we activate ourselves in pull mode.
 * We can perform  random access to the resource and we start a task
 * to start reading */
static gboolean
gst_flv_demux_sink_activate_pull (GstPad * sinkpad, gboolean active)
{
  GstFlvDemux *demux;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (sinkpad));

  if (active) {
    demux->random_access = TRUE;
    gst_object_unref (demux);
    return gst_pad_start_task (sinkpad, (GstTaskFunction) gst_flv_demux_loop,
        sinkpad);
  } else {
    demux->random_access = FALSE;
    gst_object_unref (demux);
    return gst_pad_stop_task (sinkpad);
  }
}

static gboolean
gst_flv_demux_sink_event (GstPad * pad, GstEvent * event)
{
  GstFlvDemux *demux;
  gboolean ret = FALSE;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (demux, "handling event %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      GST_DEBUG_OBJECT (demux, "trying to force chain function to exit");
      demux->flushing = TRUE;
      ret = gst_flv_demux_push_src_event (demux, event);
      break;
    case GST_EVENT_FLUSH_STOP:
      GST_DEBUG_OBJECT (demux, "flushing FLV demuxer");
      gst_flv_demux_flush (demux, TRUE);
      ret = gst_flv_demux_push_src_event (demux, event);
      break;
    case GST_EVENT_EOS:
      GST_DEBUG_OBJECT (demux, "received EOS");
      if (demux->index) {
        GST_DEBUG_OBJECT (demux, "committing index");
        gst_index_commit (demux->index, demux->index_id);
      }
      if (!demux->no_more_pads) {
        gst_element_no_more_pads (GST_ELEMENT (demux));
        demux->no_more_pads = TRUE;
      }

      if (!gst_flv_demux_push_src_event (demux, event))
        GST_WARNING_OBJECT (demux, "failed pushing EOS on streams");
      ret = TRUE;
      break;
    case GST_EVENT_NEWSEGMENT:
    {
      GstFormat format;
      gdouble rate;
      gint64 start, stop, time;
      gboolean update;

      GST_DEBUG_OBJECT (demux, "received new segment");

      gst_event_parse_new_segment (event, &update, &rate, &format, &start,
          &stop, &time);

      if (format == GST_FORMAT_TIME) {
        /* time segment, this is perfect, copy over the values. */
        gst_segment_set_newsegment (&demux->segment, update, rate, format,
            start, stop, time);

        GST_DEBUG_OBJECT (demux, "NEWSEGMENT: %" GST_SEGMENT_FORMAT,
            &demux->segment);

        /* and forward */
        ret = gst_flv_demux_push_src_event (demux, event);
      } else {
        /* non-time format */
        demux->audio_need_segment = TRUE;
        demux->video_need_segment = TRUE;
        ret = TRUE;
        gst_event_unref (event);
      }
      break;
    }
    default:
      ret = gst_flv_demux_push_src_event (demux, event);
      break;
  }

  gst_object_unref (demux);

  return ret;
}

static gboolean
gst_flv_demux_src_event (GstPad * pad, GstEvent * event)
{
  GstFlvDemux *demux;
  gboolean ret = FALSE;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (demux, "handling event %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      if (demux->random_access) {
        ret = gst_flv_demux_handle_seek_pull (demux, event, TRUE);
      } else {
        ret = gst_flv_demux_handle_seek_push (demux, event);
      }
      break;
    default:
      ret = gst_pad_push_event (demux->sinkpad, event);
      break;
  }

  gst_object_unref (demux);

  return ret;
}

static gboolean
gst_flv_demux_query (GstPad * pad, GstQuery * query)
{
  gboolean res = TRUE;
  GstFlvDemux *demux;

  demux = GST_FLV_DEMUX (gst_pad_get_parent (pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      /* duration is time only */
      if (format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (demux, "duration query only supported for time "
            "format");
        res = FALSE;
        goto beach;
      }

      GST_DEBUG_OBJECT (pad, "duration query, replying %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->duration));

      gst_query_set_duration (query, GST_FORMAT_TIME, demux->duration);

      break;
    }
    case GST_QUERY_POSITION:
    {
      GstFormat format;

      gst_query_parse_position (query, &format, NULL);

      /* position is time only */
      if (format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (demux, "position query only supported for time "
            "format");
        res = FALSE;
        goto beach;
      }

      GST_DEBUG_OBJECT (pad, "position query, replying %" GST_TIME_FORMAT,
          GST_TIME_ARGS (demux->segment.last_stop));

      gst_query_set_position (query, GST_FORMAT_TIME, demux->segment.last_stop);

      break;
    }

    case GST_QUERY_SEEKING:{
      GstFormat fmt;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);

      /* First ask upstream */
      if (fmt == GST_FORMAT_TIME && gst_pad_peer_query (demux->sinkpad, query)) {
        gboolean seekable;

        gst_query_parse_seeking (query, NULL, &seekable, NULL, NULL);
        if (seekable) {
          res = TRUE;
          break;
        }
      }
      res = TRUE;
      if (fmt != GST_FORMAT_TIME || !demux->index) {
        gst_query_set_seeking (query, fmt, FALSE, -1, -1);
      } else if (demux->random_access) {
        gst_query_set_seeking (query, GST_FORMAT_TIME, TRUE, 0,
            demux->duration);
      } else {
        GstQuery *peerquery = gst_query_new_seeking (GST_FORMAT_BYTES);
        gboolean seekable = gst_pad_peer_query (demux->sinkpad, peerquery);

        if (seekable)
          gst_query_parse_seeking (peerquery, NULL, &seekable, NULL, NULL);
        gst_query_unref (peerquery);

        if (seekable)
          gst_query_set_seeking (query, GST_FORMAT_TIME, seekable, 0,
              demux->duration);
        else
          gst_query_set_seeking (query, GST_FORMAT_TIME, FALSE, -1, -1);
      }
      break;
    }
    case GST_QUERY_LATENCY:
    default:
    {
      GstPad *peer;

      if ((peer = gst_pad_get_peer (demux->sinkpad))) {
        /* query latency on peer pad */
        res = gst_pad_query (peer, query);
        gst_object_unref (peer);
      } else {
        /* no peer, we don't know */
        res = FALSE;
      }
      break;
    }
  }

beach:
  gst_object_unref (demux);

  return res;
}

static GstStateChangeReturn
gst_flv_demux_change_state (GstElement * element, GstStateChange transition)
{
  GstFlvDemux *demux;
  GstStateChangeReturn ret;

  demux = GST_FLV_DEMUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* If this is our own index destroy it as the
       * old entries might be wrong for the new stream */
      if (demux->own_index) {
        gst_object_unref (demux->index);
        demux->index = NULL;
        demux->own_index = FALSE;
      }

      /* If no index was created, generate one */
      if (G_UNLIKELY (!demux->index)) {
        GST_DEBUG_OBJECT (demux, "no index provided creating our own");

        demux->index = gst_index_factory_make ("memindex");

        gst_index_get_writer_id (demux->index, GST_OBJECT (demux),
            &demux->index_id);
        demux->own_index = TRUE;
      }
      gst_flv_demux_cleanup (demux);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_flv_demux_cleanup (demux);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_flv_demux_set_index (GstElement * element, GstIndex * index)
{
  GstFlvDemux *demux = GST_FLV_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->index)
    gst_object_unref (demux->index);
  if (index) {
    demux->index = gst_object_ref (index);
    demux->own_index = FALSE;
  } else
    demux->index = NULL;

  GST_OBJECT_UNLOCK (demux);
  /* object lock might be taken again */
  if (index)
    gst_index_get_writer_id (index, GST_OBJECT (element), &demux->index_id);
  GST_DEBUG_OBJECT (demux, "Set index %" GST_PTR_FORMAT, demux->index);

}

static GstIndex *
gst_flv_demux_get_index (GstElement * element)
{
  GstIndex *result = NULL;

  GstFlvDemux *demux = GST_FLV_DEMUX (element);

  GST_OBJECT_LOCK (demux);
  if (demux->index)
    result = gst_object_ref (demux->index);
  GST_OBJECT_UNLOCK (demux);

  return result;
}

static void
gst_flv_demux_dispose (GObject * object)
{
  GstFlvDemux *demux = GST_FLV_DEMUX (object);

  GST_DEBUG_OBJECT (demux, "disposing FLV demuxer");

  if (demux->adapter) {
    gst_adapter_clear (demux->adapter);
    g_object_unref (demux->adapter);
    demux->adapter = NULL;
  }

  if (demux->taglist) {
    gst_tag_list_free (demux->taglist);
    demux->taglist = NULL;
  }

  if (demux->new_seg_event) {
    gst_event_unref (demux->new_seg_event);
    demux->new_seg_event = NULL;
  }

  if (demux->close_seg_event) {
    gst_event_unref (demux->close_seg_event);
    demux->close_seg_event = NULL;
  }

  if (demux->audio_codec_data) {
    gst_buffer_unref (demux->audio_codec_data);
    demux->audio_codec_data = NULL;
  }

  if (demux->video_codec_data) {
    gst_buffer_unref (demux->video_codec_data);
    demux->video_codec_data = NULL;
  }

  if (demux->audio_pad) {
    gst_object_unref (demux->audio_pad);
    demux->audio_pad = NULL;
  }

  if (demux->video_pad) {
    gst_object_unref (demux->video_pad);
    demux->video_pad = NULL;
  }

  if (demux->index) {
    gst_object_unref (demux->index);
    demux->index = NULL;
  }

  if (demux->times) {
    g_array_free (demux->times, TRUE);
    demux->times = NULL;
  }

  if (demux->filepositions) {
    g_array_free (demux->filepositions, TRUE);
    demux->filepositions = NULL;
  }

  GST_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
gst_flv_demux_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class,
      &flv_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &audio_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &video_src_template);
  gst_element_class_set_details_simple (element_class, "FLV Demuxer",
      "Codec/Demuxer",
      "Demux FLV feeds into digital streams",
      "Julien Moutte <julien@moutte.net>");
}

static void
gst_flv_demux_class_init (GstFlvDemuxClass * klass)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gst_flv_demux_dispose;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_flv_demux_change_state);
  gstelement_class->set_index = GST_DEBUG_FUNCPTR (gst_flv_demux_set_index);
  gstelement_class->get_index = GST_DEBUG_FUNCPTR (gst_flv_demux_get_index);
}

static void
gst_flv_demux_init (GstFlvDemux * demux, GstFlvDemuxClass * g_class)
{
  demux->sinkpad =
      gst_pad_new_from_static_template (&flv_sink_template, "sink");

  gst_pad_set_event_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_event));
  gst_pad_set_chain_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_chain));
  gst_pad_set_activate_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_activate));
  gst_pad_set_activatepull_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_activate_pull));
  gst_pad_set_activatepush_function (demux->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flv_demux_sink_activate_push));

  gst_element_add_pad (GST_ELEMENT (demux), demux->sinkpad);

  demux->adapter = gst_adapter_new ();
  demux->taglist = gst_tag_list_new ();
  gst_segment_init (&demux->segment, GST_FORMAT_TIME);

  demux->own_index = FALSE;

  gst_flv_demux_cleanup (demux);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (flvdemux_debug, "flvdemux", 0, "FLV demuxer");

  if (!gst_element_register (plugin, "flvdemux", GST_RANK_PRIMARY,
          gst_flv_demux_get_type ()) ||
      !gst_element_register (plugin, "flvmux", GST_RANK_PRIMARY,
          gst_flv_mux_get_type ()))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    "flv", "FLV muxing and demuxing plugin",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
