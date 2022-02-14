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

#ifndef __FTDI_bitbang_H__
#define __FTDI_bitbang_H__

#include <stdlib.h>
#include <libftdi1/ftdi.h>

struct ftdi_bitbang_state {
    uint8_t l_value;
    uint8_t l_changed;
    uint8_t l_io;
    uint8_t h_value;
    uint8_t h_changed;
    uint8_t h_io;
    /* BITMODE_BITBANG or BITMODE_MPSSE */
    int mode;
};
struct ftdi_bitbang_context {
    struct ftdi_context *ftdi;
    struct ftdi_bitbang_state state;
};

struct ftdi_bitbang_context *ftdi_bitbang_init(struct ftdi_context *ftdi, int mode, int load_state);
void ftdi_bitbang_free(struct ftdi_bitbang_context *dev);

int ftdi_bitbang_set_pin(struct ftdi_bitbang_context *dev, int bit, int value);
int ftdi_bitbang_set_io(struct ftdi_bitbang_context *dev, int bit, int io);

int ftdi_bitbang_write(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read_low(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read_high(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read_pin(struct ftdi_bitbang_context *dev, uint8_t pin);

int ftdi_bitbang_load_state(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_save_state(struct ftdi_bitbang_context *dev);


#endif /* __FTDI_bitbang_H__ */
