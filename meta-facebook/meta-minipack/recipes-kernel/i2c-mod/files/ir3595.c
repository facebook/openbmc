/*
* ir3595.c - The i2c driver for IR3595A
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

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define IR3595_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define IR3595_DEBUG(fmt, ...)
#endif

static ssize_t ir3595_vout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result = -1;

  result = i2c_dev_read_word_bigendian(dev, attr);

  if (result < 0) {
    /* error case */
    IR3595_DEBUG("I2C read error, result: %d\n", result);
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", (result * 1000) / 2048);
}

static ssize_t ir3595_iout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result = -1;

  result = i2c_dev_read_byte(dev, attr);

  if (result < 0) {
    /* error case */
    IR3595_DEBUG("I2C read error, result: %d\n", result);
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result * 2 * 1000);
}

static ssize_t ir3595_temp_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result = -1;

  result = i2c_dev_read_byte(dev, attr);

  if (result < 0) {
    /* error case */
    IR3595_DEBUG("I2C read error, result: %d\n", result);
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result * 1000);
}


static const i2c_dev_attr_st ir3595_attr_table[] = {
  {
    "in0_input",
    NULL,
    ir3595_vout_show,
    NULL,
    0x9a, 0, 8,
  },
  {
    "curr1_input",
    NULL,
    ir3595_iout_show,
    NULL,
    0x94, 0, 8,
  },
  {
    "temp1_input",
    NULL,
    ir3595_temp_show,
    NULL,
    0x9d, 0, 8,
  },
  {
    "in0_label",
    "TH3 serdes Voltage",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "curr1_label",
    "TH3 serdes Current",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "temp1_label",
    "TH3 serdes Temp",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
};

/*
 * ir3595 i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x12, I2C_CLIENT_END
};


/* ir3595 id */
static const struct i2c_device_id ir3595_id[] = {
  {"ir3595", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, ir3595_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int ir3595_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "ir3595", I2C_NAME_SIZE);
  return 0;
}

static int ir3595_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(ir3595_attr_table) / sizeof(ir3595_attr_table[0]);
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data,
                                 ir3595_attr_table, n_attrs);
}

static int ir3595_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

  return 0;
}

static struct i2c_driver ir3595_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "ir3595",
  },
  .probe    = ir3595_probe,
  .remove   = ir3595_remove,
  .id_table = ir3595_id,
  .detect   = ir3595_detect,
  .address_list = normal_i2c,
};

static int __init ir3595_mod_init(void)
{
  return i2c_add_driver(&ir3595_driver);
}

static void __exit ir3595_mod_exit(void)
{
  i2c_del_driver(&ir3595_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("ir3595 Driver");
MODULE_LICENSE("GPL");

module_init(ir3595_mod_init);
module_exit(ir3595_mod_exit);
