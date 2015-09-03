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

#include <openbmc/log.h>
#include <openbmc/gpio.h>

#include "bitbang.h"

typedef struct {
  gpio_st m_mdc;
  gpio_st m_mdio;
} mdio_context_st;

/*
 * 32b preamble, 2b start of frame, 2b operation code,
 * 5b phy addr, 5b register addr, 2b turnaround, 16b data
 */
#define N_BITS (32 + 2 + 2 + 5 + 5 + 2 + 16)
#define N_BYTES (N_BITS + 7 / 8)

#define START_OF_FRAME 0x1
#define OP_READ 0x2
#define OP_WRITE 0x1
#define TURNAROUND 0x2          /* TA for write, for read, phy sends out TA */

void usage()
{
  fprintf(stderr,
          "Usage:\n"
          "mdio-bb: -c <GPIO for MDC> [-C <HIGH|low>]\n"
          "         -d <GPIO for MDIO> [-O <rising|FALLING>]\n"
          "         [-I <RISING|falling>] [-p] [-b]\n"
          "         <read|write> <phy address> <register address>\n"
          "         [value to write]\n");
}

bitbang_pin_value_en mdio_pin_f(
    bitbang_pin_type_en pin, bitbang_pin_value_en value, void *context)
{
  mdio_context_st *ctx = (mdio_context_st *)context;
  gpio_st *gpio;
  bitbang_pin_value_en res;

  switch (pin) {
  case BITBANG_CLK_PIN:
    gpio = &ctx->m_mdc;
    break;
  case BITBANG_DATA_IN:
  case BITBANG_DATA_OUT:
    gpio = &ctx->m_mdio;
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

int main(int argc, char* const argv[])
{
  int opt;
  int mdc = -1, mdio = -1;
  bitbang_pin_value_en mdc_start = BITBANG_PIN_HIGH;
  bitbang_clk_edge_en out_edge = BITBANG_CLK_EDGE_FALLING;
  bitbang_clk_edge_en in_edge = BITBANG_CLK_EDGE_RISING;
  int is_write;
  uint32_t phy_addr;
  uint32_t reg_addr;
  uint32_t data;                /* data to write/read*/
  uint8_t buf[N_BYTES];
  uint8_t *buf_p;
  mdio_context_st ctx;
  bitbang_init_st init;
  bitbang_handle_st *hdl = NULL;
  bitbang_io_st io;
  int n_bits;
  int i;
  int rc = 0;
  int preamble = 0;
  int binary = 0;

  while ((opt = getopt(argc, argv, "bc:C:d:D:p")) != -1) {
    switch (opt) {
    case 'b':
      binary = 1;
      break;
    case 'c':
      mdc = atoi(optarg);
      break;
    case 'C':
      if (!strcasecmp(optarg, "high")) {
        mdc_start = BITBANG_PIN_HIGH;
      } else if (!strcasecmp(optarg, "low")) {
        mdc_start = BITBANG_PIN_LOW;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'd':
      mdio = atoi(optarg);
      break;
    case 'I':
      if (!strcasecmp(optarg, "rising")) {
        in_edge = BITBANG_CLK_EDGE_RISING;
      } if (!strcasecmp(optarg, "falling")) {
        in_edge = BITBANG_CLK_EDGE_FALLING;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'O':
      if (!strcasecmp(optarg, "rising")) {
        out_edge = BITBANG_CLK_EDGE_RISING;
      } if (!strcasecmp(optarg, "falling")) {
        out_edge = BITBANG_CLK_EDGE_FALLING;
      } else {
        usage();
        exit(-1);
      }
      break;
    case 'p':
      preamble = 1;
      break;
    default:
      usage();
      exit(-1);
    }
  }

  if (mdc < 0 || mdio < 0) {
    usage();
    exit(-1);
  }

  if (optind + 2 >= argc) {
    usage();
    exit(-1);
  }

  /* read or write */
  if (!strcasecmp(argv[optind], "read")) {
    is_write = 0;
  } else if (!strcasecmp(argv[optind], "write")) {
    is_write = 1;
  } else {
    usage();
    exit(-1);
  }

  /* phy address, 5 bits only, so must be <= 0x1f */
  phy_addr = strtoul(argv[optind + 1], NULL, 0);
  if (phy_addr > 0x1f) {
    usage();
    exit(-1);
  }

  /* register address, 5 bits only, so must be <= 0x1f */
  reg_addr = strtoul(argv[optind + 2], NULL, 0);
  if (reg_addr > 0x1f) {
    usage();
    exit(-1);
  }

  /* data */
  if (is_write) {
    if ((!binary && (optind + 4 != argc)) ||
        (binary && (optind + 3 != argc))) {
      usage();
      exit(-1);
    }
    if (binary) {
      uint16_t temp = 0;
      if (fread(&temp, sizeof(temp), 1, stdin) != 1) {
        usage();
        exit(-1);
      }
      data = htons(temp);
    } else {
      data = strtoul(argv[optind + 3], NULL, 0);
    }
    if (data > 0xFFFF) {
      usage();
      exit(-1);
    }
  } else {
    if ((!binary && (optind + 3 != argc)) ||
        (binary && (optind + 2 != argc))) {
      usage();
      exit(-1);
    }
  }

  /* open all gpio */
  memset(&ctx, sizeof(ctx), 0);
  gpio_init_default(&ctx.m_mdc);
  gpio_init_default(&ctx.m_mdio);
  if (gpio_open(&ctx.m_mdc, mdc) || gpio_open(&ctx.m_mdio, mdio)) {
    goto out;
  }

  if (gpio_change_direction(&ctx.m_mdc, GPIO_DIRECTION_OUT)
      || gpio_change_direction(&ctx.m_mdio, GPIO_DIRECTION_OUT)) {
    goto out;
  }

  bitbang_init_default(&init);
  init.bbi_clk_start = mdc_start;
  init.bbi_data_out = out_edge;
  init.bbi_data_in = in_edge;
  init.bbi_freq = 1000 * 1000;   /* 1M Hz */
  init.bbi_pin_f = mdio_pin_f;
  init.bbi_context = &ctx;
  hdl = bitbang_open(&init);
  if (!hdl) {
    goto out;
  }

  if (is_write) {
    buf[0] = (data >> 8) & 0xFF;
    buf[1] = data & 0xFF;
    io.bbio_out_bits = 16;
    io.bbio_dout = buf;
    io.bbio_in_bits = 0;
    io.bbio_din = NULL;
  } else {
    io.bbio_in_bits = 16;
    io.bbio_din = buf;
    io.bbio_out_bits = 0;
    io.bbio_dout = NULL;
  }

  /* preamble, 32b */
  buf_p = buf;
  n_bits = 0;
  if (preamble) {
    /* 32 bit of 1 for preamble */
    for (i = 0; i < 4; i++) {
      *buf_p++ = 0xFF;
    }
    n_bits += 32;
  }

  /*
   * MDIO transaction header is:
   * 2b START, 2b OPER CODE, 5b PHY ADDR, 5b register addr, 2b TURNROUND
   */
  *buf_p++ = (START_OF_FRAME << 6) | (((is_write) ? OP_WRITE : OP_READ) << 4)
    | ((phy_addr >> 1) & 0xF);
  *buf_p++ = ((phy_addr & 0x1) << 7) | ((reg_addr & 0x1F) << 2) | TURNAROUND;
  if (is_write) {
    *buf_p++ = (data >> 8) & 0xFF;
    *buf_p++ = data & 0xFF;
    /* total # of bits is transaction header + 2 bytes to write */
    n_bits += 2 + 2 + 5 + 5 + 2 + 16;
  } else {
    /* for read, master does not send TR, so, n_bits should not include TR */
    n_bits += 2 + 2 + 5 + 5;
  }

  memset(&io, sizeof(io), 0);
  io.bbio_out_bits = n_bits;
  io.bbio_dout = buf;
  io.bbio_in_bits = 0;
  io.bbio_din = NULL;

  rc = bitbang_io(hdl, &io);
  if (rc != 0) {
    goto out;
  }

  /* for read, need to do another io for (2b TR + 16b data) reading */
  if (!is_write) {
    /* first, change the MDIO to input */
    gpio_change_direction(&ctx.m_mdio, GPIO_DIRECTION_IN);
    /* then, run the clock for read */
    memset(&io, sizeof(io), 0);
    io.bbio_out_bits = 0;
    io.bbio_dout = NULL;;
    io.bbio_in_bits = 18;
    io.bbio_din = buf;

    rc = bitbang_io(hdl, &io);
    if (rc != 0) {
      goto out;
    }

    data = ((buf[0] << 2) | (buf[1] >> 6)) & 0xFF;
    data <<= 8;
    data |= ((buf[1] << 2) | (buf[2] >> 6)) & 0xFF;
  }

  if (binary) {
    if (!is_write) {
      uint16_t temp = ntohs(data);
      fwrite(&temp, sizeof(temp), 1, stdout);
    }
  } else {
    printf("%s: 0x%02x\n", (is_write) ? "Wrote" : "Read", data);
  }

 out:
  if (hdl) {
    bitbang_close(hdl);
  }
  gpio_close(&ctx.m_mdc);
  gpio_close(&ctx.m_mdio);

  return 0;
}
