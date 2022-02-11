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
#include "ast-jtag.h"
#include "ast-jtag-intf.h"

typedef enum jtag_xfer_mode {
	HW_MODE = 0,
	SW_MODE
} xfer_mode;

typedef enum {
    JTAGDriverState_Master = 0,
    JTAGDriverState_Slave
} JTAGDriverState;

struct controller_mode_param {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     controller_mode;
};

struct runtest_idle {
	xfer_mode 	mode;		//0 :HW mode, 1: SW mode
	unsigned char 	reset;		//Test Logic Reset
	unsigned char 	end;		//o: idle, 1: ir pause, 2: drpause
	unsigned char 	tck;		//keep tck
};

struct sir_xfer {
	xfer_mode 	mode;		//0 :HW mode, 1: SW mode
	unsigned short length;		//bits
	unsigned int tdi;
	unsigned int tdo;
	unsigned char endir;		//0: idle, 1:pause
};

struct sdr_xfer {
	xfer_mode 	mode;		//0 :HW mode, 1: SW mode
	unsigned char 	direct; 	// 0 ; read , 1 : write
	unsigned short length;		//bits
	unsigned int *tdio;
	unsigned char enddr;		//0: idle, 1:pause
};

#define JTAGIOC_BASE		'T'

#define AST_JTAG_IOCRUNTEST	_IOW(JTAGIOC_BASE, 0, struct runtest_idle)
#define AST_JTAG_IOCSIR		_IOWR(JTAGIOC_BASE, 1, struct sir_xfer)
#define AST_JTAG_IOCSDR		_IOWR(JTAGIOC_BASE, 2, struct sdr_xfer)
#define AST_JTAG_SIOCFREQ	_IOW(JTAGIOC_BASE, 3, unsigned int)
#define AST_JTAG_GIOCFREQ	_IOR(JTAGIOC_BASE, 4, unsigned int)
#define AST_JTAG_SLAVECONTLR      _IOW( JTAGIOC_BASE, 8, struct controller_mode_param)

static xfer_mode mode;
static int jtag_fd = -1;

static void _ast_jtag_set_mode(unsigned int m)
{
  if (m == JTAG_XFER_HW_MODE) {
    mode = HW_MODE;
  } else {
    mode = SW_MODE;
  }
}

/*************************************************************************************/
/*				AST JTAG LIB					*/
static int _ast_jtag_open(void)
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

static void _ast_jtag_close(void)
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

static unsigned int _ast_get_jtag_freq(void)
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

static int _ast_set_jtag_freq(unsigned int freq)
{
	int retval;
	retval = ioctl(jtag_fd, AST_JTAG_SIOCFREQ, freq);
	if (retval == -1) {
		perror("ioctl JTAG run reset fail!\n");
		return -1;
	}

	return 0;
}

static int _ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)
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

static int _ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi)
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
  return 0;
}

static int _ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
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

static int _ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
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

struct jtag_ops astjtag_ops = {
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

