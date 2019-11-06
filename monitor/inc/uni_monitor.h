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
 * Description : uni_monitor.h
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef MONITOR_INC_UNI_MONITOR_H_
#define MONITOR_INC_UNI_MONITOR_H_

#ifdef __cplusplus
extern "C" {
#endif

int MonitorInitialize();
int MonitorFinalize();
int MonitorFunctionBegin(const char *file, const char *function, int line);
int MonitorFunctionEnd(const char *file, const char *function, int line);
int MonitorPrintStatus();

#define MonitorStepInto() MonitorFunctionBegin(__FILE__, __FUNCTION__, __LINE__)
#define MonitorStepOut()  MonitorFunctionEnd(__FILE__, __FUNCTION__, __LINE__)

#ifdef __cplusplus
}
#endif
#endif  //MONITOR_INC_UNI_MONITOR_H_
