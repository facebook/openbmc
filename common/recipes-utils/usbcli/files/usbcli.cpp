/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#include <libusb-1.0/libusb.h>

/*
 * Global macros.
 */
#define UCLI_DFT_TIMEOUT	5000	/* 5 seconds */
#define UCLI_DATA_MAX		1024	/* max # of bytes from command line */
#define UCLI_INVALID_DEVID	{0xFF, 0xFF, 0xFF, 0xFF}
#define UCLI_INVALID_ENDPOINT	0xFF

/*
 * Logging utilities.
 */
#define UCLI_ERR(fmt, args...)		fprintf(stderr, fmt, ##args)
#define UCLI_INFO(fmt, args...)		printf(fmt, ##args)
#define UCLI_VERBOSE(fmt, args...)		\
	do {					\
		if (verbose_logging)		\
			printf(fmt, ##args);	\
	} while (0)

/*
 * Structure to identify a usb device.
 */
typedef struct {
	uint8_t bus;
	uint8_t addr;
	uint16_t vid;
	uint16_t pid;
} ucli_dev_t;

/*
 * Parameters for command handlers.
 */
typedef struct {
	/* Identify a usb device and its endpoint. */
	ucli_dev_t dev_id;
	uint8_t endp;

	/* User command. */
	const char *cmd;

	/* usb transfer data buffer. */
	int xfer_len;
	uint8_t data[UCLI_DATA_MAX];
	const char *datafile;

	/* transfer timeout in milliseconds. */
	unsigned int timeout;
} ucli_param_t;

typedef struct {
	const char *cmd;
	const char *desc;
	int (*func)(ucli_param_t *params);
} ucli_cmd_t;

/*
 * Global variables
 */
static bool verbose_logging = false;

static int total_udevs;
static libusb_device **udev_list;

static int ucli_list_devices(ucli_param_t *params)
{
	int i, ret = 0;
	uint8_t bus, addr;
	libusb_device *dev;
	struct libusb_device_descriptor desc;

	UCLI_INFO("Total %d usb devices:\n", total_udevs);
	for (i = 0; udev_list[i] != NULL; i++) {
		dev = udev_list[i];
		bus = libusb_get_bus_number(dev);
		addr = libusb_get_device_address(dev);

		ret = libusb_get_device_descriptor(dev, &desc);
		if (ret < 0) {
			UCLI_ERR("failed to get device descriptor "
				 "(bus=%u, addr=%u): %s\n",
				 bus, addr, strerror(errno));
			return -1;
		}

		printf("  + bus=%u, addr=%u, vendor=%04x, product=%04x\n",
			bus, addr, desc.idVendor, desc.idProduct);
	}

	return ret;
}

static libusb_device* ucli_lookup_device(ucli_dev_t *dev_id)
{
	int i, ret;
	uint8_t bus, addr;
	libusb_device *dev;
	struct libusb_device_descriptor desc;

	for (i = 0; udev_list[i] != NULL; i++) {
		dev = udev_list[i];
		bus = libusb_get_bus_number(dev);
		addr = libusb_get_device_address(dev);

		if (bus == dev_id->bus && addr == dev_id->addr) {
			return dev;
		}

		ret = libusb_get_device_descriptor(dev, &desc);
		if (ret < 0) {
			return NULL;
		}

		if (desc.idVendor == dev_id->vid &&
		    desc.idProduct == dev_id->pid) {
			return dev;
		}
	}

	return NULL;
}

static libusb_device_handle* ucli_open_device(ucli_dev_t *dev_id)
{
	libusb_device *dev;
	libusb_device_handle *handle;

	dev = ucli_lookup_device(dev_id);
	if (dev == NULL) {
		UCLI_ERR("unable to locate device: "
			 "bus=%u, addr=%u, vid=%04x, pid=%04x\n",
			 dev_id->bus, dev_id->addr, dev_id->vid, dev_id->pid);
		return NULL;
	}

	if (libusb_open(dev, &handle) != 0) {
		UCLI_ERR("failed to open device: %s\n", strerror(errno));
		return NULL;
	}

	return handle;
}

/*
 * FIXME: the function (listing all active endpoints of a given usb
 * device) is not implemented.
 */
static int ucli_list_endpoints (ucli_param_t *params)
{
	UCLI_ERR("Error: function not implemented yet!\n");
	return -1;
}

static void ucli_dump_xfer_info(uint8_t ep, uint8_t *data,
				int exp_len, int act_len)
{
	int i;

	UCLI_INFO("endp=%#02x: requested %d bytes, actual transferred %d bytes:\n",
		  ep, exp_len, act_len);
	for (i = 0; i < act_len; i++) {
		UCLI_INFO("%#02x ", data[i]);
	}
	UCLI_INFO("\n");
}

static int ucli_bulk_xfer_series(libusb_device_handle *handle,
				 ucli_param_t *params)
{
	uint8_t ep;
	int ret = 0;
	int offset, xfer_len, act_len;
	FILE *fp;
	char line[4096];
	uint8_t payload[1024];
	char *curr, *next;

	fp = fopen(params->datafile, "r");
	if (fp == NULL) {
		UCLI_ERR("failed to open %s: %s\n",
			 params->datafile, strerror(errno));
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		/* Skip leading white spaces */
		for (curr = line; isspace(*curr); curr++) {}

		/*
		 * The first entry should be endpoint. Format: endp=<value>
		 * Example: endp=0x81, [data...]
		 */
		curr = strtok_r(curr, ",", &next);
		if ((curr == NULL) ||
		    (strncmp(curr, "endp=", strlen("endp=")) != 0)) {
			continue; /* invalid endpoint entry */
		}

		curr += strlen("endp=");
		ep = (uint8_t)strtol(curr, NULL, 0);
		if (ep == 0) {
			continue; /* invalid endpoint number */
		}

		/*
		 * For bulk in endpoint, the first data entry is parsed
		 * as transfer length, and following data entries (if any)
		 * are ignored.
		 * For bulk out endpoint, all the data entries are sent
		 * to device as payload.
		 */
		if (ep & 0x80) { /* bulk in */
			curr = strtok_r(NULL, " ", &next);
			if (curr == NULL) {
				continue; /* xfer length not specified */
			}

			xfer_len = (int)strtol(curr, NULL, 0);
		} else { /* bulk out */
			for (offset = 0;
			     (offset < sizeof(payload)) &&
			     ((curr = strtok_r(NULL, " ", &next)) != NULL);
			     offset++) {
				payload[offset] = (uint8_t)strtol(curr, NULL, 0);
			}
			xfer_len = offset;
		}
		if (offset == 0) {
			continue; /* no payload, nothing to do */
		}

		ret = libusb_bulk_transfer(handle, ep, payload, xfer_len,
					   &act_len, params->timeout);
		if (ret != 0) {
			UCLI_ERR("%s (endp=%#02x) failed: %s\n",
				 params->cmd, params->endp, strerror(errno));
			ret = -1;
			break;
		}
		ucli_dump_xfer_info(ep, payload, xfer_len, act_len);
	} /* while (fgets...) */

	fclose(fp);
	return ret;
}

static int ucli_bulk_xfer_single(libusb_device_handle *handle,
				 ucli_param_t *params)
{
	int ret, act_len;

	if (params->endp == UCLI_INVALID_ENDPOINT) {
		UCLI_ERR("endpoint is required for %s command.\n",
			 params->cmd);
		return -1;
	}

	ret = libusb_bulk_transfer(handle, params->endp, params->data,
			params->xfer_len, &act_len, params->timeout);
	if (ret != 0) {
		UCLI_ERR("%s (endp=%#02x) failed: %s\n",
			 params->cmd, params->endp, strerror(errno));
		return -1;
	}

	ucli_dump_xfer_info(params->endp, params->data,
			    params->xfer_len, act_len);
	return 0;
}

static int ucli_bulk_xfer(ucli_param_t *params)
{
	int ret = 0;
	libusb_device_handle *handle;

	handle = ucli_open_device(&params->dev_id);
	if (handle == NULL) {
		return -1;
	}

	if (params->datafile) {
		ret = ucli_bulk_xfer_series(handle, params);
	} else {
		ret = ucli_bulk_xfer_single(handle, params);
	}

	libusb_close(handle);
	return ret;
}

static ucli_cmd_t ucli_cmds[] = {
	{
		"list-devices",
		"list all usb devices in the sytem",
		ucli_list_devices,
	},
	{
		"list-endpoints",
		"list all active endpoints of the given usb device",
		ucli_list_endpoints,
	},
	{
		"bulk-xfer",
		"transfer data to/from the specified bulk-in endpoint",
		ucli_bulk_xfer,
	},

	/* Always the last entry. */
	{
		NULL,
		NULL,
		NULL,
	},
};


static ucli_cmd_t* ucli_match_cmd(const char *cmd)
{
	int i;

	for (i = 0; ucli_cmds[i].cmd != NULL; i++) {
		if (strcmp(cmd, ucli_cmds[i].cmd) == 0) {
			return &ucli_cmds[i];
		}
	}

	return NULL;
}

static void ucli_usage(const char *prog_name)
{
	int i;
	struct {
		const char *opt;
		const char *desc;
	} options[] = {
		{"-h|--help", "print this help message"},
		{"-v|--verbose", "enable verbose logging"},
		{"-B|--bus", "usb device's bus number"},
		{"-A|--address", "usb device's address"},
		{"-V|--vendor-id", "usb device's vendor ID"},
		{"-P|--product-id", "usb device's product ID"},
		{"-E|--endpoint", "usb device's endpoint"},
		{"-l|--length", "number of bytes for usb tranfer"},
		{"-t|--timeout", "timeout (in milliseconds) of a tranfer"},
		{"-D|--datafile", "data file containing bulk transfer payload"},
		{NULL, NULL},
	};

	/* Print available options */
	printf("Usage: %s [options] <usb-cmd> [DATA0, DATA1..]\n", prog_name);

	printf("\nAvailable options:\n");
	for (i = 0; options[i].opt != NULL; i++) {
		printf("    %-12s - %s\n", options[i].opt, options[i].desc);
	}

	/* Print available commands */
	printf("\nSupported usbcli commands:\n");
	for (i = 0; ucli_cmds[i].cmd != NULL; i++) {
		printf("    %-10s - %s\n",
		       ucli_cmds[i].cmd, ucli_cmds[i].desc);
	}
}

int main(int argc, char **argv)
{
	int i, ret, opt_index;
	ucli_cmd_t *cmd_info;
	ucli_param_t params = {
		.dev_id = UCLI_INVALID_DEVID,
		.endp = UCLI_INVALID_ENDPOINT,
		.timeout = UCLI_DFT_TIMEOUT,
	};

	struct option long_opts[] = {
		{"help",	no_argument,		NULL,	'h'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"bus",		required_argument,	NULL,	'B'},
		{"address",	required_argument,	NULL,	'A'},
		{"vendor-id",	required_argument,	NULL,	'V'},
		{"product-id",	required_argument,	NULL,	'P'},
		{"endpoint",	required_argument,	NULL,	'E'},
		{"length",	required_argument,	NULL,	'l'},
		{"timeout",	required_argument,	NULL,	't'},
		{"datafile",	required_argument,	NULL,	'D'},
		{NULL,		0,			NULL,	0},
	};

	while (1) {
		opt_index = 0;
		ret = getopt_long(argc, argv, "hvB:A:V:P:E:l:t:D:",
				  long_opts, &opt_index);
		if (ret == -1)
			break;	/* end of arguments */

		switch (ret) {
		case 'h':
			ucli_usage(argv[0]);
			return 0;

		case 'v':
			verbose_logging = true;
			break;

		case 'B':
			params.dev_id.bus = (uint8_t)strtol(optarg, NULL, 0);
			break;

		case 'A':
			params.dev_id.addr = (uint8_t)strtol(optarg, NULL, 0);
			break;

		case 'V':
			params.dev_id.vid = (uint16_t)strtol(optarg, NULL, 0);
			break;

		case 'P':
			params.dev_id.pid = (uint16_t)strtol(optarg, NULL, 0);
			break;

		case 'E':
			params.endp = (uint8_t)strtol(optarg, NULL, 0);
			break;

		case 'l':
			params.xfer_len = (int)strtol(optarg, NULL, 0);
			break;

		case 't':
			params.timeout = (unsigned int)strtol(optarg, NULL, 0);
			break;

		case 'D':
			params.datafile = optarg;
			break;

		default:
			return -1;
		}
	} /* while */

	if (optind >= argc) {
		UCLI_ERR("Error: usbcli command is missing!\n\n");
		return -1;
	}
	params.cmd = argv[optind++];

	/*
	 * Fetch data from command line if any.
	 */
	for (i = 0; optind < argc; i++, optind++) {
		params.data[i] = (uint8_t)strtol(argv[optind], NULL, 0);
	}
	if (params.xfer_len == 0) {
		params.xfer_len = i;
	}

	cmd_info = ucli_match_cmd(params.cmd);
	if (cmd_info == NULL) {
		UCLI_ERR("Error: unsupported command %s\n", params.cmd);
		return -1;
	}


	UCLI_VERBOSE("initializing libusb library.\n");
	ret = libusb_init(NULL);
	if (ret < 0) {
		UCLI_ERR("libusb_init failed: %s\n", strerror(errno));
		return -1;
	}

	UCLI_VERBOSE("searching for usb devices.\n");
	total_udevs = libusb_get_device_list(NULL, &udev_list);
	if (total_udevs < 0) {
		UCLI_ERR("failed to get device list: %s\n", strerror(errno));
		return -1;
	}

	UCLI_VERBOSE("running usbcli command <%s>.\n", params.cmd);
	assert(cmd_info->func != NULL);
	ret = cmd_info->func(&params);

	UCLI_VERBOSE("cleaning up environment.\n");
	libusb_free_device_list(udev_list, 1);
	libusb_exit(NULL);

	if (ret == 0) {
		UCLI_INFO("<%s> command completed successfully!\n", params.cmd);
	} else {
		UCLI_ERR("<%s> command failed with errors!\n", params.cmd);
		ret = -1;
	}
	return ret;
}
