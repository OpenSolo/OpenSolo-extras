/* GStreamer
 * Copyright (C) 2010 Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>
 * Copyright (C) 2010 Nokia Corporation. All rights reserved.
 *   Contact: Stefan Kost <stefan.kost@nokia.com>
 *
 * Tremor modifications <2006>:
 *   Chris Lord, OpenedHand Ltd. <chris@openedhand.com>, http://www.o-hand.com/
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

#include <string.h>
#include "gstvorbisdeclib.h"

#ifndef TREMOR
/* These samples can be outside of the float -1.0 -- 1.0 range, this
 * is allowed, downstream elements are supposed to clip */
static void
copy_samples_m (vorbis_sample_t * out, vorbis_sample_t ** in, guint samples,
    gint channels, gint width)
{
  memcpy (out, in[0], samples * sizeof (float));
}

static void
copy_samples_s (vorbis_sample_t * out, vorbis_sample_t ** in, guint samples,
    gint channels, gint width)
{
#ifdef GST_VORBIS_DEC_SEQUENTIAL
  memcpy (out, in[0], samples * sizeof (float));
  out += samples;
  memcpy (out, in[1], samples * sizeof (float));
#else
  gint j;

  for (j = 0; j < samples; j++) {
    *out++ = in[0][j];
    *out++ = in[1][j];
  }
#endif
}

static void
copy_samples (vorbis_sample_t * out, vorbis_sample_t ** in, guint samples,
    gint channels, gint width)
{
#ifdef GST_VORBIS_DEC_SEQUENTIAL
  gint i;

  for (i = 0; i < channels; i++) {
    memcpy (out, in[i], samples * sizeof (float));
    out += samples;
  }
#else
  gint i, j;

  for (j = 0; j < samples; j++) {
    for (i = 0; i < channels; i++) {
      *out++ = in[i][j];
    }
  }
#endif
}

CopySampleFunc
get_copy_sample_func (gint channels, gint width)
{
  CopySampleFunc f = NULL;

  g_assert (width == 4);

  switch (channels) {
    case 1:
      f = copy_samples_m;
      break;
    case 2:
      f = copy_samples_s;
      break;
    default:
      f = copy_samples;
      break;
  }

  return f;
}

#else

/* Taken from Tremor, misc.h */
#ifdef _ARM_ASSEM_
static inline ogg_int32_t
CLIP_TO_15 (ogg_int32_t x)
{
  int tmp;
  asm volatile ("subs	%1, %0, #32768\n\t"
      "movpl	%0, #0x7f00\n\t"
      "orrpl	%0, %0, #0xff\n"
      "adds	%1, %0, #32768\n\t"
      "movmi	%0, #0x8000":"+r" (x), "=r" (tmp)
      ::"cc");

  return (x);
}
#else
static inline ogg_int32_t
CLIP_TO_15 (ogg_int32_t x)
{
  int ret = x;

  ret -= ((x <= 32767) - 1) & (x - 32767);
  ret -= ((x >= -32768) - 1) & (x + 32768);
  return (ret);
}
#endif

static void
copy_samples_32_m (vorbis_sample_t * _out, vorbis_sample_t ** _in,
    guint samples, gint channels, gint width)
{
  gint32 *out = (gint32 *) _out;
  ogg_int32_t **in = (ogg_int32_t **) _in;
  gint j;

  for (j = 0; j < samples; j++) {
    *out++ = CLIP_TO_15 (in[0][j] >> 9);
  }
}

static void
copy_samples_32_s (vorbis_sample_t * _out, vorbis_sample_t ** _in,
    guint samples, gint channels, gint width)
{
  gint32 *out = (gint32 *) _out;
  ogg_int32_t **in = (ogg_int32_t **) _in;
  gint j;

  for (j = 0; j < samples; j++) {
    *out++ = CLIP_TO_15 (in[0][j] >> 9);
    *out++ = CLIP_TO_15 (in[1][j] >> 9);
  }
}

static void
copy_samples_32 (vorbis_sample_t * _out, vorbis_sample_t ** _in, guint samples,
    gint channels, gint width)
{
  gint32 *out = (gint32 *) _out;
  ogg_int32_t **in = (ogg_int32_t **) _in;
  gint i, j;

  for (j = 0; j < samples; j++) {
    for (i = 0; i < channels; i++) {
      *out++ = CLIP_TO_15 (in[i][j] >> 9);
    }
  }
}

static void
copy_samples_16_m (vorbis_sample_t * _out, vorbis_sample_t ** _in,
    guint samples, gint channels, gint width)
{
  gint16 *out = (gint16 *) _out;
  ogg_int32_t **in = (ogg_int32_t **) _in;
  gint j;

  for (j = 0; j < samples; j++) {
    *out++ = CLIP_TO_15 (in[0][j] >> 9);
  }
}

static void
copy_samples_16_s (vorbis_sample_t * _out, vorbis_sample_t ** _in,
    guint samples, gint channels, gint width)
{
  gint16 *out = (gint16 *) _out;
  ogg_int32_t **in = (ogg_int32_t **) _in;
  gint j;

  for (j = 0; j < samples; j++) {
    *out++ = CLIP_TO_15 (in[0][j] >> 9);
    *out++ = CLIP_TO_15 (in[1][j] >> 9);
  }
}

static void
copy_samples_16 (vorbis_sample_t * _out, vorbis_sample_t ** _in, guint samples,
    gint channels, gint width)
{
  gint16 *out = (gint16 *) _out;
  ogg_int32_t **in = (ogg_int32_t **) _in;
  gint i, j;

  for (j = 0; j < samples; j++) {
    for (i = 0; i < channels; i++) {
      *out++ = CLIP_TO_15 (in[i][j] >> 9);
    }
  }
}

CopySampleFunc
get_copy_sample_func (gint channels, gint width)
{
  CopySampleFunc f = NULL;

  if (width == 4) {
    switch (channels) {
      case 1:
        f = copy_samples_32_m;
        break;
      case 2:
        f = copy_samples_32_s;
        break;
      default:
        f = copy_samples_32;
        break;
    }
  } else if (width == 2) {
    switch (channels) {
      case 1:
        f = copy_samples_16_m;
        break;
      case 2:
        f = copy_samples_16_s;
        break;
      default:
        f = copy_samples_16;
        break;
    }
  } else {
    g_assert_not_reached ();
  }
  return f;
}

#endif
