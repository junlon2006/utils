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
 * Description : uni_dlopen.c
 * Author      : junlon2006@163.com
 * Date        : 2019.05.16
 *
 **************************************************************************/
#include "uni_dlopen.h"

#include "uni_iot.h"
#include "uni_log.h"
#include <dlfcn.h>
#include <gnu/lib-names.h>

#define DLOPEN_TAG  "dlopen"

DlOpenHandle DlOpenLoadSharedLibrary(const char *libname) {
  DlOpenHandle handle = NULL;
  if (NULL == libname) {
    LOGE(DLOPEN_TAG, "libname=%p invalid", libname);
    return (DlOpenHandle)NULL;
  }
  if (NULL == (handle = (DlOpenHandle)dlopen(libname, RTLD_LAZY))) {
    LOGE(DLOPEN_TAG, "dlopen failed, [%s]", dlerror());
  }
  return handle;
}

void DlOpenSharedLibraryClose(DlOpenHandle handle) {
  if (NULL == handle) {
    LOGE(DLOPEN_TAG, "handle=%p invalid", handle);
    return;
  }
  dlclose(handle);
}

void* DlOpenLoadSharedLibrarySymbol(DlOpenHandle handle, const char *symbol) {
  void *sym = NULL;
  if (NULL == handle) {
    LOGE(DLOPEN_TAG, "handle=%p invalid", handle);
    return NULL;
  }
  if (NULL == (sym = dlsym(handle, symbol))) {
    LOGE(DLOPEN_TAG, "load symbol[%s] failed, [%s]", symbol, dlerror());
  }
  return sym;
}
