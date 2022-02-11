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
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <regex.h>

#include <openbmc/obmc-mmc.h>

#define BLKDEV_ADDR_INVALID	UINT32_MAX

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
	char dev_path[PATH_MAX];

	mmc_dev_t *mmc;

	/*
	 * offset and length associated with the command, and they may
	 * have different meanings depending on commands. For example:
	 *  - trim/erase "offset" specifies the starting address of the
	 *    command.
	 *  - write-extcsd "offset" refers to the starting index within
	 *    512-byte extcsd register.
	 */
	uint32_t offset;
	uint32_t length;

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

static int mmc_cal_erase_range(struct m_cmd_args *args,
			       uint32_t *start,
			       uint32_t *end)
{
	int ret;
	uint32_t total_blks;

	/*
	 * Get total number of sectors (512 bytes) of the device.
	 */
	ret = mmc_write_blk_count(args->mmc, &total_blks);
	if (ret != 0)
		return -1;
	if (total_blks == 0) {
		*start = *end = 0;
		return 0;
	}

	/*
	 * Set <start> to 0 unless it's specified in command line.
	 */
	if (args->offset == BLKDEV_ADDR_INVALID)
		*start = 0;
	else
		*start = args->offset;

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

static int mmc_do_sec_trim(mmc_dev_t *mmc, uint32_t start, uint32_t end)
{
	int ret;

	MMC_DEBUG("starting secure-trim step #1: range [%u, %u]\n",
		  start, end);
	ret = mmc_erase_range(mmc, MMC_SECURE_TRIM1_ARG, start, end);
	if (ret != 0)
		return ret;

	MMC_DEBUG("starting secure-trim step #2: range [%u, %u]\n",
		  start, end);
	ret = mmc_erase_range(mmc, MMC_SECURE_TRIM2_ARG, start, end);
	if (ret != 0)
		return ret;

	return 0;
}

static int mmc_trim_cmd(struct m_cmd_args *args)
{
	int ret;
	uint32_t start, end;
	mmc_dev_t *mmc = args->mmc;

	/*
	 * Determine the trim range.
	 */
	if (mmc_cal_erase_range(args, &start, &end) != 0)
		return -1;
	if (start >= end)
		return 0;	/* Nothing needs to be done. */

	if (mmc_flags.secure) {
		if (!mmc_is_secure_supported(mmc)) {
			MMC_ERR("Error: secure feature not supported\n");
			return -1;
		}

		ret = mmc_do_sec_trim(mmc, start, end);
	} else {
		MMC_DEBUG("starting trim: range [%u, %u]\n", start, end);
		ret = mmc_erase_range(mmc, MMC_TRIM_ARG, start, end);
	}

	return ret;
}

static int mmc_erase_cmd(struct m_cmd_args *args)
{
	int ret;
	uint32_t start, end;
	mmc_dev_t *mmc = args->mmc;

	if (mmc_cal_erase_range(args, &start, &end) != 0)
		return -1;
	if (start >= end)
		return 0;       /* Nothing needs to be done. */

	if (mmc_flags.secure) {
		if (!mmc_is_secure_supported(mmc)) {
			MMC_ERR("Error: secure feature not supported\n");
			return -1;
		}

		MMC_DEBUG("starting secure-erase: range [%u, %u]\n",
			  start, end);
		ret = mmc_erase_range(mmc, MMC_SECURE_ERASE_ARG, start, end);
	} else {
		MMC_DEBUG("starting erase: range [%u, %u]\n", start, end);
		ret = mmc_erase_range(mmc, MMC_ERASE_ARG, start, end);
	}

	return ret;
}

static void mmc_dump_cid(mmc_cid_t *cid)
{
	char mfr[NAME_MAX];

	MMC_INFO("- Manufacturer: %s\n",
		mmc_mfr_str(cid->mid, mfr, sizeof(mfr)));
	MMC_INFO("- Device Type (CBX): %#01x\n", cid->cbx);
	MMC_INFO("- OEM/Application ID: %#02x\n", cid->oid);
	MMC_INFO("- Product Name: %s\n", cid->pnm);
	MMC_INFO("- Product Revision: %d.%d\n",
		 cid->prv_major, cid->prv_minor);
	MMC_INFO("- Product Serial Number: %#08x\n", cid->psn);
	MMC_INFO("- Manufacturing Date (month/year): %d/%d\n",
		 cid->mdt_month, cid->mdt_year);
}

static void mmc_dump_extcsd(mmc_extcsd_t *extcsd)
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
			MMC_INFO("%02x ", extcsd->raw[start++]);

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

static int mmc_read_cid_cmd(struct m_cmd_args *cmd_args)
{
	int ret;
	mmc_cid_t cid;

	MMC_INFO("eMMC %s CID Register:\n", cmd_args->dev_path);
	ret = mmc_cid_read(cmd_args->mmc, &cid);
	if (ret < 0) {
		MMC_ERR("failed to read cid: %s\n", strerror(errno));
		return -1;
	}

	mmc_dump_cid(&cid);
	MMC_INFO("\n");

	return 0;
}

static int mmc_read_extcsd_cmd(struct m_cmd_args *cmd_args)
{
	int ret;
	mmc_extcsd_t extcsd;

	ret = mmc_extcsd_read(cmd_args->mmc, &extcsd);
	if (ret < 0) {
		MMC_ERR("failed to read extcsd: %s\n", strerror(errno));
		return -1;
	}

	mmc_dump_extcsd(&extcsd);
	MMC_INFO("\n");
	return 0;
}

static int mmc_extcsd_data_parse(char *input, uint8_t *values, size_t size)
{
	int i = 0;
	char *start, *end;

	for (start = input; *start != '\0' && i < size; start = end) {
		uint8_t val = (uint8_t)strtol(start, &end, 0);
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
	uint8_t values[MMC_EXTCSD_SIZE];
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
		ret = mmc_extcsd_write_byte(cmd_args->mmc, offset + i,
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

static void regex_extract(char * buf, char * data, const char * pattern,
			  size_t matchId)
{
	int status;
	int cflags = REG_EXTENDED | REG_ICASE;
	const size_t nmatch = 10;
	regmatch_t pmatch[nmatch];
	regex_t reg;

	regcomp(&reg, pattern, cflags);
	status = regexec(&reg, data, nmatch, pmatch, 0);
	if (status == REG_NOMATCH) {
		buf[0] = '\0';
	} else if (status == 0) {
		int len = pmatch[matchId].rm_eo - pmatch[matchId].rm_so;

		strncpy(buf, data + pmatch[matchId].rm_so, len);
		buf[len] = '\0';
	}

	regfree(&reg);
}

static int mmc_bus_width_debugfs(uint8_t *width)
{
	FILE *fp;
	int ret = -1;
	char data[1024];
	const char *path = "/sys/kernel/debug/mmc0/ios";
	const char *pattern = "bus width:\t([0-9]+)";

	fp = fopen(path, "r");
	if (fp == NULL)
		return -1;

	if (fread(data, sizeof(data), 1, fp) == 1) {
		char buf[1024];

		buf[0] = '\0';
		data[sizeof(data) - 1] = '\0';
		regex_extract(buf, data, pattern, 1);
		if (strlen(buf) > 0) {
			*width = (uint8_t)atoi(buf) - 1;
			ret = 0;
		}
	}

	fclose(fp);
	return ret;
}

static void mmc_dump_base_info(mmc_cid_t *cid, mmc_extcsd_t *extcsd)
{
	char buf[NAME_MAX];
	uint8_t rev = extcsd->raw[EXT_CSD_REV];

	MMC_INFO("- Vendor/Product: %s %s\n",
		mmc_mfr_str(cid->mid, buf, sizeof(buf)), cid->pnm);
	MMC_INFO("- eMMC Revision: %s\n",
		mmc_extcsd_rev_str(rev, buf, sizeof(buf)));
}

static void mmc_dump_secure_info(mmc_extcsd_t *extcsd)
{
	uint8_t sec_info = extcsd->raw[EXT_CSD_SEC_FEATURE_SUPPORT];

	MMC_INFO("- Secure Feature: %s%s%s%s\n",
		(sec_info & SEC_SANITIZE) ? "sanitize," : "",
		(sec_info & SEC_GB_CL_EN) ? "secure-trim,"  : "",
		(sec_info & SEC_BD_BLK_EN) ? "auto-erase,"  : "",
		(sec_info & SECURE_ER_EN) ? "secure-purge" : "");
}

static void mmc_dump_health_info(mmc_extcsd_t *extcsd)
{
	char buf[NAME_MAX];
	uint8_t eol = extcsd->raw[EXT_CSD_PRE_EOL_INFO];
	uint8_t lta = extcsd->raw[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A];
	uint8_t ltb = extcsd->raw[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B];

	MMC_INFO("- Device Health (PRE_EOL): %s\n",
		mmc_pre_eol_str(eol, buf, sizeof(buf)));
	MMC_INFO("- Estimated Life Time (Type A): %s\n",
		mmc_dev_life_time_str(lta, buf, sizeof(buf)));
	if (ltb > 0) {
		MMC_INFO("- Estimated Life Time (Type B): %s\n",
			mmc_dev_life_time_str(ltb, buf, sizeof(buf)));
	}
}

static void mmc_dump_misc_info(mmc_extcsd_t *extcsd)
{
	char buf[NAME_MAX];
	uint8_t width, rst_n, cache_ctrl;

	if (mmc_bus_width_debugfs(&width) != 0) {
		width = extcsd->raw[EXT_CSD_BUS_WIDTH];
	}
	MMC_INFO("- Bus Width: %s\n",
		mmc_bus_width_str(width, buf, sizeof(buf)));

	rst_n = extcsd->raw[EXT_CSD_RST_N_FUNCTION];
	MMC_INFO("- H/W Reset Function: %s\n",
		mmc_reset_n_state_str(rst_n, buf, sizeof(buf)));

	cache_ctrl = extcsd->raw[EXT_CSD_CACHE_CTRL];
	MMC_INFO("- Cache Control: %s\n",
		mmc_cache_ctrl_str(cache_ctrl, buf, sizeof(buf)));
}

static int mmc_show_summary_cmd(struct m_cmd_args *cmd_args)
{
	int ret;
	mmc_cid_t cid;
	mmc_extcsd_t extcsd;

	/*
	 * Load CID and EXT_CSD for reference.
	 */
	ret = mmc_cid_read(cmd_args->mmc, &cid);
	if (ret < 0)
		return -1;
	ret = mmc_extcsd_read(cmd_args->mmc, &extcsd);
	if (ret < 0)
		return -1;

	/*
	 * Basic device information.
	 */
	MMC_INFO("eMMC %s Device Summary:\n", cmd_args->dev_path);
	mmc_dump_base_info(&cid, &extcsd);

	/*
	 * Secure feature.
	 */
	mmc_dump_secure_info(&extcsd);

	/*
	 * Health status.
	 */
	mmc_dump_health_info(&extcsd);

	/*
	 * Others.
	 */
	mmc_dump_misc_info(&extcsd);

	MMC_INFO("\n");
	return 0;
}

static int mmc_read_report_cmd_wd(struct m_cmd_args *cmd_args)
{
	int ret;
	mmc_extcsd_t extcsd;

	ret = mmc_read_report_wd(cmd_args->mmc, &extcsd);
	if (ret < 0)
		return ret;

	mmc_dump_extcsd(&extcsd);
	return 0;
}

static int mmc_read_report_cmd_sk(struct m_cmd_args *cmd_args)
{
	int ret;
	mmc_extcsd_t extcsd;

	ret = mmc_read_report_sk(cmd_args->mmc, &extcsd);
	if (ret < 0)
		return ret;

	mmc_dump_extcsd(&extcsd);
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
		"read-cid",
		"read CID register of the mmc device",
		mmc_read_cid_cmd,
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
		"show-summary",
		"display summary information of the mmc device",
		mmc_show_summary_cmd,
	},
	{
		"read-devreport-wd",
		"read device report of the mmc device (support for WD iNAND 7250)",
		mmc_read_report_cmd_wd,
	},
	{
		"read-devreport-sk",
		"read device report of the mmc device (support for SK hynix)",
		mmc_read_report_cmd_sk,
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
	char *mmc_path;
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
			cmd_args.offset = (uint32_t)strtol(optarg, NULL, 0);
			break;

		case 'l':
			cmd_args.length = (uint32_t)strtol(optarg, NULL, 0);
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
	mmc_path = argv[argc - 1];
	snprintf(cmd_args.dev_path, sizeof(cmd_args.dev_path),
		 "%s", argv[argc - 1]);

	cmd_info = mmc_match_cmd(cmd_args.cmd);
	if (cmd_info == NULL) {
		MMC_ERR("Error: unsupported command <%s>!\n", cmd_args.cmd);
		mmc_usage(argv[0]);
		return -1;
	}

	/*
	 * Open mmcblkX device.
	 */
	cmd_args.mmc = mmc_dev_open(mmc_path);
	if (cmd_args.mmc == NULL) {
		MMC_ERR("failed to open %s: %s\n",
			mmc_path, strerror(errno));
		return -1;
	}

	/*
	 * Launch command handler.
	 */
	MMC_DEBUG("starting '%s' command (mmc=%s)\n",
		  cmd_args.cmd, mmc_path);
	ret = cmd_info->func(&cmd_args);

	mmc_dev_close(cmd_args.mmc);

	if (ret == 0) {
		MMC_DEBUG("'%s' command completed successfully!\n",
			 cmd_args.cmd);
	} else {
		MMC_ERR("'%s' command failed with errors!\n", cmd_args.cmd);
	}

	return ret;
}
