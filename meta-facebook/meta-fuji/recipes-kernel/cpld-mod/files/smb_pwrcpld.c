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

static i2c_dev_data_st smb_pwrcpld_data;

/*
 * SMB PWRCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x60, I2C_CLIENT_END
};

/* SMB PWRCPLD id */
static const struct i2c_device_id smb_pwrcpld_id[] = {
  { "smb_pwrcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smb_pwrcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int smb_pwrcpld_detect(struct i2c_client *client,
                              struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SMB PWRCPLD
   */
  strlcpy(info->type, "smb_pwrcpld", I2C_NAME_SIZE);
  return 0;
}

static int smb_pwrcpld_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
  int n_attrs = sizeof(smb_pwrcpld_attr_table) / sizeof(smb_pwrcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &smb_pwrcpld_data,
                                 smb_pwrcpld_attr_table, n_attrs);
}

static int smb_pwrcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &smb_pwrcpld_data);
  return 0;
}

static struct i2c_driver smb_pwrcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smb_pwrcpld",
  },
  .probe    = smb_pwrcpld_probe,
  .remove   = smb_pwrcpld_remove,
  .id_table = smb_pwrcpld_id,
  .detect   = smb_pwrcpld_detect,
  .address_list = normal_i2c,
};

static int __init smb_pwrcpld_mod_init(void)
{
  return i2c_add_driver(&smb_pwrcpld_driver);
}

static void __exit smb_pwrcpld_mod_exit(void)
{
  i2c_del_driver(&smb_pwrcpld_driver);
}

MODULE_AUTHOR("Xiaohua Wang");
MODULE_DESCRIPTION("SMB PWRCPLD Driver");
MODULE_LICENSE("GPL");

module_init(smb_pwrcpld_mod_init);
module_exit(smb_pwrcpld_mod_exit);
