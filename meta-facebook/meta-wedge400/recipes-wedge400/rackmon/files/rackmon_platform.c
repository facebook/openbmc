/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openbmc/log.h>

#include "rackmon_platform.h"

static int usb_serial_open(struct rackmon_io_handler *handler)
{
	int fd;

	fd = open(handler->dev_path, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		OBMC_ERROR(errno, "failed to open %s", handler->dev_path);
		return -1;
	}

	return fd;
}

struct rackmon_io_handler usb_serial_io = {
  .dev_path = "/dev/ttyUSB0",
  .open = usb_serial_open,
};

int rackmon_plat_init(void)
{
	rackmon_io = &usb_serial_io;
	return 0;
}

void rackmon_plat_cleanup(void)
{
}
