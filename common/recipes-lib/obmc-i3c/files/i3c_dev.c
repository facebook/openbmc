/*
 * Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This file contains code to provide addendum functionality over the I2C
 * device interfaces to utilize additional driver functionality.
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <i3c_dev.h>

int i3c_open(const char *fname) {
  int fd = open(fname, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  return fd;
}

void i3c_close(int fd) {
  if (fd >= 0) {
    close(fd);
  }
}

int i3c_rdwr_msg_xfer(int fd, uint8_t *wdata, uint16_t wlen,
                              uint8_t *rdata, uint16_t rlen) {
  int num_xfers = 0;
  struct i3c_ioc_priv_xfer *xfers;

  if (fd < 0 || wlen > MAX_DATA_LEN || rlen > MAX_DATA_LEN) {
    return -1;
  }

  if (wlen > 0) num_xfers++;
  if (rlen > 0) num_xfers++;
  xfers = (struct i3c_ioc_priv_xfer *) calloc(num_xfers, sizeof(*xfers));

  int i = 0;
  // Set up write transfer
  if (wlen > 0) {
    xfers[i].rnw   = 0;
    xfers[i].len   = wlen;
    xfers[i].data = (uintptr_t)wdata;
    i++;
  }

  // Set up read transfer
  if (rlen > 0) {
    xfers[i].rnw  = 1;
    xfers[i].len  = rlen;
    xfers[i].data = (uintptr_t)rdata;
  }

  int ret = ioctl(fd, I3C_IOC_PRIV_XFER(num_xfers), xfers);
  if (ret < 0) {
    free(xfers);
    return -1;
  }
  
  free(xfers);
  return 0;
}

