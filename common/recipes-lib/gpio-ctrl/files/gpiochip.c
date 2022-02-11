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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gpio_int.h"

static int num_chips = -1;
static gpiochip_desc_t chip_descs[GPIO_CHIP_MAX];

static int gpiochip_enumerate(void)
{
	GLOG_DEBUG("Enumerate gpio chips\n");
	num_chips = GPIO_OPS()->chip_enumerate(chip_descs,
					       ARRAY_SIZE(chip_descs));
	return num_chips;
}

static bool gpiochip_match(gpiochip_desc_t *gcdesc, const char *chip)
{
	if (strcmp(gcdesc->dev_name, chip) == 0 ||
	    strcmp(gcdesc->chip_type, chip) == 0)
		return true;

	return false;
}

/*
 * List all the gpio chips.
 *
 * Return:
 *   Number of chips, or -1 if error happens. If the return value is
 *   greater than <size>, means <chips> array is not big enough to store
 *   all the chips.
 */
int gpiochip_list(gpiochip_desc_t **chips, size_t size)
{
	int i;

	if (chips == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (num_chips < 0 && gpiochip_enumerate() < 0)
		return -1;

	if (num_chips < size)
		size = num_chips;
	for (i = 0; i < size; i++)
		chips[i] = &chip_descs[i];

	return num_chips;
}

/*
 * Look up gpio chip based on chip type or device name.
 *
 * Return:
 *   The pointer to opaque gpiochip_desc, or NULL on failures.
 */
gpiochip_desc_t* gpiochip_lookup(const char *chip)
{
	int i;

	if (num_chips < 0 && gpiochip_enumerate() < 0)
		return NULL;

	for (i = 0; i < num_chips; i++) {
		if (gpiochip_match(&chip_descs[i], chip))
			return &chip_descs[i];
	}

	return NULL;
}

/*
 * Get the base pin number of given gpio chip.
 *
 * Return:
 *   Base pin number for success, or -1 on failures.
 */
int gpiochip_get_base(gpiochip_desc_t *gcdesc)
{
	if (gcdesc == NULL) {
		errno = EINVAL;
		return -1;
	}

	return gcdesc->base;
}

/*
 * Get the number of gpio pins managed by the given gpio chip.
 *
 * Return:
 *   Number of gpio pins for success, or -1 on failures.
 */
int gpiochip_get_ngpio(gpiochip_desc_t *gcdesc)
{
	if (gcdesc == NULL) {
		errno = EINVAL;
		return -1;
	}

	return gcdesc->ngpio;
}

/*
 * Get chip type.
 *
 * Return:
 *   Chip type string for success, or -1 on failures.
 */
char* gpiochip_get_type(gpiochip_desc_t *gcdesc, char *type, size_t size)
{
	if (gcdesc == NULL || type == NULL) {
		errno = EINVAL;
		return NULL;
	}

	strncpy(type, gcdesc->chip_type, size);
	return type;
}

/*
 * Get chip device name.
 *
 * Return:
 *   Chip device name for success, or -1 on failures.
 */
char* gpiochip_get_device(gpiochip_desc_t *gcdesc,
			  char *dev_name,
			  size_t size)
{
	if (gcdesc == NULL || dev_name == NULL) {
		errno = EINVAL;
		return NULL;
	}

	strncpy(dev_name, gcdesc->dev_name, size);
	return dev_name;
}

/*
 * Translate pin name to the offset within the chip's namespace.
 *
 * Return:
 *   Offset for success, or -1 on failures.
 */
int gpiochip_pin_name_to_offset(gpiochip_desc_t *gcdesc,
				const char *pin_name)
{
	if (gcdesc == NULL || pin_name == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (gcdesc->ops == NULL ||
	    gcdesc->ops->pin_name_to_offset == NULL) {
		errno = ENOTSUP;
		return -1;
	}

	return gcdesc->ops->pin_name_to_offset(gcdesc, pin_name);
}
