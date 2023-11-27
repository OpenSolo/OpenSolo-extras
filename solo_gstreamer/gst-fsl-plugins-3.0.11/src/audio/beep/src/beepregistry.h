/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Copyright (c) 2011-2012, Freescale Semiconductor, Inc. All rights reserved.
 *
 */



/*
 * Module Name:    beepregistry.h
 *
 * Description:    Head file of unified audio decoder core functions
 *
 * Portability:    This code is written for Linux OS and Gstreamer
 */

/*
 * Changelog: 
 *
 */


#ifndef __BEEPREGISTRY_H__
#define __BEEPREGISTRY_H__

typedef struct _BeepCoreDlEntry
{
  char *mime;
  gchar **dl_names;
  gchar *dl_name;
  gchar *name;
  gchar *longname;
  gchar *description;
  gint rank;
  struct _BeepCoreDlEntry *next;
} BeepCoreDlEntry;

typedef struct
{
  UniACodecVersionInfo getVersionInfo;
  UniACodecCreate createDecoder;
  UniACodecDelete deleteDecoder;
  UniACodecReset resetDecoder;
  UniACodecSetParameter setDecoderPara;
  UniACodecGetParameter getDecoderPara;
  UniACodec_decode_frame decode;

  void *dl_handle;              /* must be last, for dl handle */
  BeepCoreDlEntry *dlentry;
} BeepCoreInterface;


BeepCoreDlEntry *beep_get_core_entry ();
GstCaps *beep_core_get_caps ();
BeepCoreInterface *beep_core_create_interface_from_caps (GstCaps * caps);
void beep_core_destroy_interface (BeepCoreInterface * inf);
GstCaps *beep_core_get_cap (BeepCoreDlEntry * entry);
gboolean beepDLAvialable (BeepCoreDlEntry *entry);


#endif /* __BEEPREGISTRY_H__ */
