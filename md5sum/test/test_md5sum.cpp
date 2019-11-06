/*
 * All header file for c language MUST be include : in extern "C" { }
 */
extern "C" {
#include "../src/uni_md5sum.c"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
}

H2UNIT(md5sum) {
  void setup(void) {
  }
  void teardown(void) {
  }
};

H2CASE(md5sum, "md5sum test") {
  uint8_t digest[16] = {0};
  uint8_t md5sum[16] = {0xe8, 0x07, 0xf1, 0xfc, 0xf8, 0x2d, 0x13, 0x2f,
                        0x9b, 0xb0, 0x18, 0xca, 0x67, 0x38, 0xa1, 0x9f};
  uint8_t msg[11] = {"1234567890"};
  Md5sum(msg, strlen((char *)msg), digest);
  int i;
  for (i = 0; i < 16; i++) {
    printf("%2.2x", digest[i]);
  }
  printf("\n");
  H2EQ_TRUE(0 == memcmp(digest, md5sum, 16));
}
