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
#include <libusb-1.0/libusb.h>

#define MAX_GPIO_PINS 96

#define PRESENT_1OU 1
#define PRESENT_2OU 2
#define DEV_ID_1U 11
#define DEV_ID_2U 12
#define RETRY_TIME 10
#define IPMB_RETRY_DELAY_TIME 500

#define MAX_CHECK_DEVICE_TIME 8
#define GPIO_RST_USB_HUB 0x10
#define EXP_GPIO_RST_USB_HUB 0x9
#define VALUE_LOW 0
#define VALUE_HIGH 1

/*IFX VR pages*/
#define VR_PAGE   0x00
#define VR_PAGE32 0x32
#define VR_PAGE50 0x50
#define VR_PAGE60 0x60
#define VR_PAGE62 0x62

#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

typedef struct
{
  struct libusb_device**          devs;
  struct libusb_device*           dev;
  struct libusb_device_handle*    handle;
  struct libusb_device_descriptor desc;
  char    manufacturer[64];
  char    product[64];
  int     config;
  int     ci;
  uint8_t epaddr;
  uint8_t path[8];
} usb_dev;

enum {
  FW_CPLD = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_VR,
  FW_BIOS,
  FW_1OU_BIC,
  FW_1OU_BIC_BOOTLOADER,
  FW_1OU_CPLD,
  FW_2OU_BIC,
  FW_2OU_BIC_BOOTLOADER,
  FW_2OU_CPLD,
  FW_BB_BIC,
  FW_BB_BIC_BOOTLOADER,
  FW_BB_CPLD,
  FW_BIOS_CAPSULE,
  FW_CPLD_CAPSULE,
  FW_BIOS_RCVY_CAPSULE,
  FW_CPLD_RCVY_CAPSULE,

  // last id
  FW_COMPONENT_LAST_ID
};

enum {
  VR_ISL = 0x0,
  VR_TI  = 0x1,
  VR_IFX = 0x2,
  IFX_DEVID_LEN = 0x2,
  ISL_DEVID_LEN = 0x4,
  TI_DEVID_LEN  = 0x6,

  /*----VR ADDR-----*/
  VCCIN_ADDR = 0xC0,
  VCCIO_ADDR = 0xC4,
  VDDQ_ABC_ADDR = 0xC8,
  VDDQ_DEF_ADDR = 0xCC,
};

enum {
  FORCE_UPDATE_UNSET = 0x0,
  FORCE_UPDATE_SET,
};

enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

/* Generic GPIO configuration */
typedef struct _bic_gpio_t {
  uint32_t gpio[3]; 
} bic_gpio_t;

typedef struct _bic_gpio_config_t {
  uint8_t dir:1;
  uint8_t ie:1;
  uint8_t edge:1;
  uint8_t trig:2;
} bic_gpio_config_t;

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

int active_config(struct libusb_device *dev,struct libusb_device_handle *handle);
int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio, uint8_t intf);
int bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
