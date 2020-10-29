/* Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include "fbgc_gpio.h"

const char *gpio_pin_name[] = {
  "FPGA_CRCERROR",
  "FPGA_NCONFIG",
  "BMC_NIC_PWRBRK_R",
  "BMC_NIC_SMRST_N_R",
  "NIC_WAKE_BMC_N",
  "BMC_NIC_FULL_PWR_EN_R",
  "NIC_FULL_PWR_PG",
  "BOARD_REV_ID0",
  "BOARD_REV_ID1",
  "BOARD_REV_ID2",
  "EN_ASD_DEBUG",
  "DEBUG_RST_BTN_N",
  "E1S_1_P3V3_PG_R",
  "E1S_2_P3V3_PG_R",
  "BMC_FPGA_UART_SEL0_R",
  "BMC_FPGA_UART_SEL1_R",
  "BMC_FPGA_UART_SEL2_R",
  "BMC_FPGA_UART_SEL3_R",
  "DEBUG_BMC_UART_SEL_R",
  "DEBUG_BMC_1",
  "DEBUG_BMC_2",
  "DEBUG_BMC_3",
  "DEBUG_BMC_4",
  "DEBUG_BMC_5",
  "DEBUG_BMC_6",
  "USB_OC_N1",
  "SCC_I2C_EN_R",
  "BMC_READY_R",
  "LED_POSTCODE_0",
  "LED_POSTCODE_1",
  "LED_POSTCODE_2",
  "LED_POSTCODE_3",
  "LED_POSTCODE_4",
  "LED_POSTCODE_5",
  "LED_POSTCODE_6",
  "LED_POSTCODE_7",
  "BMC_LOC_HEARTBEAT_R",
  "BIC_READY_IN",
  "COMP_STBY_PG_IN",
  "UIC_LOC_ID_IN",
  "UIC_RMT_ID_IN",
  "BMC_COMP_PWR_EN_R",
  "EXT_MINISAS_INS_OR_N_IN",
  "NIC_PRSNTB3_N",
  "FM_BMC_TPM_PRSNT_N",
  "DEBUG_CARD_PRSNT_N",
  "BMC_RST_BTN_N_R",
  "PCIE_COMP_UIC_RST_N",
  "BMC_COMP_SYS_RST_N_R",
  "BMC_LED_STATUS_BLUE_EN_R",
  "BMC_LED_STATUS_YELLOW_EN_R",
  "BMC_LED_PWR_BTN_EN_R",
  "UIC_STBY_PG",
  "PWRGD_NIC_BMC",
  "EMMC_PRESENT_N",
  "HSC_P12V_DPB_FAULT_N_IN",
  "HSC_COMP_FAULT_N_IN",
  "SCC_FAULT_N_IN",
  "EMMC_RST_N",
};

const uint8_t gpio_pin_size = sizeof(gpio_pin_name)/sizeof(gpio_pin_name[0]);

uint8_t
fbgc_get_gpio_list_size(void) {
  return gpio_pin_size;
}

const char *
fbgc_get_gpio_name(uint8_t gpio) {
  if (gpio >= MAX_GPIO_PINS || gpio >= gpio_pin_size) {
    syslog(LOG_WARNING, "%s() Invalid gpio number: %u\n", __func__, gpio);
    return "";
  }

  return gpio_pin_name[gpio];
}
