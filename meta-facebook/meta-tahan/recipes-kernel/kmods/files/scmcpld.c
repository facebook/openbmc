// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>	/* for ARRAY_SIZE */
#include <linux/module.h>
#include <linux/i2c.h>
#include "i2c_dev_sysfs.h"

#define COME_PWR_CTRL_REG 		 0x14
#define COME_PWR_CTRL_REG_DEFVAL 0x7

/*
 * NOTE: ONLY export register fields that are required from user space.
 */
static const i2c_dev_attr_st scmcpld_attrs[] = {
	{
		"board_id",
		"Meta Networking System Board Type:\n"
		" 0x1: NW TH5 Blade System",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x00,
		0,
		4,
	},
	{
		"version_id",
		"NW TH5 Blade SCM Board revision:\n"
		" 0x0: EVT-1",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x00,
		4,
		4,
	},
	{
		"cpld_major_ver",
		"CPLD MAJOR Revision",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x01,
		0,
		7,
	},
	{
		"cpld_minor_ver",
		"CPLD MINOR Revision",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x02,
		0,
		8,
	},
	{
		"pwr_force_off",
		"use to make COMe power off when set to 0\n"
		" 0: COMe power is off\n"
		" 1: COMe power is on",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x14,
		1,
		1,
	},
	{
		"pwr_cyc_n",
		"write 0 to trigger CPLD power cycling COMe\n"
		"then this bit will auto set to 1 after Power cycle finish",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x14,
		2,
		1,
	},
  {
		"iob_fpga_program",
		" 0x00: Re-init\n"
		" 0x01: Normal\n",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x18,
		2,
		1,
	},
	{
		"spi_select",
		" 0x00: BMC SPI to IOB FPGA FLASH\n"
		" 0x01: BMC SPI to COMe\n"
		" 0x02: BMC SPI to PROT\n",
		I2C_DEV_ATTR_SHOW_DEFAULT,
		I2C_DEV_ATTR_STORE_DEFAULT,
		0x41,
		0,
		8,
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
	int ret = 0;
	i2c_dev_data_st *pdata;
	pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;

	/* Set pwr_come_en to 1 simpler to used, use only pwr_force_off to power ctrl */  
	ret = i2c_smbus_write_byte_data(client,
									COME_PWR_CTRL_REG,
									COME_PWR_CTRL_REG_DEFVAL);
	if (ret < 0) {
		dev_err(&client->dev, "Error writing in COMe PWR register\n");
		return ret;
	}

	return i2c_dev_sysfs_data_init(client, pdata, scmcpld_attrs,
				       ARRAY_SIZE(scmcpld_attrs));
}

static void scmcpld_remove(struct i2c_client *client)
{
	i2c_dev_data_st *pdata = i2c_get_clientdata(client);

	i2c_dev_sysfs_data_clean(client, pdata);
}

static struct i2c_driver scmcpld_driver = {
	.driver = {
		.name = "scmcpld",
	},
	.probe    = scmcpld_probe,
	.remove   = scmcpld_remove,
	.id_table = scmcpld_id,
};

module_i2c_driver(scmcpld_driver);

MODULE_AUTHOR("Tao Ren <taoren@meta.com>");
MODULE_DESCRIPTION("FBOSS OpenBMC SCM-CPLD Driver");
MODULE_LICENSE("GPL");
