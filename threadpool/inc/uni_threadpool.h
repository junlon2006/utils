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
 * Description : uni_threadpool.h
 * Author      : junlon2006@163.com
 * Date        : 2019.03.24
 *
 **************************************************************************/
#ifndef THREADPOOL_INC_UNI_THREADPOOL_H_
#define THREADPOOL_INC_UNI_THREADPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define THREADPOOL_DEFAULT_THREAD_CNT  (-1)

typedef void* ThreadPoolHandle;
typedef void* (*ThreadWorkerTask)(void *args);

ThreadPoolHandle ThreadPoolCreate(int thread_cnt);
void             ThreadPoolDestroy(ThreadPoolHandle handle);
int              ThreadPoolJoinWorker(ThreadPoolHandle handle,
                                      ThreadWorkerTask worker, void *args);
#ifdef __cplusplus
}
#endif
#endif
