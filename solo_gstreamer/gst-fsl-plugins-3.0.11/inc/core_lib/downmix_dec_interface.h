
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
 
/**************************** Change History ***********************************
 *
 *   DD/MM/YYYY Code Ver     Description                        Author
 *   --------   -------      -----------                        ------
 *   16/10/2007 0.0          create                             Guoyin Chen
 *
 ******************************************************************************/

#ifndef __DM_DEC_INTERFACE_H__
#define __DM_DEC_INTERFACE_H__

#include "ppp_interface.h"

#ifndef __cplusplus
#define DM_EXTERN_API extern
#else
#define DM_EXTERN_API extern "C"
#endif

#define DM_MAX_NUM_MEM_REQS 2
#define DM_FAST_MEMORY        0
#define DM_SLOW_MEMORY        1

/* Data type */
typedef unsigned char     DM_UINT8;
typedef char                   DM_INT8;
typedef unsigned short    DM_UINT16;
typedef short                  DM_INT16;
typedef unsigned int        DM_UINT32;
typedef int                      DM_INT32;

typedef enum
{
    DM_OK = 0,
    DM_ERROR_INVALIDARG,
    DM_ERROR_NOMEMORY,
    DM_ERROR_NOTSUPPORTED,
    DM_ERROR_NOENOUGH_OUTPUT,
    DM_ERROR_INIT,
    DM_ERROR_EOF
}DM_RET_TYPE;

typedef enum
{
    DM_MEM_SCRATCH = 0,
    DM_MEM_PERSISTENT
}DM_MEM_DESC;

typedef enum
{
	DM_MEM_PRIORITY_HIGH = 0,
	DM_MEM_PRIORITY_LOW
}DM_MEM_PRIORITY;

typedef struct {
	DM_INT32           		dm_size;     /* Size in bytes */
	DM_INT32 	     		dm_type;	   /* Memory type Fast or Slow */
	DM_MEM_DESC   		dm_mem_desc; /* to indicate if it is scratch memory */
	DM_MEM_PRIORITY       dm_priority; /* In case of fast memory, specify the priority */
	void 	            		*app_base_ptr; /* Pointer to the base memory , which will be allocated and
                                    * filled by the  application */
}DM_Mem_Alloc_Info_Sub;

typedef struct {
    DM_INT32         dm_num_reqs;    /*  Number of valid memory requests */
    DM_Mem_Alloc_Info_Sub mem_info_sub[DM_MAX_NUM_MEM_REQS];
}DM_Mem_Alloc_Info;

typedef void* DM_HANDLE;

typedef struct DM_Dec_Config {
       DM_Mem_Alloc_Info	        dm_mem_info;
       DM_HANDLE                    h_dm_decoder;
       DM_UINT32		            app_instance_id;
       DM_INT32             		dm_input_channels;
       DM_INT32             		dm_input_ch_mask;
       DM_INT32                     dm_input_bitwidth;
       DM_INT32                     dm_input_bitdepth;
       DM_INT32				        dm_input_low24bit;	/* flag of low 24 bit, only for 24 bit input*/
       DM_INT32             		dm_output_channels;
       DM_INT32             		dm_output_ch_mask;
       DM_INT32                     dm_output_bitwidth;
       DM_INT32                     dm_output_bitdepth;
       DM_INT32				        dm_output_low24bit;	/* flag of low 24 bit, only for 24 bit output*/
       DM_INT32             		dm_input_freq;
       DM_INT32             		WMflag;
} DM_Decode_Config;

DM_EXTERN_API DM_UINT32 dm_default_channel_mask(DM_UINT32 nChannels);

DM_EXTERN_API DM_RET_TYPE dm_query_dec_mem (DM_Decode_Config * dec_config);

DM_EXTERN_API DM_RET_TYPE dm_decode_init (DM_Decode_Config *dec_config);

DM_EXTERN_API DM_RET_TYPE dm_decode_frame (DM_Decode_Config *dec_config,
                                        PPP_INPUTPARA *ppp_inputpara,
                                        PPP_INFO *ppp_info);

DM_EXTERN_API const char *DownmixCodecVersionInfo (void);

#endif

