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

#ifndef _RACKMON_PLATFORM_H_
#define _RACKMON_PLATFORM_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This structure holds variables/ops to perform rackmon I/O transactions.
 */
struct rackmon_io_handler {
	const char *dev_path;

	int (*open)(struct rackmon_io_handler *handler);
};

extern struct rackmon_io_handler *rackmon_io;

int rackmon_plat_init(void);
void rackmon_plat_cleanup(void);

#endif /* _RACKMON_PLATFORM_H_ */
