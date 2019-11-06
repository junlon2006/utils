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
 * Description : uni_log.h
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef UTILS_CODEC_SRC_SPEEX_TEST_UNI_LOG_H_
#define UTILS_CODEC_SRC_SPEEX_TEST_UNI_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  N_LOG_NONE = -1,
  N_LOG_ERROR,
  N_LOG_WARN,
  N_LOG_INFO,
  N_LOG_DEBUG,
  N_LOG_ALL
} UniLogLevel;

typedef struct {
  char        b_enable_time :1;
  char        b_enable_thread_id :1;
  char        b_enable_fuction_line :1;
  char        b_enable_sync :1;
  UniLogLevel set_level;
} UniLogConfig;

int UniLogInitialize(const UniLogConfig *config);
int UniLogFinalize(void);
int UniLogLevelSet(UniLogLevel level);

int UniLogLevelValid(UniLogLevel level);
int UniLogWrite(UniLogLevel level, const char *tags, const char *function, int line, char *fmt, ...);
#define LOG(level, tag, fmt, ...) \
  do { \
    if (UniLogLevelValid(level)) { \
      UniLogWrite(level, tag, __FUNCTION__, __LINE__, fmt"\n", ##__VA_ARGS__); \
    } \
  } while (0);

#define LOGD(tag, fmt, ...) LOG(N_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOGT(tag, fmt, ...) LOG(N_LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) LOG(N_LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) LOG(N_LOG_ERROR, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
