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

/*
 * This library is usually used to access pmbus registers which are not
 * exported by pmbus kernel drivers.
 */

#ifndef _OBMC_PMBUS_H_
#define _OBMC_PMBUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdint.h>

#ifndef BIT
#define BIT(pos)		(1UL << (pos))
#endif

/*
 * Opaque data type to represent a pmbus device.
 */
typedef struct pmbus_device pmbus_dev_t;

/*
 * Supported PMBus Commands. Refer to PMBus Spec, "APPENDIX I. Command
 * Summary" for details.
 */
enum pmbus_regs {
	PMBUS_PAGE = 0x00,
	PMBUS_OPERATION = 0x01,
	PMBUS_MFR_ID = 0x99,
	PMBUS_MFR_MODEL = 0x9A,
};

/*
 * Supported OPERATION values. Refer to PMBus Spec, section 12.1 for
 * details.
 */
#define PMBUS_OPERATION_ON	BIT(7)
#define PMBUS_OPERATION_OFF	0

/*
 * Flags for pmbus_device_open().
 *
 * - PMBUS_FLAG_FORCE_CLAIM
 *   force open the device even though it may be controlled by a kernel
 *   driver.
 */
#define PMBUS_FLAG_FORCE_CLAIM	BIT(0)

/*
 * Open the pmbus device for future read/write.
 *
 * Return:
 *   the opaque "pmbus_dev_t" type for  success, or NULL on failures.
 */
pmbus_dev_t* pmbus_device_open(int bus, uint16_t addr, int flags);

/*
 * Release all the resources allocated by pmbus_device_open().
 *
 * Return:
 *   None.
 */
void pmbus_device_close(pmbus_dev_t *pmdev);

/*
 * Update the page of given pmbus device.
 *
 * Return:
 *   0 for success, and -1 on failures. errno is set in case of failures.
 */
int pmbus_set_page(pmbus_dev_t *pmdev, uint8_t page);

/*
 * Send byte command to the given pmbus device.
 *
 * Return:
 *   0 for success, and -1 on failures. errno is set in case of failures.
 */
int pmbus_write_byte_data(pmbus_dev_t *pmdev, uint8_t page,
			  uint8_t reg, uint8_t value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_PMBUS_H_ */
