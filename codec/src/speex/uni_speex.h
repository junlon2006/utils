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
 * Description : uni_speex.h
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef UTILS_CODEC_SRC_SPEEX_UNI_SPEEX_H_
#define UTILS_CODEC_SRC_SPEEX_UNI_SPEEX_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"
#include "speex.h"

enum{
    UNI_SPEEX_FRAME_LEN = 320, /*20ms samples, @16KHz, mono */
};

struct speex_info_t{
    int       frame_len;  /*20ms samples*/

    /*encoder*/
    SpeexBits enc_bits;
    void*     enc_state;
    char*     enc_buf;
    int       enc_size;
    int       sample_rate;
    int       quality;
    int       comp;

    /*decoder*/
    SpeexBits dec_bits;
    void*     dec_state;
    short*    dec_buf;
    int       dec_size;
    int       enh; /*enhancement*/
};

int uni_init_speex_encode(void);    
int uni_init_speex_decode(void);

void uni_init_speex(void);

int uni_uninit_speex_encode(void);
int uni_uninit_speex_decode(void);

void uni_uninit_speex(void);

int uni_speex_encode(short *ibuf, int frame_size, char **obuf , int *olen);
int uni_speex_decode(char *inbuf, int nbBytes, short **outbuf, int *outlen);

#ifdef __cplusplus
}
#endif
#endif 
