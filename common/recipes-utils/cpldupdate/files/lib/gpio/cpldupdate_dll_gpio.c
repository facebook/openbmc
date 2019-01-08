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
#include <assert.h>

#include <openbmc/cpldupdate_dll.h>
#include <openbmc/libgpio.h>

struct gpio_ctx {
  gpio_desc_t* gpio_desc[CPLDUPDATE_PIN_MAX];
  int gpio_init_done[CPLDUPDATE_PIN_MAX];
};

void cpldupdate_dll_free(void *ctx)
{
  int i;
  struct gpio_ctx *gctx;

  if (ctx != NULL) {
    gctx = (struct gpio_ctx *)ctx;

    for (i = 0; i < CPLDUPDATE_PIN_MAX; i++) {
      if (gctx->gpio_init_done[i])
        gpio_close(gctx->gpio_desc[i]);
    }
    free(gctx);
  }
}

static cpldupdate_pin_en cpldupdate_match_pin(const char *opt_str)
{
  int i;
  static struct {
    cpldupdate_pin_en pin;
    const char *opt;
  } cpldupdate_opt_table[] = {
    {CPLDUPDATE_PIN_TDI, "--tdi"},
    {CPLDUPDATE_PIN_TDO, "--tdo"},
    {CPLDUPDATE_PIN_TMS, "--tms"},
    {CPLDUPDATE_PIN_TCK, "--tck"},
    /* This is the last entry. */
    {-1, NULL},
  };

  for (i = 0; cpldupdate_opt_table[i].opt != NULL; i++) {
    if (strcasecmp(opt_str, cpldupdate_opt_table[i].opt) == 0)
      return cpldupdate_opt_table[i].pin;
  }

  return -1;
}

int cpldupdate_dll_init(int argc, const char *const argv[], void **ctx)
{
  int i, rc, total;
  const char *shadow;
  cpldupdate_pin_en pin;
  struct gpio_ctx *new_ctx;
  struct {
     cpldupdate_pin_en pin;
     gpio_direction_t direction;
  } pin_directions[] = {
    {CPLDUPDATE_PIN_TDI, GPIO_DIRECTION_OUT},
    {CPLDUPDATE_PIN_TDO, GPIO_DIRECTION_IN},
    {CPLDUPDATE_PIN_TMS, GPIO_DIRECTION_OUT},
    {CPLDUPDATE_PIN_TCK, GPIO_DIRECTION_OUT},
    {-1, GPIO_DIRECTION_INVALID},
  };

  new_ctx = calloc(sizeof(*new_ctx), 1);
  if (!new_ctx) {
    rc = ENOMEM;
    goto err_out;
  }

  /* parse command line arguments. */
  for (i = total = 0; i < argc - 1; i++) {
    pin = cpldupdate_match_pin(argv[i]);
    if (pin == -1) {
      continue; /* skip unknown option */
    }

    i++;
    shadow = argv[i];
    new_ctx->gpio_desc[pin] = gpio_open_by_shadow(shadow);
    if (new_ctx->gpio_desc[pin] == NULL) {
      rc = errno;
      goto err_out;
    }
    new_ctx->gpio_init_done[pin] = 1;
    total++;
  }

  if (total != CPLDUPDATE_PIN_MAX) {
    /* not all pins are specified */
    rc = EINVAL;
    goto err_out;
  }

  /* prepare the directions */
  for (i = 0; pin_directions[i].pin != -1; i++) {
    pin = pin_directions[i].pin;
    if (gpio_set_direction(new_ctx->gpio_desc[pin],
                           pin_directions[i].direction) != 0) {
      rc = errno;
      goto err_out;
    }
  }

  *ctx = new_ctx;
  return 0;

err_out:
  cpldupdate_dll_free(new_ctx);
  return rc;
}

int cpldupdate_dll_write_pin(void *ctx,
                             cpldupdate_pin_en pin,
                             cpldupdate_pin_value_en value)
{
  gpio_value_t g_value;
  struct gpio_ctx *gctx = (struct gpio_ctx *)ctx;

  if (gctx == NULL ||
      pin < 0 ||
      pin >= CPLDUPDATE_PIN_MAX ||
      gctx->gpio_init_done[pin] == 0) {
    return EINVAL;
  }

  assert(gctx->gpio_desc[pin] != NULL);
  g_value = (value == CPLDUPDATE_PIN_VALUE_LOW ?
             GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
  if (gpio_set_value(gctx->gpio_desc[pin], g_value) != 0)
    return errno;

  return 0;
}

int cpldupdate_dll_read_pin(void *ctx,
                            cpldupdate_pin_en pin,
                            cpldupdate_pin_value_en *value)
{
  gpio_value_t g_value;
  struct gpio_ctx *gctx = (struct gpio_ctx *)ctx;

  if (gctx == NULL ||
      value == NULL ||
      pin < 0 ||
      pin >= CPLDUPDATE_PIN_MAX ||
      gctx->gpio_init_done[pin] == 0) {
    return EINVAL;
  }

  assert(gctx->gpio_desc[pin] != NULL);
  if (gpio_get_value(gctx->gpio_desc[pin], &g_value) != 0)
    return errno;

  *value = (g_value == GPIO_VALUE_LOW ?
            CPLDUPDATE_PIN_VALUE_LOW : CPLDUPDATE_PIN_VALUE_HIGH);

  return 0;
}
