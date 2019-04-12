/* Copyright 2015-present Facebook. All Rights Reserved.
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
#include "fby2_gpio.h"

// List of GPIO pins to be monitored
const uint8_t gpio_pin_list[] = {
  PWRGD_COREPWR,
  PWRGD_PCH_PWROK,
  PVDDR_AB_VRHOT_N,
  PVDDR_DE_VRHOT_N,
  PVCCIN_VRHOT_N,
  FM_THROTTLE_N,
  FM_PCH_BMC_THERMTRIP_N,
  H_MEMHOT_CO_N,
  FM_CPU0_THERMTRIP_LVT3_N,
  CPLD_PCH_THERMTRIP,
  FM_CPLD_FIVR_FAULT,
  FM_CPU_CATERR_N,
  FM_CPU_ERROR_2,
  FM_CPU_ERROR_1,
  FM_CPU_ERROR_0,
  FM_SLPS4_N,
  FM_NMI_EVENT_BMC_N,
  FM_SMI_BMC_N,
  PLTRST_N,
  FP_RST_BTN_N,
  RST_BTN_BMC_OUT_N,
  FM_BIOS_POST_COMPT_N,
  FM_SLPS3_N,
  PWRGD_PVCCIN,
  FM_BACKUP_BIOS_SEL_N,
  FM_EJECTOR_LATCH_DETECT_N,
  BMC_RESET,
  FM_JTAG_BIC_TCK_MUX_SEL_N,
  BMC_READY_N,
  BMC_COM_SW_N,
  RST_I2C_MUX_N,
  XDP_BIC_PREQ_N,
  XDP_BIC_TRST,
  FM_SYS_THROTTLE_LVC3,
  XDP_BIC_PRDY_N,
  XDP_PRSNT_IN_N,
  XDP_PRSNT_OUT_N,
  XDP_BIC_PWR_DEBUG_N,
  FM_BIC_JTAG_SEL_N,
  FM_PCIE_BMC_RELINK_N,
  FM_DISABLE_PCH_VR,
  FM_BIC_RST_RTCRST,
  FM_BIC_ME_RCVR,
  RST_RSMRST_PCH_N,
};

size_t gpio_pin_cnt = sizeof(gpio_pin_list)/sizeof(uint8_t);
const uint32_t gpio_ass_val = 0x0 | (1 << FM_CPLD_FIVR_FAULT);

// GPIO name
const char *gpio_pin_name[] = {
  "PWRGD_COREPWR",
  "PWRGD_PCH_PWROK",
  "PVDDR_AB_VRHOT_N",
  "PVDDR_DE_VRHOT_N",
  "PVCCIN_VRHOT_N",
  "FM_THROTTLE_N",
  "FM_PCH_BMC_THERMTRIP_N",
  "H_MEMHOT_CO_N",
  "FM_CPU0_THERMTRIP_LVT3_N",
  "CPLD_PCH_THERMTRIP",
  "FM_CPLD_FIVR_FAULT",
  "FM_CPU_CATERR_N",
  "FM_CPU_ERROR_2",
  "FM_CPU_ERROR_1",
  "FM_CPU_ERROR_0",
  "FM_SLPS4_N",
  "FM_NMI_EVENT_BMC_N",
  "FM_SMI_BMC_N",
  "PLTRST_N",
  "FP_RST_BTN_N",
  "RST_BTN_BMC_OUT_N",
  "FM_BIOS_POST_COMPT_N",
  "FM_SLPS3_N",
  "PWRGD_PVCCIN",
  "FM_BACKUP_BIOS_SEL_N",
  "FM_EJECTOR_LATCH_DETECT_N",
  "BMC_RESET",
  "FM_JTAG_BIC_TCK_MUX_SEL_N",
  "BMC_READY_N",
  "BMC_COM_SW_N",
  "RST_I2C_MUX_N",
  "XDP_BIC_PREQ_N",
  "XDP_BIC_TRST",
  "FM_SYS_THROTTLE_LVC3",
  "XDP_BIC_PRDY_N",
  "XDP_PRSNT_IN_N",
  "XDP_PRSNT_OUT_N",
  "XDP_BIC_PWR_DEBUG_N",
  "FM_BIC_JTAG_SEL_N",
  "FM_PCIE_BMC_RELINK_N",
  "FM_DISABLE_PCH_VR",
  "FM_BIC_RST_RTCRST",
  "FM_BIC_ME_RCVR",
  "RST_RSMRST_PCH_N",
};

int
fby2_get_gpio_name(uint8_t fru, uint8_t gpio, char *name) {

  //TODO: Add support for BMC GPIO pins
  if (fru < 1 || fru > 4) {
    syslog(LOG_WARNING, "%s: Wrong fru %u", __func__, fru);
    return -1;
  }

  if (gpio < 0 || gpio > MAX_GPIO_PINS) {
    syslog(LOG_WARNING, "%s: Wrong gpio pin %u", __func__, gpio);
    return -1;
  }

  sprintf(name, "%s", gpio_pin_name[gpio]);
  return 0;
}
