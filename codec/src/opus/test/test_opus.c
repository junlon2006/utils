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
 * Description : test_opus.c
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "uni_opus.h"

const char *input_file = "test.pcm";
const char *encode_file = "out.opus";
const char *decode_file = "out.pcm";

#define FRAME_SIZE 320

void test_encode(void) {
  FILE *f_input = NULL, *f_encode = NULL;
  unsigned char in_data[FRAME_SIZE];
  unsigned int in_len;
  unsigned char *out_data;
  unsigned int out_len;
  f_input = fopen(input_file, "rb");
  f_encode = fopen(encode_file, "wb");
  while ((in_len = fread(in_data, 1, FRAME_SIZE, f_input)) > 0) {
    uni_opus_encode((opus_int16 *)in_data, in_len / sizeof(opus_int16),
                    &out_data, &out_len);
    printf("param_encode: %p %lu %p %d\n", (opus_int16 *)in_data,
           in_len / sizeof(opus_int16), out_data, out_len);
    fwrite(out_data, 1, out_len, f_encode);
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
  f_encode = fopen(encode_file, "rb");
  f_decode = fopen(decode_file, "wb");
  while ((fread(in_data, 1, 2, f_encode)) == 2) {
    in_len = in_data[0] + (in_data[1] << 8);
    if (in_len == 0) {
      break;
    }
    fread(in_data, 1, in_len, f_encode);
    uni_opus_decode(in_data, in_len, (opus_int16 **)&out_data, &out_len);
    printf("param_decode: %p %d %p %d\n", in_data, in_len, out_data, out_len);
    fwrite(out_data, 1, out_len * sizeof(opus_int16), f_decode);
  }
  fclose(f_encode);
  fclose(f_decode);
}

int main(void) {
  uni_init_opus();
  test_encode();
  printf("\n");
  test_decode();
  uni_uninit_opus();
  return 0;
}
