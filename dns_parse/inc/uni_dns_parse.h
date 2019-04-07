/**************************************************************************
 * Copyright (C) 2017-2017  Junlon2006
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
 * Description : uni_dns_parse.h
 * Author      : junlon2006@163.com
 * Date        : 2019.04.07
 *
 **************************************************************************/
#ifndef DNS_PARSE_INC_UNI_DNS_PARSE_H_
#define DNS_PARSE_INC_UNI_DNS_PARSE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DNS_PARSE_INVALID_IP    ((uint64_t)-1)

int      DnsParseInit(void);
void     DnsParseFinal(void);
uint64_t DnsParseByDomain(char *domain);

#ifdef __cplusplus
}
#endif
#endif   /* DNS_PARSE_INC_UNI_DNS_PARSE_H_ */
