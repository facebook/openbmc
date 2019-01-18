/*
 * max34461.c - The i2c driver for MAX34461
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
#define MAX34461_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define MAX34461_DEBUG(fmt, ...)
#endif

#define MAX34461_PAGE_REG 0x00
#define MAX34461_DELAY 10

static int max34461_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int count = 10;
  u8 page = dev_attr->ida_bit_offset;

  mutex_lock(&data->idd_lock);
  if (i2c_smbus_write_byte_data(client, MAX34461_PAGE_REG, page) < 0) {
    mutex_unlock(&data->idd_lock);
    MAX34461_DEBUG("I2C write error\n");
    return -1;
  }
  msleep(MAX34461_DELAY);

  while((value < 0 || value == 0xffff) && count--) {
    value = i2c_smbus_read_word_data(client, dev_attr->ida_reg);
  }
  mutex_unlock(&data->idd_lock);

  if (value < 0) {
    /* error case */
    MAX34461_DEBUG("I2C read error, value: %d\n", value);
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static const i2c_dev_attr_st max34461_attr_table[] = {
  {
    "in1_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 0, 16,
  },
  {
    "in2_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 1, 16,
  },
  {
    "in3_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 2, 16,
  },
  {
    "in4_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 3, 16,
  },
  {
    "in5_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 4, 16,
  },
  {
    "in6_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 5, 16,
  },
  {
    "in7_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 6, 16,
  },
  {
    "in8_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 7, 16,
  },
  {
    "in9_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 8, 16,
  },
  {
    "in10_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 9, 16,
  },
  {
    "in11_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 10, 16,
  },
  {
    "in12_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 11, 16,
  },
  {
    "in13_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 12, 16,
  },
  {
    "in14_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 13, 16,
  },
  {
    "in15_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 14, 16,
  },
  {
    "in16_input",
    NULL,
    max34461_vol_show,
    NULL,
    0x8b, 15, 16,
  },
};

static i2c_dev_data_st max34461_data;

/*
 * max34461 i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x74, I2C_CLIENT_END
};

enum {
  MAX34461,
};

/* max34461 id */
static const struct i2c_device_id max34461_id[] = {
  {"max34461", MAX34461},
  { },
};
MODULE_DEVICE_TABLE(i2c, max34461_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int max34461_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "max34461", I2C_NAME_SIZE);
  return 0;
}

static int max34461_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(max34461_attr_table) / sizeof(max34461_attr_table[0]);

  return i2c_dev_sysfs_data_init(client, &max34461_data,
                                 max34461_attr_table, n_attrs);
}

static int max34461_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &max34461_data);
  return 0;
}

static struct i2c_driver max34461_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "max34461",
  },
  .probe    = max34461_probe,
  .remove   = max34461_remove,
  .id_table = max34461_id,
  .detect   = max34461_detect,
  .address_list = normal_i2c,
};

static int __init max34461_mod_init(void)
{
  return i2c_add_driver(&max34461_driver);
}

static void __exit max34461_mod_exit(void)
{
  i2c_del_driver(&max34461_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("max34461 Driver");
MODULE_LICENSE("GPL");

module_init(max34461_mod_init);
module_exit(max34461_mod_exit);
