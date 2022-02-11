/*
 * scmcpld.c - The i2c driver for SCMCPLD
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

static const i2c_dev_attr_st scmcpld_attr_table[] = {
  {
    "board_ver",
    "000: R0A\n"
    "001: R0B\n"
    "010: R0C\n"
    "100: R01\n"
    "101: R02\n"
    "110: R03",
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
    "sys_led",
    "0: LED is OFF\n"
    "1: LED is ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 0, 1,
  },
  {
    "sys_led_twinkle",
    "0: LED is not twinking\n"
    "1: LED is twinking",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 1, 1,
  },
  {
    "sys_led_color",
    "0: LED color is BLUE\n"
    "1: LED color is AMBER",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x08, 2, 1,
  },
  {
    "rj45_led1",
    "Need 0x09[7] set to 0\n"
    "0: LED GREEN is OFF\n"
    "1: LED GREEN is ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 0, 1,
  },
  {
    "rj45_led2",
    "Need 0x09[7] set to 0\n"
    "0: LED ORANGE is OFF\n"
    "1: LED ORANGE is ON",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 1, 1,
  },
  {
    "sfp_led3_n",
    "Need 0x09[7] set to 0\n"
    "1: LED ORANGE is ON\n"
    "0: LED ORANGE is OFF",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 2, 1,
  },
  {
    "sfp_led3_p",
    "Need 0x09[7] set to 0\n"
    "1: LED GREEN is ON\n"
    "0: LED GREEN is OFF",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 3, 1,
  },
  {
    "oob_mode_sel",
    "Need 0x09[7] set to 0\n"
    "1: OOB LED control by OOB\n"
    "0: SW control, control by 0x09[3:0]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 5, 1,
  },
  {
    "sfp_rj45_sel",
    "Need 0x09[7] set to 0\n"
    "1: SFP\n"
    "0: RJ45",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 6, 1,
  },
  {
    "sfp_auto_detect",
    "If SFP present then swith OOB LED to SFP, else switch OOB LED to RJ45\n"
    "1: Auto detect SFP present\n"
    "0: SW control, Control by SW register 0x09[6:5][3:0]",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x09, 7, 1,
  },
  {
    "oob_led1_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0a, 0, 1,
  },
  {
    "oob_led2_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0a, 1, 1,
  },
  {
    "oob_led3_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0a, 2, 1,
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
    "com_pwrok",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 4, 1,
  },
  {
    "com_pwr_btn_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 5, 1,
  },
  {
    "com_exp_rst_ctrl",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 6, 1,
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
    "com_exp_pwr_cycle",
    "Write 0 to this bit will trigger CPLD power cycling the COMe Module, This bit will auto set to 1 after Power Cycle finish",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 2, 1,
  },
  {
    "com_spi_sel",
    "0: SPI Flash Accessed by COM-e\n"
    "1: SPI Flash Accessed by BMC",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 0, 1,
  },
  {
    "com_spi_oe_n",
    "0:Enable\n"
    "1:Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 1, 1,
  },
  {
    "uart_mux_rst",
    "0: Reset UART Mux Logic\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "pcie_wake",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 0, 1,
  },
  {
    "hs_timer",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 1, 1,
  },
  {
    "hs_fault_n",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 2, 1,
  },
  {
    "hs_alert1",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 3, 1,
  },
  {
    "hs_alert2",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 4, 1,
  },
  {
    "ds100_int_n",
    "0: Interrupt\n"
    "1: No interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 5, 1,
  },
  {
    "hostwap_pg",
    "The Status of HOTSWAP_PG",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x21, 6, 1,
  },
  {
    "pcie_wake_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 0, 1,
  },
  {
    "hs_timer_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 1, 1,
  },
  {
    "hs_fault_n_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 2, 1,
  },
  {
    "hs_alert1_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 3, 1,
  },
  {
    "hs_alert2_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 4, 1,
  },
  {
    "ds100_int_n_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 5, 1,
  },
  {
    "hotswap_pg_mask",
    "0: CPLD passes the interrupt to CPU\n"
    "1: CPLD blocks incoming the interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x28, 5, 1,
  },
  {
    "c1p0_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 1,
  },
  {
    "c1p2_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 1, 1,
  },
  {
    "c3_3v_stby_pg",
    "0: Fail\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "a_3_3v_stby_pg",
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
    "c1p0_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 1,
  },
  {
    "c1p2_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 1, 1,
  },
  {
    "vdd_1v8_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 2, 1,
  },
  {
    "c5v_pwr_stby_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 3, 1,
  },
  {
    "c12v_userver_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 4, 1,
  },
  {
    "a_pwr_stby_en",
    "0: disable\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 6, 1,
  },
  {
    "iso_com_i2c_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 1,
  },
  {
    "iso_usbhub",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 1, 1,
  },
  {
    "iso_usbcdp",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 2, 1,
  },
  {
    "iso_uart_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 3, 1,
  },
  {
    "iso_m2_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 4, 1,
  },
  {
    "iso_tpm_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 6, 1,
  },
  {
    "iso_spi_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 7, 1,
  },
  {
    "iso_pcie_m2",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 0, 1,
  },
  {
    "iso_brg_thrm_n",
    "input from off-Module temp sensor indicating an over-temp situation",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 2, 1,
  },
  {
    "iso_fpgarp_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 3, 1,
  },
  {
    "iso_th3rp_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 4, 1,
  },
  {
    "iso_ds100_en",
    "0: Enable\n"
    "1: Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 5, 1,
  },
  {
    "iso_brg_thrmtrip_n",
    "Indicating that the CPU has entered thermal shutdown",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x34, 0, 1,
  },
  {
    "i2c_bus_en",
    "0: Disbale\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 1, 1,
  },
  {
    "usb_debug_en",
    "0: Disbale\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 2, 1,
  },
  {
    "usb_pwron_n",
    "0: Disbale\n"
    "1: Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 3, 1,
  },
  {
    "a_ds80_eeprom_wp",
    "0: Normal\n"
    "1: Write to the memory",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 4, 1,
  },
  {
    "ds80_eeprom_wp",
    "0: Normal\n"
    "1: Write to the memory",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 5, 1,
  },
  {
    "ds100_eeprom_wp",
    "0: Normal\n"
    "1: Write to the memory",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 6, 1,
  },
  {
    "rtc_clear",
    "Control The Status of RTC_CLEAR",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 7, 1,
  },
  {
    "g_batlow_n",
    "0: Enable\n"
    "1: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x36, 0, 1,
  },
  {
    "sys_eeprom_wp",
    "0: Enable\n"
    "1: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x36, 1, 1,
  },
  {
    "com_e_pwr_on",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x36, 2, 1,
  },
  {
    "ds80_all_done_n",
    "0: External EEPROM load passed\n"
    "1: External EEPROM load failed or incomplete",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 0, 1,
  },
  {
    "a_ds80_all_done_n",
    "0: External EEPROM load passed\n"
    "1: External EEPROM load failed or incomplete",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 1, 1,
  },
  {
    "ds100_done_n",
    "0: External EEPROM load passed\n"
    "1: External EEPROM load failed or incomplete",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 2, 1,
  },
  {
    "ds100_sd_th",
    "Controls LOS threshold setting",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 3, 1,
  },
  {
    "cpld_reset_1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 4, 1,
  },
  {
    "cpld_reset_2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x37, 5, 1,
  },
  {
    "uart5_sel",
    "0: Default mode\n"
    "1: SOL mode",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 0, 1,
  },
  {
    "uart2_sel",
    "0: Default mode\n"
    "1: SOL mode",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x38, 1, 1,
  },
  {
    "hitless_en",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 0, 1,
  },
  {
    "scm_seat_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 1, 1,
  },
  {
    "i2c_bus_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 3, 1,
  },
  {
    "i2c_usb_ready",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 4, 1,
  },
  {
    "iso_usb_fault",
    "USB over-current sense",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 5, 1,
  },
  {
    "usb_pwrflt_n",
    "Overcurrent",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 6, 1,
  },
  {
    "rate_s",
    "0: Reduced Bandwidth\n"
    "1: Full Bandwidth",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x39, 7, 1,
  },
  {
    "iso_brg_com_gpo_0",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 0, 1,
  },
  {
    "iso_com_gbe0_link1000_n",
    "Controller 0 1000 Mbit / sec link indicator",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 1, 1,
  },
  {
    "b_com_type0",
    "Module Type Descriptions",
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
  {
    "scm_uart_switch_n",
    "Debug card UART-Sel button",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 5, 1,
  },
  {
    "scm_debug_rst_btn_n",
    "Debug card reset button",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 6, 1,
  },
  {
    "scm_pwr_btn_n",
    "Debug card power button",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x3a, 7, 1,
  },
  {
    "scm_uart_switch_n_clr",
    "0: Clear 0x3A Bit [5] data\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3b, 0, 1,
  },
  {
    "scm_debug_rst_btn_n_clr",
    "0: Clear 0x3A Bit [6] data\n"
    "1: Normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3b, 1, 1,
  },
  {
    "iso_com_wdt_en",
    "0: Enable\n"
    "1: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3c, 0, 1,
  },
  {
    "iso_com_early_en",
    "0: Enable\n"
    "1: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3c, 1, 1,
  },
  {
    "iso_switch_en",
    "0: Enable\n"
    "1: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3c, 2, 1,
  },
  {
    "iso_com_thrm_en",
    "0: Enable\n"
    "1: Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x3c, 3, 1,
  },
  {
    "ds100_ensmb",
    "0:Disbale\n"
    "1:Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 0, 1,
  },
  {
    "ds80_ensmb",
    "0:Disbale\n"
    "1:Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 1, 1,
  },
  {
    "a_ds80_ensmb",
    "0:Disbale\n"
    "1:Enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 2, 1,
  },
  {
    "a_ds80_prsnt",
    "0:Enable\n"
    "1:Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 3, 1,
  },
  {
    "ds80_prsnt",
    "0:Enable\n"
    "1:Disbale",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 4, 1,
  },
  {
    "mod_gn",
    "Indicate that the module is present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 0, 1,
  },
  {
    "tx_loss",
    "Loss of Signal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 1, 1,
  },
  {
    "tx_fault",
    "Transmitter Fault Indication",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 2, 1,
  },
  {
    "tx_disable",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 0, 1,
  },
};

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
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data,
                                 scmcpld_attr_table, n_attrs);
}

static int scmcpld_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

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

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("SCMCPLD Driver");
MODULE_LICENSE("GPL");

module_init(scmcpld_mod_init);
module_exit(scmcpld_mod_exit);
