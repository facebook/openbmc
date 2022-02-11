/*
 * syscpld.c - The i2c driver for SYSCPLD
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

static const i2c_dev_attr_st syscpld_attr_table[] = {
  {
    "board_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 0, 4,
  },
  {
    "model_id",
    "0x0: wedge100\n"
    "0x1: 6-pack100 linecard\n"
    "0x2: 6-pack100 fabric card\n"
    "0x3: reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 4, 2,
  },
  {
    "cpld_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 0, 6,
  },
  {
    "cpld_released",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 6, 1,
  },
  {
    "cpld_sub_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    2, 0, 8,
  },
  {
    "slotid",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    3, 0, 5,
  },
  {
    "psu1_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    8, 0, 1,
  },
  {
    "psu2_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    8, 1, 1,
  },
  {
    "fan_rackmon_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    8, 2, 1,
  },
  {
    "micro_srv_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    8, 3, 1,
  },
  {
    "th_rov",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xb, 0, 4,
  },
  {
    "vcore_idsel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xb, 4, 3,
  },
  {
    "reset_reason",
    "0x00: default state (unknown)\n"
    "0x01: power domain standby reset\n"
    "0x02: power domain main reset\n"
    "0x03: front panel push button reset\n"
    "0x04: on board debug push button reset\n"
    "0x05: facebook debug header reset\n"
    "0x10: SW reg 0x31_bit4 hot reset\n"
    "0x11: SW reg 0x31_bit5 warm reset\n"
    "0x12: SW reg 0x31_bit6 cold reset\n"
    "0x13: SW reg 0x31_bit7 power reset\n"
    "0x20: BMC request reset, BMC only\n"
    "0x21: BMC request to reset tomahawk\n"
    "0x22: BMC request to reset micro-server\n"
    "0x23: BMC request to reset main power domain\n"
    "0x24: BMC request to reset all board\n"
    "0x25: BMC watchdog timer-1 reset\n"
    "0x26: BMC watchdog timer-2 reset\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xd, 0, 8,
  },
  {
    "pwr_stby_ok",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "vcore_vrdy1",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 0, 1,
  },
  {
    "vcore_hot",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 1, 1,
  },
  {
    "vanlog_vrdy1",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 2, 1,
  },
  {
    "vanlog_hot",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 3, 1,
  },
  {
    "v3v3_vrdy1",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 4, 1,
  },
  {
    "v3v3_hot",
    "0x0: Power not OK\n"
    "0x1: Power is OK\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 5, 1,
  },
  {
    "com-e_board_type",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x18, 0, 3,
  },
  {
    "com-e_gbe_link_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x18, 3, 1,
  },
  {
    "com-e_sus_stat_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x18, 4, 1,
  },
  {
    "com-e_sus_s3_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x18, 5, 1,
  },
  {
    "com-e_sus_s4_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x18, 6, 1,
  },
  {
    "com-e_sus_s5_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x18, 7, 1,
  },
  {
    "bb_scl_in_status",
    "Requires CPLD version 6.96 and up",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1F, 6, 1,
  },
  {
    "bb_sda_in_status",
    "Requires CPLD version 6.96 and up",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1F, 7, 1,
  },
  {
    "uart_mux",
    "0x0: UART_SELECT_BMC\n0x1: UART_SELECT_DBG\n"
    "0x2: Force to select 0\n0x3: Force to select 1\n\n"
    "UART_SEL\n"
    "1: micro-server console connected to BMC\n"
    "0: micro-server console connected to debug header",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x26, 0, 2,
  },
  {
    "tpm_lpc_rst_n",
    "0: reset LPC TPM\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 0, 1,
  },
  {
    "tpm_spi_rst_n",
    "0: reset SPI TPM\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 1, 1,
  },
  {
    "com-e_phy_rst_n",
    "0: reset COM-E PHY\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 2, 1,
  },
  {
    "lpc_rst_n",
    "0: reset BMC LPC bus\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 3, 1,
  },
  {
    "com-e_smbus_isobuf_en",
    "0: disabled\n"
    "1: enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 4, 1,
  },
  {
    "com-e_i2c_isobuf_en",
    "0: disabled\n"
    "1: enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 5, 1,
  },
  {
    "com-e_pwr_ok_en",
    "0: disabled\n"
    "1: enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 6, 1,
  },
  {
    "bmc_phy_wp",
    "0: enable write of BMC PHY EEPROM\n"
    "1: dsiable write of BMC PHY EEPROM",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 0, 1,
  },
  {
    "fp_phy_wp",
    "0: enable write of front PHY EEPROM\n"
    "1: dsiable write of front PHY EEPROM",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 1, 1,
  },
  {
    "com-e_spi_boot_wp_n",
    "0: enable write of COMe BIOS flash\n"
    "1: dsiable write of COMe BIOS flash",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 2, 1,
  },
  {
    "bb_scl_oe",
    "Requires CPLD version 6.96 and up\n"
    "0: bb_scl_out is not enabled to main_i2c_scl\n"
    "1: bb_scl_out is enabled and drive main_i2c_scl",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2C, 6, 1,
  },
  {
    "bb_sda_oe",
    "Requires CPLD version 6.96 and up\n"
    "0: bb_sda_out is not enabled to main_i2c_scl\n"
    "1: bb_sda_out is enabled and drive main_i2c_scl",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2C, 7, 1,
  },
  {
    "bb_scl_out",
    "Requires CPLD version 6.96 and up\n"
    "0: bb_scl_out is not enabled to main_i2c_scl\n"
    "1: bb_scl_out is enabled and drive main_i2c_scl",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2D, 6, 1,
  },
  {
    "bb_sda_out",
    "Requires CPLD version 6.96 and up\n"
    "0: bb_scl_out is not enabled to main_i2c_scl\n"
    "1: bb_scl_out is enabled and drive main_i2c_scl",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2D, 7, 1,
  },

  {
    "heart_attach_en",
    "0: no fan tray fatal error attack\n"
    "1: fan-tray fatal error attack mode enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2e, 7, 1,
  },
  {
    "dual_boot_en",
    "0: single boot\n"
    "1: dual boot",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2f, 0, 1,
  },
  {
    "2nd_flash_wp",
    "0: writeable\n"
    "1: write protected",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2f, 1, 1,
  },
  {
    "pwr_cyc_all_n",
    "0: power cycle all power\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 0, 1,
  },
  {
    "pwr_main_n",
    "0: main power is off\n"
    "1: main power is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 1, 1,
  },
  {
    "pwr_usrv_en",
    "0: micro-server power is off\n"
    "1: micro-server power is on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 2, 1,
  },
  {
    "pwr_usrv_btn_en",
    "0: micro-server power button is off\n"
    "1: micro-server power button is on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 3, 1,
  },
  {
    "usrv_rst_n",
    "0: write 0 to trigger micro-server reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 1,
  },
  {
    "th_sys_rst_n",
    "0: write 0 to trigger tomahawk system reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 1, 1,
  },
  {
    "th_pcie_rst_n",
    "0: write 0 to trigger tomahawk pcie reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 2, 1,
  },
  {
    "i2c_mux0_rst_n",
    "0: write 0 to trigger i2c mux0 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 0, 1,
  },
  {
    "i2c_mux1_rst_n",
    "0: write 0 to trigger i2c mux1 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 1, 1,
  },
  {
    "i2c_mux2_rst_n",
    "0: write 0 to trigger i2c mux2 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 2, 1,
  },
  {
    "i2c_mux3_rst_n",
    "0: write 0 to trigger i2c mux3 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 3, 1,
  },
  {
    "i2c_mux_psu_rst_n",
    "0: write 0 to trigger i2c mux psu reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 4, 1,
  },
  {
    "i2c_flush_en",
    "Requires CPLD version 6.96 and up\n"
    "Write 1, then write 0 to this bit to trigger i2c flush function\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 7, 1,
  },
  {
    "fan_rackmon_rst_n",
    "0: write 0 to trigger FAN_RACKMON card reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 0, 1,
  },
  {
    "usb2cp2112_rst_n",
    "0: write 0 to trigger USB bridge CP2112 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 1, 1,
  },
  {
    "usb_hub_rst_n",
    "0: write 0 to trigger USB hub reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 2, 1,
  },
  {
    "oob_bcm5387_rst_n",
    "0: write 0 to trigger OOB switch BCM5387 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 3, 1,
  },
  {
    "fp_phy_rst_n",
    "0: write 0 to trigger front port PHY reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 4, 1,
  },
  {
    "ob_phy_rst_n",
    "0: write 0 to trigger on board PHY reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 5, 1,
  },
  {
    "th_led_clr",
    "0: Tomahawk LED stream 0/1 normal\n"
    "1: Tomahawk LED stream 0/1 reset clear",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 0, 1,
  },
  {
    "th_led_en",
    "0: Tomahawk LED stream 0/1 disabled\n"
    "1: Tomahawk LED stream 0/1 enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 1, 1,
  },
  {
    "walk_test_en",
    "0: LED walk test disabled\n"
    "1: LED walk test enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 3, 1,
  },
  {
    "th_led_steam",
    "led number and color is decided by register 0x3d\n"
    "00: stream-0 single led check\n"
    "01: stream-1 single led check\n"
    "10: stream-0 all led check\n"
    "11: stream-1 all led check",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 4, 2,
  },
  {
    "led_test_blink_en",
    "0: normal constant\n"
    "1: blinking during LED test",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 6, 1,
  },
  {
    "led_test_mode_en",
    "0: normal\n"
    "1: port LED test mode enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 7, 1,
  },
};

static i2c_dev_data_st syscpld_data;

/*
 * SYSCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x31, I2C_CLIENT_END
};

/* SYSCPLD id */
static const struct i2c_device_id syscpld_id[] = {
  { "syscpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, syscpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int syscpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SYSCPLD
   */
  strlcpy(info->type, "syscpld", I2C_NAME_SIZE);
  return 0;
}

static int syscpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(syscpld_attr_table) / sizeof(syscpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &syscpld_data,
                                 syscpld_attr_table, n_attrs);
}

static int syscpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &syscpld_data);
  return 0;
}

static struct i2c_driver syscpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "syscpld",
  },
  .probe    = syscpld_probe,
  .remove   = syscpld_remove,
  .id_table = syscpld_id,
  .detect   = syscpld_detect,
  .address_list = normal_i2c,
};

static int __init syscpld_mod_init(void)
{
  return i2c_add_driver(&syscpld_driver);
}

static void __exit syscpld_mod_exit(void)
{
  i2c_del_driver(&syscpld_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("SYSCPLD Driver");
MODULE_LICENSE("GPL");

module_init(syscpld_mod_init);
module_exit(syscpld_mod_exit);
