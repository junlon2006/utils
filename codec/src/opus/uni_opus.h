/* Copyright (c) 2013 Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CODEC_SRC_OPUS_UNI_OPUS_H_
#define CODEC_SRC_OPUS_UNI_OPUS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This is meant to be a simple example of encoding and decoding audio
   using Opus. It should make it easy to understand how the Opus API
   works. For more information, see the full API documentation at:
   https://www.opus-codec.org/docs/ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "opus.h"

struct opus_info_t {
  int frame_size;
  int sample_rate;
  int channels;
  int application;
  int bitrate;
  int max_frame_size;
  int max_packet_size;
  OpusDecoder *decoder;
  OpusEncoder *encoder;
  opus_int16 *out;
  unsigned char *in;
};

int uni_init_opus_encode(void);
int uni_init_opus_decode(void);
void uni_init_opus(void);
int uni_uninit_opus_encode(void);
int uni_uninit_opus_decode(void);
void uni_uninit_opus(void);
int uni_opus_decode(unsigned char *inbuf, int nbBytes, opus_int16 **outbuf, int *outlen);
int uni_opus_encode(opus_int16 *ibuf, int frame_size, unsigned char **obuf , int *olen);

#ifdef __cplusplus
}
#endif
#endif  //  CODEC_SRC_OPUS_UNI_OPUS_H_
