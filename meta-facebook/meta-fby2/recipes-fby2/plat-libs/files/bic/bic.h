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

#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/kv.h>
#include <facebook/fby2_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GPIO_PINS     64
#define MAX_POSTCODE_NUM  1024
#define DEV_SENSOR_INFO_LEN  6

#define CC_BIC_RETRY 0x70
#define BIC_RETRY_ACTION  -2

// Device fw update
#define CFM1_START 0x00002000
#define CFM1_END 0x000127ff
#define CFM0_START 0x00012800
#define CFM0_END 0x00022fff
#define SPRINGHILL_M2_OFFSET_BASE 1 // one byte for FBID
#define DEV_UPDATE_BATCH_SIZE 192
#define DEV_UPDATE_IPMI_HEAD_SIZE 6

// GPIO PINS
enum {
  PWRGD_COREPWR = 0x0,
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

// RC GPIO PINS
enum {
  RC_PWRGD_PS_PWROK,
  RC_QDF_PS_HOLD_OUT,
  RC_PVDDQ_510_VRHOT_N_R1,
  RC_PVDDQ_423_VRHOT_N_R1,
  RC_VR_I2C_SEL,
  RC_QDF_THROTTLE_3P3_N,
  RC_QDF_FORCE_PMIN,
  RC_QDF_FORCE_PSTATE,
  RC_QDF_TEMPTRIP_N,
  RC_FAST_THROTTLE_N,
  RC_QDF_LIGHT_THROTTLE_3P3_N,
  RC_UEFI_DB_MODE_N,
  RC_QDF_RAS_ERROR_2,
  RC_QDF_RAS_ERROR_1,
  RC_QDF_RAS_ERROR_0,
  RC_SPI_MUX_SEL,
  RC_QDF_NMI,
  RC_QDF_SMI,
  RC_QDF_RES_OUT_N_R,
  RC_SYS_BUF_RST_N,
  RC_SYS_BIC_RST_N,
  RC_FM_BIOS_POST_CMPLT_N,
  RC_IMC_RDY,
  RC_PWRGD_SYS_PWROK,
  RC_FM_BACKUP_BIOS_SEL_N,
  RC_T32_JTAG_DET,
  RC_BB_BMC_RST_N,
  RC_QDF_PRSNT_0_N,
  RC_QDF_PRSNT_1_N,
  RC_EJCT_DET_N,
  RC_M2_I2C_MUX_RST_N,
  RC_BIC_REMOTE_SRST_N,
  RC_BIC_DB_TRST_N,
  RC_IMC_BOOT_ERROR,
  RC_BIC_RDY,
  RC_QDF_PROCHOT_N,
  RC_PWR_BIC_BTN_N,
  RC_PWR_BTN_BUF_N,
  RC_PMF_REBOOT_REQ_N,
  RC_BIC_BB_I2C_ALERT,
};

// EP GPIO PINS
enum {
  EP_PWRGD_COREPWR,
  EP_PWRGD_SYS_PWROK,
  EP_PWRGD_PS_PWROK_PLD,
  EP_PVCCIN_VRHOT_N,
  EP_H_MEMHOT_CO_N,
  EP_FM_CPLD_BIC_THERMTRIP_N,
  EP_FM_CPU_ERROR_2,
  EP_FM_CPU_ERROR_1,
  EP_FM_CPU_ERROR_0,
  EP_FM_NMI_EVENT_BMC_N,
  EP_FM_CRASH_DUMP_M3_N,
  EP_FP_RST_BTN_N,
  EP_FP_RST_BTN_OUT_N,
  EP_FM_BIOS_POST_COMPT_N,
  EP_FM_BACKUP_BIOS_SEL_N,
  EP_FM_EJECTOR_LATCH_DETECT_N,
  EP_BMC_RESET,
  EP_FM_SMI_BMC_N,
  EP_PLTRST_N,
  EP_TMP_ALERT,
  EP_RST_I2C_MUX_N,
  EP_XDP_BIC_TRST,
  EP_SMB_BMC_ALERT_N,
  EP_FM_CPLD_BIC_M3_HB,
  EP_IRQ_MEM_SOC_VRHOT_N,
  EP_FM_CRASH_DUMP_V8_N,
  EP_FM_M3_ERR_N,
  EP_FM_CPU_CATERR_LVT3_N,
  EP_BMC_READY_N,
  EP_BMC_COM_SW_N,
  EP_BMC_HB_LED_N,
  EP_FM_VR_FAULT_N,
  EP_IRQ_CPLD_BIC_PROCHOT_N,
  EP_FM_SMB_VR_SOC_MUX_EN,
};

// GPv2 GPIO PINS
enum {
  GPV2_BIC_HB_LED_N,
  GPV2_I2C_PESW_MULTI_CONFIG_ADDR0_R,
  GPV2_I2C_PESW_MULTI_CONFIG_ADDR1_R,
  GPV2_I2C_PESW_MULTI_CONFIG_ADDR2_R,
  GPV2_I2C_PESW_MULTI_CONFIG_ADDR3_R,
  GPV2_I2C_PESW_MULTI_CONFIG_ADDR4_R,
  GPV2_SMB_BIC_3V3SB_READY_N_R,
  GPV2_P3V3_DEV0_EN,
  GPV2_P3V3_DEV1_EN,
  GPV2_P3V3_DEV2_EN,
  GPV2_P3V3_DEV3_EN,
  GPV2_P3V3_DEV4_EN,
  GPV2_P3V3_DEV5_EN,
  GPV2_P3V3_DEV6_EN,
  GPV2_P3V3_DEV7_EN,
  GPV2_P3V3_DEV8_EN,
  GPV2_P3V3_DEV9_EN,
  GPV2_P3V3_DEV10_EN,
  GPV2_P3V3_DEV11_EN,
  GPV2_PWRGD_P3V3_DEV0,
  GPV2_PWRGD_P3V3_DEV1,
  GPV2_PWRGD_P3V3_DEV2,
  GPV2_PWRGD_P3V3_DEV3,
  GPV2_PWRGD_P3V3_DEV4,
  GPV2_PWRGD_P3V3_DEV5,
  GPV2_PWRGD_P3V3_DEV6,
  GPV2_PWRGD_P3V3_DEV7,
  GPV2_PWRGD_P3V3_DEV8,
  GPV2_PWRGD_P3V3_DEV9,
  GPV2_PWRGD_P3V3_DEV10,
  GPV2_PWRGD_P3V3_DEV11,
  GPV2_FM_COM_EN_N_R,
  GPV2_FM_COM_SEL_0_R,
  GPV2_FM_COM_SEL_1_R,
  GPV2_FM_COM_SEL_2_R,
  GPV2_FM_COM_SEL_3_R,
  GPV2_FM_JTAG_EN_N_R,
  GPV2_FM_JTAG_SEL_0_R,
  GPV2_FM_JTAG_SEL_1_R,
  GPV2_FM_JTAG_SEL_2_R,
  GPV2_FM_JTAG_SEL_3_R,
  GPV2_BIC_REMOTE_DEBUG_SELECT_N,
  GPV2_RST_I2C_MUX1_N_R,
  GPV2_RST_I2C_MUX2_N_R,
  GPV2_RST_I2C_MUX3_N_R,
  GPV2_RST_I2C_MUX4_N_R,
  GPV2_RST_I2C_MUX5_N_R,
  GPV2_RST_I2C_MUX6_N_R,
  GPV2_FM_BIC_DU_DEV0_EN_R,
  GPV2_FM_BIC_DU_DEV1_EN_R,
  GPV2_FM_BIC_DU_DEV2_EN_R,
  GPV2_FM_BIC_DU_DEV3_EN_R,
  GPV2_FM_BIC_DU_DEV4_EN_R,
  GPV2_FM_BIC_DU_DEV5_EN_R,
  GPV2_FM_BIC_DU_DEV6_EN_R,
  GPV2_FM_BIC_DU_DEV7_EN_R,
  GPV2_FM_BIC_DU_DEV8_EN_R,
  GPV2_FM_BIC_DU_DEV9_EN_R,
  GPV2_FM_BIC_DU_DEV10_EN_R,
  GPV2_FM_BIC_DU_DEV11_EN_R,
  GPV2_FM_POWER_EN,
  GPV2_BIC_BOARD_ID_0,
  GPV2_BIC_BOARD_ID_1,
  GPV2_BIC_BOARD_ID_2,
};

// ND GPIO PINS
enum {
  ND_PWRGD_CPU0_LVC3,
  ND_PWRGD_SYS_PWROK_R,
  ND_IRQ_PVDDIO_ABCD_VRHOT_N,
  ND_IRQ_PVDDIO_EFGH_VRHOT_N,
  ND_IRQ_TMP_ALERT_N,
  ND_FM_THROTTLE_N,
  ND_IRQ_VDDCR_CPU_VRHOT_N,
  ND_IRQ_VDDCR_SOC_VRHOT_N,
  ND_FM_CPU_BMC_THERMTRIP_N,
  ND_SMB_BIC_3V3SB_READY_N,
  ND_IRQ_P0_ALERT_N,
  ND_FM_PVDDIO_ABCD_SMB_ALERT_N,
  ND_IRQ_VDDCR_CPU_ALERT_N,
  ND_IRQ_VDDCR_SOC_ALERT_N,
  ND_FM_PVDDIO_EFGH_SMB_ALERT_N,
  ND_FM_SMI_ACTIVE_N,
  ND_FM_FAST_THROTTLE_N,
  ND_IRQ_BIC_CPU_SMI_LPC_R_N,
  ND_PLTRST_N,
  ND_BB_RST_BTN_BUF_N,
  ND_RST_BMC_RSTBTN_OUT_N,
  ND_FM_BIOS_POST_COMPT_N,
  ND_FM_SLP_S3_N,
  ND_PWRGD_PS_PWROK_PLD,
  ND_SPI_SW_SELECT,
  ND_FM_EJECTOR_LATCH_DETECT_N,
  ND_BMC_RESET_IN,
  ND_BIC_COM_SW2_N,
  ND_BMC_READY,
  ND_BIC_COM_SW1_N,
  ND_RST_I2C_M2_MUX_N,
  ND_BIC_REMOTEJTAG_EN_N,
  ND_JTAG_BIC_TRST,
  ND_FM_SYS_THROTTLE_LVC3,
  ND_IRQ_PMBUS_ALERT_N,
  ND_IRQ_M2_3V3_ALERT_N,
  ND_FM_BMC_PWR_BTN_OUT_N,
  ND_HDT_BIC_DBREQ_L,
  ND_M_ABCD_EVENT_BUF_N,
  ND_M_EFGH_EVENT_BUF_N,
  ND_BB_PWR_BTN_BUF_N,
  ND_RST_BIC_RTCRST,
  ND_FM_BIOS_RCVR,
  ND_RST_RSMRST_P0_BUF_N,
  ND_FM_PWR_LED_N,
  ND_FM_FAULT_LED_N,
  ND_FM_BIC_READ_SPD,
  ND_FM_CPU_CATERR_N,
};

// Server type
enum {
  SERVER_TYPE_TL = 0x0,
  SERVER_TYPE_RC = 0x1,
  SERVER_TYPE_EP = 0x2,
  SERVER_TYPE_ND = 0x4,
  SERVER_TYPE_NONE = 0xFF,
};

enum {
  FW_CPLD = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_PVCCIO_VR,
  FW_PVCCIN_VR,
  FW_PVCCSA_VR,
  FW_DDRAB_VR,
  FW_DDRDE_VR,
  FW_PVNNPCH_VR,
  FW_P1V05_VR,
};

// RC
enum {
  FW_IMC = 3,
  FW_VDD_APC_CBF_VR = 5,
  FW_DDR510_VR = 6,
  FW_DDR423_VR = 7,
};

// EP
enum {
  FW_M3 = 3,
  FW_VDD_CORE_VR = 5,
  FW_VDD_SRAM_VR = 6,
  FW_VDD_MEM_VR = 7,
  FW_VDD_SOC_VR = 8,
  FW_DDR_AG_VR = 9,
  FW_DDR_BH_VR = 10,
};

// GPv2
enum {
  FW_3V3_VR = 5,
  FW_0V92 = 6,
  FW_PCIE_SWITCH = 7,
};

// ND
enum {
  FW_BOOTLOADER = 3,
  FW_PVDDCR_CPU_VR = 4,
  FW_PVDDCR_SOC_VR = 5,
  FW_PVDDIO_ABCD_VR = 6,
  FW_PVDDIO_EFGH_VR = 7,
};

enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_BIC,
  UPDATE_VR,
  UPDATE_PCIE_SWITCH,
  UPDATE_SPH,
  UPDATE_BRCM,
  UPDATE_VSI,
};

enum {
  FW_PCIE_SWITCH_STAT_IDLE = 0,
  FW_PCIE_SWITCH_STAT_INPROGRESS,
  FW_PCIE_SWITCH_STAT_DONE,
  FW_PCIE_SWITCH_STAT_ERROR = 0xFF,
};

enum {
  DUMP_BIOS = 0,
};

//Force Intel ME Recovery Command Options
enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

enum {
  DEV_TYPE_ASIC = 0,
  DEV_TYPE_M_2,
};

//Device update option
enum {
  ERASE_DEV_FW = 0x00,
  PROGRAM_DEV_FW = 0x01,
  RELOAD_DEV_IMG = 0x02,
  GET_BOOT_LOCATION = 0x03,
};

/* Generic GPIO configuration */
typedef struct _bic_gpio_t {
  uint64_t gpio;
} bic_gpio_t;

typedef struct _bic_gpio_config_t {
  union {
    struct {
      uint8_t dir:1;
      uint8_t ie:1;
      uint8_t edge:1;
      uint8_t trig:2;
    };
    uint8_t config;
  };
} bic_gpio_config_t;

typedef struct _bic_config_t {
  union {
    struct {
      uint8_t sol:1;
      uint8_t post:1;
      uint8_t rsvd:6;
    };
    uint8_t config;
  };
} bic_config_t;

typedef struct
{
  uint8_t sensor_num;
  uint8_t int_value;
  uint8_t dec_value; // for accuracy sensor
  uint8_t flags;
  uint8_t status;
  uint8_t ext_status;
} ipmi_device_sensor_t;

typedef struct _ipmi_device_sensor_reading_t {
  ipmi_device_sensor_t data[MAX_NUM_DEV_SENSORS];
} ipmi_device_sensor_reading_t;

int bic_is_slot_12v_on(uint8_t slot_id);
int bic_set_slot_12v(uint8_t slot_id, uint8_t status);
uint8_t is_bic_ready(uint8_t slot_id);
int bic_is_slot_power_en(uint8_t slot_id);

int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *id);

int bic_get_bic_config(uint8_t slot_id, bic_config_t *cfg);
int bic_set_bic_config(uint8_t slot_id, bic_config_t *cfg);

int bic_get_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *ffi, uint8_t *meff, uint16_t *vendor_id, uint8_t *major_ver, uint8_t *minor_ver);
int bic_set_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t status);
int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio);
int bic_get_gpio_status(uint8_t slot_id, uint8_t pin, uint8_t *status);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value);
int bic_set_gpio64(uint8_t slot_id, uint8_t gpio, uint8_t value);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_get_gpio64_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_set_gpio64_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_get_config(uint8_t slot_id, bic_config_t *cfg);
int bic_set_config(uint8_t slot_id, bic_config_t *cfg);
int bic_get_post_buf(uint8_t slot_id, uint8_t *buf, uint8_t *len);

int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info);
int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size);
int bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path);

int bic_get_sel_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info);
int bic_get_sel_rsv(uint8_t slot_id, uint16_t *rsv);
int bic_get_sel(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);

int bic_get_sdr_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info);
int bic_get_sdr_rsv(uint8_t slot_id, uint16_t *rsv);
int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);

int bic_read_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor);
int bic_read_device_sensors(uint8_t slot_id, uint8_t dev_id, ipmi_device_sensor_reading_t *sensor, uint8_t *len);

int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid);

int bic_request_post_buffer_data(uint8_t slot_id, uint8_t *port_buff, uint8_t *len);
int bic_request_post_buffer_dword_data(uint8_t slot_id, uint32_t *port_buff, uint32_t input_len, uint32_t *output_len);

int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);

int bic_dump_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_update_firmware(uint8_t slot_id, uint8_t comp, char *path, uint8_t force);
int bic_update_dev_firmware(uint8_t slot_id, uint8_t dev_id, uint8_t comp, char *path, uint8_t force);
int bic_update_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_imc_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int me_recovery(uint8_t slot_id, uint8_t command);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result);
int bic_read_accuracy_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_accuracy_sensor_reading_t *sensor);
int bic_get_slot_type(uint8_t fru);
int bic_set_slot_type(uint8_t fru,uint8_t type);
int bic_get_server_type(uint8_t fru, uint8_t *type);
int bic_clear_cmos(uint8_t slot_id);
int bic_reset(uint8_t slot_id);
int bic_asd_init(uint8_t slot_id, uint8_t cmd);
int bic_set_pcie_config(uint8_t slot_id, uint8_t config);
int get_imc_version(uint8_t slot, uint8_t *ver);

int bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt);
int bic_disable_sensor_monitor(uint8_t slot_id, uint8_t dis);
int bic_send_jtag_instruction(uint8_t slot_id, uint8_t dev_id, uint8_t *rbuf, uint8_t ir);
int bic_get_debug_mode(uint8_t slot_id, uint8_t *debug_mode);
int bic_set_sdr_threshold_update_flag(uint8_t slot, uint8_t update);
int bic_get_sdr_threshold_update_flag(uint8_t slot);
int bic_request_post_buffer_page_data(uint8_t slot_id, uint8_t page_num, uint8_t *port_buff, uint8_t *len);

int bic_get_device_type(uint8_t slot_id, uint8_t drv_num);
int reverse_bit(int raw_val);
int program_dev_fw(uint8_t slot_id, uint8_t dev_id, int bus, char* image, int start, int end);
int update_dev_firmware (uint8_t slot_id, uint8_t dev_id, char* image);
int bic_fget_device_info(uint8_t slot_id, uint8_t dev_num, uint8_t *ffi, uint8_t *meff, uint16_t *vendor_id, uint8_t *major_ver, uint8_t *minor_ver);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
