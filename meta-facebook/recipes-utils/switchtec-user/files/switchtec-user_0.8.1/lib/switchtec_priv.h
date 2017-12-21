/*
 * Microsemi Switchtec(tm) PCIe Management Library
 * Copyright (c) 2017, Microsemi Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef LIBSWITCHTEC_SWITCHTEC_PRIV_H
#define LIBSWITCHTEC_SWITCHTEC_PRIV_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

enum {
	SWITCHTEC_GAS_CHAN_INBAND = 0,
	SWITCHTEC_GAS_CHAN_TWI = 1,
};

struct switchtec_dev {
	int fd;
	int partition;
	char name[PATH_MAX];

	int gas_chan;
	uint8_t twi_slave;
	void *gas_map;
	size_t gas_map_size;
};

static inline void version_to_string(uint32_t version, char *buf, size_t buflen)
{
	int major = version >> 24;
	int minor = (version >> 16) & 0xFF;
	int build = version & 0xFFFF;

	snprintf(buf, buflen, "%x.%02x B%03X", major, minor, build);
}

#endif
