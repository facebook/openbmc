/*
* pwr1014a.c - The i2c driver for PWR1014A for Wedge100
 *
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

//#define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "i2c_dev_sysfs.h"

#ifdef DEBUG
#define PWR_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PWR_DEBUG(fmt, ...)
#endif

#define PWR1014A_DELAY 11 //ms
#define PWR1013A_CONTROL_REG 0x09

static const i2c_dev_attr_st pwr1014a_attr_table[] = {
  {
    "mod_hard_powercycle",
    "Writing 0 here will powercycle the entire module (NOT the chassis)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 0, 8,
  },
};

/*
 * PWR1014A i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x3a, I2C_CLIENT_END
};

/*PWR1014A  id */
static const struct i2c_device_id pwr1014a_id[] = {
  {"pwr1014a", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, pwr1014a_id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
static int pwr1014a_probe(struct i2c_client *client)
#else
static int pwr1014a_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
#endif
{
  i2c_dev_data_st *pdata;

  pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
  if (pdata == NULL)
    return -ENOMEM;
  i2c_set_clientdata(client, pdata);

  return devm_i2c_dev_sysfs_init(client, pdata, pwr1014a_attr_table,
                                ARRAY_SIZE(pwr1014a_attr_table));
}

static struct i2c_driver pwr1014a_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "pwr1014a",
  },
  .probe    = pwr1014a_probe,
  .id_table = pwr1014a_id,
  .address_list = normal_i2c,
};

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("PWR1014A Driver for Wedge100");
MODULE_LICENSE("GPL");

module_i2c_driver(pwr1014a_driver);
