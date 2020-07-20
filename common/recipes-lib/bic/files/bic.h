/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#define FRUID_READ_COUNT_MAX 0x20

#define BIC_GPIO_LIST                                                       \
  BIC_GPIO_DEF(PWRGOOD_CPU, "XDP_CPU_SYSPWROK"),  /* 0 */                   \
  BIC_GPIO_DEF(PWRGD_PCH_PWROK, "PWRGD_PCH_PWROK"),                         \
  BIC_GPIO_DEF(PVDDR_VRHOT_N, "PVDDR_VRHOT_N"),                             \
  BIC_GPIO_DEF(PVCCIN_VRHOT_N, "PVCCIN_VRHOT_N"),                           \
  BIC_GPIO_DEF(FM_FAST_PROCHOT_N, "FM_FAST_PROCHOT_N"),                     \
  BIC_GPIO_DEF(PCHHOT_CPU_N, "PCHHOT_CPU_N"),                               \
  BIC_GPIO_DEF(FM_CPLD_CPU_DIMM_EVENT_CO_N, "FM_CPLD_CPU_DIMM_EVENT_CO_N"), \
  BIC_GPIO_DEF(FM_CPLD_BDXDE_THERMTRIP_N, "FM_CPLD_BDXDE_THERMTRIP_N"),     \
  BIC_GPIO_DEF(THERMTRIP_PCH_N, "THERMTRIP_PCH_N"),                         \
  BIC_GPIO_DEF(FM_CPLD_FIVR_FAULT, "FM_CPLD_FIVR_FAULT"),                   \
  BIC_GPIO_DEF(FM_BDXDE_CATERR_LVT3_N, "FM_BDXDE_CATERR_LVT3_N"), /* 10 */  \
  BIC_GPIO_DEF(FM_BDXDE_ERR2_LVT3_N, "FM_BDXDE_ERR2_LVT3_N"),               \
  BIC_GPIO_DEF(FM_BDXDE_ERR1_LVT3_N, "FM_BDXDE_ERR1_LVT3_N"),               \
  BIC_GPIO_DEF(FM_BDXDE_ERR0_LVT3_N, "FM_BDXDE_ERR0_LVT3_N"),               \
  BIC_GPIO_DEF(SLP_S4_N, "SLP_S4_N"),                                       \
  BIC_GPIO_DEF(FM_NMI_EVENT_BMC_N, "FM_NMI_EVENT_BMC_N"),                   \
  BIC_GPIO_DEF(FM_SMI_BMC_N, "FM_SMI_BMC_N"),                               \
  BIC_GPIO_DEF(RST_PLTRST_BMC_N, "RST_PLTRST_BMC_N"),                       \
  BIC_GPIO_DEF(FP_RST_BTN_BUF_N, "FP_RST_BTN_BUF_N"),                       \
  BIC_GPIO_DEF(BMC_RST_BTN_OUT_N, "BMC_RST_BTN_OUT_N"),                     \
  BIC_GPIO_DEF(FM_BDE_POST_CMPLT_N, "FM_BDE_POST_CMPLT_N"), /* 20 */        \
  BIC_GPIO_DEF(FM_BDXDE_SLP3_N, "FM_BDXDE_SLP3_N"),                         \
  BIC_GPIO_DEF(FM_PWR_LED_N, "FM_PWR_LED_N"),                               \
  BIC_GPIO_DEF(PWRGD_PVCCIN, "PWRGD_PVCCIN"),                               \
  BIC_GPIO_DEF(SVR_ID0, "SVR_ID0"),                                         \
  BIC_GPIO_DEF(SVR_ID1, "SVR_ID1"),                                         \
  BIC_GPIO_DEF(SVR_ID2, "SVR_ID2"),                                         \
  BIC_GPIO_DEF(SVR_ID3, "SVR_ID3"),                                         \
  BIC_GPIO_DEF(BMC_READY_N, "BMC_READY_N"),                                 \
  BIC_GPIO_DEF(BMC_COM_SW_N, "BMC_COM_SW_N"),                               \
  BIC_GPIO_DEF(RESERVED_30, "RESERVED_30"), /* 30 */                        \
  BIC_GPIO_DEF(RESERVED_31, "RESERVED_31")

#define BIC_GPIO_DEF(type, name)   type
enum {
  BIC_GPIO_LIST,
  BIC_GPIO_MAX,
};
#undef BIC_GPIO_DEF

enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_BIC,
  UPDATE_VR,
};

enum {
  FW_CPLD = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_PVCCIN_VR,
  FW_DDRAB_VR,
  FW_P1V05_VR,
};

/* Force Intel ME Recovery Command Options */
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
  uint32_t fm_cpld_cpu_dimm_event_c0_n:1;
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

typedef union _bic_gpio_u {
  uint8_t gpio[4];
  bic_gpio_t bits;
} bic_gpio_u;

typedef union _bic_gpio_config_u {
  uint8_t config;
  bic_gpio_config_t bits;
} bic_gpio_config_u;

typedef union _bic_config_u {
  uint8_t config;
  bic_config_t bits;
} bic_config_u;

int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                     uint8_t *txbuf, size_t txlen,
                     uint8_t *rxbuf, size_t *rxlen);
const char *bic_gpio_name(unsigned int pin);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio);
int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value);
int bic_get_config(uint8_t slot_id, bic_config_t *cfg);
int bic_get_post_buf(uint8_t slot_id, uint8_t *buf, uint8_t *len);
int bic_get_sel_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info);
int bic_get_sel(uint8_t slot_id, ipmi_sel_sdr_req_t *req,
                ipmi_sel_sdr_res_t *res, size_t *rlen);
int bic_get_sdr_info(uint8_t slot_id, ipmi_sel_sdr_info_t *info);
int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req,
                ipmi_sel_sdr_res_t *res, size_t *rlen);
int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id);
int bic_read_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor);
int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size);
int bic_read_mac(uint8_t slot_id, char *rbuf, size_t rlen);
int bic_update_fw(uint8_t slot_id, uint8_t comp, const char *path);
int bic_get_fw_cksum(uint8_t slot_id, uint8_t comp, uint32_t offset, uint32_t len, uint8_t *ver);
int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);
int bic_set_config(uint8_t slot_id, bic_config_t *cfg);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, size_t txlen,
                uint8_t *rxbuf, size_t *rxlen);
int me_recovery(uint8_t slot_id, uint8_t command);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
