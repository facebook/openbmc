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
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include <openbmc/obmc-i2c.h>
#include "gpio_int.h"

/*
 * gpio sysfs paths and files.
 */
#define GPIO_SYSFS_ROOT			"/sys/class/gpio"
#define GPIO_SYSFS_EXPORT		"/sys/class/gpio/export"
#define GPIO_SYSFS_UNEXPORT		"/sys/class/gpio/unexport"
#define GPIO_SYSFS_EDGE_FILE		"edge"
#define GPIO_SYSFS_VALUE_FILE		"value"
#define GPIO_SYSFS_DIRECTION_FILE	"direction"

/*
 * Macros to reference gpio file descriptors.
 */
#define GPIO_EDGE_FD(g)			((g)->u.sysfs_attr.edge_fd)
#define GPIO_VALUE_FD(g)		((g)->u.sysfs_attr.value_fd)
#define GPIO_DIRECTION_FD(g)		((g)->u.sysfs_attr.direction_fd)

/*
 * Default buffer size when reading/writing gpio sysfs files.
 */
#define GPIO_SYSFS_IO_BUF_SIZE		16

/*
 * Aspeed gpio controller's device name in sysfs tree.
 */
#define GPIO_SYSFS_ASPEED_DEVICE	"1e780000.gpio"

static char* gsysfs_pin_dir(char *buf, size_t size, int pin_num)
{
	snprintf(buf, size, "%s/gpio%d", GPIO_SYSFS_ROOT, pin_num);
	return buf;
}

static char* gsysfs_value_abspath(char *buf, size_t size, int pin_num)
{
	char pin_dir[PATH_MAX];

	gsysfs_pin_dir(pin_dir, sizeof(pin_dir), pin_num);
	return path_join(buf, size,
			 pin_dir, GPIO_SYSFS_VALUE_FILE, NULL);
}

static char* gsysfs_direction_abspath(char *buf, size_t size, int pin_num)
{
	char pin_dir[PATH_MAX];

	gsysfs_pin_dir(pin_dir, sizeof(pin_dir), pin_num);
	return path_join(buf, size,
			 pin_dir, GPIO_SYSFS_DIRECTION_FILE, NULL);
}

static char* gsysfs_edge_abspath(char *buf, size_t size, int pin_num)
{
	char pin_dir[PATH_MAX];

	gsysfs_pin_dir(pin_dir, sizeof(pin_dir), pin_num);
	return path_join(buf, size,
			 pin_dir, GPIO_SYSFS_EDGE_FILE, NULL);
}

static int gsysfs_export_control(const char *ctrl_file, int pin_num)
{
	int fd, count;
	int status = 0;
	char data[GPIO_SYSFS_IO_BUF_SIZE];

	GLOG_DEBUG("open <%s> for write\n", ctrl_file);
	fd = open(ctrl_file, O_WRONLY);
	if (fd < 0) {
		GLOG_ERR("failed to open <%s>: %s\n",
			 ctrl_file, strerror(errno));
		return -1;
	}

	snprintf(data, sizeof(data), "%d", pin_num);
	count = strlen(data);
	GLOG_DEBUG("write data (%s) to <%s>\n", data, ctrl_file);
	if (write(fd, data, count) != count) {
		GLOG_ERR("failed to write <%s> to <%s>: %s\n",
			 data, ctrl_file, strerror(errno));
		status = -1;
	}

	close(fd);
	return status;
}

static int sysfs_gpio_export(int pin_num, const char *shadow_path)
{
	char pin_dir[PATH_MAX];

	if (gsysfs_export_control(GPIO_SYSFS_EXPORT, pin_num) != 0) {
		return -1;
	}

	gsysfs_pin_dir(pin_dir, sizeof(pin_dir), pin_num);
	GLOG_DEBUG("check if <%s> is created properly\n", pin_dir);
	if (!path_exists(pin_dir)) {
		GLOG_ERR("unable to find gpio sysfs direction <%s>\n",
			 pin_dir);
		errno = ENOENT;
		return -1;
	}

	GLOG_DEBUG("setup mapping between <%s> and <%s>\n",
		   shadow_path, pin_dir);
	if (symlink(pin_dir, shadow_path) != 0) {
		gsysfs_export_control(GPIO_SYSFS_UNEXPORT, pin_num);
		return -1;
	}

	return 0;
}

static int sysfs_gpio_unexport(int pin_num, const char *shadow_path)
{
	GLOG_DEBUG("remove symlink <%s>\n", shadow_path);
	if (unlink(shadow_path) != 0) {
		GLOG_ERR("failed to remove <%s>: %s\n",
			 shadow_path, strerror(errno));
		/* fall through */
	}

	return gsysfs_export_control(GPIO_SYSFS_UNEXPORT, pin_num);
}

static int sysfs_gpio_open(gpio_desc_t* gdesc)
{
	assert(gdesc != NULL);
	assert(gdesc->pin_num >= 0);

	GPIO_EDGE_FD(gdesc) = -1;
	GPIO_VALUE_FD(gdesc) = -1;
	GPIO_DIRECTION_FD(gdesc) = -1;

	return 0;
}

static int sysfs_gpio_close(gpio_desc_t* gdesc)
{
	assert(gdesc != NULL);

	if (GPIO_EDGE_FD(gdesc) >= 0) {
		close(GPIO_EDGE_FD(gdesc));
		GPIO_EDGE_FD(gdesc) = -1;
	}
	if (GPIO_VALUE_FD(gdesc) >= 0) {
		close(GPIO_VALUE_FD(gdesc));
		GPIO_VALUE_FD(gdesc) = -1;
	}
	if (GPIO_DIRECTION_FD(gdesc) >= 0) {
		close(GPIO_DIRECTION_FD(gdesc));
		GPIO_DIRECTION_FD(gdesc) = -1;
	}

	return 0;
}

static int gsysfs_setup_fd(const char *pathname, int *fd)
{
	GLOG_DEBUG("set up file descriptor for <%s>\n", pathname);

	if (*fd < 0) {
		*fd = open(pathname, O_RDWR);
		if (*fd < 0) {
			GLOG_ERR("failed to open <%s>: %s\n",
				 pathname, strerror(errno));
			return -1;
		}
	} else if (lseek(*fd, 0, SEEK_SET) < 0) {
		GLOG_ERR("failed to update offset of <%s>: %s\n",
			 pathname, strerror(errno));
		return -1;
	}

	return 0;
}

static int gsysfs_read_str(const char *pathname, int fd,
			   char *data, size_t size)
{
	int nread;

	GLOG_DEBUG("read data from <%s>\n", pathname);
	nread = file_read_bytes(fd, data, size - 1);
	if (nread <= 0) {
		GLOG_ERR("failed to read data from <%s>\n", pathname);
		errno = EIO;
		return -1;
	}

	data[nread] = '\0';
	str_rstrip(data); /* remove trailing new-line character. */
	GLOG_DEBUG("data read from <%s>: %s\n", pathname, data);
	return 0;
}

static int gsysfs_write_str(const char *pathname, int fd,
			    const char *data)
{
	int nwrite;
	int count = strlen(data);

	GLOG_DEBUG("write %d bytes (%s) to <%s>\n",
		   count, data, pathname);
	nwrite = write(fd, data, count);
	if (nwrite < 0) {
		GLOG_ERR("failed to write <%s>: %s\n",
			 pathname, strerror(errno));
		return -1;
	} else if (nwrite != count) {
		GLOG_ERR("failed to write <%s>: "
			 "expect %d bytes, actual %d bytes\n",
			 pathname, count, nwrite);
		errno = EIO;
		return -1;
	}

	return 0;
}

static int sysfs_gpio_get_value(gpio_desc_t *gdesc, gpio_value_t *value)
{
	int val;
	char pathname[PATH_MAX];
	char buf[GPIO_SYSFS_IO_BUF_SIZE];

	assert(IS_VALID_GPIO_DESC(gdesc));
	assert(value != NULL);

	gsysfs_value_abspath(pathname, sizeof(pathname), gdesc->pin_num);
	if (gsysfs_setup_fd(pathname, &GPIO_VALUE_FD(gdesc)) != 0)
		return -1;

	if (gsysfs_read_str(pathname, GPIO_VALUE_FD(gdesc),
			    buf, sizeof(buf)) != 0)
		return -1;

	val = strtol(buf, NULL, 10);
	*value = (val ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
	return 0;
}

static int sysfs_gpio_set_value(gpio_desc_t *gdesc, gpio_value_t value)
{
	char *data;
	char pathname[PATH_MAX];

	assert(IS_VALID_GPIO_DESC(gdesc));
	assert(IS_VALID_GPIO_VALUE(value));

	gsysfs_value_abspath(pathname, sizeof(pathname), gdesc->pin_num);
	if (gsysfs_setup_fd(pathname, &GPIO_VALUE_FD(gdesc)) != 0)
		return -1;

	data = (value == GPIO_VALUE_LOW ? "0" : "1");
	return gsysfs_write_str(pathname, GPIO_VALUE_FD(gdesc), data);
}

int sysfs_gpio_get_direction(gpio_desc_t *gdesc, gpio_direction_t *dir)
{
	char pathname[PATH_MAX];
	char buf[GPIO_SYSFS_IO_BUF_SIZE];

	assert(IS_VALID_GPIO_DESC(gdesc));
	assert(dir != NULL);

	gsysfs_direction_abspath(pathname, sizeof(pathname),
				 gdesc->pin_num);
	if (gsysfs_setup_fd(pathname, &GPIO_DIRECTION_FD(gdesc)) != 0)
		return -1;

	if (gsysfs_read_str(pathname, GPIO_DIRECTION_FD(gdesc),
			    buf, sizeof(buf)) != 0)
		return -1;

	if (strcmp(buf, "in") == 0) {
		*dir = GPIO_DIRECTION_IN;
	} else if (strcmp(buf, "out") == 0) {
		*dir = GPIO_DIRECTION_OUT;
	} else {
		GLOG_ERR("unable to parse gpio direction <%s>>\n", buf);
		errno = EPROTO;
		return -1;
	}

	return 0;
}

int sysfs_gpio_set_direction(gpio_desc_t *gdesc, gpio_direction_t dir)
{
	char *data;
	char pathname[PATH_MAX];

	assert(IS_VALID_GPIO_DESC(gdesc));
	assert(IS_VALID_GPIO_DIRECTION(dir));

	gsysfs_direction_abspath(pathname, sizeof(pathname),
				 gdesc->pin_num);
	if (gsysfs_setup_fd(pathname, &GPIO_DIRECTION_FD(gdesc)) != 0)
		return -1;

	data = (dir == GPIO_DIRECTION_IN ? "in" : "out");
	return gsysfs_write_str(pathname, GPIO_DIRECTION_FD(gdesc), data);
}

static struct {
	gpio_edge_t val;
	const char *str;
} gsysfs_edge_types[] = {
	{GPIO_EDGE_NONE,	"none"},
	{GPIO_EDGE_RISING,	"rising"},
	{GPIO_EDGE_FALLING,	"falling"},
	{GPIO_EDGE_BOTH,	"both"},

	/* This is the last entry */
	{GPIO_EDGE_INVALID,	NULL},
};

static const char* gsysfs_edge_val_to_str(gpio_edge_t edge)
{
	int i;

	for (i = 0; gsysfs_edge_types[i].str != NULL; i++) {
		if (gsysfs_edge_types[i].val == edge)
			return gsysfs_edge_types[i].str;
	}

	return "";
}

static gpio_edge_t gsysfs_edge_str_to_val(const char *str)
{
	int i;

	for (i = 0; gsysfs_edge_types[i].str != NULL; i++) {
		if (strcmp(str, gsysfs_edge_types[i].str) == 0)
			return gsysfs_edge_types[i].val;
	}

	return GPIO_EDGE_INVALID;
}

static int sysfs_gpio_get_edge(gpio_desc_t *gdesc, gpio_edge_t *edge)
{
	gpio_edge_t val;
	char pathname[PATH_MAX];
	char buf[GPIO_SYSFS_IO_BUF_SIZE];

	assert(IS_VALID_GPIO_DESC(gdesc));
	assert(edge != NULL);

	gsysfs_edge_abspath(pathname, sizeof(pathname), gdesc->pin_num);
	if (gsysfs_setup_fd(pathname, &GPIO_EDGE_FD(gdesc)) != 0)
		return -1;

	if (gsysfs_read_str(pathname, GPIO_EDGE_FD(gdesc),
			    buf, sizeof(buf)) != 0)
		return -1;

	val = gsysfs_edge_str_to_val(buf);
	if (val == GPIO_EDGE_INVALID) {
		errno = EPROTO;
		return -1;
	}

	*edge = val;
	return 0;
}

static int sysfs_gpio_set_edge(gpio_desc_t *gdesc, gpio_edge_t edge)
{
	const char *data;
	char pathname[PATH_MAX];

	assert(IS_VALID_GPIO_DESC(gdesc));
	assert(IS_VALID_GPIO_EDGE(edge));

	gsysfs_edge_abspath(pathname, sizeof(pathname), gdesc->pin_num);
	if (gsysfs_setup_fd(pathname, &GPIO_EDGE_FD(gdesc)) != 0)
		return -1;

	data = gsysfs_edge_val_to_str(edge);
	return gsysfs_write_str(pathname, GPIO_EDGE_FD(gdesc), data);
}

static char* gsysfs_chip_read_device(char *buf,
				     size_t size,
				     const char *chip_dir)
{
	int len;
	char *base_name;
	char dev_path[PATH_MAX];
	char target_path[PATH_MAX];

	path_join(dev_path, sizeof(dev_path), chip_dir, "device", NULL);
	if (!path_islink(dev_path))
		return NULL;

	len = readlink(dev_path, target_path, sizeof(target_path));
	if (len < 0) {
		GLOG_ERR("failed to read target from symlink <%s>: %s\n",
			 dev_path, strerror(errno));
		return NULL;
	} else if (len >= sizeof(target_path)) {
		GLOG_ERR("target path of symlink <%s> is truncated\n",
			 dev_path);
		return NULL;
	}
	target_path[len] = '\0';

	base_name = basename(target_path);
	strncpy(buf, base_name, size);
	return buf;
}

static int gsysfs_chip_read_base(const char *chip_dir)
{
	int fd;
	char pathname[PATH_MAX];
	char buf[GPIO_SYSFS_IO_BUF_SIZE];

	path_join(pathname, sizeof(pathname), chip_dir, "base", NULL);
	fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		GLOG_ERR("failed to open <%s>: %s\n",
			 pathname, strerror(errno));
		return -1;
	}

	if (gsysfs_read_str(pathname, fd, buf, sizeof(buf)) != 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return (int)strtol(buf, NULL, 0);
}

static int gsysfs_chip_read_ngpio(const char *chip_dir)
{
	int fd;
	char pathname[PATH_MAX];
	char buf[GPIO_SYSFS_IO_BUF_SIZE];

	path_join(pathname, sizeof(pathname), chip_dir, "ngpio", NULL);
	fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		GLOG_ERR("failed to open <%s>: %s\n",
			 pathname, strerror(errno));
		return -1;
	}

	if (gsysfs_read_str(pathname, fd, buf, sizeof(buf)) != 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return (int)strtol(buf, NULL, 0);
}

static void chip_desc_init(gpiochip_desc_t *gcdesc,
			   int base,
			   int ngpio,
			   const char *dev_name)
{
	memset(gcdesc, 0, sizeof(*gcdesc));
	gcdesc->base = base;
	gcdesc->ngpio = ngpio;
	strncpy(gcdesc->dev_name, dev_name, sizeof(gcdesc->dev_name));

	/*
	 * Update "chip_type" and gpiochip_ops.
	 */
	if (strcmp(dev_name, GPIO_SYSFS_ASPEED_DEVICE) == 0) {
		strncpy(gcdesc->chip_type, GPIO_CHIP_ASPEED_SOC,
			sizeof(gcdesc->chip_type));
		gcdesc->ops = &aspeed_gpiochip_ops;
	} else if (i2c_sysfs_path_parse(dev_name, NULL, NULL) == 0) {
		strncpy(gcdesc->chip_type, GPIO_CHIP_I2C_EXPANDER,
			sizeof(gcdesc->chip_type));
	}
}

/*
 * XXX workaround for openbmc kernel 4.1:
 * The 4.1 aspeed gpio driver creates multiple "gpiochip" entries for
 * the soc gpio controller: each "gpiochip" represents a 8-bit gpio group.
 * As it takes a lot of extra overhead to maintain multiple "gpiochip"
 * structures (which belongs to the same gpio controller), we decide to
 * combine all aspeed "gpiochip" to a single one (same as linux 4.17 and
 * higher versions).
 */
static void add_aspeed_chip(gpiochip_desc_t *gcdesc)
{
	int ngpio;
	soc_model_t soc_model;

	soc_model = get_soc_model();
	ngpio = (soc_model == SOC_MODEL_ASPEED_G5 ? 232 : 220);

	GLOG_DEBUG("manually add aspeed gpio chip\n");
	chip_desc_init(gcdesc, 0, ngpio, GPIO_SYSFS_ASPEED_DEVICE);
}

static int sysfs_gpiochip_enumerate(gpiochip_desc_t *chips, size_t size)
{
	int i = 0;
	DIR* dirp;
	struct dirent *dent;
	char chip_dir[PATH_MAX];
	bool found_aspeed_chip = false;

	assert(chips != NULL);
	assert(size > 0);

	dirp = opendir(GPIO_SYSFS_ROOT);
	if (dirp == NULL)
		return -1;

	while ((dent = readdir(dirp)) != NULL) {
		int base, ngpio;
		char *dev_name;
		char buf[NAME_MAX];

		if (!str_startswith(dent->d_name, "gpiochip"))
			continue;

		path_join(chip_dir, sizeof(chip_dir),
			  GPIO_SYSFS_ROOT, dent->d_name, NULL);

		dev_name = gsysfs_chip_read_device(buf, sizeof(buf),
						   chip_dir);
		if (dev_name == NULL) {
			GLOG_DEBUG("skip %s: device entry is missing\n",
				   dent->d_name);
			continue;
		}

		base = gsysfs_chip_read_base(chip_dir);
		if (base < 0) {
			GLOG_ERR("unable to read gpio base from <%s>\n",
				 dent->d_name);
			continue;
		}

		ngpio = gsysfs_chip_read_ngpio(chip_dir);
		if (ngpio < 0) {
			GLOG_ERR("unable to read gpio base from <%s>\n",
				 dent->d_name);
			continue;
		}

		/*
		 * We found a gpiochip.
		 */
		GLOG_DEBUG("found gpiochip <%s>, base=%d, ngpio=%d\n",
			   dev_name, base, ngpio);
		chip_desc_init(&chips[i], base, ngpio, dev_name);
		if (strcmp(dev_name, GPIO_SYSFS_ASPEED_DEVICE) == 0)
			found_aspeed_chip = true;
		if (++i >= size)
			break;
	}
	closedir(dirp);	/* ignore errors */

	if (!found_aspeed_chip) {
		assert(i < size);
		if (i < size) {
			add_aspeed_chip(&chips[i]);
			i++;
		} else {
			GLOG_ERR("no space left for aspeed gpio chip\n");
		}
	}

	return i;
}

struct gpio_backend_ops gpio_sysfs_ops = {
	.export_pin = sysfs_gpio_export,
	.unexport_pin = sysfs_gpio_unexport,

	.open_pin = sysfs_gpio_open,
	.close_pin = sysfs_gpio_close,

	.get_pin_value = sysfs_gpio_get_value,
	.set_pin_value = sysfs_gpio_set_value,
	.get_pin_direction = sysfs_gpio_get_direction,
	.set_pin_direction = sysfs_gpio_set_direction,
	.get_pin_edge = sysfs_gpio_get_edge,
	.set_pin_edge = sysfs_gpio_set_edge,

	.chip_enumerate = sysfs_gpiochip_enumerate,
};
