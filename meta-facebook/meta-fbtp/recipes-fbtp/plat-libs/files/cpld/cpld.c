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
FILE *fp_in = NULL;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

int cpld_intf_open(void)
{
	int i = 0;
	unsigned int dev_id;

	if (ast_jtag_open()) {
		return -1;
	}

	if (cpld_get_device_id(&dev_id)) {
		ast_jtag_close();
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(lattice_device_list); i++) {
		if (dev_id == lattice_device_list[i].dev_id)
			break;
	}

	if (i == ARRAY_SIZE(lattice_device_list)) {
		//Unknow CPLD device
		ast_jtag_close();
		return -1;
	} else {
		//Found CPLD device
		cur_dev = &lattice_device_list[i];
	}

	return 0;
}

int cpld_intf_close(void)
{
	ast_jtag_close();

	return 0;
}

int cpld_get_ver(unsigned int *ver)
{
	cur_dev->cpld_ver(ver);

	return 0;
}

int cpld_get_device_id(unsigned int *dev_id)
{
	/* SIR 8 TDI (16); */
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);
	ast_jtag_tdo_xfer(0, 32, dev_id);

	return 0;
}

int cpld_verify(void)
{
	if (cur_dev == NULL || fp_in == NULL)
		return -1;

	return cur_dev->cpld_verify(fp_in);
}

int cpld_program(char *JEDFILE)
{
  fp_in = fopen(JEDFILE,"r");
  if ( NULL == cur_dev )
  {
      printf("[%s] Cannot get pointer from DEVICE!\n", __func__);

      return -1;
  }
  else if ( NULL == fp_in )
  {
      printf("[%s] Cannot Open File!\n", __func__);

      return -1;
  }

	return cur_dev->cpld_program(fp_in);
}

int cpld_erase(void)
{
	if (cur_dev == NULL)
		return -1;

	return cur_dev->cpld_erase();

}
