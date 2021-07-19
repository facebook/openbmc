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
#include "fby3_gpio.h"

//BIC GPIO 
const char *gpio_pin_name_sb[] = {
  "PWRGD_BMC_PS_PWROK_R",               // 0
  "FM_PCH_BMC_THERMTRIP_N",
  "IRQ_HSC_ALERT2_N",
  "FM_BMC_ONCTL_R_N",
  "RST_RSTBTN_OUT_N",
  "FM_JTAG_TCK_MUX_SEL",
  "FM_HSC_TIMER",
  "FM_CPU_MEMHOT_OUT_N",
  "FM_SPD_DDRCPU_LVLSHFT_EN",
  "RST_PLTRST_FROM_PCH_N",
  "FM_MRC_DEBUG_EN",                    // 10
  "IRQ_BMC_PRDY_NODE_OD_N",
  "FM_CPU_SKTOCC_LVT3_N",
  "SGPIO_BMC_DOUT",
  "IRQ_NMI_EVENT_R_N",
  "IRQ_SMB_IO_LVC3_STBY_ALRT_N",
  "RST_USB_HUB_N",
  "A_P3V_BAT_SCALED_EN",
  "PWRBTN_N",
  "FM_MP_PS_FAIL_N",
  "FM_PEHPCPU_INT",                          // 20
  "FM_CPU_THERMTRIP_LATCH_LVT3_N",
  "FM_BMC_PCHIE_N",
  "FM_SLPS4_R_N",
  "JTAG_BMC_NTRST_R_N",
  "FM_SOL_UART_CH_SEL",
  "FM_BMC_PCH_SCI_LPC_N",
  "FM_BMC_DEBUG_ENABLE_N",
  "DBP_PRESENT_R2_N",
  "PVCCIO_CPU",
  "BIC_READY",                              // 30
  "FM_FAST_PROCHOT_EN_N",
  "FM_CPU_FIVR_FAULT_LVT3_N",
  "BOARD_ID0",
  "BOARD_ID1",
  "BOARD_ID2",
  "BOARD_ID3",
  "FAST_PROCHOT_N",
  "BMC_JTAG_SEL",
  "FM_PWRBTN_OUT_N",
  "FM_CPU_THERMTRIP_LVT3_N",           // 40
  "FM_CPU_MSMI_CATERR_LVT3_N",
  "IRQ_BMC_PCH_NMI_R",
  "FM_BIC_RST_RTCRST",
  "SGPIO_BMC_DIN",
  "PWRGD_CPU_LVC3_R",
  "PWRGD_SYS_PWROK",
  "HSC_MUX_SWITCH",
  "FM_FORCE_ADR_N",
  "FM_THERMTRIP_DLY_TO_PCH",
  "FM_BMC_CPU_PWR_DEBUG_N",                 // 50
  "SGPIO_BMC_CLK",
  "SGPIO_BMC_LD_N",
  "IRQ_PVCCIO_CPU_VRHOT_LVC3_N",
  "IRQ_PVDDQ_ABC_VRHOT_LVT3_N",
  "IRQ_PVCCIN_CPU_VRHOT_LVC3_N",
  "IRQ_SML1_PMBUS_ALERT_N",
  "HSC_SET_EN",
  "FM_BMC_PREQ_N_NODE_R1",
  "FM_MEM_THERM_EVENT_LVT3_N",
  "IRQ_PVDDQ_DEF_VRHOT_LVT3_N",             // 60
  "FM_CPU_ERR0_LVT3_N",
  "FM_CPU_ERR1_LVT3_N",
  "FM_CPU_ERR2_LVT3_N",
  "IRQ_SML0_ALERT_MUX_R_N",
  "RST_PLTRST_BMC_N",
  "FM_SLPS3_R_N",
  "DBP_SYSPWROK_R",
  "IRQ_UV_DETECT_N",
  "FM_UV_ADR_TRIGGER_EN",
  "IRQ_SMI_ACTIVE_BMC_N",               // 70
  "RST_RSMRST_BMC_N",
  "BMC_HARTBEAT_LED_R",
  "IRQ_BMC_PCH_SMI_LPC_R_N",
  "FM_BIOS_POST_CMPLT_BMC_N",
  "RST_BMC_R_N",
  "BMC_READY",
};

const char *gpio_pin_name_2ou[] = {
  "BIC_HEARTBEAT_LED_N",		// 0
  "BIC_OCP_DISABLE",
  "BIC_READY",
  "VR_ALRT_BIC_DETECT_N",
  "SSD1_PRSNT_N",
  "SSD0_PRSNT_N",
  "PESW_SYS_ERROR_3V3_N",
  "BOARD_ID_0",
  "BOARD_ID_1",
  "BOARD_ID_2",
  "BOARD_ID_3",			// 10
  "CART_TYPE_DETECTION0",
  "CART_TYPE_DETECTION1",
  "CART_TYPE_DETECTION2",
  "FM_BIC_PESW_MULTI_CONFIG0",
  "FM_BIC_PESW_MULTI_CONFIG1",
  "FM_BIC_PESW_MULTI_CONFIG2",
  "FM_BIC_PESW_MULTI_CONFIG3",
  "FM_BIC_PESW_MULTI_CONFIG4",
  "FM_BIC_PESW_RECOVERY_0",
  "FM_BIC_PESW_RECOVERY_1",		// 20
  "FM_P12V_E1S_A_BIC_CPLD_EN",
  "FM_P12V_E1S_B_BIC_CPLD_EN",
  "I2C_SSD_A_RST_N_R",
  "I2C_SSD_B_RST_N_R",
  "FM_P3V3_M2A_BIC_CPLD_EN_R",
  "FM_P3V3_M2B_BIC_CPLD_EN_R",
  "FM_P3V3_M2C_BIC_CPLD_EN_R",
  "FM_P3V3_M2D_BIC_CPLD_EN_R",
  "FM_P3V3_M2E_BIC_CPLD_EN_R",
  "FM_P3V3_M2F_BIC_CPLD_EN_R",	// 30
  "FM_P3V3_M2G_BIC_CPLD_EN_R",
  "FM_P3V3_M2H_BIC_CPLD_EN_R",
  "FM_P3V3_M2I_BIC_CPLD_EN_R",
  "FM_P3V3_M2J_BIC_CPLD_EN_R",
  "FM_P3V3_M2K_BIC_CPLD_EN_R",
  "FM_P3V3_M2L_BIC_CPLD_EN_R",
  "FM_PLD_VCCIO_2_4_5_P1V8",
  "FM_PLD_VCCIO_2_4_5_P3V3SB",
  "FM_POWER_EN",
  "JTAG_BIC_M2_TRST_R",		// 40
  "LED_E1S_A_BIC_N",
  "LED_E1S_B_BIC_N",
  "PWRGD_P0V84",
  "PWRGD_P12V_E1S_A",
  "PWRGD_P12V_E1S_B",
  "PWRGD_P1V8",
  "PWRGD_P3V3_E1S_A",
  "PWRGD_P3V3_E1S_B",
  "TYPE_C_DETECT_R",
  "PWRGD_P3V3_M2A",			// 50
  "PWRGD_P3V3_M2B",
  "PWRGD_P3V3_M2C",
  "PWRGD_P3V3_M2D",
  "PWRGD_P3V3_M2E",
  "PWRGD_P3V3_M2F",
  "PWRGD_P3V3_M2G",
  "PWRGD_P3V3_M2H",
  "PWRGD_P3V3_M2I",
  "PWRGD_P3V3_M2J",
  "PWRGD_P3V3_M2K",			// 60
  "PWRGD_P3V3_M2L",
  "PWRGD_P3V3_STBY_1",
  "PWRGD_P3V3_STBY_2",
  "PWRGD_P3V3_STBY_3",
  "RST_I2C_MUX1_N",
  "SMB_M2A_ALERT_LVC_N",
  "SMB_M2B_ALERT_LVC_N",
  "SMB_M2C_ALERT_LVC_N",
  "SMB_M2D_ALERT_LVC_N",
  "SMB_M2E_ALERT_LVC_N",		// 70
  "SMB_M2F_ALERT_LVC_N",
  "SMB_M2G_ALERT_LVC_N",
  "SMB_M2H_ALERT_LVC_N",
  "SMB_M2I_ALERT_LVC_N",
  "SMB_M2J_ALERT_LVC_N",
  "SMB_M2K_ALERT_LVC_N",
  "SMB_M2L_ALERT_LVC_N",
  "SMB_M2A_INA233_ALRT_N",
  "SMB_M2B_INA233_ALRT_N",
  "SMB_M2C_INA233_ALRT_N",		// 80
  "SMB_M2D_INA233_ALRT_N",
  "SMB_M2E_INA233_ALRT_N",
  "SMB_M2F_INA233_ALRT_N",
  "SMB_M2G_INA233_ALRT_N",
  "SMB_M2H_INA233_ALRT_N",
  "SMB_M2I_INA233_ALRT_N",
  "SMB_M2J_INA233_ALRT_N",
  "SMB_M2K_INA233_ALRT_N",
  "SMB_M2L_INA233_ALRT_N",
  "SMB_SSD0_INA233_ALRT_N",		// 90
  "SMB_SSD1_INA233_ALRT_N",
  "TMP75_ALERT_N",
  "USB_HUB1_3_RESET_N",
  "USB_HUB2_RESET_N",
  "UART_MUX_BIC_RESET_R_N",
  "BIC_REMOTE_DEBUG_SELECT_N",
  "PWRGD_P5V_STBY_R",
};

const uint8_t gpio_pin_size_sb = sizeof(gpio_pin_name_sb)/sizeof(gpio_pin_name_sb[0]);
const uint8_t gpio_pin_size_2ou = sizeof(gpio_pin_name_2ou)/sizeof(gpio_pin_name_2ou[0]);

uint8_t
fby3_get_gpio_list_size(uint8_t intf) {
  switch(intf) {
    case NONE_INTF:
      return gpio_pin_size_sb;
    case REXP_BIC_INTF:
      return gpio_pin_size_2ou;
    default:
      return 0;
  }
}

int
fby3_get_gpio_name(uint8_t fru, uint8_t gpio, char *name, uint8_t intf) {
  uint8_t max_gpio_pins = 0;
  const char **gpio_name_table;

  switch(intf) {
    case NONE_INTF:
      max_gpio_pins = gpio_pin_size_sb;
      gpio_name_table = gpio_pin_name_sb;
      break;
    case REXP_BIC_INTF:
      max_gpio_pins = gpio_pin_size_2ou;
      gpio_name_table = gpio_pin_name_2ou;
      break;
    default:
      syslog(LOG_WARNING, "%s interface %u not supported", __func__, intf);
      return -1;
  }
  //TODO: Add support for BMC GPIO pins
  if (fru < 1 || fru > 4) {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby3_get_gpio_name: Wrong fru %u", fru);
#endif
    return -1;
  }

  if (gpio < 0 || gpio >= max_gpio_pins) {
#ifdef DEBUG
    syslog(LOG_WARNING, "fby3_get_gpio_name: Wrong gpio pin %u", gpio);
#endif
    return -1;
  }

  sprintf(name, "%s", gpio_name_table[gpio]);
  return 0;
}
