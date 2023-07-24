// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>	/* for ARRAY_SIZE */
#include <linux/module.h>
#include <linux/i2c.h>
#include "i2c_dev_sysfs.h"

#define buf_enable_help_str                     \
  "0: Buf enable\n"                             \
  "1: Buf disable"

#define write_protect_help_str                  \
  "0: Write enable\n"                           \
  "1: Write protect"                            \

static const i2c_dev_attr_st scmcpld_attrs[] = {
  {
    "board_type",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x00, 0, 4,
  },
  {
    "board_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x00, 4, 3,
  },
  {
    "golden_flag",
    "CPLD Configuration Image Flag\n"
    " 0: CPLD load external upgrade image\n"
    " 1: CPLD load internal golden image",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 7, 1,
  },
  {
    "cpld_major_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 7,
  },
  {
    "cpld_minor_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x02, 0, 8,
  },
  {
    "cpld_sub_ver",
    "used for HW debug only",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 0, 8,
  },
  {
    "software_scratch",
    "scratchpad for software use.\n"
    "shall reset default when got POR reset.\n"
    "default: 0xde",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x04, 0, 8,
  },
  {
    "bios_post_cmplt_l", // be used for PECI checking ready to used
    "COMe BIOS post code complete\n"
    "0: Normal\n"
    "1: Fail",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 6, 1,
  }
};

static const struct i2c_device_id scmcpld_id[] = {
	{ "scmcpld", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, scmcpld_id);

static int scmcpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	i2c_dev_data_st *pdata;

	pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;

	i2c_set_clientdata(client, pdata);

	return i2c_dev_sysfs_data_init(client, pdata, scmcpld_attrs,
				       ARRAY_SIZE(scmcpld_attrs));
}

static int scmcpld_remove(struct i2c_client *client)
{
	i2c_dev_data_st *pdata = i2c_get_clientdata(client);

	i2c_dev_sysfs_data_clean(client, pdata);
	return 0;
}

static struct i2c_driver scmcpld_driver = {
	.class    = I2C_CLASS_HWMON,
	.driver = {
		.name = "scmcpld",
	},
	.probe    = scmcpld_probe,
	.remove   = scmcpld_remove,
	.id_table = scmcpld_id,
};

module_i2c_driver(scmcpld_driver);

MODULE_AUTHOR("Sittisak Sinprem <ssinprem@celestica.com>");
MODULE_DESCRIPTION("SCMCPLD Driver");
MODULE_LICENSE("GPL");
