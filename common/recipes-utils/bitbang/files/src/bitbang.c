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

#include "bitbang.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <openbmc/log.h>

#define NANOSEC_IN_SEC (1000 * 1000 * 1000)

#define BITBANG_FREQ_MAX (500 * 1000 * 1000) /* 500M Hz */
#define BITBANG_FREQ_DEFAULT (1 * 1000 * 1000) /* 1M Hz */

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

struct bitbang_handle {
  bitbang_init_st bbh_init;
  uint32_t bbh_half_clk;           /* ns per clock cycle */
};

void bitbang_init_default(bitbang_init_st *init)
{
  memset(init, sizeof(*init), 0);
  init->bbi_clk_start = BITBANG_PIN_HIGH;
  init->bbi_data_out = BITBANG_CLK_EDGE_FALLING;
  init->bbi_data_in = BITBANG_CLK_EDGE_RISING;
  init->bbi_freq = BITBANG_FREQ_DEFAULT;
}

bitbang_handle_st* bitbang_open(const bitbang_init_st *init)
{
  bitbang_handle_st *hdl;

  if (!init || !init->bbi_pin_f
      || !init->bbi_freq || init->bbi_freq > BITBANG_FREQ_MAX) {
    LOG_ERR(EINVAL, "Invalid init structure");
    return NULL;
  }

  hdl = calloc(1, sizeof(*hdl));
  if (!hdl) {
    return NULL;
  }

  hdl->bbh_init = *init;
  hdl->bbh_half_clk = NANOSEC_IN_SEC / init->bbi_freq / 2;

  LOG_DBG("Bitbang open with initial %s, data out at %s, data in at %s, "
          "freq at %uHz, half clk %uns",
          (init->bbi_clk_start == BITBANG_PIN_LOW) ? "LOW" : "HIGH",
          (init->bbi_data_out == BITBANG_CLK_EDGE_RISING)
          ? "RISING" : "FALLING",
          (init->bbi_data_in == BITBANG_CLK_EDGE_RISING)
          ? "RISING" : "FALLING",
          init->bbi_freq, hdl->bbh_half_clk);

  return hdl;
}

void bitbang_close(bitbang_handle_st *hdl)
{
  free(hdl);
}

/*
 * The threshold (ns) to use spin instead of nanosleep().
 * Before adding the high resolution timer support, either spin or nanosleep()
 * will not bring the process wakeup within 10ms. It turns out the system time
 * update is also controlled by HZ (100).
 * After I added the high resolution timer support, the spin works as the
 * system time is updated more frequently. However, nanosleep() solution is
 * still noticeable slower comparing with spin. There could be some kernel
 * scheduling tweak missing. Did not get time on that yet.
 * For now, use 10ms as the threshold to determine if spin or nanosleep()
 * is used.
 */
#define BITBANG_SPIN_THRESHOLD (10 * 1000 * 1000)

static int sleep_ns(uint32_t clk)
{
  struct timespec req, rem;
  int rc = 0;
  if (clk <= BITBANG_SPIN_THRESHOLD) {
    struct timespec orig;
    rc = clock_gettime(CLOCK_MONOTONIC, &req);
    orig = req;
    while (!rc && clk) {
      uint32_t tmp;
      rc = clock_gettime(CLOCK_MONOTONIC, &rem);
      tmp = (rem.tv_sec - req.tv_sec) * NANOSEC_IN_SEC;
      if (rem.tv_nsec >= req.tv_nsec) {
        tmp += rem.tv_nsec - req.tv_nsec;
      } else {
        tmp -= req.tv_nsec - rem.tv_nsec;
      }
      if (tmp >= clk) {
        break;
      }
      clk -= tmp;
      req = rem;
    }
  } else {
    req.tv_sec = 0;
    req.tv_nsec = clk;
    while ((rc = nanosleep(&req, &rem)) == -1 && errno == EINTR) {
      req = rem;
    }
  }
  if (rc == -1) {
    rc = errno;
    LOG_ERR(rc, "Failed to sleep %u nanoseconds", clk);
  }
  return rc;
}

int bitbang_io(const bitbang_handle_st *hdl, bitbang_io_st *io)
{
  int rc = 0;
  uint32_t clk = hdl->bbh_half_clk;
  const struct {
    bitbang_pin_value_en value;
    bitbang_clk_edge_en edge;
  } clks[] = {
    {BITBANG_PIN_HIGH, BITBANG_CLK_EDGE_FALLING},
    {BITBANG_PIN_LOW, BITBANG_CLK_EDGE_RISING},
  };
  int clk_idx;
  int n_clk = 0;
  int n_bits = 0;
  const uint8_t *dout = io->bbio_dout;
  uint8_t *din = io->bbio_din;
  int bit_pos = 7;
  bitbang_pin_func pin_f = hdl->bbh_init.bbi_pin_f;
  void *context = hdl->bbh_init.bbi_context;

  if ((io->bbio_in_bits == 0 && io->bbio_din)
      || (io->bbio_in_bits > 0 && !io->bbio_din)) {
    rc = EINVAL;
    LOG_ERR(rc, "Incorrect in bits and in buffer");
    goto out;
  }

  if ((io->bbio_out_bits == 0 && io->bbio_dout)
      || (io->bbio_out_bits > 0 && !io->bbio_dout)) {
    rc = EINVAL;
    LOG_ERR(rc, "Incorrect out bits and out buffer");
    goto out;
  }

  if (io->bbio_in_bits == 0 && io->bbio_out_bits == 0) {
    rc = EINVAL;
    LOG_ERR(rc, "Both in and out bits are 0");
    goto out;
  }

  if (hdl->bbh_init.bbi_clk_start == BITBANG_PIN_HIGH) {
    clk_idx = 0;
  } else {
    clk_idx = 1;
  }

  /* set the CLK pin start position */
  pin_f(BITBANG_CLK_PIN, clks[clk_idx].value, context);

  /* clear the first byte of din */
  if (din && io->bbio_in_bits) {
    memset(din, 0, (io->bbio_in_bits + 7) / 8);
  }

  do {
    if ((rc = sleep_ns(clk))) {
      goto out;
    }

    /* output first */
    if (hdl->bbh_init.bbi_data_out == clks[clk_idx].edge) {
      if (dout && n_bits < io->bbio_out_bits) {
        pin_f(BITBANG_DATA_OUT, (*dout >> bit_pos) & 0x1, context);
      }
    }

    /* then, input */
    if (hdl->bbh_init.bbi_data_in == clks[clk_idx].edge) {
      if (din && n_bits < io->bbio_in_bits) {
        *din |= (pin_f(BITBANG_DATA_IN, 0, context) & 0x1) << bit_pos;
      }
    }

    if (++n_clk % 2 == 0) {
      /* one bit for every 2 half clks */
      n_bits ++;
      if (bit_pos == 0) {
        if (dout) {
          dout++;
        }
        if (din) {
          din++;
        }
        bit_pos = 7;
      } else {
        bit_pos --;
      }
    }
    clk_idx = 1 - clk_idx;
    pin_f(BITBANG_CLK_PIN, clks[clk_idx].value, context);
  } while (n_bits < MAX(io->bbio_in_bits, io->bbio_out_bits));

 out:

  return -rc;
}
