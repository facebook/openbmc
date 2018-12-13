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
#ifndef _OBMC_GPIO_H_
#define _OBMC_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Terms in the "gpio" library:
 *   - gpio chip:
 *     It refers to a gpio controller. Example sources would be soc gpio
 *     controllers, FPGAs, multifunction chips, dedicated gpio expanders,
 *     and so on.
 *
 *   - gpio pin:
 *     It refers to a gpio pin/line/signal on a gpio chip. A pin may be
 *     specified by:
 *       1) shadow (name of the symlink under /tmp/gpionames).
 *       2) (chip, name) pair.
 *          This approach is normally used to identify a pin on aspeed
 *          gpio controller. For example, (GPIO_CHIP_AST2500, "GPIOA0").
 *       3) (chip, offset) pair.
 *          This approach is normally used to specify a pin on i2c i/o
 *          expander. For example, ("12-0035", 3).
 *
 *   - shadow:
 *     It refers to the symbolic link under "/tmp/gpionames/".
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <linux/limits.h>

/*
 * Constants.
 */
#define GPIO_SHADOW_ROOT	"/tmp/gpionames"
#define GPIO_CHIP_ASPEED	"aspeed-gpio"
#define GPIO_CHIP_MAX		64

/*
 * Type declarations.
 */
typedef enum {
	GPIO_VALUE_INVALID = -1,
	GPIO_VALUE_LOW = 0,
	GPIO_VALUE_HIGH = 1,
	GPIO_VALUE_MAX,
} gpio_value_t;
#define IS_VALID_GPIO_VALUE(v)	((v) == GPIO_VALUE_LOW || \
				 (v) == GPIO_VALUE_HIGH)

typedef enum {
	GPIO_DIRECTION_INVALID = -1,
	GPIO_DIRECTION_IN,
	GPIO_DIRECTION_OUT,
} gpio_direction_t;
#define IS_VALID_GPIO_DIRECTION(d)	((d) == GPIO_DIRECTION_IN || \
					 (d) == GPIO_DIRECTION_OUT)

typedef enum {
	GPIO_EDGE_INVALID = -1,
	GPIO_EDGE_NONE,
	GPIO_EDGE_RISING,
	GPIO_EDGE_FALLING,
	GPIO_EDGE_BOTH,
} gpio_edge_t;
#define IS_VALID_GPIO_EDGE(e)	((e) >= GPIO_EDGE_NONE && \
				 (e) <= GPIO_EDGE_BOTH)

typedef struct gpio_desc gpio_desc_t;
typedef struct gpiochip_desc gpiochip_desc_t;
typedef struct gpiopoll_desc gpiopoll_desc_t;

struct gpiopoll_config {
	char shadow[NAME_MAX];
	gpio_edge_t edge;
	void (*handler)(gpiopoll_desc_t *gpdesc);
};

/*
 * Functions to export control of a gpio pin to userspace.
 * Normally, (chip, name) pair is used to identify a pin on aspeed gpio
 * controller, while (chip, offset) is used to specify a pin on i2c i/o
 * expander.
 * Note: "shadow" is mandatory required so the pin can be referenced by
 * the shadow (/tmp/gpionames/<shadow>) after it's exported.
 * 
 * Return:
 *   Both functions return 0 for success, and -1 on failures.
 */
int gpio_export_by_name(const char *chip, const char *name,
			const char *shadow);
int gpio_export_by_offset(const char *chip, int offset,
			  const char *shadow);

/*
 * Function to reverse the effect of gpio_export_*.
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int gpio_unexport(const char *shadow);

/*
 * Functions to open a gpio pin.
 * Although a gpio pin can be specified by global pin number, we don't
 * provide "gpio_open_by_num" interface because pin number assigned to
 * a specific gpio pin may change cross different kernel releases (or
 * even different reboots).
 *
 * Return:
 *   Pointer to the opaque gpio descriptor (gpio_desc_t) which is used
 *   by all "gpio_set|get_" functions, or NULL on failures.
 */
gpio_desc_t* gpio_open_by_shadow(const char *shadow);
gpio_desc_t* gpio_open_by_name(const char *chip, const char *name);
gpio_desc_t* gpio_open_by_offset(const char *chip, int offset);

/*
 * Function to release resources allocated by "gpio_open_*" functions.
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_close(gpio_desc_t *gdesc);

/*
 * Functions to get and set value/direction/edge of a gpio pin.
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_get_value(gpio_desc_t *gdesc, gpio_value_t *value);
int gpio_set_value(gpio_desc_t *gdesc, gpio_value_t value);
int gpio_get_direction(gpio_desc_t *gdesc, gpio_direction_t *dir);
int gpio_set_direction(gpio_desc_t *gdesc, gpio_direction_t dir);
int gpio_get_edge(gpio_desc_t *gdesc, gpio_edge_t *edge);
int gpio_set_edge(gpio_desc_t *gdesc, gpio_edge_t edge);

/*
 * Function to setup gpio pins for gpio_poll() call.
 *
 * Return:
 *   Pointer to the opaque gpio poll descriptor which is used by
 *   "gpio_poll" function.
 */
gpiopoll_desc_t* gpio_poll_open(struct gpiopoll_config *config,
				size_t num_config);

/*
 * Function to release resources allocated by gpio_poll_open().
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_poll_close(gpiopoll_desc_t *gpdesc);

/*
 * Function to poll on a set of gpio pins: the registered handlers will
 * be called when pin state is changed.
 *
 * Return:
 *   0 for success, and -1 on failures.
 */
int gpio_poll(gpiopoll_desc_t *gpdesc, int timeout);

/*
 * gpio chip related functions.
 */
int gpiochip_list(gpiochip_desc_t **chips, size_t size);
gpiochip_desc_t* gpiochip_lookup(const char *chip);
int gpiochip_get_base(gpiochip_desc_t *gcdesc);
int gpiochip_get_ngpio(gpiochip_desc_t *gcdesc);
char* gpiochip_get_type(gpiochip_desc_t *gcdesc,
			char *type, size_t size);
char* gpiochip_get_device(gpiochip_desc_t *gcdesc,
			  char *dev_name, size_t size);
int gpiochip_pin_name_to_offset(gpiochip_desc_t *gcdesc,
				const char *pin_name);

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* _OBMC_GPIO_H_ */
