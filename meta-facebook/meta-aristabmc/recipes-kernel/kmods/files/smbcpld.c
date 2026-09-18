/*
 * Copyright 2026-present Facebook. All Rights Reserved.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "i2c_dev_sysfs.h"

/*
 * Switch-card/management-card CPLD register map.
 */
static const i2c_dev_attr_st smbcpld_attr_table[] = {
	{
		"cpld_ver_minor",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x00,
		0,
		8,
	},
	{
		"cpld_ver_major",
		NULL,
		I2C_DEV_ATTR_SHOW_DEFAULT,
		NULL,
		0x01,
		0,
		8,
	},
};

static const struct i2c_device_id smbcpld_id[] = {
	{ "smbcpld", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smbcpld_id);

static int smbcpld_probe(struct i2c_client *client)
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
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = "smbcpld",
	},
	.probe = smbcpld_probe,
	.id_table = smbcpld_id,
};
module_i2c_driver(smbcpld_driver);

MODULE_AUTHOR("Bianca Giocas <bgiocas@arista.com>");
MODULE_DESCRIPTION("FBOSS Aristabmc OpenBMC SMB-CPLD Driver");
MODULE_LICENSE("GPL");
