/**************************************************************************
 * Copyright (C) 2018-2019  Junlon2006
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
 * Description : uni_config.h
 * Author      : junlon2006@163.com
 * Date        : 2019.03.23
 *
 **************************************************************************/
#ifndef CONFIG_INC_UNI_CONFIG_H_
#define CONFIG_INC_UNI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

int ConfigInitialize();
int ConfigFinalize();
int ConfigReadItemNumber(const char *filename, const char *fmt,
                         int64_t *number);
int ConfigReadItemDouble(const char *filename, const char *fmt, double *number);
int ConfigReadItemString(const char *filename, const char *fmt,
                         char *dst, int len);
int ConfigWriteItemNumber(const char *filename, const char *fmt,
                          int64_t number);
int ConfigWriteItemDouble(const char *filename, const char *fmt, double number);
int ConfigWriteItemString(const char *filename, const char *fmt, char *new_str);
int ConfigReadItemNumberArray(const char *filename, const char *fmt,
                              int64_t *number, int cnt);
int ConfigReadItemDoubleArray(const char *filename, const char *fmt,
                              double *number, int cnt);
int ConfigReadItemstringArray(const char *filename, const char *fmt,
                              char **dst, int len);

#ifdef __cplusplus
}
#endif
#endif
