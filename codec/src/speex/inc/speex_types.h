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
 * Description : speex_types.h
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
/* speex_types.h taken from libogg */
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.
 last mod: $Id: os_types.h 7524 2004-08-11 04:20:36Z conrad $

 ********************************************************************/
/**
   @file speex_types.h
   @brief Speex types
*/

#ifndef UTILS_CODEC_SRC_SPEEX_INC_SPEEX_TYPES_H_
#define UTILS_CODEC_SRC_SPEEX_INC_SPEEX_TYPES_H_
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)

#  if defined(__CYGWIN__)
#    include <_G_config.h>
     typedef _G_int32_t spx_int32_t;
     typedef _G_uint32_t spx_uint32_t;
     typedef _G_int16_t spx_int16_t;
     typedef _G_uint16_t spx_uint16_t;
#  elif defined(__MINGW32__)
     typedef short spx_int16_t;
     typedef unsigned short spx_uint16_t;
     typedef int spx_int32_t;
     typedef unsigned int spx_uint32_t;
#  elif defined(__MWERKS__)
     typedef int spx_int32_t;
     typedef unsigned int spx_uint32_t;
     typedef short spx_int16_t;
     typedef unsigned short spx_uint16_t;
#  else
     /* MSVC/Borland */
     typedef __int32 spx_int32_t;
     typedef unsigned __int32 spx_uint32_t;
     typedef __int16 spx_int16_t;
     typedef unsigned __int16 spx_uint16_t;
#  endif

#elif defined(__MACOS__)

#  include <sys/types.h>
   typedef SInt16 spx_int16_t;
   typedef UInt16 spx_uint16_t;
   typedef SInt32 spx_int32_t;
   typedef UInt32 spx_uint32_t;

#elif (defined(__APPLE__) && defined(__MACH__)) /* MacOS X Framework build */

#  include <sys/types.h>
   typedef int16_t spx_int16_t;
   typedef u_int16_t spx_uint16_t;
   typedef int32_t spx_int32_t;
   typedef u_int32_t spx_uint32_t;

#elif defined(__BEOS__)

   /* Be */
#  include <inttypes.h>
   typedef int16_t spx_int16_t;
   typedef u_int16_t spx_uint16_t;
   typedef int32_t spx_int32_t;
   typedef u_int32_t spx_uint32_t;

#elif defined (__EMX__)

   /* OS/2 GCC */
   typedef short spx_int16_t;
   typedef unsigned short spx_uint16_t;
   typedef int spx_int32_t;
   typedef unsigned int spx_uint32_t;

#elif defined (DJGPP)

   /* DJGPP */
   typedef short spx_int16_t;
   typedef int spx_int32_t;
   typedef unsigned int spx_uint32_t;

#elif defined(R5900)

   /* PS2 EE */
   typedef int spx_int32_t;
   typedef unsigned spx_uint32_t;
   typedef short spx_int16_t;

#elif defined(__SYMBIAN32__)

   /* Symbian GCC */
   typedef signed short spx_int16_t;
   typedef unsigned short spx_uint16_t;
   typedef signed int spx_int32_t;
   typedef unsigned int spx_uint32_t;

#elif defined(CONFIG_TI_C54X) || defined (CONFIG_TI_C55X)

   typedef short spx_int16_t;
   typedef unsigned short spx_uint16_t;
   typedef long spx_int32_t;
   typedef unsigned long spx_uint32_t;

#elif defined(CONFIG_TI_C6X)

   typedef short spx_int16_t;
   typedef unsigned short spx_uint16_t;
   typedef int spx_int32_t;
   typedef unsigned int spx_uint32_t;

#else

#include "speex_config_types.h"

#endif

#ifdef __cplusplus
}
#endif
#endif  //  UTILS_CODEC_SRC_SPEEX_INC_SPEEX_TYPES_H_
