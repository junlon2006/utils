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
 * Description : uni_blackboard.h
 * Author      : junlon2006@163.com
 * Date        : 2019.12.21
 *
 **************************************************************************/

#ifndef BLACKBORD_INC_UNI_BLACKBOARD_H_
#define BLACKBORD_INC_UNI_BLACKBOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    BB_KEY_INVALID = -1,
    BB_KEY_1,
    BB_KEY_2,
    BB_KEY_3,
    BB_KEY_CNT,
} BlackboardKey;

typedef void* VALUE;
typedef void (* FreeValueHandler)(VALUE value);

int BlackbordInit();
void BlackboardFinal();

VALUE BlackboardRead(BlackboardKey key);
int BlackboardWrite(BlackboardKey key, VALUE value, FreeValueHandler free_handler);

#ifdef __cplusplus
}
#endif
#endif   //  BLACKBORD_INC_UNI_BLACKBOARD_H_
