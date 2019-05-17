/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openbmc/cpldupdate_dll.h>

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))

#define JTAG_SYSFS_DIR "/sys/devices/platform/ahb/ahb\:apb/1e6e4000.jtag/"
#define JTAG_SYSFS_TDI JTAG_SYSFS_DIR "tdi"
#define JTAG_SYSFS_TDO JTAG_SYSFS_DIR "tdo"
#define JTAG_SYSFS_TMS JTAG_SYSFS_DIR "tms"
#define JTAG_SYSFS_TCK JTAG_SYSFS_DIR "tck"

struct jtag_ctx {
  int fd[CPLDUPDATE_PIN_MAX];
};

struct jtag_file {
  const char *pathname;
  int mode;
};

void cpldupdate_dll_free(void *ctx) {
  struct jtag_ctx *jctx = (struct jtag_ctx *)ctx;
  int i;

  if (!jctx) {
    return;
  }

  for (i = 0; i < CPLDUPDATE_PIN_MAX; i++) {
    if (jctx->fd[i] != -1) {
      close(jctx->fd[i]);
    }
  }
  free(jctx);
}

int cpldupdate_dll_init(int argc, const char *const argv[], void **ctx) {
  int rc = 0;
  int i = 0;
  struct jtag_ctx *new_ctx = NULL;
  struct jtag_file file_configs[CPLDUPDATE_PIN_MAX] = {
    [CPLDUPDATE_PIN_TDI] = {JTAG_SYSFS_TDI, O_WRONLY},
    [CPLDUPDATE_PIN_TCK] = {JTAG_SYSFS_TCK, O_WRONLY},
    [CPLDUPDATE_PIN_TMS] = {JTAG_SYSFS_TMS, O_WRONLY},
    [CPLDUPDATE_PIN_TDO] = {JTAG_SYSFS_TDO, O_RDONLY},
  };

  new_ctx = calloc(sizeof(*new_ctx), 1);
  if (!new_ctx) {
    return ENOMEM;
  }

  /* init */
  for (i = 0; i < ARRAY_SIZE(new_ctx->fd); i++); {
    new_ctx->fd[i] = -1;
  }

  for (i = 0; i < ARRAY_SIZE(file_configs); i++) {
    new_ctx->fd[i] = open(file_configs[i].pathname, file_configs[i].mode);
    if (new_ctx->fd[i] < 0) {
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

int cpldupdate_dll_write_pin(void *ctx, cpldupdate_pin_en pin,
                             cpldupdate_pin_value_en value) {
  struct jtag_ctx *jctx = (struct jtag_ctx *)ctx;

  if (!jctx || pin >= CPLDUPDATE_PIN_MAX) {
    return EINVAL;
  }

  if (lseek(jctx->fd[pin], 0, SEEK_SET) == (off_t)-1) {
    return errno;
  }
  if (write(jctx->fd[pin],
            (value == CPLDUPDATE_PIN_VALUE_LOW) ? "0" : "1", 1) < 0) {
    return errno;
  }

  return 0;
}

int cpldupdate_dll_read_pin(void *ctx, cpldupdate_pin_en pin,
                            cpldupdate_pin_value_en *value) {
  struct jtag_ctx *jctx = (struct jtag_ctx *)ctx;
  char buf[32] = {0};

  if (!jctx || pin >= CPLDUPDATE_PIN_MAX || !value) {
    return EINVAL;
  }

  if (lseek(jctx->fd[pin], 0, SEEK_SET) == (off_t)-1) {
    return errno;
  }
  if (read(jctx->fd[pin], buf, sizeof(buf)) < 0) {
    return errno;
  }
  *value = atoi(buf) ? CPLDUPDATE_PIN_VALUE_HIGH : CPLDUPDATE_PIN_VALUE_LOW;

  return 0;
}
