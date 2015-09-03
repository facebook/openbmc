/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#ifndef BITBANG_H
#define BITBANG_H

#include <stdint.h>

typedef enum {
  BITBANG_CLK_PIN,
  BITBANG_DATA_IN,
  BITBANG_DATA_OUT,
} bitbang_pin_type_en;

typedef enum {
  BITBANG_PIN_LOW = 0,
  BITBANG_PIN_HIGH = 1,
} bitbang_pin_value_en;

typedef enum {
  BITBANG_CLK_EDGE_RISING,
  BITBANG_CLK_EDGE_FALLING,
} bitbang_clk_edge_en;

typedef bitbang_pin_value_en  (* bitbang_pin_func)(
    bitbang_pin_type_en pin, bitbang_pin_value_en value, void *context);

typedef struct {
  bitbang_pin_value_en bbi_clk_start;
  bitbang_clk_edge_en bbi_data_out;
  bitbang_clk_edge_en bbi_data_in;
  uint32_t bbi_freq;
  bitbang_pin_func bbi_pin_f;
  void *bbi_context;
} bitbang_init_st;

typedef struct bitbang_handle bitbang_handle_st;

void bitbang_init_default(bitbang_init_st *init);
bitbang_handle_st* bitbang_open(const bitbang_init_st *init);
void bitbang_close(bitbang_handle_st *hdl);

typedef struct {
  uint32_t bbio_in_bits;
  uint32_t bbio_out_bits;
  uint8_t *bbio_dout;
  uint8_t *bbio_din;
} bitbang_io_st;

int bitbang_io(const bitbang_handle_st *hdl, bitbang_io_st *io);

#endif
