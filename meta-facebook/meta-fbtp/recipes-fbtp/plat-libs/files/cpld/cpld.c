/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
/*
Please get the JEDEC file format before you read the code
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>

#include <sys/mman.h>
#include "lattice.h"
#include "ast-jtag.h"
#include "cpld.h"

int debug = 0;
struct cpld_dev_info *cur_dev;
xfer_mode mode = HW_MODE;

int cpld_get_ver(unsigned int *ver)
{
	int jtag_fd = open("/dev/ast-jtag", O_RDWR);
	if(jtag_fd == -1) {
		printf("Can't open /dev/ast-jtag, please install driver!! \n");
		return -1;
	}

	/*
	 * Enable Flash Conf I/F (Transparent Mode)
	 * Command 0x74, Operand 0x08 00 00
	 */
	ast_jtag_sir_xfer(jtag_fd, 0, 32, 0x74080000);

	// Read USERCODE 0xC0, Operand 0x00 00 00
	ast_jtag_sir_xfer(jtag_fd, 0, 32, 0xC0000000);

        // Issue TDO for 32 bits
	ast_jtag_tdo_xfer(jtag_fd, 0, 32, ver);

	// Disable Conf IF 0x26, Operand 0x00 00
	ast_jtag_sir_xfer(jtag_fd, 0, 24, 0x260000);

	// Issue Bypass CMD 0xFF, Operand 0xFF FF FF
	ast_jtag_sir_xfer(jtag_fd, 0, 32, 0xFFFFFFFF);

	close(jtag_fd);

	return 0;
}


int cpld_get_device_id(unsigned int *dev_id)
{
	int jtag_fd = open("/dev/ast-jtag", O_RDWR);
	if(jtag_fd == -1) {
		printf("Can't open /dev/ast-jtag, please install driver!! \n");
		return -1;
	}

	/*
	 * Enable Flash Conf I/F (Transparent Mode)
	 * Command 0x74, Operand 0x08 00 00
	 */
	ast_jtag_sir_xfer(jtag_fd, 0, 32, 0x74080000);

	// Read Device ID Code 0xE0, Operand 0x00 00 00
	ast_jtag_sir_xfer(jtag_fd, 0, 32, 0xE0000000);

        // Issue TDO for 32 bits
        ast_jtag_tdo_xfer(jtag_fd, 0, 32, dev_id);

	// Disable Conf IF 0x26, Operand 0x00 00
	ast_jtag_sir_xfer(jtag_fd, 0, 24, 0x260000);

	// Issue Bypass CMD 0xFF, Operand 0xFF FF FF
	ast_jtag_sir_xfer(jtag_fd, 0, 32, 0xFFFFFFFF);

	close(jtag_fd);

	return 0;
}
