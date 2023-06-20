/*
 * scdcpld.c - The i2c driver for the SCD CPLD on YAMP
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

static const i2c_dev_attr_st scdcpld_attr_table[] = {
  {
    "cpld_ver_minor",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 8,
  },
  {
    "cpld_ver_major",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 8,
  },
  {
    "scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2, 0, 8,
  },
  {
    "jtag_ctrl",
    "JTAG target selection\n"
    "0xB-0xF  Unused\n"
    "0xA      Fan Card\n"
    "0x9      Unused\n"
    "0x8      LC8 , PIM8\n"
    "0x7      LC7 , PIM7\n"
    "0x6      LC6 , PIM6\n"
    "0x5      LC5 , PIM5\n"
    "0x4      LC4 , PIM4\n"
    "0x3      LC3 , PIM3\n"
    "0x2      LC2 , PIM2\n"
    "0x1      LC1 , PIM1\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xC, 0, 8,
  },
  {
    "fancpld_wp",
    "1: Write Protected (Default)\n"
    "0: Write Enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xD, 0, 1,
  },
  {
    "lc_smb_mux_rst",
    "0x1: Reset\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 0, 1,
  },
  {
    "scd_reset",
    "0x1: Reset SCD on PCIe intf to COMe CPU\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 0, 1,
  },
  {
    "th3_reset",
    "0x1: Reset\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 1,
  },
  {
    "th3_pci_reset",
    "0x1: Reset\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 1, 1,
  },
  {
    "scd_power_en",
    "0x1: Enable Switch Card Full Power Domain\n"
    "0x0: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x1A, 0, 1,
  },
  {
    "bcm_int_sta",
    "BMC Interrupt Status (Read Only)\n"
    "Bit7 SCD Fatal Error             (reg 0x28)\n"
    "Bit6 SCD Temp / Power Intr       (reg 0x24)\n"
    "Bit5 PSU status changed          (reg 0x51)\n"
    "Bit4 Fan Card Status             (reg 0x40)\n"
    "Bit3 LC/PIM Fatal Error Intr     (reg 0x3c)\n"
    "Bit2 LC/PIM DPM Alert Intr       (reg 0x38)\n"
    "Bit1 LC/PIM Temp Alert Intr      (reg 0x34)\n"
    "Bit0 LC/PIM Present Changed Intr (reg 0x31)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x20, 0, 8,
  },
  {
    "scd_temp_pwr_alert_int",
    "Switch Card Temp and Power Alert (Read Only)\n"
    "Bit7 Reserved\n"
    "Bit6 SCD IR35223 POS0V8_ANLG Alert\n"
    "Bit5 SCD IR35223 POS0V8_CORE Alert\n"
    "Bit4 SCD DPM Alert\n"
    "Bit3 Reserved\n"
    "Bit2 SCD POS0V8_ANLG Temp Alert\n"
    "Bit1 SCD POS0V8_CORE Temp Alert\n"
    "Bit0 SCD MAX 6581 Temp Alert",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x24, 0, 8,
  },
  {
    "scd_temp_pwr_alerat_mask",
    "Switch Card Temp and Power Alert Mask Register (Read and writeable)\n"
    "Bit7 Reserved\n"
    "Bit6 Mask SCD IR35223 POS0V8_ANLG Alert\n"
    "Bit5 Mask SCD IR35223 POS0V8_CORE Alert\n"
    "Bit4 Mask SCD DPM Alert\n"
    "Bit3 Reserved\n"
    "Bit2 Mask SCD POS0V8_ANLG Temp Alert\n"
    "Bit1 Mask SCD POS0V8_CORE Temp Alert\n"
    "Bit0 Mask SCD MAX 6581 Temp Alert",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 0, 8,
  },
  {
    "scd_fatal_error",
    "Switch Card Fatal Error (Read Only)\n"
    "Bit 1: Shadow Bus Parity Error from CPU to Switch\n"
    "Bit 0: Switch Card SCD Watchdog",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x28, 0, 2,
  },
  {
    "scd_fatal_error_mask",
    "Switch Card Fatal Error Mask Register(Read and Writable)\n"
    "Bit 1: Mask Shadow Bus Parity Error from CPU to Switch\n"
    "Bit 0: Mask Switch Card SCD Watchdog",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 0, 2,
  },
  {
    "lc1_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "lc2_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "lc3_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "lc4_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 3, 1,
  },
  {
    "lc5_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
  {
    "lc6_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 5, 1,
  },
  {
    "lc7_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 6, 1,
  },
  {
    "lc8_prsnt_sta",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 7, 1,
  },
  {
    "lc1_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 0, 1,
  },
  {
    "lc2_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 1, 1,
  },
  {
    "lc3_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 2, 1,
  },
  {
    "lc4_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 3, 1,
  },
  {
    "lc5_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 4, 1,
  },
  {
    "lc6_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 5, 1,
  },
  {
    "lc7_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 6, 1,
  },
  {
    "lc8_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 7, 1,
  },
  {
    "lc_present_mask",
    "LC Present interrupt mask. Bit0-7 corresponds to LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 8,
  },
  {
    "lc_temp_alert_sta",
    "LC temp sensor alert status. Bit 0-7 corresponds to LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 0, 8,
  },
  {
    "lc_temp_alert_mask",
    "LC temp sensor alert interrupt mask. Bit0-7 for LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 0, 8,
  },
  {
    "lc_dpm_alert_sta",
    "LC DPM alert status. Bit 0-7 corresponds to LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x38, 0, 8,
  },
  {
    "lc_dpm_alert_mask",
    "LC DPM alert interrupt mask. Bit0-7 for LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 0, 8,
  },
  {
    "lc_fatal_sta",
    "LC fatal error status. Bit 0-7 corresponds to LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3C, 0, 8,
  },
  {
    "lc_fatal_mask",
    "LC fatal error mask. Bit0-7 for LC1-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3D, 0, 8,
  },
  {
    "fan_card_status",
    "Bit3 : Fan card fatal error\n"
    "Bit2 : Fan card temp alert\n"
    "Bit1 : Fan card intr req\n"
    "Bit0 : Fan card present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x40, 0, 4,
  },
  {
    "fan_card_mask",
    "Maskout fan card status bit in interrupt generation\n"
    "Bit3 : Mask Fan card fatal error\n"
    "Bit2 : Mask Fan card temp alert\n"
    "Bit1 : Mask Fan card intr req\n"
    "Bit0 : Mask Fan card present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 0, 3,
  },
  {
    "psu_status",
    "Bit 7: PSU4 Input OK\n"
    "Bit 6: PSU3 Input OK\n"
    "Bit 5: PSU2 Input OK\n"
    "Bit 4: PSU1 Input OK\n"
    "Bit 3: PSU4 Present\n"
    "Bit 2: PSU3 Present\n"
    "Bit 1: PSU2 Present\n"
    "Bit 0: PSU1 Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 0, 8,
  },
  {
    "psu_change",
    "Value set to 1 when corresponding bit in psu_status changes\n"
    "(Clear on read)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 0, 8,
  },
  {
    "psu_mask",
    "Mask each bit of psu_status in interrupt generation.\n"
    "Bit definition is the same as psu_status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 8,
  },
  {
    "lc1_fpga_rev_major",
    "Linecard 1 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x81, 0, 8,
  },
  {
    "lc2_fpga_rev_major",
    "Linecard 2 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x82, 0, 8,
  },
  {
    "lc3_fpga_rev_major",
    "Linecard 3 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x83, 0, 8,
  },
  {
    "lc4_fpga_rev_major",
    "Linecard 4 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x84, 0, 8,
  },
  {
    "lc5_fpga_rev_major",
    "Linecard 5 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x85, 0, 8,
  },
  {
    "lc6_fpga_rev_major",
    "Linecard 6 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x86, 0, 8,
  },
  {
    "lc7_fpga_rev_major",
    "Linecard 7 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x87, 0, 8,
  },
  {
    "lc8_fpga_rev_major",
    "Linecard 8 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x88, 0, 8,
  },
  {
    "lc1_fpga_rev_minor",
    "Linecard 1 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x91, 0, 8,
  },
  {
    "lc2_fpga_rev_minor",
    "Linecard 2 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x92, 0, 8,
  },
  {
    "lc3_fpga_rev_minor",
    "Linecard 3 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x93, 0, 8,
  },
  {
    "lc4_fpga_rev_minor",
    "Linecard 4 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x94, 0, 8,
  },
  {
    "lc5_fpga_rev_minor",
    "Linecard 5 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x95, 0, 8,
  },
  {
    "lc6_fpga_rev_minor",
    "Linecard 6 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x96, 0, 8,
  },
  {
    "lc7_fpga_rev_minor",
    "Linecard 7 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x97, 0, 8,
  },
  {
    "lc8_fpga_rev_minor",
    "Linecard 8 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or linecard not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x98, 0, 8,
  },
};

static i2c_dev_data_st scdcpld_data;

/*
 * SCD CPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x23, I2C_CLIENT_END
};

/* SCDCPLD id */
static const struct i2c_device_id scdcpld_id[] = {
  { "scdcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, scdcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int scdcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the YAMPCPLD
   */
  strlcpy(info->type, "scdcpld", I2C_NAME_SIZE);
  return 0;
}

static int scdcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(scdcpld_attr_table) / sizeof(scdcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &scdcpld_data,
                                 scdcpld_attr_table, n_attrs);
}

static void scdcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &scdcpld_data);
}

static struct i2c_driver scdcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "scdcpld",
  },
  .probe    = scdcpld_probe,
  .remove   = scdcpld_remove,
  .id_table = scdcpld_id,
  .detect   = scdcpld_detect,
  .address_list = normal_i2c,
};

static int __init scdcpld_mod_init(void)
{
  return i2c_add_driver(&scdcpld_driver);
}

static void __exit scdcpld_mod_exit(void)
{
  i2c_del_driver(&scdcpld_driver);
}

MODULE_AUTHOR("Mike Choi <mikechoi@fb.com>");
MODULE_DESCRIPTION("YAMP SCD CPLD Driver");
MODULE_LICENSE("GPL");

module_init(scdcpld_mod_init);
module_exit(scdcpld_mod_exit);
