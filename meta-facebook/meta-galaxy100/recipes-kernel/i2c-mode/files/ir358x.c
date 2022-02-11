/*
* ir358x.c - The i2c driver for PWR1014A
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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define IR358X_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define IR358X_DEBUG(fmt, ...)
#endif

static ssize_t ir358x_vout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int result;
  int count = 10;

  mutex_lock(&data->idd_lock);
  while((value < 0 || value == 0xffff) && count--) {
	value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
  }
  mutex_unlock(&data->idd_lock);

  if (value < 0) {
    /* error case */
	IR358X_DEBUG("I2C read error!\n");
    return -1;
  }

  result = (value * 1000) / 512;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ir358x_iout_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int result;
  int count = 10;

  mutex_lock(&data->idd_lock);
  while((value < 0 || value == 0xffff) && count--) {
	value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
  }
  mutex_unlock(&data->idd_lock);

  if (value < 0) {
    /* error case */
	IR358X_DEBUG("I2C read error!\n");
    return -1;
  }

  result = ((value & 0x7ff) * 1000)/ 4;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}


static const i2c_dev_attr_st ir358x_attr_table[] = {
  {
    "in0_input",
    NULL,
    ir358x_vout_show,
    NULL,
    0x8b, 0, 8,
  },
  {
    "curr1_input",
    NULL,
    ir358x_iout_show,
    NULL,
    0x8c, 0, 8,
  },
};

static i2c_dev_data_st ir358x_data;

/*
 * IR358x i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x70, 0x72, I2C_CLIENT_END
};

enum {
  IR3581,
  IR3584,
};

/* IR358X id */
static const struct i2c_device_id ir358x_id[] = {
  {"IR3581", IR3581},
  {"IR3584", IR3584},
  { },
};
MODULE_DEVICE_TABLE(i2c, ir358x_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int ir358x_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "ir358x", I2C_NAME_SIZE);
  return 0;
}

static int ir358x_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(ir358x_attr_table) / sizeof(ir358x_attr_table[0]);

  client->flags |= I2C_CLIENT_PEC;
  return i2c_dev_sysfs_data_init(client, &ir358x_data,
                                 ir358x_attr_table, n_attrs);
}

static int ir358x_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &ir358x_data);
  return 0;
}

static struct i2c_driver ir358x_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "ir358x",
  },
  .probe    = ir358x_probe,
  .remove   = ir358x_remove,
  .id_table = ir358x_id,
  .detect   = ir358x_detect,
  .address_list = normal_i2c,
};

static int __init ir358x_mod_init(void)
{
  return i2c_add_driver(&ir358x_driver);
}

static void __exit ir358x_mod_exit(void)
{
  i2c_del_driver(&ir358x_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("IR358X Driver");
MODULE_LICENSE("GPL");

module_init(ir358x_mod_init);
module_exit(ir358x_mod_exit);
