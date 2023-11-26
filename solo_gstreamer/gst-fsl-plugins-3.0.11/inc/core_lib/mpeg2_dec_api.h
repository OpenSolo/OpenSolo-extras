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

#ifndef DECODERINTERFACE_H
#define DECODERINTERFACE_H

#include "render_type.h"
/*****************************************************************************
*  File Description:   Contains header info for decoder interface.
*
*    DD/MMM/YYYY   Code Ver      Description                 Author
*    -----------   --------      -----------              ------------
*    12/Jun/2006       01           Created       			MANOJ ARVIND & DURGAPRASAD S. BILAGI
*    07/Nov/2007      02           add memory type		Eagle Zhou
*                    				(ENGR00056087)
*    09/Nov/2007      03           DR(ENGR00055417)        Wang Zening
*    11/Aug/2009      04           ENGR00115078              Eagle Zhou: add dropping B frames
*******************************************************************************
*
*/


#define MAX_NUM_MEM_REQS 30         /*! Maximum Number of MEM Requests  */

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


/*! All the API return one of the following */

typedef enum
{
    /* Successfull return values */
    E_MPEG2D_SUCCESS = 0,         /*!< Success                             */
    E_MPEG2D_PARTIAL_DECODE,
    E_MPEG2D_ENDOF_BITSTREAM,     /*!< End of Bit Stream                   */
    E_MPEG2D_FRAME_READY,
    E_MPEG2D_FRAME_SKIPPED,
    /* Successful return with warning, decoding can continue */
    /* Start with number 11 */

    /* recoverable error return, correct the situation and continue */
    E_MPEG2D_NOT_ENOUGH_BITS = 31,/*!< Not enough bits are provided        */
    E_MPEG2D_OUT_OF_MEMORY,       /*!< Out of Memory                       */
    E_MPEG2D_WRONG_ALIGNMENT,     /*!< Incorrect Memory alignment          */
    E_MPEG2D_SIZE_CHANGED,        /*!< Image size changed                  */
    E_MPEG2D_INVALID_ARGUMENTS,   /*!< API arguments are invalid           */
    E_MPEG2D_DEMO_PROTECT,         /* the output is corrupted by demo protection    */

    /* irrecoverable error type */
    E_MPEG2D_ERROR_STREAM = 51,   /*!< Errored Bitstream                   */
    E_MPEG2D_FAILURE,             /*!< Failure                             */
    E_MPEG2D_UNSUPPORTED,         /*!< Unsupported Format                  */
    E_MPEG2D_NO_IFRAME,           /*!< MPEG2D_first frame is not an I frame       */
    E_MPEG2D_SIZE_NOT_FOUND,      /*!< Frame size not found in bitstream   */
    E_MPEG2D_NOT_INITIALIZED      /*!< Decoder Not Initialised             */

} eMpeg2DecRetType;


/*! \enum Specifies the current functional state of the Decoder */
typedef enum
{
    E_MPEG2D_INVALID = 0,        /*!< Invalid Decoder State */
    E_MPEG2D_PLAY,               /*!< Decoder is decoding frames */
    E_MPEG2D_FF,                 /*!< Decoder is skipping frames */
    E_MPEG2D_REW                 /*!< Decoder is skipping frames in a direction
                                      opposite to PLAY and FF. Current Decoder
                                      doesn't support REW feature */
} eDecodeState;

/*! \enum Enumeration of possible buffer alignment */
typedef enum
{
    E_MPEG2D_ALIGN_NONE = 0,  /*!< buffer can start at any place           */
    E_MPEG2D_ALIGN_HALF_WORD, /*!< start address's last bit has to be 0    */
    E_MPEG2D_ALIGN_WORD       /*!< start adresse's last 2 bits has to be 0 */
} eMpeg2DecMemAlignType;


/**************************************************
 * Direct rendering type and data structure
 **************************************************/
//typedef void* (*bufferGetter)(void* /*pvAppContext*/);
//typedef void (*bufferRejecter)(void* /*mem_ptr*/, void* /*pvAppContext*/);

typedef struct _MPEG2D_FrameManager
{
    bufferGetter BfGetter;
    bufferRejecter BfRejector;
}MPEG2D_FrameManager;

/*! Different structure definitions */

/*! structure for Y Cb Cr output from the decoder. */
typedef struct
{
    unsigned char   *pu8YBuf;     /*!< Y Buf       */
    int             s32YBufLen;   /*!< Y Buf Len   */
} sMpeg2DecYCbCrBuffer;


/*! Structure to hold each memory block requests from the decoder.
 *  The size and alignment are must to meet criteria, whereas others
 *  help to achive the performance projected.
 */

typedef struct
{
    int 	s32Size;         /*!< size of the memory block            */
    int 	s32Type;         /*!< type of the memory - slow/fast and
                                  static/scratch                      */
    int     s32Priority;     /*!< how important the block is          */
    int 	s32Align;        /*!< alignment of the memory block       */
    void 	*pvBuffer;       /*!< pointer to allocated memory buffer  */
} sMpeg2DecMemBlock;


/*! Structure to hold all the memory requests from the decoder  */

typedef struct
{
    int               s32NumReqs;                   /*!< Number of blocks  */
	int 			  s32BlkNum ;					/*!< block number  */
    sMpeg2DecMemBlock asMemBlks[MAX_NUM_MEM_REQS];  /*!< array of requests */
    int               s32MinFrameBufferNum;  /*!< minimum number of frame buffer*/
} sMpeg2DecMemAllocInfo;


/*! Structure for defining the decoder output. */
typedef struct
{
    sMpeg2DecYCbCrBuffer sOutputBuffer;           /*!< Decoded frame      */
    signed char         *p8MbQuants;              /*!< Quant values       */       //ask
    unsigned short int 	 u16FrameWidth;           /*!< FrameWidth         */
    unsigned short int 	 u16FrameHeight;          /*!< FrameHeight        */
    unsigned int         bitrate;                 /*!< Bitrate			  */


} sMpeg2DecoderParams;



/*! This structure defines the decoder context. All the decoder API
 *  needs this structure as one of its argument.
 */
typedef struct
{
    sMpeg2DecMemAllocInfo   sMemInfo;      /*!< memory requirements info */
    sMpeg2DecoderParams     sDecParam;     /*!< decoder parameters       */
    void                    *pvMpeg2Obj;   /*!< decoder library object   */
    void                    *pvAppContext; /*!< Anything app specific    */
    eDecodeState            eState;        /*!< Indicates current Decoder
                                                State */
	/*Added for call back changes*/

    int (* ptr_cbkMPEG2DBufRead) (int *, unsigned char **, int , void *);

    /*Added for call back changes*/


} sMpeg2DecObject;



//Function Declarations
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

/*! Function to query memory requrement of the decoder */
EXTERN eMpeg2DecRetType  eMPEG2DQuerymem(sMpeg2DecObject *psMp2Obj);

/*! Function to query memory requrement of the decoder */
EXTERN eMpeg2DecRetType  eMPEG2D_Re_Querymem(sMpeg2DecObject *psMp2Obj);

/*! Function to initialize the decoder */
EXTERN eMpeg2DecRetType  eMPEG2D_Init (sMpeg2DecObject *psMp2Obj);

/*! API to decode single frame of the encoded bitstream */
EXTERN eMpeg2DecRetType eMPEG2Decode(sMpeg2DecObject *psMp2Obj,unsigned int *s32decodedBytes, void *pvAppContext);

/*! Function to free the resources allocated by decoder, if any */
EXTERN eMpeg2DecRetType  eMPEG2DFree (sMpeg2DecObject *psMp2Obj);


EXTERN eMpeg2DecRetType  eMPEG2D_ReInit (sMpeg2DecObject *psMp2Obj);

/*! \brief
 * Call back function for reading the input buffer by the decoder.
 *
 * Whenver the internul buffer inside the decoder is empty, the decoder will
 * call this function, passing the application context also. The application
 * shall implement a function to provide the required portion of the bitstream
 * of the given size, starting at the given offset in the provided buffer.
 */
EXTERN  int cbkMPEG2DBufRead (int *s32BufLen, unsigned char **pu8Buf,
                             int s32Offset, void *pvAppContext);

/*Added for call back by changes*/

EXTERN void Mpeg2_register_func(sMpeg2DecObject *,int (*) (int *, unsigned char **, int , void *));

/*Added to call back by changes*/
/**************************************************
 * Direct rendering API functions
 **************************************************/
/*! \brief
 *        Set the Frame manager to decoder,should be invoked after App invoked eMPEG2D_Init
 */
EXTERN void MPEG2DSetBufferManager (sMpeg2DecObject *psMp2Obj, MPEG2D_FrameManager* manager);


// new api format to support additional callback such as release,...
//typedef void (*bufferReleaser)(void* /*mem_ptr*/, void* /*pvAppContext*/);
//typedef enum
//{
//	E_GET_FRAME =0,
//	E_REJECT_FRAME,
//	E_RELEASE_FRAME,
//} eCallbackType; //add this to indicate additional callback function type.

//typedef enum
//{
//	E_CB_SET_OK =0,
//	E_CB_SET_FAIL,
//} eCallbackSetRet; //add this to indicate additional callback function type.

EXTERN eCallbackSetRet MPEG2DSetAdditionalCallbackFunction (sMpeg2DecObject *psMp2Obj, eCallbackType funcType, void* cbFunc);

EXTERN const char * MPEG2DCodecVersionInfo();

EXTERN eMpeg2DecRetType MPEG2DEnableSkipBMode(sMpeg2DecObject *psMp2Obj,int enableflag);

#endif  /* ifndef DECODERINTERFACE_H */
