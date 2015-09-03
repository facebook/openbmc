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
//#define DEBUG
//#define VERBOSE

#include <stdlib.h>
#include <unistd.h>

#include <openbmc/gpio.h>
#include <openbmc/log.h>

#include "bitbang.h"

void usage()
{
  fprintf(stderr,
          "Usage:\n"
          "spi-bb: -s <GPIO for CS> [-S <HIGH|low>]\n"
          "        -c <GPIO for CLK> [-C <HIGH|low>]\n"
          "        -o <GPIO for MOSI> [-O <rising|FALLING>]\n"
          "        -i <GPIO for MISO> [-I <RISING|falling>]\n"
          "        [-b]\n"
          "        < [-r <number of bits to read>]\n"
          "          [-w <number of bits to write> <byte 1> [... byte N]>\n\n"
          "Note: If both '-r' and '-w' are provided, 'write' will be performed\n"
          "      before 'read'.\n");
}

typedef struct {
  gpio_st sc_clk;
  gpio_st sc_mosi;
  gpio_st sc_miso;
} spi_context_st;

bitbang_pin_value_en spi_pin_f(
    bitbang_pin_type_en pin, bitbang_pin_value_en value, void *context)
{
  spi_context_st *ctx = (spi_context_st *)context;
  gpio_st *gpio;
  bitbang_pin_value_en res;

  switch (pin) {
  case BITBANG_CLK_PIN:
    gpio = &ctx->sc_clk;
    break;
  case BITBANG_DATA_IN:
    gpio = &ctx->sc_miso;
    break;
  case BITBANG_DATA_OUT:
    gpio = &ctx->sc_mosi;
    break;
  }
  if (pin == BITBANG_DATA_IN) {
    res = gpio_read(gpio) ? BITBANG_PIN_HIGH : BITBANG_PIN_LOW;
  } else {
    res = value;
    gpio_write(gpio, ((res == BITBANG_PIN_HIGH)
                      ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW));
  }
  return res;
}

int main(int argc, char * const argv[])
{
  bitbang_init_st init;
  bitbang_handle_st *hdl = NULL;
  int cs = -1, clk = -1, in = -1, out = -1;
  gpio_st cs_gpio;
  int opt;
  int is_write = 0;
  int is_read = 0;
  int read_bits = 0;
  int write_bits = 0;
  int read_bytes = 0;
  int write_bytes = 0;
  int i;
  uint8_t *read_buf = NULL;;
  uint8_t *write_buf = NULL;;
  bitbang_clk_edge_en dout_edge = BITBANG_CLK_EDGE_FALLING;
  bitbang_clk_edge_en din_edge = BITBANG_CLK_EDGE_RISING;
  bitbang_pin_value_en clk_start = BITBANG_PIN_HIGH;
  bitbang_pin_value_en cs_value = BITBANG_PIN_HIGH;
  spi_context_st ctx;
  bitbang_io_st io;
  int rc = 0;
  int binary = 0;

  memset(&ctx, sizeof(ctx), 0);
  gpio_init_default(&ctx.sc_clk);
  gpio_init_default(&ctx.sc_mosi);
  gpio_init_default(&ctx.sc_miso);
  gpio_init_default(&cs_gpio);

  while ((opt = getopt(argc, argv, "bs:S:c:C:o:O:i:I:w:r:")) != -1) {
    switch (opt) {
    case 'b':
      binary = 1;
      break;
    case 's':
      cs = atoi(optarg);
      break;
    case 'S':
      if (!strcmp(optarg, "high")) {
        cs_value = BITBANG_PIN_HIGH;
      } else if (!strcmp(optarg, "low")) {
        cs_value = BITBANG_PIN_LOW;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'c':
      clk = atoi(optarg);
      break;
    case 'C':
      if (!strcasecmp(optarg, "high")) {
        clk_start = BITBANG_PIN_HIGH;
      } else if (!strcasecmp(optarg, "low")) {
        clk_start = BITBANG_PIN_LOW;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'o':
      out = atoi(optarg);
      break;
    case 'O':
      if (!strcasecmp(optarg, "rising")) {
        dout_edge = BITBANG_CLK_EDGE_RISING;
      } else if (!strcasecmp(optarg, "falling")) {
        dout_edge = BITBANG_CLK_EDGE_FALLING;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'i':
      in = atoi(optarg);
      break;
    case 'I':
      if (!strcasecmp(optarg, "rising")) {
        din_edge = BITBANG_CLK_EDGE_RISING;
      } else if (!strcasecmp(optarg, "falling")) {
        din_edge = BITBANG_CLK_EDGE_FALLING;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'w':
      is_write = 1;
      write_bits = atoi(optarg);
      if (write_bits <= 0) {
        usage();
        exit(-1);
      }
      break;
    case 'r':
      is_read = 1;
      read_bits = atoi(optarg);
      if (read_bits <= 0) {
        usage();
        exit(-1);
      }
      break;
    default:
      usage();
      exit(-1);
    }
  }

  if (clk < 0 || in < 0 || out < 0) {
    usage();
    exit(-1);
  }

  if ((!is_read && !is_write)) {
    usage();
    exit(-1);
  }

  write_bytes = ((write_bits + 7) / 8);
  if (write_bytes) {
    write_buf = calloc(write_bytes, sizeof(uint8_t));
    if (!write_buf) {
      goto out;
    }
    if (binary) {
      size_t written_bytes;
      written_bytes = fread(write_buf, sizeof(*write_buf), write_bytes, stdin);
      if( written_bytes != write_bytes ) {
        goto out;
      }
    } else {
      for (i = 0; i < write_bytes && i + optind < argc; i++) {
        write_buf[i] = strtoul(argv[i + optind], NULL, 0);
      }
    }
  }

  read_bytes = ((read_bits + 7) / 8);
  if (read_bytes) {
    read_buf = calloc(read_bytes, sizeof(uint8_t));
    if (!read_buf) {
      goto out;
    }
  }

  if (gpio_open(&ctx.sc_clk, clk) || gpio_open(&ctx.sc_miso, in)
      || gpio_open(&ctx.sc_mosi, out)) {
    goto out;
  }

  /* change GPIO directions, only MISO is input, all others are output */
  if (gpio_change_direction(&ctx.sc_clk, GPIO_DIRECTION_OUT)
      || gpio_change_direction(&ctx.sc_miso, GPIO_DIRECTION_IN)
      || gpio_change_direction(&ctx.sc_mosi, GPIO_DIRECTION_OUT)) {
    goto out;
  }

  if (cs != -1) {
    if (gpio_open(&cs_gpio, cs)) {
        goto out;
    }
    if (gpio_change_direction(&cs_gpio, GPIO_DIRECTION_OUT)) {
      goto out;
    }
  }

  bitbang_init_default(&init);
  init.bbi_clk_start = clk_start;
  init.bbi_data_out = dout_edge;
  init.bbi_data_in = din_edge;
  init.bbi_freq = 1000 * 1000;   /* 1M Hz */
  init.bbi_pin_f = spi_pin_f;
  init.bbi_context = &ctx;

  hdl = bitbang_open(&init);
  if (!hdl) {
    goto out;
  }

  if (cs != -1) {
    /* have chip select */
    gpio_write(&cs_gpio, ((cs_value == BITBANG_PIN_HIGH)
                          ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW));
  }

  memset(&io, sizeof(io), 0);
  io.bbio_in_bits = read_bits;
  io.bbio_din = read_buf;
  io.bbio_out_bits = write_bits;
  io.bbio_dout = write_buf;

  rc = bitbang_io(hdl, &io);
  if (rc != 0) {
    goto out;
  }

  if (binary) {
    fwrite(read_buf, sizeof(*read_buf), read_bytes, stdout);
  } else {
    if (write_bits) {
      printf("Wrote %u bits:", write_bits);
      for (i = 0; i < write_bytes; i++) {
        printf(" %02x", write_buf[i]);
      }
      printf("\n");
    }

    if (read_bits) {
      printf("Read %u bits:", read_bits);
      for (i = 0; i < read_bytes; i++) {
        printf(" %02x", read_buf[i]);
      }
      printf("\n");
    }
  }

 out:
  if (hdl) {
    bitbang_close(hdl);
  }
  gpio_close(&ctx.sc_clk);
  gpio_close(&ctx.sc_miso);
  gpio_close(&ctx.sc_mosi);
  if (cs != -1) {
    /* reset have chip select */
    gpio_write(&cs_gpio, ((cs_value == BITBANG_PIN_HIGH)
                          ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH));
    gpio_close(&cs_gpio);
  }

  if (read_buf) {
    free(read_buf);
  }
  if (write_buf) {
    free(write_buf);
  }
  return rc;
}
