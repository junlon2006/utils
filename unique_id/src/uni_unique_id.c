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
 * Description : uni_unique_id.c
 * Author      : junlon2006@163.com
 * Date        : 2018.12.27
 *
 **************************************************************************/
#include "uni_unique_id.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include </usr/include/net/if.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define WLAN            "wlan0"

static int _get_mac_address(uint8_t *address, int len) {
  int sock, ret;
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket create failed\n");
    return -1;
  }
  strcpy(ifr.ifr_name, WLAN);
  if ((ret = ioctl(sock, SIOCGIFHWADDR, &ifr, sizeof(ifr))) == 0) {
    memcpy(address, ifr.ifr_hwaddr.sa_data, len);
    ret = 0;
  } else {
    printf("get mac failed\n");
    ret = -1;
  }
  close(sock);
  return ret;
}

static void _srand_init() {
  static bool inited = false;
  if (!inited) {
    srand(time(NULL));
    inited = true;
  }
}

static int64_t _get_now_msec(void) {
  struct timeval t1;
  gettimeofday(&t1, NULL);
  return ((int64_t)t1.tv_sec * 1000 + t1.tv_usec/1000);
}

/* mac + timestamp_msec + rand*/
void UniqueStringIdGenerate(char *buf, int len) {
  uint8_t mac[6] = {0};
  _srand_init();
  _get_mac_address(mac, sizeof(mac));
  snprintf(buf, len, "%02x%02x%02x%02x%02x%02x-",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(buf + strlen(buf), len - strlen(buf), "%ld-%d",
           (int64_t)_get_now_msec(), rand() % 100000000);
  printf("unique_id=[%lu, %s]\n", strlen(buf), buf);
}
