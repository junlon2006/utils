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
 * Description : uni_iot.h
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
/*
 * Unisound 2013 Unisound Atheros, Inc..
 * All Rights Reserved.
 * Unisound Atheros Confidential and Proprietary.
 *
 * history:
 *  Unisound SDK V1.0.1 
 *
 * Date: 2016-06-12
 * Author: gaochuan@unisound.com
 * Version:	Unisound SDK V2.0.1
 *
 **/

#ifndef UTILS_CODEC_SRC_ADPCM_TEST_UNI_IOT_H_
#define UTILS_CODEC_SRC_ADPCM_TEST_UNI_IOT_H_
#ifndef _UNI_IOT_H__
#define _UNI_IOT_H__

#ifdef __cplusplus
extern "C" {
#endif // C++

#include <sys/select.h>
#include <math.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>   
#include <sys/param.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <unistd.h>  
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <semaphore.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <time.h>
#include <sched.h>
#include <linux/types.h>
#include <linux/errqueue.h>
#include <linux/sockios.h>

/* common definitions start
 ************************************
 */

#define UNI_OK      0
#define UNI_FAIL    -1

#define UNI_TRUE    1
#define UNI_FALSE   0


/*task name*/
#define CAPTURE_TASK 	"capture_task"
#define EVENT_TASK 		"event_task"
#define LSR_TASK 		"lsr_task"
#define ASR_TASK 		"asr_task"
#define PLAYBACK_TASK 	"playback_task"
#define PLAYLIST_TASK 	"playlist_task"
#define THPOOL_TASK 	"thpool_task_"
/*
 *
 **/
typedef enum module_status_e {
    UNI_MODULE_OPEN = 0,
    UNI_MODULE_CLOSE,
    UNI_MODULE_START,
    UNI_MODULE_STOP,
    UNI_MODULE_RUNNING,
    UNI_MODULE_BUSY,
    UNI_MODULE_SUSPEND,
    UNI_MODULE_IDLE,
    UNI_MODULE_NONE

} module_status_t;

/*
 * Types for `void *' pointers.
 **/
#if __WORDSIZE == 64
# ifndef __intptr_t_defined
    typedef long int        intptr_t;
#define __intptr_t_defined
#endif
    typedef unsigned long int   uintptr_t;
#else
#ifndef __intptr_t_defined
    typedef int         intptr_t;
#define __intptr_t_defined
#endif
    typedef unsigned int        uintptr_t;
#endif

typedef int                 uni_s32;	// UNI_S32
typedef unsigned int        uni_u32;
typedef short               uni_s16;
typedef unsigned short      uni_u16;
typedef char                uni_char;
typedef unsigned char       uni_u8;
typedef void                uni_void;
typedef long long           uni_s64;
typedef float               uni_float;
typedef unsigned long       uni_ulong;
typedef long                uni_long;
typedef uni_long            uni_double;

#ifndef NULL
   #define NULL ((void *)0)
#endif

typedef enum uni_module_e {
    UNI_WAKEUP_M = 0x60,
    UNI_ASR_M,
    UNI_TTS_M,
    UNI_AUDIO_M,
    UNI_CAPTURE_M,
    UNI_PLAYBACK_M,
    UNI_PLAYLIST_M,
    UNI_MEM_POOL_M,
    UNI_QUEUE_M,
    UNI_RESAMP_M,
    UNI_ENCRY_M,
    UNI_COMM_M,
    UNI_NET_M,
    UNI_MOD_M,
    UNI_NETTY_M,

} uni_module_t;

typedef enum uni_com_err_e {
    UNI_OPEN_ERR = 0x600,        // 打开文件|模块|设备失败
    UNI_CLOSE_ERR,               // 关闭文件|模块|设备失败
    UNI_START_ERR,               // 启动操作|模块|设备失败
    UNI_STOP_ERR,                // 停止操作|模块|设备失败
    UNI_DEST_ERR,
    UNI_SUSP_ERR,

    UNI_THREAD_ERR,

    UNI_INVALID_HANDLE_ERR,      // 无效的句柄|指针|参数地址
    UNI_INVALID_FILE_ERR,        // 无效的文件
    UNI_INVALID_SIZE_ERR,        // 无效的文件|结构体|数组大小
    UNI_INVALID_FORMAT_ERR,      // 无效的格式
    UNI_INVALID_PARAM_ERR,       // 无效的参数

    UNI_NO_CMD_ERR,              // 无效的指令
    UNI_NO_MEM_ERR,              // 没有足够内存
    UNI_NO_SUPPORT_ERR,          // 不支持此功能|设备|操作

    UNI_MODULE_INIT_ERR,         // 模块初始化失败
    UNI_MODULE_UNINIT_ERR,       // 模块未初始化
    UNI_MODULE_INIT_ALREADY_ERR, // 模块已初始化
    UNI_MODULE_BUSY_ERR,         // 模块|设备繁忙
    UNI_MODULE_NONE_ERR,         // 模块|设备空闲

    UNI_INVALID_USERNAME_ERR,    // 无效的用户名
    UNI_INVALID_PASSWORD_ERR,    // 无效的密码
    UNI_NO_PERMISION_ERR,        // 没有权限

    UNI_GET_STATUS_ERR,
    UNI_SET_STATUS_ERR,

    UNI_ERR_NUM_END

} uni_com_err_t;

#define uni_errno(m, c)     \
{   \
    uni_printf("Failed. Module:%x,Error Code:%x\n", m, c);    \
} \

#define UNI_ISSPACE(x)               (((x) == 0x20) || ((x) > 0x8 && (x) < 0xe))
/* common definitions end
 ************************************
 */


/*
 * uni_assert
 */
#define uni_assert assert

/*
 * uni_*printf
 */
#define uni_printf printf
#define uni_sprintf sprintf
#define uni_perror perror
#define uni_sscanf sscanf
#define uni_snprintf snprintf
 
#ifdef __cplusplus
}	/* extern "C" */
#endif /*C++*/
/* end uni_select.h */
#endif //_UNI_IOT_H__
#endif  //  UTILS_CODEC_SRC_ADPCM_TEST_UNI_IOT_H_
