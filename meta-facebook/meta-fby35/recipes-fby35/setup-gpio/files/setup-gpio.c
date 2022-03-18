/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include <openbmc/phymem.h>
#include <openbmc/libgpio.h>
#include <facebook/fby35_gpio.h>
#include <facebook/fby35_common.h>

#define GPIO_BASE    	0x1E780000
#define REG_GPIO0FC   	0x0FC

#define SCU_BASE        0x1E6E2000
#define REG_SCU630      0x630

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))

int setup_gpio_with_value(const char *chip_name, const char *shadow_name, const char *pin_name, int offset, gpio_direction_t direction, gpio_value_t value)
{
	int ret = 0;
	if (gpio_is_exported(shadow_name) == false) {
		if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
			ret = gpio_export_by_name(GPIO_CHIP_ASPEED, pin_name, shadow_name);
		}
		else if (strncmp(chip_name, GPIO_CHIP_I2C_IO_EXP, GPIO_CHIP_MAX) == 0) {
			ret = gpio_export_by_offset(GPIO_CHIP_I2C_IO_EXP, offset, shadow_name);
		}
		else {
			printf("failed to recognize chip name: %s\n", chip_name);
			syslog(LOG_ERR, "failed to recognize chip name: %s\n", chip_name);
			return -1;
		}

		if (ret != 0) {
			printf("failed to export %s\n", shadow_name);
			syslog(LOG_ERR, "failed to export %s\n", shadow_name);
			return ret;
		}

		if (direction == GPIO_DIRECTION_OUT) {
			ret = gpio_set_init_value_by_shadow(shadow_name, value);

			if (ret != 0) {
				printf("failed to set initial value to %s\n", shadow_name);
				syslog(LOG_ERR, "failed to set initial value to %s\n", shadow_name);
				return ret;
			}
		}
	}

	return ret;
}

void setup_gpios_by_table(const char *chip_name, gpio_cfg *gpio_config) {
	int ret = 0, offset = 0;
	uint8_t bmc_location = 0;

	if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
		offset = MAX_GPIO_EXPANDER_GPIO_PINS; // BMC GPIO table index start after Expander GPIO table
	}
	if (fby35_common_get_bmc_location(&bmc_location) < 0) {
  	  syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
  	}

	while (gpio_config[offset].shadow_name != NULL) {
		if (((bmc_location == BB_BMC) && ((gpio_config[offset].sys_cfg & SYS_CFG_CLASS1) == 0)) ||
			((bmc_location == NIC_BMC) && ((gpio_config[offset].sys_cfg & SYS_CFG_CLASS2) == 0))) {
			// this pin not exist on corresponding config
			offset += 1;
  			continue;
  		}
		ret = setup_gpio_with_value(chip_name,
			gpio_config[offset].shadow_name,
			gpio_config[offset].pin_name,
			offset,
			gpio_config[offset].direction,
			gpio_config[offset].value);

		if (ret != 0) {
			printf("failed to setup %s\n", gpio_config[offset].shadow_name);
			syslog(LOG_ERR, "failed to setup %s\n", gpio_config[offset].shadow_name);
		}
		offset += 1;
	}
}

static int setup_board_id(gpio_cfg *gpio_config) {
	const char *shadows[] = {
		"BOARD_ID0",
		"BOARD_ID1",
		"BOARD_ID2",
		"BOARD_ID3",
	};
	char value[8] = {0};
	unsigned int brd_id = 0;
	int ret = -1;
	int i = 0, offset = 0;

	for (i = GPIO_BOARD_ID0; i <= GPIO_BOARD_ID3; i++) {
		offset = MAX_GPIO_EXPANDER_GPIO_PINS + i;
		ret = setup_gpio_with_value(GPIO_CHIP_ASPEED,
			gpio_config[offset].shadow_name,
			gpio_config[offset].pin_name,
			offset,
			gpio_config[offset].direction,
			gpio_config[offset].value);
		if (ret != 0) {
			return ret;
		}
	}
	if (!gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &brd_id)) {
		snprintf(value, sizeof(value), "%u", brd_id);
		kv_set("board_id", value, 0, 0);
		ret = 0;
	}

	return ret;
}

int
main(int argc, char **argv) {
	uint32_t reg_value = 0;

	printf("Set up BMC GPIO pins...\n");

	// Cache for BOARD_ID
	setup_board_id(bmc_gpio_table);

	setup_gpios_by_table(GPIO_CHIP_ASPEED, bmc_gpio_table);

	// # Disable GPION4-BMC_READY pin reset tolerant
	phymem_get_dword(GPIO_BASE, REG_GPIO0FC, &reg_value);
	reg_value = reg_value & 0xFFFFEFFF;
	phymem_set_dword(GPIO_BASE, REG_GPIO0FC, reg_value);

	/*
	Disable the below BMC GPIO internal pull-down: GPIOS2
	*/
	// GPIOS2-P5V_USB_PG_BMC
	phymem_get_dword(SCU_BASE, REG_SCU630, &reg_value);
	reg_value = reg_value | 0x00040000;
	phymem_set_dword(SCU_BASE, REG_SCU630, reg_value);


	printf("done.\n");

	return 0;
}
