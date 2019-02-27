/* Copyright 2018-present Facebook. All Rights Reserved.
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
#include "minipack_gpio.h"

// List of GPIO pins to be monitored
const uint8_t gpio_pin_list[] = {
  PWRGOOD_CPU,
  PWRGD_PCH_PWROK,
  PVDDR_VRHOT_N,
  PVCCIN_VRHOT_N,
  FM_FAST_PROCHOT_N,
  PCHHOT_CPU_N,
  FM_CPLD_CPU_DIMM_EVENT_CO_N,
  FM_CPLD_BDXDE_THERMTRIP_N,
  THERMTRIP_PCH_N,
  FM_CPLD_FIVR_FAULT,
  FM_BDXDE_CATERR_LVT3_N,
  FM_BDXDE_ERR2_LVT3_N,
  FM_BDXDE_ERR1_LVT3_N,
  FM_BDXDE_ERR0_LVT3_N,
  SLP_S4_N,
  FM_NMI_EVENT_BMC_N,
  FM_SMI_BMC_N,
  RST_PLTRST_BMC_N,
  FP_RST_BTN_BUF_N,
  BMC_RST_BTN_OUT_N,
  FM_BDE_POST_CMPLT_N,
  FM_BDXDE_SLP3_N,
  FM_PWR_LED_N,
  PWRGD_PVCCIN,
  SVR_ID0,
  SVR_ID1,
  SVR_ID2,
  SVR_ID3,
  BMC_READY_N,
  BMC_COM_SW_N,
  //RESERVED_30,
  //RESERVED_31,
};

size_t gpio_pin_cnt = sizeof(gpio_pin_list)/sizeof(uint8_t);
const uint32_t gpio_ass_val = 0x0 | (1 << FM_CPLD_FIVR_FAULT);

#define BIC_GPIO_DEF(type, name)  [type] = name
static const char *gpio_pin_names[BIC_GPIO_MAX] = {
  BIC_GPIO_LIST,
};
#undef BIC_GPIO_DEF

const char* minipack_gpio_type_to_name(uint8_t gpio) {

  if ((gpio < BIC_GPIO_MAX) && (gpio_pin_names[gpio] != NULL)) {
    return gpio_pin_names[gpio];
  }

  return "";
}
