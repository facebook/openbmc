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
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "gpio_int.h"

#define GPIO_SHADOW_PATH_MAX 128

/*
 * Global variables.
 */
#define GPIO_DEF(type, str)		[type] = str
static const char* g_value_str[GPIO_VALUE_MAX] = {
	GPIO_VALUE_TYPES
};

static const char* g_direction_str[GPIO_DIRECTION_MAX] = {
	GPIO_DIRECTION_TYPES,
};

static const char* g_edge_str[GPIO_EDGE_MAX] = {
	GPIO_EDGE_TYPES,
};
#undef GPIO_DEF

const char* gpio_value_type_to_str(gpio_value_t val)
{
	if (IS_VALID_GPIO_VALUE(val))
		return g_value_str[val];

	return "";
}

gpio_value_t gpio_value_str_to_type(const char *val_str)
{
	int i;

	if (val_str == NULL)
		return GPIO_VALUE_INVALID;

	for (i = 0; i < ARRAY_SIZE(g_value_str); i++)
		if (strcmp(val_str, g_value_str[i]) == 0)
			return i;

	return GPIO_VALUE_INVALID;
}

const char* gpio_direction_type_to_str(gpio_direction_t dir)
{
	if (IS_VALID_GPIO_DIRECTION(dir))
		return g_direction_str[dir];

	return "";
}

gpio_direction_t gpio_direction_str_to_type(const char *dir_str)
{
	int i;

	if (dir_str == NULL)
		return GPIO_DIRECTION_INVALID;

	for (i = 0; i < ARRAY_SIZE(g_direction_str); i++)
		if (strcmp(dir_str, g_direction_str[i]) == 0)
			return i;

	return GPIO_DIRECTION_INVALID;
}

const char* gpio_edge_type_to_str(gpio_edge_t edge)
{
	if (IS_VALID_GPIO_EDGE(edge))
		return g_edge_str[edge];

	return "";
}

gpio_edge_t gpio_edge_str_to_type(const char *edge_str)
{
	int i;

	if (edge_str == NULL)
		return GPIO_EDGE_INVALID;

	for (i = 0; i < ARRAY_SIZE(g_edge_str); i++)
		if (strcmp(edge_str, g_edge_str[i]) == 0)
			return i;

	return GPIO_EDGE_INVALID;
}

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
	char target_path[GPIO_SHADOW_PATH_MAX];

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
	if (shadow_path != NULL) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
		strncpy(gdesc->shadow_path, shadow_path,
			sizeof(gdesc->shadow_path) - 1);
#pragma GCC diagnostic pop
	}
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
	char shadow_path[GPIO_SHADOW_PATH_MAX];

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
	char shadow_path[GPIO_SHADOW_PATH_MAX];

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
	char shadow_path[GPIO_SHADOW_PATH_MAX];

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

bool gpio_is_exported(const char * shadow)
{
	char path[64] = {0};

	snprintf(path, sizeof(path), "%s/%s", GPIO_SHADOW_ROOT, shadow);
	return (access(path, F_OK) == 0);
}

/*
 * Public functions to open/close a specific gpio pin.
 */
gpio_desc_t* gpio_open_by_shadow(const char *shadow)
{
	int pin_num;
	gpio_desc_t *gdesc;
	char shadow_path[GPIO_SHADOW_PATH_MAX];

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

int gpio_set_init_value(gpio_desc_t *gdesc, gpio_value_t value)
{
	if (!IS_VALID_GPIO_DESC(gdesc) || !IS_VALID_GPIO_VALUE(value)) {
		errno = EINVAL;
		return -1;
	}

	return GPIO_OPS()->set_pin_init_value(gdesc, value);
}

gpio_value_t gpio_get_value_by_shadow(const char *shadow)
{
  gpio_value_t value = GPIO_VALUE_INVALID;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    return GPIO_VALUE_INVALID;
  }
  if (gpio_get_value(desc, &value)) {
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);
  return value;
}

int gpio_set_value_by_shadow(const char *shadow, gpio_value_t value)
{
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    return -1;
  }
  if (gpio_set_value(desc, value)) {
    return -1;
  }
  gpio_close(desc);
  return 0;
}

int gpio_set_init_value_by_shadow(const char *shadow, gpio_value_t value)
{
	gpio_desc_t *desc = gpio_open_by_shadow(shadow);
	if (!desc) {
		return -1;
	}
	if (gpio_set_init_value(desc, value)) {
		gpio_close(desc);
		return -1;
	}
	gpio_close(desc);
	return 0;
}

gpiopoll_desc_t* gpio_poll_open(struct gpiopoll_config *config,
				size_t num_config)
{
	int i;
	gpiopoll_desc_t *ret;

	ret = calloc(1, sizeof(gpiopoll_desc_t));
	if (!ret) {
		return NULL;
	}
	ret->num_pins = num_config;
	ret->pins = calloc(num_config, sizeof(ret->pins[0]));
	if (!ret->pins) {
		goto err_pins_alloc_bail;
	}
	for (i = 0; i < num_config; i++) {
		gpiopoll_pin_t *desc = &ret->pins[i];
		desc->handler_started = false;
		desc->cfg = config[i];
		if (config[i].handler == NULL || config[i].shadow[0] == '\0') {
			GLOG_ERR("Incorrect configuration at index: %d\n", i);
			goto err_bail;
		}
		desc->gpio = gpio_open_by_shadow(desc->cfg.shadow);
		if (!desc->gpio) {
			GLOG_ERR("Failed to open GPIO by shadow: %s <%s>\n",
				 desc->cfg.shadow, strerror(errno));
			goto err_bail;
		}
		if (gpio_set_direction(desc->gpio, GPIO_DIRECTION_IN)) {
			GLOG_ERR("Failed to set direction of GPIO: %s <%s>\n",
				 desc->cfg.shadow, strerror(errno));
			goto err_bail;
		}
		if (gpio_set_edge(desc->gpio, desc->cfg.edge)) {
			GLOG_ERR("Failed to set edge on GPIO: %s <%s>\n",
				 desc->cfg.shadow, strerror(errno));
			goto err_bail;
		}
		if (gpio_get_value(desc->gpio, &desc->last_value)) {
			GLOG_ERR("Failed to get value of GPIO: %s <%s>\n",
				 desc->cfg.shadow, strerror(errno));
			goto err_bail;
		}
		desc->curr_value = desc->last_value;
		if (desc->cfg.init_value) {
			desc->cfg.init_value(desc, desc->curr_value);
		}
	}
	return ret;
err_bail:
	for (i = 0; i < num_config; i++) {
		gpiopoll_pin_t *desc = &ret->pins[i];
		if (desc->gpio)
			gpio_close(desc->gpio);
	}
	free(ret->pins);
err_pins_alloc_bail:
	free(ret);
	return NULL;
}

int gpio_poll_close(gpiopoll_desc_t *gpdesc)
{
	int i;

	if (!gpdesc || !gpdesc->pins) {
		return -1;
	}

	for (i = 0; i < gpdesc->num_pins; i++) {
		gpiopoll_pin_t *desc = &gpdesc->pins[i];
		if (desc->handler_started) {
			int rc = pthread_cancel(desc->tid);
			if(rc != 0) {
				GLOG_ERR("Kill of thread failed for GPIO: %s <%s>\n",
					 desc->cfg.shadow, strerror(rc));
			}
			desc->handler_started = false;
		}
		if (gpio_close(desc->gpio)) {
			GLOG_ERR("Close failed for GPIO: %s <%s>\n",
				 desc->cfg.shadow, strerror(errno));
		}
	}
	free(gpdesc->pins);
	free(gpdesc);
	return 0;
}

static void *gpio_poll_handler(void *priv)
{
	gpiopoll_pin_t *desc = (gpiopoll_pin_t *)priv;
	int rc;

	assert(GPIO_OPS()->poll_pin != NULL);

	while (1) {
		rc = GPIO_OPS()->poll_pin(desc->gpio, desc->timeout);
		if (rc < 0) {
			GLOG_ERR("Wait failed with rc=%d for GPIO: %s <%s>\n",
				 rc, desc->cfg.shadow, strerror(errno));
			break;
		}
		desc->last_value = desc->curr_value;
		if (gpio_get_value(desc->gpio, &desc->curr_value)) {
			GLOG_ERR("Getting current value failed for GPIO: %s <%s>\n",
				 desc->cfg.shadow, strerror(errno));
			break;
		}
		desc->cfg.handler(desc, desc->last_value, desc->curr_value);
	}
	return NULL;
}

int gpio_poll(gpiopoll_desc_t *gpdesc, int timeout)
{
	int i;

	if (!gpdesc || !gpdesc->pins) {
		return -1;
	}

	for (i = 0; i < gpdesc->num_pins; i++) {
		int rc;
		gpiopoll_pin_t *desc = &gpdesc->pins[i];
		assert(desc->handler_started == false);
		desc->timeout = timeout;
		rc = pthread_create(&desc->tid, NULL, gpio_poll_handler, desc);
		if (rc) {
			GLOG_ERR("Create of handler thread failed for GPIO: %s <%s>\n",
				 desc->cfg.shadow, strerror(rc));
			if (gpio_close(desc->gpio)) {
				GLOG_ERR("Close failed for GPIO: %s <%s>\n",
					 desc->cfg.shadow, strerror(errno));
			}
			continue;
		}
		desc->handler_started = true;
	}

	for (i = 0; i < gpdesc->num_pins; i++) {
		int rc;
		void *res;
		gpiopoll_pin_t *desc = &gpdesc->pins[i];
		if (!desc->handler_started) {
			continue;
		}
		rc = pthread_join(desc->tid, &res);
		if (rc != 0) {
			GLOG_ERR("Pthread_join fialed for %s <%s>\n",
				 desc->cfg.shadow, strerror(rc));
		} else if (res == PTHREAD_CANCELED) {
			GLOG_ERR("Potential race condition between close and poll");
		}
	}
	return 0;
}

const struct gpiopoll_config *gpio_poll_get_config(gpiopoll_pin_t *gpdesc)
{
	if (!gpdesc) {
		return NULL;
	}
	return &gpdesc->cfg;
}

gpio_desc_t *gpio_poll_get_descriptor(gpiopoll_pin_t *gpdesc)
{
	if (!gpdesc) {
		return NULL;
	}
	return gpdesc->gpio;
}

int gpio_get_value_by_shadow_list(const char *const *shadows, size_t num, unsigned int *mask)
{
  size_t i;

  if (num > sizeof(*mask) * 8 || !shadows || !mask) {
    errno = EINVAL;
    return -1;
  }
  *mask = 0;

  for (i = 0; i < num; i++) {
    gpio_value_t value = gpio_get_value_by_shadow(shadows[i]);
    if (value == GPIO_VALUE_INVALID) {
      return -1;
    }
    *mask |= (value == GPIO_VALUE_HIGH ? 1 : 0) << i;
  }
  return 0;
}

int gpio_set_value_by_shadow_list(const char *const *shadows, size_t num, unsigned int mask)
{
  size_t i;

  if (num > sizeof(mask) * 8 || !shadows) {
    errno = EINVAL;
    return -1;
  }
  for (i = 0; i < num; i++) {
    gpio_value_t value = (mask & (1 << i)) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    if (gpio_set_value_by_shadow(shadows[i], value)) {
      return -1;
    }
  }
  return 0;
}
