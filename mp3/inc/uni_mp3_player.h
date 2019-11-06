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
 * Description : uni_mp3_player.h
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef MP3_INC_UNI_MP3_PLAYER_H_
#define MP3_INC_UNI_MP3_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
  int channels;
  int rate;
  int bit;
} AudioParam;

int Mp3Init(AudioParam *param);
int Mp3Final(void);

int Mp3Play(char *filename);
int Mp3Start(void);
int Mp3Pause(void);
int Mp3Resume(void);
int Mp3Stop(void);

bool Mp3CheckIsPlaying(void);
bool Mp3CheckIsPause(void);

#ifdef __cplusplus
}
#endif
#endif  //  MP3_INC_UNI_MP3_PLAYER_H_
