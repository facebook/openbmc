/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openbmc/cpldupdate_dll.h>

/*
 * The following structures and ioctl macros are copied from kernel header
 * "include/uapi/linux/jtag.h".
 */

struct tck_bitbang {
   unsigned char  tms;
   unsigned char  tdi;
   unsigned char  tdo;
} __attribute__((__packed__));

struct run_cycle_param {
    unsigned char     tdi;        // TDI bit value to write
    unsigned char     tms;        // TMS bit value to write
    unsigned char     tck;        // TCK bit value to write
    unsigned char     tdo;        // TDO bit value to read
};

/* ioctl interface */
#define __JTAG_IOCTL_MAGIC  0xb2
#define JTAG_IOCBITBANG _IOW(__JTAG_IOCTL_MAGIC, 6, unsigned int)
#define JTAG_RUN_CYCLE  _IOR(__JTAG_IOCTL_MAGIC, 11, struct run_cycle_param)

struct jtag_ctx {
    int fd;
    int do_bitbang;
};

void cpldupdate_dll_free(void *ctx)
{
    struct jtag_ctx *jctx;

    if (ctx != NULL) {
        jctx = (struct jtag_ctx *)ctx;
        if (jctx->fd >= 0) {
            close(jctx->fd);
        }
        free(jctx);
    }
}

static const char *dev_name = "/dev/jtag0";
int cpldupdate_dll_init(int argc, const char *const argv[], void **ctx)
{
    int i, rc;
    struct jtag_ctx *new_ctx;
    new_ctx = calloc(sizeof(*new_ctx), 1);
    if (!new_ctx) {
        rc = ENOMEM;
        goto err_out;
    }

    new_ctx->do_bitbang = -1;
    for (i = 0; i < argc - 1; ++i) {
        if (strcasecmp(argv[i], "--ioctl") == 0) {
            ++i;
            if (strcasecmp(argv[i], "RUN_CYCLE") == 0) {
                new_ctx->do_bitbang = 0;
            } else if (strcasecmp(argv[i], "IOCBITBANG") == 0) {
                new_ctx->do_bitbang = 1;
            } else {
                printf("Error: unrecognized ioctl %s\n", argv[i]);
                rc = EFAULT;
                goto err_out;
            }
            break;
        }
    }
    if (new_ctx->do_bitbang < 0) {
        printf("Error: unrecognized ioctl %s\n", argv[i]);
        rc = EFAULT;
        goto err_out;
    }

    new_ctx->fd = open(dev_name, O_RDWR);
    if (new_ctx->fd < 0) {
        rc = EFAULT;
        goto err_out;
    }

    *ctx = new_ctx;
    return 0;

err_out:
  cpldupdate_dll_free(new_ctx);
  return rc;
}

static int do_bitbang(struct jtag_ctx *jctx,
                      cpldupdate_pin_value_en tms,
                      cpldupdate_pin_value_en tdi,
                      cpldupdate_pin_value_en *tdo)
{
    struct tck_bitbang bitbang;

    bitbang.tms = tms == CPLDUPDATE_PIN_VALUE_HIGH ? 1 : 0;
    bitbang.tdi = tdi == CPLDUPDATE_PIN_VALUE_HIGH ? 1 : 0;
    bitbang.tdo = 0;
    int rc = ioctl(jctx->fd, JTAG_IOCBITBANG, &bitbang);
    if (rc < 0) {
        return errno;
    }

    if (tdo) {
        *tdo = bitbang.tdo ? CPLDUPDATE_PIN_VALUE_HIGH : CPLDUPDATE_PIN_VALUE_LOW;
    }

    return 0;
}

static int do_run_cycle(struct jtag_ctx *jctx,
                        cpldupdate_pin_value_en tms,
                        cpldupdate_pin_value_en tdi,
                        cpldupdate_pin_value_en *tdo)
{
    struct run_cycle_param run_cycle;
    run_cycle.tms = tms == CPLDUPDATE_PIN_VALUE_HIGH ? 1 : 0;
    run_cycle.tdi = tdi == CPLDUPDATE_PIN_VALUE_HIGH ? 1 : 0;
    run_cycle.tck = 0;
    run_cycle.tdo = 0;
    int rc = ioctl(jctx->fd, JTAG_RUN_CYCLE, &run_cycle);
    if (rc < 0) {
        return errno;
    }

    run_cycle.tck = 1;
    rc = ioctl(jctx->fd, JTAG_RUN_CYCLE, &run_cycle);
    if (rc < 0) {
        return errno;
    }

    if (tdo) {
        *tdo = run_cycle.tdo ? CPLDUPDATE_PIN_VALUE_HIGH : CPLDUPDATE_PIN_VALUE_LOW;
    }

    return 0;
}

int cpldupdate_dll_do_io(void *ctx,
                         cpldupdate_pin_value_en tms,
                         cpldupdate_pin_value_en tdi,
                         cpldupdate_pin_value_en *tdo)
{
    struct jtag_ctx *jctx = (struct jtag_ctx *)ctx;
    if (jctx->do_bitbang) {
        return do_bitbang(jctx, tms, tdi, tdo);
    } else {
        return do_run_cycle(jctx, tms, tdi, tdo);
    }

    return 0;
}
