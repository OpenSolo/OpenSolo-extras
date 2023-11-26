#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#if _USE_SEMAPHORE_
 #include <sys/sem.h>
#endif
#include <math.h>

#define _DBG_MIN_0_

#define _NUM_OF_REC_		1
#define DEFAULT_QUEUE_SIZE    6
#define _MINI_FRM_SZ_		DEFAULT_QUEUE_SIZE*45*45
#define _BITRATE_VIA_QP_	 0
#define _USE_SEMAPHORE_	0
#define _USE_WIRELESS_STATUS_ 0
#define _USE_REPORT_
#define _MAX_REC_		1
#define SEQTIME_STORE_SIZE 1000



typedef struct {
	int port;
	unsigned ipv4;
} socket_addr;

typedef struct {
	int frmrt_nu;
	int frmrt_de;
} frmrt;


#ifdef _USE_REPORT_
typedef struct {
    int enable;
    int shmid;
    int type;
    int size;
} enc_rep_info;
#endif

typedef struct
{
    socket_addr udp_sckt_rec[_NUM_OF_REC_];
#if _USE_SEMAPHORE_
    /*preset*/int semid_snd;
#endif
    unsigned long frames_ts_in[DEFAULT_QUEUE_SIZE];
    unsigned char mini_frames[_MINI_FRM_SZ_]; 
    int mini_frm_idx;	
    int numframebufs;
#ifdef _USE_REPORT_
    enc_rep_info mbrep;
    enc_rep_info mvrep;
    enc_rep_info slicerep;
#endif
#if _BITRATE_VIA_QP_
    int Qp;
    int intraQp; 
#endif
    int encoding; 
    unsigned bitsSize;
    int picType;
    int numSlices;
    uint64_t frame_cnt;
} enc_params;

typedef struct {
  int wrptr;
  int seqnum;
  unsigned char time[SEQTIME_STORE_SIZE];
  unsigned char seq[SEQTIME_STORE_SIZE];
}seqtime_info;

typedef struct
{
    int is_sender;
    enc_params enc; 
    unsigned reserved1;
    frmrt framerate;
    int minifrmsz;	
    int e1;
    int vbvBuffSize; 
    int initialDelay;
    unsigned reserved2;
    unsigned reserved3; 
    unsigned reserved4;
    unsigned reserved5;
    unsigned reserved6;
#if _USE_WIRELESS_STATUS_
    stats_wireless stats;
#endif
    unsigned bitrateall;
    unsigned maxseqnum;
    unsigned packetlost;
    unsigned jitter;
    int shmid;
    int reserved8;
    unsigned version;
    seqtime_info seqtimetrack;
    unsigned reserved9;
    unsigned reserved10;
    unsigned reserved11;
    unsigned int reserved12;
    unsigned int reserved13;
    unsigned int reserved14;
    unsigned int reserved15;
    unsigned int reserved16;
    unsigned int reserved17;
    unsigned int socket_error_cnt;
    unsigned int udpbuffersize;
    unsigned int reserved18;
    unsigned int reserved19;
    unsigned int reserved20;
    unsigned int reserved21;
    unsigned int reserved22;
    unsigned int reserved23;
    unsigned int reserved24;
    unsigned int reserved25;
    unsigned int reserved26;
    unsigned int reserved27;
    unsigned int reserved28;
    unsigned char reserve[988];
} intf_enc_rc_sn;

typedef struct
{
    int is_sender;
    unsigned ssrc;
    frmrt framerate;
    int decoding;
    int reserved1;
    unsigned reserved2;
#if _USE_SEMAPHORE_
    int semid_rec;
#endif
	struct sockaddr_in udpout;
	int reserved3;
    int reserved4;
    int reserved5;
    int vbvBuffSize;
    int initialDelay;
    unsigned reserved6;
    unsigned bitrateall;
    unsigned maxseqnum;
    unsigned packetlost;
    unsigned jitter;
    unsigned reserved7;
    int reserved8;
    int shmid;
    int reserved9;
    unsigned version;
    unsigned reserved10;
    unsigned reserved11;
    unsigned int reserved12;
    unsigned int reserved13;
    unsigned int reserved14;
    unsigned int reserved15;
    unsigned int reserved16;
    unsigned int reserved17;
    unsigned int reserved18;
    unsigned int socket_error_cnt;
    unsigned int udpbuffersize;
    unsigned int reserved19;
    unsigned int reserved20;
    unsigned int reserved21;
    unsigned int reserved22;
    unsigned int reserved23;
    unsigned int reserved24;
    unsigned int reserved25;
    unsigned char reserve[1320];
} intf_dec_rc_sn;

typedef struct 
{
  unsigned int resample_error;
  unsigned int gst_terminated;
  unsigned int shmid_audio;
} shmem_audio_struct;


