/*
 * i2c_dev_sysfs.c - The i2c device sysfs library
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

//#define DEBUG

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#include "i2c_dev_sysfs.h"

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif

ssize_t i2c_dev_show_label(struct device *dev,
                           struct device_attribute *attr,
                           char *buf)
{
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;

  if (dev_attr->ida_help) {
    return sprintf(buf, "%s\n", dev_attr->ida_help);
  }
  return sprintf(buf, "%s\n", dev_attr->ida_name);
}
EXPORT_SYMBOL_GPL(i2c_dev_show_label);

int i2c_dev_read_byte(struct device *dev,
                      struct device_attribute *attr)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;
  int val_mask;

  val_mask = ~(((-1) >> (dev_attr->ida_n_bits)) << (dev_attr->ida_n_bits));

  mutex_lock(&data->idd_lock);

  val = i2c_smbus_read_byte_data(client, dev_attr->ida_reg);

  mutex_unlock(&data->idd_lock);

  if (val < 0) {
    /* error case */
    return val;
  }

  val = (val >> dev_attr->ida_bit_offset) & val_mask;
  return val;
}
EXPORT_SYMBOL_GPL(i2c_dev_read_byte);

int i2c_dev_read_nbytes(struct device *dev,
                        struct device_attribute *attr,
                        uint8_t values[],
                        int nbytes)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int i;
  int ret_val;

  mutex_lock(&data->idd_lock);
  for (i = 0; i < nbytes; ++i) {
    ret_val = i2c_smbus_read_byte_data(client, dev_attr->ida_reg + i);
    if (ret_val < 0) {
      mutex_unlock(&data->idd_lock);
      return ret_val;
    }
    if (ret_val > 255) {
      mutex_unlock(&data->idd_lock);
      return EFAULT;
    }
    values[i] = ret_val;
  }
  mutex_unlock(&data->idd_lock);
  return nbytes;
}
EXPORT_SYMBOL_GPL(i2c_dev_read_nbytes);

int i2c_dev_read_word_littleendian(struct device *dev,
                                struct device_attribute *attr)
{
  uint8_t values[2];
  int ret_val;

  ret_val = i2c_dev_read_nbytes(dev, attr, values, 2);
  if (ret_val < 0) {
    return ret_val;
  }
  // values[0] : LSB
  // values[1] : MSB
  return ((values[1]<<8) + values[0]);
}
EXPORT_SYMBOL_GPL(i2c_dev_read_word_littleendian);

int i2c_dev_read_word_bigendian(struct device *dev,
                                struct device_attribute *attr)
{
  uint8_t values[2];
  int ret_val;

  ret_val = i2c_dev_read_nbytes(dev, attr, values, 2);
  if (ret_val < 0) {
    return ret_val;
  }
  // values[0] : MSB
  // values[1] : LSB
  return ((values[0]<<8) + values[1]);
}
EXPORT_SYMBOL_GPL(i2c_dev_read_word_bigendian);

ssize_t i2c_dev_show_ascii(struct device *dev,
                           struct device_attribute *attr,
                           char *buf)
{
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int nbytes, ret_val;

  nbytes = (dev_attr->ida_n_bits) / 8;
  ret_val = i2c_dev_read_nbytes(dev, attr, buf, nbytes);
  if (ret_val < 0) {
    return ret_val;
  }
  //buf[] : ascii data
  buf[nbytes] = '\n';
  return (nbytes + 1);
}
EXPORT_SYMBOL_GPL(i2c_dev_show_ascii);

static ssize_t i2c_dev_sysfs_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val, val_mask, reg_val;

  if (!dev_attr->ida_show) {
    return -EOPNOTSUPP;
  }

  if (dev_attr->ida_show != I2C_DEV_ATTR_SHOW_DEFAULT) {
    return dev_attr->ida_show(dev, attr, buf);
  }

  val_mask = (1 << (dev_attr->ida_n_bits)) - 1;

  mutex_lock(&data->idd_lock);

  /* default handling */
  reg_val = i2c_smbus_read_byte_data(client, dev_attr->ida_reg);

  mutex_unlock(&data->idd_lock);

  if (reg_val < 0) {
    /* error case */
    return reg_val;
  }

  val = (reg_val >> dev_attr->ida_bit_offset) & val_mask;

  return scnprintf(buf, PAGE_SIZE,
                   "%#x\n\nNote:\n%s\n"
                   "Bit[%d:%d] @ register %#x, register value %#x\n",
                   val, (dev_attr->ida_help) ? dev_attr->ida_help : "",
                   dev_attr->ida_bit_offset + dev_attr->ida_n_bits - 1,
                   dev_attr->ida_bit_offset, dev_attr->ida_reg, reg_val);
}

static ssize_t i2c_dev_sysfs_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;
  int req_val;
  int req_val_mask;

  if (!dev_attr->ida_store) {
    return -EOPNOTSUPP;
  }

  if (dev_attr->ida_store != I2C_DEV_ATTR_STORE_DEFAULT) {
    return dev_attr->ida_store(dev, attr, buf, count);
  }

  /* parse the buffer */
  if (sscanf(buf, "%i", &req_val) <= 0) {
    return -EINVAL;
  }
  req_val_mask = ~(((-1) >> (dev_attr->ida_n_bits)) << (dev_attr->ida_n_bits));
  req_val &= req_val_mask;

  mutex_lock(&data->idd_lock);

  /* default handling, first read back the current value */
  val = i2c_smbus_read_byte_data(client, dev_attr->ida_reg);

  if (val < 0) {
    /* fail to read */
    goto unlock_out;
  }

  /* mask out all bits for the value requested */
  val &= ~(req_val_mask << dev_attr->ida_bit_offset);
  val |= req_val << dev_attr->ida_bit_offset;

  val = i2c_smbus_write_byte_data(client, dev_attr->ida_reg, val);

 unlock_out:
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
    /* error case */
    return val;
  }

  return count;
}

void i2c_dev_sysfs_data_clean(struct i2c_client *client, i2c_dev_data_st *data)
{
  if (!data) {
    return;
  }
  if (data->idd_hwmon_dev) {
    hwmon_device_unregister(data->idd_hwmon_dev);
  }
  if (data->idd_attr_group.attrs) {
    sysfs_remove_group(&client->dev.kobj, &data->idd_attr_group);
    kfree(data->idd_attr_group.attrs);
  }
  if (data->idd_attrs) {
    kfree(data->idd_attrs);
  }
  memset(data, 0, sizeof(*data));
}
EXPORT_SYMBOL_GPL(i2c_dev_sysfs_data_clean);

int i2c_dev_sysfs_data_init(struct i2c_client *client,
                            i2c_dev_data_st *data,
                            const i2c_dev_attr_st *dev_attrs,
                            int n_attrs)
{
  int i;
  int err;
  i2c_sysfs_attr_st *cur_attr;
  const i2c_dev_attr_st *cur_dev_attr;
  struct attribute **cur_grp_attr;

  memset(data, 0, sizeof(*data));

  i2c_set_clientdata(client, data);
  mutex_init(&data->idd_lock);

  /* allocate all attribute */
  data->idd_attrs = kzalloc(sizeof(*data->idd_attrs) * n_attrs, GFP_KERNEL);
  /* allocate the attribute group. +1 for the last NULL */
  data->idd_attr_group.attrs
    = kzalloc(sizeof(*data->idd_attr_group.attrs) * (n_attrs + 1), GFP_KERNEL);
  if (!data->idd_attrs || !data->idd_attr_group.attrs) {
    err = -ENOMEM;
    goto exit_cleanup;
  }
  PP_DEBUG("Allocated %u attributes", n_attrs);

  for (i = 0,
         cur_attr = &data->idd_attrs[0],
         cur_grp_attr = &data->idd_attr_group.attrs[0],
         cur_dev_attr = dev_attrs;
       i < n_attrs;
       i++, cur_attr++, cur_grp_attr++, cur_dev_attr++) {

    mode_t mode = S_IRUGO;
    if (cur_dev_attr->ida_store) {
      mode |= S_IWUSR;
    }
    cur_attr->isa_dev_attr.attr.name = cur_dev_attr->ida_name;
    cur_attr->isa_dev_attr.attr.mode = mode;
    cur_attr->isa_dev_attr.show = i2c_dev_sysfs_show;
    cur_attr->isa_dev_attr.store = i2c_dev_sysfs_store;
    *cur_grp_attr = &cur_attr->isa_dev_attr.attr;
    cur_attr->isa_i2c_attr = cur_dev_attr;
    PP_DEBUG("Created attribute \"%s\"", cur_attr->isa_dev_attr.attr.name);
  }

  /* Register sysfs hooks */
  if ((err = sysfs_create_group(&client->dev.kobj, &data->idd_attr_group))) {
    goto exit_cleanup;
  }

  data->idd_hwmon_dev = hwmon_device_register(&client->dev);
  if (IS_ERR(data->idd_hwmon_dev)) {
    err = PTR_ERR(data->idd_hwmon_dev);
    data->idd_hwmon_dev = NULL;
    goto exit_cleanup;
  }

  PP_DEBUG("i2c device sysfs init data done");
  return 0;

 exit_cleanup:
  i2c_dev_sysfs_data_clean(client, data);
  return err;
}
EXPORT_SYMBOL_GPL(i2c_dev_sysfs_data_init);


MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("i2c device sysfs attribute library");
MODULE_LICENSE("GPL");
