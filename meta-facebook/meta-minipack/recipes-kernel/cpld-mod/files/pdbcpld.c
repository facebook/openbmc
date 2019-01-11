/*
 * pdbcpld.c - The i2c driver for PDBCPLD
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

static const i2c_dev_attr_st pdbcpld_attr_table[] = {
  {
    "board_ver",
    "0x0: R0A\n"
    "0x1: R0B\n"
    "0x2: R0C\n"
    "0x3: R01",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 2,
  },
  {
    "board_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 4, 3,
  },
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 6,
  },
  {
    "cpld_released",
    "0: Not released\n"
    "1: Released version after PVT",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 6, 1,
  },
  {
    "cpld_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x02, 0, 8,
  },
  {
    "cpld_pdb_psu1_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 1,
  },
  {
    "cpld_pdb_psu2_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 1, 1,
  },
  {
    "smb_psu_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "smb_psu_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 1, 1,
  },
  {
    "psu1_pwr_ok",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "psu2_pwr_ok",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 3, 1,
  },
  {
    "timer_base_10ms",
    "Timer base 10ms",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "timer_base_100ms",
    "Timer base 100ms",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "timer_base_1s",
    "Timer base 1s",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "timer_base_10s",
    "Timer base 10s",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 3, 1,
  },
  {
    "timer_counter_setting",
    "This timer is used for power up automatically,\n"
    "When counter down to zero, the power will re-power up.\n"
    "(Note: This value needs 0x23[1] to update)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 0, 8,
  },
  {
    "timer_counter_state",
    "This timer state",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x22, 0, 8,
  },
  {
    "power_cycle_go",
    "0: No power cycle\n"
    "1: Start the power cycle",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 0, 1,
  },
  {
    "timer_counter_setting_update",
    "0: No update\n"
    "1: Update the 0x21 TIMER_COUNTER_SETTING",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 1, 1,
  },
};

static i2c_dev_data_st pdbcpld_data;

/*
 * PDBCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x60, I2C_CLIENT_END
};

/* PDBCPLD id */
static const struct i2c_device_id pdbcpld_id[] = {
  { "pdbcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, pdbcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int pdbcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the PDBCPLD
   */
  strlcpy(info->type, "pdbcpld", I2C_NAME_SIZE);
  return 0;
}

static int pdbcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(pdbcpld_attr_table) / sizeof(pdbcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &pdbcpld_data,
                                 pdbcpld_attr_table, n_attrs);
}

static int pdbcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &pdbcpld_data);
  return 0;
}

static struct i2c_driver pdbcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "pdbcpld",
  },
  .probe    = pdbcpld_probe,
  .remove   = pdbcpld_remove,
  .id_table = pdbcpld_id,
  .detect   = pdbcpld_detect,
  .address_list = normal_i2c,
};

static int __init pdbcpld_mod_init(void)
{
  return i2c_add_driver(&pdbcpld_driver);
}

static void __exit pdbcpld_mod_exit(void)
{
  i2c_del_driver(&pdbcpld_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("PDBCPLD Driver");
MODULE_LICENSE("GPL");

module_init(pdbcpld_mod_init);
module_exit(pdbcpld_mod_exit);
