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
#include <assert.h>
#include <ctype.h>

#include "gpio_int.h"

#define BASE_ALPHABET		('Z' - 'A' + 1)
#define ALPHABET_DIGIT(c)	((c) - 'A' + 1)

/*
 * Valid aspeed gpio pin name format "GPIO<group><offset>". The <group>
 * is expressed by base 26 alphabet numbers (such as "F", "AC"), and
 * <offset> is 1-byte digit between '0' and '7'.
 * Example aspeed gpio names: "GPIOF3", "GPIOAC5".
 */
#define AST_GPIO_NAME_PREFIX	"GPIO"
#define AST_GPIO_GROUP_SIZE	8	/* # of bits in a gpio group */

static int ast_gpio_name_to_offset(gpiochip_desc_t *gcdesc,
				   const char *name)
{
	const char *ptr;
	int group, offset, max_groups;

	assert(gcdesc != NULL);
	assert(name != NULL);

	if (!str_startswith(name, AST_GPIO_NAME_PREFIX)) {
		GLOG_ERR("unable to parse gpio name <%s>: %s\n",
			 name, "invalid prefix");
		return -1;
	}

	/* parse gpio group field */
	ptr = name + strlen(AST_GPIO_NAME_PREFIX);
	if (!isupper(*ptr)) {
		GLOG_ERR("unable to parse gpio name <%s>: %s\n",
			 name, "gpio group is missing");
		return -1;
	}

	max_groups = (gcdesc->ngpio + AST_GPIO_GROUP_SIZE - 1) /
		      AST_GPIO_GROUP_SIZE;
	for (group = 0; isupper(*ptr); ptr++) {
		group = group * BASE_ALPHABET + ALPHABET_DIGIT(*ptr);
		if ((group - 1) >= max_groups) {
			GLOG_ERR("unable to parse gpio name <%s>: %s\n",
				 name, "gpio group exceeds limit");
			return -1;
		}
	}

	/*
	 * 'A' is parsed as 1 in 26-based number convertion, but its
	 * offset is 0. That's why "group--" is needed.
	 */
	group--;

	/* parse group offset field */
	if (strlen(ptr) != 1 || *ptr < '0' || *ptr > '7') {
		GLOG_ERR("unable to parse gpio name <%s>: %s\n",
			 name, "group offset is missing");
		return -1;
	}

	offset = (group * AST_GPIO_GROUP_SIZE) + (*ptr - '0');
	if (offset >= gcdesc->ngpio) {
		GLOG_ERR("unable to parse gpio name <%s>: %s\n",
			 name, "pin offset exceeds limit");
		return -1;
	}

	GLOG_DEBUG("aspeed gpio pin %s: offset=%d\n", name, offset);
	return offset;
}

struct gpiochip_ops aspeed_gpiochip_ops = {
	.pin_name_to_offset = ast_gpio_name_to_offset,
};
