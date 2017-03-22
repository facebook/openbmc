/*
 * scmcpld.c - The i2c driver for SCMCPLD
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
 */

//#define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif

static const i2c_dev_attr_st scmcpld_attr_table[] = {
  {
    "board_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 4,
  },
  {
    "cpld_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 6,
  },
  {
    "cpld_released",
    "0x0: not released\n"
	"0x1: released after PVT",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 6, 1,
  },
  {
    "cpld_sub_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2, 0, 8,
  },
  {
    "slotid",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3, 0, 4,
  },
  {
    "hcp_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4, 0, 2,
  },
  {
    "led_red",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 0, 1,
  },
  {
    "led_green",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 1, 1,
  },
  {
    "led_blue",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 2, 1,
  },
  {
    "led_blink",
    "0x0: no blink\n"
	"0x1: blink",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 3, 1,
  },
  {
    "led_test_en",
    "0x0: led controled by hw\n"
	"0x1: led controled by sw",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 4, 1,
  },
  {
    "module_present",
    "0x0: present\n"
	"0x1: not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 0, 1,
  },
  {
    "hotswap_pg_inv_sta",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xa, 0, 1,
  },
  {
    "rtc_clear_ctrl",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xb, 0, 1,
  },
  {
    "pwr_come_en",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 1,
  },
  {
    "pwr_come_force",
    "0x0: force power off COMe",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 1, 1,
  },
  {
    "pwr_syc_n",
    "0x0: trigger power cycling COMe",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 2, 1,
  },
  {
    "come_rst_n",
    "0x0: trigger COMe reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 0, 1,
  },
  {
    "come_sta",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 0, 4,
  },
  {
    "come_bios_dis0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 0, 1,
  },
  {
    "come_bios_dis1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 1, 1,
  },
  {
    "come_pwr_off_rec",
    "0x0: normal\n"
    "0x11: LC/FAB remove to off\n"
    "0x22: LC/FAB software control to off\n"
    "0x33: write to clear",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 8,
  },
  {
    "spi_sel",
    "0: spi flash accessed by COMe\n"
    "1: spi flash accessed by BMC",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 0, 1,
  },
  {
    "spi_buf_sel",
    "0x0: enable\n"
	"0x1: disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 0, 1,
  },
  {
    "smb_buf_en",
    "0x0: enable\n"
	"0x1: disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 1, 1,
  },
  {
    "i2c_buf_en",
    "0x0: enable\n"
	"0x1: disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 2, 1,
  },
  {
    "uart_mux_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 0, 1,
  },
  {
    "lpc_bridge_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 1,
  },
  {
    "cmm1_sta",
    "0x0: CMM1 is master\n"
	"0x1: CMM1 is slave",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 0, 1,
  },
  {
    "cmm2_sta",
    "0x0: CMM2 is master\n"
	"0x1: CMM2 is slave",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 1, 1,
  },
};

static i2c_dev_data_st scmcpld_data;

/*
 * SCMCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x3e, I2C_CLIENT_END
};

/* SCMCPLD id */
static const struct i2c_device_id scmcpld_id[] = {
  { "scmcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, scmcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int scmcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SCMCPLD
   */
  strlcpy(info->type, "scmcpld", I2C_NAME_SIZE);
  return 0;
}

static int scmcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(scmcpld_attr_table) / sizeof(scmcpld_attr_table[0]);
  i2c_dev_sysfs_data_init(client, &scmcpld_data,
					scmcpld_attr_table, n_attrs);
}

static int scmcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &scmcpld_data);
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
  .detect   = scmcpld_detect,
  .address_list = normal_i2c,
};

static int __init scmcpld_mod_init(void)
{
  return i2c_add_driver(&scmcpld_driver);
}

static void __exit scmcpld_mod_exit(void)
{
  i2c_del_driver(&scmcpld_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("SCMCPLD Driver");
MODULE_LICENSE("GPL");

module_init(scmcpld_mod_init);
module_exit(scmcpld_mod_exit);
