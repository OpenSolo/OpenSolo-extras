/*
* Copyright (c) 2011-2012, Freescale Semiconductor, Inc. All rights reserved.
* Copyright (c) 2015, Sculpture Networks Inc. All rights reserved.
*
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

/*
* Module Name:   vpuenc.c
*
* Description:   Implementation of VPU-based video encoder gstreamer plugin
*
* Portability:   This code is written for Linux OS and Gstreamer
*/

/*
* Changelog: 
*
*/

#include <string.h>

#include "vpuenc.h"
#include "gstsutils.h"

#include "stats_sn_gst.h"
#define SN_VERSION  0x2000

#define DEFAULT_BITSTREAM_BUFFER_LENGTH (1024*1024)     /* 1M bytes */

#define IS_DMABLE_BUFFER(buffer) ( (GST_IS_BUFFER_META(buffer->_gst_reserved[G_N_ELEMENTS(buffer->_gst_reserved)-1])) \
                                 || ( GST_IS_BUFFER(buffer) \
                                 &&  GST_BUFFER_FLAG_IS_SET((buffer),GST_BUFFER_FLAG_LAST)))
#define DMABLE_BUFFER_PHY_ADDR(buffer) (GST_IS_BUFFER_META(buffer->_gst_reserved[G_N_ELEMENTS(buffer->_gst_reserved)-1]) ? \
                                        ((GstBufferMeta *)(buffer->_gst_reserved[G_N_ELEMENTS(buffer->_gst_reserved)-1]))->physical_data : \
                                        (gpointer)(guint32)GST_BUFFER_OFFSET(buffer))


#define GST_CAT_DEFAULT gst_vpuenc_debug


#define VPU_RAW_CAPS \
  VIDEO_RAW_CAPS_TILED ";"\
  VIDEO_RAW_CAPS_NV12 ";"\
  VIDEO_RAW_CAPS_I420

#define CORE_API(func, elseroutine, err, ...)\
 do{\
     g_mutex_lock(vpuenc->lock);\
     (err) = (func)( __VA_ARGS__ );\
     g_mutex_unlock(vpuenc->lock);\
     if (VPU_DEC_RET_SUCCESS!=(err)){\
       GST_ERROR("func %s failed!! with ret %d", STR(func), (err));\
       elseroutine;\
     }\
 }while(0)

#define CORE_API_UNLOCKED(func, elseroutine, err, ...)\
 do{\
     (err) = (func)( __VA_ARGS__ );\
     if (VPU_DEC_RET_SUCCESS!=(err)){\
       GST_ERROR("func %s failed!!", STR(func));\
       elseroutine;\
     }\
 }while(0)


#define Align(ptr,align)	((align) ? ((((guint32)(ptr))+(align)-1)/(align)*(align)) : ((guint32)(ptr)))

#define VPUENC_TS_BUFFER_LENGTH_DEFAULT (1024)
#define VPUENC_DEFAULT_BITRARE_KBPS(w,h,fr) ((w)*(h)*3/2*(fr)*8/80/1000)

#define ATTACH_MEM2VPUDEC(vpuenc, desc)\
 do {\
   (desc)->next = (vpuenc)->mems;\
   (vpuenc)->mems = (desc);\
 }while(0)

enum
{
  PROP_0,
  PROP_CODEC,
  PROP_SEQHEADER_METHOD,
  PROP_TIMESTAMP_METHOD,

  PROP_CBR,
  PROP_BITRATE,
  PROP_GOPSIZE,
  PROP_QUANT,

  PROP_FRAMERATE_NU,
  PROP_FRAMERATE_DE,
  PROP_FORCE_FRAMERATE,
  SN_RC_INTF_SHM_ID,	// property to assign shm id from client/manager master thread
};


static GType gst_vpuenc_get_codec_type (void);


static GstsutilsOptionEntry g_vpuenc_option_table[] = {
  {PROP_CODEC, "codec", "codec type",
        "The codec type for encoding",
        G_TYPE_ENUM, G_STRUCT_OFFSET (VpuEncOption, codec), "0",
        NULL, NULL,
      gst_vpuenc_get_codec_type},

  {PROP_SEQHEADER_METHOD, "seqheader-method",
        "method of sending sequence header",
        "(0)auto; (1)send in codec_data of caps; (2)send with first buffer; (3)always send codec_data with buffers",
        G_TYPE_INT,
      G_STRUCT_OFFSET (VpuEncOption, seqheader_method), "0", "0", "3"},

  {PROP_TIMESTAMP_METHOD, "timestamp-method",
        "method of do timestamp of buffer",
        "(0)use input buffer's timestamp; (1)use GST_CLOCK_TIME_NONE",
        G_TYPE_INT,
      G_STRUCT_OFFSET (VpuEncOption, timestamp_method), "0", "0", "1"},

  /* encode settings */
  {PROP_CBR, "cbr", "cbr", "constant bitrate", G_TYPE_BOOLEAN,
      G_STRUCT_OFFSET (VpuEncOption, cbr), "true"},
  {PROP_BITRATE, "bitrate", "bit rate", "set bit rate in bps, valid only when cbr is true, 0: auto", G_TYPE_INT64,
      G_STRUCT_OFFSET (VpuEncOption, bitrate), "0", "0", G_MAXINT_STR},
  {PROP_GOPSIZE, "gopsize", "gop size", "set number of frame in a gop",
        G_TYPE_INT, G_STRUCT_OFFSET (VpuEncOption, gopsize), "15", "1", 
      G_MAXINT_STR},
  {PROP_QUANT, "quant", "quant", "set quant value, valid only when cbr is false: H.264(0-51), Mpeg4/H.263(1-31), -1:auto", 
        G_TYPE_INT, G_STRUCT_OFFSET (VpuEncOption, quant), "-1", "-1",
      "51"},
  {PROP_FRAMERATE_NU, "framerate-nu", "framerate numerator",
        "set framerate numerator", G_TYPE_INT,
      G_STRUCT_OFFSET (VpuEncOption, framerate_nu), "30", "1", G_MAXINT_STR},
  {PROP_FRAMERATE_DE, "framerate-de", "framerate denominator",
        "set framerate denominator", G_TYPE_INT,
      G_STRUCT_OFFSET (VpuEncOption, framerate_de), "1", "1", G_MAXINT_STR},
  {PROP_FORCE_FRAMERATE, "force-framerate", "force framerate",
        "enable force fixed framerate for encoding", G_TYPE_BOOLEAN,
      G_STRUCT_OFFSET (VpuEncOption, force_framerate), "false"},
      {SN_RC_INTF_SHM_ID, "shmid", "ShmId-Ctrl-Rc",
          "ShmId for SN RC_CTRL", G_TYPE_INT,
          	G_STRUCT_OFFSET (VpuEncOption, shmid), "0", "0", G_MAXINT_STR},
  {-1, NULL, NULL, NULL, 0, 0, NULL},
};

static intf_enc_rc_sn* stats_sn = 0;	 
#if _USE_SEMAPHORE_
 static int semid_snd = -1;
 static struct sembuf sops[2];
#endif

typedef struct
{
  gint std;
  char *mime;
} VPUMapper;

static GEnumValue vpu_mappers[] = {
  {VPU_V_MPEG4, "video/mpeg, mpegversion=(int)4, systemstream=(boolean)false",
      "mpeg4"},
  {VPU_V_H263, "video/x-h263", "h263"},
  {VPU_V_AVC, "video/x-h264", "avc"},
  {VPU_V_MJPG, "image/jpeg", "mjpg"},
  {0, NULL, NULL}
};


static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VPU_RAW_CAPS)
    );

GST_DEBUG_CATEGORY (gst_vpuenc_debug);

static void gst_vpuenc_finalize (GObject * object);

static GstFlowReturn gst_vpuenc_chain (GstPad * pad, GstBuffer * buffer);
static gboolean gst_vpuenc_setcaps (GstPad * pad, GstCaps * caps);
static GstStateChangeReturn gst_vpuenc_state_change (GstElement * element,
    GstStateChange transition);
static gboolean gst_vpuenc_sink_event (GstPad * pad, GstEvent * event);
static gboolean gst_vpuenc_src_event (GstPad * pad, GstEvent * event);
static const GstQueryType *gst_vpuenc_query_types (GstPad * pad);
static gboolean gst_vpuenc_src_query (GstPad * pad, GstQuery * query);
static void vpuenc_free_memories (VpuEncMem * mems);



static GstCaps *
vpu_core_get_caps ()
{
  GstCaps *caps = NULL;
  GEnumValue *map = vpu_mappers;
  while ((map) && (map->value_name)) {
    if (caps) {
      GstCaps *newcaps = gst_caps_from_string (map->value_name);
      if (newcaps) {
        if (!gst_caps_is_subset (newcaps, caps)) {
          gst_caps_append (caps, newcaps);
        } else {
          gst_caps_unref (newcaps);
        }
      }
    } else {
      caps = gst_caps_from_string (map->value_name);
    }
    map++;
  }
  return caps;
}

static GstPadTemplate *
gst_vpuenc_src_pad_template (void)
{
  static GstPadTemplate *templ = NULL;

  if (!templ) {
    GstCaps *caps = vpu_core_get_caps ();

    if (caps) {
      templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, caps);
    }
  }

  return templ;
}


static const gchar *
vpuenc_find_mime (gint std)
{

  GEnumValue *mapper = vpu_mappers;
  while (mapper->value_name) {
    if (mapper->value == std) {
      return mapper->value_name;
    }
    mapper++;
  }
  return NULL;
}


static void
vpuenc_core_mem_free_normal_buffer (VpuEncMem * mem)
{
  VpuEncRetCode core_ret;

  if (mem) {
    if (mem->handle) {
      MM_FREE (mem->handle);
    }
    MM_FREE (mem);
  }
}


static VpuEncMem *
vpuenc_core_mem_alloc_normal_buffer (gint size)
{
  void * vaddr;
  VpuEncRetCode core_ret;
  VpuEncMem *mem = MM_MALLOC (sizeof (VpuEncMem));

  vaddr = MM_MALLOC (size);

  if ((mem == NULL) || (vaddr == NULL)) {
    goto fail;
  }

  mem->freefunc = vpuenc_core_mem_free_normal_buffer;
  mem->handle = vaddr;
  mem->size = size;
  mem->vaddr = vaddr;
  mem->paddr = NULL;
  mem->parent = NULL;
  return mem;
fail:
  if (vaddr) {
    MM_FREE (vaddr);
  }
  if (mem) {
    MM_FREE (mem);
    mem = NULL;
  }
  return mem;
}


static void
vpuenc_core_mem_free_dma_buffer (VpuEncMem * mem)
{
  VpuEncRetCode core_ret;

  if (mem) {
    if (mem->handle) {
      MM_UNREGRES (mem->handle, RES_FILE_DEVICE);
      CORE_API_UNLOCKED (VPU_EncFreeMem,, core_ret, mem->handle);
      MM_FREE (mem->handle);
    }
    MM_FREE (mem);
  }
}


static VpuEncMem *
vpuenc_core_mem_alloc_dma_buffer (gint size)
{
  VpuEncRetCode core_ret;
  VpuEncMem *mem = MM_MALLOC (sizeof (VpuEncMem));
  VpuMemDesc *vmem = MM_MALLOC (sizeof (VpuMemDesc));

  if ((mem == NULL) || (vmem == NULL)) {
    goto fail;
  }
  vmem->nSize = size;
  CORE_API_UNLOCKED (VPU_EncGetMem, goto fail, core_ret, vmem);
  mem->paddr = (void *) vmem->nPhyAddr;
  mem->vaddr = (void *) vmem->nVirtAddr;
  MM_REGRES (vmem, RES_FILE_DEVICE);
  mem->freefunc = vpuenc_core_mem_free_dma_buffer;
  mem->handle = vmem;
  mem->size = size;
  mem->parent = NULL;
  return mem;
fail:
  if (vmem) {
    MM_FREE (vmem);
  }
  if (mem) {
    MM_FREE (mem);
    mem = NULL;
  }
  return mem;
}

static void
vpuenc_free_framemem_pool(GstVpuEnc * vpuenc)
{
  vpuenc_free_memories(vpuenc->framemem_pool.mems);
  vpuenc->framemem_pool.mems = NULL;
  vpuenc->framemem_pool.frame_size = 0;
}


static gboolean
vpuenc_prealloc_memories (GstVpuEnc * vpuenc, VpuMemInfo * mem)
{
  gboolean ret = FALSE;
  int i;
  for (i = 0; i < mem->nSubBlockNum; i++) {
    VpuMemSubBlockInfo *block = &mem->MemSubBlock[i];

    if (block->nSize) {
      VpuEncMem *vblock;
      int size = block->nSize + block->nAlignment;
      if (block->MemType == VPU_MEM_VIRT) {
        if (vblock = vpuenc_core_mem_alloc_normal_buffer (size)) {
          block->pVirtAddr = (unsigned char *) Align (vblock->vaddr, block->nAlignment);
        } else {
          goto fail;
        }
      } else if (block->MemType == VPU_MEM_PHY) {
        if (vblock = vpuenc_core_mem_alloc_dma_buffer (size)) {
          block->pPhyAddr = (unsigned char *) Align (vblock->paddr, block->nAlignment);
          block->pVirtAddr = (unsigned char *) Align (vblock->vaddr, block->nAlignment);
        } else {
          goto fail;
        }
      } else {
        goto fail;
      }

      ATTACH_MEM2VPUDEC (vpuenc, vblock);

    } else {
      goto fail;
    }
  }
  ret = TRUE;
fail:
  return ret;
}

static void
vpuenc_free_memories (VpuEncMem * mems)
{
  VpuEncMem *mem = mems, *memnext;

  while (mem) {
    memnext = mem->next;
    if ((mem->handle)) {
      if (mem->freefunc) {
        mem->freefunc (mem);
      }
    }
    mem = memnext;
  }
}



static gboolean
vpuenc_core_init (GstVpuEnc * vpuenc)
{
  gboolean ret = FALSE;

  VpuVersionInfo version;

  VpuWrapperVersionInfo w_version;
  VpuEncRetCode core_ret;


  vpuenc->init = FALSE;
  vpuenc->downstream_caps_set = FALSE;

  vpuenc->frame_cnt = 0;
  vpuenc->codec_data = NULL;
  vpuenc->frameblock = NULL;
  vpuenc->mems = NULL;
  vpuenc->tsm = createTSManager (VPUENC_TS_BUFFER_LENGTH_DEFAULT);
  vpuenc->tsm_mode = MODE_AI;

  vpuenc->ispec.buffer_align = 1;
  vpuenc->ispec.tiled = FALSE;

  memset (&vpuenc->vpu_stat, 0, sizeof (VpuEncStat));

  vpuenc->force_copy = FALSE;
  vpuenc->forcekeyframe = FALSE;
  vpuenc->gop_frm_cnt=0;

  CORE_API (VPU_EncGetVersionInfo, goto fail, core_ret, &version);
  CORE_API (VPU_EncGetWrapperVersionInfo, goto fail, core_ret, &w_version);

  g_print (GREEN_STR ("vpuenc versions :)\n"));
  g_print (GREEN_STR ("\tplugin: %s\n", VERSION));
  g_print (GREEN_STR ("\twrapper: %d.%d.%d(%s)\n", w_version.nMajor,
          w_version.nMinor, w_version.nRelease,
          (w_version.pBinary ? w_version.pBinary : "unknown")));
  g_print (GREEN_STR ("\tvpulib: %d.%d.%d\n", version.nLibMajor,
          version.nLibMinor, version.nLibRelease));
  g_print (GREEN_STR ("\tfirmware: %d.%d.%d.%d\n", version.nFwMajor,
          version.nFwMinor, version.nFwRelease, version.nFwCode));

  CORE_API (VPU_EncQueryMem, goto fail, core_ret, &vpuenc->context.meminfo);

  if (!vpuenc_prealloc_memories (vpuenc, &vpuenc->context.meminfo)) {
    vpuenc_free_memories (vpuenc->mems);
    vpuenc->mems = NULL;
    goto fail;
  }

  ret = TRUE;
fail:
  return ret;

}

static void
vpuenc_core_deinit (GstVpuEnc * vpuenc)
{
  VpuEncRetCode core_ret;
  if (vpuenc->tsm) {
    destroyTSManager (vpuenc->tsm);
    vpuenc->tsm = NULL;
  }


  if (vpuenc->codec_data) {
    gst_buffer_unref (vpuenc->codec_data);
    vpuenc->codec_data = NULL;
  }

  if (vpuenc->frameblock) {
      vpuenc_core_mem_free_dma_buffer (vpuenc->frameblock);
      vpuenc->frameblock = NULL;
  }

  if (vpuenc->context.handle) {
    CORE_API (VPU_EncClose,, core_ret, vpuenc->context.handle);
    vpuenc->context.handle = 0;
  }

  vpuenc_free_memories (vpuenc->mems);
  vpuenc->mems = NULL;

  g_mutex_lock(vpuenc->framemem_pool_lock);
  vpuenc_free_framemem_pool(vpuenc);
  g_mutex_unlock(vpuenc->framemem_pool_lock);

  GST_INFO ("stat:\n\tin  : %lld\n\tout : %lld\n\tshow: %lld",
      vpuenc->vpu_stat.in_cnt, vpuenc->vpu_stat.out_cnt,
      vpuenc->vpu_stat.show_cnt);

}

static void vpuenc_detect_avcc(GstVpuEnc * vpuenc)
{
    gboolean isAvcc=FALSE;
    if(vpuenc->options.codec== VPU_V_AVC){
        GstCaps *peer_caps;
        GstStructure *s;
        gchar *stream_format;
        peer_caps = gst_pad_peer_get_caps_reffed (vpuenc->srcpad);
        if (peer_caps) {
           gint i, num;
           num=gst_caps_get_size (peer_caps);
           GST_LOG("peer caps num: %d ",num);
           for (i = 0; i < num; i++) {
               GstStructure *s = gst_caps_get_structure (peer_caps, i);
               if (gst_structure_has_name (s, "video/x-h264") &&
                    gst_structure_has_field (s, "stream-format")) {
                   stream_format = gst_structure_get_string (s, "stream-format");
                   GST_LOG("peer caps:  x-h264/stream-format= %s !",stream_format);
                   if(stream_format){
                       if(!strcmp (stream_format, "avc")){
                           isAvcc=TRUE;
                           break;
                       }
                   }
               }
            }
            gst_caps_unref (peer_caps);
        }
        else{
            GST_LOG("no peer caps !!");
        }
    }
    vpuenc->context.openparam.nIsAvcc=(TRUE==isAvcc)?1:0;
}

static void
_do_init (GType object_type)
{
  GST_DEBUG_CATEGORY_INIT (gst_vpuenc_debug, "vpuenc", 0, "VPU video ");
}

GST_BOILERPLATE_FULL (GstVpuEnc, gst_vpuenc, GstElement, GST_TYPE_ELEMENT,
    _do_init);

static void
gst_vpuenc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVpuEnc *self = GST_VPUENC (object);

  if (gstsutils_options_set_option (g_vpuenc_option_table,
          (gchar *) & self->options, prop_id, value) == FALSE) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
  if(self->options.gopsize == 8973) self->options.gopsize=0;
  if (SN_RC_INTF_SHM_ID==prop_id) {	
      GST_DEBUG (">>VPU_ENC: Shm ID =%u", self->options.shmid);
      if (stats_sn && -1!=(int)stats_sn) {
			GST_WARNING("VPU_ENC: Shm isn't null...");
	      shmdt((void*)stats_sn); }
  		stats_sn = (intf_enc_rc_sn*)shmat(self->options.shmid,NULL,0);
  		if((unsigned int)stats_sn == -1) {
    		GST_ERROR ("VPU_ENC: Shared memory attach failed...");
    		MFW_WEAK_ASSERT(0);	// don't apply exit() @ run time...
 		}
  GST_WARNING ("VPU_ENC: Shared memory attached");
#if _USE_SEMAPHORE_
    	MFW_WEAK_ASSERT(0<stats_sn->semid_snd);
      GST_DEBUG (">>VPU_ENC: Sem ID=%u", stats_sn->semid_snd);
	  		semid_snd= stats_sn->semid_snd;
#endif
     if (stats_sn && -1!=(int)stats_sn)  
       stats_sn->version = (stats_sn->version & 0xffff0fff) | SN_VERSION;
	 }

}

static void
gst_vpuenc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstVpuEnc *self = GST_VPUENC (object);

  if (gstsutils_options_get_option (g_vpuenc_option_table,
          (gchar *) & self->options, prop_id, value) == FALSE) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static GType
gst_vpuenc_get_codec_type (void)
{
  static GType codectype = 0;

  if (!codectype) {
    codectype = g_enum_register_static ("vpuenc_codec", vpu_mappers);
  }

  return codectype;
}


static void
gst_vpuenc_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_vpuenc_src_pad_template ());

  FSL_GST_ELEMENT_SET_DETAIL_SIMPLE (element_class, "VPU-based video encoder",
      "Codec/Encoder/Video",
      "Encode raw video to compressed data by using VPU");
}

static void
gst_vpuenc_class_init (GstVpuEncClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  object_class->set_property = GST_DEBUG_FUNCPTR (gst_vpuenc_set_property);
  object_class->get_property = GST_DEBUG_FUNCPTR (gst_vpuenc_get_property);
  object_class->finalize = GST_DEBUG_FUNCPTR (gst_vpuenc_finalize);

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_vpuenc_state_change);

  gstsutils_options_install_properties_by_options (g_vpuenc_option_table,
      object_class);
}


static void
gst_vpuenc_free_internal_frame(gpointer p)
{
  GstBufferMeta *meta = (GstBufferMeta *) p;
  VpuEncMem * mem = (VpuEncMem *) meta->priv;
  GstVpuEnc * vpuenc = (GstVpuEnc *)(mem->parent);

  g_mutex_lock(vpuenc->framemem_pool_lock);

  if (mem->size==vpuenc->framemem_pool.frame_size){
    mem->next = vpuenc->framemem_pool.mems;
    vpuenc->framemem_pool.mems = mem;
  }else{

    if (mem->freefunc) {
      mem->freefunc (mem);
    }

  }
  gst_object_unref(vpuenc);
  gst_buffer_meta_free (meta);
  g_mutex_unlock(vpuenc->framemem_pool_lock);
}




static GstFlowReturn
gst_vpuenc_alloc_buffer (GstPad * pad, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstVpuEnc * vpuenc = (GstVpuEnc *)GST_PAD_PARENT (pad);
  GstFlowReturn ret = GST_FLOW_ERROR;
  GstBuffer *gstbuf;
  VpuEncMem * mem = NULL;

  g_mutex_lock(vpuenc->framemem_pool_lock);

  if (size!=vpuenc->framemem_pool.frame_size){
    vpuenc_free_framemem_pool(vpuenc);
    vpuenc->framemem_pool.frame_size = size;
  }else if (vpuenc->framemem_pool.mems) {
    mem = vpuenc->framemem_pool.mems;
    vpuenc->framemem_pool.mems = mem->next;
  }

  GstBufferMeta *bufmeta = gst_buffer_meta_new ();
  gstbuf = gst_buffer_new ();

  if (mem==NULL)
    mem = vpuenc_core_mem_alloc_dma_buffer (size);

  if (mem == NULL) {
    goto bail;
  }

  GST_BUFFER_SIZE (gstbuf) = size;
  GST_BUFFER_DATA (gstbuf) = mem->vaddr;
  GST_BUFFER_OFFSET (gstbuf) = offset;
  gst_buffer_set_caps (gstbuf, caps);

  gint index = G_N_ELEMENTS (gstbuf->_gst_reserved) - 1;
  bufmeta->physical_data = mem->paddr;
  bufmeta->priv = mem;
  gstbuf->_gst_reserved[index] = bufmeta;
  mem->parent = gst_object_ref (vpuenc);

  GST_BUFFER_MALLOCDATA (gstbuf) = (guint8 *) bufmeta;
  GST_BUFFER_FREE_FUNC (gstbuf) = gst_vpuenc_free_internal_frame;
  *buf = gstbuf;
  ret = GST_FLOW_OK;

bail:
  g_mutex_unlock(vpuenc->framemem_pool_lock);
  return ret;
}


static void
gst_vpuenc_init (GstVpuEnc * vpuenc, GstVpuEncClass * klass)
{
  vpuenc->sinkpad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_pad_set_setcaps_function (vpuenc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_vpuenc_setcaps));
  gst_pad_set_bufferalloc_function (vpuenc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_vpuenc_alloc_buffer));
  gst_pad_set_chain_function (vpuenc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_vpuenc_chain));
  gst_pad_set_event_function (vpuenc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_vpuenc_sink_event));
  gst_element_add_pad (GST_ELEMENT (vpuenc), vpuenc->sinkpad);

  vpuenc->srcpad =
      gst_pad_new_from_template (gst_vpuenc_src_pad_template (), "src");
  gst_pad_set_event_function (vpuenc->srcpad,
      GST_DEBUG_FUNCPTR (gst_vpuenc_src_event));
  gst_element_add_pad (GST_ELEMENT (vpuenc), vpuenc->srcpad);
  gst_pad_use_fixed_caps (vpuenc->srcpad);

  gstsutils_options_load_default (g_vpuenc_option_table,
      (gchar *) & vpuenc->options);
  gstsutils_options_load_from_keyfile (g_vpuenc_option_table,
      (gchar *) & vpuenc->options, FSL_GST_CONF_DEFAULT_FILENAME, "vpuenc");

  memset (&vpuenc->context, 0, sizeof (VpuEncContext));
  vpuenc->context.openparam.eFormat = vpuenc->options.codec;
  vpuenc->context.openparam.nRotAngle = 0;      //90;
	vpuenc->context.openparam.sliceMode.sliceMode = 0;	/* 0: 1 slice per picture; 1: Multiple slices per picture */
	vpuenc->context.openparam.sliceMode.sliceSizeMode = 0; /* 0: silceSize defined by bits; 1: sliceSize defined by MB number*/
	vpuenc->context.openparam.sliceMode.sliceSize = 4000;  /* Size of a slice in bits or MB numbers */

	vpuenc->context.openparam.nInitialDelay=500;
	vpuenc->context.openparam.nVbvBufferSize=9000000;
    vpuenc->context.openparam.nIntraRefresh = 0;
	    vpuenc->context.openparam.nRcIntraQp = -1;

	vpuenc->context.openparam.nUserQpMax = 0;
	vpuenc->context.openparam.nUserQpMin = 0;
	vpuenc->context.openparam.nUserQpMinEnable = 0;
	vpuenc->context.openparam.nUserQpMaxEnable = 0;

	vpuenc->context.openparam.nUserGamma = (int)(0.75*32768);         /*  (0*32768 <= gamma <= 1*32768) */
	vpuenc->context.openparam.nRcIntervalMode= 0;        /* 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level */
	vpuenc->context.openparam.nMbInterval = 0;
	vpuenc->context.openparam.nAvcIntra16x16OnlyModeEnable = 0;
	// these params pertain to AVC codec
			//vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_constrainedIntraPredFlag = 1;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_disableDeblk = 0;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_deblkFilterOffsetAlpha = 6;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_deblkFilterOffsetBeta = 0;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_chromaQpOffset = 10;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_audEnable = 0;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_fmoEnable = 0;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_fmoType = 0;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_fmoSliceNum = 1;
			vpuenc->context.openparam.VpuEncStdParam.avcParam.avc_fmoSliceSaveBufSize = 32; /* FMO_SLICE_SAVE_BUF_SIZE */

	if (1/*cpu_is_mx6x*/) 
	{
		vpuenc->context.openparam.nMESearchRange=0;
		vpuenc->context.openparam.nMEUseZeroPmv=0;
		vpuenc->context.openparam.nIntraCostWeight=0;
	}
/////////////////////////////////////////////////////////////////////////////////////////////
  vpuenc->context.openparam.nChromaInterleave = 0;
  vpuenc->context.openparam.sMirror = VPU_ENC_MIRDIR_NONE;

  vpuenc->lock = g_mutex_new ();
  vpuenc->framemem_pool_lock = g_mutex_new ();
  VPU_EncLoad ();
}

static gboolean
gst_vpuenc_check_and_set_input_spec (GstVpuEnc * vpuenc, gint width,
    gint height)
{
  VpuInputSpec *ispec = &vpuenc->ispec;

  if (vpuenc->context.initinfo.nAddressAlignment > ispec->buffer_align) {
    vpuenc->force_copy = TRUE;
    if (ispec->tiled) {
      return FALSE;
    }
  }

  ispec->ysize =
      Align (width * height, vpuenc->context.initinfo.nAddressAlignment);
  ispec->uvsize =
      Align (width * height / 4, vpuenc->context.initinfo.nAddressAlignment);

  ispec->frame_size = ispec->ysize + 2 * ispec->uvsize;
  ispec->frame_extra_size = width * height / 4;
  return TRUE;
}


static gboolean
gst_vpuenc_core_create_and_register_frames (GstVpuEnc * vpuenc, gint num)
{
  gboolean ret = FALSE;
  GstFlowReturn retval;
  void *mv_paddr, *mv_vaddr;
  void *frame_paddr, *frame_vaddr;
  VpuFrameBuffer *vpucore_frame;
  VpuFrameBuffer *vpuframebuffers = MM_MALLOC (sizeof (VpuFrameBuffer) * num);
  VpuEncMem *mvblock;
  VpuEncMem *frameblock;
  VpuInputSpec *ispec = &vpuenc->ispec;

  if (vpuframebuffers == NULL) {
    goto fail;
  }

  if (gst_vpuenc_check_and_set_input_spec (vpuenc, Align(ispec->width,16),
          Align(ispec->height,16)) == FALSE) {
    goto fail;
  }

  if (ispec->frame_extra_size) {
    if ((mvblock =
            vpuenc_core_mem_alloc_dma_buffer (num *
                ispec->frame_extra_size))) {
      mv_paddr = mvblock->paddr;
      mv_vaddr = mvblock->vaddr;
      ATTACH_MEM2VPUDEC (vpuenc, mvblock);
    } else {
      goto fail;
    }
  };

  memset (vpuframebuffers, 0, sizeof (VpuFrameBuffer) * num);

  vpuenc->frame_num = 0;

  while (vpuenc->frame_num < num) {
    int size =
        ispec->frame_size + vpuenc->context.initinfo.nAddressAlignment - 1;
    frameblock =
        vpuenc_core_mem_alloc_dma_buffer (size);

    if (frameblock) {
      frame_paddr =
          (void *) Align (frameblock->paddr,
          vpuenc->context.initinfo.nAddressAlignment);
      frame_vaddr =
          (void *) Align (frameblock->vaddr,
          vpuenc->context.initinfo.nAddressAlignment);
      ATTACH_MEM2VPUDEC (vpuenc, frameblock);
      vpucore_frame = &vpuframebuffers[vpuenc->frame_num];

      vpucore_frame->nStrideY = Align(ispec->width,16);
      vpucore_frame->nStrideC = vpucore_frame->nStrideY / 2;

      vpucore_frame->pbufY = frame_paddr;
      vpucore_frame->pbufVirtY = frame_vaddr;

      vpucore_frame->pbufCb = frame_paddr + ispec->ysize;
      vpucore_frame->pbufVirtCb = frame_vaddr + ispec->ysize;

      vpucore_frame->pbufCr = vpucore_frame->pbufCb + ispec->uvsize;
      vpucore_frame->pbufVirtCr = vpucore_frame->pbufVirtCb + ispec->uvsize;

      vpucore_frame->pbufMvCol = mv_paddr;
      vpucore_frame->pbufVirtMvCol = mv_vaddr;

      mv_vaddr += ispec->frame_extra_size;
      mv_paddr += ispec->frame_extra_size;
      vpuenc->frame_num++;

    } else {
      GST_ERROR ("Can not allocate enough framebuffers for output!!");
      goto fail;
    }
  }
  VpuEncRetCode core_ret;
  CORE_API (VPU_EncRegisterFrameBuffer, goto fail, core_ret,
      vpuenc->context.handle, vpuframebuffers, num, ispec->width/*needn't align*/);
  ret = TRUE;


fail:
  if (vpuframebuffers) {
    MM_FREE (vpuframebuffers);
  }

  return ret;

}


static void
gst_vpuenc_finalize (GObject * object)
{
  GstVpuEnc *vpuenc;

  vpuenc = GST_VPUENC (object);

  VPU_EncUnLoad ();

  g_mutex_free (vpuenc->lock);
  g_mutex_free (vpuenc->framemem_pool_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
	if (stats_sn && -1!=(int)stats_sn) {
		shmdt ((void*)stats_sn); // detach SN shm upon mfw_vpu_enc exit
  		GST_ERROR ("VPU_ENC: Shared memory detached"); // for debug
	}
}

static gboolean
gst_vpuenc_check_and_adjust_input (GstVpuEnc * vpuenc, VpuInputSpec * ispec)
{
  gboolean ret = FALSE;
  gint pad_width = ispec->crop_left + ispec->width + ispec->crop_right;
  gint pad_height = ispec->crop_top + ispec->height + ispec->crop_bottom;


  /* FIXME should use copy for this situation */
  if ((pad_width % 8) || (pad_height % 8)) {
    GST_ERROR ("Input crop/resolution is out of alignment");
    vpuenc->force_copy = TRUE;
    goto bail;
  }
  if (ispec->crop_left) {
    if (ispec->tiled) {
      ispec->crop_left = 0;
    } else {
      ispec->crop_left = GST_ROUND_UP_16 (ispec->crop_left);
    }
  }
  if (ispec->crop_right) {
    ispec->crop_right = GST_ROUND_UP_16 (ispec->crop_right);
  }
  if (ispec->crop_top) {
    if (ispec->tiled) {
      ispec->crop_top = 0;
    } else {
      ispec->crop_top = GST_ROUND_UP_16 (ispec->crop_top);
    }
  }
  if (ispec->crop_bottom) {
    ispec->crop_bottom = GST_ROUND_UP_16 (ispec->crop_bottom);
  }

  ispec->width = pad_width - ispec->crop_left - ispec->crop_right;
  ispec->height = pad_height - ispec->crop_top - ispec->crop_bottom;

  ret = TRUE;

bail:
  return ret;
}

static gboolean
gst_vpuenc_setcaps (GstPad * pad, GstCaps * caps)
{
  gboolean ret = TRUE;
  GstVpuEnc *vpuenc;
  GstCaps *src_caps;
  VpuInputSpec *ispec;

  vpuenc = GST_VPUENC (GST_PAD_PARENT (pad));
  ispec = &vpuenc->ispec;

  GST_INFO ("Get upstream caps %" GST_PTR_FORMAT, caps);

  if (vpuenc->context.handle == 0) {
    GstStructure *structure;
    gint intvalue, intvalue1;
    guint32 u32value;
    const gchar *mime;
    const GValue *value = NULL;
    structure = gst_caps_get_structure (caps, 0);
    if (gst_structure_get_int (structure, "width", &intvalue)) {
      GST_INFO ("Set Width %d", intvalue);
      ispec->width = intvalue;
    }
    if (gst_structure_get_int (structure, "height", &intvalue)) {
      GST_INFO ("Set Height %d", intvalue);
      ispec->height = intvalue;
    }


    if (gst_structure_get_int (structure, CAPS_FIELD_CROP_TOP, &intvalue)) {
      GST_INFO ("Set crop-top %d", intvalue);
      ispec->crop_top = intvalue;
    }
    if (gst_structure_get_int (structure, CAPS_FIELD_CROP_BOTTOM, &intvalue)) {
      GST_INFO ("Set crop-bottom %d", intvalue);
      ispec->crop_bottom = intvalue;
    }
    if (gst_structure_get_int (structure, CAPS_FIELD_CROP_LEFT, &intvalue)) {
      GST_INFO ("Set crop-left %d", intvalue);
      ispec->crop_left = intvalue;
    }
    if (gst_structure_get_int (structure, CAPS_FIELD_CROP_RIGHT, &intvalue)) {
      GST_INFO ("Set crop-right %d", intvalue);
      ispec->crop_right = intvalue;
    }

    if (gst_structure_get_int (structure, "alignment", &intvalue)) {
      if (intvalue) {
        ispec->buffer_align = intvalue;
      }
    }

    ispec->width = ispec->width - ispec->crop_left - ispec->crop_right;
    ispec->height = ispec->height - ispec->crop_top - ispec->crop_bottom;

    if (mime = gst_structure_get_name (structure)) {
      vpuenc->context.openparam.nChromaInterleave = 0;
      vpuenc->context.openparam.nMapType = 0;
      vpuenc->context.openparam.nLinear2TiledEnable = 0;
      if (!strcmp (mime, VIDEO_RAW_CAPS_YUV)) {
        if (gst_structure_get_fourcc (structure, "format", &u32value)) {
          if (u32value == GST_STR_FOURCC ("NV12")) {
            vpuenc->context.openparam.nChromaInterleave = 1;
          } else if (u32value == GST_STR_FOURCC ("TNVP")) {
            ispec->tiled = TRUE;
            vpuenc->context.openparam.nChromaInterleave = 1;
            vpuenc->context.openparam.nMapType = 1;
          } else if (u32value != GST_STR_FOURCC ("I420")) {
            goto error;
          }
        } else {
          goto error;
        }
      } else {
        goto error;
      }
    } else {
      goto error;
    }


    if (gst_vpuenc_check_and_adjust_input (vpuenc, ispec) == FALSE) {
      goto error;
    }

    vpuenc->context.openparam.nPicWidth = ispec->width;
    vpuenc->context.openparam.nPicHeight = ispec->height;

    ispec->pad_frame_size =
        Align ((ispec->width + ispec->crop_left + ispec->crop_right)
        * (ispec->height + ispec->crop_top + ispec->crop_bottom),
        ispec->buffer_align) + 2 * Align (((ispec->width + ispec->crop_left +
                ispec->crop_right)
            * (ispec->height + ispec->crop_top + ispec->crop_bottom)) / 4,
        ispec->buffer_align);

    if ((!vpuenc->options.force_framerate)
        && (gst_structure_get_fraction (structure, "framerate", &intvalue,
                &intvalue1))) {
      GST_INFO ("Setframerate %d/%d", intvalue, intvalue1);
      if ((intvalue > 0) && (intvalue1 > 0)) {
        vpuenc->options.framerate_nu = intvalue;
        vpuenc->options.framerate_de = intvalue1;
      }
    }

    vpuenc->frame_interval =
        gst_util_uint64_scale_int (GST_SECOND, vpuenc->options.framerate_de,
                vpuenc->options.framerate_nu);
  } else {
    GST_ERROR ("Change support after encoding start not support");
    return FALSE;
  }

  return ret;
error:
  return FALSE;
}


static void
gst_vpuenc_update_parameters (GstVpuEnc * vpuenc)
{
  vpuenc->context.params.eFormat = vpuenc->context.openparam.eFormat;
  vpuenc->context.params.nPicWidth =
      vpuenc->ispec.width + vpuenc->ispec.crop_left + vpuenc->ispec.crop_right;
  vpuenc->context.params.nPicHeight =
      vpuenc->ispec.height + vpuenc->ispec.crop_top + vpuenc->ispec.crop_bottom;
  vpuenc->context.params.nFrameRate = vpuenc->context.openparam.nFrameRate;
  vpuenc->context.params.nQuantParam = vpuenc->options.quant;
  vpuenc->context.params.nInInputSize = vpuenc->ispec.frame_size;

  /* FIX ME : H264 GOP Setting */
  if (vpuenc->context.params.eFormat == VPU_V_AVC) {
  if (stats_sn) {
#if _USE_SEMAPHORE_
	sops[0].sem_op=0; sops[1].sem_op=1;
	semop(semid_snd,sops,2);
#endif
#if _BITRATE_VIA_QP_
	if (stats_sn->enc.Qp< H264_QP_MAX &&
		 stats_sn->enc.Qp>H264_QP_MIN) {
  GST_ERROR ("VPU_ENC: Set nQuantParam"); // for debug
    	vpuenc->context.params.nQuantParam = stats_sn->enc.Qp;
 	}
#endif
#if _USE_SEMAPHORE_
	sops[0].sem_op=-1;
	semop(semid_snd,sops,1);
#endif
  }
  } else {

  }

  vpuenc->context.params.nSkipPicture = 0;
  vpuenc->context.params.nEnableAutoSkip = 0;
}


static GstFlowReturn
gst_vpuenc_push_buffer (GstVpuEnc * vpuenc, GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_ERROR;
  if (G_UNLIKELY (vpuenc->downstream_caps_set == FALSE)) {
    GstCaps *caps;
    const gchar *mime = vpuenc_find_mime (vpuenc->context.openparam.eFormat);
    if (mime) {
      caps = gst_caps_from_string (mime);
      if (caps == NULL) {
        goto bail;
      }
      gst_caps_set_simple (caps, "width", G_TYPE_INT, vpuenc->ispec.width,
          "height", G_TYPE_INT, vpuenc->ispec.height,
          "framerate", GST_TYPE_FRACTION, vpuenc->options.framerate_nu,
          vpuenc->options.framerate_de, "framed", G_TYPE_BOOLEAN, TRUE, NULL);

      if(vpuenc->context.openparam.nIsAvcc){ //support third-party muxer: includes mp4mux/flvmux/matroskamux
          GST_LOG("set stream-format: avc ");
          gst_caps_set_simple (caps, "stream-format", G_TYPE_STRING, "avc",
                        "alignment", G_TYPE_STRING, "au", NULL);			
      }

      if (vpuenc->codec_data) {
        gboolean sendwithbuffer = FALSE;
        if (vpuenc->options.seqheader_method == 0) {
          if ((vpuenc->context.params.eFormat == VPU_V_AVC)&&(FALSE==vpuenc->context.openparam.nIsAvcc)) {
            sendwithbuffer = TRUE;
          }
        } else if ((vpuenc->options.seqheader_method == 2)
            || (vpuenc->options.seqheader_method == 3)) {
          sendwithbuffer = TRUE;
        }
        /* FIXME: should allocate len+codec_data_len to avoid memory copy */
        if (sendwithbuffer) {
          buffer =
              gst_buffer_join (gst_buffer_ref (vpuenc->codec_data), buffer);
        } else {
          gst_caps_set_simple (caps, "codec_data", GST_TYPE_BUFFER,
              vpuenc->codec_data, NULL);
        }

      }

      gst_pad_set_caps (vpuenc->srcpad, caps);
      GST_INFO ("set downstream caps %" GST_PTR_FORMAT, caps);
      gst_caps_unref (caps);
      vpuenc->downstream_caps_set = TRUE;
    } else {
      GST_ERROR ("Can not find std %d!!", vpuenc->context.params.eFormat);
      goto bail;
    }
  } else if ((vpuenc->options.seqheader_method == 3) && (vpuenc->codec_data)) {
    buffer = gst_buffer_join (gst_buffer_ref (vpuenc->codec_data), buffer);
  }
  gst_buffer_set_caps (buffer, GST_PAD_CAPS (vpuenc->srcpad));
  TSM_TIMESTAMP ts = TSManagerSend (vpuenc->tsm);
  if (vpuenc->options.timestamp_method == 0) {
    GST_BUFFER_TIMESTAMP (buffer) = ts;
  } else if (vpuenc->options.timestamp_method == 1) {
    GST_BUFFER_TIMESTAMP (buffer) = GST_CLOCK_TIME_NONE;
  }

  GST_BUFFER_DURATION (buffer) = vpuenc->frame_interval;

  ret = gst_pad_push (vpuenc->srcpad, buffer);
  buffer = NULL;

bail:
  if (buffer) {
    gst_buffer_unref (buffer);
  }
  return ret;
}

static void
gst_vpuenc_assign_frame (VpuInputSpec * ispec, VpuFrameBuffer * frame,
    void *paddr, void *vaddr)
{
  gint offset, pad_ysize, strideh;

  frame->nStrideY = ispec->width + ispec->crop_left + ispec->crop_right;
  strideh = ispec->height + ispec->crop_top + ispec->crop_bottom;
  pad_ysize = Align (frame->nStrideY * strideh, ispec->buffer_align);
  frame->nStrideC = frame->nStrideY / 2;

  offset = frame->nStrideY * ispec->crop_top + ispec->crop_left;
  frame->pbufY = (unsigned char *) paddr + offset;
  frame->pbufVirtY = (unsigned char *) vaddr + offset;

  offset = pad_ysize + frame->nStrideC * ispec->crop_top / 2
      + ispec->crop_left / 2;
  frame->pbufCb = (unsigned char *) paddr + offset;
  frame->pbufVirtCb = (unsigned char *) vaddr + offset;

  offset = Align (pad_ysize / 4, ispec->buffer_align);
  frame->pbufCr = frame->pbufCb + offset;
  frame->pbufVirtCr = frame->pbufVirtCb + offset;

}

static GstBuffer *
gst_vpuenc_preencode_process (GstVpuEnc * vpuenc, GstBuffer * buffer)
{
  GstBuffer *ret = buffer;
  if (buffer) {
    vpuenc->vpu_stat.in_cnt++;
    if ((vpuenc->options.force_framerate)
        && (GST_BUFFER_TIMESTAMP_IS_VALID (buffer))) {
      GstClockTime expect_ts =
          vpuenc->segment_start + (vpuenc->frame_cnt +
          1) * vpuenc->frame_interval;
      if (GST_BUFFER_TIMESTAMP (buffer) < expect_ts) {
        GST_DEBUG ("Drop frame since force framerate control");
        gst_buffer_unref (buffer);
        ret = NULL;
        goto bail;
      }
    }
    vpuenc->frame_cnt++;
    if (stats_sn) {
    	stats_sn->enc.frame_cnt = vpuenc->frame_cnt; // a way to monitor gst-activity
      }
  }

bail:
  return ret;
}

static void
gst_vpuenc_copy_frame (GstVpuEnc * vpuenc, GstBuffer * buffer, void *vaddr)
{
  memcpy ((void *) (vaddr), GST_BUFFER_DATA (buffer), GST_BUFFER_SIZE (buffer));

}

static GstFlowReturn
gst_vpuenc_chain (GstPad * pad, GstBuffer * buffer)
{
  GstVpuEnc *vpuenc;
  GstFlowReturn ret = GST_FLOW_UNEXPECTED;
  VpuEncRetCode core_ret;
  static int sn_bitrate=0;
  static int bradjcnt=0;

  vpuenc = GST_VPUENC (GST_PAD_PARENT (pad));

  if (G_UNLIKELY (vpuenc->init == FALSE)) {
    if ((vpuenc->context.openparam.nPicWidth == 0)
        || (vpuenc->context.openparam.nPicHeight == 0)) {
      GST_ERROR ("Can not get resolution information!!");
      goto bail;
    }

/////////////////////////////////// bookmark ////////////////////////////////////////
  if (stats_sn) {
#if _USE_SEMAPHORE_
		sops[0].sem_op=0; sops[1].sem_op=1;
		semop(semid_snd,sops,2);
#endif
  GST_ERROR ("VPU_ENC: Set nInitialDelay");
		vpuenc->context.openparam.nInitialDelay = stats_sn->initialDelay;
  GST_ERROR ("VPU_ENC: Set nVbvBufferSize");
		vpuenc->context.openparam.nVbvBufferSize = stats_sn->vbvBuffSize;
#if _USE_SEMAPHORE_
		sops[0].sem_op=-1;
		semop(semid_snd,sops,1);
#endif
#if _BITRATE_VIA_QP_
    if (stats_sn->enc.intraQp< H264_QP_MAX &&
    		stats_sn->enc.intraQp>H264_QP_MIN) {
  GST_ERROR ("VPU_ENC: Set nRcIntraQp");
      vpuenc->context.openparam.nRcIntraQp = stats_sn->enc.intraQp;
    }
#endif
  }
///////////////////////////////////////////////////////////////////////////////////////////

    vpuenc->context.openparam.eFormat = vpuenc->options.codec;
    vpuenc->context.openparam.nGOPSize = vpuenc->options.gopsize;

    vpuenc->context.openparam.nFrameRate =
        vpuenc->options.framerate_nu / vpuenc->options.framerate_de;

    /*assign bitrate*/
    if(vpuenc->options.cbr){
        if(0==vpuenc->options.bitrate){
            vpuenc->context.openparam.nBitRate=VPUENC_DEFAULT_BITRARE_KBPS(vpuenc->context.openparam.nPicWidth,
                          vpuenc->context.openparam.nPicHeight,vpuenc->context.openparam.nFrameRate);
        }
        else{
            vpuenc->context.openparam.nBitRate = (int)vpuenc->options.bitrate/1000;
        }
    }
    else{
        vpuenc->context.openparam.nBitRate=0; //ignore bitrate
    }
    GST_INFO("cbr: %d, bitrate: %lld(bps) , bitrate set to vpu: %d(kbps) \r\n",vpuenc->options.cbr,vpuenc->options.bitrate,vpuenc->context.openparam.nBitRate);

    /*assign qp value*/
    if(-1==vpuenc->options.quant){
        if(VPU_V_AVC==vpuenc->options.codec){
            vpuenc->options.quant=35;
        }
        else{
            vpuenc->options.quant=15;
        }
    }
    GST_INFO("qp value: %d ",vpuenc->options.quant);

    vpuenc_detect_avcc(vpuenc);

    CORE_API (VPU_EncOpen, goto bail, core_ret, &vpuenc->context.handle,
        &vpuenc->context.meminfo, &vpuenc->context.openparam);
   if (!stats_sn) {
    CORE_API (VPU_EncConfig, goto bail, core_ret, vpuenc->context.handle,
        VPU_ENC_CONF_NONE, NULL); }
   else {
    CORE_API (VPU_EncConfig, goto bail, core_ret, vpuenc->context.handle,
        VPU_ENC_CONF_NONE, (void*)stats_sn);
   }

    CORE_API (VPU_EncGetInitialInfo, goto bail, core_ret,
        vpuenc->context.handle, &vpuenc->context.initinfo);
  if (stats_sn) {
		stats_sn->enc.numframebufs = vpuenc->context.initinfo.nMinFrameBufferCount;
		GST_ERROR ("VPU_ENC: numframebufs = %d",stats_sn->enc.numframebufs);
	}
    GST_INFO ("get vpuenc require %d frames.",
        vpuenc->context.initinfo.nMinFrameBufferCount);
    if (!gst_vpuenc_core_create_and_register_frames (vpuenc,
            vpuenc->context.initinfo.nMinFrameBufferCount)) {
      goto bail;
    }

    if ((vpuenc->obuf = vpuenc_core_mem_alloc_dma_buffer (DEFAULT_BITSTREAM_BUFFER_LENGTH)) == NULL) {
      goto bail;
    }
    ATTACH_MEM2VPUDEC (vpuenc, vpuenc->obuf);

    setTSManagerFrameRate (vpuenc->tsm, vpuenc->options.framerate_nu,
        vpuenc->options.framerate_de);

    vpuenc->init = TRUE;

  }

  buffer = gst_vpuenc_preencode_process (vpuenc, buffer);

  if (buffer) {
    void *paddr, *vaddr;
    VpuFrameBuffer frame = { 0 };
    if (GST_BUFFER_SIZE (buffer) != vpuenc->ispec.pad_frame_size) {
      GST_ERROR ("Buffer size is not the same framesize of %d",
          vpuenc->ispec.frame_size);
      goto bail;
    }
    GST_LOG ("chain in with inbuffer size = %d ts %" GST_TIME_FORMAT,
        GST_BUFFER_SIZE (buffer),
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

    TSManagerReceive (vpuenc->tsm, GST_BUFFER_TIMESTAMP (buffer));

    if ((IS_DMABLE_BUFFER (buffer)) && (vpuenc->force_copy == FALSE)) {
      paddr = DMABLE_BUFFER_PHY_ADDR (buffer);
      vaddr = GST_BUFFER_DATA (buffer);
    } else {
      GST_ERROR ("Need memcpy input buffer, performance maybe drop");
      if (!vpuenc->frameblock) {
          if ((vpuenc->frameblock =
                      vpuenc_core_mem_alloc_dma_buffer (vpuenc->ispec.pad_frame_size)) == NULL) {
              GST_ERROR ("Can not create dmaable buffer for input copy");
              goto bail;
          }
      }
      paddr = (void *) Align (vpuenc->frameblock->paddr, vpuenc->ispec.buffer_align);
      vaddr = (void *) Align (vpuenc->frameblock->vaddr, vpuenc->ispec.buffer_align);
      gst_vpuenc_copy_frame (vpuenc, buffer, vaddr);
    }

//////////////////////////////////////////////////////////////////////////////////////////
   //frame duplication routine
   /*
   static int framedup_cnt = 0;
   static void * paddr_curr;
   static void * vaddr_curr;
   
   framedup_cnt++; if(framedup_cnt==3) framedup_cnt=0;
   if(framedup_cnt>1) {
   GST_ERROR ("VPU_ENC: duplicate input frame %d %x %x",framedup_cnt,paddr,vaddr); 
   }else
   {
     paddr_curr = paddr;
     vaddr_curr = vaddr;
     //GST_ERROR ("VPU_ENC: no duplicate input frame %d",framedup_cnt); 
   }
   paddr=paddr_curr;
   vaddr=vaddr_curr;
   */
//////////////////////////////////////////////////////////////////////////////////////////

    memset (&vpuenc->context.params, 0, sizeof (VpuEncEncParam));
    gst_vpuenc_update_parameters (vpuenc);
    gst_vpuenc_assign_frame (&vpuenc->ispec, &frame, paddr, vaddr);
    vpuenc->context.params.pInFrame = &frame;

    if(vpuenc->forcekeyframe){
      GST_INFO("one key frame is forced .");
      gst_pad_push_event (vpuenc->srcpad,
          gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM, 
            gst_structure_new ("GstForceKeyUnit","timestamp", G_TYPE_UINT64, TSManagerQuery(vpuenc->tsm),NULL)));

      vpuenc->context.params.nForceIPicture= 1;
      vpuenc->gop_frm_cnt=0;
      GST_OBJECT_LOCK (vpuenc);
      vpuenc->forcekeyframe = FALSE;
      GST_OBJECT_UNLOCK (vpuenc);
    }
    else{
      vpuenc->context.params.nForceIPicture=0;
    }

    //GST_ERROR ("VPU_ENC: gotframe %d",vpuenc->frame_cnt);
        /**/ //Insert sps/pps
	 if (!(63 & vpuenc->frame_cnt)) {
         //GST_ERROR ("VPU_ENC: Enable SPS/PPS"); 
	  //VPU_RequireHeader(vpuenc->context.handle);
	}

     
    do {
      vpuenc->context.params.eOutRetCode = 0;
      vpuenc->context.params.nInPhyOutput = (unsigned int) vpuenc->obuf->paddr;
      vpuenc->context.params.nInVirtOutput = (unsigned int) vpuenc->obuf->vaddr;
      vpuenc->context.params.nInOutputBufLen = vpuenc->obuf->size;
    if (stats_sn) stats_sn->enc.encoding = 1;	// perform encoding...
      CORE_API (VPU_EncEncodeFrame, {
            if (core_ret == VPU_DEC_RET_FAILURE_TIMEOUT)
    if (stats_sn) stats_sn->enc.encoding = 0;	// encoding failed & reset...
          CORE_API (VPU_EncReset,, core_ret, vpuenc->context.handle);
            goto bail;}
          , core_ret, vpuenc->context.handle, &vpuenc->context.params);
    if (stats_sn) stats_sn->enc.encoding = 0;	// encoding failed & reset...
      GST_LOG ("vpuenc output return 0x%x", vpuenc->context.params.eOutRetCode);
      if (vpuenc->context.params.eOutRetCode & VPU_ENC_OUTPUT_SEQHEADER) {
        if ((vpuenc->codec_data == NULL)
            && (vpuenc->context.params.nOutOutputSize)) {
          vpuenc->codec_data =
              gst_buffer_new_and_alloc (vpuenc->context.params.nOutOutputSize);
          memcpy (GST_BUFFER_DATA (vpuenc->codec_data),
              (void *) (vpuenc->obuf->vaddr),
              vpuenc->context.params.nOutOutputSize);
          GST_INFO ("got codec data %d bytes %" GST_PTR_FORMAT,
              vpuenc->context.params.nOutOutputSize, vpuenc->codec_data);
        }
      } else if (vpuenc->context.params.eOutRetCode & VPU_ENC_OUTPUT_DIS) {
        GstBuffer *gstbuf;
        MFW_WEAK_ASSERT (vpuenc->context.params.nOutOutputSize <=
            vpuenc->obuf->size);
        GST_LOG ("got compressed frame %d bytes",
            vpuenc->context.params.nOutOutputSize);
        if ((ret =
                gst_pad_alloc_buffer (vpuenc->srcpad, 0,
                    vpuenc->context.params.nOutOutputSize, NULL,
                    &gstbuf)) != GST_FLOW_OK) {
          GST_INFO ("Create output buffer failed , ret = %d", ret);
          goto bail;
        }
        /* FIX ME : currently vpu wrapper still need copy since 6q does not dynamic output buffer */
        memcpy (GST_BUFFER_DATA (gstbuf), (void *) (vpuenc->obuf->vaddr),
            vpuenc->context.params.nOutOutputSize);
        if(vpuenc->options.gopsize !=0)
        { if ( vpuenc->gop_frm_cnt % vpuenc->options.gopsize) 
              GST_BUFFER_FLAG_SET (gstbuf, GST_BUFFER_FLAG_DELTA_UNIT); 
        }
        else if(vpuenc->gop_frm_cnt ==0)
        { 
              GST_BUFFER_FLAG_SET (gstbuf, GST_BUFFER_FLAG_DELTA_UNIT); 
        }
        
        if(vpuenc->gop_frm_cnt < 10000) { 
        vpuenc->gop_frm_cnt++; 
        vpuenc->vpu_stat.out_cnt++; }
          
        ret = gst_vpuenc_push_buffer (vpuenc, gstbuf);

      } else {
        GST_INFO ("Got no output.");
      }

    } while (!(vpuenc->context.params.eOutRetCode & VPU_ENC_INPUT_USED));

  }

  ret = GST_FLOW_OK;

bail:
  {
    if (buffer) {
      gst_buffer_unref (buffer);
    }
    return ret;
  }
}



static GstStateChangeReturn
gst_vpuenc_state_change (GstElement * element, GstStateChange transition)
{
  GstVpuEnc *vpuenc;
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
  vpuenc = GST_VPUENC (element);
  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    {
      MM_INIT_DBG_MEM ("vpuenc");
      if (!vpuenc_core_init (vpuenc)) {
        goto bail;
      }
      break;
    }
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
      vpuenc_core_deinit (vpuenc);
      MM_DEINIT_DBG_MEM ();
      break;
    }
    default:
      break;
  }
bail:
  return ret;
}





static gboolean
gst_vpuenc_src_event (GstPad * pad, GstEvent * event)
{

  gboolean ret = TRUE;
  GstVpuEnc *vpuenc;
  vpuenc = GST_VPUENC (GST_PAD_PARENT (pad));
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:{
      const GstStructure *s;
      GST_INFO ("Got GST_EVENT_CUSTOM_UPSTREAM src event.");
      s = gst_event_get_structure (event);
      if (gst_structure_has_name (s, "GstForceKeyUnit")) {
        GST_OBJECT_LOCK (vpuenc);
        vpuenc->forcekeyframe= TRUE;
        GST_OBJECT_UNLOCK (vpuenc);
        gst_event_unref (event);
      }
      break;
    }
    default:
      ret = gst_pad_event_default (pad, event);
      break;
  }
  return ret;
bail:
  if (event) {
    gst_event_unref (event);
  }
  return FALSE;
}


static gboolean
gst_vpuenc_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean ret = TRUE;
  GstVpuEnc *vpuenc;
  vpuenc = GST_VPUENC (GST_PAD_PARENT (pad));
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NEWSEGMENT:
    {
      GstFormat format;
      gint64 start, stop, position;
      gint64 nstart, nstop;
      GstEvent *nevent;
      gdouble rate;
      gst_event_parse_new_segment (event, NULL, &rate, &format, &start,
          &stop, &position);
      if (format == GST_FORMAT_TIME) {

        if ((rate <= 2.0) && (rate >= 0.0)) {
          vpuenc->tsm_mode = MODE_AI;
        } else {
          vpuenc->tsm_mode = MODE_FIFO;
        }
        vpuenc->new_segment = TRUE;
        vpuenc->segment_start = start;
        vpuenc->frame_cnt = 0;
        resyncTSManager (vpuenc->tsm, vpuenc->segment_start, vpuenc->tsm_mode);
        GST_INFO ("Get newsegment event from %" GST_TIME_FORMAT "to %"
            GST_TIME_FORMAT " pos %" GST_TIME_FORMAT, GST_TIME_ARGS (start),
            GST_TIME_ARGS (stop), GST_TIME_ARGS (position));
      } else {
        GST_WARNING ("Unsupport newsegment format %d", format);
        goto bail;
      }
      ret = gst_pad_event_default (pad, event);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      VpuEncRetCode core_ret;
      ret = gst_pad_event_default (pad, event);
      break;
    }
    case GST_EVENT_EOS:
    {
      GST_INFO ("EOS received");
      ret = gst_pad_event_default (pad, event);
      break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM:{
      const GstStructure *s;
      GST_INFO ("Got GST_EVENT_CUSTOM_DOWNSTREAM sink event.");
      s = gst_event_get_structure (event);
      if (gst_structure_has_name (s, "GstForceKeyUnit")) {
        GST_OBJECT_LOCK (vpuenc);
        vpuenc->forcekeyframe= TRUE;
        GST_OBJECT_UNLOCK (vpuenc);
      }
      gst_pad_event_default (pad, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, event);
      break;
  }

  return ret;
bail:
  if (event) {
    gst_event_unref (event);
  }
  return FALSE;
}


static const GstQueryType *
gst_vpuenc_query_types (GstPad * pad)
{
  static const GstQueryType src_query_types[] = {
    GST_QUERY_DURATION, 0
  };
  return src_query_types;
}

static gboolean
gst_vpuenc_src_query (GstPad * pad, GstQuery * query)
{

  gboolean ret = FALSE;
  GstVpuEnc *vpuenc;
  vpuenc = GST_VPUENC (GST_PAD_PARENT (pad));
  switch (GST_QUERY_TYPE (query)) {

    default:
      goto bail;
      break;
  }
bail:
  return ret;
}
