// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>	/* for ARRAY_SIZE */
#include <linux/module.h>
#include <linux/i2c.h>
#include "i2c_dev_sysfs.h"

/*
 * NOTE: ONLY export register fields that are required from user space.
 */
static const i2c_dev_attr_st pwrcpld_attrs[] = {
	/*
	 * Board/firmware revision ID registers (read-only).
	 */
	{
		"board_id",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x0,
		0,
		4,
	},
	{
		"version_id",
		NULL,
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
		"cpld_minor_ver",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x2,
		0,
		8,
	},

	/*
	 * Chassis power cycle (register 0x23)
	 */
	{
		"power_cycle_go",
		"0: No power cycle\n"
		"1: Start the power cycle",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x23,
		0,
		1,
	},
};

static const struct i2c_device_id pwrcpld_id[] = {
	{ "pwrcpld", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, pwrcpld_id);

static int pwrcpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	i2c_dev_data_st *pdata;

	pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;


	return i2c_dev_sysfs_data_init(client, pdata, pwrcpld_attrs,
				       ARRAY_SIZE(pwrcpld_attrs));
}

static void pwrcpld_remove(struct i2c_client *client)
{
	i2c_dev_data_st *pdata = i2c_get_clientdata(client);

	i2c_dev_sysfs_data_clean(client, pdata);
}

static struct i2c_driver pwrcpld_driver = {
	.driver = {
		.name = "pwrcpld",
	},
	.probe    = pwrcpld_probe,
	.remove   = pwrcpld_remove,
	.id_table = pwrcpld_id,
};

module_i2c_driver(pwrcpld_driver);

MODULE_AUTHOR("Tao Ren <taoren@meta.com>");
MODULE_DESCRIPTION("FBOSS OpenBMC Power-CPLD Driver");
MODULE_LICENSE("GPL");
