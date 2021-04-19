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

/* Cloudripper smb cpld sysfile node */
static const i2c_dev_attr_st smb_syscpld_attr_table_gb[] = {
  {
    "board_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 3,
  },
  {
    "board_type",
    "0: Cloudripper\n"
    "1: Cloudripper",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 4, 2,
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
    "psu1_dc_power_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 0, 1,
  },
  {
    "psu2_dc_power_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 1, 1,
  },
  {
    "psu1_ac_input_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 2, 1,
  },
  {
    "psu2_ac_input_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 3, 1,
  },
  {
    "mac_reset_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 0, 1,
  },
  {
    "mac_pcie_perst_l",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 1, 1,
  },
  {
    "bcm5389_resetb_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 2, 1,
  },
  {
    "bmc_phy1_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 3, 1,
  },
  {
    "bmc_phy2_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 4, 1,
  },
  {
    "bmc_lpc_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 5, 1,
  },
  {
    "usbhub_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 6, 1,
  },
  {
    "si5391b_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 7, 1,
  },
  {
    "pca9548a_2_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 0, 1,
  },
  {
    "pca9535_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 1, 1,
  },
  {
    "pca9534_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 2, 1,
  },
  {
    "fcb_pca9548_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 3, 1,
  },
  {
    "fcb_cpld_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 4, 1,
  },
  {
    "scm_cpld_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 5, 1,
  },
  {
    "tpm_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 6, 1,
  },
  {
    "ft323_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 7, 1,
  },
  {
    "dom_fpga1_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 0, 1,
  },
  {
    "dom_fpga2_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 1, 1,
  },
  {
    "temp_sensor_cpld_alert_1_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 0, 1,
  },
  {
    "temp_sensor_cpld_alert_2_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 1, 1,
  },
  {
    "temp_sensor_cpld_alert_3_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 2, 1,
  },
  {
    "temp_sensor_cpld_alert_4_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 3, 1,
  },
  {
    "fcb_cpld_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 4, 1,
  },
  {
    "scm_cpld_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 5, 1,
  },
  {
    "psu_alert_1_L_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 6, 1,
  },
  {
    "psu_alert_2_L_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 7, 1,
  },
  {
    "tpm_pp_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "mac_cpld_pcie_intr_L_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 1, 1,
  },
  {
    "smb_tpm_intr_N_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "mac_cpld_pcie_wake_L_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 3, 1,
  },
  {
    "debug_card_present_N_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 4, 1,
  },
  {
    "scm_present_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 5, 1,
  },
  {
    "psu_present_1_n_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 6, 1,
  },
  {
    "psu_present_2_n_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 7, 1,
  },
  {
    "psu_dc_power_ok_1_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 0, 1,
  },
  {
    "psu_dc_power_ok_2_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 1, 1,
  },
  {
    "psu_ac_power_ok_1_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 2, 1,
  },
  {
    "psu_ac_power_ok_2_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 3, 1,
  },
  {
    "gb_power_ok_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 4, 1,
  },
  {
    "bmc_power_ok_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 5, 1,
  },
  {
    "xp5rov_power_ok_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 6, 1,
  },
  {
    "fault_xp5rov_usb_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 7, 1,
  },
  {
    "temp_sensor_cpld_alert_1_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "temp_sensor_cpld_alert_2_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "temp_sensor_cpld_alert_3_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "temp_sensor_cpld_alert_4_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 3, 1,
  },
  {
    "fcb_cpld_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 4, 1,
  },
  {
    "scm_cpld_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 5, 1,
  },
  {
    "psu_alert_1_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 6, 1,
  },
  {
    "psu_alert_2_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 7, 1,
  },
  {
    "tpm_pp_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 0, 1,
  },
  {
    "mac_cpld_pcie_intr_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 1, 1,
  },
  {
    "smb_tpm_init_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 2, 1,
  },
  {
    "mac_cpld_pcie_wake_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 3, 1,
  },
  {
    "debug_card_present_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 4, 1,
  },
  {
    "scm_present_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 5, 1,
  },
  {
    "psu_present_1_N_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 6, 1,
  },
  {
    "psu_present_2_N_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 7, 1,
  },
  {
    "psu_dc_power_ok_1_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 1,
  },
  {
    "psu_dc_power_ok_2_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 1, 1,
  },
  {
    "psu_ac_power_ok_1_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 2, 1,
  },
  {
    "psu_ac_power_ok_2_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 3, 1,
  },
  {
    "gb_power_ok_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 4, 1,
  },
  {
    "bmc_power_ok_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 5, 1,
  },
  {
    "xp5rov_power_good_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 6, 1,
  },
  {
    "fault_R_xp5rov_usb_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 7, 1,
  },
  {
    "temp_sensor_cpld_alert_1_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "temp_sensor_cpld_alert_2_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "temp_sensor_cpld_alert_3_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "temp_sensor_cpld_alert_4_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 3, 1,
  },
  {
    "fcb_cpld_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
  {
    "scm_cpld_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 5, 1,
  },
  {
    "psu_alert_1_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 6, 1,
  },
  {
    "psu_alert_2_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 7, 1,
  },
  {
    "tpm_pp_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 1,
  },
  {
    "mac_cpld_pcie_intr_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 1, 1,
  },
  {
    "smb_tpm_intr_N_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 2, 1,
  },
  {
    "mac_cpld_pcie_wake_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 3, 1,
  },
  {
    "debug_card_present_N_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 4, 1,
  },
  {
    "scm_present_int_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 5, 1,
  },
  {
    "psu_present_1_N_int_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 6, 1,
  },
  {
    "psu_present_2_N_int_status",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 7, 1,
  },
  {
    "psu_dc_power_ok_1_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 0, 1,
  },
  {
    "psu_dc_power_ok_2_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 1, 1,
  },
  {
    "psu_ac_power_ok_1_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 2, 1,
  },
  {
    "psu_ac_power_ok_2_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 3, 1,
  },
  {
    "gb_power_ok_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 4, 1,
  },
  {
    "bmc_power_ok_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 5, 1,
  },
  {
    "xp5r0v_power_good_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 6, 1,
  },
  {
    "fault_r_xp5r0v_usb_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 7, 1,
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
    "led_test_enable",
    "0: LED control by FPGA\n"
    "1: Test mode, LED manual control",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 3, 1,
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
    "00: GB UART-0\n"
    "01: GB UART-2\n"
    "10: GB UART-3\n"
    "11: reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3a, 6, 2,
  },
  {
    "fpga2_spi_wp_n",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 0, 1,
  },
  {
    "fpga1_spi_wp_n",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 1, 1,
  },
  {
    "scm_spi_wp_n",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 2, 1,
  },
  {
    "cpld_bmc_phy_2_wp",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 3, 1,
  },
  {
    "cpld_bmc_spi_2_wp",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 4, 1,
  },
  {
    "cpld_bmc_phy_1_wp",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 5, 1,
  },
  {
    "cpld_bmc_spi_1_wp_n",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 6, 1,
  },
  {
    "cpld_56980_qspi_wp_n",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 7, 1,
  },
  {
    "usb_en_1",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 0, 1,
  },
  {
    "usb_en_2",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 1, 1,
  },
  {
    "usb_en_3",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 2, 1,
  },
  {
    "cpld_usb_mux_sel_0",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 3, 1,
  },
  {
    "cpld_usb_mux_sel_1",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 4, 1,
  },
  {
    "gb_turn_on",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 0, 1,
  },
  {
    "fcb_3r3v_en",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 1, 1,
  },
  {
    "scm_power_en",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 2, 1,
  },
  {
    "xp5r0v_usb_en",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 3, 1,
  },
  {
    "xp3r3v_1220_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 0, 1,
  },
  {
    "xp5r0v_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 1, 1,
  },
  {
    "xp1r2v_bmc_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 2, 1,
  },
  {
    "xp2r5v_bmc_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 3, 1,
  },
  {
    "xp3r3v_bmc_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 4, 1,
  },
  {
    "xp1r15_bmc_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 5, 1,
  },
  {
    "xp3r3v_fpga_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 0, 1,
  },
  {
    "xp1r8v_fpga_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 1, 1,
  },
  {
    "xp1r0v_fpga_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 2, 1,
  },
  {
    "usb_oc_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 3, 1,
  },
  {
    "xp3r3v_optical_left_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 4, 1,
  },
  {
    "xp3r3v_optical_right_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 5, 1,
  },
  {
    "xp1r2v_gb_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x45, 0, 1,
  },
  {
    "xp1r8v_gb_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x45, 1, 1,
  },
  {
    "vddcore_gb_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x45, 2, 1,
  },
  {
    "trvdd_gb_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x45, 3, 1,
  },
  {
    "xp3r3v_gb_power_good",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x45, 4, 1,
  },

  {
    "gb_rov0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 0, 1,
  },
  {
    "gb_rov1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 1, 1,
  },
  {
    "gb_rov2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 2, 1,
  },
  {
    "gb_rov3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 3, 1,
  },
  {
    "gb_rov4",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 4, 1,
  },
  {
    "gb_rov5",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 5, 1,
  },
  {
    "gb_rov6",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 6, 1,
  },
  {
    "gb_rov7",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x46, 7, 1,
  },
  {
    "dom_fpga_1_done",
    "0: don't finish loading DOM FPGA image\n"
    "1: finsh loading DOM FPGA image",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x47, 0, 1,
  },
  {
    "dom_fpga_2_done",
    "0: don't finish loading DOM FPGA image\n"
    "1: finsh loading DOM FPGA image",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x47, 1, 1,
  },
  {
    "dom_fpga_1_init",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 2, 1,
  },
  {
    "dom_fpga_2_init",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 3, 1,
  },
  {
    "dom_fpga_1_program",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 4, 1,
  },
  {
    "dom_fpga_2_program",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 5, 1,
  },

  {
    "spi_1_sel",
    "00h: BMC Select the System_E2.\n"
    "01h: BMC Select the BIOS.\n"
    "02h: BMC Select the BCM5389 E2.\n"
    "03h: BMC Select the GB PCIE E2.\n"
    "04h: BMC Select the FPGA1 flash.\n"
    "05h: BMC Select the FPGA2 flash.\n"
    "Others: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 0, 3,
  },
  {
    "rack_mon_r",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4a, 0, 8,
  },

  {
    "rack_mon_w",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 0, 8,
  },
  {
    "rack_mon_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4c, 0, 8,
  },
  {
    "rack_mon_pf_1_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4d, 0, 1,
  },
  {
    "rack_mon_rf_1_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4d, 1, 1,
  },
  {
    "rack_mon_pf_2_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4d, 2, 1,
  },
  {
    "rack_mon_rf_2_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4d, 3, 1,
  },
  {
    "rack_mon_pf_3_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4d, 4, 1,
  },
  {
    "rack_mon_rf_3_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4d, 5, 1,
  },
  {
    "rack_mon_pf_1_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4e, 0, 1,
  },
  {
    "rack_mon_rf_1_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4e, 1, 1,
  },
  {
    "rack_mon_pf_2_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4e, 2, 1,
  },
  {
    "rack_mon_rf_2_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4e, 3, 1,
  },
  {
    "rack_mon_pf_3_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4e, 4, 1,
  },
  {
    "rack_mon_rf_3_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4e, 5, 1,
  },
  {
    "rack_mon_pf_1_output_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4f, 0, 1,
  },
  {
    "rack_mon_rf_1_output_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4f, 1, 1,
  },
  {
    "rack_mon_pf_2_output_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4f, 2, 1,
  },
  {
    "rack_mon_rf_2_output_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4f, 3, 1,
  },
  {
    "rack_mon_pf_3_output_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4f, 4, 1,
  },
  {
    "rack_mon_rf_3_output_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4f, 5, 1,
  },
  {
    "fpga1_cpld_reserved_1_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 0, 1,
  },
  {
    "fpga1_cpld_reserved_2_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 1, 1,
  },
  {
    "fpga1_cpld_reserved_3_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 2, 1,
  },
  {
    "fpga1_cpld_reserved_4_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 3, 1,
  },
  {
    "fpga2_cpld_reserved_1_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 4, 1,
  },
  {
    "fpga2_cpld_reserved_2_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 5, 1,
  },
  {
    "fpga2_cpld_reserved_3_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 6, 1,
  },
  {
    "fpga2_cpld_reserved_4_input",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 7, 1,
  },
  {
    "fpga1_cpld_reserved_1_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 0, 1,
  },
  {
    "fpga1_cpld_reserved_2_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 1, 1,
  },
  {
    "fpga1_cpld_reserved_3_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 2, 1,
  },
  {
    "fpga1_cpld_reserved_4_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 3, 1,
  },
  {
    "fpga2_cpld_reserved_1_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 4, 1,
  },
  {
    "fpga2_cpld_reserved_2_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 5, 1,
  },
  {
    "fpga2_cpld_reserved_3_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 6, 1,
  },
  {
    "fpga2_cpld_reserved_4_output",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 7, 1,
  },
  {
    "fpga1_cpld_reserved_1_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 1,
  },
  {
    "fpga1_cpld_reserved_2_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 1, 1,
  },
  {
    "fpga1_cpld_reserved_3_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 2, 1,
  },
  {
    "fpga1_cpld_reserved_4_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 3, 1,
  },
  {
    "fpga2_cpld_reserved_1_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 4, 1,
  },
  {
    "fpga2_cpld_reserved_2_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 5, 1,
  },
  {
    "fpga2_cpld_reserved_3_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 6, 1,
  },
  {
    "fpga2_cpld_reserved_4_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 7, 1,
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
  int n_attrs = sizeof(smb_syscpld_attr_table_gb) / sizeof(smb_syscpld_attr_table_gb[0]);
  if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
    return -ENODEV;

  return i2c_dev_sysfs_data_init(client, &smb_syscpld_data,
                                 smb_syscpld_attr_table_gb, n_attrs);
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

MODULE_AUTHOR("Facebook");
MODULE_DESCRIPTION("SMB SYSCPLD Driver");
MODULE_LICENSE("GPL");

module_init(smb_syscpld_mod_init);
module_exit(smb_syscpld_mod_exit);
