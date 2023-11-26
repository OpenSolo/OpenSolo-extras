/*
* Copyright (c) 2007-2013, Freescale Semiconductor, Inc. 
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

#ifndef _ASF_H_
#define _ASF_H_

#define ASF_HEADER sizeof(struct s_asf_packet_header_type)


/* status */
#ifndef _ASFRESULTS_DEFINED
#define _ASFRESULTS_DEFINED
typedef enum tagASFResults
{
    cASF_NoErr,                 /* -> always first entry */
                                /* remaining entry order is not guaranteed */
    cASF_Failed,
    cASF_BadArgument,
    cASF_BadAsfHeader,
    cASF_BadPacketHeader,
    cASF_BrokenFrame,
    cASF_NoMoreFrames,
}ASFRESULTS;
#endif


//***********************************************************************
//   Data type 
//***********************************************************************
typedef unsigned char           ASF_UINT8;
typedef char                    ASF_INT8;
typedef unsigned short          ASF_UINT16;
typedef short		            ASF_INT16;
typedef unsigned int            ASF_UINT32;
typedef int                     ASF_INT32;
typedef ASF_UINT32              ASF_Bool;

#if _WIN32
typedef unsigned __int64        ASF_UINT64;
typedef __int64                 ASF_INT64;
#else
typedef unsigned long long      ASF_UINT64;
typedef long long               ASF_INT64;
#endif



typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long long qword;

#ifndef _STRUCT_ASF_PACKET_
#define _STRUCT_ASF_PACKET_

#if ((defined(TGT_OS_LERVDS) || defined(TGT_OS_ELINUX))) && (defined(__arm))
#define PACKED  __packed
#else
#define PACKED  
#pragma pack(push,1)
#endif

typedef PACKED struct 
{ 
#ifdef UNDER_CE        
        char reserved[16];
#else
        byte reserved[16];
#endif  
  qword len;
} id_size_t;

typedef PACKED struct s_asf_header_type {

    // Header Object
    PACKED struct s_header_obj {
        id_size_t header;  
        byte cno[4];  
        byte v1;  
        byte v2;
    } header_obj;                       

    // File Properties Object
   PACKED struct s_file_obj {
        id_size_t fheader;
        byte file_id[16];
        byte file_size[8];
        byte creation_time[8];
        byte num_packets[8];
        byte play_duration[8];
        byte send_duration[8];
        byte preroll[8];
        byte flags[4];
        byte min_packet_size[4];
        byte max_packet_size[4];
        byte max_bitrate[4];
    } file_obj;                    

    // Stream Properites Object
    PACKED struct s_stream_obj {
        id_size_t sheader;
        byte     stream_type[16];
        byte     concealment[16];
        byte     time_offset[8];
        byte     type_size[4];
        byte     error_corr_size[4];
        byte     stream_num[2];
        byte     reserved1[4];

        PACKED struct s_audio_media {   // Specific data: Audio Media Type
            byte codec_id[2];
            byte channel_num[2];
            byte samples_per_sec[4];
            byte bytes_per_sec[4];
            byte block_alignment[2];
            byte bits_per_sample[2];
            byte codec_specific_size[2];
            byte samples_per_block[4];
            byte encode_options[2];
            byte super_block_align[4];
        } audio_media;

        PACKED struct s_spread_audio {       // Error Correction Data: Spread Audio
            byte span;
            byte virtual_packet_len[2];
            byte virtual_chunk_len[2];
            byte silence_data_len[2];
            byte silence_data;
        } spread_audio;
    } stream_obj;

    // Header Extension Object
    PACKED struct s_ext_obj {
        id_size_t extheader;
#ifdef UNDER_CE        
        char reserved[16];
#else
        byte reserved[16];
#endif        
        byte unknown[2];
        byte length[4];
    } ext_obj;

    // Codec List Object
    PACKED struct s_codec_obj {
        id_size_t codecheader;
        byte codec_reserved[16];
        byte entries_num[4];
        byte codec_type[2];
        byte codec_name_len[2];
        word codec_name[26];
        byte codec_descript_len[2];
        word codec_descript[26];
        byte codec_info_len[2];
        byte codec_info[2];
    } codec_obj;  

    // Content Description Object
    // This structure does not include the actual
    // content description fields (title, author...)
    PACKED struct s_cont_desc_obj {
        id_size_t contdescheader;
        word      title_len;
        word      author_len;
        word      copyright_len;
        word      description_len;
        word      rating_len;
    } cont_desc_obj;

    // Data Object
    PACKED struct s_data_obj {
        id_size_t dataheader;
        byte file_id[16];
        byte num_data_packets[8];
        byte reserved[2];    /* reserved the value shall be 0x11 */
    } data_obj;

} asf_header_type;


typedef PACKED struct s_asf_packet_header_type {
    byte error_corr_flags;
    byte error_corr_data[2];
    byte length_type_flags;
    byte property_flags;
    byte padding_length[2];
    byte send_time[4];
    byte duration[2];
    byte stream_id;
    byte media_obj_id;
    byte offset[4];
    byte replicated_len;
    byte media_obj_size[4];
    byte pres_time[4];
} asf_packet_header_type;
#ifndef __arm
#pragma pack(pop)
#endif
#endif

#ifndef _STRUCT_ASFFORMATINFO_
#define _STRUCT_ASFFORMATINFO_
typedef struct 
{
    // base WAVEFORMATEX
    ASF_UINT32 nSamplesPerSec;
    ASF_UINT32 nChannels;
    ASF_UINT32 nAvgBytesPerSec;
    ASF_UINT32 nBlockAlign;
    // extended WAVEFORMATES
    ASF_UINT32 nSamplesPerBlock;
    ASF_UINT32 dwSuperBlockAlign;
    ASF_UINT16 wEncodeOptions;
    // miscellaneous
    ASF_UINT32 nFramesPerPacket;
    ASF_UINT32 nSamplesPerFrame;
    ASF_UINT32 nMaxSamplesPerPacket;
    ASF_UINT32 nLookaheadSamples;
    ASF_UINT32 nSuperFrameSamples; // useless ?
    ASF_UINT32 ulOfficialBitrate;
    ASF_INT64  nAudioDelaySizeMs;  
} ASFFormatInfo;
#endif

typedef struct
{
  asf_header_type asf_header;
  ASF_INT32 media_offset;
  ASF_INT64 asf_packet_count;
  ASF_UINT8 wma_packet_id;
  ASF_INT64 wma_packet_count;
  ASF_INT64 asf_file_size;
  
  ASF_INT32 nAudioSampleperSec;
  ASF_INT32 nMaxBitRate;
  ASF_INT64 nSendTime;
  ASF_INT16 nDuration;
  
  ASF_UINT32 WMAE_packet_byte_length;
  ASF_INT32  g_asf_packet_size;
  ASF_INT32  g_asf_payload_length;

  ASF_UINT32 g_space_in_packet;
  
  ASF_UINT16 *g_wszTitle;        // Cont Desc: Title
  ASF_UINT16 *g_wszAuthor;       // Cont Desc: Author
  ASF_UINT16 *g_wszCopyright;    // Cont Desc: Copyright
  ASF_UINT16 *g_wszDescription;  // Cont Desc: Description
  ASF_UINT16 *g_wszRating;       // Cont Desc: Rating
  
  ASF_UINT16 g_cTitle;              // length of Title
  ASF_UINT16 g_cAuthor;             // length of Author
  ASF_UINT16 g_cCopyright;          // length of Copyright
  ASF_UINT16 g_cDescription;        // length of Description
  ASF_UINT16 g_cRating;             // length of Rating
  
  ASF_INT64 nAudioDelayBuffer;
  ASF_INT64 presentation_time; 
  ASF_INT64 nAudioSamplesDone;
  ASF_UINT32 nSize,nSR;  
  ASFFormatInfo pFormat; 
}ASFParams;

#define SWAP_WORD( w )    (w) = (((w) & 0xFF ) << 8) | (((w) & 0xFF00 ) >> 8)
#define SWAP_DWORD( dw )  (dw) = ((dw) << 24) | ( ((dw) & 0xFF00) << 8 ) | ( ((dw) & 0xFF0000) >> 8 ) | ( ((dw) & 0xFF000000) >> 24);

#define ASFPUT8(slot, val) {        \
  char *dst =(char *) slot;         \
  *dst++ = (char)((val));           \
  *dst++ = (char)((val)>> 8);       \
  *dst++ = (char)((val)>>16);       \
  *dst++ = (char)((val)>>24);       \
  *dst++ = (char)((val)>>32);       \
  *dst++ = (char)((val)>>40);       \
  *dst++ = (char)((val)>>48);       \
  *dst++ = (char)((val)>>56); }

#define ASFPUT4(slot, val) {        \
  char *dst = (char *) slot;        \
  *dst++ = (char)((val));           \
  *dst++ = (char)((val)>> 8);       \
  *dst++ = (char)((val)>>16);       \
  *dst++ = (char)((val)>>24); }

#define ASFPUT2(slot, val) {        \
  char *dst =(char *) slot;         \
  *dst++ = (char)((val));           \
  *dst++ = (char)((val)>>8); }

#ifdef L_ENDIAN

#define ASF_2(  word )  (word)
#define ASF_4( dword ) (dword)
#define ASF_8( qword ) (qword)

#else

// these may require modification depending
// upon your big-endian memory layout. the
// values need to be written out in little-
// endian.

#define ASF_2(  word )  SWAP_WORD(  word )
#define ASF_4( dword ) SWAP_DWORD( dword )
#define ASF_8( qword ) ( ( SWAP_DWORD( qword >> 32 ) << 32 ) \
                         | SWAP_DWORD( qword & 0xffffffff ) )
#endif


#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN 
#endif
//void asf_packetize(ASFParams *pAsfParams,char *outBuf);
//void update_asf_file_header(ASFParams *pAsfParams, char *outBuf);
EXTERN ASFRESULTS write_asf_file_header ( ASFParams *pAsfParams,ASF_INT8* buf);
EXTERN ASFRESULTS update_asf_file_header(ASFParams *pAsfParams,ASF_UINT32 kbps,ASF_UINT32 kHz,ASF_UINT32 ch,ASF_UINT16 nBlockAlign,ASF_INT32 cFrameSize);
EXTERN ASF_INT32 add_asf_file_header (ASFParams *pAsfParams);
EXTERN ASFRESULTS asf_packetize (ASFParams *pAsfParams,ASF_INT8 *RawInput, ASF_INT8 *asfOutBuf,ASF_Bool WMAE_isPacketReady,ASF_INT32 WMAE_nEncodeSamplesDone);
#endif



