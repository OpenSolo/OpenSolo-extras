
/*
* Copyright (c) 2011-2013, Freescale Semiconductor, Inc. 
 */

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

/*!
 ***********************************************************************
 * Header file of Freescale deinterlace algoritm
 *
 *
 * History
 *   Date          Changed                                Changed by
 *   Dec. 04, 2007 Create                                 Zhenyong Chen
 *   Jan. 30, 2008 Add API DeinterlacerGetVersion         Zhenyong Chen
 *   Jun  26, 2008 Change version string API              Zhenyong Chen
 ***********************************************************************
 */


#ifndef __DEINTERLACE_API_H__
#define __DEINTERLACE_API_H__

#ifdef __cplusplus 
extern "C" { 
#endif 


// Chroma format
#define CHROM_FMT_420   0
#define CHROM_FMT_422   1
#define CHROM_FMT_444   2

// Deinterlace methods - predefined methods
#define DEINTMETHOD_BOB      0
#define DEINTMETHOD_WEAVE    1


/* Methods in this collection
 */
#define SAFE_BASE          2000
#define DEINTMETHOD_BLOCK_VT_NEW_FIELDSAD_BOB     (SAFE_BASE+2)
#define DEINTMETHOD_BLOCK_VT_NEW_FRAMESAD_BOB     (SAFE_BASE+3)
#define DEINTMETHOD_BLOCK_VT_NEW_FIELDSAD_4TAP    (SAFE_BASE+4)
#define DEINTMETHOD_BLOCK_VT_NEW_FRAMESAD_4TAP    (SAFE_BASE+5)
#define DEINTMETHOD_BLOCK_VT_GROUP_FRAMESAD_4TAP  (SAFE_BASE+6)
#define DEINTMETHOD_BLOCK_VT_GROUP_FRAMESAD_BOB   (SAFE_BASE+7)


// Attribute
typedef struct tagDeintMethod
{
    unsigned int method;  // ID of method
    char name[64];        // Name of method
    char need_prev_frame; // Whether previous frame is needed for reference
    char need_next_frame; // Whether next frame is needed for reference
    char safe;            // Safety for use of this method
}DEINTMETHOD;

typedef struct tagPICTURE
{
    BYTE *y;
    BYTE *cb;
    BYTE *cr;
}PICTURE;

typedef struct tagDEINTER
{
    // Frame buffers
    PICTURE frame[3]; // frame[0]: prevous frame; frame[1]: current frame; frame[2]: next frame
    // Parameter of luma/chroma stride
    int y_stride;
    int uv_stride;
    // Chroma format. 4:2:0 (0), 4:2:2 (1), 4:4:4 (2) support.
    int chrom_fmt;
    // Parameter of picture size (in pixel)
    int width;
    int height;
    // Which field is first
    BOOL top_first;
    // How to deinterlace
    int method;
    // Research usage, a way to let outside control deinterlacing process. Set NULL
    void *dynamic_params;
}DEINTER;

/* \Function
 *   InitDeinterlaceSafe
 * \Brief
 *   Init deinterlace algorithm, and get information of implemented 
 *   algorithms
 * \Return value
 *   None
 * \Parameters
 *   pMethods   [out] Buffer to store information
 *   count      [out] Count of methods implemented
 * \See also
 *   N/A
 */
void InitDeinterlaceSafe(DEINTMETHOD *pMethods, int *count);

/* \Function
 *   DeinterlaceSafe
 * \Brief
 *   Core function to deinterlace an interlace frame, and output a  
 *   progressive frame in same place as current frame
 * \Return value
 *   None
 * \Parameters
 *   pDeinterInfo  [inout] See definition of DEINTER
 * \See also
 *   Document ...
 */
void DeinterlaceSafe(DEINTER *pDeinterInfo);

/* \Function
 *   IsFrameReusableSafe
 * \Brief
 *   To determine current frame buffer can be reused when requres reloading
 * \Return value
 *   1  - reusable
 *   0  - not reusable
 *   -1 - undetermined
 * \Parameters
 *   nMethod    [in] Deinterlace method
 *   bDrawTrack [in] Whether tracking motion block is enabled. When this 
 *                   feature is enabled, frame buffer will be overwritten 
 *                   to show block edge
 * \Remark
 *   For somet deinterlace algorithms, current frame after filtering can 
 *   be reused for another frame deinterlacing (next frame, current frame, 
 *   or previous frame). To avoid reloading it, use this API to check whether
 *   whether current frame need (or not) reload.
 *
 * \See also
 *   N/A
 */
int  IsFrameReusableSafe(int nMethod, BOOL bDrawTrack);

/*
 * Get version description
 *
 * Parameters
 *   None
 *
 * Return value
 *   ASCII string describing version information. Details see
 *   api document
 */
const char * GetDeinterlaceVersionInfo();


#ifdef __cplusplus 
} 
#endif 

#endif /* __DEINTERLACE_API_H__ */

