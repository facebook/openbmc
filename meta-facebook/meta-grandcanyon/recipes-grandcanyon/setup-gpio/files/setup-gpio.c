/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <openbmc/libgpio.h>
#include <openbmc/phymem.h>

#define SCU_BASE    	0x1E6E2000
#define REG_SCU410      0x410
#define REG_SCU4B0      0x4B0


// Registers to set
enum {
	SCU410 = 0,
	SCU4B0,
	LASTEST_REG,
};

int set_gpio_init_value_after_export(const char * name, const char *shadow, gpio_value_t value)
{
	int ret;
	if (gpio_is_exported(shadow) == false) {
		gpio_export_by_name(GPIO_CHIP_ASPEED, name, shadow);
	}
	ret = gpio_set_init_value_by_shadow(shadow, value);
	return ret;
}

/* ToDo:
 * Initialize GPIO after schematic frozen
 */
int
main(int argc, char **argv) {
	uint32_t reg[LASTEST_REG];
	gpio_desc_t *desc = NULL;

	printf("Set up GPIO pins.....\n");


	// AST2600 test - start
	// Read register initial value
	phymem_get_dword(SCU_BASE, REG_SCU410, &reg[SCU410]);
	phymem_get_dword(SCU_BASE, REG_SCU4B0, &reg[SCU4B0]);

	// To use GPIOA0, SCU410[0] and SCU4B0[0] must be 0
	reg[SCU410] &= ~(0x00000001);
	reg[SCU4B0] &= ~(0x00000001);
	phymem_set_dword(SCU_BASE, REG_SCU410, reg[SCU410]);
	phymem_set_dword(SCU_BASE, REG_SCU4B0, reg[SCU4B0]);

	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOA0", "AST2600_GPIOA0");
	// AST2600 test - end

	return 0;
}
