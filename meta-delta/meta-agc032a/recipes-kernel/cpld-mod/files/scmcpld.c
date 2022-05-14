/*
 * scmcpld.c - The i2c driver for SCMCPLD
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

static const i2c_dev_attr_st scmcpld_attr_table[] = {
  {
    "board_ver",
    "000: R0A\n"
    "001: R0B\n"
    "010: R0C\n"
    "011: R01\n"
    "100: R02\n"
    "101: R03\n"
    "110: PVT1\n"
    "111: PVT2",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 3,
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
    "iso_com_brg_wdt",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0c, 0, 2,
  },
  {
    "iso_com_rst_n",
    "0: write 0 to trigger COMe reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 1,
  },
  {
    "cpld_com_phy_rst_n",
    "0: write 0 to trigger BCM54616S reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 1, 1,
  },
  {
    "i2c_mux_rst_1_n",
    "0: write 0 to trigger PCA9548 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 2, 1,
  },
  {
    "perst_n",
    "0: write 0 to trigger M.2 reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 3, 1,
  },
  {
    "iso_smb_cb_reset_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 4, 1,
  },
  {
    "come_pwr_btn_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 5, 1,
  },
  {
    "iso_com_sus_s3_n",
    "COMe Module SUS_S3_N Status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "iso_com_sus_s4_n",
    "COMe Module SUS_S4_N Status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 1, 1,
  },
  {
    "iso_com_sus_s5_n",
    "COMe Module SUS_S5_N Status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "iso_com_sus_stat_n",
    "COMe Module SUS_STAT_N Status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 3, 1,
  },
  {
    "iso_buff_brg_com_bios_dis0_n",
    "Control COMe BIOS DIS0",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 0, 1,
  },
  {
    "iso_buff_brg_com_bios_dis1_n",
    "Control COMe BIOS DIS1",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 1, 1,
  },
  {
    "com_exp_pwr_enable",
    "0: COMe power is OFF\n"
    "1: COMe power is ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 1,
  },
  {
    "com_exp_pwr_force_off",
    "0: COMe power is OFF\n"
    "1: COMe power is ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 1, 1,
  },
  {
    "com_exp_pwr_cycle",
    "Write 0 to this bit will trigger CPLD power cycling the COMe Module, This bit will auto set to 1 after Power Cycle finish",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 2, 1,
  },
  {
    "hs_fault_n",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 0, 1,
  },
  {
    "hs_alert1",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 1, 1,
  },
  {
    "hs_alert2",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 2, 1,
  },
  {
    "hostwap_pg",
    "The Status of HOTSWAP_PG",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 3, 1,
  },
  {
    "lm75b_int_n",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 4, 1,
  },
  {
    "bcm54616s_int_n",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 5, 1,
  },
  {
    "hs_fault_n_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 0, 1,
  },
  {
    "hs_alert1_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 1, 1,
  },
  {
    "hs_alert2_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 2, 1,
  },
  {
    "hotswap_pg_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 3, 1,
  },
  {
    "lm75b_int_n_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 4, 1,
  },
  {
    "bcm54616s_int_n_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 5, 1,
  },
  {
    "xp3r3v_ssd_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "xp1r8v_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "xp5r0v_come_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "xp12r0v_come_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 3, 1,
  },
  {
    "pwrgd_pch_pwrok",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
   {
    "come_pwrok",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
  {
    "xp3r3v_ssd_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 1,
  },
  {
    "xp1r8v_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 1, 1,
  },
  {
    "xp5r0v_come_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 2, 1,
  },
  {
    "xp12r0v_come_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 3, 1,
  },
  {
    "io_buf_come_3v3_oe_n",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 1,
  },
  {
    "io_buf_3v3_scm_smb_oe_n",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 1, 1,
  },
  {
    "come_usb_buf_oe_n",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 2, 1,
  },
  {
    "i2c1_buf_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 3, 1,
  },
  {
    "iso_brg_thrmtrip_n",
    "Indicating that the CPU has entered thermal shutdown",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 0, 1,
  },
  {
    "rtc_clear",
    "0: Clear CMOS\n"
    "1: Normal work",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 0, 1,
  },
  {
    "come_gpio",
    "0: Disbale\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 1, 1,
  },
  {
    "batlow_n",
    "0: Battery is low\n"
    "1: Battery is normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x36, 0, 1,
  },
  {
    "nvme_ssd_clkreq_n",
    "0: Normal operation\n"
    "1: NVME SSD Req",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x36, 3, 1,
  },
  {
    "iso_hitless_en",
    "0: Normal operation\n"
    "1: Hitless ongoing",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x36, 4, 1,
  },
  {
    "come_gpo_0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 0, 1,
  },
  {
    "come_type0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 2, 1,
  },
  {
    "come_type1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 3, 1,
  },
  {
    "come_type2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 4, 1,
  },
  {
    "bcm54616_phy_eeprom_wp",
    "0: Write enable\n"
    "1: Write protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 0, 1,
  },
  {
    "scm_eeprom_wp",
    "0: Write enable\n"
    "1: Write protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 1, 1,
  },
  {
    "pcie_clk_buffer_dif0_oe_n",
    "0: Buffer enable\n"
    "1: Buffer disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 0, 1,
  },
  {
    "pcie_clk_buffer_dif1_oe_n",
    "0: Buffer enable\n"
    "1: Buffer disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 1, 1,
  },
  {
    "pcie_clk_buffer_dif2_oe_n",
    "0: Buffer enable\n"
    "1: Buffer disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 2, 1,
  },
  {
    "pcie_clk_buffer_dif3_oe_n",
    "0: Buffer enable\n"
    "1: Buffer disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 3, 1,
  },
  {
    "sus_s3_n_clk_buffer_pwrd_n",
    "0: Power down\n"
    "1: Power on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 4, 1,
  },
  {
    "cb_gbe0_link100_n",
    "Link speed on 100M",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 0, 1,
  },
  {
    "cb_gbe0_link1000_n",
    "Link speed on 1G",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 1, 1,
  },
  {
    "cb_gbe0_act_n",
    "Link active",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 2, 1,
  },
  {
    "b_com_type1",
    "Module Type Descriptions",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 3, 1,
  },
  {
    "b_com_type2",
    "Module Type Descriptions",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 4, 1,
  },
};

static i2c_dev_data_st scmcpld_data;

/*
 * SCMCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x35, I2C_CLIENT_END
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
  return i2c_dev_sysfs_data_init(client, &scmcpld_data,
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

MODULE_AUTHOR("Siyu Li");
MODULE_DESCRIPTION("SCMCPLD Driver");
MODULE_LICENSE("GPL");

module_init(scmcpld_mod_init);
module_exit(scmcpld_mod_exit);
