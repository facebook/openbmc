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

extern xfer_mode mode;
int jtag_fd = -1;

/*************************************************************************************/
/*				AST JTAG LIB					*/
int ast_jtag_open(void)
{
        struct controller_mode_param params = {0};
        int retval;

        params.mode = mode;
        params.controller_mode = JTAGDriverState_Master;

	jtag_fd = open("/dev/ast-jtag", O_RDWR);
	if(jtag_fd == -1) {
		perror("Can't open /dev/ast-jtag, please install driver!! \n");
		return -1;
	}

        retval = ioctl(jtag_fd, AST_JTAG_SLAVECONTLR, &params);
        if ( retval == -1 )
        {
            perror("Failed to set JTAG mode to master.!\n");
            return -1;
        }

	return 0;
}

void ast_jtag_close(void)
{
        struct controller_mode_param params = {0};
        int retval;

        params.mode = mode;
        params.controller_mode = JTAGDriverState_Slave;

        retval = ioctl(jtag_fd, AST_JTAG_SLAVECONTLR, &params);
        if ( retval == -1 )
        {
            perror("Failed to set JTAG mode to slave.!\n");
        }

	close(jtag_fd);
}

unsigned int ast_get_jtag_freq(void)
{
	int retval;
	unsigned int freq = 0;
	retval = ioctl(jtag_fd, AST_JTAG_GIOCFREQ, &freq);
	if (retval == -1) {
		perror("ioctl JTAG run reset fail!\n");
		return 0;
	}

	return freq;
}

int ast_set_jtag_freq(unsigned int freq)
{
	int retval;
	retval = ioctl(jtag_fd, AST_JTAG_SIOCFREQ, freq);
	if (retval == -1) {
		perror("ioctl JTAG run reset fail!\n");
		return -1;
	}

	return 0;
}

int ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)
{
	int retval;
	struct runtest_idle run_idle;

	run_idle.mode = mode;
	run_idle.end = end;
	run_idle.reset = reset;
	run_idle.tck = tck;

	retval = ioctl(jtag_fd, AST_JTAG_IOCRUNTEST, &run_idle);
	if (retval == -1) {
			perror("ioctl JTAG run reset fail!\n");
			return -1;
	}

	return 0;
}

unsigned int ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi)
{
	int 	retval;
	struct sir_xfer	sir;

	if(len > 32 || jtag_fd == -1) {
		return -1;
	}

	sir.mode = mode;
	sir.length = len;
	sir.endir = endir;
	sir.tdi = tdi;

	retval = ioctl(jtag_fd, AST_JTAG_IOCSIR, &sir);
	if (retval == -1) {
			perror("ioctl JTAG sir fail!\n");
			return -1;
	}

	return sir.tdo;
}

int ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
	/* write */
	int retval;
	struct sdr_xfer sdr;

	if (jtag_fd == -1) {
		return -1;
	}

	sdr.mode = mode;
	sdr.direct = 1;
	sdr.enddr = enddr;
	sdr.length = len;
	sdr.tdio = tdio;

	retval = ioctl(jtag_fd, AST_JTAG_IOCSDR, &sdr);
	if (retval == -1) {
			perror("ioctl JTAG data xfer fail!\n");
			return -1;
	}

	return 0;
}

int ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
	/* read */
	int retval;

	struct sdr_xfer sdr;

	if (jtag_fd == -1) {
		return -1;
	}

	sdr.mode = mode;
	sdr.direct = 0;
	sdr.enddr = enddr;
	sdr.length = len;
	sdr.tdio = tdio;

	retval = ioctl(jtag_fd, AST_JTAG_IOCSDR, &sdr);
	if (retval == -1) {
			perror("ioctl JTAG data xfer fail!\n");
			return -1;
	}

	return 0;
}

