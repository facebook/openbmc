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

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GPIO_PINS     32

// GPIO PINS
enum {
  PWRGOOD_CPU = 0x0,
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
  RESERVED_30,
  RESERVED_31,
};

enum {
  FW_CPLD = 1,
  FW_BIC,
  FW_ME,
  FW_PVCCIN_VR,
  FW_DDRAB_VR,
  FW_P1V05_VR,
  FW_PVCCGBE_VR,
  FW_PVCCSCSUS_VR,
  FW_BIC_BOOTLOADER,
};

enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_BIC,
};

//Force Intel ME Recovery Command Options
enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

// Bridge IC Spec
typedef struct _bic_gpio_t {
  uint32_t pwrgood_cpu:1;
  uint32_t pwrgd_pch_pwrok:1;
  uint32_t pvddr_vrhot_n:1;
  uint32_t pvccin_vrhot_n:1;
  uint32_t fm_fast_prochot_n:1;
  uint32_t pchhot_cpu_n:1;
  uint32_t fm_cpld_cpu_dimm_event_co_n:1;
  uint32_t fm_cpld_bdxde_thermtrip_n:1;
  uint32_t thermtrip_pch_n:1;
  uint32_t fm_cpld_fivr_fault:1;
  uint32_t fm_bdxde_caterr_lvt3_n:1;
  uint32_t fm_bdxde_err_lvt3_n:3;
  uint32_t slp_s4_n:1;
  uint32_t fm_nmi_event_bmc_n:1;
  uint32_t fm_smi_bmc_n:1;
  uint32_t rst_pltrst_bmc_n:1;
  uint32_t fp_rst_btn_buf_n:1;
  uint32_t bmc_rst_btn_out_n:1;
  uint32_t fm_bde_post_cmplt_n:1;
  uint32_t fm_bdxde_slp3_n:1;
  uint32_t fm_pwr_led_n:1;
  uint32_t pwrgd_pvccin:1;
  uint32_t svr_id:4;
  uint32_t bmc_ready_n:1;
  uint32_t bmc_com_sw_n:1;
  uint32_t rsvd:2;
} bic_gpio_t;

typedef union _bic_gpio_u {
  uint8_t gpio[4];
  bic_gpio_t bits;
} bic_gpio_u;

typedef struct _bic_gpio_config_t {
  uint8_t dir:1;
  uint8_t ie:1;
  uint8_t edge:1;
  uint8_t trig:2;
} bic_gpio_config_t;

typedef union _bic_gpio_config_u {
  uint8_t config;
  bic_gpio_config_t bits;
} bic_gpio_config_u;

typedef struct _bic_config_t {
  uint8_t sol:1;
  uint8_t post:1;
  uint8_t kcs:1;
  uint8_t ipmb:1;
  uint8_t rsvd:4;
} bic_config_t;

typedef union _bic_config_u {
  uint8_t config;
  bic_config_t bits;
} bic_config_u;

int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *id);

int bic_get_config(uint8_t slot_id, bic_config_t *cfg);
int bic_set_config(uint8_t slot_id, bic_config_t *cfg);

int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
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

int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);

int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);

int bic_update_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

int me_recovery(uint8_t slot_id, uint8_t command);

int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
