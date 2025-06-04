// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>	/* for ARRAY_SIZE */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include "i2c_dev_sysfs.h"

/*
 * NOTE: ONLY export register fields that are required from user space.
 */
static const i2c_dev_attr_st mcbcpld_attrs[] = {
	{
		"board_id",
		"Meta Networking system board type:\n"
		" 0x7 : Santabarbara\n"
		" 0x8 : Icecube\n"
		" 0x9 : Icetray",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x0,
		0,
		4,
	},
	{
		"version_id",
		"Board revision:\n"
		" 0x00 Pre-EVT & EVT1\n"
		" 0x01 EVT-2A\n"
		" 0x02 EVT-2B/C\n"
		" 0x03 DVT-1A\n"
		" 0x04 DVT-1B\n"
		" 0x05 PPVT\n"
		" 0x06 PVT\n"
		" 0x07 MP\n",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x0,
		4,
		4,
	},
	{
		"cpld_ver",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x1,
		0,
		7,
	},
	{
		"golden_flag",
		"0: CPLD load external upgrade image\n"
		"1: CPLD load internal golden image",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x1,
		7,
		1,
	},
	{
		"cpld_minor_ver",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x2,
		0,
		8,
	},
	{
		"cpld_sub_ver",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x3,
		0,
		8,
	},
	{
		"iob_flash_wp_l",
		"Control IOB FPGA SPI flash write protect\n"
		"1: Not protect\n"
		"0: Protect",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x6,
		1,
		1,
	},
	{
		"power_cycle_go",
		"0: No power cycle\n"
		"1: Start the power cycle",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x23,
		0,
		1,
	}
};

static const struct i2c_device_id mcbcpld_id[] = {
	{ "mcbcpld", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mcbcpld_id);

#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 5, 0)
static int mcbcpld_probe(struct i2c_client *client)
#else
static int mcbcpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
#endif
{
	i2c_dev_data_st *pdata;

	pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;

	i2c_set_clientdata(client, pdata);

	return i2c_dev_sysfs_data_init(client, pdata, mcbcpld_attrs,
				       ARRAY_SIZE(mcbcpld_attrs));
}

static void mcbcpld_remove(struct i2c_client *client)
{
	i2c_dev_data_st *pdata = i2c_get_clientdata(client);

	i2c_dev_sysfs_data_clean(client, pdata);
}

static struct i2c_driver mcbcpld_driver = {
	.class    = I2C_CLASS_HWMON,
	.driver = {
		.name = "mcbcpld",
	},
	.probe    = mcbcpld_probe,
	.remove   = mcbcpld_remove,
	.id_table = mcbcpld_id,
};

module_i2c_driver(mcbcpld_driver);

MODULE_DESCRIPTION("FBOSS OpenBMC mcbcpld Driver");
MODULE_LICENSE("GPL");