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

#define SCU_BASE	0x1E6E2000
#define REG_SCU458	0x458

int
main(int argc, char **argv) {
	uint32_t reg_value = 0;

	// SALT to GPIO
	phymem_get_dword(SCU_BASE, REG_SCU458, &reg_value);
	reg_value = ((reg_value & 0xFFFFFFF3) | 0x4);
	phymem_set_dword(SCU_BASE, REG_SCU458, reg_value);

	printf("Modify NCSI strength done.\n");

	return 0;
}
