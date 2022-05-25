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
#include <openbmc/phymem.h>
#include <openbmc/libgpio.h>
#include <facebook/netlakemtp_gpio.h>

#define SCU_BASE	0x1E6E2000
#define REG_SCU41C	0x41C // Multi-function Pin Control #7
#define REG_SCU694	0x694 // Multi-function Pin Control #25

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

	if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
		offset = MAX_GPIO_EXPANDER_GPIO_PINS; // BMC GPIO table index start after Expander GPIO table
	}

	while (gpio_config[offset].shadow_name != NULL) {
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

int
main(int argc, char **argv) {
	uint32_t reg_value = 0;

	printf("Set up BMC GPIO pins...\n");
	setup_gpios_by_table(GPIO_CHIP_ASPEED, bmc_gpio_table);

	printf("Set up GPIO-expander GPIO pins...\n");
	setup_gpios_by_table(GPIO_CHIP_I2C_IO_EXP, gpio_expander_gpio_table);

	// SALT to GPIO
	phymem_get_dword(SCU_BASE, REG_SCU694, &reg_value);
	reg_value = reg_value & 0xFF0FFFFF;
	phymem_set_dword(SCU_BASE, REG_SCU694, reg_value);

	// NCTS1, NDCD1, NDSR1 and NRI1 to GPIO
	phymem_get_dword(SCU_BASE, REG_SCU41C, &reg_value);
	reg_value = reg_value & 0xFFFFFFC0;
	phymem_set_dword(SCU_BASE, REG_SCU41C, reg_value);

	printf("done.\n");

	return 0;
}
