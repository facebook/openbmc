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
#define GPIO_CHIP_I2C_IO_EXP	"i2c-io-expander"
#define GPIO_CHIP_MAX		64

/*
 * Type declarations.
 */
#define GPIO_VALUE_TYPES					\
	GPIO_DEF(GPIO_VALUE_LOW, "low"),			\
	GPIO_DEF(GPIO_VALUE_HIGH, "high")

#define GPIO_DIRECTION_TYPES					\
	GPIO_DEF(GPIO_DIRECTION_IN, "in"),			\
	GPIO_DEF(GPIO_DIRECTION_OUT, "out")

#define GPIO_EDGE_TYPES						\
	GPIO_DEF(GPIO_EDGE_NONE, "none"),			\
	GPIO_DEF(GPIO_EDGE_RISING, "rising"),			\
	GPIO_DEF(GPIO_EDGE_FALLING, "falling"),			\
	GPIO_DEF(GPIO_EDGE_BOTH, "both")

#define GPIO_DEF(type, str)			type
typedef enum {
	GPIO_VALUE_INVALID = -1,
	GPIO_VALUE_TYPES,
	GPIO_VALUE_MAX,
} gpio_value_t;

typedef enum {
	GPIO_DIRECTION_INVALID = -1,
	GPIO_DIRECTION_TYPES,
	GPIO_DIRECTION_MAX,
} gpio_direction_t;

typedef enum {
	GPIO_EDGE_INVALID = -1,
	GPIO_EDGE_TYPES,
	GPIO_EDGE_MAX,
} gpio_edge_t;
#undef GPIO_DEF

#define IS_VALID_GPIO_VALUE(v)		((v) > GPIO_VALUE_INVALID && \
					 (v) < GPIO_VALUE_MAX)
#define IS_VALID_GPIO_DIRECTION(d)	((d) > GPIO_DIRECTION_INVALID && \
					 (d) < GPIO_DIRECTION_MAX)
#define IS_VALID_GPIO_EDGE(e)		((e) > GPIO_EDGE_INVALID && \
					 (e) < GPIO_EDGE_MAX)

typedef struct gpio_desc gpio_desc_t;
typedef struct gpiochip_desc gpiochip_desc_t;
typedef struct gpiopoll_pin_desc gpiopoll_pin_t;
typedef struct gpiopoll_desc gpiopoll_desc_t;

struct gpiopoll_config {
	/* Name of the GPIO shadow */
	char shadow[NAME_MAX];

	/* Short description. Useful only for debugging */
	char description[NAME_MAX];

	/* Edge of GPIO pin to listen on */
	gpio_edge_t edge;

	/* Called everytime the specified edge transition occurs */
	void (*handler)(gpiopoll_pin_t *gpdesc, gpio_value_t last_value, gpio_value_t curr_value);

	/* (optional) Called once during creation. This allows the user
	 * to set the state machine at the initial value of the given GPIO */
	void (*init_value)(gpiopoll_pin_t *gpdesc, gpio_value_t value);
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
 * Function to check given gpio is exported or not.
 *
 * Return:
 *   true: exported, false: not exported.
 */
bool gpio_is_exported(const char * shadow);



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

/* Get GPIO value given the shadow name of the gpio-pin.
 *
 * Return:
 * GPIO_VALUE_INVALID on failure,
 * GPIO_VALUE_LOW/GPIO_VALUE_HIGH on success
 */
gpio_value_t gpio_get_value_by_shadow(const char *shadow);

/* Set GPIO value given the shadow name of the gpio-pin.
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_set_value_by_shadow(const char *shadow, gpio_value_t);

/* Set GPIO init value given the shadow name of the gpio-pin.
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_set_init_value_by_shadow(const char *shadow, gpio_value_t);

/* Get value of an array of GPIOs given their shadow names
 * as a bit-mask of their values
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_get_value_by_shadow_list(const char * const *shadows, size_t num, unsigned int *mask);


/* Set value provided as a bitmask of individual GPIO values of an array
 * of GPIOs given their shadow names.
 *
 * Return:
 * 0 for success, or -1 on failures.
 */
int gpio_set_value_by_shadow_list(const char * const *shadows, size_t num, unsigned int mask);


/*
 * Sets the gpio pin to output with given initial value atomically.
 *
 * Return:
 *   0 for success, or -1 on failures.
 */
int gpio_set_init_value(gpio_desc_t *gdesc, gpio_value_t value);

/*
 * Functions to map between gpio value/direction/edge types and strings.
 */
const char* gpio_value_type_to_str(gpio_value_t val);
gpio_value_t gpio_value_str_to_type(const char *val_str);
const char* gpio_direction_type_to_str(gpio_direction_t dir);
gpio_direction_t gpio_direction_str_to_type(const char *dir_str);
const char* gpio_edge_type_to_str(gpio_edge_t edge);
gpio_edge_t gpio_edge_str_to_type(const char *edge_str);

/*
 * Function to setup gpio pins for gpio_poll() call.
 *
 * Return:
 *   Pointer to the array of opaque gpio poll descriptor which is used by
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
 * Function to retrieve the configuration of the GPIO pin described
 * by the poll descriptor. Typical use would be to call from the
 * handler.
 *
 * Return:
 *   pointer to the config structure on success, NULL on failure.
 * Note:
 *   Caller must make no assumption on the pointer lifetime/scope.
 */
const struct gpiopoll_config *gpio_poll_get_config(gpiopoll_pin_t *gpdesc);

/* 
 * Function to retrieve the GPIO descriptor of the GPIO poin.
 *
 * Return:
 *   pointer to the gpio descriptor on success. NULL on failure.
 * Note:
 *   Returned descriptor is valid only for the duration of gpdesc
 *   which may go out of scope after the handler returns.
 */
gpio_desc_t *gpio_poll_get_descriptor(gpiopoll_pin_t *gpdesc);

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
