/*
* isl68127.c - The i2c driver for ISL68127
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

// #define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define ISL68127_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define ISL68127_DEBUG(fmt, ...)
#endif

#define ISL68127_PAGE_REG 0x00
#define ISL68127_DELAY 10

static int isl_convert(struct device *dev, struct device_attribute *attr)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int count = 10;

  mutex_lock(&data->idd_lock);
  i2c_smbus_write_byte_data(client, ISL68127_PAGE_REG, 0x01);
  msleep(ISL68127_DELAY);

  while((value < 0 || value == 0xffff) && count--) {
    value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
  }

  mutex_unlock(&data->idd_lock);

  if (value < 0) {
    /* error case */
    ISL68127_DEBUG("I2C read error, value: %d\n", value);
    return -1;
  }

  return value;
}

static ssize_t isl68127_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result = -1;

  result = isl_convert(dev, attr);
  if (result < 0) {
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t isl68127_iout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result = -1;

  result = isl_convert(dev, attr);
  if (result < 0) {
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", (result * 1000) / 10);
}

static ssize_t isl68127_temp_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result = -1;

  result = isl_convert(dev, attr);
  if (result < 0) {
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result * 1000);
}

static int parse_val(long val)
{
  if (val > 927 || val < 750)
    return -1;

  return 0;
}

static ssize_t isl68127_vol_write(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;
  
  if (sscanf(buf, "%i", &val) <= 0) {
    return -EINVAL;
  }
  if (parse_val(val))
    return -EINVAL;

  mutex_lock(&data->idd_lock);
  i2c_smbus_write_word_data(client, dev_attr->ida_reg, val);
  mutex_unlock(&data->idd_lock);
  
  return count;
}


static const i2c_dev_attr_st isl68127_attr_table[] = {
  {
    "vo0_input",
    NULL,
    isl68127_vol_show,
    isl68127_vol_write,
    0x21, 0, 8,
  },
  {
    "in0_input",
    NULL,
    isl68127_vol_show,
    NULL,
    0x8b, 0, 8,
  },
  {
    "curr1_input",
    NULL,
    isl68127_iout_show,
    NULL,
    0x8c, 0, 8,
  },
  {
    "temp1_input",
    NULL,
    isl68127_temp_show,
    NULL,
    0x8d, 0, 8,
  },
  {
    "in0_label",
    "TH3 core Voltage",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "vo0_label",
    "TH3 core Voltage for setting",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "curr1_label",
    "TH3 core Current",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "temp1_label",
    "TH3 core Temp",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
};

/*
 * isl68127 i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x60, I2C_CLIENT_END
};

enum {
  ISL68127,
};

/* isl68127 id */
static const struct i2c_device_id isl68127_id[] = {
  {"isl68127", ISL68127},
  { },
};
MODULE_DEVICE_TABLE(i2c, isl68127_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int isl68127_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "isl68127", I2C_NAME_SIZE);
  return 0;
}

static int isl68127_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(isl68127_attr_table) / sizeof(isl68127_attr_table[0]);
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  client->flags |= I2C_CLIENT_PEC;
  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data,
                                 isl68127_attr_table, n_attrs);
}

static int isl68127_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

  return 0;
}

static struct i2c_driver isl68127_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "isl68127",
  },
  .probe    = isl68127_probe,
  .remove   = isl68127_remove,
  .id_table = isl68127_id,
  .detect   = isl68127_detect,
  .address_list = normal_i2c,
};

static int __init isl68127_mod_init(void)
{
  return i2c_add_driver(&isl68127_driver);
}

static void __exit isl68127_mod_exit(void)
{
  i2c_del_driver(&isl68127_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("isl68127 Driver");
MODULE_LICENSE("GPL");

module_init(isl68127_mod_init);
module_exit(isl68127_mod_exit);
