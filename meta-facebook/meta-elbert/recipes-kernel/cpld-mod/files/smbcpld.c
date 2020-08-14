/*
 * smbcpld.c - The i2c driver for the SMB CPLD on ELBERT
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

static const i2c_dev_attr_st smbcpld_attr_table[] = {
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
    "spi_ctrl",
    "SPI target selection\n"
    "0xB-0xF  Unused\n"
    "0x8      TH4 QSFPi Flash\n"
    "0x4      SMB CPLD FPGA Select\n"
    "0x2      Fancard FPGA Select\n"
    "0x1      PIM FPGA flash Select\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xA, 0, 8,
  },
  {
    "spi_pim_en",
    "SPI PIM FPGA Flash Master Select Enable\n"
    "0x1      PIM FPGA Flash Select\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xA, 0, 1,
  },
  {
    "spi_fancard_en",
    "SPI Fancard Select Enable\n"
    "0x1      PIM FPGA Flash Select\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xA, 1, 1,
  },
  {
    "spi_smb_cpld_en",
    "SPI SMB CPLD Select Enable\n"
    "0x1      SMB CPLD FPGA Select\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xA, 2, 1,
  },
  {
    "jtag_ctrl",
    "JTAG target selection\n"
    "0x8-0xF  Unused\n"
    "0x3      TH4 jtag Select\n"
    "0x2      SMB CPLD FPGA jtag Select\n"
    "0x1      Fan FPGA jtag Select\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xC, 0, 8,
  },
  {
    "spi_th4_qspi_en",
    "SPI TH4 QSFPi Select Enable\n"
    "0x1      TH4 QSFPi Select\n"
    "0x0      Normal Operation (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xA, 3, 1,
  },
  {
    "pim_smb_mux_rst",
    "PIM SMBus Mux reset\n"
    "0x1: Reset\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 0, 1,
  },
  {
    "psu_smb_mux_rst",
    "PSU SMBus Mux reset\n"
    "0x1: Reset\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 1, 1,
  },
  {
    "th4_mhost0_boot_dev",
    "0x1: mHost0 boots from QSFP flash (default)\n"
    "0x0: mHost0 is held in reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 0, 1,
  },
  {
    "th4_pcie_wake_l",
    "0x1: PCIE_WAKE_L is 1 (default)\n"
    "0x2: PCIE_WAKE_L is 0",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 1, 1,
  },
  {
    "th4_pcie_force_lane",
    "TH4 PCIe_Force_Lane[1:0]\n"
    "Select maximum link width of PCIe Interface\n"
    "0x3-0x4: Reserved\n"
    "0x2: PCIe link can operate at x1 or x2 link width only\n"
    "0x1: PCIe link can operate at x1 link width only\n"
    "0x0: PCIe link can operate at x1, x2 or x4 link width (default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 2, 2,
  },
  {
    "th4_pcie_force_gen",
    "TH4 PCIe_Force_gen[1:0]\n"
    "Select maximum operating rate of PCIe Interface\n"
    "0x3-0x4: Reserved\n"
    "0x2: PCIe link can operate at Gen1 or Gen2 only\n"
    "0x1: PCIe link can operate at Gen1 only\n"
    "0x0: PCIe link can operate at Gen1, Gen2, or Gen3 (Default)\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 4, 2,
  },
  {
    "smb_reset",
    "0x1: Reset SMB on PCIe intf to COMe CPU\n"
    "0x0: Out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x13, 0, 1,
  },
  {
    "th4_reset",
    "0x1: TH4 in Reset\n"
    "0x0: TH4 out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 1,
  },
  {
    "th4_pci_reset",
    "0x1: TH4 PCIE in Reset\n"
    "0x0: TH4 PCIE out of reset",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 1, 1,
  },
  {
    "th4_avs",
    "TH4 AVS pin voltage\n",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 0, 8,
  },
  {
    "powercycle_duration",
    "SPI target selection\n"
    "0x7      10 minutes\n"
    "0x6      5 minutes\n"
    "0x5      2 minutes\n"
    "0x4      1 minute\n"
    "0x3      40 seconds\n"
    "0x2      30 seconds\n"
    "0x1      20 seconds\n"
    "0x0      10 seconds (Default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x1A, 0, 3,
  },
  {
    "smb_power_en",
    "0x1: Enable Switch Card Full Power Domain\n"
    "0x0: Full power disabled (default)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x1C, 0, 1,
  },
  {
    "bmc_int_sta",
    "BMC Interrupt Status (Read Only)\n"
    "Bit7 SMB Fatal Error          (reg 0x28)\n"
    "Bit6 SMB Temp / Power Intr    (reg 0x24)\n"
    "Bit5 PSU status changed       (reg 0x51)\n"
    "Bit4 Fan Card Status          (reg 0x40)\n"
    "Bit3 PIM Fatal Error Intr     (reg 0x3c)\n"
    "Bit2 PIM DPM Alert Intr       (reg 0x38)\n"
    "Bit1 PIM Temp Alert Intr      (reg 0x34)\n"
    "Bit0 PIM Present Changed Intr (reg 0x31)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x20, 0, 8,
  },
  {
    "smb_temp_pwr_alert_int",
    "Switch Card Temp and Power Alert (Read Only)\n"
    "Bit7 Reserved\n"
    "Bit6 SMB ISL68226 POS0V75_ANLG Alert\n"
    "Bit5 SMB RAA228228 POS0V75_CORE Alert\n"
    "Bit4 SMB DPM Alert\n"
    "Bit3 Reserved\n"
    "Bit2 SMB POS0V75_ANLG Temp Alert\n"
    "Bit1 SMB POS0V75_CORE Temp Alert\n"
    "Bit0 SMB MAX6581 Temp Alert",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x24, 0, 8,
  },
  {
    "smb_temp_pwr_alert_mask",
    "Switch Card Temp and Power Alert Mask Register (R/W)\n"
    "Bit7 Reserved\n"
    "Bit6 Mask SMB ISL68226 POS0V75_ANLG Alert\n"
    "Bit5 Mask SMB ISL68226 POS0V75_CORE Alert\n"
    "Bit4 Mask SMB DPM Alert\n"
    "Bit3 Reserved\n"
    "Bit2 Mask SMB POS0V75_ANLG Temp Alert\n"
    "Bit1 Mask SMB POS0V75_CORE Temp Alert\n"
    "Bit0 Mask SMB MAX6581 Temp Alert",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 0, 8,
  },
  {
    "smb_fatal_error",
    "Switch Card Fatal Error (Read Only)\n"
    "Bit 3: Flyover Cable Error\n"
    "Bit 2: Switch Module SMB/CPLD Communcation Error\n"
    "Bit 1: Shadow Bus Parity Error from CPU to Switch\n"
    "Bit 0: Switch Card SMB Watchdog Timeout",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x28, 0, 4,
  },
  {
    "smb_fatal_error_mask",
    "Switch Card Fatal Error Mask Register (R/W)\n"
    "Bit 3: Mask Flyover Cable Error\n"
    "Bit 2: Mask Switch Module SMB/CPLD Communcation Error\n"
    "Bit 1: Mask Shadow Bus Parity Error from CPU to Switch\n"
    "Bit 0: Mask Switch Card SMB Watchdog Timeout",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 0, 4,
  },
  {
    "pim2_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "pim3_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "pim4_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "pim5_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 3, 1,
  },
  {
    "pim6_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 4, 1,
  },
  {
    "pim7_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 5, 1,
  },
  {
    "pim8_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 6, 1,
  },
  {
    "pim9_present",
    "1: Present (currently)\n"
    "0: Not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 7, 1,
  },
  {
    "pim2_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 0, 1,
  },
  {
    "pim3_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 1, 1,
  },
  {
    "pim4_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 2, 1,
  },
  {
    "pim5_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 3, 1,
  },
  {
    "pim6_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 4, 1,
  },
  {
    "pim7_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 5, 1,
  },
  {
    "pim8_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 6, 1,
  },
  {
    "pim9_prsnt_change",
    "1: Presence changed since last read (Clear on read)\n"
    "0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x31, 7, 1,
  },
  {
    "pim_present_mask",
    "PIM Present interrupt mask. Bit0-7 corresponds to PIM2-9",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 8,
  },
  {
    "pim_temp_alert_sta",
    "PIM temp sensor alert status. Bit 0-7 corresponds to PIM2-9",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 0, 8,
  },
  {
    "pim_temp_alert_mask",
    "PIM temp sensor alert interrupt mask. Bit0-7 for PIM2-9",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 0, 8,
  },
  {
    "pim_dpm_alert_sta",
    "PIM DPM alert status. Bit 0-7 corresponds to PIM2-9",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x38, 0, 8,
  },
  {
    "pim_dpm_alert_mask",
    "PIM DPM alert interrupt mask. Bit0-7 for PIM2-9",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 0, 8,
  },
  {
    "pim_fatal_sta",
    "PIM fatal error status. Bit 0-7 corresponds to PIM2-9\n"
    "(Read Only, Clear on Write)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3C, 0, 8,
  },
  {
    "pim_fatal_mask",
    "PIM fatal error mask. Bit0-7 for pim2-8",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3D, 0, 8,
  },
  {
    "fan_card_status",
    "Bit3 : Fan card fatal error (Clear on write)\n"
    "Bit2 : Fan card temp alert (Read Only)\n"
    "Bit1 : Fan card intr req (Read Only)\n"
    "Bit0 : Fan card present (Read Only)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
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
    "psu_present",
    "Bit 3: PSU4 Present\n"
    "Bit 2: PSU3 Present\n"
    "Bit 1: PSU2 Present\n"
    "Bit 0: PSU1 Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 0, 4,
  },
  {
    "psu1_present",
    "0x1: PSU1 Present\n"
    "0x0: PSU1 Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 0, 1,
  },
  {
    "psu2_present",
    "0x1: PSU2 Present\n"
    "0x0: PSU2 Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 1, 1,
  },
  {
    "psu3_present",
    "0x1: PSU3 Present\n"
    "0x0: PSU3 Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 2, 1,
  },
  {
    "psu4_present",
    "0x1: PSU4 Present\n"
    "0x0: PSU4 Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 3, 1,
  },
  {
    "psu_present_change",
    "Value set to 1 when corresponding bit in psu_present changes\n"
    "(Clear on Write)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 0, 4,
  },
  {
    "psu_present_mask",
    "Mask each bit of psu_present in interrupt generation.\n"
    "Bit definition is the same as psu_present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 4,
  },
  {
    "psu_status",
    "Bit 7: PSU4 Input Feed OK\n"
    "Bit 6: PSU3 Input Feed OK\n"
    "Bit 5: PSU2 Input Feed OK\n"
    "Bit 4: PSU1 Input Feed OK\n"
    "Bit 3: PSU4 Output Feed OK\n"
    "Bit 2: PSU3 Output Feed OK\n"
    "Bit 1: PSU2 Output Feed OK\n"
    "Bit 0: PSU1 Output Feed OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 0, 8,
  },
  {
    "psu1_output_ok",
    "0x1: PSU1 Output Feed OK\n"
    "0x0: PSU1 Output Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 0, 1,
  },
  {
    "psu2_output_ok",
    "0x1: PSU2 Output Feed OK\n"
    "0x0: PSU2 Output Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 1, 1,
  },
  {
    "psu3_output_ok",
    "0x1: PSU3 Output Feed OK\n"
    "0x0: PSU3 Output Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 2, 1,
  },
  {
    "psu4_output_ok",
    "0x1: PSU4 Output Feed OK\n"
    "0x0: PSU4 Output Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 3, 1,
  },
  {
    "psu1_input_ok",
    "0x1: PSU1 Input Feed OK\n"
    "0x0: PSU1 Input Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 4, 1,
  },
  {
    "psu2_input_ok",
    "0x1: PSU2 Input Feed OK\n"
    "0x0: PSU2 Input Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 5, 1,
  },
  {
    "psu3_input_ok",
    "0x1: PSU3 Input Feed OK\n"
    "0x0: PSU3 Input Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 6, 1,
  },
  {
    "psu4_input_ok",
    "0x1: PSU4 Input Feed OK\n"
    "0x0: PSU4 Input Feed Not OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 7, 1,
  },
  {
    "psu_status_changed",
    "Value set to 1 when corresponding bit in psu_status changes\n"
    "(Clear on Write)",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x54, 0, 8,
  },
  {
    "psu_status_mask",
    "Mask each bit of psu_status in interrupt generation.\n"
    "Bit definition is the same as psu_status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x55, 0, 8,
  },
  {
    "pim_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done\n"
    "Bit 7: PIM9 Powercycle\n"
    "Bit 6: PIM8 Powercycle\n"
    "Bit 5: PIM7 Powercycle\n"
    "Bit 4: PIM6 Powercycle\n"
    "Bit 3: PIM5 Powercycle\n"
    "Bit 2: PIM4 Powercycle\n"
    "Bit 1: PIM3 Powercycle\n"
    "Bit 0: PIM2 Powercycle",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 0, 8,
  },
  {
    "pim2_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 0, 1,
  },
  {
    "pim3_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 1, 1,
  },
  {
    "pim4_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 2, 1,
  },
  {
    "pim5_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 3, 1,
  },
  {
    "pim6_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 4, 1,
  },
  {
    "pim7_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 5, 1,
  },
  {
    "pim8_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 6, 1,
  },
  {
    "pim9_reset",
    "1: Powercycle PIM\n"
    "0: Powercycle Done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x61, 7, 1,
  },
  {
    "pim2_fpga_rev_major",
    "PIM2 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x81, 0, 8,
  },
  {
    "pim3_fpga_rev_major",
    "PIM 3 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x82, 0, 8,
  },
  {
    "pim4_fpga_rev_major",
    "PIM 4 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x83, 0, 8,
  },
  {
    "pim5_fpga_rev_major",
    "PIM 5 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or pim not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x84, 0, 8,
  },
  {
    "pim6_fpga_rev_major",
    "PIM 6 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x85, 0, 8,
  },
  {
    "pim7_fpga_rev_major",
    "PIM 7 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x86, 0, 8,
  },
  {
    "pim8_fpga_rev_major",
    "PIM 8 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x87, 0, 8,
  },
  {
    "pim9_fpga_rev_major",
    "PIM 9 FPGA Major Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x88, 0, 8,
  },
  {
    "pim2_fpga_rev_minor",
    "PIM 2 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x91, 0, 8,
  },
  {
    "pim3_fpga_rev_minor",
    "PIM 3 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x92, 0, 8,
  },
  {
    "pim4_fpga_rev_minor",
    "PIM 4 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x93, 0, 8,
  },
  {
    "pim5_fpga_rev_minor",
    "PIM 5 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x94, 0, 8,
  },
  {
    "pim6_fpga_rev_minor",
    "PIM 6 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x95, 0, 8,
  },
  {
    "pim7_fpga_rev_minor",
    "PIM 7 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x96, 0, 8,
  },
  {
    "pim8_fpga_rev_minor",
    "PIM 8 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x97, 0, 8,
  },
  {
    "pim9_fpga_rev_minor",
    "PIM 9 FPGA Minor Revision\n"
    "0xff – FPGA unprogrammed or PIM not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x98, 0, 8,
  },
  {
    "th4_cpld_ver_minor",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xb0, 0, 8,
  },
  {
    "th4_cpld_ver_major",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xb1, 0, 8,
  },
};

static i2c_dev_data_st smbcpld_data;

/*
 * SMB CPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x23, I2C_CLIENT_END
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
   * We don't currently do any detection of the ELBERTCPLD
   */
  strlcpy(info->type, "smbcpld", I2C_NAME_SIZE);
  return 0;
}

static int smbcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(smbcpld_attr_table) / sizeof(smbcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &smbcpld_data,
                                 smbcpld_attr_table, n_attrs);
}

static int smbcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &smbcpld_data);
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

MODULE_AUTHOR("Dean Kalla");
MODULE_DESCRIPTION("ELBERT SMB CPLD Driver");
MODULE_LICENSE("GPL");

module_init(smbcpld_mod_init);
module_exit(smbcpld_mod_exit);
