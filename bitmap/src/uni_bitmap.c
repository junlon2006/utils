/**********************************************************************
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
 **********************************************************************
 *
 * Description : uni_bitmap.c
 * Author      : junlon2006@163.com
 * Date        : 2017.9.19
 *
 **********************************************************************/

#include "uni_bitmap.h"

#include <stdlib.h>
#include <string.h>

#define BITMAP_TAG "bitmap"

#define BITMAP_SHIFT   5
#define BITMAP_MASK    0x1F

BitMap* BitMapNew(int size) {
  BitMap *bitmap;
  uint32_t len;
  if (size <= 0) {
    LOGE(BITMAP_TAG, "invalid size %d", size);
    return NULL;
  }
  len = (size + BITMAP_MASK) >> BITMAP_SHIFT;
  bitmap = (BitMap *)malloc(sizeof(BitMap));
  if (NULL == bitmap) {
    LOGE(BITMAP_TAG, "memory alloc failed");
    return NULL;
  }
  bitmap->map = (uint32_t *)malloc(sizeof(uint32_t) * len);
  if (NULL == bitmap->map) {
    LOGE(BITMAP_TAG, "memory alloc failed");
    free(bitmap);
    return NULL;
  }
  memset(bitmap->map, 0, sizeof(uint32_t) * len);
  bitmap->size = size;
  return bitmap;
}

void BitMapDel(BitMap *bitmap) {
  if (NULL != bitmap) {
    if (NULL != bitmap->map) {
      free(bitmap->map);
    }
    free(bitmap);
  } else {
    LOGE(BITMAP_TAG, "to free NULL");
  }
}

int BitMapSet(BitMap *bitmap, int i) {
  if (i < 0 || i >= bitmap->size) {
    LOGE(BITMAP_TAG, "param error");
    return -1;
  }
  bitmap->map[i >> BITMAP_SHIFT] |= (1 << (i & BITMAP_MASK));
  return 0;
}

int BitMapClear(BitMap *bitmap, int i) {
  if (i < 0 || i >= bitmap->size) {
    LOGE(BITMAP_TAG, "param error");
    return -1;
  }
  bitmap->map[i >> BITMAP_SHIFT] &= ~(1 << (i & BITMAP_MASK));
  return 0;
}

int BitMapTest(BitMap *bitmap, int i) {
  if (i < 0 || i >= bitmap->size) {
    LOGE(BITMAP_TAG, "param error");
    return -1;
  }
  if (bitmap->map[i >> BITMAP_SHIFT] & (1 << (i & BITMAP_MASK))) {
    return 0;
  }
  return -1;
}
