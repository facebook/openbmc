/*
 * smbcpld.c - The i2c driver for SMBCPLD
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

#define pim_fpga_plug_help_str                  \
  "0: PIM_FPGA had plugged\n"                   \
  "1: No PIM_FPGA"                              \

static const i2c_dev_attr_st smbcpld_attr_table[] = {
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
    "psu_L1_smb_alert",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 0, 1,
  },
  {
    "psu_L1_pwr_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 1, 1,
  },
  {
    "psu_L1_present_L",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 2, 1,
  },
  {
    "psu_L1_input_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 3, 1,
  },
  {
    "psu_L2_smb_alert",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 4, 1,
  },
  {
    "psu_L2_pwr_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 5, 1,
  },
  {
    "psu_L2_present_L",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 6, 1,
  },
  {
    "psu_L2_input_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 7, 1,
  },
  {
    "psu_R1_smb_alert",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 0, 1,
  },
  {
    "psu_R1_pwr_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 1, 1,
  },
  {
    "psu_R1_present_L",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 2, 1,
  },
  {
    "psu_R1_input_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 3, 1,
  },
  {
    "psu_R2_smb_alert",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 4, 1,
  },
  {
    "psu_R2_pwr_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 5, 1,
  },
  {
    "psu_R2_present_L",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 6, 1,
  },
  {
    "psu_R2_input_ok",
    fail_normal_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 7, 1,
  },
  {
    "cpld_mac_reset_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 0, 1,
  },
  {
    "cpld_mac_pcie_perst_l",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 1, 1,
  },
  {
    "cpld_bcm5396_resetb_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 2, 1,
  },
  {
    "cpld_bmc_phy_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 3, 1,
  },
  {
    "cpld_bmc_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 4, 1,
  },
  {
    "bmc9_9548_0_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 5, 1,
  },
  {
    "bmc10_9548_1_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 6, 1,
  },
  {
    "bmc12_9548_2_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x05, 7, 1,
  },
  {
    "bmc_scm_lpc_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 0, 1,
  },
  {
    "bmc_extrst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 1, 1,
  },
  {
    "cpld_fpga_flash_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 2, 1,
  },
  {
    "cpld_smb_phy_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 3, 1,
  },
  {
    "cpld_usb2112a_brdg_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 4, 1,
  },
  {
    "cpld_cp2112_i2c_9548_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 5, 1,
  },
  {
    "cpld_clkgen_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 6, 1,
  },
  {
    "cpld_tpm_i2c_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x06, 7, 1,
  },
  {
    "bmc_pim8_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 0, 1,
  },
  {
    "bmc_pim7_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 1, 1,
  },
  {
    "bmc_pim6_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 2, 1,
  },
  {
    "bmc_pim5_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 3, 1,
  },
  {
    "bmc_pim4_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 4, 1,
  },
  {
    "bmc_pim3_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 5, 1,
  },
  {
    "bmc_pim2_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 6, 1,
  },
  {
    "bmc_pim1_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x07, 7, 1,
  },
  {
    "smb_cpld_fcm_cpld_rst_t",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 0, 1,
  },
  {
    "smb_cpld_fcm_cpld_rst_b",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 1, 1,
  },
  {
    "smb_cpld_fcm_9548_rst_t",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 2, 1,
  },
  {
    "smb_cpld_fcm_9548_rst_b",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 3, 1,
  },
  {
    "bmc_spi_2_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 4, 1,
  },
  {
    "bmc_spi_1_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 5, 1,
  },
  {
    "psu_pdb_9548_R_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 6, 1,
  },
  {
    "psu_pdb_9548_L_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 7, 1,
  },
  {
    "cpld_pim_hw_rst_1",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 0, 1,
  },
  {
    "cpld_pim_hw_rst_2",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 1, 1,
  },
  {
    "cpld_pim_hw_rst_3",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 2, 1,
  },
  {
    "cpld_pim_hw_rst_4",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 3, 1,
  },
  {
    "cpld_pim_hw_rst_5",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 4, 1,
  },
  {
    "cpld_pim_hw_rst_6",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 5, 1,
  },
  {
    "cpld_pim_hw_rst_7",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 6, 1,
  },
  {
    "cpld_pim_hw_rst_8",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 7, 1,
  },
  {
    "cpld_pim1_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 0, 1,
  },
  {
    "cpld_pim2_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 1, 1,
  },
  {
    "cpld_pim3_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 2, 1,
  },
  {
    "cpld_pim4_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 3, 1,
  },
  {
    "cpld_pim5_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 4, 1,
  },
  {
    "cpld_pim6_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 5, 1,
  },
  {
    "cpld_pim7_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 6, 1,
  },
  {
    "cpld_pim8_9548_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 7, 1,
  },
  {
    "cpld_9548_0_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0b, 0, 1,
  },
  {
    "cpld_9548_1_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0b, 1, 1,
  },
  {
    "cpld_9548_2_rst",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0b, 2, 1,
  },
  {
    "cpld_usbhub_rst_n",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0b, 3, 1,
  },
  {
    "scm_reset_cpld_1",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0c, 0, 1,
  },
  {
    "scm_reset_cpld_2",
    reset_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0c, 1, 1,
  },
  {
    "system_reset_lock_unlock",
    sys_lock_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0d, 0, 1,
  },
  {
    "pim_fpga_cpld_1_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 0, 1,
  },
  {
    "pim_fpga_cpld_2_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 1, 1,
  },
  {
    "pim_fpga_cpld_3_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 2, 1,
  },
  {
    "pim_fpga_cpld_4_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 3, 1,
  },
  {
    "pim_fpga_cpld_5_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 4, 1,
  },
  {
    "pim_fpga_cpld_6_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 5, 1,
  },
  {
    "pim_fpga_cpld_7_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 6, 1,
  },
  {
    "pim_fpga_cpld_8_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 7, 1,
  },
  {
    "temp_sensor_cpld_alert1_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "temp_sensor_cpld_alert2_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 1, 1,
  },
  {
    "temp_sensor_cpld_alert3_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "temp_sensor_cpld_alert4_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 3, 1,
  },
  {
    "pdb_L_temp_alert_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 4, 1,
  },
  {
    "pdb_R_temp_alert_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 5, 1,
  },
  {
    "fcm_smb_cpld_fan_alert_b_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 6, 1,
  },
  {
    "fcm_smb_cpld_fan_alert_t_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 7, 1,
  },
  {
    "scm_alert_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 0, 1,
  },
  {
    "scm_m2_alert_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 1, 1,
  },
  {
    "scm_tpm_pp_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 2, 1,
  },
  {
    "vdd_core_fault_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 3, 1,
  },
  {
    "mac_cpld_pcie_intr_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 4, 1,
  },
  {
    "tpm_cpld_i2c_daint_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 5, 1,
  },
  {
    "mac_cpld_pcie_wake_L_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 6, 1,
  },
  {
    "pim_fpga_cpld_1_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "pim_fpga_cpld_2_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "pim_fpga_cpld_3_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "pim_fpga_cpld_4_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 3, 1,
  },
  {
    "pim_fpga_cpld_5_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 4, 1,
  },
  {
    "pim_fpga_cpld_6_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 5, 1,
  },
  {
    "pim_fpga_cpld_7_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 6, 1,
  },
  {
    "pim_fpga_cpld_8_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 7, 1,
  },
  {
    "temp_sensor_cpld_alert1_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 0, 1,
  },
  {
    "temp_sensor_cpld_alert2_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 1, 1,
  },
  {
    "temp_sensor_cpld_alert3_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 2, 1,
  },
  {
    "temp_sensor_cpld_alert4_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 3, 1,
  },
  {
    "pdb_L_temp_alert_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 4, 1,
  },
  {
    "pdb_R_temp_alert_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 5, 1,
  },
  {
    "fcm_smb_cpld_fan_alt_b_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 6, 1,
  },
  {
    "fcm_smb_cpld_fan_alt_t_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 7, 1,
  },
  {
    "scm_alert_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 1,
  },
  {
    "scm_m2_alert_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 1, 1,
  },
  {
    "scm_tpm_pp_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 2, 1,
  },
  {
    "vdd_core_fault_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 3, 1,
  },
  {
    "mac_cpld_pcie_intr_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 4, 1,
  },
  {
    "tpm_cpld_i2c_daint_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 5, 1,
  },
  {
    "mac_cpld_pcie_wake_L_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 6, 1,
  },
  {
    "pim_fpga_cpld_1_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "pim_fpga_cpld_2_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "pim_fpga_cpld_3_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "pim_fpga_cpld_4_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 3, 1,
  },
  {
    "pim_fpga_cpld_5_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
  {
    "pim_fpga_cpld_6_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 5, 1,
  },
  {
    "pim_fpga_cpld_7_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 6, 1,
  },
  {
    "pim_fpga_cpld_8_prsnt_n_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 7, 1,
  },
  {
    "pim_fpga_cpld_1_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 1,
  },
  {
    "pim_fpga_cpld_2_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 1, 1,
  },
  {
    "pim_fpga_cpld_3_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 2, 1,
  },
  {
    "pim_fpga_cpld_4_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 3, 1,
  },
  {
    "pim_fpga_cpld_5_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 4, 1,
  },
  {
    "pim_fpga_cpld_6_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 5, 1,
  },
  {
    "pim_fpga_cpld_7_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 6, 1,
  },
  {
    "pim_fpga_cpld_8_prsnt_n_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 7, 1,
  },
  {
    "pim_fpga_cpld_1_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 0, 1,
  },
  {
    "pim_fpga_cpld_2_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 1, 1,
  },
  {
    "pim_fpga_cpld_3_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 2, 1,
  },
  {
    "pim_fpga_cpld_4_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 3, 1,
  },
  {
    "pim_fpga_cpld_5_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 4, 1,
  },
  {
    "pim_fpga_cpld_6_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 5, 1,
  },
  {
    "pim_fpga_cpld_7_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 6, 1,
  },
  {
    "pim_fpga_cpld_8_prsnt_n_status",
    pim_fpga_plug_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x32, 7, 1,
  },
  {
    "fcm_smb_cpld_present_t_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 0, 1,
  },
  {
    "fcm_smb_cpld_present_b_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 1, 1,
  },
  {
    "scm_presnt_int_status",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x33, 2, 1,
  },
  {
    "fcm_smb_cpld_present_t_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 0, 1,
  },
  {
    "fcm_smb_cpld_present_b_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 1, 1,
  },
  {
    "scm_presnt_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 2, 1,
  },
  {
    "fcm_smb_cpld_present_t_status",
    "0: SMB had plugged\n"
    "1: No SMB",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x35, 0, 1,
  },
  {
    "fcm_smb_cpld_present_b_status",
    "0: SMB had plugged\n"
    "1: No SMB",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x35, 1, 1,
  },
  {
    "scm_presnt_status",
    "0: SCM had plugged\n"
    "1: No SCM",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x35, 2, 1,
  },
  {
    "bmc_cpld_heartbeat_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 0, 1,
  },
  {
    "mac_cpld_pcie_wake_L",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x40, 1, 1,
  },
  {
    "cpld_smb_phy_lowpwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 2, 1,
  },
  {
    "cpld_smb_phy_wp",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 3, 1,
  },
  {
    "cpld_bmc_phy_lowpwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 4, 1,
  },
  {
    "cpld_bmc_phy_wp",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 5, 1,
  },
  {
    "cpld_bmc_spi_1_wp_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 6, 1,
  },
  {
    "cpld_cp2112_ee_wp",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 0, 1,
  },
  {
    "cpld_brdid_ee_wp",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 1, 1,
  },
  {
    "cpld_mac_wp",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 2, 1,
  },
  {
    "cpld_spi_sec_boot_wp_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 3, 1,
  },
  {
    "usbhub_en1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 4, 1,
  },
  {
    "usbhub_en3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 5, 1,
  },
  {
    "cpld_usbhub_ocs_n1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 6, 1,
  },
  {
    "cpld_usbhub_ocs_n3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x41, 7, 1,
  },
  {
    "cpld_in_p1220",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 0, 1,
  },
  {
    "cpld_fpga_prg",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 1, 1,
  },
  {
    "smb_cpld_uart1_sel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 2, 1,
  },
  {
    "smb_cpld_fcm_b_cpld_sel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 3, 1,
  },
  {
    "smb_cpld_fcm_t_cpld_sel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 4, 1,
  },
  {
    "bmc_pwr_ok",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 0, 1,
  },
  {
    "th3_pwr_ok",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 1, 1,
  },
  {
    "ps1_R_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x43, 2, 1,
  },
  {
    "ps2_R_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x43, 3, 1,
  },
  {
    "ps1_L_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x43, 4, 1,
  },
  {
    "ps2_L_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x43, 5, 1,
  },
  {
    "cpld_pim1_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 0, 1,
  },
  {
    "cpld_pim2_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 1, 1,
  },
  {
    "cpld_pim3_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 2, 1,
  },
  {
    "cpld_pim4_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 3, 1,
  },
  {
    "cpld_pim5_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 4, 1,
  },
  {
    "cpld_pim6_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 5, 1,
  },
  {
    "cpld_pim7_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 6, 1,
  },
  {
    "cpld_pim8_pca9548_pwr",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 7, 1,
  },
  {
    "mac_cpld_rov0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 0, 1,
  },
  {
    "mac_cpld_rov1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 1, 1,
  },
  {
    "mac_cpld_rov2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 2, 1,
  },
  {
    "mac_cpld_rov3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 3, 1,
  },
  {
    "mac_cpld_rov4",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 4, 1,
  },
  {
    "mac_cpld_rov5",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 5, 1,
  },
  {
    "mac_cpld_rov6",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 6, 1,
  },
  {
    "mac_cpld_rov7",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x46, 7, 1,
  },
  {
    "cp2112a_cpld_gpio0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 0, 1,
  },
  {
    "cp2112a_cpld_gpio1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 1, 1,
  },
  {
    "cp2112a_cpld_gpio2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 2, 1,
  },
  {
    "cp2112a_cpld_gpio3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 3, 1,
  },
  {
    "cp2112a_cpld_gpio4",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 4, 1,
  },
  {
    "cp2112a_cpld_gpio5",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 5, 1,
  },
  {
    "cp2112a_cpld_gpio6",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x47, 6, 1,
  },
  {
    "spi_1_b0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 0, 1,
  },
  {
    "spi_1_b1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 1, 1,
  },
  {
    "spi_1_b2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 2, 1,
  },
  {
    "spi_2_b0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 3, 1,
  },
  {
    "spi_2_b1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 4, 1,
  },
  {
    "spi_2_b2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 5, 1,
  },
  {
    "spi_2_b3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x48, 6, 1,
  },
  {
    "th3_spi_mux_sel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 0, 1,
  },
  {
    "fpga_spi_mux_sel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 1, 1,
  },
  {
    "cpld_bcm5396_mux_sel",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 2, 1,
  },
  {
    "bmc_cpld_gpio_spare1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4a, 0, 1,
  },
  {
    "bmc_cpld_gpio_spare2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4a, 1, 1,
  },
  {
    "bmc_cpld_gpio_spare3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4a, 2, 1,
  },
  {
    "bmc_cpld_gpio_spare4",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4a, 3, 1,
  },
  {
    "bmc_cpld_gpio_spare5",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4a, 4, 1,
  },
  {
    "cpld_reserved_1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 0, 1,
  },
  {
    "cpld_reserved_2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 1, 1,
  },
  {
    "cpld_reserved_3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 2, 1,
  },
  {
    "cpld_reserved_4",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4b, 3, 1,
  },
  {
    "iso_reserved_1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4b, 4, 1,
  },
  {
    "iso_reserved_2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4b, 5, 1,
  },
  {
    "iso_reserved_3",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4b, 6, 1,
  },
};

/*
 * SMBCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x3e, I2C_CLIENT_END
};

/* SMBCPLD id */
static const struct i2c_device_id smbcpld_id[] = {
  { "smbcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, smbcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int smbcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SMBCPLD
   */
  strlcpy(info->type, "smbcpld", I2C_NAME_SIZE);
  return 0;
}

static int smbcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(smbcpld_attr_table) / sizeof(smbcpld_attr_table[0]);
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data,
                                 smbcpld_attr_table, n_attrs);
}

static int smbcpld_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

  return 0;
}

static struct i2c_driver smbcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "smbcpld",
  },
  .probe    = smbcpld_probe,
  .remove   = smbcpld_remove,
  .id_table = smbcpld_id,
  .detect   = smbcpld_detect,
  .address_list = normal_i2c,
};

static int __init smbcpld_mod_init(void)
{
  return i2c_add_driver(&smbcpld_driver);
}

static void __exit smbcpld_mod_exit(void)
{
  i2c_del_driver(&smbcpld_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("SMBCPLD Driver");
MODULE_LICENSE("GPL");

module_init(smbcpld_mod_init);
module_exit(smbcpld_mod_exit);
