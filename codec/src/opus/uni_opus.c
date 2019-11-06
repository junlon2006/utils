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

/* This is meant to be a simple example of encoding and decoding audio
   using Opus. It should make it easy to understand how the Opus API
   works. For more information, see the full API documentation at:
   https://www.opus-codec.org/docs/ */

#include "uni_opus.h"

/*The frame size is hardcoded for this sample code but it doesn't have to be*/
struct opus_info_t opus_info = {
  .frame_size = 320,
  .sample_rate = 16000,
  .channels = 1,
  .application = OPUS_APPLICATION_VOIP,
  .bitrate = 14000,
  .max_frame_size = 6*960,
  .max_packet_size = 3*1276,
  .decoder = NULL,
  .encoder = NULL,
  .out = NULL,
  .in = NULL,
};

int uni_init_opus_decode(void) {
  int err;
  /* Create a new decoder state. */
  opus_info.decoder = opus_decoder_create(opus_info.sample_rate,
                                          opus_info.channels, &err);
  if (err < 0) {
    fprintf(stderr, "failed to create decoder: %s\n", opus_strerror(err));
    goto error;
  }

  opus_info.out = calloc(1, opus_info.max_frame_size * opus_info.channels);
  if (opus_info.out == NULL) goto error;

  return 0;

error:
  uni_uninit_opus();
  return -1;
}

int uni_init_opus_encode(void){
  int err;
  /*Create a new encoder state */
  opus_info.encoder = opus_encoder_create(opus_info.sample_rate,
                                          opus_info.channels,
                                          opus_info.application, &err);
  if (err < 0) {
    fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
    goto error;
  }
  /* Set the desired bit-rate. You can also set other parameters if needed.
     The Opus library is designed to have good defaults, so only set
     parameters you know you need. Doing otherwise is likely to result
     in worse quality, but better. */
  err = opus_encoder_ctl(opus_info.encoder, OPUS_SET_BITRATE(opus_info.bitrate));
  if (err < 0) {
    fprintf(stderr, "failed to set bitrate: %s\n", opus_strerror(err));
    goto error;
  }
  opus_info.in = calloc(1, opus_info.max_packet_size);
  if (opus_info.in == NULL) goto error;
  return 0;

error:
  uni_uninit_opus();
  return -1;
}

void uni_init_opus(void){
  uni_init_opus_decode();
  uni_init_opus_encode();
}

int uni_uninit_opus_decode(void){
  if (opus_info.out) {
    free(opus_info.out);
    opus_info.out = NULL;
  }
  if (opus_info.decoder) {
    opus_decoder_destroy(opus_info.decoder);
    opus_info.decoder = NULL;
  }
  return 0;
}

int uni_uninit_opus_encode(void){
  if (opus_info.in) {
    free(opus_info.in);
    opus_info.in = NULL;
  }
  if (opus_info.encoder){
    opus_encoder_destroy(opus_info.encoder);
    opus_info.encoder = NULL;
  }
  return 0;
}

void uni_uninit_opus(void){
  uni_uninit_opus_decode();
  uni_uninit_opus_encode();
}

int uni_opus_decode(unsigned char *inbuf, int nbBytes/*bytes*/,
                    opus_int16 **outbuf, int *outlen/*samples*/) {
  int frame_size;
  memset(opus_info.out, 0, opus_info.max_frame_size * opus_info.channels);
  /* Decode the data. In this example, frame_size will be constant because
     the encoder is using a constant frame size. However, that may not
     be the case for all encoders, so the decoder must always check
     the frame size returned. */
  frame_size = opus_decode(opus_info.decoder, inbuf, nbBytes, opus_info.out,
                           opus_info.max_frame_size, 0);
  if (frame_size < 0) {
    fprintf(stderr, "decoder failed: %s\n", opus_strerror(frame_size));
    return EXIT_FAILURE;
  }
  *outlen = frame_size;
  *outbuf = opus_info.out;
  return 0;
}

int uni_opus_encode(opus_int16 *ibuf, int frame_size/*samples*/,
                    unsigned char **obuf , int *olen/*bytes*/) {
  int nbBytes;
  unsigned char *head = opus_info.in;
  memset(opus_info.in, 0, opus_info.max_packet_size);
  /* Encode the frame. */
  nbBytes = opus_encode(opus_info.encoder, ibuf, frame_size, opus_info.in + 2,
                        opus_info.max_packet_size - 2);
  if (nbBytes < 0) {
    fprintf(stderr, "encode failed: %s\n", opus_strerror(nbBytes));
    return EXIT_FAILURE;
  } else {
    printf("%s%d: encode success. len=%d\n", __FUNCTION__, __LINE__, nbBytes);
  }
  head[0] = nbBytes & 0xff;
  head[1] = nbBytes>>8 & 0xff;
  *olen = nbBytes + 2;
  *obuf = opus_info.in;
  return 0;
}
