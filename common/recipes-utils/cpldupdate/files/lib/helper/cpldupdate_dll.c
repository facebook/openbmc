/*
 * Copyright 2016-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "cpldupdate_dll.h"

int cpldupdate_helper_open(const char *dll_name,
                           struct cpldupdate_helper_st *helper) {
  int rc = 0;

  if (!dll_name || !helper) {
    return EINVAL;
  }

  memset(helper, sizeof(*helper), 0);

  helper->dll_hdl = dlopen(dll_name, RTLD_LAZY);
  if (!helper->dll_hdl) {
    rc = errno;
    fprintf(stderr, "Failed to open %s: %s\n", dll_name, dlerror());
    goto err_out;
  }

#define _OPEN_SYM(field, sym) do {                            \
  helper->field = dlsym(helper->dll_hdl, sym);                \
  if (!helper->field) {                                       \
    rc = errno;                                               \
    fprintf(stderr, "Failed to find function %s in %s: %s\n", \
            sym, dll_name, dlerror());                        \
    goto err_out;                                             \
  }                                                           \
} while(0)

  _OPEN_SYM(init, CPLDUPDATE_DLL_INIT_FN_NAME);
  _OPEN_SYM(write_pin, CPLDUPDATE_DLL_WRITE_PIN_FN_NAME);
  _OPEN_SYM(read_pin, CPLDUPDATE_DLL_READ_PIN_FN_NAME);
  _OPEN_SYM(free, CPLDUPDATE_DLL_FREE_FN_NAME);

#undef _OPEN_SYM

  return 0;

 err_out:
  if (helper->dll_hdl) {
    dlclose(helper->dll_hdl);
  }
  memset(helper, sizeof(*helper), 0);

  return rc;
}

void cpldupdate_helper_close(struct cpldupdate_helper_st *helper) {
  if (!helper) {
    return;
  }

  if (helper->func_ctx) {
    helper->free(helper->func_ctx);
  }

  if (helper->dll_hdl) {
    dlclose(helper->dll_hdl);
  }

  memset(helper, sizeof(*helper), 0);
}
