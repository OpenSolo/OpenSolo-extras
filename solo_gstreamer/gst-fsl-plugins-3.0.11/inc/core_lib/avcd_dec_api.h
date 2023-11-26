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

/*!
 ***********************************************************************
 *
 *  \brief
 *      Contains all the H.264 interface function definitions
 *
 *
 *  \author
 *      - Faisal Ishtiaq            <faisal@motorola.com> <BR>
 *      - Raghavan Subramaniyan     <rags@labs.mot.com> <BR>
 *      - Shan Yu                   <shanyu@labs.mot.com> <BR>
 *      - Rohit Bodas               <bodas@labs.mot.com> <BR>
 *      - Bhavan Gandhi             <gandhi@labs.mot.com> <BR>
 *       Multimedia Communications Research Labs, Motorola Labs
 *
 *  \version
 *      $Revision: 1.8 $ - $Date: 2004/03/24 20:54:11 $
 *
 *  \Revision History
 *      MCRL             Created                          08 Aug, 2003
 *      Chandra          Changed variable names              Oct, 2004
 *                       and added Query Memory
 *      Chandra          Added Requery                    5, Nov, 2004
 *      Chandra          Added more return parameters    26, Nov, 2004
 *      Raja             Added application data pointer
 *                       to the callback functions       21, Nov, 2005
 *
 *
 ***********************************************************************
 */
  /*!
 ***********************************************************************
 * Author                  Comments                          Date      *
 * Abhishek Mehrotra    CR no - dsphl 27994(PAF Scheme)  9 March, 2006 *
 *                      Code changes for PAF Scheme                    *
 *                                                                     *
 ***********************************************************************
 */

  /*
****************************************************************************
 * Freescale ShangHai Video Codec Team Change History

  Version    Date                   Author		     CRs                       Comments
  01           1/Apr/2008          Li Xianzhong    ENGR00068494      Add buffer release API
  02           10/Apr/2008        Li Xianzhong     ENGR00072210     Add API to query physical address
  03           13/Jun/2008        Li Xianzhong     ENGR00080171     Add API to query codec version info
  04           25/Nov/2008        Chen Zhenyong    ENGR00100173     Syntax refinement;visibility declaration is added, sync with its implementation
  05           05/10/2010 	      Lyon Wang        engr123203        add include render_type.h for name conflict when include different ghdr
****************************************************************************
*/

#ifndef AVCD_DECODER_H
#define AVCD_DECODER_H

#include "render_type.h"

#ifdef __cplusplus
extern "C"{
#endif

#define AVCD_MAX_NUM_MEM_REQS          350
#define AVCD_DEFAULT_LEVEL_SUPPORT     51

/*! defines to specify type of memory. Only one of the two in each group
 * (speed and usage) shall be on but not both.
 */
#define E_AVCD_SLOW_MEMORY       0x1   /*! slower memory is acceptable */
#define E_AVCD_FAST_MEMORY       0x2   /*! faster memory is preferable */
#define E_AVCD_STATIC_MEMORY     0x4   /*! content is used over API calls */
#define E_AVCD_SCRATCH_MEMORY    0x8   /*! content is not used over
                                             successessive Decode API calls */

/*! retrieve the type of memory */

#define AVCD_IS_FAST_MEMORY(memType)    (memType & E_AVCD_FAST_MEMORY)
#define AVCD_IS_SLOW_MEMORY(memType)    (memType & E_AVCD_SLOW_MEMORY)
#define AVCD_IS_STATIC_MEMORY(memType)  (memType & E_AVCD_STATIC_MEMORY)
#define AVCD_IS_SCRATCH_MEMORY(memType) (memType & E_AVCD_SCRATCH_MEMORY)


/*! \enum Specifies the return state of the APIs */
typedef enum
{
    //!< Successful Completion
    E_AVCD_NOERROR =  0,                  //!< Successful decoding of frame
    E_AVCD_INIT,                          //!< Successful Initialization
    E_AVCD_QUERY,                         //!< Successful Query
    E_AVCD_SEQ_CHANGE,                    //!< Sequence Change Detected
    E_AVCD_CHANGE_SERVICED,               //!< Changed serviced by requery
    E_AVCD_FF,                            //!< In the fast forward state
//DSPhl28316-begin
//#if defined(DPB_FIX)
	E_AVCD_FLUSH_STATE,					  //!< In the flush state
//#endif
//DSPhl28316-end
    //!< Recoverable Errors, warnings and information
    E_AVCD_NOT_SUPPORTED,
    E_AVCD_BAD_PARAMETER,                 //!< Invalid parameter(s)
    E_AVCD_BAD_DATA_PTR ,                 //!< DataPtr -> invalid memory
    E_AVCD_NOMEM,                         //!< Not enough memory for decoding
    E_AVCD_NO_FRAME_BUFFER_CHANGE,        //!< No change in buffer requirement
    E_AVCD_FRAME_BUFFER_CHANGE,           //!< Change in memory requirement detected
    E_AVCD_NO_OUTPUT,                     //!< Successful decode but output not generated
    E_AVCD_NULL_POINTER,                  //!< Buffers not allocated as requested
    E_AVCD_BAD_DATA,                      //!< Input data not decodable
    //TLSbo72722
    E_AVCD_OUTPUT_FORMAT_NOT_SUPPORTED,	  //!< Output format expected is not supported
    //TLSbo72722-
/*ENGR00035713    Add feature to tolerate bit-stream without Sequence and/or Picture NAL  start*/
    /*these 2 error is recoverable, just try to feed in more NAL until these 2 kind of NAL appears
    it means App can just treat these 2 types as  E_AVCD_NO_OUTPUT*/
    E_NO_PICTURE_PAR_SET_NAL,        //!< did not receive PICTURE_PAR_SET_NAL yet
    E_NO_SEQUENCE_PAR_SET_NAL,        //!< did not receive SEQUENCE_PAR_SET_NAL yet
/*ENGR00035713    Add feature to tolerate bit-stream without Sequence and/or Picture NAL  end*/
	/*ENGR80171 Add Demo protection type*/
	E_AVCD_DEMO_PROTECT,                  //!<only for demo version, returned after decoded frame count is greater than 9000
    //!< Irrecoverable Errors
    E_AVCD_CODEC_TYPE_NOT_SUPPORTED,      //!< Unsupported codec type
    E_AVCD_INVALID_PARAMETER_SET,         //!< Invalid Pic/Seq param set
    E_AVCD_UNKNOWN_ERROR = 127,           //!< Some Unknown error
}eAVCDRetType;

/*! \enum Specifies the alignment requirement of the each of the memory to be allocated */
typedef enum
{
   E_AVCD_BYTE_ALIGN = 0,
   E_AVCD_HALFWORD_ALIGN,
   E_AVCD_THIRDBYTE_ALIGN,
   E_AVCD_WORD_ALIGN,
}eAVCDAlign;

//TLSbo72722
/*! \enum Specifies the output format required */
typedef enum
{
   E_AVCD_420_PLANAR = 0,
   E_AVCD_420_PLANAR_PADDED,
   E_AVCD_422_UYVY

}eAVCDOutputFormat;
//TLSbo72722-

/*! Structure to hold each memory block requests from the decoder.
 *  The size and alignment are must to meet crteria, whereas others
 *  help to achive the performace projected. Usage is provided to help
 *  the application to decide if the memory has to be saved, in case of
 *  memory scarcity. Type of the memory has a direct impact on the
 *  performance. Priority of the block shall act as a hint, in case not
 *  all requested FAST memory is available. There is no gurantee that
 *  the priority will be unique for all the memory blocks.
 */
typedef struct
 {
     int                 s32Size;                   //!< Size of the memory to be allocated
     int                 s32Align;                  //!< memory usage -  static/scratch
     int                 s32Type;                   //!< type of the memory slow/fast
     int                 s32Priority;               //!< how important the block is
     int                 s32SizeDependant;          /*!< Indication if the parameter
                                                         depends on size of frame*/
     int                 s32Allocate;               //!< Indicates memory is required to be
                                                    //!< allocated to the element
     int                 s32Copy;                   /*!< Indicates if whether data in previously
                                                         allocated memory is to be
                                                         copied before reallocaing memory*/
     int                 s32MaxSize;                //!< Maximum possible size
     void               *pvBuffer;                  //!< Pointer to the memory
}sAVCDMemBlock;

/*! Structure to hold all the memory requests from the decoder  */
typedef struct
{
     int                 s32NumReqs;
     sAVCDMemBlock       asMemBlks[AVCD_MAX_NUM_MEM_REQS];
     int               s32MinFrameBufferNum;  /*!< minimum number of frame buffer*/
}sAVCDMemAllocInfo;

/*! Structure for defining the decoder output */
typedef struct
{
    // 4:2:0 format is assumed
    unsigned char       *pu8y, *pu8cb, *pu8cr;      //!< Ptr to Output Y.Cb.Cr buffer
    long                 s32FrameNumber;            //!< Current Frame Number (@29.97 fps)
    short                s16FrameWidth;             //!< Width of frame
    short                s16FrameHeight;            //!< Height of frame
    short                s16Xsize;                  /*!<
                                                      X dimension of y buffer. Should
                                                      be greater than or equal to
                                                      frameWidth */
    short                s16CxSize;                 /*!<
                                                      X dimension of cb/cr buffer.
                                                      Should be greater than or equal
                                                      to frameWidth/2 */
   //TLSbo72722
   eAVCDOutputFormat 	eOutputFormat;				//!< Format of the output data, to be
   //TLSbo72722-													//!< populated by the application before calling
    int                 cropLeft_display;
    int                 cropTop_display;

  													//!< decoder init operation
}
sAVCDYCbCrStruct;

/*! Structure for defining the latest decoder configuration (NAL unit that is decoded)*/
typedef struct
{
    short                s16FrameWidth;              //!< Width of frame
    short                s16FrameHeight;             //!< Height of frame
    short                s16NumRefFrames;            //!< Number of reference frames used
    short                s16Level;                   //!< Level of the decoded bitstream
//DSPhl28316-begin
//#if defined(DPB_FIX)
	unsigned int		 u32MaxDPB;					//!< Max DPB supported for the corresponding level
//#endif
//DSPhl28316-end
}
sAVCDConfigInfo;

/*! Structure for defining the Decoder Configuration */
typedef struct
{
    // --- Parameters set by application ---
    // NOTE: These need to be set before call to eAVCDInitVideoDecoder (by application)
    long                 s32NumBytes;                //! Num of bytes in input buffer
    unsigned short       s32NalType;                 //! Type of NAL unit being decoded

    // --- Parameters set by video decoder ---
    void                *pvInBuffer;                  //!< Ptr to Input Bitstream (1 NALU)
    int                  s32FrameNumber;              //!< Frame number decoded
    long                 s32InBufferLength;           //!< Size (in bytes) of input frame
    void                *pvAVCData;                   //!< Ptr to Decoder datastructure
    sAVCDConfigInfo      sConfig;                     //!< Config Info of the bitstream
    sAVCDMemAllocInfo    sMemInfo;                    //!< Memory Configuration structure
    sAVCDYCbCrStruct     sFrameData;                  //!< Output pointer where decoded frame
                                                      //!< is to be written.
    int                  paf;
    unsigned char        u8Status;                    //!<flag indicating decoding status
	void                *pAppContext;                 //!< application data pointer //DSPhl27777

    // Call Back Function
    int   (*cbkAVCDBufRead)  (unsigned char *pu8Buf, int s32BufLen ,
                              int *s32Last, void *pAppContext); //DSPhl27777


}
sAVCDecoderConfig;
/**************************************************
 * Direct rendering type and data structure
 **************************************************/
//typedef void* (*bufferGetter)(void* /*pvAppContext*/);
//typedef void (*bufferRejecter)(void* /*mem_ptr*/, void* /*pvAppContext*/);
typedef struct _AVCD_FrameManager
{
    bufferGetter BfGetter;
    bufferRejecter BfRejector;
}AVCD_FrameManager;
/**************************************************
 * Deblock type
 **************************************************/
typedef enum
{
	E_AVCD_SW_DEBLOCK = 0,
	E_AVCD_HW_DEBLOCK,
}eAVCDDeblockOption;

/* Visibility declaration when building and using dynamic library
 * 1. Microsoft Visual Studio
 *    Use __declspec(dllexport) and __declspec(dllimport)
 * 2. armcc
 *    Same as 1
 * 3. gcc
 *    Use __attribute__((visibility("default"))), and compile option -fvisibility=hidden
 *
 */
#if defined(EXPORT_DLL)
#if defined(MY__GNUC__)
#define _FSL_EXPORT_C __attribute__((__visibility__("default")))
#else // end of MY__GNUC__
#if !defined(ARMCC) && !defined(WINCE_COMPILE) && !defined(MSVC)
#pragma message("Warning: unspecified compiler type. Using default visibility syntax")
#endif
#define _FSL_EXPORT_C __declspec(dllexport)

#endif // end of MSVC and ARMCC

#elif defined(IMPORT_DLL)
#if defined(MY__GNUC__)
#define _FSL_EXPORT_C __attribute__((__visibility__("default")))
#else // end of MY__GNUC__
#if !defined(ARMCC) && !defined(WINCE_COMPILE) && !defined(MSVC)
#pragma message("Warning: unspecified compiler type. Using default visibility syntax")
#endif
#define _FSL_EXPORT_C __declspec(dllimport)

#endif // end of MSVC and ARMCC

#else // end of IMPORT_DLL
#define _FSL_EXPORT_C
#endif // end of ! IMPORT_DLL and ! EXPORT_DLL

    /*! Function to query memory requrement of the decoder */
    _FSL_EXPORT_C eAVCDRetType   eAVCDInitQueryMem(sAVCDMemAllocInfo *psMemPtr);

    /*! Function to Re-query memory requrement of the decoder */
    _FSL_EXPORT_C eAVCDRetType   eAVCDReQueryMem (sAVCDecoderConfig *psAVCDec);

    /*! Function to initialize the decoder */
    _FSL_EXPORT_C eAVCDRetType   eAVCDInitVideoDecoder(sAVCDecoderConfig *psAVCDec);

    /*! API to decode single frame of the encoded bitstream */
    _FSL_EXPORT_C eAVCDRetType   eAVCDecodeNALUnit(sAVCDecoderConfig *psAVCDec, unsigned char u8FastFwd);

//DSPhl28316-begin

	/*! Function to flush the final frames , if any */
	_FSL_EXPORT_C eAVCDRetType   eAVCDecoderFlushAll(sAVCDecoderConfig *psAVCDec);

//DSPhl28316-end

    /*! Function to free the resources allocated by decoder, if any */
    _FSL_EXPORT_C eAVCDRetType   eAVCDFreeVideoDecoder(sAVCDecoderConfig *psAVCDec);


    _FSL_EXPORT_C void eAVCDGetFrame( sAVCDecoderConfig *vdec );


// Interfaces implemented by framework
typedef int (*cbkAVCDPrefetchNAL)(unsigned char **pbuf, int *len, void* /*pvAppContext*/);
typedef void (*cbkAVCDLengthSetter)(int len_after_destuff, void* /*pvAppContext*/);
typedef struct _NAL_FUNC
{
	cbkAVCDPrefetchNAL NALFetcher;
	cbkAVCDLengthSetter NALLengthSetter;
}sAVCDNAL_FUNCs;
_FSL_EXPORT_C void eAVCDSetNALFuncs( sAVCDNAL_FUNCs *NalFuncs );
/**************************************************
 * Direct rendering API function
 **************************************************/
/*! \brief
 *        Set the Frame manager to decoder
 */
_FSL_EXPORT_C void AVCDSetBufferManager (sAVCDecoderConfig *psAVCDec, AVCD_FrameManager* manager);

// new api format to support additional callback such as release,...
//typedef void (*bufferReleaser)(void* /*mem_ptr*/, void* /*pvAppContext*/);
typedef unsigned int (*queryPhysicalAddr)(void* /*virt_ptr*/, void* /*pvAppContext*/);

//typedef enum
//{
// E_GET_FRAME =0,
// E_REJECT_FRAME,
// E_RELEASE_FRAME,
// E_QUERY_PHY_ADDR,
//} eCallbackType; //add this to indicate additional callback function type.

//typedef enum
//{
// E_CB_SET_OK =0,
// E_CB_SET_FAIL,
//} eCallbackSetRet; //add this to indicate additional callback function type.

_FSL_EXPORT_C eCallbackSetRet H264SetAdditionalCallbackFunction (sAVCDecoderConfig *psAVCDec, eCallbackType funcType, void* cbFunc);


/**************************************************
 * Deblock type set/get API functions
 **************************************************/
/*! \brief
 *        Set the SW/HW deblock option
 */
_FSL_EXPORT_C void AVCDSetDeblockOption(sAVCDecoderConfig *psAVCDec, eAVCDDeblockOption deblockOption);
/*! \brief
 *        Get the SW/HW deblock option
 */
_FSL_EXPORT_C eAVCDDeblockOption AVCDGetDeblockOption(sAVCDecoderConfig *psAVCDec);

_FSL_EXPORT_C const char * H264DCodecVersionInfo();

#ifdef __cplusplus
}
#endif

#endif

