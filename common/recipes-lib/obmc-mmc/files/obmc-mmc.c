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

#include "mmc.h"
#include "mmc_int.h"

#define DEVICE_DIR		"/dev/"
#define	SYSFS_BLKDEV_DIR	"/sys/class/block"

#define BLKDEV_SECTOR_SIZE	512

/*
 * Following flags are copied from Micron eMMC Application Notes.
 * We may need to revisit them later (for other chips).
 */
#define MMC_ERASE_GROUP_FLAGS	0x195
#define MMC_ERASE_FLAGS		0x49d

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

#define BUILD_U32(b3, b2, b1, b0)	((uint32_t)((uint8_t)(b3) << 24 | \
					 (uint8_t)(b2) << 16 | \
					 (uint8_t)(b1) << 8  | \
					 (uint8_t)(b0)))

#define IS_VALID_MMC_DESC(_mmc)	((_mmc) != NULL && (_mmc)->cdev_fd >= 0)
#define MMC_VALIDATE_DESC(_mmc, _error) do {	\
	if (!IS_VALID_MMC_DESC(_mmc)) {		\
		errno = EINVAL;			\
		return (_error);		\
	}					\
} while (0)

struct mmc_dev_desc {
	char cdev_path[PATH_MAX];	/* /dev/mmcblk# */
	int cdev_fd;

	char sysfs_path[PATH_MAX];	/* /sys/class/block/mmcblk# */

	mmc_cid_t cid_cache;

	/*
	 * Runtime flags/options.
	 */
	unsigned long cid_loaded:1;
};

/*
 * Functions defined in lsmmc.c, used in mmc_cid_cache_fill() function.
 */
extern char *read_file(char *name);
extern void parse_bin(char *hexstr, char *fmt, ...);

static int mmc_ioc_issue_cmd(int fd, struct mmc_ioc_cmd *idata)
{
	if (ioctl(fd, MMC_IOC_CMD, idata) == -1)
		return -1;

	return 0;
}

void mmc_dev_close(mmc_dev_t *mmc)
{
	if (mmc != NULL) {
		if (mmc->cdev_fd >= 0)
			close(mmc->cdev_fd);	/* ignore errors */

		free(mmc);
	}
}

mmc_dev_t* mmc_dev_open(const char *device)
{
	char *name;
	mmc_dev_t *mmc;
	char pathname[PATH_MAX];

	if (device == NULL || device[0] == '\0') {
		errno = EINVAL;
		return NULL;
	}

	mmc = malloc(sizeof(*mmc));
	if (mmc == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memset(mmc, 0, sizeof(*mmc));

	snprintf(pathname, sizeof(pathname), "%s", device);
	name = basename(pathname);
	snprintf(mmc->cdev_path, sizeof(mmc->cdev_path), "%s/%s",
		DEVICE_DIR, name);
	snprintf(mmc->sysfs_path, sizeof(mmc->sysfs_path), "%s/%s",
		SYSFS_BLKDEV_DIR, name);

	mmc->cdev_fd = open(mmc->cdev_path, O_RDWR);
	if (mmc->cdev_fd < 0) {
		int saved_errno = errno;
		mmc_dev_close(mmc);
		errno = saved_errno;
		return NULL;
	}

	return mmc;
}

/*
 * Check whether the device supports secure feature (defined in
 * SEC_FEATURE_SUPPORT field [byte 231] of EXT_CSD).
 *
 * TODO:
 *   - we need mmc_read_extcsd() and according functions to parse
 *     EXT_CSD.
 */
bool mmc_is_secure_supported(mmc_dev_t *mmc)
{
	MMC_VALIDATE_DESC(mmc, false);
	return true;
}

/*
 * 32-bit <sectors> can address 2TB (512 * U32_MAX) device, which is way
 * more than the max capacity of MMC devices (128GB). So we are safe.
 */
int mmc_sector_count(mmc_dev_t *mmc, uint32_t *sectors)
{
	int ret;
	unsigned long num_sec;

	if (!IS_VALID_MMC_DESC(mmc) || sectors == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = ioctl(mmc->cdev_fd, BLKGETSIZE, &num_sec);
	if (ret == -1) {
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
int mmc_write_blk_size(mmc_dev_t *mmc, uint32_t *size)
{
	if (!IS_VALID_MMC_DESC(mmc) || size == NULL) {
		errno = EINVAL;
		return -1;
	}

	*size = 512;
	return 0;
}

/*
 * Get the number of write blocks of the given emmc device.
 */
int mmc_write_blk_count(mmc_dev_t *mmc, uint32_t *total_blks)
{
	uint32_t sectors, blk_size;

	if (!IS_VALID_MMC_DESC(mmc) || total_blks == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (mmc_sector_count(mmc, &sectors) != 0) {
		return -1;
	}
	if (mmc_write_blk_size(mmc, &blk_size) != 0) {
		return -1;
	}

	*total_blks = (sectors / blk_size * BLKDEV_SECTOR_SIZE);
	return 0;
}

int mmc_erase_range(mmc_dev_t *mmc, uint32_t erase_arg,
		    uint32_t start, uint32_t end)
{
	int ret;
	struct mmc_ioc_cmd idata;

	MMC_VALIDATE_DESC(mmc, -1);

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
	ret = mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
	if (ret != 0)
		return ret;

	MMC_IOC_INIT(idata, MMC_ERASE_GROUP_END, end,
		     MMC_ERASE_GROUP_FLAGS);
	ret = mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
	if (ret != 0)
		return ret;

	/*
	 * Issue CMD38 to erase/trim/discard blocks. Erase type (erase,
	 * trim or discard) is determined by <erase_arg>.
	 */
	MMC_IOC_INIT(idata, MMC_ERASE, erase_arg, MMC_ERASE_FLAGS);
	return mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
}

static int mmc_cid_cache_fill(mmc_dev_t *mmc)
{
	unsigned int crc;
	char *cid_str = NULL;
	mmc_cid_t *cid = &mmc->cid_cache;

	/* Add 64 more bytes to avoid gcc snprintf truncation warnings. */
	char pathname[PATH_MAX + 64];

	snprintf(pathname, sizeof(pathname), "%s/device/cid",
		 mmc->sysfs_path);
	cid_str = read_file(pathname);
	parse_bin(cid_str, "8u6r2u8u48a4u4u32u4u4u7u1r",
		  &cid->mid, &cid->cbx, &cid->oid, &cid->pnm[0],
		  &cid->psn, &cid->prv_major, &cid->prv_minor,
		  &cid->mdt_year, &cid->mdt_month, &crc);
	free(cid_str);

	cid->pnm[6] = '\0';	/* Product name is at most 6 bytes. */
	mmc->cid_loaded = 1;
	return 0;
}

int mmc_cid_read(mmc_dev_t *mmc, mmc_cid_t *cid)
{
	int ret;

	if (!IS_VALID_MMC_DESC(mmc) || cid == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (!mmc->cid_loaded) {
		ret = mmc_cid_cache_fill(mmc);
		if (ret)
			return ret;
	}

	memcpy(cid, &mmc->cid_cache, sizeof(*cid));
	return 0;
}

int mmc_extcsd_read(mmc_dev_t *mmc, mmc_extcsd_t *extcsd)
{
	int ret;
	struct mmc_ioc_cmd idata;

	if (!IS_VALID_MMC_DESC(mmc) || extcsd == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(extcsd, 0, sizeof(*extcsd));
	MMC_IOC_INIT(idata, MMC_SEND_EXT_CSD, 0, 0);
	MMC_IOC_SET_DATA(idata, extcsd, 1, sizeof(*extcsd), 0);
	ret = mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
	if (ret < 0)
		return -1;

	return 0;
}

/*
 * Below logic is copied from mmc-utils, write_extcsd_value() function.
 */
int mmc_extcsd_write_byte(mmc_dev_t *mmc, uint8_t index, uint8_t value)
{
	uint32_t arg, flags;
	struct mmc_ioc_cmd idata;

	MMC_VALIDATE_DESC(mmc, -1);

	arg = BUILD_U32(MMC_SWITCH_MODE_WRITE_BYTE, index, value,
			EXT_CSD_CMD_SET_NORMAL);
	flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	MMC_IOC_INIT(idata, MMC_SWITCH, arg, flags);
	idata.write_flag = 1;

	return mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
}

/*
 * Read vendor specific device report from WD iNAND 7250.
 */
int mmc_read_report_wd(mmc_dev_t *mmc, mmc_extcsd_t *extcsd)
{
	int ret;
	uint32_t arg, flags;
	struct mmc_ioc_cmd idata;

	if (!IS_VALID_MMC_DESC(mmc) || extcsd == NULL) {
		errno = EINVAL;
		return -1;
	}

	arg = BUILD_U32(0x96, 0xC9, 0xD7, 0x1C);
	flags = (MMC_RSP_R1B | MMC_CMD_AC);
	MMC_IOC_INIT(idata, MMC_SEND_MANUFACTURER_1, arg, flags);
	ret = mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
	if (ret != 0)
		return ret;

	memset(extcsd, 0, sizeof(*extcsd));
	MMC_IOC_INIT(idata, MMC_SEND_MANUFACTURER_2, 0, 0);
	MMC_IOC_SET_DATA(idata, &extcsd->raw, 1, MMC_EXTCSD_SIZE, 0);
	return mmc_ioc_issue_cmd(mmc->cdev_fd, &idata);
}

/*
 * Read vendor specific device report from SK hynix.
 */
int mmc_read_report_sk(mmc_dev_t *mmc, mmc_extcsd_t *extcsd)
{
	int ret = 0;
	int num_cmds = 3;
	uint8_t dummy_data[MMC_EXTCSD_SIZE];
	struct mmc_ioc_multi_cmd *multi_cmd;

	if (!IS_VALID_MMC_DESC(mmc) || extcsd == NULL) {
		errno = EINVAL;
		return -1;
	}

	multi_cmd = calloc(1, sizeof(struct mmc_ioc_multi_cmd) +
			   num_cmds * sizeof(struct mmc_ioc_cmd));
	if (multi_cmd == NULL) {
		errno = ENOMEM;
		return -1;
	}

	multi_cmd->num_of_cmds = num_cmds;
	memset(dummy_data, 0, sizeof(dummy_data));
	memset(extcsd, 0, sizeof(*extcsd));

	multi_cmd->cmds[0].opcode = MMC_SEND_MANUFACTURER_3;
	multi_cmd->cmds[0].arg = 0x534D4900;
	multi_cmd->cmds[0].flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	multi_cmd->cmds[0].write_flag = 1;
	mmc_ioc_cmd_set_data(multi_cmd->cmds[0], dummy_data);

	multi_cmd->cmds[1].opcode = MMC_SEND_MANUFACTURER_3;
	multi_cmd->cmds[1].arg = 0x48525054;
	multi_cmd->cmds[1].flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	multi_cmd->cmds[1].write_flag = 1;
	mmc_ioc_cmd_set_data(multi_cmd->cmds[1], dummy_data);

	multi_cmd->cmds[2].opcode = MMC_SEND_EXT_CSD;
	multi_cmd->cmds[2].arg = 0;
	multi_cmd->cmds[2].flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	multi_cmd->cmds[2].write_flag = 0;
	multi_cmd->cmds[2].blksz = MMC_EXTCSD_SIZE;
	multi_cmd->cmds[2].blocks = 1;
	mmc_ioc_cmd_set_data(multi_cmd->cmds[2], &extcsd->raw);

	ret = ioctl(mmc->cdev_fd, MMC_IOC_MULTI_CMD, multi_cmd);
	free(multi_cmd);

	return (ret == -1) ? ret : 0;
}

char *mmc_mfr_str(uint8_t mid, char *buf, size_t size)
{
	const char *mfr_name = NULL;

	if (buf == NULL || size == 0)
		return NULL;

	switch (mid) {
	case 0x03:
	case 0x11:
		mfr_name = "Toshiba";
		break;

	case 0x13:
	case 0xfe:
		mfr_name = "Micron";
		break;

	case 0x45:
		mfr_name = "Western Digital";
		break;
	}

	if (mfr_name) {
		snprintf(buf, size, "%s", mfr_name);
	} else {
		snprintf(buf, size, "MFR(%#02X)", mid);
	}

	return buf;
}

char *mmc_extcsd_rev_str(uint8_t rev, char *buf, size_t size)
{
	static const char *revisions[] = {
		"v4.0",
		"v4.1",
		"v4.2",
		"v4.3",
		"Obsolete",
		"v4.41",
		"v4.5",
		"v5.0",
		"v5.1",
	};

	if (buf == NULL || size == 0)
		return NULL;

	if (rev < ARRAY_SIZE(revisions)) {
		snprintf(buf, size, "%s", revisions[rev]);
	} else {
		snprintf(buf, size, "REV(%#02x)", rev);
	}

	return buf;
}

char *mmc_pre_eol_str(uint8_t eol, char *buf, size_t size)
{
	static const char *eol_states[] = {
		"Undefined",
		"Normal",
		"Wanring",
		"Urgent",
	};

	if (buf == NULL || size == 0)
		return NULL;

	if (eol < ARRAY_SIZE(eol_states)) {
		snprintf(buf, size, "%s", eol_states[eol]);
	} else {
		snprintf(buf, size, "EOL(%#02x)", eol);
	}

	return buf;
}

char *mmc_dev_life_time_str(uint8_t life_time, char *buf, size_t size)
{
	static const char *life_time_ranges[] = {
		"Undefined",
		"0%-10% life time used",
		"10%-20% life time used",
		"20%-30% life time used",
		"30%-40% life time used",
		"40%-50% life time used",
		"50%-60% life time used",
		"60%-70% life time used",
		"70%-80% life time used",
		"80%-90% life time used",
		"90%-10% life time used",
		"excedded maximum life time",
	};

	if (buf == NULL || size == 0)
		return NULL;

	if (life_time < ARRAY_SIZE(life_time_ranges)) {
		snprintf(buf, size, "%s", life_time_ranges[life_time]);
	} else {
		snprintf(buf, size, "LifeTime(%#02x)", life_time);
	}

	return buf;
}

char *mmc_bus_width_str(uint8_t width, char *buf, size_t size)
{
	static const char *bus_width[] = {
		"1",
		"4",
		"8",
		"Reserved",
		"Reserved",
		"4 (dual data rate)",
		"8 (dual data rate)",
	};

	if (buf == NULL || size == 0)
		return NULL;

	if (width < ARRAY_SIZE(bus_width)) {
		snprintf(buf, size, "%s", bus_width[width]);
	} else {
		snprintf(buf, size, "WIDTH(%#02x)", width);
	}

	return buf;
}

char *mmc_reset_n_state_str(uint8_t rst_n, char *buf, size_t size)
{
	static const char *reset_states[] = {
		"temporarily disabled (default)",
		"permanently enabled",
		"permanently disabled",
		"Reserved",
	};

	if (buf == NULL || size == 0)
		return NULL;

	if (rst_n < ARRAY_SIZE(reset_states)) {
		snprintf(buf, size, "%s", reset_states[rst_n]);
	} else {
		snprintf(buf, size, "RST_N(%#02x)", rst_n);
	}

	return buf;
}

char *mmc_cache_ctrl_str(uint8_t cache_ctrl, char *buf, size_t size)
{
	static const char *cache_states[] = {
		"OFF",
		"ON",
	};

	if (buf == NULL || size == 0)
		return NULL;

	if (cache_ctrl < ARRAY_SIZE(cache_states)) {
		snprintf(buf, size, "%s", cache_states[cache_ctrl]);
	} else {
		snprintf(buf, size, "CACHE_CTRL(%#02x)", cache_ctrl);
	}

	return buf;
}
