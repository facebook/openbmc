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

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define UBRG_DEV_PATH	"/dev/usb-fpga"

#define MAX_DATA_LEN	512

/*
 * Definitions from usb-fpga kernel driver.
 */
#define UBRG_CMD_MEM_IO	0x101

/*
 * Structure to pass usb memory read/write command and data between user
 * and kernel space.
 */
struct ubrg_ioc_xfer {
	uint32_t addr;
	void *buf;
	unsigned int len;
	unsigned int flags;
#define UMEM_IOF_WRITE	0x1
};

/*
 * Logging utilities.
 */
#define UFMEM_ERR(fmt, args...)		fprintf(stderr, fmt, ##args)
#define UFMEM_INFO(fmt, args...)		printf(fmt, ##args)
#define UFMEM_VERBOSE(fmt, args...)		\
	do {					\
		if (verbose_logging)		\
			printf(fmt, ##args);	\
	} while (0)

/*
 * Global variables
 */
static bool verbose_logging = false;

static void usage(const char *prog_name)
{
	int i;
	struct {
		const char *opt;
		const char *desc;
	} options[] = {
		{"-h|--help", "print this help message"},
		{"-v|--verbose", "enable verbose logging"},
		{"-l|--length", "number of bytes read from the device"},
		{NULL, NULL},
	};

	/* Print available options */
	printf("Usage: %s [options] <address> [BYTE0, BYTE1..]\n", prog_name);

	printf("\nAvailable options:\n");
	for (i = 0; options[i].opt != NULL; i++) {
		printf("    %-12s - %s\n", options[i].opt, options[i].desc);
	}
}

static void dump_row(uint8_t *buf, unsigned int offset, unsigned int size)
{
	int len = 0;
	char output[512];
	unsigned int end = offset + size - 1;

	len += snprintf(&output[len], sizeof(output) - len,
			"* [%2u-%2u] ", offset, end);
	while (offset <= end)
		len += snprintf(&output[len], sizeof(output) - len,
				"0x%02x ", buf[offset++]);

	UFMEM_INFO("%s\n", output);
}

static void dump_data(void *data, unsigned int size)
{
	unsigned int i;
	unsigned int columns = 8;
	unsigned int rows = size / columns;
	unsigned int remainder = size % columns;
	uint8_t *buf = (uint8_t *)data;

	for (i = 0; i < rows; i++)
		dump_row(buf, i * columns, columns);
	if (remainder > 0)
		dump_row(buf, rows * columns, remainder);
}

int main(int argc, char **argv)
{
	int fd;
	unsigned int i;
	struct option long_opts[] = {
		{"help",	no_argument,		NULL,	'h'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"length",	required_argument,	NULL,	'l'},
		{NULL,		0,			NULL,	0},
	};
	uint8_t data_buf[MAX_DATA_LEN];
	struct ubrg_ioc_xfer ioc_xfer = {
		.addr = (uint32_t)-1,
		.buf = data_buf,
		.len = 4,	/* default read length: 4 bytes */
		.flags = 0,
	};

	while (1) {
		int ret;
		int opt_index = 0;

		ret = getopt_long(argc, argv, "hvl:", long_opts, &opt_index);
		if (ret == -1)
			break;	/* end of arguments */

		switch (ret) {
		case 'h':
			usage(argv[0]);
			return 0;

		case 'v':
			verbose_logging = true;
			break;

		case 'l':
			ioc_xfer.len = strtol(optarg, NULL, 0);
			break;

		default:
			return -1;
		}
	} /* while */

	if (optind >= argc) {
		UFMEM_ERR("Error: <address> argument is missing!\n\n");
		return -1;
	}
	ioc_xfer.addr = (uint32_t)strtol(argv[optind++], NULL, 0);

	/*
	 * If there are extra data/bytes after <address> argument, then
	 * it's memory-write, and data length is determined by data count
	 * in command line.
	 * Otherwise, it's memory-read, and read_length is 4 unless it's
	 * specificed in command line (--length).
	 */
	if (optind < argc) {
		ioc_xfer.flags = UMEM_IOF_WRITE;
		ioc_xfer.len = (argc - optind);

		if (ioc_xfer.len > sizeof(data_buf)) {
			UFMEM_ERR("Input data being truncated: %u -> %lu\n",
				  ioc_xfer.len, sizeof(data_buf));
			ioc_xfer.len = sizeof(data_buf);
		}
		for (i = 0; i < ioc_xfer.len; i++) {
			data_buf[i] = (uint8_t)strtol(argv[optind + i], NULL, 0);
		}
	}

	if (ioc_xfer.flags & UMEM_IOF_WRITE) {
		UFMEM_INFO("write %u bytes, start_addr=%#x\n",
			   ioc_xfer.len, ioc_xfer.addr);
		if (verbose_logging) {
			UFMEM_INFO("Data listed as below:\n");
			dump_data(ioc_xfer.buf, ioc_xfer.len);
		}
	} else {
		UFMEM_INFO("read %u bytes, start_addr=%#x\n",
			   ioc_xfer.len, ioc_xfer.addr);
	}

	fd = open(UBRG_DEV_PATH, O_RDWR);
	if (fd < 0) {
		UFMEM_ERR("failed to open %s: %s\n",
			  UBRG_DEV_PATH, strerror(errno));
		return -1;
	}

	if (ioctl(fd, UBRG_CMD_MEM_IO, &ioc_xfer) < 0) {
		UFMEM_ERR("ioctl (cmd=%#x) error: %s\n",
			  UBRG_CMD_MEM_IO, strerror(errno));
		return -1;
	}

	if (!(ioc_xfer.flags & UMEM_IOF_WRITE)) {
		UFMEM_INFO("Read below %u bytes from address %#x:\n",
			   ioc_xfer.len, ioc_xfer.addr);
		dump_data(ioc_xfer.buf, ioc_xfer.len);
	}

	close(fd);
	return 0;
}
