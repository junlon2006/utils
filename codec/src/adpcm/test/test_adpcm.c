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
 * Description : test_adpcm.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "uni_adpcm.h"

const char *input_file = "test.pcm";
const char *encode_file = "out.adpcm";
const char *decode_file = "out.pcm";

static adpcm_state_t encode_status = {0}, decode_status = {0};

#define FRAME_SIZE        640
#define COMPRESS_RATIO    4
#define COMP_FRAME_SIZE   (FRAME_SIZE / COMPRESS_RATIO)

void test_encode(void) {
  FILE *f_input = NULL, *f_encode = NULL;
  short in_data[FRAME_SIZE / 2];
  unsigned int in_len;
  char out_data[COMP_FRAME_SIZE];
  f_input = fopen(input_file, "r");
  f_encode = fopen(encode_file, "w");
  while ((in_len = fread(in_data, 1, FRAME_SIZE, f_input)) > 0) {
    adpcm_coder(in_data, out_data, in_len, &encode_status);
    printf("param_encode: %p %p %d\n", in_data, out_data, in_len);
    printf("write byte %d\n", in_len / COMPRESS_RATIO);
    fwrite(out_data, 1, in_len / COMPRESS_RATIO, f_encode);
  }
  fclose(f_input);
  fclose(f_encode);
}

void test_decode(void) {
  FILE *f_encode = NULL, *f_decode = NULL;
  char in_data[COMP_FRAME_SIZE];
  short out_data[FRAME_SIZE / 2];
  unsigned int in_len;
  f_encode = fopen(encode_file, "r");
  f_decode = fopen(decode_file, "w");
  while ((in_len = fread(in_data, 1, COMP_FRAME_SIZE, f_encode)) > 0) {
    adpcm_decoder(in_data, out_data, in_len, &decode_status);
    printf("param_decode: %p %p %d\n", in_data, out_data, in_len);
    fwrite(out_data, 1, in_len * COMPRESS_RATIO, f_decode);
  }
  fclose(f_encode);
  fclose(f_decode);
}

int main(void) {
  test_encode();
  printf("\n");
  test_decode();
  return 0;
}
