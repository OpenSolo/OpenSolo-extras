
/*
* Copyright (c) 2005-2009, 2013 Freescale Semiconductor, Inc. 
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
 
 /***************************************************************************
 *   CHANGE HISTORY
 *   dd/mm/yy        Code Ver     Description             Author
 *   --------        -------      -----------             ------
 *   23/10/07         0.1 	      created file            Jackie Yin.
 **************************************************************************/
#ifndef __WMAE_ENC_INTERFACE_H_
#define __WMAE_ENC_INTERFACE_H_
#define WMAE_TRUE               1
#define WMAE_FALSE              0
//***********************************************************************
//   Data type 
//***********************************************************************
typedef unsigned char		  WMAE_UINT8;
typedef char			        WMAE_INT8;
typedef unsigned short		WMAE_UINT16;
typedef short		        	WMAE_INT16;
typedef unsigned int		  WMAE_UINT32;
typedef int			          WMAE_INT32;
typedef WMAE_UINT32       WMAE_Bool;
#if _WIN32
typedef unsigned __int64        WMAE_UINT64;
typedef __int64                 WMAE_INT64;
#else
typedef unsigned long long      WMAE_UINT64;
typedef long long               WMAE_INT64;
#endif

/* status */
#ifndef _WMAENCODESTATUS_DEFINED
#define _WMAENCODESTATUS_DEFINED

typedef enum tagWMAEncodeStatus
{
    WMA_Succeeded = 0,
    WMA_Failed,
    WMA_BadMemory,
    WMA_NoMoreFrames,
    WMA_EncodeFailed,
    WMA_UnSupportedInputFormat,
    WMA_UnSupportedCompressedFormat,
    WMA_InValidArguments,
    WMA_BadSource,
} tWMAEncodeStatus;
#endif

//***********************************************************************
//  memory definition
//***********************************************************************
#define WMAE_FAST_MEMORY                  1
#define WMAE_SLOW_MEMORY                  0
#define WMAE_MAX_NUM_MEM_REQS             1 // (Temporary)
#define WMAE_MEM_TYPE                     WMAE_FAST_MEMORY /* can be changed */
#define STRING_SIZE 512		     //Really long name also will be taken care of
#define MAX_METADATA_CHARS 255 // Max length of Metadata in chars
#define NUMPACKET 4
#define MAX_PACKET_BYTE_LENGTH 1372*7 //WMA packet size. Should be multiple integer of 1372

//***********************************************************************
//  enum definition
//***********************************************************************
typedef enum
{
    WMAE_STATIC_MEM,        /* 0 for static memory */
    WMAE_SCRATCH_MEM        /* 1 for scratch memory */
} WMAE_MEM_DESC;

//***********************************************************************
//  struct definition
//***********************************************************************

#ifdef __arm
#define EMBARM_PACK __packed
#else
#define EMBARM_PACK
#endif

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
#pragma pack(push,1)
#ifdef _LINUX_BUILD_ 
typedef struct
#else
typedef EMBARM_PACK struct
#endif
{
    WMAE_UINT16     wFormatTag;        /* format type */
    WMAE_UINT16     nChannels;         /* number of channels (i.e. mono, stereo...) */
    WMAE_UINT32     nSamplesPerSec;    /* sample rate */
    WMAE_UINT32     nAvgBytesPerSec;   /* for buffer estimation */
    WMAE_UINT16     nBlockAlign;       /* block size of data */
    WMAE_UINT16     wBitsPerSample;    /* Number of bits per sample of mono data */
    WMAE_UINT16     cbSize;            /* The count in bytes of the size of
                                        * extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
#pragma pack(pop)
#endif

typedef struct {
    WMAE_INT32	    s32WMAESize;     /* Size in bytes */
    WMAE_INT32 	    s32WMAEType;     /* Memory type Fast or Slow */
    WMAE_MEM_DESC   s32WMAEMemDesc;  /* to indicate if it is scratch memory */
    WMAE_INT32      s32WMAEPriority; /* In case of fast memory, specify the priority */
    void 	          *app_base_ptr;   /* Pointer to the base memory , which will be allocated and
                                      * filled by the  application */
} WMAEMemAllocInfoSub;

typedef struct {
    WMAE_INT32              s32NumReqs;
    WMAEMemAllocInfoSub	    sMemInfoSub [WMAE_MAX_NUM_MEM_REQS];
} WMAEMemAllocInfo;

#ifndef _STRUCT_WMAFORMATINFO_
#define _STRUCT_WMAFORMATINFO_
typedef struct 
{
    // base WAVEFORMATEX
    WMAE_UINT32 nSamplesPerSec;
    WMAE_UINT32 nChannels;
    WMAE_UINT32 nAvgBytesPerSec;
    WMAE_UINT32 nBlockAlign;
    // extended WAVEFORMATES
    WMAE_UINT32 nSamplesPerBlock;
    WMAE_UINT32 dwSuperBlockAlign;
    WMAE_UINT16 wEncodeOptions;
    // miscellaneous
    WMAE_UINT32 nFramesPerPacket;
    WMAE_UINT32 nSamplesPerFrame;
    WMAE_UINT32 nMaxSamplesPerPacket;
    WMAE_UINT32 nLookaheadSamples;
    WMAE_UINT32 nSuperFrameSamples; // useless ?
    WMAE_UINT32 ulOfficialBitrate;
    WMAE_INT64  nAudioDelaySizeMs;  
} WMAFormatInfo;
#endif
#ifndef _STRUCT_WMAEENCODERPARAMS_
#define _STRUCT_WMAEENCODERPARAMS_
typedef struct
{
  void * App_szInputFileName;                 // input wav file name
  void * App_szOutputFileName;                // output wma file name
  WAVEFORMATEX	*App_pAudioInput;		      // audio input settings
  WMAE_UINT32   App_iAudioSrcLength;		  // length of the audio source
  WMAE_UINT32   App_iDstAudioBitRate;		  // audio output bitrate
  WMAE_UINT32   App_iDstAudioSampleRate;	// audio output sample rate
  WMAE_UINT32   App_iDstAudioChannels;		// audio output channel count
                                          //wma encoder variables here 
  WMAE_UINT32 WMAE_packet_byte_length;
  WMAE_Bool   WMAE_isPacketReady;
  WMAE_UINT32 WMAE_nEncodeSamplesDone;
  WMAFormatInfo pFormat;  
  
}WMAEEncoderParams;
#endif

typedef struct
{
  WMAEMemAllocInfo		 sWMAEMemInfo;
  void			          *psWMAEncodeInfoStructPtr; // Global_struct
  WMAEEncoderParams		*psEncodeParams; //for Encoder Params
}WMAEEncoderConfig;

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN 
#endif

EXTERN tWMAEncodeStatus eWMAEQueryMem( WMAEEncoderConfig *psEncodeConfig);

EXTERN tWMAEncodeStatus eInitWMAEncoder( WMAEEncoderConfig *psEncodeConfig );
                                
EXTERN tWMAEncodeStatus eWMAEncodeFrame( WMAEEncoderConfig* psEncodeConfig,
													      WMAE_INT16 *pInputBuffer,
                                WMAE_INT8 *pOutputBuffer,
                                WMAE_Bool bNoMoreData);

EXTERN const char * WMA8ECodecVersionInfo();

#endif
