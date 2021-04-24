/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include "bic_ipmi.h"
#include "bic_fwupdate.h"
#include "bic_vr_fwupdate.h"
#include "bic_power.h"
#include "error.h"
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <openbmc/kv.h>

#define MAX_RETRY             3
#define BIC_MAX_RETRY         180 // 1 s * 180 = 3 mins

#define IPMB_RETRY_DELAY_TIME 20 // millisecond

#define MAX_CHECK_USB_DEV_TIME 8

#define MAX_USB_MANUFACTURER_LEN 64
#define MAX_USB_PRODUCT_LEN      64

#define MAX_BIC_VER_STR_LEN      32

/*IFX VR pages*/
#define VR_PAGE   0x00
#define VR_PAGE32 0x32
#define VR_PAGE50 0x50
#define VR_PAGE60 0x60
#define VR_PAGE62 0x62

typedef struct {
  struct  libusb_device**          devs;
  struct  libusb_device*           dev;
  struct  libusb_device_handle*    handle;
  struct  libusb_device_descriptor desc;
  char    manufacturer[MAX_USB_MANUFACTURER_LEN];
  char    product[MAX_USB_PRODUCT_LEN];
  int     config;
  int     ci;
  uint8_t epaddr;
  uint8_t path[MAX_PATH_LEN];
} usb_dev;

enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

enum {
  FW_BS_FPGA = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_VR,
  FW_BIOS,
};

// VR vendor
enum {
  VR_ISL = 0x0,
  VR_TI  = 0x1,
  VR_IFX = 0x2,
};

enum {
  FORCE_UPDATE_UNSET = 0x0,
  FORCE_UPDATE_SET,
};

// BIC GPIO
enum {
  PWRGD_BMC_PS_PWROK_R = 0,
  FM_PCH_BMC_THERMTRIP_N,
  IRQ_HSC_ALERT2_N,
  RST_RSTBTN_OUT_N,
  FM_JTAG_TCK_MUX_SEL,
  FM_HSC_TIMER,
  FM_CPU_MEMHOT_OUT_N,
  FM_SPD_DDRCPU_LVLSHFT_EN,
  RST_PLTRST_FROM_PCH_N,
  IRQ_BMC_PRDY_NODE_OD_N,
  FM_CPU_SKTOCC_LVT3_N,          //10
  IRQ_NMI_EVENT_R_N,
  IRQ_SMB_IO_LVC3_STBY_ALRT_N,
  PWRBTN_N,
  A_P3V_BAT_SCALED_EN,
  RST_BMC_R_N,
  FM_MP_PS_FAIL_N,
  FM_CPU_THERMTRIP_LATCH_LVT3_N,
  FM_SLPS4_R_N,
  JTAG_BMC_NTRST_R_N,
  FM_BMC_PCH_SCI_LPC_R_N,        //20
  FM_BMC_DEBUG_ENABLE_N,
  DBP_PRESENT_R2_N,
  PVCCIO_CPU,
  FM_FAST_PROCHOT_EN_N,
  FM_CPU_FIVR_FAULT_LVT3_N,
  CRC_ERROR_R,
  FM_BOARD_REV_ID0,
  FM_BOARD_REV_ID1,
  FM_BOARD_REV_ID2,
  FM_PWRBTN_OUT_N,               //30
  FM_CPU_THERMTRIP_LVT3_N,
  FM_CPU_MSMI_CATERR_LVT3_N,
  IRQ_BMC_PCH_NMI_R,
  FM_BIC_RST_RTCRST,
  PWRGD_CPU_LVC3_R,
  PWRGD_SYS_PWROK,
  BIC_READY,
  HSC_MUX_SWITCH,
  FM_FORCE_ADR_N,
  FM_PEHPCPU_INT,                //40
  FM_MRC_DEBUG_EN,
  BMC_READY,
  FM_THERMTRIP_DLY_TO_PCH,
  FM_BMC_CPU_PWR_DEBUG_N,
  IRQ_PVCCIO_CPU_VRHOT_LVC3_N,
  IRQ_PVDDQ_ABC_VRHOT_LVT3_N,
  IRQ_PVCCIN_CPU_VRHOT_LVC3_N,
  IRQ_SML1_PMBUS_ALERT_N,
  FM_BMC_PREQ_N_NODE_R1,
  FM_MEM_THERM_EVENT_LVT3_N,     //50
  IRQ_PVDDQ_DEF_VRHOT_LVT3_N,
  FM_CPU_ERR0_LVT3_N,
  FM_CPU_ERR1_LVT3_N,
  FAST_PROCHOT_N,
  BMC_JTAG_SEL,
  FM_CPU_ERR2_LVT3_N,
  IRQ_SML0_ALERT_MUX_R_N,
  RST_PLTRST_BMC_N,
  FM_SLPS3_R_N,
  DBP_SYSPWROK_R,                //60
  IRQ_UV_DETECT_N,
  FM_UV_ADR_TRIGGER_EN,
  IRQ_SMI_ACTIVE_BMC_N,
  RST_RSMRST_BMC_N,
  BIC_HEARTBEAT_LED_R,
  IRQ_BMC_PCH_SMI_LPC_R_N,
  FM_BIOS_POST_CMPLT_BMC_N,
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
