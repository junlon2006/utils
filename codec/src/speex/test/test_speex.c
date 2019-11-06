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
 * Description : test_speex.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "uni_speex.h"

const char *input_file = "test.pcm";
const char *encode_file = "out.speex";
const char *decode_file = "out.pcm";

#define FRAME_SIZE 640

void test_encode(void) {
  FILE *f_input = NULL, *f_encode = NULL;
  unsigned char in_data[FRAME_SIZE];
  unsigned int in_len;
  char *out_data;
  unsigned int out_len;
  f_input = fopen(input_file, "r");
  f_encode = fopen(encode_file, "w");
  while ((in_len = fread(in_data, 1, FRAME_SIZE, f_input)) > 0) {
    uni_speex_encode((uni_s16 *)in_data, in_len / sizeof(uni_s16),
                    &out_data, &out_len);
    printf("param_encode: %p %lu %p %d\n", (uni_s16 *)in_data,
           in_len / sizeof(uni_s16), out_data, out_len);
    if (out_len > 0) {
      printf("write byte %d\n", out_len);
      fwrite(out_data, 1, out_len, f_encode);
    }
  }
  fclose(f_input);
  fclose(f_encode);
}

void test_decode(void) {
  FILE *f_encode = NULL, *f_decode = NULL;
  unsigned char in_data[FRAME_SIZE];
  unsigned int in_len;
  unsigned char *out_data;
  int out_len;
  f_encode = fopen(encode_file, "r");
  f_decode = fopen(decode_file, "w");
  while ((fread(in_data, 1, 2, f_encode)) == 2) {
    in_len = in_data[0] + (in_data[1] << 8);
    if (in_len == 0) {
      break;
    }
    fread(in_data, 1, in_len, f_encode);
    uni_speex_decode(in_data, in_len, (uni_s16 **)&out_data, &out_len);
    printf("param_decode: %p %d %p %d\n", in_data, in_len, out_data, out_len);
    fwrite(out_data, 1, out_len * sizeof(uni_s16), f_decode);
  }
  fclose(f_encode);
  fclose(f_decode);
}

int main(void) {
  uni_init_speex();
  test_encode();
  printf("\n");
  test_decode();
  uni_uninit_speex();
  return 0;
}
