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
#include <linux/module.h>
#include <linux/i2c.h>
#include <i2c_dev_sysfs.h>

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

static i2c_dev_data_st smb_debugcardcpld_data;

/*
 * SMB DEGUGCARDCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x27, I2C_CLIENT_END
};

/* SMB DEGUGCARDCPLD id */
static const struct i2c_device_id smb_debugcardcpld_id[] = {
  { "smb_debugcardcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smb_debugcardcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int smb_debugcardcpld_detect(struct i2c_client *client,
                              struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SMB DEGUGCARDCPLD
   */
  strlcpy(info->type, "smb_debugcardcpld", I2C_NAME_SIZE);
  return 0;
}

static int smb_debugcardcpld_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
   int n_attrs = sizeof(smb_debugcardcpld_attr_table) / sizeof(smb_debugcardcpld_attr_table[0]);
   return i2c_dev_sysfs_data_init(client, &smb_debugcardcpld_data,
                                 smb_debugcardcpld_attr_table, n_attrs);
}

static int smb_debugcardcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &smb_debugcardcpld_data);
  return 0;
}

static struct i2c_driver smb_debugcardcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smb_debugcardcpld",
  },
  .probe    = smb_debugcardcpld_probe,
  .remove   = smb_debugcardcpld_remove,
  .id_table = smb_debugcardcpld_id,
  .detect   = smb_debugcardcpld_detect,
  .address_list = normal_i2c,
};

static int __init smb_debugcardcpld_mod_init(void)
{
  return i2c_add_driver(&smb_debugcardcpld_driver);
}

static void __exit smb_debugcardcpld_mod_exit(void)
{
  i2c_del_driver(&smb_debugcardcpld_driver);
}

MODULE_AUTHOR("Facebook/Celestica");
MODULE_DESCRIPTION("SMB DEGUGCARDCPLD Driver");
MODULE_LICENSE("GPL");

module_init(smb_debugcardcpld_mod_init);
module_exit(smb_debugcardcpld_mod_exit);
