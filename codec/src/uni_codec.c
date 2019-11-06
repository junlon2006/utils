/**************************************************************************
 * Copyright (C) 2017-2017  junlon2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_codec.c
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_log.h"
#include "uni_codec.h"
#include "adpcm/uni_adpcm.h"

#ifdef UNI_OPUS_CODEC_MODULE
#include "opus/uni_opus.h"
#endif

#ifdef UNI_SPEEX_CODEC_MODULE
#include "speex/uni_speex.h"
#endif

#ifdef UNI_ADPCM_CODEC_MODULE
static adpcm_state_t adp_status;
static adpcm_state_t decoder_status;
#endif

#define CODEC_OK   0
#define CODEC_ERR  1
#define CODEC_DEBUG	\
  uni_printf("%s[%d]:", __func__, __LINE__);	\
uni_printf

/**********************************************************
 * static function declear
 * *******************************************************/
static int _adpcm_encoder_init(void);
static int _adpcm_encoder(char *in, unisigned int inLen, char **out, unisigned int *olen);
static int _adpcm_decoder_init(void);
static int _adpcm_decoder(char *in, unisigned int inLen, char **out, unisigned int *olen);
static int _opus_encoder_init(void);
static int _opus_encoder(char *in, unisigned int inLen, char **out, unisigned int *olen);
static int _opus_decoder_init(void);
static int _opus_decoder(char *in, unisigned int inLen, char **out, unisigned int *olen);
static int _speex_encoder_init(void);
static int _speex_encoder(char *in, unisigned int inLen, char **out, unisigned int *olen);
static int _speex_decoder_init(void);
static int _speex_decoder(char *in, unisigned int inLen, char **out, unisigned int *olen);

typedef int (*CODEC_INIT_FUNC)(void);
typedef int (*CODEC_WORK_FUNC)(char *in, unisigned int inLen, char **out, unisigned int *olen);

/**********************************************************
 * Name:codec_tlb_t
 * Description:support adpcm/opus/speex now
 * *******************************************************/
typedef struct {
  int iType;
  CODEC_INIT_FUNC pEncoderInit;
  CODEC_WORK_FUNC pEncoder;
  CODEC_INIT_FUNC pDecoderInit;
  CODEC_WORK_FUNC pDecoder;
} codec_tlb_t;

/**********************************************************
 * Name:g_codec_tlb_list
 * Description:codec tlb list
 * *******************************************************/
static codec_tlb_t g_codec_tlb_list[] = {
  {2, _adpcm_encoder_init, _adpcm_encoder, _adpcm_decoder_init, _adpcm_decoder},
  {3, _opus_encoder_init, _opus_encoder, _opus_decoder_init, _opus_decoder},
  {4, _speex_encoder_init, _speex_encoder, _speex_decoder_init, _speex_decoder}
};

/**********************************************************
 * Name:GET_CODEC_TLB_PTR
 * Description:
 * *******************************************************/
#define GET_CODEC_TLB_PTR(ptr, type)\
  do \
{\
  int _loop = 0; \
  for (_loop = 0; _loop < (sizeof(g_codec_tlb_list) / sizeof(g_codec_tlb_list[0])); _loop++){ \
    if (g_codec_tlb_list[_loop].iType == (type)) {\
      ptr = &g_codec_tlb_list[_loop]; \
    } \
  } \
}while(0)

#define __STATIC_FUNC_START__
static int _adpcm_encoder_init(void) {
#ifdef UNI_ADPCM_CODEC_MODULE
  uni_memset(&adp_status,0,sizeof(adpcm_state_t));
  return CODEC_OK;
#else
  CODEC_DEBUG("adpcm encoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _adpcm_encoder(char *in, unisigned int inLen, char **out,
                          unisigned int *olen) {
#ifdef UNI_ADPCM_CODEC_MODULE
  adpcm_coder((uni_s16 *)in, *out, inLen, &adp_status);
  *olen =  inLen / COMPRESS_RATIO;
  return CODEC_OK;
#else
  CODEC_DEBUG("adpcm encoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _adpcm_decoder_init(void) {
#ifdef UNI_ADPCM_CODEC_MODULE
  uni_memset(&decoder_status, 0, sizeof(adpcm_state_t));
  return CODEC_OK;
#else
  CODEC_DEBUG("adpcm decoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _adpcm_decoder(char *in, unisigned int inLen, char **out,
                          unisigned int *olen) {
#ifdef UNI_ADPCM_CODEC_MODULE
  adpcm_decoder(in, (uni_s16 *)*out, inLen, &decoder_status);
  *olen = inLen * COMPRESS_RATIO;
  return CODEC_OK;
#else
  CODEC_DEBUG("adpcm decoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _opus_encoder_init(void) {
#ifdef UNI_OPUS_CODEC_MODULE
  uni_uninit_opus_encode();
  uni_init_opus_encode();
  return CODEC_OK;
#else
  CODEC_DEBUG("opus encoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _opus_encoder(char *in, unisigned int inLen, char **out,
                         unisigned int *olen) {
#ifdef UNI_OPUS_CODEC_MODULE
  uni_opus_encode((opus_int16 *)in, inLen / sizeof(opus_int16), (uni_u8 **)out,
                  olen);
  return CODEC_OK;
#else
  CODEC_DEBUG("opus encoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _opus_decoder_init(void) {
#ifdef UNI_OPUS_CODEC_MODULE
  uni_uninit_opus_decode();
  uni_init_opus_decode();
  return CODEC_OK;
#else
  CODEC_DEBUG("opus decoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _opus_decoder(char *in, unisigned int inLen, char **out,
                         unisigned int *olen) {
#ifdef UNI_OPUS_CODEC_MODULE
  uni_opus_decode((unsigned char *)in, inLen, (opus_int16**)out, olen);
  *olen *= sizeof(opus_int16);
  return CODEC_OK;
#else
  CODEC_DEBUG("opus decoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _speex_encoder_init(void) {
#ifdef UNI_SPEEX_CODEC_MODULE
  uni_uninit_speex_encode();
  uni_init_speex_encode();
  return CODEC_OK;
#else
  CODEC_DEBUG("speex encoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

//#define UNI_ENFORCEMENT_RATE_TEST		1
static int _speex_encoder(char *in, unisigned int inlen, char **out,
                          unisigned int *olen) {
#ifdef UNI_SPEEX_CODEC_MODULE
  /*实施率测试*/
#ifdef UNI_ENFORCEMENT_RATE_TEST
  unisigned int time_basic;
  time_basic = uni_times_ms();
  uni_printf("\ntimes_ms():[%d]\n", time_basic);
#endif
  uni_speex_encode((uni_s16 *)in, inlen / sizeof(uni_s16), (char **)out, (int *)olen);

#ifdef UNI_ENFORCEMENT_RATE_TEST
  if (*olen > 0) {
    uni_printf("len[%d],speex_encode_cost[%d] ", *olen, uni_uptime_ms() - time_basic);
  }
#endif
  return CODEC_OK;
#else
  CODEC_DEBUG("speex encoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _speex_decoder_init(void) {
#ifdef UNI_SPEEX_CODEC_MODULE
  uni_uninit_speex_decode();
  uni_init_speex_decode();
  return CODEC_OK;
#else
  CODEC_DEBUG("speex decoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

static int _speex_decoder(char *in, unisigned int inLen, char **out,
                          unisigned int *olen) {
#ifdef UNI_SPEEX_CODEC_MODULE
  uni_speex_decode((char *)in, inLen, (uni_s16 **)out, (int *)olen);
  *olen *= sizeof(uni_s16);
  return CODEC_OK;
#else
  CODEC_DEBUG("speex decoder is not support in this system!\n");
  return CODEC_ERR;
#endif
}

#define __INTERFACE_FUNC_START__
int uni_coder_init(int type) {
  codec_tlb_t *pCodec = NULL;
  GET_CODEC_TLB_PTR(pCodec, type);
  if (NULL == pCodec) {
    LOGW("codec", "codec type=%d.", type);
    return CODEC_ERR;
  }

  if (NULL == pCodec->pEncoderInit) {
    LOGW("codec", "pEncoderInit is null[%d].", pCodec->iType);
    return CODEC_ERR;
  }

  pCodec->pEncoderInit();
  return CODEC_OK;
}

/*
 * @inlen: bytes
 * @olen : bytes
 * */
int uni_coder_data(int type, char *in,unisigned int inlen,
                   char **out, int *olen) {
  codec_tlb_t *pCodec = NULL;

  GET_CODEC_TLB_PTR(pCodec, type);
  if (NULL == pCodec) {
    LOGW("codec", "codec type=%d.", type);
    return CODEC_ERR;
  }

  if (NULL == pCodec->pEncoder) {
    LOGW("codec", "pEncoder is null[%d].", pCodec->iType);
    return CODEC_ERR;
  }

  pCodec->pEncoder(in, inlen, out, (unisigned int *)olen);
  return CODEC_OK;
}

int uni_decode_init(int type) {
  codec_tlb_t *pCodec = NULL;

  GET_CODEC_TLB_PTR(pCodec, type);
  if (NULL == pCodec) {
    LOGW("codec", "codec type=%d.", type);
    return CODEC_ERR;
  }

  if (NULL == pCodec->pDecoderInit) {
    LOGW("codec", "pDecoderInit is null[%d].", pCodec->iType);
    return CODEC_ERR;
  }

  pCodec->pDecoderInit();
  return CODEC_OK;
}

/*
 * @len:  bytes
 * @olen: bytes
 * */
int uni_decode(int type, char *in, int len ,
               char **out,  int *olen) {
  codec_tlb_t *pCodec = NULL;

  GET_CODEC_TLB_PTR(pCodec, type);
  if (NULL == pCodec) {
    LOGW("codec", "codec type=%d.", type);
    return CODEC_ERR;
  }

  if (NULL == pCodec->pDecoder) {
    LOGW("codec", "pDecoder is null[%d].", pCodec->iType);
    return CODEC_ERR;
  }

  pCodec->pDecoder(in, (unisigned int)len, out, (unisigned int *)olen);
  return CODEC_OK;
}
