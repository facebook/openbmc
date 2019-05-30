/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
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

#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "peci.h"

/*
 * Function to handle PECI messages
 */
int
peci_cmd_xfer(struct peci_xfer_msg *msg) {
  int fd, ret;

  if ((fd = open(PECI_DEVICE, O_RDWR)) < 0) {
    syslog(LOG_ERR, "failed to open %s", PECI_DEVICE);
    return -1;
  }

  ret = ioctl(fd, PECI_IOC_XFER, msg);
  close(fd);

  return ret;
}

int
peci_cmd_xfer_fd(int fd, struct peci_xfer_msg *msg) {
  return ioctl(fd, PECI_IOC_XFER, msg);
}
