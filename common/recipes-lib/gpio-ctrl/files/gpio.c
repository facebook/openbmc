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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "gpio_int.h"


/*
 * Internal helper functions.
 */
static int create_shadow_dir(void)
{
	if (!path_isdir(GPIO_SHADOW_ROOT)) {
		GLOG_DEBUG("create <%s> direction", GPIO_SHADOW_ROOT);
		if (mkdir(GPIO_SHADOW_ROOT, 0755) != 0) {
			GLOG_ERR("failed to create <%s>: %s\n",
				 GPIO_SHADOW_ROOT, strerror(errno));
			return -1;
		}
	}

	return 0;
}

static char* gpio_shadow_abspath(char *buf,
				 size_t size,
				 const char* shadow)
{
	return path_join(buf, size, GPIO_SHADOW_ROOT, shadow, NULL);
}

static int gpio_shadow_to_num(const char *shadow_path)
{
	int len, pin_num;
	char *base_name;
	char target_path[PATH_MAX];

	len = readlink(shadow_path, target_path, sizeof(target_path));
	if (len < 0) {
		GLOG_ERR("failed to read target from symlink <%s>: %s\n",
			 shadow_path, strerror(errno));
		return -1;
	} else if (len >= sizeof(target_path)) {
		GLOG_ERR("target path of symlink <%s> is truncated\n",
			 shadow_path);
		return -1;
	}
	target_path[len] = '\0';

	base_name = basename(target_path);
	if (!str_startswith(base_name, "gpio")) {
		GLOG_ERR("unable to extract gpio number from path <%s>",
			 target_path);
		return -1;
	}

	pin_num = (int)strtol(&base_name[strlen("gpio")], NULL, 10);
	GLOG_DEBUG("shadow <%s> is mapped to gpio pin %d\n",
		   shadow_path, pin_num);
	return pin_num;
}

static int gpio_name_to_num(const char *chip, const char *pin_name)
{
	int base, offset, pin_num;
	gpiochip_desc_t *gcdesc;

	gcdesc = gpiochip_lookup(chip);
	if (gcdesc == NULL) {
		GLOG_ERR("failed to find gpio chip <%s>\n", chip);
		return -1;
	}

	offset = gpiochip_pin_name_to_offset(gcdesc, pin_name);
	if (offset < 0)
		return -1;

	base = gpiochip_get_base(gcdesc);
	pin_num = base + offset;
	GLOG_DEBUG("(%s, %s) is mapped to gpio pin %d (%d + %d)\n",
		   chip, pin_name, pin_num, base, offset);
	return pin_num;
}

static int gpio_offset_to_num(const char *chip, int offset)
{
	int base, pin_num;
	gpiochip_desc_t *gcdesc;

	gcdesc = gpiochip_lookup(chip);
	if (gcdesc == NULL) {
		GLOG_ERR("failed to find gpio chip <%s>\n", chip);
		return -1;
	}

	base = gpiochip_get_base(gcdesc);
	pin_num = base + offset;
	GLOG_DEBUG("(%s, %d) is mapped to gpio pin %d (%d + %d)\n",
		   chip, offset, pin_num, base, offset);
	return pin_num;
}

static gpio_desc_t* gpio_desc_alloc(int pin_num, const char *shadow_path)
{
	gpio_desc_t *gdesc;

	gdesc = malloc(sizeof(*gdesc));
	if (gdesc == NULL) {
		return NULL;
	}

	memset(gdesc, 0, sizeof(*gdesc));
	gdesc->pin_num = pin_num;
	if (shadow_path != NULL)
		strncpy(gdesc->shadow_path, shadow_path,
			sizeof(gdesc->shadow_path));
	return gdesc;
}

static void gpio_desc_free(gpio_desc_t *gdesc)
{
	if (gdesc != NULL)
		free(gdesc);
}

/*
 * Public functions to export/unexport control of gpio pins to userspace.
 */
int gpio_export_by_name(const char *chip,
			const char *name,
			const char *shadow)
{
	int pin_num;
	char shadow_path[PATH_MAX];

	if (chip == NULL || name == NULL || shadow == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (create_shadow_dir() != 0) {
		return -1;
	}
	gpio_shadow_abspath(shadow_path, sizeof(shadow_path), shadow);
	if (path_exists(shadow_path)) {
		errno = EEXIST;
		return -1;
	}

	pin_num = gpio_name_to_num(chip, name);
	if (pin_num < 0) {
		errno = ENXIO;
		return -1;
	}

	return GPIO_OPS()->export_pin(pin_num, shadow_path);
}

int gpio_export_by_offset(const char *chip,
			  int offset,
			  const char *shadow)
{
	int pin_num;
	char shadow_path[PATH_MAX];

	if (chip == NULL || offset < 0 || shadow == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (create_shadow_dir() != 0) {
		return -1;
	}
	gpio_shadow_abspath(shadow_path, sizeof(shadow_path), shadow);
	if (path_exists(shadow_path)) {
		errno = EEXIST;
		return -1;
	}

	pin_num = gpio_offset_to_num(chip, offset);
	if (pin_num < 0) {
		errno = ENXIO;
		return -1;
	}

	return GPIO_OPS()->export_pin(pin_num, shadow_path);
}

int gpio_unexport(const char *shadow)
{
	int pin_num;
	char shadow_path[PATH_MAX];

	if (shadow == NULL) {
		errno = EINVAL;
		return -1;
	}

	gpio_shadow_abspath(shadow_path, sizeof(shadow_path), shadow);
	if (!path_islink(shadow_path)) {
		errno = ENOENT;
		return -1;
	}

	pin_num = gpio_shadow_to_num(shadow_path);
	if (pin_num < 0) {
		errno = ENXIO;
		return -1;
	}

	return GPIO_OPS()->unexport_pin(pin_num, shadow_path);
}

/*
 * Public functions to open/close a specific gpio pin.
 */
gpio_desc_t* gpio_open_by_shadow(const char *shadow)
{
	int pin_num;
	gpio_desc_t *gdesc;
	char shadow_path[PATH_MAX];

	gpio_shadow_abspath(shadow_path, sizeof(shadow_path), shadow);
	if (!path_islink(shadow_path)) {
		errno = ENOENT;
		return NULL;
	}

	pin_num = gpio_shadow_to_num(shadow_path);
	if (pin_num < 0) {
		errno = ENXIO;
		return NULL;
	}

	gdesc = gpio_desc_alloc(pin_num, shadow_path);
	if (gdesc == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	if (GPIO_OPS()->open_pin(gdesc) != 0) {
		gpio_desc_free(gdesc);
		return NULL;
	}

	return gdesc;
}

gpio_desc_t* gpio_open_by_name(const char *chip, const char *name)
{
	int pin_num;
	gpio_desc_t *gdesc;

	if (chip == NULL || name == NULL) {
		errno = EINVAL;
		return NULL;
	}

	pin_num = gpio_name_to_num(chip, name);
	if (pin_num < 0) {
		errno = ENXIO;
		return NULL;
	}

	gdesc = gpio_desc_alloc(pin_num, NULL);
	if (gdesc == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	if (GPIO_OPS()->open_pin(gdesc) != 0) {
		gpio_desc_free(gdesc);
		return NULL;
	}

	return gdesc;
}

gpio_desc_t* gpio_open_by_offset(const char *chip, int offset)
{
	int pin_num;
	gpio_desc_t *gdesc;

	if (chip == NULL || offset < 0) {
		errno = EINVAL;
		return NULL;
	}

	pin_num = gpio_offset_to_num(chip, offset);
	if (pin_num < 0) {
		errno = ENXIO;
		return NULL;
	}

	gdesc = gpio_desc_alloc(pin_num, NULL);
	if (gdesc == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	if (GPIO_OPS()->open_pin(gdesc) != 0) {
		gpio_desc_free(gdesc);
		return NULL;
	}

	return gdesc;
}

int gpio_close(gpio_desc_t *gdesc)
{
	if (gdesc != NULL) {
		GPIO_OPS()->close_pin(gdesc);
		gpio_desc_free(gdesc);
	}

	return 0;
}

/*
 * Public functions to get/set a gpio pin's attributes, such as value,
 * direction and etc.
 */
int gpio_get_value(gpio_desc_t *gdesc, gpio_value_t *value)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || value == NULL) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->get_pin_value(gdesc, value);
}

int gpio_set_value(gpio_desc_t *gdesc, gpio_value_t value)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || !IS_VALID_GPIO_VALUE(value)) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->set_pin_value(gdesc, value);
}

int gpio_get_direction(gpio_desc_t *gdesc, gpio_direction_t *dir)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || dir == NULL) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->get_pin_direction(gdesc, dir);
}

int gpio_set_direction(gpio_desc_t *gdesc, gpio_direction_t dir)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || !IS_VALID_GPIO_DIRECTION(dir)) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->set_pin_direction(gdesc, dir);
}

int gpio_get_edge(gpio_desc_t *gdesc, gpio_edge_t *edge)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || edge == NULL) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->get_pin_edge(gdesc, edge);
}

int gpio_set_edge(gpio_desc_t *gdesc, gpio_edge_t edge)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || !IS_VALID_GPIO_EDGE(edge)) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->set_pin_edge(gdesc, edge);
}

/*
 * FIXME function not implemented.
 */
gpiopoll_desc_t* gpio_poll_open(struct gpiopoll_config *config,
				size_t num_config)
{
	errno = ENOTSUP;
	return NULL;
}

/*
 * FIXME function not implemented.
 */
int gpio_poll_close(gpiopoll_desc_t *gpdesc)
{
	errno = ENOTSUP;
	return -1;
}

/*
 * FIXME function not implemented.
 */
int gpio_poll(gpiopoll_desc_t *gpdesc, int timeout)
{
	errno = ENOTSUP;
	return -1;
}
