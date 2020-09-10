/*
 * ast-jtag CPLD module
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "ast-jtag-intf.h"
#include "ast-jtag.h"

static unsigned int g_mode = 0;
static int jtag_fd = -1;

static void _ast_jtag_set_mode(unsigned int mode)
{
  g_mode = mode;
}

static int _ast_jtag_open(void)
{
  struct jtag_mode m;

  jtag_fd = open("/dev/jtag0", O_RDWR);
  if (jtag_fd == -1) {
    perror("Can't open /dev/jtag0, please install driver!! \n");
    return -1;
  }

  if (g_mode != 0) {
    m.feature = JTAG_XFER_MODE;
    m.mode = g_mode;
    // Set to desired mode
    if (ioctl(jtag_fd, JTAG_SIOCMODE, &m) < 0) {
      perror("Failed to set JTAG mode!\n");
      return -1;
    }
  }

  return 0;
}

static void _ast_jtag_close(void)
{
  close(jtag_fd);
}

static unsigned int _ast_get_jtag_freq(void)
{
  int retval;
  unsigned int freq = 0;

  if (jtag_fd == -1)
    return 0;

  retval = ioctl(jtag_fd, JTAG_GIOCFREQ, &freq);
  if (retval == -1) {
    perror("ioctl JTAG get freq fail!\n");
    return 0;
  }

  return freq;
}

static int _ast_set_jtag_freq(unsigned int freq)
{
  int retval = 0;

  if (jtag_fd == -1)
    return -1;

  retval = ioctl(jtag_fd, JTAG_SIOCFREQ, &freq);
  if (retval == -1) {
    perror("ioctl JTAG set freq fail!\n");
  }

  return retval;
}

/**
 * ast_jtag_run_test_idle
 *
 * @reset:
 * @end: end state
 * @tck: tck cycles
 */
static int _ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)
{
  int retval = 0;
  struct jtag_end_tap_state run_idle;

  if (jtag_fd == -1)
    return -1;

  run_idle.reset = reset;
  run_idle.endstate = end;
  run_idle.tck = tck;

  retval = ioctl(jtag_fd, JTAG_SIOCSTATE, &run_idle);
  if (retval == -1) {
    perror("ioctl JTAG run reset fail!\n");
  }

  return retval;
}

/**
 * ast_jtag_sir_xfer
 *
 * @endir: IR end state
 *         0 = idle, otherwise = pause
 * @len: data length in bit
 * @tdi: instruction data
 */
static int _ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi)
{
  int retval = 0;
  struct jtag_xfer xfer;
  unsigned long int addr = (unsigned long int)&tdi;

  if (len > 32 || jtag_fd == -1) {
    return -1;
  }

  xfer.type = JTAG_SIR_XFER;
  xfer.direction = JTAG_WRITE_XFER;
  xfer.endstate = endir;
  xfer.length = len;
  xfer.tdio = addr;

  retval = ioctl(jtag_fd, JTAG_IOCXFER, &xfer);
  if (retval == -1) {
    perror("ioctl JTAG sir fail!\n");
  }

  return retval;
}

/**
 * ast_jtag_tdi_xfer
 *
 * @enddr: DR end state
 *         0 = idle, otherwise = pause
 * @len: data length in bit
 * @tdio: data array
 */
static int _ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
  /* write */
  int retval = 0;
  struct jtag_xfer xfer;
  unsigned long int addr = (unsigned long int)tdio;

  if (tdio == NULL || jtag_fd == -1) {
    return -1;
  }

  xfer.type = JTAG_SDR_XFER;
  xfer.direction = JTAG_WRITE_XFER;
  xfer.endstate = enddr;
  xfer.length = len;
  xfer.tdio = addr;

  retval = ioctl(jtag_fd, JTAG_IOCXFER, &xfer);
  if (retval == -1) {
    perror("ioctl JTAG data xfer fail!\n");
  }

  return retval;
}

/**
 * ast_jtag_tdo_xfer
 *
 * @enddr: DR end state
 *         0 = idle, otherwise = pause
 * @len: data length in bit
 * @tdio: data array
 */
static int _ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
  /* read */
  int retval = 0;
  struct jtag_xfer xfer;
  unsigned long int addr = (unsigned long int)tdio;

  if (tdio == NULL || jtag_fd == -1) {
    return -1;
  }

  xfer.type = JTAG_SDR_XFER;
  xfer.direction = JTAG_READ_XFER;
  xfer.endstate = enddr;
  xfer.length = len;
  xfer.tdio = addr;

  retval = ioctl(jtag_fd, JTAG_IOCXFER, &xfer);
  if (retval == -1) {
    perror("ioctl JTAG data xfer fail!\n");
  }

  return retval;
}

struct jtag_ops jtag0_ops = {
  _ast_jtag_open,
  _ast_jtag_close,
  _ast_jtag_set_mode,
  _ast_get_jtag_freq,
  _ast_set_jtag_freq,
  _ast_jtag_run_test_idle,
  _ast_jtag_sir_xfer,
  _ast_jtag_tdo_xfer,
  _ast_jtag_tdi_xfer
};

