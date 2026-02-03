/*
 * smbcpld.c - The i2c driver for the SMB CPLD on MERU
 *
 * Copyright 2023-present Facebook. All Rights Reserved.
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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include "i2c_dev_sysfs.h"

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif

static const i2c_dev_attr_st smbcpld_attr_table[] = {
  {
    "cpld_ver_minor",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 8,
  },
  {
    "cpld_ver_major",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 8,
  },
  {
    "smb_power_status",
    "0x1: Power is good\n"
		"0x0: Power is not good",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x5, 0, 1,
  },
  {
    "asic_0_power_status",
    "0x1: Power is good\n"
		"0x0: Power is not good",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x5, 1, 1,
  },
  {
    "asic_1_power_status",
    "0x1: Power is good\n"
		"0x0: Power is not good",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x5, 2, 1,
  },
  {
    "j3_system_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 1,
  },
  {
    "j3_pcie_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 1, 1,
  },
  {
    "asic_0_system_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 0, 1,
  },
  {
    "asic_0_pcie_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 1, 1,
  },
  {
    "asic_1_system_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 2, 1,
  },
  {
    "asic_1_pcie_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 3, 1,
  },
  {
    "ramon3_0_system_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xd1, 0, 1,
  },
  {
    "ramon3_0_pcie_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xd1, 1, 1,
  },
  {
    "ramon3_1_system_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xd1, 2, 1,
  },
  {
    "ramon3_1_pcie_reset",
    "0x1: In reset\n"
		"0x0: Not in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xd1, 3, 1,
  }
};

/*
 * SMB CPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x23, I2C_CLIENT_END
};

/* SMBcpld id */
static const struct i2c_device_id smbcpld_id[] = {
  { "smbcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smbcpld_id);

#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 5, 0)
static int smbcpld_probe(struct i2c_client *client)
#else
static int smbcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
#endif
{
  i2c_dev_data_st *pdata;

  pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
  if (pdata == NULL)
    return -ENOMEM;

  i2c_set_clientdata(client, pdata);
  return devm_i2c_dev_sysfs_init(client, pdata, smbcpld_attr_table,
                                 ARRAY_SIZE(smbcpld_attr_table));
}

static struct i2c_driver smbcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smbcpld",
  },
  .probe    = smbcpld_probe,
  .id_table = smbcpld_id,
  .address_list = normal_i2c,
};

module_i2c_driver(smbcpld_driver);

MODULE_AUTHOR("Alla Alamsi");
MODULE_DESCRIPTION("MERU SMB CPLD Driver");
MODULE_LICENSE("GPL");
