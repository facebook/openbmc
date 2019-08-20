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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "ast-jtag.h"

unsigned int g_mode = 0;
int jtag_fd = -1;

void ast_jtag_set_mode(unsigned int mode)
{
  g_mode = mode;
}

int ast_jtag_open(void)
{
  int retval = 0;

  jtag_fd = open("/dev/jtag0", O_RDWR);
  if (jtag_fd == -1) {
    perror("Can't open /dev/jtag0, please install driver!! \n");
    return -1;
  }

  if (g_mode != 0) {
    retval = ioctl(jtag_fd, JTAG_SIOCMODE, &g_mode);
    if (retval == -1)
      perror("Failed to set JTAG mode.!\n");
  }

  return retval;
}

void ast_jtag_close(void)
{
  close(jtag_fd);
}

unsigned int ast_get_jtag_freq(void)
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

int ast_set_jtag_freq(unsigned int freq)
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
 * @reset: 0 = return to idle/run-test state
 *         1 = force controller and slave into reset state
 * @end: end state
 *       0 = idle, 1 = IR pause, 2 = DR pause
 * @tck: tck cycles
 */
int ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)
{
  int retval = 0;
  struct jtag_run_test_idle run_idle;

  if (jtag_fd == -1)
    return -1;

  run_idle.reset = reset;
  run_idle.endstate = end;
  run_idle.tck = tck;

  retval = ioctl(jtag_fd, JTAG_IOCRUNTEST, &run_idle);
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
int ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi)
{
  int retval = 0;
  struct jtag_xfer xfer;

  if (len > 32 || jtag_fd == -1) {
    return -1;
  }

  xfer.type = JTAG_SIR_XFER;
  xfer.direction = JTAG_WRITE_XFER;
  xfer.endstate = endir;
  xfer.length = len;
  xfer.tdio = &tdi;

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
int ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
  /* write */
  int retval = 0;
  struct jtag_xfer xfer;

  if (tdio == NULL || jtag_fd == -1) {
    return -1;
  }

  xfer.type = JTAG_SDR_XFER;
  xfer.direction = JTAG_WRITE_XFER;
  xfer.endstate = enddr;
  xfer.length = len;
  xfer.tdio = tdio;

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
int ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
  /* read */
  int retval = 0;
  struct jtag_xfer xfer;

  if (tdio == NULL || jtag_fd == -1) {
    return -1;
  }

  xfer.type = JTAG_SDR_XFER;
  xfer.direction = JTAG_READ_XFER;
  xfer.endstate = enddr;
  xfer.length = len;
  xfer.tdio = tdio;

  retval = ioctl(jtag_fd, JTAG_IOCXFER, &xfer);
  if (retval == -1) {
    perror("ioctl JTAG data xfer fail!\n");
  }

  return retval;
}

