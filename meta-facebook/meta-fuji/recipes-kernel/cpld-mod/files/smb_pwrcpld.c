/*
 * smb_pwrcpld.c - The i2c driver for SMB PWRCPLD
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/version.h>

#include "i2c_dev_sysfs.h"

#define fail_normal_help_str                    \
  "0: Fail\n"                                   \
  "1: Normal"

static const i2c_dev_attr_st smb_pwrcpld_attr_table[] = {
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 8,
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
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 0, 8,
  },
  {
    "soft_scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 0, 8,
  },
  {
    "cpld_pdb_L_sim_on",
    "Enable - high active",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 0, 8,
  },
  {
    "cpld_psu1_on",
    "0: L1/R1 PSU1 POWER OFF\n"
    "1: L1/R1 PSU1 POWER ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 1,
  },
  {
    "cpld_psu2_on",
    "0: L2/R2 PSU2 POWER OFF\n"
    "1: L2/R2 PSU2 POWER ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 1, 1,
  },
  {
    "cpld_psu1_pg",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "cpld_psu2_pg",
    fail_normal_help_str,
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
  {
    "upgrade_run",
    "0: No upgrade\n"
    "1: Start the upgrade",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa0, 0, 1,
  },
  {
    "upgrade_data",
    "UPGRADE DATA",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa1, 0, 8,
  },
  {
    "almost_fifo_empty",
    "0: No upgrade  data fifo almost empty\n"
    "1: Upgrade data fifo almost empty",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 0, 1,
  },
  {
    "fifo_full",
    "0: No upgrade  data fifo full\n"
    "1: Upgrade data fifo full",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 1, 1,
  },
  {
    "fifo_empty",
    "0: No upgrade  data fifo empty\n"
    "1: Upgrade data fifo empty",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 2, 1,
  },
  {
    "almost_fifo_full",
    "0: No upgrade  data fifo almost full\n"
    "1: Upgrade data fifo almost full",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 3, 1,
  },
  {
    "done",
    "0: No upgrade done\n"
    "1: Upgrade done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 4, 1,
  },
  {
    "error",
    "0: No upgrade error\n"
    "1: Upgrade error",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 5, 1,
  },
  {
    "st_pgm_done_flag",
    "0: FSM No upgrade done\n"
    "1: FSM Upgrade done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 6, 1,
  },
  {
    "upgrade_refresh_r",
    "0: No update\n"
    "1: Start the upgrade refresh",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa3, 0, 1,
  }
};

/* SMB PWRCPLD id */
static const struct i2c_device_id smb_pwrcpld_id[] = {
  { "smb_pwrcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smb_pwrcpld_id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
static int smb_pwrcpld_probe(struct i2c_client *client)
#else
static int smb_pwrcpld_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
#endif
{
  i2c_dev_data_st *pdata;

  pdata = devm_kmalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
  if (pdata == NULL)
    return -ENOMEM;
  i2c_set_clientdata(client, pdata);

  return devm_i2c_dev_sysfs_init(client, pdata, smb_pwrcpld_attr_table,
                                 ARRAY_SIZE(smb_pwrcpld_attr_table));
}

static struct i2c_driver smb_pwrcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smb_pwrcpld",
  },
  .probe    = smb_pwrcpld_probe,
  .id_table = smb_pwrcpld_id,
};
module_i2c_driver(smb_pwrcpld_driver);

MODULE_AUTHOR("Xiaohua Wang");
MODULE_DESCRIPTION("SMB PWRCPLD Driver");
MODULE_LICENSE("GPL");
