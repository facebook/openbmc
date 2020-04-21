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
#include <ctype.h>
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

/*
 * Utilities to initialize struct mmc_ioc_cmd.
 */
#define MMC_IOC_INIT(_idata, _opcode, _arg, _flags)	\
	do {						\
		memset(&(_idata), 0, sizeof(_idata));	\
		(_idata).opcode = (_opcode);		\
		(_idata).arg = (_arg);			\
		(_idata).flags = (_flags);		\
	} while (0)

#define MMC_IOC_SET_DATA(_idata, _data, _blocks, _blksz, _wr_flag)	\
	do {								\
		mmc_ioc_cmd_set_data(_idata, _data);			\
		(_idata).blocks = (_blocks);				\
		(_idata).blksz = (_blksz);				\
		(_idata).write_flag = (_wr_flag);			\
	} while (0)

#define BUILD_U32(b3, b2, b1, b0)	((__u32)((__u8)(b3) << 24 | \
						 (__u8)(b2) << 16 | \
						 (__u8)(b1) << 8  | \
						 (__u8)(b0)))

struct m_cmd_args {
	const char *cmd;
	const char *dev_path;

	int dev_fd;

	/*
	 * offset and length associated with the command, and they may
	 * have different meanings depending on commands. For example:
	 *  - trim/erase "offset" specifies the starting address of the
	 *    command.
	 *  - write-extcsd "offset" refers to the starting index within
	 *    512-byte extcsd register.
	 */
	__u32 offset;
	__u32 length;

	void* data;	/* Data associated with the command. */
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


static int mmc_ioc_issue_cmd(int fd, struct mmc_ioc_cmd *idata)
{
	MMC_DEBUG("starting ioctl cmd " MMC_CMD_FMT "\n",
		  idata->opcode, idata->arg, idata->flags);
	if (ioctl(fd, MMC_IOC_CMD, idata) == -1)
		return -1;

	return 0;
}

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

static int mmc_erase_range(int fd, __u32 erase_arg, __u32 start, __u32 end)
{
	int ret;
	struct mmc_ioc_cmd idata;

	/*
	 * Issue CMD35/CMD36 to set ERASE_GROUP_START|END.
	 * NOTE:
	 *   - For devices <=2GB, <arg> defines byte address.
	 *   - For devices >2GB, <arg> defines sector (512-byte) address.
	 *   - Erase applies to Erase Groups, while Trim/Discard applies
	 *     to Write Blocks.
	 */
	MMC_IOC_INIT(idata, MMC_ERASE_GROUP_START, start,
		     MMC_ERASE_GROUP_FLAGS);
	ret = mmc_ioc_issue_cmd(fd, &idata);
	if (ret != 0) {
		MMC_ERR("failed to set erase start " MMC_CMD_FMT ": %s\n",
			MMC_ERASE_GROUP_START, start, MMC_ERASE_GROUP_FLAGS,
			strerror(errno));
		return ret;
	}

	MMC_IOC_INIT(idata, MMC_ERASE_GROUP_END, end,
		     MMC_ERASE_GROUP_FLAGS);
	ret = mmc_ioc_issue_cmd(fd, &idata);
	if (ret != 0) {
		MMC_ERR("failed to set erase end " MMC_CMD_FMT ": %s\n",
			MMC_ERASE_GROUP_END, end, MMC_ERASE_GROUP_FLAGS,
			strerror(errno));
		return ret;
	}

	/*
	 * Issue CMD38 to erase/trim/discard blocks. Erase type (erase,
	 * trim or discard) is determined by <erase_arg>.
	 */
	MMC_IOC_INIT(idata, MMC_ERASE, erase_arg, MMC_ERASE_FLAGS);
	ret = mmc_ioc_issue_cmd(fd, &idata);
	if (ret != 0) {
		MMC_ERR("erase command " MMC_CMD_FMT " failed: %s\n",
			MMC_ERASE, erase_arg, 0, strerror(errno));
		return ret;
	}

	return 0;
}

static int mmc_do_sec_trim(int fd, __u32 start, __u32 end)
{
	int ret;

	MMC_DEBUG("starting secure-trim step #1: range [%u, %u]\n",
		  start, end);
	ret = mmc_erase_range(fd, MMC_SECURE_TRIM1_ARG, start, end);
	if (ret != 0) {
		return ret;
	}

	MMC_DEBUG("starting secure-trim step #2: range [%u, %u]\n",
		  start, end);
	ret = mmc_erase_range(fd, MMC_SECURE_TRIM2_ARG, start, end);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int mmc_trim_cmd(struct m_cmd_args *args)
{
	int ret;
	__u32 start, end;

	/*
	 * Determine the trim range.
	 */
	if (mmc_cal_erase_range(args, &start, &end) != 0)
		return -1;
	if (start >= end)
		return 0;	/* Nothing needs to be done. */

	if (mmc_flags.secure) {
		if (!mmc_is_secure_supported(args->dev_fd)) {
			MMC_ERR("Error: secure feature not supported\n");
			return -1;
		}

		ret = mmc_do_sec_trim(args->dev_fd, start, end);
	} else {
		MMC_DEBUG("starting trim: range [%u, %u]\n", start, end);
		ret = mmc_erase_range(args->dev_fd, MMC_TRIM_ARG, start, end);
	}

	return ret;
}

static int mmc_erase_cmd(struct m_cmd_args *args)
{
	int ret;
	__u32 start, end;

	if (mmc_cal_erase_range(args, &start, &end) != 0)
		return -1;
	if (start >= end)
		return 0;       /* Nothing needs to be done. */

	if (mmc_flags.secure) {
		if (!mmc_is_secure_supported(args->dev_fd)) {
			MMC_ERR("Error: secure feature not supported\n");
			return -1;
		}

		MMC_DEBUG("starting secure-erase: range [%u, %u]\n",
			  start, end);
		ret = mmc_erase_range(args->dev_fd, MMC_SECURE_ERASE_ARG,
				      start, end);
	} else {
		MMC_DEBUG("starting erase: range [%u, %u]\n", start, end);
		ret = mmc_erase_range(args->dev_fd, MMC_ERASE_ARG, start, end);
	}

	return ret;
}

static int mmc_read_extcsd(int fd, __u8 *extcsd)
{
	struct mmc_ioc_cmd idata;

	memset(extcsd, 0, sizeof(__u8) * MMC_EXTCSD_SIZE);
	MMC_IOC_INIT(idata, MMC_SEND_EXT_CSD, 0, 0);
	MMC_IOC_SET_DATA(idata, extcsd, 1, MMC_EXTCSD_SIZE, 0);
	return mmc_ioc_issue_cmd(fd, &idata);
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

/*
 * Below logic is copied from mmc-utils, write_extcsd_value() function.
 */
static int mmc_extcsd_write_byte(int fd, __u8 index, __u8 value)
{
	__u32 arg, flags;
	struct mmc_ioc_cmd idata;

	arg = BUILD_U32(MMC_SWITCH_MODE_WRITE_BYTE, index, value,
			EXT_CSD_CMD_SET_NORMAL);
	flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	MMC_IOC_INIT(idata, MMC_SWITCH, arg, flags);
	idata.write_flag = 1;

	return mmc_ioc_issue_cmd(fd, &idata);
}

static int mmc_extcsd_data_parse(char *input, __u8 *values, size_t size)
{
	int i = 0;
	char *start, *end;

	for (start = input; *start != '\0' && i < size; start = end) {
		__u8 val = (__u8)strtol(start, &end, 0);
		if (start == end)
			return -1;	/* invalid entry. */

		values[i++] = val;

		/*
		 * remove trailing spaces to avoid strtol() failure in
		 * case <input> has trailing spaces.
		 */
		while(isspace(*end))
			end++;
	}

	return i; /* number of entries. */
}

static int mmc_write_extcsd_cmd(struct m_cmd_args *cmd_args)
{
	__u8 values[MMC_EXTCSD_SIZE];
	int i, ret, num, offset;

	if (cmd_args->data == NULL) {
		MMC_ERR("Error: data option is required for '%s' command\n",
			cmd_args->cmd);
		return -1;
	}
	if (cmd_args->offset >= MMC_EXTCSD_SIZE) {
		MMC_ERR("Error: '%s' command offset exceeds limit (%d)\n",
			cmd_args->cmd, MMC_EXTCSD_SIZE);
		return -1;
	}

	/*
	 * data should be list of 8-byte unsigned integers separated by
	 * spaces. For example, "0x3b 0 0xff".
	 */
	offset = cmd_args->offset;
	num = mmc_extcsd_data_parse(cmd_args->data, values,
				    MMC_EXTCSD_SIZE - offset);
	if (num <= 0) {
		MMC_ERR("Error: invalid data <%s> for command %s\n",
			(char*)cmd_args->data, cmd_args->cmd);
		return -1;
	}

	for (i = 0; i < num; i++) {
		MMC_DEBUG("writing %#x to extcsd[%d]\n",
			  values[i], offset + i);
		ret = mmc_extcsd_write_byte(cmd_args->dev_fd, offset + i,
					    values[i]);

		/*
		 * If unfortunately write fails, we will exit immediately,
		 * and callers need to decide how to roll back or fix write
		 * failures.
		 */
		if (ret < 0) {
			MMC_ERR("failed to write %#x to extcsd[%d]: %s\n",
				values[i], offset + i, strerror(errno));
			return -1;
		}
	}

	return 0;
}

static int mmc_read_report_cmd(struct m_cmd_args *cmd_args)
{
	__u32  arg, flags;
	__u8 *extcsd = mmc_reg_cache.extcsd;
	struct mmc_ioc_cmd idata;
	int ret;

	arg = BUILD_U32(0x96, 0xC9, 0xD7, 0x1C);
	flags = ( MMC_RSP_R1B | MMC_CMD_AC );
	MMC_IOC_INIT(idata, MMC_SEND_MANUFACTURER_1, arg, flags);
	ret = mmc_ioc_issue_cmd(cmd_args->dev_fd, &idata);
	if (ret != 0) {
		MMC_ERR("Error: query for CMD%d \n",
			MMC_SEND_MANUFACTURER_1);
		return ret;
	}

	memset(extcsd, 0, sizeof(__u8) * MMC_EXTCSD_SIZE);
	MMC_IOC_INIT(idata, MMC_SEND_MANUFACTURER_2, 0, 0);
	MMC_IOC_SET_DATA(idata, extcsd, 1, MMC_EXTCSD_SIZE, 0);
	ret = mmc_ioc_issue_cmd(cmd_args->dev_fd, &idata);
	if (ret != 0) {
		MMC_ERR("Error: read device report for CMD%d \n",
			MMC_SEND_MANUFACTURER_2);
		return ret;
	}

	mmc_dump_extcsd(extcsd);
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
	{
		"write-extcsd",
		"write Extended CSD register of the mmc device",
		mmc_write_extcsd_cmd,
	},
	{
		"read-devreport",
		"read device report of the mmc device (support for WD iNAND 7250)",
		mmc_read_report_cmd,
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
		{"-D|--data", "data associated with the command"},
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
		.data = NULL,
	};
	struct option long_opts[] = {
		{"help",	no_argument,		NULL,	'h'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"secure",	no_argument,		NULL,	's'},
		{"offset",	required_argument,	NULL,	'o'},
		{"length",	required_argument,	NULL,	'l'},
		{"data",	required_argument,	NULL,	'D'},
		{NULL,		0,			NULL,	0},
	};

	while (1) {
		opt_index = 0;
		ret = getopt_long(argc, argv, "hvso:l:D:", long_opts, &opt_index);
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

		case 'D':
			cmd_args.data = optarg;
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
	MMC_DEBUG("starting '%s' command (mmc=%s)\n",
		  cmd_args.cmd, cmd_args.dev_path);
	ret = cmd_info->func(&cmd_args);

	close(cmd_args.dev_fd);

	if (ret == 0) {
		MMC_INFO("'%s' command completed successfully!\n",
			 cmd_args.cmd);
	} else {
		MMC_ERR("'%s' command failed with errors!\n", cmd_args.cmd);
	}

	return ret;
}
