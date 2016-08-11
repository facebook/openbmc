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
#ifndef CPLDUPDATE_DLL_H
#define CPLDUPDATE_DLL_H

typedef enum {
  CPLDUPDATE_PIN_TDI,
  CPLDUPDATE_PIN_TDO,
  CPLDUPDATE_PIN_TMS,
  CPLDUPDATE_PIN_TCK,
  CPLDUPDATE_PIN_MAX,
} cpldupdate_pin_en;

typedef enum {
  CPLDUPDATE_PIN_VALUE_LOW = 0,
  CPLDUPDATE_PIN_VALUE_HIGH = 1,
} cpldupdate_pin_value_en;

typedef int (* cpldupdate_dll_init_fn)(int argc, const char * const argv[],
                                       void**ctx);
typedef int (* cpldupdate_dll_write_pin_fn)(void *ctx, cpldupdate_pin_en pin,
                                            cpldupdate_pin_value_en value);
typedef int (* cpldupdate_dll_read_pin_fn)(void *ctx, cpldupdate_pin_en pin,
                                           cpldupdate_pin_value_en *value);
typedef void (* cpldupdate_dll_free_fn)(void *ctx);

#define CPLDUPDATE_DLL_INIT_FN_NAME "cpldupdate_dll_init"
#define CPLDUPDATE_DLL_WRITE_PIN_FN_NAME "cpldupdate_dll_write_pin"
#define CPLDUPDATE_DLL_READ_PIN_FN_NAME "cpldupdate_dll_read_pin"
#define CPLDUPDATE_DLL_FREE_FN_NAME "cpldupdate_dll_free"

struct cpldupdate_helper_st {
  void *dll_hdl;
  void *func_ctx;
  cpldupdate_dll_init_fn init;
  cpldupdate_dll_write_pin_fn write_pin;
  cpldupdate_dll_read_pin_fn read_pin;
  cpldupdate_dll_free_fn free;
};

int cpldupdate_helper_open(const char* dll_name, struct cpldupdate_helper_st *helper);
void cpldupdate_helper_close(struct cpldupdate_helper_st *helper);

static int cpldupdate_helper_init(
    struct cpldupdate_helper_st *helper, int argc, const char *const argv[]) {
  return helper->init(argc, argv, &helper->func_ctx);
}

static int cpldupdate_helper_write_pin(
    struct cpldupdate_helper_st *helper,
    cpldupdate_pin_en pin, cpldupdate_pin_value_en value) {
  return helper->write_pin(helper->func_ctx, pin, value);
}

static int cpldupdate_helper_read_pin(
    struct cpldupdate_helper_st *helper,
    cpldupdate_pin_en pin, cpldupdate_pin_value_en *value) {
  return helper->read_pin(helper->func_ctx, pin, value);
}

#endif
