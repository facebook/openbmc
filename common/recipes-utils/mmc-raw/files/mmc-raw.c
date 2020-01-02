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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include "mmc.h"

#define BLKDEV_SECTOR_SIZE	512
#define BLKDEV_ADDR_INVALID	UINT32_MAX

/*
 * Extended CSD register size. Refer to JEDEC Standard, Section 7.4 for
 * details.
 */
#define MMC_EXTCSD_SIZE		512

/*
 * Following flags are copied from Micron eMMC Application Notes.
 * We may need to revisit them later (for other chips).
 */
#define MMC_ERASE_GROUP_FLAGS	0x195
#define MMC_ERASE_FLAGS		0x49d

/*
 * Logging utilities.
 */
#define MMC_CMD_FMT		"(opcode=%u, arg=%#x, flags=%#x)"
#define MMC_DEBUG(fmt, args...)			\
	do {					\
		if (mmc_flags.verbose)		\
			printf(fmt, ##args);	\
	} while (0)
#define MMC_INFO(fmt, args...)	printf(fmt, ##args)
#define MMC_ERR(fmt, args...)	fprintf(stderr, fmt, ##args)

struct m_cmd_args {
	const char *cmd;
	const char *dev_path;

	int dev_fd;
	__u32 offset;
	__u32 length;
};

struct m_cmd_info {
	const char *cmd;
	const char *desc;
	int (*func)(struct m_cmd_args *args);
};

static struct {
	unsigned int verbose:1;
	unsigned int secure:1;
} mmc_flags;

/*
 * Cache of mmc registers (extcsd, csd, cid and etc.).
 */
static struct {
	unsigned int extcsd_loaded:1;
	__u8 extcsd[MMC_EXTCSD_SIZE];
} mmc_reg_cache;

/*
 * Check whether the device supports secure feature (defined in
 * SEC_FEATURE_SUPPORT field [byte 231] of EXT_CSD).
 *
 * TODO:
 *   - we need mmc_read_extcsd() and according functions to parse
 *     EXT_CSD.
 */
static bool mmc_is_secure_supported(int fd)
{
	return true;
}

/*
 * 32-bit <sectors> can address 2TB (512 * U32_MAX) device, which is way
 * more than the max capacity of MMC devices (128GB). So we are safe.
 */
static int mmc_sector_count(int fd, __u32 *sectors)
{
	int ret;
	unsigned long num_sec;

	ret = ioctl(fd, BLKGETSIZE, &num_sec);
	if (ret == -1) {
		MMC_ERR("failed to get sector size: %s\n", strerror(errno));
		return -1;
	}

	*sectors = num_sec;
	return 0;
}

/*
 * TODO: Some Micron emmc devices report incorrect WR_BLOCK_SIZE in CSD
 * register. We've confirmed with Micron it's safe to assume 512-byte
 * write-block-size, but ideally we should try to read CSD register, and
 * fall back to this default value if CSD reports incorrect size.
 */
static int mmc_wr_blk_size(int fd, __u32 *size)
{
	*size = 512;
	return 0;
}

/*
 * Get the number of write blocks of the given emmc device.
 */
static int mmc_wr_blk_count(int fd, __u32 *total_blks)
{
	__u32 sectors, blk_size;

	if (mmc_sector_count(fd, &sectors) != 0) {
		return -1;
	}
	if (mmc_wr_blk_size(fd, &blk_size) != 0) {
		return -1;
	}

	*total_blks = (sectors / blk_size * BLKDEV_SECTOR_SIZE);
	MMC_INFO("total %u write blocks, write block size %u\n",
		 *total_blks, blk_size);
	return 0;
}

static int mmc_cal_erase_range(struct m_cmd_args *args,
			       __u32 *start,
			       __u32 *end)
{
	__u32 total_blks;

	/*
	 * Get total number of sectors (512 bytes) of the device.
	 */
	if (mmc_wr_blk_count(args->dev_fd, &total_blks) != 0) {
		return -1;
	}
	if (total_blks == 0) {
		*start = *end = 0;
		return 0;
	}

	/*
	 * Set <start> to 0 unless it's specified in command line.
	 */
	if (args->offset != BLKDEV_ADDR_INVALID) {
		*start = args->offset;
	} else {
		*start = 0;
	}

	/*
	 * Set <end> to be the last block (covering the entire device)
	 * unless <length> is specified in command line.
	 */
	if (args->length != BLKDEV_ADDR_INVALID) {
		if (((UINT32_MAX - args->length) < *start) ||
		    ((*start + args->length) > total_blks)) {
			MMC_ERR("offset (%u) or length (%u) exceeds limit\n",
				args->offset, args->length);
			return -1;
		}
		*end = *start + args->length - 1;
	} else {
		*end = total_blks - 1;
	}

	return 0;
}

/*
 * Issue CMD35/CMD36 to set ERASE_GROUP_START|END.
 * NOTE:
 *   - For devices <=2GB, <arg> defines byte address.
 *   - For devices >2GB, <arg> defines sector (512-byte) address.
 *   - Erase operation applies to Erase Groups, while Trim/Discard applies
 *     to Write Blocks.
 */
static int mmc_set_erase_range(int fd, __u32 opcode, __u32 arg)
{
	int ret;
	struct mmc_ioc_cmd idata;

	memset(&idata, 0, sizeof(idata));
	idata.write_flag = 0;
	idata.opcode = opcode;
	idata.arg = arg;
	idata.flags = MMC_ERASE_GROUP_FLAGS;
	ret = ioctl(fd, MMC_IOC_CMD, &idata);
	if (ret == -1) {
		MMC_ERR("set erase range " MMC_CMD_FMT " failed: %s\n",
			opcode, arg, idata.flags, strerror(errno));
		return ret;
	}

	return 0;
}

/*
 * Issue CMD38 to erase/trim/discard blocks.
 * NOTE:
 *   - Erase type (erase, trim or discard) is determined by <arg> value.
 */
static int mmc_issue_erase(int fd, __u32 arg)
{
	int ret;
	struct mmc_ioc_cmd idata;

	memset(&idata, 0, sizeof(idata));
	idata.write_flag = 0;
	idata.opcode = MMC_ERASE;
	idata.arg = arg;
	idata.flags = MMC_ERASE_FLAGS;
	ret = ioctl(fd, MMC_IOC_CMD, &idata);
	if (ret == -1) {
		MMC_ERR("mmc erase " MMC_CMD_FMT " failed: %s\n",
			MMC_ERASE, arg, idata.flags, strerror(errno));
		return ret;
	}

	return 0;
}

/*
 * TODO: fill in the function.
 */
static int mmc_do_trim(struct m_cmd_args *args, __u32 start, __u32 end)
{
	int ret;

	MMC_DEBUG("starting trim, range [%u, %u]\n", start, end);

	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_START, start);
	if (ret != 0)
		return ret;
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_END, end);
	if (ret != 0)
		return ret;
	ret = mmc_issue_erase(args->dev_fd, MMC_TRIM_ARG);
	if (ret != 0)
		return -1;

	MMC_DEBUG("trim completed\n");
	return 0;
}

static int mmc_do_sec_trim(struct m_cmd_args *args, __u32 start, __u32 end)
{
	int ret;

	MMC_DEBUG("starting secure trim, range [%u, %u]\n", start, end);

	/*
	 * Secure trim step 1.
	 */
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_START, start);
	if (ret != 0)
		return ret;
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_END, end);
	if (ret != 0)
		return ret;
	ret = mmc_issue_erase(args->dev_fd, MMC_SECURE_TRIM1_ARG);
	if (ret != 0)
		return -1;
	MMC_DEBUG("secure trim step #1 completed\n");

	/*
	 * Secure trim step 2.
	 */
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_START, start);
	if (ret != 0)
		return -1;
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_END, end);
	if (ret != 0)
		return -1;
	ret = mmc_issue_erase(args->dev_fd, MMC_SECURE_TRIM2_ARG);
	if (ret != 0)
		return ret;
	MMC_DEBUG("secure trim step #2 completed\n");

	return 0;
}

static int mmc_trim_cmd(struct m_cmd_args *args)
{
	int ret;
	__u32 start, end;

	if (mmc_flags.secure && !mmc_is_secure_supported(args->dev_fd)) {
		MMC_ERR("secure trim failed: feature not supported\n");
		return -1;
	}

	/*
	 * Determine the trim range.
	 */
	if (mmc_cal_erase_range(args, &start, &end) != 0) {
		return -1;
	}
	if (start >= end) {
		return 0;	/* Nothing needs to be done. */
	}

	if (mmc_flags.secure)
		ret = mmc_do_sec_trim(args, start, end);
	else
		ret = mmc_do_trim(args, start, end);

	return ret;
}

static int mmc_erase_cmd(struct m_cmd_args *args)
{
	int ret;
	const char *erase_type;
	__u32 start, end, erase_arg;

	if (mmc_flags.secure && !mmc_is_secure_supported(args->dev_fd)) {
		MMC_ERR("secure erase failed: feature not supported\n");
		return -1;
	}

	/*
	 * Set up erase range.
	 */
	if (mmc_cal_erase_range(args, &start, &end) != 0) {
		return -1;
	}
	if (start >= end) {
		return 0;	/* Nothing needs to be done. */
	}
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_START, start);
	if (ret != 0)
		return ret;
	ret = mmc_set_erase_range(args->dev_fd, MMC_ERASE_GROUP_END, end);
	if (ret != 0)
		return ret;

	/*
	 * Send erase command to the device.
	 */
	if (mmc_flags.secure) {
		erase_arg = MMC_SECURE_ERASE_ARG;
		erase_type = "secure erase";
	} else {
		erase_arg = MMC_ERASE_ARG;
		erase_type = "erase";
	}
	MMC_DEBUG("start %s: range [%u, %u]\n", erase_type, start, end);
	ret = mmc_issue_erase(args->dev_fd, erase_arg);
	if (ret != 0)
		return -1;

	MMC_DEBUG("%s completed\n", erase_type);
	return 0;
}

static int mmc_read_extcsd(int fd, __u8 *extcsd)
{
	int ret;
	struct mmc_ioc_cmd idata;

	memset(&idata, 0, sizeof(idata));
	memset(extcsd, 0, sizeof(__u8) * MMC_EXTCSD_SIZE);
	idata.opcode = MMC_SEND_EXT_CSD;
	idata.blksz = MMC_EXTCSD_SIZE;
	idata.blocks = 1;
	mmc_ioc_cmd_set_data(idata, extcsd);

	ret = ioctl(fd, MMC_IOC_CMD, &idata);
	if (ret < 0)
		return -1;

	return 0;
}

static int mmc_load_extcsd(struct m_cmd_args *cmd_args)
{
	int ret;

	if (!mmc_reg_cache.extcsd_loaded) {
		ret = mmc_read_extcsd(cmd_args->dev_fd, mmc_reg_cache.extcsd);
		if (ret != 0) {
			MMC_ERR("failed to read extcsd: %s\n",
				strerror(errno));
			return -1;
		}

		mmc_reg_cache.extcsd_loaded = 1;
	}

	return 0;
}

static void mmc_dump_extcsd(__u8 *extcsd)
{
	int i, rows, columns, start, end;

	columns = 16;
	rows = MMC_EXTCSD_SIZE / columns;
	assert((MMC_EXTCSD_SIZE % columns) == 0);

	/*
	 * Decimal index is used intentionally so it's consistent with
	 * JEDEC Standard; otherwise, people will have to translate index
	 * between dec/hex while looking up fields in the Standard doc.
	 */
	MMC_INFO("EXTCSD Register (decimal index, hexadecimal value):\n");
	for (i = 0; i < rows; i++) {
		start = i * columns;
		end = start + columns - 1;

		MMC_INFO("%-3d-%3d: ", start, end);
		while (start <= end) {
			MMC_INFO("%02x ", extcsd[start++]);

			/*
			 * Insert extra space between every 4 bytes for
			 * easy reading.
			 */
			if ((start & 3) == 0)
				MMC_INFO(" ");
		}
		MMC_INFO("\n");
	}
}

static int mmc_read_extcsd_cmd(struct m_cmd_args *cmd_args)
{
	if (mmc_load_extcsd(cmd_args) != 0)
		return -1;

	mmc_dump_extcsd(mmc_reg_cache.extcsd);
	return 0;
}

static struct m_cmd_info mmc_cmds[] = {
	{
		"trim",
		"send trim command to the mmc device",
		mmc_trim_cmd,
	},
	{
		"erase",
		"send erase command to the mmc device",
		mmc_erase_cmd,
	},
	{
		"read-extcsd",
		"read Extended CSD register of the mmc device",
		mmc_read_extcsd_cmd,
	},

	/* This is the last entry. */
	{
		NULL,
		NULL,
		NULL,
	},
};

static struct m_cmd_info* mmc_match_cmd(const char *cmd)
{
	int i;

	for (i = 0; mmc_cmds[i].cmd != NULL; i++) {
		if (strcmp(cmd, mmc_cmds[i].cmd) == 0) {
			return &mmc_cmds[i];
		}
	}

	return NULL;
}

static void mmc_usage(const char *prog_name)
{
	int i;
	struct {
		char *opt;
		char *desc;
	} options[] = {
		{"-h|--help", "print this help message"},
		{"-v|--verbose", "enable verbose logging"},
		{"-s|--secure", "secure trim/erase mmc device"},
		{"-o|--offset", "offset of the operation"},
		{"-l|--length", "length of the operation"},
		{NULL, NULL},
	};

	/* Print available options */
	printf("Usage: %s [options] <mmc-cmd> <mmc-device>\n", prog_name);

	printf("\nAvailable options:\n");
	for (i = 0; options[i].opt != NULL; i++) {
		printf("    %-12s - %s\n", options[i].opt, options[i].desc);
	}

	/* Print available commands */
	printf("\nSupported mmc commands:\n");
	for (i = 0; mmc_cmds[i].cmd != NULL; i++) {
		printf("    %-10s - %s\n",
		       mmc_cmds[i].cmd, mmc_cmds[i].desc);
	}
}

int main(int argc, char **argv)
{
	int ret, opt_index;
	struct m_cmd_info *cmd_info;
	struct m_cmd_args cmd_args = {
		.offset = BLKDEV_ADDR_INVALID,
		.length = BLKDEV_ADDR_INVALID,
	};
	struct option long_opts[] = {
		{"help",	no_argument,		NULL,	'h'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"secure",	no_argument,		NULL,	's'},
		{"offset",	required_argument,	NULL,	's'},
		{"length",	required_argument,	NULL,	's'},
		{NULL,		0,			NULL,	0},
	};

	while (1) {
		opt_index = 0;
		ret = getopt_long(argc, argv, "hvso:l:", long_opts, &opt_index);
		if (ret == -1)
			break;	/* end of arguments */

		switch (ret) {
		case 'h':
			mmc_usage(argv[0]);
			return 0;

		case 'v':
			mmc_flags.verbose = 1;
			break;

		case 's':
			mmc_flags.secure = 1;
			break;

		case 'o':
			cmd_args.offset = (__u32)strtol(optarg, NULL, 0);
			break;

		case 'l':
			cmd_args.length = (__u32)strtol(optarg, NULL, 0);
			break;

		default:
			return -1;
		}
	} /* while */

	if ((optind + 1) >= argc) {
		MMC_ERR("Error: mmc command/device is missing!\n\n");
		return -1;
	}
	cmd_args.cmd = argv[optind];
	cmd_args.dev_path = argv[argc - 1];

	cmd_info = mmc_match_cmd(cmd_args.cmd);
	if (cmd_info == NULL) {
		MMC_ERR("Error: unsupported command <%s>!\n", cmd_args.cmd);
		mmc_usage(argv[0]);
		return -1;
	}

	/*
	 * Open mmcblkX device.
	 */
	cmd_args.dev_fd = open(cmd_args.dev_path, O_RDWR);
	if (cmd_args.dev_fd < 0) {
		MMC_ERR("failed to open %s: %s\n",
			cmd_args.dev_path, strerror(errno));
		return -1;
	}

	/*
	 * Launch command handler.
	 */
	MMC_DEBUG("launching <%s> command (mmc=%s)\n",
		  cmd_args.cmd, cmd_args.dev_path);
	ret = cmd_info->func(&cmd_args);
	close(cmd_args.dev_fd);

	if (ret == 0) {
		MMC_INFO("mmc <%s> command completed successfully!\n",
			 cmd_args.cmd);
	}
	return ret;
}
