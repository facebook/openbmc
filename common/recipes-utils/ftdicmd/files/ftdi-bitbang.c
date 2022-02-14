/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 * https://github.com/aehparta/ftdi-bitbang
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "ftdi-bitbang.h"


struct ftdi_bitbang_context *ftdi_bitbang_init(struct ftdi_context *ftdi, int mode, int load_state)
{
    struct ftdi_bitbang_context *dev = malloc(sizeof(struct ftdi_bitbang_context));
    if (!dev) {
        return NULL;
    }
    memset(dev, 0, sizeof(*dev));

    /* save args */
    dev->ftdi = ftdi;

    /* load state if requested */
    if (load_state) {
        ftdi_bitbang_load_state(dev);
    }
    if (dev->state.mode != BITMODE_RESET && mode != BITMODE_RESET && dev->state.mode != mode) {
        /* force writing of all pins since mode was changed */
        dev->state.l_changed = 0xff;
        dev->state.h_changed = 0xff;
    }

    if (mode != BITMODE_MPSSE &&
            (mode == BITMODE_BITBANG ||
             dev->state.mode == BITMODE_RESET ||
             dev->state.mode == BITMODE_BITBANG)) {
        /* do not actually set bitmode here, might not know full state yet */
        dev->state.mode = BITMODE_BITBANG;
        /* set baud rate */
        /** @todo add support for changing baud rate */
        if (ftdi_set_baudrate(dev->ftdi, 1e6)) {
            free(dev);
            return  NULL;
        }
    } else if (ftdi->type == TYPE_4232H) {
        /* there is no point in supporting MPSSE within this library when using FT4232H */
        free(dev);
        return NULL;
    } else {
        /* set bitmode to mpsse */
        if (ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE)) {
            free(dev);
            return NULL;
        }
        dev->state.mode = BITMODE_MPSSE;
    }

    return dev;
}

void ftdi_bitbang_free(struct ftdi_bitbang_context *dev)
{
    free(dev);
}

int ftdi_bitbang_set_pin(struct ftdi_bitbang_context *dev, int bit, int value)
{
    /* if device is not in MPSSE mode, it only supports pins through 0-7 */
    if (dev->state.mode != BITMODE_MPSSE && bit >= 8) {
        return -1;
    }
    /* get which byte it is, higher or lower */
    uint8_t *v = bit < 8 ? &dev->state.l_value : &dev->state.h_value;
    uint8_t *c = bit < 8 ? &dev->state.l_changed : &dev->state.h_changed;
    /* get bit in byte mask */
    uint8_t mask = 1 << (bit - (bit < 8 ? 0 : 8));
    /* save old pin state */
    uint8_t was = (*v) & mask;
    /* clear and set new value */
    (*v) = ((*v) & ~mask) | (value ? mask : 0);
    /* set changed if actually changed */
    (*c) |= was != ((*v) & mask) ? mask : 0;
    return 0;
}

int ftdi_bitbang_set_io(struct ftdi_bitbang_context *dev, int bit, int io)
{
    /* if device is not in MPSSE mode, it only supports pins through 0-7 */
    if (dev->state.mode != BITMODE_MPSSE && bit >= 8) {
        return -1;
    }
    /* get which byte it is, higher or lower */
    uint8_t *v = bit < 8 ? &dev->state.l_io : &dev->state.h_io;
    uint8_t *c = bit < 8 ? &dev->state.l_changed : &dev->state.h_changed;
    /* get bit in byte mask */
    uint8_t mask = 1 << (bit - (bit < 8 ? 0 : 8));
    /* save old pin state */
    uint8_t was = (*v) & mask;
    /* clear and set new value */
    (*v) = ((*v) & ~mask) | (io ? mask : 0);
    /* set changed */
    (*c) |= was != ((*v) & mask) ? mask : 0;
    return 0;
}

int ftdi_bitbang_write(struct ftdi_bitbang_context *dev)
{
    if (dev->state.mode == BITMODE_MPSSE) {
        uint8_t buf[6];
        int n = 0;
        if (dev->state.l_changed) {
            buf[n++] = 0x80;
            buf[n++] = dev->state.l_value;
            buf[n++] = dev->state.l_io;
            dev->state.l_changed = 0;
        }
        if (dev->state.h_changed) {
            buf[n++] = 0x82;
            buf[n++] = dev->state.h_value;
            buf[n++] = dev->state.h_io;
            dev->state.h_changed = 0;
        }
        if (n > 0) {
            return ftdi_write_data(dev->ftdi, buf, n) > 0 ? 0 : -1;
        }
        return 0;
    } else if (dev->state.mode == BITMODE_BITBANG) {
        if (!dev->state.l_changed) {
            return 0;
        }
        /** @todo setting bitmode every time, should fix */
        if (ftdi_set_bitmode(dev->ftdi, dev->state.l_io, BITMODE_BITBANG)) {
            return -1;
        }
        if (ftdi_write_data(dev->ftdi, &dev->state.l_value, 1) < 1) {
            return -1;
        }
        dev->state.l_changed = 0;
        dev->state.h_changed = 0;
        return 0;
    }

    return -1;
}

int ftdi_bitbang_read_low(struct ftdi_bitbang_context *dev)
{
    if (dev->state.mode == BITMODE_MPSSE) {
        uint8_t buf[1] = { 0x81 };
        ftdi_usb_purge_rx_buffer(dev->ftdi);
        if (ftdi_write_data(dev->ftdi, &buf[0], 1) != 1) {
            return -1;
        }
        if (ftdi_read_data(dev->ftdi, &buf[0], 1) != 1) {
            return -1;
        }
        return (int)buf[0];
    } else if (dev->state.mode == BITMODE_BITBANG) {
        /** @todo setting bitmode every time, should fix */
        if (ftdi_set_bitmode(dev->ftdi, dev->state.l_io, BITMODE_BITBANG)) {
            return -1;
        }
        uint8_t pins;
        if (ftdi_read_pins(dev->ftdi, &pins)) {
            return -1;
        }
        return pins;
    }
    return -1;

}

int ftdi_bitbang_read_high(struct ftdi_bitbang_context *dev)
{
    /* if device is not in MPSSE mode, it only supports pins through 0-7 */
    if (dev->state.mode != BITMODE_MPSSE) {
        return -1;
    }

    uint8_t buf[1] = { 0x83 };
    ftdi_usb_purge_rx_buffer(dev->ftdi);
    if (ftdi_write_data(dev->ftdi, &buf[0], 1) != 1) {
        return -1;
    }
    if (ftdi_read_data(dev->ftdi, &buf[0], 1) != 1) {
        return -1;
    }
    return (int)buf[0];
}

int ftdi_bitbang_read(struct ftdi_bitbang_context *dev)
{
    int h = 0, l = 0;
    l = ftdi_bitbang_read_low(dev);
    /* if device is not in MPSSE mode, it only supports pins through 0-7 */
    if (dev->state.mode == BITMODE_MPSSE) {
        h = ftdi_bitbang_read_high(dev);
    }
    if (l < 0 || h < 0) {
        return -1;
    }
    return (h << 8) | l;
}

int ftdi_bitbang_read_pin(struct ftdi_bitbang_context *dev, uint8_t pin)
{
    if (pin <= 7 && pin >= 0) {
        return (ftdi_bitbang_read_low(dev) & (1 << pin)) ? 1 : 0;
    } else if (pin >= 8 && pin <= 15) {
        /* if device is not in MPSSE mode, it only supports pins through 0-7 */
        if (dev->state.mode != BITMODE_MPSSE) {
            return -1;
        }
        return (ftdi_bitbang_read_high(dev) & (1 << (pin - 8))) ? 1 : 0;
    }
    return -1;
}

static char *_generate_state_filename(struct ftdi_bitbang_context *dev)
{
    int i;
    char *state_filename = NULL;
    uint8_t bus, addr, port;
    libusb_device *usb_dev = libusb_get_device(dev->ftdi->usb_dev);
    /* create unique device filename */
    bus = libusb_get_bus_number(usb_dev);
    addr = libusb_get_device_address(usb_dev);
    port = libusb_get_port_number(usb_dev);
    i = asprintf(&state_filename, "%sftdi-bitbang-device-state-%03d.%03d.%03d-%d.%d-%d", _PATH_TMP, bus, addr, port, dev->ftdi->interface, dev->ftdi->type, (unsigned int)getuid());
    if (i < 1 || !state_filename) {
        return NULL;
    }
    return state_filename;
}

int ftdi_bitbang_load_state(struct ftdi_bitbang_context *dev)
{
    int fd, n;
    char *state_filename;
    struct ftdi_bitbang_state state;
    /* generate filename and open file */
    state_filename = _generate_state_filename(dev);
    if (!state_filename) {
        return -1;
    }
    fd = open(state_filename, O_RDONLY, 0755);
    if (fd < 0) {
        free(state_filename);
        return -1;
    }
    /* read contents */
    n = read(fd, &state, sizeof(state));
    if (n == sizeof(state)) {
        memcpy(&dev->state, &state, sizeof(state));
    }
    /* free resources */
    free(state_filename);
    close(fd);
    return 0;
}

int ftdi_bitbang_save_state(struct ftdi_bitbang_context *dev)
{
    int fd, n;
    char *state_filename;
    /* generate filename and open file */
    state_filename = _generate_state_filename(dev);
    if (!state_filename) {
        return -1;
    }
    fd = open(state_filename, O_WRONLY | O_CREAT, 0755);
    if (fd < 0) {
        free(state_filename);
        return -1;
    }
    /* write contents */
    n = write(fd, &dev->state, sizeof(dev->state));
    if (n == sizeof(dev->state)) {
        /* set accessible only by current user */
        fchmod(fd, 0600);
    }
    /* free resources */
    free(state_filename);
    close(fd);
    return 0;
}
