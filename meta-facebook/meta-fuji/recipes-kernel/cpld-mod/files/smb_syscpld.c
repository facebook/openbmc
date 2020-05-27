/*
 * smb_syscpld.c - The i2c driver for SMB SYSCPLD
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

#define present_help_str                        \
  "0: Present\n"                                \
  "1: Not present"

#define intr_help_str                           \
  "0: Interrupt active\n"                       \
  "1: No interrupt"

#define reset_help_str                          \
  "0: It is placed in reset state\n"            \
  "1: It is placed in normal operation state"

#define fail_normal_help_str                    \
  "0: Fail\n"                                   \
  "1: Normal"

#define sys_lock_help_str                       \
  "0: System reset signal to be normal\n"       \
  "1: Lock system reset signal to be high"

#define cpld_cpu_blk_help_str                   \
  "0: CPLD passes the interrupt to CPU\n"       \
  "1: CPLD blocks incoming the interrupt"

#define enable_help_str                         \
  "0: Disable\n"                                \
  "1: Enable"

/* fuji smb sysfile node */
static const i2c_dev_attr_st smb_syscpld_attr_table[] = {
  {
    "board_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x00, 0, 8,
  },
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
    "software_scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 0, 8,
  },
  {
    "psu_L_pwr_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 0, 1,
  },
  {
    "psu_R_pwr_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 1, 1,
  },
  {
    "psu1_acok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 2, 1,
  },
  {
    "psu2_ac_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 3, 1,
  },
  {
    "psu3_ac_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 4, 1,
  },
  {
    "psu4_ac_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 5, 1,
  },
  {
    "cpld_pdb9548_R_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 0, 1,
  },
  {
    "cpld_pdb9548_L_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 1, 1,
  },
  {
    "pca9548_1_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 2, 1,
  },
  {
    "pca9548_2_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 3, 1,
  },
  {
    "syscpld_bcm5387_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 4, 1,
  },
  {
    "bmc_lpcrst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 5, 1,
  },
  {
    "syscpld_bcm54616_1_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 6, 1,
  },
  {
    "bmc_reset_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 7, 1,
  },
  {
    "fcm2_pca9548_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 0, 1,
  },
  {
    "fcm1_pca9548_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 1, 1,
  },
  {
    "fcm2_cpld_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 2, 1,
  },
  {
    "fcm1_cpld_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 3, 1,
  },
  {
    "sys_cpld_fpga_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 5, 1,
  },
  {
    "cpld_usbphy_reset_b_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 6, 1,
  },
  {
    "syspld_si5391b_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 7, 1,
  },
  {
    "ucd90160a_gpi4_w",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 0, 1,
  },
  {
    "ucd90160a_gpi3_w",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 1, 1,
  },
  {
    "syscpld_usbphy_id",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 2, 1,
  },
  {
    "syscpld_usbpwr_en_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 3, 1,
  },
  {
    "ucd9016a_cntrl",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 4, 1,
  },
  {
    "tcxo_50mhz_en",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 5, 1,
  },
  {
    "th4_boot_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 6, 1,
  },
  {
    "xp1r8v_flash_en",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 7, 1,
  },
  {
    "pm40028_gpio_8_w",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 0, 1,
  },
  {
    "syscpld_reserve_9",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 1, 1,
  },
  {
    "syscpld_si53108_oe_1_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 4, 1,
  },
  {
    "syscpld_si53108_oe_3_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 5, 1,
  },
  {
    "syscpld_si53108_oe_4_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 6, 1,
  },
  {
    "r_si53362_1_oe",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 7, 1,
  },
  {
    "iso_pwr_btn_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 0, 1,
  },
  {
    "scm_power_enable_rst",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 1, 1,
  },
  {
    "debug_rst_btn_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 2, 1,
  },
  {
    "scm_cpld_rst",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 3, 1,
  },
  {
    "pm40028_boot_rst_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 4, 1,
  },
  {
    "fcm1_3r3v_en",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 5, 1,
  },
  {
    "fcm2_3r3v_en",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 6, 1,
  },
  {
    "bmc_usb2apwren",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 7, 1,
  },
  {
    "psu_alert_1_l_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "psu_alert_2_l_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 1, 1,
  },
  {
    "psu_alert_1_r_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "psu_alert_2_r_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 3, 1,
  },
  {
    "scm_present_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 3, 1,
  },

  {
    "psu_prnst_1_int_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 4, 1,
  },
  {
    "psu_prnst_2_int_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 5, 1,
  },
  {
    "psu_prnst_3_int_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 6, 1,
  },
  {
    "psu_prnst_4_int_n",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 7, 1,
  },
  {
    "lm75b_1_int_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 0, 1,
  },
  {
    "lm75b_2_int_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 1, 1,
  },
  {
    "lm75b_3_int_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 2, 1,
  },
  {
    "nvme_i2c_alert_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 4, 1,
  },
  {
    "pwrgd_pch_pwrok_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 5, 1,
  },
  {
    "scm_smb_alrt_n_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 6, 1,
  },
  {
    "scm_pwrok_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 7, 1,
  },
  {
    "psu_pwrok_1_status_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 1,
  },
  {
    "psu_pwrok_2_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 1, 1,
  },
  {
    "psu_acok_1_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 2, 1,
  },
  {
    "psu_acok_2_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 3, 1,
  },
  {
    "fan1_alarm_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 4, 1,
  },
  {
    "fan2_alarm_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 5, 1,
  },
  {
    "psu_acok_3_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 6, 1,
  },
  {
    "psu_ac0k_4_int",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 7, 1,
  },
  {
    "pim1_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 0, 1,
  },
  {
    "pim2_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 1, 1,
  },
  {
    "pim3_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 2, 1,
  },
  {
    "pim4_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 3, 1,
  },
  {
    "pim5_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 4, 1,
  },
  {
    "pim6_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 5, 1,
  },
  {
    "pim7_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 6, 1,
  },
  {
    "pim8_i2c_mux_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 7, 1,
  },
  {
    "pim1_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 0, 1,
  },
  {
    "pim2_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 1, 1,
  },
  {
    "pim3_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 2, 1,
  },
  {
    "pim4_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 3, 1,
  },
  {
    "pim5_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 4, 1,
  },
  {
    "pim6_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 5, 1,
  },
  {
    "pim7_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 6, 1,
  },
  {
    "pim8_pca9534_pwr",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 7, 1,
  },
  {
    "pim1_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 0, 1,
  },
  {
    "pim2_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 1, 1,
  },
  {
    "pim3_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 2, 1,
  },
  {
    "pim4_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 3, 1,
  },
  {
    "pim5_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 4, 1,
  },
  {
    "pim6_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 5, 1,
  },
  {
    "pim7_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 6, 1,
  },
  {
    "pim8_smb_pg",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x17, 7, 1,
  },
  {
    "pim1_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 0, 1,
  },
  {
    "pim2_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 1, 1,
  },
  {
    "pim3_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 2, 1,
  },
  {
    "pim4_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 3, 1,
  },
  {
    "pim5_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 4, 1,
  },
  {
    "pim6_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 5, 1,
  },
  {
    "pim7_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 6, 1,
  },
  {
    "pim8_hw_rst_l",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x18, 7, 1,
  },
  {
    "syscpld_si5391b_oe_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x19, 0, 1,
  },
  {
    "th4_sys_rst_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x19, 1, 1,
  },
  {
    "th4_pcie_perst_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x19, 2, 1,
  },
  {
    "syscpld_si53108_oe_0_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x19, 3, 1,
  },
  {
    "syscpld_pm40028_rst_n",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x19, 4, 1,
  },
  {
    "psu_alert_1_l_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "psu_alert_2_l_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "psu_alert_3_l_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "psu_alert_4_l_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 3, 1,
  },
  {
    "psu_prnst_1_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 4, 1,
  },
  {
    "psu_prnst_2_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 5, 1,
  },
  {
    "psu_prnst_3_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 6, 1,
  },
  {
    "psu_prnst_4_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 7, 1,
  },
  {
    "lm75b_1_int_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 1,
  },
  {
    "lm75b_2_int_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 1, 1,
  },
  {
    "lm75b_3_int_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 2, 1,
  },
  {
    "nvme_i2c_alert_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 4, 1,
  },
  {
    "pwrgd_pch_pwrok_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 5, 1,
  },
  {
    "scm_smb_alrt_N_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 6, 1,
  },
  {
    "scm_pwrok_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 7, 1,
  },
  {
    "psu_pwrok_L_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 0, 1,
  },
  {
    "psu_pwrok_R_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 1, 1,
  },
  {
    "psu_acok_1_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 2, 1,
  },
  {
    "psu_acok_2_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 3, 1,
  },
  {
    "fan1_alarm_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 4, 1,
  },
    {
    "fan2_alarm_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 5, 1,
  },
  {
    "psu_acok_3_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 6, 1,
  },
  {
    "psu_acok_4_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 7, 1,
  },
  {
    "syscpld_a_si53108_oe_0_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "syscpld_a_si53108_oe_1_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "syscpld_a_si53108_oe_2_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "syscpld_a_si53108_oe_3_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 3, 1,
  },
  {
    "syscpld_a_si53108_oe_4_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
  {
    "syscpld_a_si53108_oe_5_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 5, 1,
  },
  {
    "syscpld_a_si53108_oe_6_N",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 6, 1,
  },
  {
    "psu_alert_1_L_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 1,
  },
  {
    "psu_alert_2_L_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 1, 1,
  },
  {
    "psu_alert_3_L_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 2, 1,
  },
  {
    "psu_alert_4_L_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 3, 1,
  },
  {
    "scm_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 3, 1,
  },
  {
    "psu_prnst_1_N_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 4, 1,
  },
  {
    "psu_prnst_2_N_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 5, 1,
  },
  {
    "psu_prnst_3_N_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 6, 1,
  },
  {
    "psu_prnst_4_N_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 7, 1,
  },
  {
    "lm75b_1_int_N_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 0, 1,
  },
  {
    "lm75b_2_int_N_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 1, 1,
  },
  {
    "lm75b_3_int_N_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 2, 1,
  },
  {
    "nvme_i2c_alert_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 4, 1,
  },
  {
    "pwrgd_pch_alert_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 5, 1,
  },
  {
    "scm_smb_alrt_N_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 6, 1,
  },
  {
    "scm_pwrok_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 7, 1,
  },
  {
    "psu_pwrok_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 0, 1,
  },
  {
    "psu_pwrok_R_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 1, 1,
  },
  {
    "psu_acok1_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 2, 1,
  },
  {
    "psu_acok2_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 3, 1,
  },
  {
    "fan1_alarm_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 4, 1,
  },
  {
    "fan2_alarm_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 5, 1,
  },
  {
    "psu_acok_3_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 6, 1,
  },
  {
    "psu_acok_4_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 7, 1,
  },
  {
    "led_red",
    "0: All led red off\n"
    "1: All led red on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 0, 1,
  },
  {
    "led_blue",
    "0: All led blue off\n"
    "1: All led blue on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 1, 1,
  },
  {
    "led_green",
    "0: All led green off\n"
    "1: All led green on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 2, 1,
  },
  {
    "uart_selection",
    "00: using BMC_UART_SEL5 controls UART selection\n" 
    "01: using FB USB Debug Card's USB_UART_SEL controls UART selection\n"
    "10: force the FB USB debug UART connect to COMe's UART\n"
    "11: force COMe's UART connect to BMC UART-5 and FB USB debug UART connect to BMC UART-2",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3a, 0, 2,
  },
  {
    "debug_uart_selection",
    "00: Tomahawk UART-0\n" 
    "01: Tomahawk UART-2\n"
    "10: Tomahawk UART-3\n"
    "11: reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3a, 6, 2,
  },
  {
    "cpld_pm40028_spi_hold_N",
    "PM40028 SPI  HOLD",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 0, 1,
  },
  {
    "th4_qspi_hold_N",
    "TH4_QSPI  HOLD",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 1, 1,
  },
  {
    "cpld_th4_qspi_hold_N",
    "56990 SPI  HOLD",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 2, 1,
  },
  {
    "cpld_pm40028_spi_wp_N",
    "PM40028 SPI  WP",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 3, 1,
  },
  {
    "th4_qspi_wp_N",
    "TH4_QSPI  WP",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 4, 1,
  },
  {
    "cpld_th4_qspi_wp_N",
    "56990  SPI  WP",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 5, 1,
  },
  {
    "bmc_bios_wp_N",
    "COMe  SPI  WP",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 6, 1,
  },
  {
    "cpld_fpga_spi_wp_N",
    "IOB FPGA SPI  WP",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 7, 1,
  },
  {
    "fpga_init_N",
    "0: don't finish loading DOM FPGA image\n"
    "1: finsh loading DOM FPGA image",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x47, 0, 1,
  },
  {
    "fpga_program_N",
    "0: don't finish loading DOM FPGA image\n"
    "1: finsh loading DOM FPGA image",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x47, 1, 1,
  },
  {
    "fpga_done",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 2, 1,
  },
  {
    "spi1_sel",
    "00h: BMC SPI1 Select the bcm56990 Flash.\n"
    "01h: BMC Select the IOB FPGA Flash.\n"
    "02h: BMC Select the PM40028 Flash.\n"
    "03h: BMC Select the COMe Flash.\n"
    "Others: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 0, 3,
  },
  {
    "spi2_sel",
    "00h: BMC SPI2 Select the EEPROM Flash.\n"
    "01h: BMC Select the BCM5387 Flash.\n"
    "Others: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 0, 3,
  },
  {
    "fpga1_resered_1",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 0, 1,
  },
  {
    "fpga1_resered_2",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 1, 1,
  },
  {
    "fpga1_resered_3",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 2, 1,
  },
  {
    "fpga1_resered_4",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 3, 1,
  },
  {
    "fpga2_resered_1",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 4, 1,
  },
  {
    "fpga2_resered_2",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 5, 1,
  },
    {
    "fpga2_resered_3",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 6, 1,
  },
  {
    "fpga2_resered_4",
    "FPGA_CPLD input value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 7, 1,
  },
  {
    "fpga1_resered_1_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 0, 1,
  },
  {
    "fpga1_resered_2_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 1, 1,
  },
  {
    "fpga1_resered_3_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 2, 1,
  },
  {
    "fpga1_resered_4_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 3, 1,
  },
  {
    "fpga2_resered_1_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 4, 1,
  },
  {
    "fpga2_resered_2_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 5, 1,
  },
  {
    "fpga2_resered_3_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 6, 1,
  },
  {
    "fpga2_resered_4_output",
    "FPGA_CPLD output value",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x51, 7, 1,
  },
  {
    "fpga1_resered_1_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 0, 1,
  },
  {
    "fpga1_resered_2_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 1, 1,
  },
  {
    "fpga1_resered_3_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 2, 1,
  },
  {
    "fpga1_resered_4_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 3, 1,
  },
  {
    "fpga2_resered_1_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 4, 1,
  },
  {
    "fpga2_resered_2_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 5, 1,
  },
  {
    "fpga2_resered_3_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 6, 1,
  },
  {
    "fpga2_resered_4_en",
    "0: Output disable\n"
    "1: Output enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x52, 7, 1,
  },
  {
    "pim1_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 0, 1,
  },
  {
    "pim2_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 1, 1,
  },
  {
    "pim3_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 2, 1,
  },
  {
    "pim4_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 3, 1,
  },
  {
    "pim5_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 4, 1,
  },
  {
    "pim6_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 5, 1,
  },
  {
    "pim7_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 6, 1,
  },
  {
    "pim8_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 7, 1,
  },
  {
    "pim1_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 0, 1,
  },
  {
    "pim2_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 1, 1,
  },
  {
    "pim3_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 2, 1,
  },
  {
    "pim4_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 3, 1,
  },
  {
    "pim5_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 4, 1,
  },
  {
    "pim6_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 5, 1,
  },
  {
    "pim7_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 6, 1,
  },
  {
    "pim8_int_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x54, 7, 1,
  },
  {
    "pim1_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 0, 1,
  },
  {
    "pim2_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 1, 1,
  },
  {
    "pim3_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 2, 1,
  },
  {
    "pim4_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 3, 1,
  },
  {
    "pim5_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 4, 1,
  },
  {
    "pim6_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 5, 1,
  },
  {
    "pim7_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 6, 1,
  },
  {
    "pim8_present_L",
    "0: Present \n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x55, 7, 1,
  },
  {
    "th4_avs0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 0, 1,
  },
  {
    "th4_avs1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 1, 1,
  },
  {
    "th4_avs2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 2, 1,
  },
  {
    "th4_avs3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 3, 1,
  },
  {
    "th4_avs4",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 4, 1,
  },
  {
    "th4_avs5",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 5, 1,
  },
  {
    "th4_avs6",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 6, 1,
  },
  {
    "th4_avs7",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x56, 7, 1,
  },
  {
    "pm40028_gpio_6",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 0, 1,
  },
  {
    "pm40028_gpio_8_r",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 1, 1,
  },
  {
    "ucd90160a_gpi4_r",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 2, 1,
  },
  {
    "ucd90160a_gpi3_r",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 3, 1,
  },
  {
    "ucd90160a_gpio20",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 4, 1,
  },
  {
    "ucd90160a_gpio19",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 5, 1,
  },
  {
    "ucd90160a_gpio17",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 6, 1,
  },
  {
    "ucd90160A_alert_N",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x57, 7, 1,
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

static i2c_dev_data_st smb_syscpld_data;

/*
 * SMB SYSCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x3e, I2C_CLIENT_END
};

/* SMBCPLD id */
static const struct i2c_device_id smb_syscpld_id[] = {
  { "smb_syscpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smb_syscpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int smb_syscpld_detect(struct i2c_client *client,
                              struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SMB SYSCPLD
   */
  strlcpy(info->type, "smb_syscpld", I2C_NAME_SIZE);
  return 0;
}

static int smb_syscpld_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
   int n_attrs = sizeof(smb_syscpld_attr_table) / sizeof(smb_syscpld_attr_table[0]);
   return i2c_dev_sysfs_data_init(client, &smb_syscpld_data,
                                 smb_syscpld_attr_table, n_attrs);
}

static int smb_syscpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &smb_syscpld_data);
  return 0;
}

static struct i2c_driver smb_syscpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smb_syscpld",
  },
  .probe    = smb_syscpld_probe,
  .remove   = smb_syscpld_remove,
  .id_table = smb_syscpld_id,
  .detect   = smb_syscpld_detect,
  .address_list = normal_i2c,
};

static int __init smb_syscpld_mod_init(void)
{
  return i2c_add_driver(&smb_syscpld_driver);
}

static void __exit smb_syscpld_mod_exit(void)
{
  i2c_del_driver(&smb_syscpld_driver);
}

MODULE_AUTHOR("Xiaohua Wang");
MODULE_DESCRIPTION("SMB SYSCPLD Driver");
MODULE_LICENSE("GPL");

module_init(smb_syscpld_mod_init);
module_exit(smb_syscpld_mod_exit);
