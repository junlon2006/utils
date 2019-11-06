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
 * Description : uni_dns_parse.c
 * Author      : shangjinlong.unisound.com
 * Date        : 2018.12.06
 *
 **************************************************************************/

#include "uni_dns_parse.h"
#include "list_head.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "cJSON.h"

#define DNS_REFRESH_PERIOD_SEC  (60 * 10)
#define SUPPORT_CACHE_ITEM_MAX  (100)
#define CACHE_FILE_NAME         "dns_cache.json"

typedef struct {
  list_head link;
  char      *hostname;
  char      *portstr;
  struct    addrinfo hints;
  struct    addrinfo *ai;
  int       request_cnt;
} DnsParseItem;

typedef struct {
  list_head       list;
  int             inited;
  pthread_mutex_t mutex;
  char            file_path[256];
  int             load_cache_done;
} DnsParse;

static DnsParse g_dns_parse;

static void _get_now_str(char *buf, int len) {
  struct timeval tv;
  time_t s;
  struct tm local;
  gettimeofday(&tv, NULL);
  s = tv.tv_sec;
  localtime_r(&s, &local);
  snprintf(buf, len, "%02d:%02d:%02d.%06"PRId64" ", local.tm_hour,
           local.tm_min, local.tm_sec, (int64_t)tv.tv_usec);
}

static void _get_thread_id_str(char *buf, int len) {
  snprintf(buf, len, "%x", pthread_self());
}

static void __ivm_crow_log_write(const char *function, int line,
                                 char *fmt, ...) {
  char buf[1024];
  char now[64];
  char thread_id[32];
  va_list args;
  snprintf(buf, sizeof(buf), "\033[0m\033[41;33m%s\033[0m ", "[W]");
  _get_now_str(now, sizeof(now));
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", now);
  _get_thread_id_str(thread_id, sizeof(thread_id));
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", thread_id);
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "<%s>", "ffmpeg");
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s:%d->", function, line);
  va_start(args, fmt);
  vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, args);
  va_end(args);
  printf("%s\n", buf);
  return 0;
}

static void _delete_item(DnsParseItem *item) {
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "delete item: hostname=%s, port=%s", item->hostname, item->portstr);
  list_del(&item->link);
  if (item->hostname) {
    free(item->hostname);
  }
  freeaddrinfo(item->ai);
  free(item->portstr);
  free(item);
}

static list_head* _get_next(list_head *entry, list_head *head) {
  if (list_is_tail(entry, head)) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "head=%p, entry=%p", head, entry);
    return NULL;
  }
  return entry->next;
}

static void _save_file(char *cjson_str) {
  char file_name[512];
  int fd;
  snprintf(file_name, sizeof(file_name), "%s/%s", g_dns_parse.file_path, CACHE_FILE_NAME);
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "dns_cache_file=%s", file_name);
  fd = open(file_name, O_RDWR | O_CREAT | O_SYNC | O_TRUNC, 0644);
  if (fd < 0) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "dns_cache_file=%s open failed", file_name);
    return;
  }
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "%s", cjson_str);
  write(fd, cjson_str, strlen(cjson_str));
  close(fd);
}

static void _persistence_write() {
  DnsParseItem *p;
  cJSON *root = NULL;
  cJSON *content = NULL;
  cJSON *data = NULL;
  cJSON *hostname = NULL;
  cJSON *port = NULL;
  cJSON *flags = NULL;
  char *cjson_str;
  if (!g_dns_parse.load_cache_done) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "load cache processing, skip persistence write");
    return;
  }
  root = cJSON_CreateObject();
  content = cJSON_CreateArray();
  list_for_each_entry(p, &g_dns_parse.list, DnsParseItem, link) {
    data = cJSON_CreateObject();
    hostname = cJSON_CreateString(p->hostname);
    port = cJSON_CreateString(p->portstr);
    flags = cJSON_CreateNumber(p->hints.ai_flags);
    cJSON_AddItemToObject(data, CJSON_OBJECT_NAME(hostname), hostname);
    cJSON_AddItemToObject(data, CJSON_OBJECT_NAME(port), port);
    cJSON_AddItemToObject(data, CJSON_OBJECT_NAME(flags), flags);
    cJSON_AddItemToArray(content, data);
  }
  cJSON_AddItemToObject(root, CJSON_OBJECT_NAME(content), content);
  cjson_str = cJSON_Print(root);
  _save_file(cjson_str);
  cJSON_Delete(root);
  free(cjson_str);
}

static void _refresh_dns_cache() {
  list_head *p = &g_dns_parse.list;
  DnsParseItem *item;
  struct addrinfo *ai;
  while (1) {
    pthread_mutex_lock(&g_dns_parse.mutex);
    p = _get_next(p, &g_dns_parse.list);
    if (NULL == p) {
      _persistence_write();
      pthread_mutex_unlock(&g_dns_parse.mutex);
      break;
    }
    item = list_entry(p, DnsParseItem, link);
    if (0 == getaddrinfo(item->hostname, item->portstr, &item->hints, &ai)) {
      freeaddrinfo(item->ai);
      item->ai = ai;
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "refresh dns cache[%s]:[%s] ai=%p", item->hostname,
                           item->portstr, item->ai);
    }
    pthread_mutex_unlock(&g_dns_parse.mutex);
    /* sleep to unlock mutex, it is necessary */
    usleep(1000 * 50);
  }
}

static void _persistence_read() {
  char file_name[512];
  char *json_str = NULL;
  int fd, len;
  cJSON *root = NULL;
  cJSON *content = NULL;
  cJSON *hostname = NULL;
  cJSON *port = NULL;
  cJSON *flags = NULL;
  cJSON *data = NULL;
  int array_size, i, ret;
  snprintf(file_name, sizeof(file_name), "%s/%s", g_dns_parse.file_path, CACHE_FILE_NAME);
  struct addrinfo hints = {0}, *ai;
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "dns_cache_file=%s", file_name);
  fd = open(file_name, O_RDWR, 0664);
  if (fd < 0) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "open dns_cache_file=%s failed", file_name);
    goto L_ERROR;
  }
  len = lseek(fd, 0, SEEK_END);
  json_str = (char *)malloc(len);
  lseek(fd, 0, SEEK_SET);
  if (len != read(fd, json_str, len)) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "read dns_cache_file=%s failed", file_name);
    goto L_ERROR;
  }
  if (NULL == (root = cJSON_Parse(json_str))) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "cJSON parse failed");
    goto L_ERROR;
  }
  if (NULL == (content = cJSON_GetObjectItem(root, CJSON_OBJECT_NAME(content)))) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "content not exist");
    goto L_ERROR;
  }
  array_size = cJSON_GetArraySize(content);
  for (i = 0; i < array_size; i++) {
    if (NULL == (data = cJSON_GetArrayItem(content, i))) {
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "data not exist");
      continue;
    }
    if (NULL == (hostname = cJSON_GetObjectItem(data, CJSON_OBJECT_NAME(hostname)))) {
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "hostname not exist");
      continue;
    }
    if (NULL == (port = cJSON_GetObjectItem(data, CJSON_OBJECT_NAME(port)))) {
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "port not exist");
      continue;
    }
    if (NULL == (flags = cJSON_GetObjectItem(data, CJSON_OBJECT_NAME(flags)))) {
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "flags not exist");
      continue;
    }
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "add new cache dns data");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = flags->valueint;
    do {
      ret = dns_parse_by_domain(hostname->valuestring, port->valuestring, &hints, &ai);
      dns_parse_by_domain_done();
      if (ret != 0) {
        usleep(1000 * 500);
      }
    } while (ret != 0);
  }
L_ERROR:
  if (NULL != json_str) {
    free(json_str);
  }
  if (NULL != root) {
    cJSON_Delete(root);
  }
  if (fd > 0) {
    close(fd);
  }
  g_dns_parse.load_cache_done = 1;
}

static void* _refresh_dns_tsk(void *args) {
  _persistence_read();
  while (1) {
    _refresh_dns_cache();
    sleep(DNS_REFRESH_PERIOD_SEC);
  }
  return NULL;
}

static int _find_exist_addrinfo(char *hostname, char *portstr,
                                struct addrinfo *hints, struct addrinfo **ai) {
  int ret = -1;
  DnsParseItem *p;
  list_for_each_entry(p, &g_dns_parse.list, DnsParseItem, link) {
    if ((NULL == hostname || NULL == p->hostname) && p->hostname != hostname) {
      continue;
    }
    if (((NULL == hostname && NULL == p->hostname) ||
          0 == strcmp(hostname, p->hostname)) &&
        0 == strcmp(portstr, p->portstr) &&
        0 == memcmp(hints, &p->hints, sizeof(struct addrinfo))) {
      *ai = p->ai;
      p->request_cnt++;
      ret = 0;
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "find hostname[%s] port[%s] request_cnt[%d] ai=%p",
                           hostname, portstr, p->request_cnt, *ai);
      break;
    }
  }
  return ret;
}

static int _list_count(list_head *head) {
  int c = 0;
  list_head *t;
  list_for_each(t, head) {
    c++;
  }
  return c;
}

#define _min(a, b)  ((b) < (a) ? (b) : (a))
static void _delete_worst_item() {
  uint32_t min_request_cnt = -1;
  DnsParseItem *p = NULL;
  list_for_each_entry(p, &g_dns_parse.list, DnsParseItem, link) {
    min_request_cnt = _min(min_request_cnt, p->request_cnt);
  }
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "min_request_cnt=%d", min_request_cnt);
  list_for_each_entry(p, &g_dns_parse.list, DnsParseItem, link) {
    if (p->request_cnt == min_request_cnt) {
      __ivm_crow_log_write(__FUNCTION__, __LINE__, "find worst item: request_cnt=%d", p->request_cnt);
      _delete_item(p);
      break;
    }
  }
}

static void _list_item_overflow_process() {
  int cnt = _list_count(&g_dns_parse.list);
  if (cnt == SUPPORT_CACHE_ITEM_MAX) {
    __ivm_crow_log_write(__FUNCTION__, __LINE__, "list full");
    _delete_worst_item();
  }
}

static int _insert_new_dns_item_2_list(char *hostname, char *portstr,
                                       struct addrinfo *hints,
                                       struct addrinfo **ai) {
  int rc;
  DnsParseItem *item = NULL;
  if (0 != (rc = getaddrinfo(hostname, portstr, hints, ai))) {
    return rc;
  }
  item = malloc(sizeof(DnsParseItem));
  memset(item, 0, sizeof(DnsParseItem));
  if (hostname) {
    item->hostname = malloc(strlen(hostname) + 1);
    strcpy(item->hostname, hostname);
  }
  item->portstr = malloc(strlen(portstr) + 1);
  strcpy(item->portstr, portstr);
  memcpy(&item->hints, hints, sizeof(struct addrinfo));
  item->ai = *ai;
  item->request_cnt++;
  _list_item_overflow_process();
  list_add_before(&item->link, &g_dns_parse.list);
  _persistence_write();
  return 0;
}

void dns_parse_init(const char *file_path) {
  pthread_t pid;
  if (1 == g_dns_parse.inited) {
    return;
  }
  memset(&g_dns_parse, 0, sizeof(DnsParse));
  list_init(&g_dns_parse.list);
  pthread_mutex_init(&g_dns_parse.mutex, NULL);
  snprintf(g_dns_parse.file_path, sizeof(g_dns_parse.file_path), "%s", file_path);
  pthread_create(&pid, NULL, _refresh_dns_tsk, NULL);
  pthread_detach(pid);
  g_dns_parse.inited = 1;
}

int dns_parse_by_domain(char *hostname, char *portstr,
                        struct addrinfo *hints, struct addrinfo **ai) {
  pthread_mutex_lock(&g_dns_parse.mutex);
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "step into dns parse");
  if (0 == _find_exist_addrinfo(hostname, portstr, hints, ai)) {
    return 0;
  }
  return _insert_new_dns_item_2_list(hostname, portstr, hints, ai);
}

void dns_parse_by_domain_done() {
  pthread_mutex_unlock(&g_dns_parse.mutex);
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "leave dns parse");
}
