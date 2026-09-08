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
static const i2c_dev_attr_st scmcpld_attrs[] = {
	{
		"cpld_ver",
		"",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x1,
		0,
		8,
	},
	{
		"cpld_minor_ver",
		"",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x2,
		0,
		8,
	},
	{
		"cpld_sub_ver",
		"",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x3,
		0,
		8,
	}
};

static const struct i2c_device_id scmcpld_id[] = {
	{ "scmcpld", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, scmcpld_id);

#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 5, 0)
static int scmcpld_probe(struct i2c_client *client)
#else
static int scmcpld_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
#endif
{
	i2c_dev_data_st *pdata;

	pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;

	i2c_set_clientdata(client, pdata);

	return i2c_dev_sysfs_data_init(client, pdata, scmcpld_attrs,
				       ARRAY_SIZE(scmcpld_attrs));
}

static void scmcpld_remove(struct i2c_client *client)
{
	i2c_dev_data_st *pdata = i2c_get_clientdata(client);

	i2c_dev_sysfs_data_clean(client, pdata);
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

MODULE_DESCRIPTION("FBOSS OpenBMC scmcpld Driver");
MODULE_LICENSE("GPL");