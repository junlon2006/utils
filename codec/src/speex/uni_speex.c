/**************************************************************************
 * Copyright (C) 2017-2017  Unisound
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
 * Description : uni_speex.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_speex.h"
#include "uni_log.h"

/*use CBR mode, but transitino bps can be different*/

/*
 * speex 16bit默认支持数据帧长为20ms(640 bytes), 由于vad算法只支持10ms/16ms，且当前capture送上来的数据都是16ms(512bytes)的，所以
 * 取40ms（1280bytes, 640个short)长度来兼容处理各种帧长，送给speex 编码的必须是20ms数据
 * */
#define MAX_FRAME_PCM_LENGTH	640
#define FRAME_LEN_20MS			320

/**********************************************************
 * Name:spx_enc_t
 * Description:keep speex encode 640 bytes data at one time
 * *******************************************************/
typedef struct {
  short pcm_data[MAX_FRAME_PCM_LENGTH];		/*pcm data buf*/
  unsigned int len;							/*currently data length in enc_buf*/
}spx_enc_t;

static spx_enc_t g_spx_enc;

static struct speex_info_t speex_info = {
  .frame_len = UNI_SPEEX_FRAME_LEN,

  /*encoder*/
  .enc_bits = {0,},
  .enc_state = NULL,
  .enc_buf = NULL, /*out bufer*/
  .enc_size = UNI_SPEEX_FRAME_LEN * sizeof(short) / 4,
  .sample_rate = 16000,
  .quality = 4,
  .comp = 1,

  /*decoder*/
  .dec_bits = {0,},
  .dec_state = NULL,
  .dec_buf = NULL, /*out buffer*/
  .dec_size = UNI_SPEEX_FRAME_LEN * sizeof(short),
  .enh = 0,
};

int uni_init_speex_encode(void){
  const SpeexMode *pmode;

  speex_bits_init(&speex_info.enc_bits);
  pmode = speex_lib_get_mode(SPEEX_MODEID_WB);

  /*Create a new encoder state in wb mode*/
  speex_info.enc_state = speex_encoder_init(pmode);
  if (speex_info.enc_state == NULL)
  {        
    fprintf(stderr, "speex encoder create error\n");
    goto error;                                                             
  }                                                                          

  /*Set the quality to 4 (xx kbps)*/
  speex_encoder_ctl(speex_info.enc_state, SPEEX_SET_QUALITY, &speex_info.quality);
  speex_encoder_ctl(speex_info.enc_state, SPEEX_SET_COMPLEXITY, &speex_info.comp);
  speex_encoder_ctl(speex_info.enc_state, SPEEX_SET_SAMPLING_RATE, &speex_info.sample_rate);

#if 0
  int tmp;
  speex_encoder_ctl(speex_info.enc_state, SPEEX_GET_FRAME_SIZE, &tmp);
  printf("speex encode frame size =%d\n", tmp);
  speex_mode_query(pmode, SPEEX_MODE_FRAME_SIZE, &tmp);
  printf("speex encode frame size =%d\n", tmp);
#endif

  memset(&g_spx_enc, 0, sizeof(g_spx_enc));

  speex_info.enc_buf = (char *)calloc(1, speex_info.enc_size);
  if (speex_info.enc_buf == NULL) {
    fprintf(stderr, "malloc speex encoder buffer error\n");
    goto error;                                  
  }

  return 0;                                                             

error:                                                                   
  uni_uninit_speex();                                                  
  return -1; 
} 

int uni_init_speex_decode(void){     
  speex_bits_init(&speex_info.dec_bits);

  const SpeexMode *pmode = speex_lib_get_mode(SPEEX_MODEID_WB);

  /* Create a new decoder state. */
  speex_info.dec_state = speex_decoder_init(pmode);
  if(speex_info.dec_state == NULL){
    fprintf(stderr, "speex decoder create error\n");
    goto error;
  }

  speex_decoder_ctl(speex_info.dec_state, SPEEX_SET_ENH, &speex_info.enh);

  speex_info.dec_buf = (short *)calloc(1, speex_info.dec_size);
  if (speex_info.dec_buf == NULL){
    fprintf(stderr, "speex decoder malloc decode buffer error\n");
    goto error;
  }

  return 0;

error:
  uni_uninit_speex();
  return -1;
}

void uni_init_speex(void){
  uni_init_speex_decode();     
  uni_init_speex_encode();
}

int uni_uninit_speex_encode(void){
  if (speex_info.enc_buf) { 
    free(speex_info.enc_buf);
    speex_info.enc_buf = NULL;
  }

  speex_bits_destroy(&speex_info.enc_bits);

  if (speex_info.enc_state) {
    speex_encoder_destroy(speex_info.enc_state);
    speex_info.enc_state = NULL;
  }

  return 0;
}

int uni_uninit_speex_decode(void){
  if (speex_info.dec_buf) { 
    free(speex_info.dec_buf);
    speex_info.dec_buf = NULL;
  }

  speex_bits_destroy(&(speex_info.dec_bits));
  if (speex_info.dec_state) {
    speex_decoder_destroy(speex_info.dec_state);
    speex_info.dec_state = NULL;
  }

  return 0;
}

void uni_uninit_speex(void){
  uni_uninit_speex_encode();
  uni_uninit_speex_decode();
}

static int _uni_speex_encode_data(short *ibuf, char **obuf, int *olen)
{
  int nbBytes;
  char *head = speex_info.enc_buf;

  memset(speex_info.enc_buf, 0, speex_info.enc_size);

  /* Encode the frame. */
  speex_bits_reset(&speex_info.enc_bits);
  speex_encode_int(speex_info.enc_state, ibuf, &speex_info.enc_bits);
  nbBytes = speex_bits_write(&speex_info.enc_bits, speex_info.enc_buf + 2, speex_info.enc_size - 2);
  if (nbBytes<0)
  {
    LOGE("speex", "speex encode failed");
    return -1;
  }

  /*header*/
  head[0] = nbBytes & 0xff;
  head[1] = nbBytes>>8 & 0xff;    

  *olen = nbBytes + 2;
  *obuf = speex_info.enc_buf;

  return 0;
}

int uni_speex_encode(short *ibuf, int frame_size/*samples*/, char **obuf , int *olen/*bytes*/){
  spx_enc_t *enc_data = &g_spx_enc;
  /********************************************************************************************
   *ibuf数据如果不是640字节整,先存到缓冲里，够640字节的部分再做压缩,如果传入的数据过大，暂不处理，
   *目前没有这种场景。
   *********************************************************************************************/
  if ((MAX_FRAME_PCM_LENGTH - enc_data->len) < frame_size) {
    LOGE("speex", "not enough length for frame_size");
    return -1;
  } 

  memcpy(&enc_data->pcm_data[enc_data->len], (char *)ibuf, frame_size * sizeof(short));

  enc_data->len += frame_size;
  if (enc_data->len >= FRAME_LEN_20MS) {
    _uni_speex_encode_data(enc_data->pcm_data, obuf, olen);	
    enc_data->len -= FRAME_LEN_20MS;
    memcpy(enc_data->pcm_data, &(enc_data->pcm_data[FRAME_LEN_20MS]),
           enc_data->len * sizeof(short));
  } else {
    *olen = 0;
  }

  return 0;
}

int uni_speex_decode(char *inbuf, int nbBytes/* bytes */, short **outbuf, int *outlen/*samples*/){
  int ret = 0;

  memset(speex_info.dec_buf, 0, speex_info.dec_size);

  speex_bits_read_from(&speex_info.dec_bits, inbuf, nbBytes);
  ret = speex_decode_int(speex_info.dec_state, &speex_info.dec_bits, speex_info.dec_buf);
  if (ret != 0)
  {
    fprintf(stderr, "speex decoder failed\n");
    return ret;
  }

  *outlen = speex_info.dec_size / sizeof(short);     /*20ms @ 16KHz, mono*/
  *outbuf = speex_info.dec_buf; 
  return ret;
}


