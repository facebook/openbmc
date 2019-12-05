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

/**
 * enum jtag_endstate:
 *
 * @JTAG_STATE_IDLE: JTAG state machine IDLE state
 * @JTAG_STATE_PAUSEIR: JTAG state machine PAUSE_IR state
 * @JTAG_STATE_PAUSEDR: JTAG state machine PAUSE_DR state
 */
enum jtag_endstate {
    JTAG_STATE_IDLE,
    JTAG_STATE_PAUSEIR,
    JTAG_STATE_PAUSEDR,
};

/**
 * enum jtag_xfer_type:
 *
 * @JTAG_SIR_XFER: SIR transfer
 * @JTAG_SDR_XFER: SDR transfer
 */
enum jtag_xfer_type {
    JTAG_SIR_XFER,
    JTAG_SDR_XFER,
};

/**
 * enum jtag_xfer_direction:
 *
 * @JTAG_READ_XFER: read transfer
 * @JTAG_WRITE_XFER: write transfer
 */
enum jtag_xfer_direction {
    JTAG_READ_XFER,
    JTAG_WRITE_XFER,
};

/**
 * struct jtag_run_test_idle - forces JTAG state machine to
 * RUN_TEST/IDLE state
 *
 * @reset: 0 - run IDLE/PAUSE from current state
 *         1 - go through TEST_LOGIC/RESET state before  IDLE/PAUSE
 * @end: completion flag
 * @tck: clock counter
 *
 * Structure represents interface to JTAG device for jtag idle
 * execution.
 */
struct jtag_run_test_idle {
    uint8_t    reset;
    uint8_t    endstate;
    uint8_t    tck;
};

/**
 * struct jtag_xfer - jtag xfer:
 *
 * @type: transfer type
 * @direction: xfer direction
 * @length: xfer bits len
 * @tdio : xfer data array
 * @endir: xfer end state
 *
 * Structure represents interface to JTAG device for jtag sdr xfer
 * execution.
 */
struct jtag_xfer {
    uint8_t    type;
    uint8_t    direction;
    uint8_t    endstate;
    uint32_t   length;
    uint32_t   *tdio;
};

/* ioctl interface */
#define __JTAG_IOCTL_MAGIC  0xb2

#define JTAG_IOCRUNTEST _IOW(__JTAG_IOCTL_MAGIC, 0,\
                 struct jtag_run_test_idle)
#define JTAG_SIOCFREQ   _IOW(__JTAG_IOCTL_MAGIC, 1, unsigned int)
#define JTAG_GIOCFREQ   _IOR(__JTAG_IOCTL_MAGIC, 2, unsigned int)
#define JTAG_IOCXFER    _IOWR(__JTAG_IOCTL_MAGIC, 3, struct jtag_xfer)
#define JTAG_GIOCSTATUS _IOWR(__JTAG_IOCTL_MAGIC, 4, enum jtag_endstate)
#define JTAG_SIOCMODE   _IOW(__JTAG_IOCTL_MAGIC, 5, unsigned int)

static unsigned int g_mode = 0;
static int jtag_fd = -1;

static void _ast_jtag_set_mode(unsigned int mode)
{
  g_mode = mode;
}

static int _ast_jtag_open(void)
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
 * @reset: 0 = return to idle/run-test state
 *         1 = force controller and slave into reset state
 * @end: end state
 *       0 = idle, 1 = IR pause, 2 = DR pause
 * @tck: tck cycles
 */
static int _ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)
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
static int _ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi)
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
static int _ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
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
static int _ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
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

