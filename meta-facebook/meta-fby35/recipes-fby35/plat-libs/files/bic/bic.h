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

#include <libusb-1.0/libusb.h>
#include "bic_fwupdate.h"

#define MAX_GPIO_PINS 67

#define PRESENT_1OU 1
#define PRESENT_2OU 2
#define RETRY_3_TIME 3
#define RETRY_TIME 10
#define IPMB_RETRY_TIME 3
#define IPMB_RETRY_DELAY 100

#define MAX_CHECK_DEVICE_TIME 8
#define GPIO_RST_USB_HUB 0x10
#define EXP_GPIO_RST_USB_HUB 0x9
#define VALUE_LOW 0
#define VALUE_HIGH 1

#define MAX_VER_STR_LEN 80

/* IFX VR */
enum {
  PMBUS_STS_CML       = 0x7E,
  IFX_MFR_AHB_ADDR    = 0xCE,
  IFX_MFR_REG_WRITE   = 0xDE,
  IFX_MFR_REG_READ    = 0xDF,
  IFX_MFR_FW_CMD_DATA = 0xFD,
  IFX_MFR_FW_CMD      = 0xFE,

  OTP_PTN_RMNG  = 0x10,
  OTP_CONF_STO  = 0x11,
  OTP_FILE_INVD = 0x12,
  GET_CRC       = 0x2D,
};

#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

#define NIC_CPLD_BUS 9
#define BB_CPLD_BUS 12
#define CPLD_ADDRESS 0x1E
#define SLOT_BUS_BASE 3
#define BB_CPLD_BOARD_REV_ID_REGISTER 0x08
#define SB_CPLD_BOARD_REV_ID_REGISTER 0x07
#define CPLD_BOARD_PVT_REV 3
#define CPLD_FLAG_REG_ADDR 0x1F
#define CPLD_BB_BUS 0x01
#define CPLD_SB_BUS 0x01
#define SB_CPLD_ADDRESS_UPDATE 0x40
#define SB_USB_VENDOR_ID  0x1D6B
#define SB_USB_PRODUCT_ID 0x0104
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
  VR_ISL = 0x0,
  VR_TI  = 0x1,
  VR_IFX = 0x2,
  VR_VY  = 0x3,
  IFX_DEVID_LEN = 0x2,
  ISL_DEVID_LEN = 0x4,
  TI_DEVID_LEN  = 0x6,

  /*----VR ADDR-----*/
  VCCIN_ADDR = 0xC0,
  VCCD_ADDR = 0xC4,
  VCCINFAON_ADDR = 0xEC,
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

enum {
  //BIC to BMC
  USB_INPUT_PORT = 0x3,
  USB_OUTPUT_PORT = 0x82,
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
  FM_BMC_PCH_SCI_LPC_R_N,         //0
  FM_BIOS_POST_CMPLT_BMC_N,
  FM_SLPS3_PLD_N,
  IRQ_BMC_PCH_SMI_LPC_R_N,
  IRQ_UV_DETECT_N,
  FM_UV_ADR_TRIGGER_EN_R,
  IRQ_SMI_ACTIVE_BMC_N,
  HSC_SET_EN_R,
  FM_BIC_RST_RTCRST_R,
  RST_USB_HUB_N_R,
  A_P3V_BAT_SCALED_EN_R,          //10
  FM_SPI_PCH_MASTER_SEL_R,
  FM_PCHHOT_N,
  FM_SLPS4_PLD_N,
  FM_S3M_CPU0_CD_INIT_ERROR,
  PWRGD_SYS_PWROK,
  FM_HSC_TIMER,
  IRQ_SMB_IO_LVC3_STBY_ALRT_N,
  IRQ_CPU0_VRHOT_N,
  DBP_CPU_PREQ_BIC_N,
  FM_CPU_THERMTRIP_LATCH_LVT3_N,  //20
  FM_CPU_SKTOCC_LVT3_PLD_N,
  H_CPU_MEMHOT_OUT_LVC3_N,
  RST_PLTRST_PLD_N,
  PWRBTN_N,
  RST_BMC_R_N,
  H_BMC_PRDY_BUF_N,
  BMC_READY,
  BIC_READY,
  FM_SOL_UART_CH_SEL_R,
  HSC_MUX_SWITCH_R,               //30
  FM_FORCE_ADR_N_R,
  PWRGD_CPU_LVC3,
  FM_PCH_BMC_THERMTRIP_N,
  FM_THROTTLE_R_N,
  IRQ_HSC_ALERT2_N,
  SMB_SENSOR_LVC3_ALERT_N,
  FM_CPU_RMCA_CATERR_LVT3_N,
  SYS_PWRBTN_N,
  RST_PLTRST_BUF_N,
  IRQ_BMC_PCH_NMI_R,              //40
  IRQ_SML1_PMBUS_ALERT_N,
  IRQ_PCH_CPU_NMI_EVENT_N,
  FM_BMC_DEBUG_ENABLE_N,
  FM_DBP_PRESENT_N,
  FM_FAST_PROCHOT_EN_N_R,
  FM_SPI_MUX_OE_CTL_PLD_N,
  FBRK_N_R,
  FM_PEHPCPU_INT,
  FM_BIOS_MRC_DEBUG_MSG_DIS_R,
  FAST_PROCHOT_N,                 //50
  FM_JTAG_TCK_MUX_SEL_R,
  BMC_JTAG_SEL_R,
  H_CPU_ERR0_LVC3_N,
  H_CPU_ERR1_LVC3_N,
  H_CPU_ERR2_LVC3_N,
  RST_RSMRST_BMC_N,
  FM_MP_PS_FAIL_N,
  H_CPU_MEMTRIP_LVC3_N,
  FM_CPU_BIC_PROCHOT_LVT3_N,
  IRQ_PVCCD_CPU0_VRHOT_LVC3_N,    //60
  FM_PVCCIN_CPU0_PWR_IN_ALERT_N,
  FM_THROTTLE_IN_N,
  SGPIO_BMC_CLK_R,
  SGPIO_BMC_LD_R_N,
  SGPIO_BMC_DOUT_R,
  SGPIO_BMC_DIN,
};

enum {
  DPV2_RETIMER_X4 = 0x00,
  DPV2_RETIMER_X8 = 0x02,
  DPV2_RETIMER_X16 = 0x08,
  DPV2_PCIE_UNKNOW = 0xff,
};

int active_config(struct libusb_device *dev,struct libusb_device_handle *handle);
int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio, uint8_t intf);
int bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt);
int bic_mux_select(uint8_t slot_id, uint8_t bus, uint8_t dev_id, uint8_t intf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
