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
 * Description : uni_log.c
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_log.h"
#include "uni_iot.h"

int UniLogLevelValid(UniLogLevel level) {
  return 0;
}

int UniLogWrite(UniLogLevel level, const char *tags, const char *function, int line, char *fmt, ...) {
  return 0;
}

int UniLogLevelSet(UniLogLevel level) {
  return 0;
}

int UniLogInitialize(const UniLogConfig *config) {
  return 0;
}

int UniLogFinalize(void) {
  return 0;
}
