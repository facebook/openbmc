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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <openbmc/cpldupdate_dll.h>

static const char* const pin_names[CPLDUPDATE_PIN_MAX] = {
  [CPLDUPDATE_PIN_TDI] = "TDI",
  [CPLDUPDATE_PIN_TDO] = "TDO",
  [CPLDUPDATE_PIN_TMS] = "TMS",
  [CPLDUPDATE_PIN_TCK] = "TCK",
};

int cpldupdate_dll_init(int argc, const char *const argv[], void **ctx) {
  int i = 0;
  printf("CPLDUPDATE: init with: ");
  for (i = 0; i < argc; i++) {
    printf("'%s', ", argv[i]);
  }
  printf("\n");
  /* no need to return handler this case */
  *ctx = (void *) 1;
  return 0;
}

int cpldupdate_dll_write_pin(void *ctx, cpldupdate_pin_en pin,
                             cpldupdate_pin_value_en value) {
  if (!ctx || pin >= CPLDUPDATE_PIN_MAX) {
    return EINVAL;
  }

  printf("CPLDUPDATE: write %s %d\n", pin_names[pin], value ? 1 : 0);
  return 0;
}

int cpldupdate_dll_read_pin(void *ctx, cpldupdate_pin_en pin,
                            cpldupdate_pin_value_en *value) {
  if (!ctx || pin >= CPLDUPDATE_PIN_MAX || !value) {
    return EINVAL;
  }

  *value = CPLDUPDATE_PIN_VALUE_LOW;
  printf("CPLDUPDATE: read %s\n", pin_names[pin]);
  return 0;
}

void cpldupdate_dll_free(void *ctx) {
  return;
}
