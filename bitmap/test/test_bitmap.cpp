/*
 * All header file for c language MUST be include : in extern "C" { }
 */
extern "C" {
#include "../src/uni_bitmap.c"
}

H2UNIT(bitmap) {
};

//---------------------------------------------------------------------------------------
H2CASE(bitmap, "BitMapNew") {
  BitMap *bp;
  /* test for valid input */
  bp = BitMapNew(1);
  H2EQ_TRUE(bp != NULL);
  H2EQ_TRUE(bp->map != NULL);
  H2EQ_MATH(1, bp->size);
  free(bp->map);
  free(bp);
  bp = NULL;
  BitMapDel(bp);
  bp = BitMapNew(1024 * 1024);
  H2EQ_TRUE(bp != NULL);
  H2EQ_TRUE(bp->map != NULL);
  H2EQ_MATH(1024 * 1024, bp->size);
  free(bp->map);
  free(bp);
  bp = NULL;
  BitMapDel(bp);
  /* test for invalid input */
  bp = BitMapNew(-1);
  H2EQ_TRUE(bp == NULL);
  bp = BitMapNew(0);
  H2EQ_TRUE(bp == NULL);
}

//---------------------------------------------------------------------------------------
H2CASE(bitmap, "BitMapDel") {
  BitMap *bp = BitMapNew(1);
  BitMapDel(bp);
  BitMapDel(NULL);
}

//---------------------------------------------------------------------------------------
H2CASE(bitmap, "BitMapSet") {
  int ret;
  BitMap *bp;
  bp = BitMapNew(1);
  ret = BitMapSet(bp, 0);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(0x00000001, bp->map[0]);
  ret = BitMapSet(bp, 1);
  H2EQ_MATH(-1, ret);
  H2EQ_MATH(0x00000001, bp->map[0]);
  BitMapDel(bp);

  bp = BitMapNew(33);
  ret = BitMapSet(bp, 0);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(0x00000001, bp->map[0]);
  H2EQ_MATH(0, bp->map[1]);
  ret = BitMapSet(bp, 1);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(0x00000003, bp->map[0]);
  H2EQ_MATH(0, bp->map[1]);
  ret = BitMapSet(bp, 32);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(0x00000003, bp->map[0]);
  H2EQ_MATH(0x00000001, bp->map[1]);
  ret = BitMapSet(bp, 33);
  H2EQ_MATH(-1, ret);
  H2EQ_MATH(0x00000003, bp->map[0]);
  H2EQ_MATH(0x00000001, bp->map[1]);
  BitMapDel(bp);
}

//---------------------------------------------------------------------------------------
H2CASE(bitmap, "BitMapTest") {
  int ret;
  BitMap *bp = BitMapNew(33);
  BitMapSet(bp, 0);
  ret = BitMapTest(bp, 0);
  H2EQ_MATH(0, ret);

  ret = BitMapTest(bp, 1);
  H2EQ_MATH(-1, ret);
  BitMapSet(bp, 1);
  ret = BitMapTest(bp, 1);
  H2EQ_MATH(0, ret);

  ret = BitMapTest(bp, 31);
  H2EQ_MATH(-1, ret);
  BitMapSet(bp, 31);
  ret = BitMapTest(bp, 31);
  H2EQ_MATH(0, ret);
  BitMapDel(bp);
}

//---------------------------------------------------------------------------------------
H2CASE(bitmap, "BitMapClear") {
  int ret;
  BitMap *bp = BitMapNew(33);
  BitMapSet(bp, 0);
  ret = BitMapTest(bp, 0);
  H2EQ_MATH(0, ret);
  BitMapClear(bp, 0);
  ret = BitMapTest(bp, 0);
  H2EQ_MATH(-1, ret);

  BitMapSet(bp, 31);
  ret = BitMapTest(bp, 31);
  H2EQ_MATH(0, ret);
  BitMapClear(bp, 31);
  ret = BitMapTest(bp, 31);
  H2EQ_MATH(-1, ret);

  BitMapSet(bp, 32);
  ret = BitMapTest(bp, 32);
  H2EQ_MATH(0, ret);
  BitMapClear(bp, 32);
  ret = BitMapTest(bp, 32);
  H2EQ_MATH(-1, ret);
  BitMapDel(bp);
}
