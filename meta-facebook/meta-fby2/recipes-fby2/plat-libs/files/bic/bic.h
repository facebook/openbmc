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
#include <facebook/fby2_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GPIO_PINS     40

// GPIO PINS
enum {
  PWRGOOD_CPU = 0x0,
  PWRGD_PCH_PWROK,
  PVDDR_AB_VRHOT_N,
  PVDDR_DE_VRHOT_N,
  PVCCIN_VRHOT_N,
  FM_THROTTLE_N,
  FM_PCH_BMC_THERMTRIP_N,
  H_MEMHOT_CO_N,
  FM_CPU0_THERMTRIP_LVT3_N,
  CPLD_THERMTRIP_PCH,
  FM_CPLD_FIVR_FAULT,
  FM_CPU_CATERR_N,
  FM_CPU_ERROR2,
  FM_CPU_ERROR1,
  FM_CPU_ERROR0,
  SLP_S4_N,
  FM_NMI_EVENT_BMC_N,
  FM_SMI_BMC_N,
  PLTRST_N,
  FP_RST_BTN_N,
  RST_BTN_BMC_OUT_N,
  FM_BIOS_POST_CMPLT_N,
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

enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_BIC,
  UPDATE_VR,
};

// Bridge IC Spec
typedef struct _bic_gpio_t {
  uint32_t pwrgood_cpu:1;
  uint32_t pwrgd_pch_pwrok:1;
  uint32_t pvddr_ab_vrhot_n:1;
  uint32_t pvddr_de_vrhot_n:1;
  uint32_t pvccin_vrhot_n:1;
  uint32_t fm_throttle_n:1;
  uint32_t fm_pch_bmc_thermtrip_n:1;
  uint32_t h_memhot_co_n:1;
  uint32_t fm_cpu0_thermtrip_lvt3_n:1;
  uint32_t cpld_pch_thermtrip:1;
  uint32_t fm_cpld_fivr_fault:1;
  uint32_t fm_cpu_caterr_n:1;
  uint32_t fm_cpu_error2:1;
  uint32_t fm_cpu_error1:1;
  uint32_t fm_cpu_error0:1;
  uint32_t fm_slp4_n:1;
  uint32_t fm_nmi_event_bmc_n:1;
  uint32_t fm_smi_bmc_n:1;
  uint32_t pltrst_n:1;
  uint32_t fp_rst_btn_n:1;
  uint32_t rst_btn_bmc_out_n:1;
  uint32_t fm_bios_post_compt_n:1;
  uint32_t fm_slp3_n:1;
  uint32_t pwrgd_pvccin:1;
  uint32_t fm_backup_bios_sel_n:1;
  uint32_t fm_ejector_latch_detect_n:1;
  uint32_t bmc_reset:1;
  uint32_t fm_jtag_bic_tck_mux_sel_n:1;
  uint32_t bmc_ready_n:1;
  uint32_t bmc_com_sw_n:1;
  uint32_t rst_i2c_mux_n:1;
  uint32_t xdp_bic_preq_n:1;
  uint32_t xdp_bic_trst:1;
  uint32_t fm_sys_throttle_lvc3:1;
  uint32_t xdp_bic_prdy_n:1;
  uint32_t xdp_prsnt_in_n:1;
  uint32_t xdp_prsnt_out_n:1;
  uint32_t xdp_bic_pwr_debug_n:1;
  uint32_t fm_bic_jtag_sel_n:1;
  uint32_t rsvd:1;
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
  uint8_t rsvd:6;
} bic_config_t;

typedef union _bic_config_u {
  uint8_t config;
  bic_config_t bits;
} bic_config_u;

int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *id);

int bic_get_bic_config(uint8_t slot_id, bic_config_t *cfg);
int bic_set_bic_config(uint8_t slot_id, bic_config_t *cfg);

int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_get_config(uint8_t slot_id, bic_config_t *cfg);
int bic_get_post_buf(uint8_t slot_id, uint8_t *buf, uint8_t *len);

int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info);
int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path);
int bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path);

int bic_get_sel_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info);
int bic_get_sel_rsv(uint8_t slot_id, uint16_t *rsv);
int bic_get_sel(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);

int bic_get_sdr_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info);
int bic_get_sdr_rsv(uint8_t slot_id, uint16_t *rsv);
int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen);

int bic_read_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor);

int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid);

int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);

int bic_update_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
