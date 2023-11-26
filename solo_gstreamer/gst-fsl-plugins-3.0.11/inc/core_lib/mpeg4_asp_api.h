/*
 * Copyright (c) 2005-2010, 2013 Freescale Semiconductor, Inc.
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

/****************************************************************************
** mpeg4_asp_api.h 
** 
** 
** Description: Main file of Video Decoder Tester.
**
** Author:
**     Qiang Ding   <b02550@freescale.com>
**     
** Revision History: 
** ----------------- 
** 0.1  12/30/2008  Qiang Ding   Created this draft version
*****************************************************************************/ 
#ifndef __MPEG4_ASP_DEC_API__
#define __MPEG4_ASP_DEC_API__

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

/* The handle of decoder is the only identifier of a certain decoder instance. */
typedef void* MPEG4DHandle;

/* This enum holds the memory alignment type */
typedef enum
{
    E_MPEG4D_ALIGN_1BYTE = 0,	/*!< buffer can start at any place    */
    E_MPEG4D_ALIGN_2BYTE,	       /*!< start address's last 1 bit has to be 0  */
    E_MPEG4D_ALIGN_4BYTE,	       /*!< start address's last 2 bits has to be 0 */
    E_MPEG4D_ALIGN_8BYTE,	       /*!< start address's last 3 bits has to be 0 */
    E_MPEG4D_ALIGN_16BYTE,	       /*!< start address's last 4 bits has to be 0 */
    E_MPEG4D_ALIGN_32BYTE,	       /*!< start address's last 5 bits has to be 0 */
} eMpeg4DecMemAlignType;

/* This describes the memory chunk requirement details such as size, type, etc.  */
typedef struct
{
    signed long  			s32Size;	/*!< size of the memory block           */
    eMpeg4DecMemAlignType	eAlign;	/*!< alignment of the memory block      */
    void 			*            pvBuffer;/*!< pointer to allocated memory buffer */
} sMpeg4DecMemBlock;

/* Required memory information */
typedef struct
{
    sMpeg4DecMemBlock sFastMemBlk;  /*!Fast memory Block */
    sMpeg4DecMemBlock sSlowMemBlk; /*!Slow memory Block */
} sMpeg4DecMemAllocInfo;

/* high level stream information */
typedef struct
{
/*VOS level*/
    signed long    s32Profile;      /*stream profile, 0 for SP, 1 for ASP*/
    signed long    s32Level;        /*stream level*/
/*VOL level*/	
    unsigned short u16PaddedFrameWidth;  /*!< Padded FrameWidth*/
    unsigned short u16PaddedFrameHeight; /*!< Padded FrameHeight*/
    unsigned short u16ActFrameWidth;         /*!< Actual FrameWidth */
    unsigned short u16ActFrameHeight;        /*!< Actual FrameHeight*/
    unsigned short u16LeftOffset;            /*cropping origin x*/
    unsigned short u16TopOffset;             /*cropping origin y*/

} sMpeg4DecStreamInfo;

/* This structure holds initial information that the decoder needs to work */
typedef struct
{
    sMpeg4DecMemAllocInfo	sMemInfo;
    sMpeg4DecStreamInfo	sStreamInfo;
    signed long           s32MinFrameBufferCount;
} sMpeg4DecInitInfo;

/* This Data structure encapsulates the decoded YCbCr buffer. */
typedef struct
{
    unsigned char   *pu8YBuf;     /*!< Y Buf       must be 4 bytes aligned*/
    unsigned char   *pu8UBuf;     /*!< U Buf       must be 4 bytes aligned*/
    unsigned char   *pu8VBuf;     /*!< V Buf       must be 4 bytes aligned*/

    signed long     s32YBufLength; /*size must be padded_width x padded_height, maybe need not this item*/
    signed long     s32UBufLength; /*size must be padded_width x padded_height/4, maybe need not this item*/
    signed long     s32VBufLength; /*size must be padded_width x padded_height/4, maybe need not this item*/
    void *             pUsrTag  ;      /*a Tag that may be used by App. App can use this tag to easily manage the buffers.
                                     It's App implementation dependent, decoder will not use or change this tag, 
                                     App can ignore this tag also.*/
} sMpeg4DecYCbCrBuffer;

/* The callback functions used for getting frame buffers is defined below; it will get one frame buffer for decoder */
typedef void * (*cbGetOneFrameBuffer)( void* pvAppContext);

/* It is possible that the gotten frame buffer for decoding the current frame still stored the reference data 
   so the decoder has to reject the buffer. After the rejection by calling this callback function, 
   the decoder will invoke cbGetOneFrameBuffer to ask for frame buffer again */
typedef void (*cbRejectOneFrameBuffer)( void * mem_ptr, void* pvAppContext);

typedef void (*cbReleaseOneFrameBuffer)( void * mem_ptr, void* pvAppContext);

/* the above callback functions are grouped into a structure named FrameManager */
typedef struct _sMpeg4DecFrameManager
{
    cbGetOneFrameBuffer 	 GetterBuffer;
    cbRejectOneFrameBuffer 	 RejectorBuffer;
    cbReleaseOneFrameBuffer ReleaseBuffer;
    void*			              pvAppContext ;
}sMpeg4DecFrameManager; 

/* This structure can be used to facilitate the decoder to refine configuration according to 
** the application/platform's setting.  The maximum size of fast or slow memory (in bytes) 
** that the the application can allocate for the decoder, -1 for unrestricted  */
typedef struct
{
	  signed long  s32MaxFastMem;
	  signed long  s32MaxSlowMem;
} sMpeg4DecAppCap;

/* This enum holds the return types of the APIs */
typedef enum
{ 
	/* Successfull return values */
	E_MPEG4D_SUCCESS = 0,                  /* Success */
	E_MPEG4D_NO_OUTPUT,	                   /* decoded a frame but didn't finish, or it's NULL frame */
	E_MPEG4D_FRAME_SKIPPED,	               /* skipped this frame*/
	/* Successful return with a warning, decoding can continue */
		
	/* Recoverable error return, correct the situation and continue */
	E_MPEG4D_NOT_ENOUGH_BITS=31,           /* Not enough bits are provided to get width/height  */
	E_MPEG4D_OUT_OF_MEMORY,                /* Out of Memory                	*/
	E_MPEG4D_WRONG_ALIGNMENT,              /* Incorrect Memory Alignment        	*/
	E_MPEG4D_SIZE_CHANGED,                 /* Image size changed           */
	E_MPEG4D_INVALID_ARGUMENTS,            /* API arguments are invalid		*/
	E_MPEG4D_NO_HEADER_INFO,               /* no header in the stream when start to decode*/
		
	/* irrecoverable error type */
	E_MPEG4D_ERROR_STREAM=51,              /* Errored Bitstream		*/
	E_MPEG4D_FAILURE,                      /* Failure   		*/
	E_MPEG4D_UNSUPPORTED,                  /* Unsupported Format          */
	E_MPEG4D_NO_FRAME_BUFFER	             /* decoder can't get frame buffer */

} eMpeg4DecRetType;

/* For parameters setting and getting */
//typedef enum _eMPEG4DParameter 
//{
//	E_MPEG4_PARA_SKIP_B_FRAME=0,
//	E_MPEG4_PARA_SKIP_BNP_FRAME
//} eMPEG4DParameter;

/******************************************************************************************************/

/* The application uses this function to query the initialization info such as memory requirement 
   and minimal decoder buffer number of the decoder for a specific stream. 
   This function would return the initialization info for the decoder with being fed with the stream header. 
 */
EXTERN eMpeg4DecRetType eMPEG4DQueryInitInfo(sMpeg4DecInitInfo *psInitInfo, unsigned char  *pu8BitBuffer, signed long s32NumBytes, sMpeg4DecAppCap *pAppCap);
EXTERN eMpeg4DecRetType eMPEG4DQueryInitInfo_oldDX(sMpeg4DecInitInfo *psInitInfo, unsigned long uiWidth, unsigned long uiHeight, sMpeg4DecAppCap *pAppCap);

/* Create the decoder instance */
EXTERN eMpeg4DecRetType eMPEG4DCreate (sMpeg4DecInitInfo* psInitInfo, sMpeg4DecFrameManager* pFrameManager,	MPEG4DHandle* phMp4DecHandle); 

/* This is the main decoder function which should be called for decoding each frame.  */
EXTERN eMpeg4DecRetType eMPEG4DDecodeFrame (MPEG4DHandle hMp4DecHandle, void *pvBSBuf, long *s32NumBytes);
EXTERN eMpeg4DecRetType eMPEG4DDecodeFrame_oldDX (MPEG4DHandle hMp4DecHandle, void *pvBSBuf, long *s32NumBytes, int width, int height);

/* Flush one frame to avoid the frame was reserved by decoder. */
EXTERN eMpeg4DecRetType eMPEG4DFlushFrame (MPEG4DHandle hMp4DecHandle);

/* Get the frame to be displayed, the cropping work will be carried out in the application's scope. */
EXTERN eMpeg4DecRetType eMPEG4DGetOutputFrame (MPEG4DHandle hMp4DecHandle, sMpeg4DecYCbCrBuffer** ppsOutBuffer);

/* This function sets some indications to decoder to configure the additional features of the decoder */
EXTERN eMpeg4DecRetType eMPEG4DSetParameter (MPEG4DHandle hMp4DecHandle, int eParaName/*, void * u32ParaValue*/ );

/* This function is used for querying the current configuration of the decoder. */
EXTERN eMpeg4DecRetType eMPEG4DGetParameter (MPEG4DHandle hMp4DecHandle, /*eMPEG4DParameter eParaName, */void * pu32ParaValue );

/* Get the codec version information */
EXTERN const char * eMPEG4DCodecVersionInfo(void);

/* 
*******************************************************************************************
The acceptable name and value pairs 
-----------------------------------------------------------------------------------------
Parameter Name	                                   Parameter Value
-----------------------------------------------------------------------------------------
E_MPEG4_PARA_SKIP_B_FRAME	                       0: do not skip B frame(default value)
                                                   1: skip B frame
                                                   
E_MPEG4_PARA_SKIP_BNP_FRAME	                       0: do not skip B or P frame(default value)
                                                   1: skip B and P frame
                                                   
*******************************************************************************************
*/

#endif
