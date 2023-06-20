/*
 * supcpld.c - The i2c driver for the CPLD on YAMP
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//#define DEBUG
// YAMPTODO: Define all the register desc and helps
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

static const i2c_dev_attr_st supcpld_attr_table[] = {
  {
    "cpld_ver_major",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 8,
  },
  {
    "cpld_ver_minor",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2, 0, 8,
  },
  {
    "scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x5, 0, 8,
  },
  {
    "flash_rate_upper",
    "Upper 8 bit of flash rate",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 8,
  },
  {
    "flash_rate_lower",
    "Lower 8 bit of flash rate",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 0, 8,
  },
  {
    "system_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 4, 1,
  },
  {
    "system_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 3, 1,
  },
  {
    "system_led_flash",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 0, 3,
  },
  {
    "fan_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 4, 1,
  },
  {
    "fan_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 3, 1,
  },
  {
    "fan_led_flash",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 0, 3,
  },
  {
    "psu_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 4, 1,
  },
  {
    "psu_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 3, 1,
  },
  {
    "psu_led_flash",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 3,
  },
  {
    "sc_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 4, 1,
  },
  {
    "sc_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 3, 1,
  },
  {
    "sc_led_flash",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 0, 3,
  },
  {
    "sfp_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 4, 1,
  },
  {
    "sfp_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 3, 1,
  },
  {
    "sfp_led_flash",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 0, 3,
  },
  {
    "beacon_len",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 0, 1,
  },
  {
    "switchcard_powergood",
    "0x1: Power is good\n"
    "0x0: Power is BAD",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "jtag_en",
    "0x1: Enable\n"
    "0x0: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "jtag_sel",
    "0x1: SCD\n"
    "0x0: LC",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "sfp_disable",
    "0x1: SFP TX Disabled\n"
    "0x0: SFP TX Enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 6, 1,
  },
  {
    "sfp_present_latch",
    "0x1: Status Changed (W0C)\n"
    "0x0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 5, 1,
  },
  {
    "sfp_tx_fault_latch",
    "0x1: TX Fault (W0C)\n"
    "0x0: No TX Fault",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 4, 1,
  },
  {
    "sfp_tx_los_latch",
    "0x1: TX LOS (W0C)\n"
    "0x0: NO TX LOS",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 3, 1,
  },
  {
    "sfp_present",
    "0x1: Present\n"
    "0x0: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "sfp_tx_fault",
    "0x1: Fault\n"
    "0x0: No Fault",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "sfp_tx_los",
    "0x1: LOS\n"
    "0x0: No LOS",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "chassis_power_cycle",
    "Write 0xDE to power-cycle the chassis",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x60, 0, 8,
  },
  {
    "bios_select",
    "0x1: Enable BIOS SPI access\n"
    "0x0: Disable BIOS SPI access",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x70, 0, 1,
  },
  {
    "cpu_control",
    "0x1: Latch all BMC CTRL signals to CPU\n"
    "0x0: Unlatch all BMC CTRL signals to CPU",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x72, 0, 1,
  },
  {
    "uart_selection",
    "Write 0xA5 to switch FP UART connection to CPU",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 0, 8,
  },
};

static i2c_dev_data_st supcpld_data;

/*
 * SUP CPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x43, I2C_CLIENT_END
};

/* YAMPCPLD id */
static const struct i2c_device_id supcpld_id[] = {
  { "supcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, supcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int supcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the YAMPCPLD
   */
  strlcpy(info->type, "supcpld", I2C_NAME_SIZE);
  return 0;
}

static int supcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(supcpld_attr_table) / sizeof(supcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &supcpld_data,
                                 supcpld_attr_table, n_attrs);
}

static void supcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &supcpld_data);
}

static struct i2c_driver supcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "supcpld",
  },
  .probe    = supcpld_probe,
  .remove   = supcpld_remove,
  .id_table = supcpld_id,
  .detect   = supcpld_detect,
  .address_list = normal_i2c,
};

static int __init supcpld_mod_init(void)
{
  return i2c_add_driver(&supcpld_driver);
}

static void __exit supcpld_mod_exit(void)
{
  i2c_del_driver(&supcpld_driver);
}

MODULE_AUTHOR("Mike Choi <mikechoi@fb.com>");
MODULE_DESCRIPTION("YAMP SUP CPLD Driver");
MODULE_LICENSE("GPL");

module_init(supcpld_mod_init);
module_exit(supcpld_mod_exit);
