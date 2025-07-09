/*
 * smb_debugcardcpld.c - The i2c driver for SMB DEGUGCARDCPLD
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/version.h>

#include "i2c_dev_sysfs.h"

static const i2c_dev_attr_st smb_debugcardcpld_attr_table[] = {
  {
    "debugcard_postcode",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x01, 0, 8,
  },
  {
    "debugcard_button",
    "0xfd: pwr pressed\n"
    "0xfe: rst pressed\n"
    "0x7f: uart pressed\n"
    "0xff: clear",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x03, 0, 8,
  }
};

/* SMB DEGUGCARDCPLD id */
static const struct i2c_device_id smb_debugcardcpld_id[] = {
  { "smb_debugcardcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smb_debugcardcpld_id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
static int smb_debugcardcpld_probe(struct i2c_client *client)
#else
static int smb_debugcardcpld_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
#endif
{
  i2c_dev_data_st *pdata;

  pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
  if (pdata == NULL)
    return -ENOMEM;
  i2c_set_clientdata(client, pdata);

  return devm_i2c_dev_sysfs_init(client, pdata, smb_debugcardcpld_attr_table,
                                 ARRAY_SIZE(smb_debugcardcpld_attr_table));
}

static struct i2c_driver smb_debugcardcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smb_debugcardcpld",
  },
  .probe    = smb_debugcardcpld_probe,
  .id_table = smb_debugcardcpld_id,
};
module_i2c_driver(smb_debugcardcpld_driver);

MODULE_AUTHOR("Facebook/Celestica");
MODULE_DESCRIPTION("SMB DEGUGCARDCPLD Driver");
MODULE_LICENSE("GPL");
