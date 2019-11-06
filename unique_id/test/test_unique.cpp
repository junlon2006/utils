/*
 * All header file for c language MUST be include : in extern "C" { }
 */
extern "C" {
#include "../src/uni_unique_id.c"
}

static unsigned long _get_mac_address_stub(uni_u8 *buf, uni_s32 len) {
  buf[0] = 0x01;
  buf[1] = 0x02;
  buf[2] = 0x03;
  buf[3] = 0x04;
  buf[4] = 0x05;
  buf[5] = 0x06;
}

H2UNIT(unique_id) {
  void setup() {
    H2STUB(_get_mac_address, _get_mac_address_stub);
  }
  void teardown() {
  }
};

H2CASE(unique_id, "unique_id generate string test") {
  char buf[64] = {0};
  UniqueStringIdGenerate(buf, sizeof(buf));
  printf("%s%d: buf=%s\n", __FUNCTION__, __LINE__, buf);
  H2EQ_MATH(buf[0], '0');
  H2EQ_TRUE(buf[1] == '1');
  H2EQ_TRUE(buf[2] == '0');
  H2EQ_TRUE(buf[3] == '2');
  H2EQ_TRUE(buf[4] == '0');
  H2EQ_TRUE(buf[5] == '3');
  H2EQ_TRUE(buf[6] == '0');
  H2EQ_TRUE(buf[7] == '4');
  H2EQ_TRUE(buf[8] == '0');
  H2EQ_TRUE(buf[9] == '5');
  H2EQ_TRUE(buf[10] == '0');
  H2EQ_TRUE(buf[11] == '6');
}
