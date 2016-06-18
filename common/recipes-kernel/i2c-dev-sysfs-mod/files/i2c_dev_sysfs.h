/*
 * i2c_dev_sysfs.h - The i2c device sysfs library header
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef I2C_DEV_SYSFS_H
#define I2C_DEV_SYSFS_H

#include <linux/device.h>
#include <linux/types.h>

typedef ssize_t (*i2c_dev_attr_show_fn)(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf);
typedef ssize_t (*i2c_dev_attr_store_fn)(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t count);

#define I2C_DEV_ATTR_SHOW_DEFAULT (i2c_dev_attr_show_fn)(1)
#define I2C_DEV_ATTR_STORE_DEFAULT (i2c_dev_attr_store_fn)(1)

typedef struct i2c_dev_attr_st_ {
  const char *ida_name;
  const char *ida_help;
  i2c_dev_attr_show_fn ida_show;
  i2c_dev_attr_store_fn ida_store;
  int ida_reg;
  int ida_bit_offset;
  int ida_n_bits;
} i2c_dev_attr_st;

typedef struct i2c_sysfs_attr_st_{
	struct device_attribute isa_dev_attr;
  const i2c_dev_attr_st *isa_i2c_attr;
} i2c_sysfs_attr_st;

#define TO_I2C_SYSFS_ATTR(_attr) \
	container_of(_attr, i2c_sysfs_attr_st, isa_dev_attr)

typedef struct i2c_dev_data_st_ {
  struct device *idd_hwmon_dev;
  struct mutex idd_lock;
  i2c_sysfs_attr_st *idd_attrs;
  struct attribute_group idd_attr_group;
} i2c_dev_data_st;

int i2c_dev_sysfs_data_init(struct i2c_client *client,
                            i2c_dev_data_st *data,
                            const i2c_dev_attr_st *dev_attrs,
                            int n_attrs);
void i2c_dev_sysfs_data_clean(struct i2c_client *client, i2c_dev_data_st *data);
int i2c_dev_read_byte(struct device *dev,
                      struct device_attribute *attr);
int i2c_dev_read_nbytes(struct device *dev,
                        struct device_attribute *attr,
                        uint8_t values[],
                        int nbytes);
int i2c_dev_read_word_littleendian(struct device *dev,
                                   struct device_attribute *attr);
int i2c_dev_read_word_bigendian(struct device *dev,
                                struct device_attribute *attr);
ssize_t i2c_dev_show_label(struct device *dev,
                           struct device_attribute *attr,
                           char *buf);
ssize_t i2c_dev_show_ascii(struct device *dev,
                           struct device_attribute *attr,
                           char *buf);

#endif
