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
#include <openbmc/gpio.h>

struct gpio_ctx {
  gpio_st gpio[CPLDUPDATE_PIN_MAX];
  int gpio_init_done[CPLDUPDATE_PIN_MAX];
};

void cpldupdate_dll_free(void *ctx) {
  struct gpio_ctx *gctx = (struct gpio_ctx *)ctx;
  int i;

  if (!gctx) {
    return;
  }

  for (i = 0; i < CPLDUPDATE_PIN_MAX; i++) {
    if (gctx->gpio_init_done[i]) {
      gpio_close(&gctx->gpio[i]);
    }
  }
  free(gctx);
}

int cpldupdate_dll_init(int argc, const char *const argv[], void **ctx) {
  int rc = 0;
  int i = 0;
  int gpio_num;
  int init_total = 0;
  cpldupdate_pin_en pin;
  struct gpio_ctx *new_ctx = NULL;

  new_ctx = calloc(sizeof(*new_ctx), 1);
  if (!new_ctx) {
    rc = ENOMEM;
    goto err_out;
  }

  /* parse */
  for (i = 0; i < argc - 1; ) {
    pin = -1;
    if (!strcasecmp(argv[i], "--tdi")) {
      pin = CPLDUPDATE_PIN_TDI;
    } else if (!strcasecmp(argv[i], "--tdo")) {
      pin = CPLDUPDATE_PIN_TDO;
    } else if (!strcasecmp(argv[i], "--tms")) {
      pin = CPLDUPDATE_PIN_TMS;
    } else if (!strcasecmp(argv[i], "--tck")) {
      pin = CPLDUPDATE_PIN_TCK;
    }
    if (pin == -1) {
      /* unknown option, skip */
      i++;
      continue;
    }
    gpio_num = atoi(argv[i + 1]);
    i += 2;
    rc = gpio_open(&new_ctx->gpio[pin], gpio_num);
    if (rc) {
      goto err_out;
    }
    new_ctx->gpio_init_done[pin] = 1;
    init_total ++;
  }

  if (init_total != CPLDUPDATE_PIN_MAX) {
    /* not all pins are specified */
    rc = EINVAL;
    goto err_out;
  }

  /* prepare the directions */
  if (gpio_change_direction(&new_ctx->gpio[CPLDUPDATE_PIN_TDI],
                            GPIO_DIRECTION_OUT) ||
      gpio_change_direction(&new_ctx->gpio[CPLDUPDATE_PIN_TDO],
                            GPIO_DIRECTION_IN) ||
      gpio_change_direction(&new_ctx->gpio[CPLDUPDATE_PIN_TMS],
                            GPIO_DIRECTION_OUT) ||
      gpio_change_direction(&new_ctx->gpio[CPLDUPDATE_PIN_TCK],
                            GPIO_DIRECTION_OUT)) {
    rc = EFAULT;
    goto err_out;
  }

  *ctx = new_ctx;
  return 0;

 err_out:
  cpldupdate_dll_free(new_ctx);
  return rc;
}

int cpldupdate_dll_write_pin(void *ctx, cpldupdate_pin_en pin,
                             cpldupdate_pin_value_en value) {
  struct gpio_ctx *gctx = (struct gpio_ctx *)ctx;

  if (!gctx || pin >= CPLDUPDATE_PIN_MAX) {
    return EINVAL;
  }

  gpio_write(&gctx->gpio[pin],
             (value == CPLDUPDATE_PIN_VALUE_LOW)
             ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
  return 0;
}

int cpldupdate_dll_read_pin(void *ctx, cpldupdate_pin_en pin,
                            cpldupdate_pin_value_en *value) {
  struct gpio_ctx *gctx = (struct gpio_ctx *)ctx;
  gpio_value_en v;

  if (!gctx || pin >= CPLDUPDATE_PIN_MAX || !value) {
    return EINVAL;
  }

  v = gpio_read(&gctx->gpio[pin]);
  *value = (v == GPIO_VALUE_LOW)
    ? CPLDUPDATE_PIN_VALUE_LOW : CPLDUPDATE_PIN_VALUE_HIGH;

  return 0;
}
