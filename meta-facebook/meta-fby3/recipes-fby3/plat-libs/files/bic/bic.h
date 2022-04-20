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
#define RETRY_3_TIME 3
#define RETRY_TIME 10
#define IPMB_RETRY_DELAY_TIME 500

#define PRESENT_2U_TOP 0x01
#define PRESENT_2U_BOT 0x02

#define MAX_CHECK_DEVICE_TIME 8

#define SW_USB_HUB_DELAY 5 //seconds

/*GPIO USB HUB reset pin*/
enum {
  EXP_GPIO_RST_USB_HUB   = 9,
  GPIO_RST_USB_HUB       = 16,
  CWC_GPIO_RST_USB_HUB   = 57,
  CWC_GPIO_USB_MUX       = 58,
  GPV3_GPIO_RST_USB_HUB1 = 93,
  GPV3_GPIO_RST_USB_HUB2 = 94,
  GPV3_GPIO_RST_USB_HUB3 = 98,
};

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

#define REVISION_ID(x) ((x >> 4) & 0x0f)
#define COMPONENT_ID(x) (x & 0x0f)

#define CWC_CPLD_ADDRESS 0xA0
#define SLOT_BUS_BASE 3
#define CPLD_BOARD_PVT_REV 3
#define CPLD_FLAG_REG_ADDR 0x1F
#define CPLD_BB_BUS 0x01
#define CPLD_SB_BUS 0x05
/*Revision Number:
  BOARD_REV_EVT = 2
  BOARD_REV_DVT = 3
  BOARD_REV_PVT = 4
  BOARD_REV_MP  = 5
  FW_REV_EVT = 1
  FW_REV_DVT = 2
  FW_REV_PVT = 3
  FW_REV_MP  = 4
*/

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00200020)
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG  (ON_CHIP_FLASH_IP_CSR_BASE + 0x0)
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG    (ON_CHIP_FLASH_IP_CSR_BASE + 0x4)
#define ON_CHIP_FLASH_USER_VER           (0x00200028)
#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)

// MAX10 10M25 Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00200000)
#define MAX10M25_CFM1_START_ADDR         (0x00064000)
#define MAX10M25_CFM1_END_ADDR           (0x000BFFFF)
#define MAX10M25_CFM2_START_ADDR         (0x00008000)
#define MAX10M25_CFM2_END_ADDR           (0x00063FFF)

// MAX10 10M08 Dual-boot IP
#define MAX10M08_CFM1_START_ADDR         (0x0002B000)
#define MAX10M08_CFM1_END_ADDR           (0x0004DFFF)

extern const char *board_stage[];

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
  FW_VR,    // 5
  FW_BIOS,
  FW_1OU_BIC,
  FW_1OU_BIC_BOOTLOADER,
  FW_1OU_CPLD,
  FW_2OU_BIC, // 10
  FW_2OU_BIC_BOOTLOADER,
  FW_2OU_CPLD,
  FW_BB_BIC,
  FW_BB_BIC_BOOTLOADER,
  FW_BB_CPLD, // 15
  FW_BIOS_CAPSULE,
  FW_CPLD_CAPSULE,
  FW_BIOS_RCVY_CAPSULE,
  FW_CPLD_RCVY_CAPSULE,
  FW_2OU_3V3_VR1,  // 20
  FW_2OU_3V3_VR2,
  FW_2OU_3V3_VR3,
  FW_2OU_1V8_VR,
  FW_2OU_PESW_VR,
  FW_2OU_PESW_CFG_VER, // 25
  FW_2OU_PESW_FW_VER,
  FW_2OU_PESW_BL0_VER,
  FW_2OU_PESW_BL1_VER,
  FW_2OU_PESW_PART_MAP0_VER,
  FW_2OU_PESW_PART_MAP1_VER,
  FW_2OU_PESW,
  FW_2OU_M2_DEV0,
  FW_2OU_M2_DEV1,
  FW_2OU_M2_DEV2,
  FW_2OU_M2_DEV3,
  FW_2OU_M2_DEV4,
  FW_2OU_M2_DEV5,
  FW_2OU_M2_DEV6,
  FW_2OU_M2_DEV7,
  FW_2OU_M2_DEV8,
  FW_2OU_M2_DEV9,
  FW_2OU_M2_DEV10,
  FW_2OU_M2_DEV11,
  FW_CWC_BIC,
  FW_CWC_BIC_BL,
  FW_CWC_CPLD,
  FW_CWC_PESW,
  FW_GPV3_TOP_BIC,
  FW_GPV3_TOP_BIC_BL,
  FW_GPV3_TOP_CPLD,
  FW_GPV3_TOP_PESW,
  FW_GPV3_BOT_BIC,
  FW_GPV3_BOT_BIC_BL,
  FW_GPV3_BOT_CPLD,
  FW_GPV3_BOT_PESW,
  FW_CWC_PESW_VR,
  FW_GPV3_TOP_PESW_VR,
  FW_GPV3_BOT_PESW_VR,
  FW_2U_TOP_PESW_CFG_VER,
  FW_2U_TOP_PESW_FW_VER,
  FW_2U_TOP_PESW_BL0_VER,
  FW_2U_TOP_PESW_BL1_VER,
  FW_2U_TOP_PESW_PART_MAP0_VER,
  FW_2U_TOP_PESW_PART_MAP1_VER,
  FW_2U_BOT_PESW_CFG_VER,
  FW_2U_BOT_PESW_FW_VER,
  FW_2U_BOT_PESW_BL0_VER,
  FW_2U_BOT_PESW_BL1_VER,
  FW_2U_BOT_PESW_PART_MAP0_VER,
  FW_2U_BOT_PESW_PART_MAP1_VER,
  FW_2U_TOP_3V3_VR1,
  FW_2U_TOP_3V3_VR2,
  FW_2U_TOP_3V3_VR3,
  FW_2U_TOP_1V8_VR,
  FW_2U_BOT_3V3_VR1,
  FW_2U_BOT_3V3_VR2,
  FW_2U_BOT_3V3_VR3,
  FW_2U_BOT_1V8_VR,
  FW_TOP_M2_DEV0,
  FW_TOP_M2_DEV1,
  FW_TOP_M2_DEV2,
  FW_TOP_M2_DEV3,
  FW_TOP_M2_DEV4,
  FW_TOP_M2_DEV5,
  FW_TOP_M2_DEV6,
  FW_TOP_M2_DEV7,
  FW_TOP_M2_DEV8,
  FW_TOP_M2_DEV9,
  FW_TOP_M2_DEV10,
  FW_TOP_M2_DEV11,
  FW_BOT_M2_DEV0,
  FW_BOT_M2_DEV1,
  FW_BOT_M2_DEV2,
  FW_BOT_M2_DEV3,
  FW_BOT_M2_DEV4,
  FW_BOT_M2_DEV5,
  FW_BOT_M2_DEV6,
  FW_BOT_M2_DEV7,
  FW_BOT_M2_DEV8,
  FW_BOT_M2_DEV9,
  FW_BOT_M2_DEV10,
  FW_BOT_M2_DEV11,
  // last id
  FW_COMPONENT_LAST_ID
};

enum {
  VR_ISL = 0x0, // Renesas vr vendor ID
  VR_TI  = 0x1,
  VR_IFX = 0x2, // Infineon vr vendor ID
  VR_VY  = 0x3, // Vishay vr vendor ID
  VR_ON  = 0x4, // Onsemi vr vendor ID
  IFX_DEVID_LEN = 0x2,
  ISL_DEVID_LEN = 0x4,
  TI_DEVID_LEN  = 0x6,

  /*----VR ADDR-----*/
  VCCIN_ADDR = 0xC0,
  VCCIO_ADDR = 0xC4,
  VDDQ_ABC_ADDR = 0xC8,
  VDDQ_DEF_ADDR = 0xCC,
  VR_PESW_ADDR = 0xC8,
  VR_2OU_P3V3_STBY1 = 0x28,
  VR_2OU_P3V3_STBY2 = 0x2E,
  VR_2OU_P3V3_STBY3 = 0x30,
  VR_2OU_P1V8       = 0x36,
};

// M2 info
enum {
  MEFF_DUAL_M2 = 0xF0,
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
  uint32_t gpio[4]; 
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

enum {
  DP_RETIMER_X4 = 0x00,
  DP_RETIMER_X8 = 0x01,
  DP_RETIMER_X16 = 0x04,
  DP_PCIE_X4 = 0x0e,
  DP_PCIE_X8 = 0x0d,
  DP_PCIE_X16 = 0x0b,
  DP_PCIE_UNKNOW = 0xff,
};

int active_config(struct libusb_device *dev,struct libusb_device_handle *handle);
int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio, uint8_t intf);
int bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt);
int bic_mux_select(uint8_t slot_id, uint8_t bus, uint8_t dev_id, uint8_t intf);
int bic_disable_brcm_parity_init(uint8_t slot_id, uint8_t comp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
