/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
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

#ifndef __BIC_H__
#define __BIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bic_xfer.h"
#include "bic_power.h"
#include "bic_fwupdate.h"
#include "bic_ipmi.h"
#include "error.h"
#include <errno.h>

#define MAX_GPIO_PINS 96

enum {
  FW_CPLD = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_VR,
  FW_EXP_CPLD,
};

enum {
  BIC_EOK = 0,
  BIC_ENOTSUP = -ENOTSUP,
  BIC_ENOTREADY = -EAGAIN,
  /* non system errors start from -256 downwards */
};

enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

/* Generic GPIO configuration */
typedef struct _bic_gpio_t {
  uint32_t gpio[3]; 
} bic_gpio_t;

enum {
  PWRGD_BMC_PS_PWROK_R,               // 0
  FM_PCH_BMC_THERMTRIP_N,
  IRQ_HSC_ALERT2_N,
  FM_BMC_ONCTL_R_N,
  RST_RSTBTN_OUT_N,
  FM_JTAG_TCK_MUX_SEL,
  FM_HSC_TIMER,
  FM_CPU_MEMHOT_OUT_N,
  FM_SPD_DDRCPU_LVLSHFT_EN,
  RST_PLTRST_FROM_PCH_N,
  FM_MRC_DEBUG_EN,                    // 10
  IRQ_BMC_PRDY_NODE_OD_N,
  FM_CPU_SKTOCC_LVT3_N,
  SGPIO_BMC_DOUT,
  IRQ_NMI_EVENT_R_N,
  IRQ_SMB_IO_LVC3_STBY_ALRT_N,
  RST_USB_HUB_N,
  A_P3V_BAT_SCALED_EN,
  PWRBTN_N,
  FM_MP_PS_FAIL_N,
  RST_MCP2210_N,                          // 20
  FM_CPU_THERMTRIP_LATCH_LVT3_N,
  FM_BMC_PCHIE_N,
  FM_SLPS4_R_N,
  JTAG_BMC_NTRST_R_N,
  FM_SOL_UART_CH_SEL,
  FM_BMC_PCH_SCI_LPC_N,
  FM_BMC_DEBUG_ENABLE_N,
  DBP_PRESENT_R2_N,
  PVCCIO_CPU,
  PECI_BMC_R,                              // 30
  FM_FAST_PROCHOT_EN_N,
  FM_CPU_FIVR_FAULT_LVT3_N,
  BOARD_ID0,
  BOARD_ID1,
  BOARD_ID2,
  BOARD_ID3,
  FAST_PROCHOT_N,
  BMC_JTAG_SEL,
  FM_PWRBTN_OUT_N,
  FM_CPU_THERMTRIP_LVT3_N,           // 40
  FM_CPU_MSMI_CATERR_LVT3_N,
  IRQ_BMC_PCH_NMI_R,
  FM_BIC_RST_RTCRST,
  SGPIO_BMC_DIN,
  PWRGD_CPU_LVC3_R,
  PWRGD_SYS_PWROK,
  HSC_MUX_SWITCH,
  FM_FORCE_ADR_N,
  FM_THERMTRIP_DLY_TO_PCH,
  FM_BMC_CPU_PWR_DEBUG_N,                 // 50
  SGPIO_BMC_CLK,
  SGPIO_BMC_LD_N,
  IRQ_PVCCIO_CPU_VRHOT_LVC3_N,
  IRQ_PVDDQ_ABC_VRHOT_LVT3_N,
  IRQ_PVCCIN_CPU_VRHOT_LVC3_N,
  IRQ_SML1_PMBUS_ALERT_N,
  HSC_SET_EN,
  FM_BMC_PREQ_N_NODE_R1,
  FM_MEM_THERM_EVENT_LVT3_N,
  IRQ_PVDDQ_DEF_VRHOT_LVT3_N,             // 60
  FM_CPU_ERR0_LVT3_N,
  FM_CPU_ERR1_LVT3_N,
  FM_CPU_ERR2_LVT3_N,
  IRQ_SML0_ALERT_MUX_R_N,
  RST_PLTRST_BMC_N,
  FM_SLPS3_R_N,
  DBP_SYSPWROK_R,
  IRQ_UV_DETECT_N,
  FM_UV_ADR_TRIGGER_EN,
  IRQ_SMI_ACTIVE_BMC_N,               // 70
  RST_RSMRST_BMC_N,
  BMC_HARTBEAT_LED_R,
  IRQ_BMC_PCH_SMI_LPC_R_N,
  FM_BIOS_POST_CMPLT_BMC_N,
  RST_BMC_R_N,
  BMC_READY,      //76
  BIC_READY,
  FM_PEHPCPU_INT,
};

int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
