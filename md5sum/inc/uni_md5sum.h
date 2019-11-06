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
 * Description : uni_md5sum.h
 * Author      : junlon2006@163.com
 * Date        : 2019.05.14
 *
 **********************************************************************/

#ifndef MD5SUM_INC_UNI_MD5SUM_H_
#define MD5SUM_INC_UNI_MD5SUM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#define MD5SUM_LEN  (16)

void Md5sum(const uint8_t *msg, size_t msg_len, uint8_t digest[MD5SUM_LEN]);

#ifdef __cplusplus
}
#endif
#endif  // MD5SUM_INC_UNI_MD5SUM_H_
