// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>	/* for ARRAY_SIZE */
#include <linux/module.h>
#include <linux/i2c.h>
#include "i2c_dev_sysfs.h"

static const i2c_dev_attr_st pwrcpld_attrs[] = {
	{
		"cpld_ver_minor",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x0,
		0,
		8,
	},
	{
		"cpld_ver_major",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x1,
		0,
		8,
	},
	{
		"power_cycle",
		"0xDE: Initiate chassis power cycle",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x70,
		0,
		8,
	},
	{
		"cpu_power_on",
		"0x1: cpu power on\n"
		"0x0: cpu power off",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x72,
		6,
		1,
	},
	{
		"cpu_in_reset",
		"0x1: cpu is in reset\n"
		"0x0: cpu is not in reset",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x72,
		5,
		1,
	},
	{
		"thermtrip_fault",
		"0x1: CPU thermtrip fault\n"
		"0x0: normal operation",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x72,
		4,
		1,
	},
	{
		"cpu_power_disabled",
		"0x1: CPU power is disabled\n"
		"0x0: CPU power is enabled",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x72,
		3,
		1,
	},
	{
		"cpu_cat_error",
		"0x1: had a catastrophic error\n"
		"0x0: CPU functioning normally",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x72,
		2,
		1,
	},
	{
		"cpu_ready",
		"0x1: CPU is ready\n"
		"0x0: CPU is NOT ready",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x72,
		1,
		1,
	},
	{
		"cpu_control",
		"Write 1: take CPU out of reset"
		"Write 0: put CPU into reset"
		"0x1: CPU is NOT in reset\n"
		"0x0: CPU is in reset",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x72,
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

	i2c_set_clientdata(client, pdata);

	return i2c_dev_sysfs_data_init(client, pdata, pwrcpld_attrs,
				       ARRAY_SIZE(pwrcpld_attrs));
}

static void pwrcpld_remove(struct i2c_client *client)
{
	i2c_dev_data_st *pdata = i2c_get_clientdata(client);

	i2c_dev_sysfs_data_clean(client, pdata);
}

static struct i2c_driver pwrcpld_driver = {
	.class    = I2C_CLASS_HWMON,
	.driver = {
		.name = "pwrcpld",
	},
	.probe    = pwrcpld_probe,
	.remove   = pwrcpld_remove,
	.id_table = pwrcpld_id,
};

module_i2c_driver(pwrcpld_driver);

MODULE_AUTHOR("Adam Calabrigo <adamc@arista.com>");
MODULE_DESCRIPTION("FBOSS Meru OpenBMC Power-CPLD Driver");
MODULE_LICENSE("GPL");
