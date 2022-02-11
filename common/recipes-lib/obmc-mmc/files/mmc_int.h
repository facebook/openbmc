/*
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to provide addendum functionality over the I2C
 * device interfaces to utilize additional driver functionality.
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

#ifndef _MMC_INT_H_
#define _MMC_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MMC_EXTCSD_SIZE		512


/*
 * Extra MMC commands that are not defined in <openbmc/mmc.h>.
 */
#define MMC_ERASE_GROUP_START    35   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE_GROUP_END      36   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE                38   /* ac                      R1b */
#define MMC_SEND_MANUFACTURER_3  60   /* ac                      R1b */
#define MMC_SEND_MANUFACTURER_1  62   /* ac                      R1b */
#define MMC_SEND_MANUFACTURER_2  63   /* adtc                    R1  */

/*
 * Extra EXT_CSD fields that are not defined in <openbmc/mmc.h>.
 */
#define EXT_CSD_BUS_WIDTH		183

#define EXT_CSD_SEC_FEATURE_SUPPORT	231
#define SEC_SANITIZE			(1 << 6)
#define SEC_GB_CL_EN			(1 << 4)
#define SEC_BD_BLK_EN			(1 << 2)
#define SECURE_ER_EN			1

/*
 * Erase/trim/discard command arguments. Copied from Linux kernel
 * "include/linux/mmc/mmc.h".
 */
#define MMC_ERASE_ARG			0x00000000
#define MMC_SECURE_ERASE_ARG		0x80000000
#define MMC_TRIM_ARG			0x00000001
#define MMC_DISCARD_ARG			0x00000003
#define MMC_SECURE_TRIM1_ARG		0x80000001
#define MMC_SECURE_TRIM2_ARG		0x80008000
#define MMC_SECURE_ARGS			0x80000000
#define MMC_TRIM_ARGS			0x00008001

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a)		(sizeof(_a) / sizeof((_a)[0]))
#endif

/*
 * Structure to hold eMMC CID. Refer to JDDEC Standard, section 7.2 for
 * details.
 */
typedef struct {
	uint8_t	mid;		/* Manufacturer ID */
	uint8_t	cbx;		/* Device/BGA */
	uint8_t	oid;		/* OEM/Application ID */
	char	pnm[7];		/* Product Name */
	uint8_t	prv_major;	/* Product revision (major) */
	uint8_t	prv_minor;	/* Product revision (minor) */
	uint32_t	psn;		/* Product serial number */
	uint8_t	mdt_month;	/* Manufacturing date (month) */
	uint8_t	mdt_year;	/* Manufacturing date (month) */
} mmc_cid_t;

/*
 * Structure to hold eMMC Extended CSD register. Refer to JEDEC Standard,
 * section 7.4 for details.
 */
typedef struct {
	uint8_t raw[MMC_EXTCSD_SIZE];
} mmc_extcsd_t;

/*
 * Opaque type representing a eMMC device.
 */
typedef struct mmc_dev_desc mmc_dev_t;

/*
 * mmc_dev_open() opens an eMMC device. For example:
 *
 * mmc_dev_t *mmc = mmc_dev_open("/dev/mmcblk0");
 *
 * The function returns mmc_dev_t type pointer for success, or NULL on
 * failures. errno is set in case of failures.
 */
mmc_dev_t* mmc_dev_open(const char *dev_path);

/*
 * mmc_dev_close() releases all the resources allocated by mmc_dev_open().
 */
void mmc_dev_close(mmc_dev_t *mmc);

/*
 * mmc_is_secure_supported() checks if the given eMMC device supports
 * security features.
 *
 * Returns true if the device supports security features, otherwise false.
 */
bool mmc_is_secure_supported(mmc_dev_t *mmc);

/*
 * Below mmc_* functions always take mmc_dev_t type pointer, which is
 * returned by mmc_dev_open(), as the first argument.
 *
 * All the functions return 0 for success, and -1 on failures. errno is
 * set in case of failures.
 */
int mmc_sector_count(mmc_dev_t *mmc, uint32_t *sectors);
int mmc_write_blk_size(mmc_dev_t *mmc, uint32_t *size);
int mmc_write_blk_count(mmc_dev_t *mmc, uint32_t *total_blks);
int mmc_erase_range(mmc_dev_t *mmc, uint32_t erase_arg, uint32_t start, uint32_t end);
int mmc_cid_read(mmc_dev_t *mmc, mmc_cid_t *cid);
int mmc_extcsd_read(mmc_dev_t *mmc, mmc_extcsd_t *extcsd);
int mmc_extcsd_write_byte(mmc_dev_t *mmc, uint8_t index, uint8_t value);
int mmc_read_report_wd(mmc_dev_t *mmc, mmc_extcsd_t *extcsd);
int mmc_read_report_sk(mmc_dev_t *mmc, mmc_extcsd_t *extcsd);

/*
 * Below helper functions are used to convert a value to string.
 *
 * All the functions return the pointer to the input buffer.
 */
char *mmc_mfr_str(uint8_t mid, char *buf, size_t size);
char *mmc_extcsd_rev_str(uint8_t rev, char *buf, size_t size);
char *mmc_pre_eol_str(uint8_t eol, char *buf, size_t size);
char *mmc_dev_life_time_str(uint8_t life_time, char *buf, size_t size);
char *mmc_bus_width_str(uint8_t width, char *buf, size_t size);
char *mmc_reset_n_state_str(uint8_t rst_n, char *buf, size_t size);
char *mmc_cache_ctrl_str(uint8_t cache_ctrl, char *buf, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _MMC_INT_H_ */
