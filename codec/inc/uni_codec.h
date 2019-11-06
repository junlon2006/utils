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
 * Description : uni_codec.h
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#ifndef CODEC_INC_UNI_CODEC_H_
#define CODEC_INC_UNI_CODEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UNI_ADPCM_CODEC_MODULE
enum {
  ENCODE_COOEFF_ADPCM = 4,
};

#define COMPRESS_RATIO   ENCODE_COOEFF_ADPCM  /*adpcm ratio,if change other,please change this MACRO*/

int uni_coder_init(int type);
int uni_coder_data(int type, char *inbuf, unisigned int inlen, char **outbuf, int *olen);
int uni_decode_init(int type);
int uni_decode(int type, char *in, int len, char **out, int *olen);

#ifdef __cplusplus
}
#endif
#endif // CODEC_INC_UNI_CODEC_H_
