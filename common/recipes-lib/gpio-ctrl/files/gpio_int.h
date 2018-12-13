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

/*
 * This file contains types/declarations used within the gpio library.
 * Library consumers should just include "gpio.h" instead of this header
 * file.
 */

#ifndef _OBMC_GPIO_INT_H_
#define _OBMC_GPIO_INT_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <linux/limits.h>

#include "libgpio.h"
#include <openbmc/misc-utils.h>

/*
 * Logging facilities.
 */
#ifdef OBMC_GPIO_DEBUG
#define GLOG_DEBUG(fmt, args...)	syslog(LOG_INFO, fmt, ##args)
#else
#define GLOG_DEBUG(fmt, args...)
#endif /* OBMC_GPIO_DEBUG */
#define GLOG_INFO(fmt, args...)		syslog(LOG_INFO, fmt, ##args)
#define GLOG_ERR(fmt, args...)		syslog(LOG_ERR, fmt, ##args)

/*
 * Types of gpio chips supported by this library.
 */
#define GPIO_CHIP_ASPEED_SOC	GPIO_CHIP_ASPEED
#define GPIO_CHIP_I2C_EXPANDER	"i2c-io-expander"

/*
 * "gpio_sysfs_attr" is needed when accessing gpio via sysfs interface.
 */
struct gpio_sysfs_attr {
	int edge_fd;
	int value_fd;
	int direction_fd;
};

/*
 * "gpio_chardev_attr" is needed when accessing gpio via chardev interface.
 */
struct gpio_chardev_attr {
};

/*
 * The opaque descriptor to access a gpio pin.
 */
struct gpio_desc {
	int pin_num;
	char shadow_path[PATH_MAX];

	union {
		struct gpio_sysfs_attr sysfs_attr;
		struct gpio_chardev_attr chardev_attr;
	} u;
};
#define IS_VALID_GPIO_DESC(d) ((d) != NULL && (d)->pin_num >= 0)

struct gpiochip_ops {
	int (*pin_name_to_offset)(gpiochip_desc_t *gcdesc,
				  const char *name);
	int (*pin_offset_to_name)(gpiochip_desc_t *gcdesc,
				  int offset, char *name, size_t size);
};

struct gpiochip_desc {
	int base;
	int ngpio;
	char dev_name[NAME_MAX];
	char chip_type[NAME_MAX];

	struct gpiochip_ops *ops;
};


struct gpiopoll_desc {
};

/*
 * There are 2 ways to control/access gpio from user space: sysfs or
 * chardev interfaces. sysfs and chardev are called "backend" in the
 * library, and all the backend-specific methods are contained in below
 * structure.
 */
struct gpio_backend_ops {
	/*
	 * Functions to export/unexport a gpio pin.
	 */
	int (*export_pin)(int pin_num, const char *shadow_path);
	int (*unexport_pin)(int pin_num, const char *shadow_path);

	/*
	 * Functions to open/close a gpio pin.
	 */
	int (*open_pin)(gpio_desc_t *gdesc);
	int (*close_pin)(gpio_desc_t *gdesc);

	/*
	 * Functions to get/set a gpio pin's value, direction and etc.
	 */
	int (*get_pin_value)(gpio_desc_t *gdesc, gpio_value_t *value);
	int (*set_pin_value)(gpio_desc_t *gdesc, gpio_value_t value);
	int (*get_pin_direction)(gpio_desc_t *gdesc, gpio_direction_t *dir);
	int (*set_pin_direction)(gpio_desc_t *gdesc, gpio_direction_t dir);
	int (*get_pin_edge)(gpio_desc_t *gdesc, gpio_edge_t *edge);
	int (*set_pin_edge)(gpio_desc_t *gdesc, gpio_edge_t edge);

	/*
	 * Function to enumerate gpio chips.
	 */
	int (*chip_enumerate)(gpiochip_desc_t *chips, size_t size);
};

/*
 * Global variables.
 */
extern struct gpiochip_ops aspeed_gpiochip_ops;
extern struct gpio_backend_ops gpio_sysfs_ops;

/*
 * Method to choose backend: the function always returns sysfs backend
 * ops for now, because chardev backend ops is not implemented yet.
 */
static inline struct gpio_backend_ops* gpio_choose_backend(void)
{
	return &gpio_sysfs_ops;
}
#define GPIO_OPS()	gpio_choose_backend()

#endif /* _OBMC_GPIO_INT_H_ */
