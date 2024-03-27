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

#define MAX_GPIO_PINS 73

#define RETRY_3_TIME 3
#define RETRY_TIME 10
#define BIC_XFER_RETRY_TIME 3
#define BIC_XFER_RETRY_DELAY 100

#define MAX_CHECK_DEVICE_TIME 8
#define GPIO_RST_USB_HUB 0x10
#define EXP_GPIO_RST_USB_HUB 0x9
#define VALUE_LOW 0
#define VALUE_HIGH 1

#define MAX_VER_STR_LEN 80

#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

#define CPLD_ADDRESS 0x1E
#define SLOT_BUS_BASE 3
#define SB_CPLD_BOARD_ASD_STATUS_REG  0x1F
#define CPLD_BB_BUS 0x01
#define CPLD_SB_BUS 0x01
#define SB_USB_VENDOR_ID  0x1D6B
#define SB_USB_PRODUCT_ID 0x0104

#define SET_OP_IOEXP(reg, port)   (reg | (1ULL << port))
#define CLEAR_OP_IOEXP(reg, port) (reg & (~(1ULL << port)))

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

/* Sync with openBIC oem_1s_handler.h */
enum FIRMWARE_COMPONENT {
  COMPNT_CPLD = 1,
  COMPNT_BIC,
  COMPNT_ME,
  COMPNT_BIOS,
  COMPNT_PVCCIN,
  COMPNT_PVCCFA_EHV_FIVRA,
  COMPNT_PVCCD_HV,
  COMPNT_PVCCINFAON,
  COMPNT_PVCCFA_EHV,
  COMPNT_CXL
};

enum {
  /*----VR ADDR-----*/
  VCCIN_ADDR = 0xC0,
  VCCD_ADDR = 0xC4,
  VCCINFAON_ADDR = 0xEC,
  VR_PESW_ADDR = 0xC8,
  VR_2OU_P3V3_STBY1 = 0x28,
  VR_2OU_P3V3_STBY2 = 0x2E,
  VR_2OU_P3V3_STBY3 = 0x30,
  VR_2OU_P1V8       = 0x36,

  /*----halfdome Renesas VR ADDR-----*/
  VDDCR_CPU0_ADDR = 0xC2,
  VDDCR_CPU1_ADDR = 0xC4,
  VDD11S3_ADDR = 0xC6,

  /*----halfdome Infineon VR ADDR-----*/
  VDDCR_CPU0_IFX_ADDR = 0xC8,
  VDDCR_CPU1_IFX_ADDR = 0xCC,
  VDD11S3_IFX_ADDR = 0xD0,

  /*----halfdome MPS VR ADDR-----*/
  VDDCR_CPU0_MPS_ADDR = 0x9E,
  VDDCR_CPU1_MPS_ADDR = 0x9C,
  VDD11S3_MPS_ADDR = 0x96,

  /*----halfdome TI VR ADDR-----*/
  VDDCR_CPU0_TI_ADDR = 0x8A,
  VDDCR_CPU1_TI_ADDR = 0x8C,
  VDD11S3_TI_ADDR = 0x8E,

  /*----Rainbow Falls VR ADDR-----*/
  VR_1OU_V9_ASICA_ADDR = 0xC8,
  VR_1OU_VDDQAB_ADDR = 0xB0,
  VR_1OU_VDDQCD_ADDR = 0xB4,

  /*----greatlake VR ADDR-----*/
  GL_VCCIN_ADDR = 0xC0,
  GL_VCCD_ADDR = 0xD4,
  GL_VCCINF_ADDR = 0xDC,

  /*----greatlake EVT VR ADDR-----*/ // Refer to page 9 of "GREAT LAKES_EVT_20230412
  GL_EVT_VCCIN_ADDR = 0xC0,
  GL_EVT_VCCD_ADDR = 0xE4,
  GL_EVT_VCCINF_ADDR = 0xC4,
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
  FM_RMCA_LVT3_N,
  HSC_MUX_SWITCH_R,               //30
  FM_FORCE_ADR_N_R,
  PWRGD_CPU_LVC3,
  FM_PCH_BMC_THERMTRIP_N,
  FM_THROTTLE_R_N,
  IRQ_HSC_ALERT2_N,
  SMB_SENSOR_LVC3_ALERT_N,
  FM_CATERR_LVT3_N,
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
  BOARD_ID2,                      //60
  IRQ_PVCCD_CPU0_VRHOT_LVC3_N,
  FM_PVCCIN_CPU0_PWR_IN_ALERT_N,
  BOARD_ID0,
  BOARD_ID1,
  BOARD_ID3,
  FM_THROTTLE_IN_N,
  AUTH_COMPLETE,
  AUTH_PRSNT_N,
  SGPIO_BMC_CLK_R,
  SGPIO_BMC_LD_R_N,               //70
  SGPIO_BMC_DOUT_R,
  SGPIO_BMC_DIN,
};

enum hd_gpio {
  HD_FM_BIOS_POST_CMPLT_BIC_N = 0,
  HD_FM_CPU_BIC_SLP_S3_N,
  HD_APML_CPU_ALERT_BIC_N,
  HD_IRQ_UV_DETECT_N,
  HD_PVDDCR_CPU0_BIC_OCP_N,
  HD_HSC_OCP_GPIO1_R,
  HD_PVDDCR_CPU1_BIC_OCP_N,
  HD_RST_USB_HUB_R_N,
  HD_P3V_BAT_SCALED_EN_R,
  HD_HDT_BIC_TRST_R_N,
  HD_FM_CPU_BIC_SLP_S5_N,
  HD_PVDD11_S3_BIC_OCP_N,
  HD_FM_HSC_TIMER,
  HD_IRQ_SMB_IO_LVC3_STBY_ALRT_N,
  HD_PVDDCR_CPU1_PMALERT_N,
  HD_FM_CPU_BIC_THERMTRIP_N,
  HD_FM_PRSNT_CPU_BIC_N,
  HD_AUTH_PRSNT_BIC_N,
  HD_RST_CPU_RESET_BIC_N,
  HD_PWRBTN_R1_N,
  HD_RST_BMC_R_N,
  HD_HDT_BIC_DBREQ_R_N,
  HD_BMC_READY,
  HD_BIC_READY,
  HD_FM_SOL_UART_CH_SEL_R,
  HD_PWRGD_CPU_LVC3,
  HD_CPU_ERROR_BIC_LVC3_R_N,
  HD_PVDD11_S3_PMALERT_N,
  HD_IRQ_HSC_ALERT1_N,
  HD_SMB_SENSOR_LVC3_ALERT_N,
  HD_SYS_PWRBTN_BIC_N,
  HD_RST_PLTRST_BIC_N,
  HD_CPU_SMERR_BIC_N,
  HD_IRQ_HSC_ALERT2_N,
  HD_BIC_CPU_NMI_R_N,
  HD_FM_BMC_DEBUG_ENABLE_N,
  HD_FM_DBP_PRESENT_N,
  HD_FM_FAST_PROCHOT_EN_R_N,
  HD_FM_BIOS_ABL_DEBUG_MSG_DIS_N,
  HD_FM_BIOS_MRC_DEBUG_MSG_DIS,
  HD_FAST_PROCHOT_N,
  HD_BIC_CPU_RSVD_N,
  HD_BIC_JTAG_SEL_R,
  HD_HSC_OCP_GPIO2_R,
  HD_HSC_OCP_GPIO3_R,
  HD_RST_RSMRST_BMC_N,
  HD_FM_CPU_BIC_PROCHOT_LVT3_N,
  HD_BIC_JTAG_MUX_SEL,
  HD_BOARD_ID2,
  HD_BOARD_ID0,
  HD_BOARD_ID1,
  HD_BOARD_ID3,
  HD_BOARD_ID5,
  HD_BOARD_ID4,
  HD_HSC_TYPE_0,
  HD_HSC_TYPE_1,
};

enum gl_gpio {
  GL_HSC_TYPE = 0,
  GL_H_CPU_MON_FAIL_LVC3_N,
  GL_FM_SLPS3_LVC3_N,
  GL_IRQ_BMC_NMI_R_N,
  GL_IRQ_UV_DETECT_N,
  GL_FM_UV_ADR_TRIGGER_EN_R,
  GL_HSC_SET_EN_R,
  GL_RST_USB_HUB_N_R,
  GL_A_P3V_BAT_SCALED_EN_R,
  GL_DAM_BIC_R_EN,
  GL_FM_SLPS4_PLD_N,
  GL_FM_CPU0_CD_INIT_ERROR,
  GL_FM_HSC_TIMER,
  GL_IRQ_SMB_IO_LVC3_STBY_ALRT_N,
  GL_IRQ_PVCCD_CPU0_VRHOT_LVC3_N,
  GL_DBP_CPU_PREQ_BIC_N,
  GL_FM_CPU_THERMTRIP_LATCH_LVT3_N,
  GL_FM_CPU_SKTOCC_LVT3_PLD_N,
  GL_H_CPU_MEMHOT_OUT_LVC3_N,
  GL_RST_PLTRST_SYNC_LVC3_N,
  GL_PWRBTN_N,
  GL_RST_BMC_R_N,
  GL_H_BMC_PRDY_BUF_N,
  GL_BMC_READY,
  GL_BIC_READY,
  GL_FM_RMCA_LVT3_N,
  GL_PWRGD_AUX_PWRGD_BMC_LVC3,
  GL_FM_FORCE_ADR_N_R,
  GL_PWRGD_CPU_LVC3,
  GL_IRQ_HSC_ALERT2_N,
  GL_SMB_SENSOR_LVC3_ALERT_N,
  GL_FM_CATERR_LVT3_N,
  GL_SYS_PWRBTN_LVC3_N,
  GL_RST_PLTRST_BUF_N,
  GL_IRQ_SML1_PMBUS_ALERT_N,
  GL_FM_BMC_DEBUG_ENABLE_R_N,
  GL_FM_DBP_PRESENT_N,
  GL_FM_FAST_PROCHOT_EN_R_N,
  GL_FBRK_R_N,
  GL_FM_PEHPCPU_INT,
  GL_FAST_PROCHOT_N,
  GL_BMC_JTAG_SEL_R,
  GL_H_CPU_ERR0_LVC3_R_N,
  GL_H_CPU_ERR1_LVC3_R_N,
  GL_H_CPU_ERR2_LVC3_R_N,
  GL_FM_MP_PS_FAIL_N,
  GL_H_CPU_MEMTRIP_LVC3_N,
  GL_FM_CPU_BIC_PROCHOT_LVT3_N,
  GL_AUTH_COMPLETE,
  GL_BOARD_ID2,
  GL_IRQ_PVCCIN_CPU0_VRHOT_N,
  GL_IRQ_PVCCINF_CPU0_VRHOT_N,
  GL_BOARD_ID0,
  GL_BOARD_ID1,
  GL_BOARD_ID3,
  GL_AUTH_PRSNT_N,
};

enum gl_virtual_gpio {
  GL_BIOS_POST_COMPLETE = 0,
  GL_ADR_MODE0 = 2,
};

enum ji_gpio {
  JI_FPGA_READY = 0,
  JI_FAN_FULL_SPEED_FPGA_L,
  JI_PWR_BRAKE_L,
  JI_INA230_E1S_ALERT_L,
  JI_FPGA_WATCH_DOG_TIMER0_L,
  JI_FPGA_WATCH_DOG_TIMER1_L,
  JI_HSC_OCP_GPIO1,
  JI_FPGA_WATCH_DOG_TIMER2_L,
  JI_RST_USB_HUB_R_L,
  JI_P3V_BAT_SCALED_EN_R,
  JI_WP_HW_EXT_CTRL_L,
  JI_BIC_EROT_LVSFT_EN,
  JI_RTC_CLR_L,
  JI_I2C_SSIF_ALERT_L,
  JI_FPGA_CPU_BOOT_DONE,
  JI_FM_HSC_TIMER,
  JI_IRQ_I2C_IO_LVC_STBY_ALRT_L,
  JI_BIC_TMP_LVSFT_EN,
  JI_BIC_I2C_0_FPGA_ALERT_L,
  JI_BIC_I2C_1_FPGA_ALERT_L,
  JI_JTAG_TRST_CPU_BMC_L,
  JI_RST_I2C_E1S_L,
  JI_FM_PWRDIS_E1S,
  JI_PWRBTN_L,
  JI_RST_BMC_L,
  JI_BMC_READY,
  JI_BIC_READY,
  JI_PWR_BRAKE_CPU1_L,
  JI_RUN_POWER_PG,
  JI_SPI_BMC_FPGA_INT_L,
  JI_IRQ_HSC_ALERT1_L,
  JI_I2C_SENSOR_LVC_ALERT_L,
  JI_INA_CRIT_ALERT1_L,
  JI_RUN_POWER_EN,
  JI_SPI_HOST_TPM_RST_L,
  JI_HSC_TYPE_0,
  JI_THERM_WARN_CPU1_L_3V3,
  JI_RUN_POWER_FAULT_L,
  JI_SENSOR_AIR0_THERM_L,
  JI_SENSOR_AIR1_THERM_L,
  JI_FM_FAST_PROCHOT_EN,
  JI_THERM_BB_OVERT_L,
  JI_THERM_BB_WARN_L,
  JI_BIC_CPU_JTAG_MUX_SEL,
  JI_FM_VR_FW_PROGRAM_L,
  JI_FAST_PROCHOT_L,
  JI_CPU_EROT_FATAL_ERROR_L,
  JI_BIC_REMOTEJTAG_EN,
  JI_THERM_OVERT_CPU1_L_3V3,
  JI_HSC_OCP_GPIO2,
  JI_HSC_OCP_GPIO3,
  JI_SENSOR_AIR0_ALERT_L,
  JI_SENSOR_AIR1_ALERT_L,
  JI_BIC_CPLD_VRD_MUX_SEL,
  JI_CPU_BIC_PROCHOT_L,
  JI_CPLD_JTAG_MUX_SEL,
  JI_JTAG_FPGA_MUX_SEL,
  JI_BOARD_ID0,
  JI_BOARD_ID1,
  JI_BOARD_ID2,
  JI_BOARD_ID3,
  JI_BOARD_ID4,
  JI_BOARD_ID5,
  JI_P12V_STBY_SCALED,
  JI_VDD_1V8_SENSOR,
  JI_P3V3_STBY_SCALED,
  JI_SOCVDD_SENSOR,
  JI_P3V_BAT_SCALED,
  JI_CPUVDD_SENSOR,
  JI_FPGA_VCC_AO_SENSOR,
  JI_1V2_SENSOR,
  JI_CARD_TYPE_EXP,
  JI_VDD_3V3_M2_SCALED,
  JI_P1V2_STBY_SCALED,
  JI_FBVDDQ_SENSOR,
  JI_FBVDDP2_SENSOR,
  JI_FBVDD1_SENSOR,
  JI_P5V_STBY_SCALED,
  JI_CPU_DVDD_SENSOR,
};

enum {
  DPV2_RETIMER_X4 = 0x00,
  DPV2_RETIMER_X8 = 0x02,
  DPV2_RETIMER_X16 = 0x08,
  DPV2_PCIE_UNKNOW = 0xff,
};

enum VR_VENDOR_DEVICE_ID {
  VR_INFINEON = 2,
  VR_RENESAS = 4,
  VR_TI = 6,
};

enum {
  NO_EXPANSION_PRESENT = 0x0,
  PRESENT_1OU = 0x01,
  PRESENT_2OU = 0x02,
  PRESENT_3OU = 0x04,
  PRESENT_4OU = 0x08,
  // In Olmsted point system,
  // 3OU present bit is the same as 2OU present in other platform
  OP_PRESENT_3OU = 0x02,
};

// Port definition of Olmstead point I/O Expander
enum op_ioexp_port{
  OP_IOEXP_BIC_SRST_N_R = 0,
  OP_IOEXP_BIC_EXTRST_N_R = 1,
  OP_IOEXP_BIC_FWSPICK_R = 2,
  OP_IOEXP_UART_IOEXP_MUX_CTRL = 3,
  OP_IOEXP_P12V_STBY_POWER_EN = 4,
};

int active_config(struct libusb_device *dev,struct libusb_device_handle *handle);
int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio, uint8_t intf);
int bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt);
int bic_mux_select(uint8_t slot_id, uint8_t bus, uint8_t dev_id, uint8_t intf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
