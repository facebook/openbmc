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
};

// RC GPIO PINS
enum {
  RC_PWRGD_SYS_PWROK,
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
  RC_PWRGD_PS_PWROK,
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


// Server type
enum {
  SERVER_TYPE_TL = 0x0,
  SERVER_TYPE_RC = 0x1,
  SERVER_TYPE_EP = 0x2,
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


enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_BIC,
  UPDATE_VR,
};

enum {
  DUMP_BIOS = 0,
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

// RC gpio
typedef struct _bic_rc_gpio_t {
  uint32_t pwrgd_sys_pwrok:1;
  uint32_t qdf_ps_hold_out:1;
  uint32_t pvddq_510_vrhot_n_r1:1;
  uint32_t pvddq_423_vrhot_n_r1:1;
  uint32_t vr_i2c_sel:1;
  uint32_t qdf_throttle_3p3_n:1;
  uint32_t qdf_force_pmin:1;
  uint32_t qdf_force_pstate:1;
  uint32_t qdf_temptrip_n:1;
  uint32_t fast_throttle:1;
  uint32_t qdf_light_throttle_3p3_n:1;
  uint32_t uefi_db_mode_n:1;
  uint32_t qdf_ras_error_2:1;
  uint32_t qdf_ras_error_1:1;
  uint32_t qdf_ras_error_0:1;
  uint32_t spi_mux_sel:1;
  uint32_t qdf_nmi:1;
  uint32_t qdf_smi:1;
  uint32_t qdf_res_out_n_r:1;
  uint32_t sys_buf_rst_n:1;
  uint32_t sys_bic_rst_n:1;
  uint32_t fm_bios_post_cmplt_n:1;
  uint32_t imc_ready:1;
  uint32_t pwrgd_ps_pwrok:1;
  uint32_t fm_backup_bios_sel_n:1;
  uint32_t t32_jtag_det:1;
  uint32_t bb_bmc_rst_n:1;
  uint32_t qdf_prsnt_0_n:1;
  uint32_t qdf_prsnt_1_n:1;
  uint32_t ejct_det_n:1;
  uint32_t m2_i2c_mux_rst_n:1;
  uint32_t bic_remote_srst_n:1;
  uint32_t bic_db_trst_n:1;
  uint32_t imc_boot_error:1;
  uint32_t bic_rdy:1;
  uint32_t qdf_prochot_n:1;
  uint32_t pwr_bic_btn_n:1;
  uint32_t pwr_btn_buf_n:1;
  uint32_t pmf_reboot_req_n:1;
  uint32_t bic_bb_i2c_alert:1;
} bic_rc_gpio_t;

// EP gpio
typedef struct _bic_ep_gpio_t {
  uint32_t pwrgood_cpu:1;
  uint32_t pwrgd_sys_pwrok:1;
  uint32_t pwrgd_ps_pwrok_pld:1;
  uint32_t pvccin_vrhot_n:1;
  uint32_t h_memhot_co_n:1;
  uint32_t fm_cpld_bic_thermtrip_n:1;
  uint32_t fm_cpu_error_2:1;
  uint32_t fm_cpu_error_1:1;
  uint32_t fm_cpu_error_0:1;
  uint32_t fm_nmi_event_bmc_n:1;
  uint32_t fm_crash_dump_m3_n:1;
  uint32_t fp_rst_btn_n:1;
  uint32_t fp_rst_btn_out_n:1;
  uint32_t fm_bios_post_compt_n:1;
  uint32_t fm_backup_bios_sel_n:1;
  uint32_t fm_ejector_latch_detect_n:1;
  uint32_t bmc_reset:1;
  uint32_t fm_smi_bmc_n:1;
  uint32_t pltrst_n:1;
  uint32_t tmp_alert:1;
  uint32_t rst_i2c_mux_n:1;
  uint32_t xdp_bic_trst:1;
  uint32_t smb_bmc_alert_n:1;
  uint32_t fm_cpld_bic_m3_hb:1;
  uint32_t irq_mem_soc_vrhot_n:1;
  uint32_t fm_crash_dump_v8_n:1;
  uint32_t fm_m3_err_n:1;
  uint32_t fm_cpu_caterr_lvt3_n:1;
  uint32_t bmc_ready_n:1;
  uint32_t bmc_com_sw_n:1;
  uint32_t bmc_hb_led_n:1;
  uint32_t fm_vr_fault_n:1;
  uint32_t irq_cpld_bic_prochot_n:1;
  uint32_t fm_smb_vr_soc_mux_en:1;
  uint32_t rsvd:6;
} bic_ep_gpio_t;

typedef union _bic_gpio_u {
  uint8_t gpio[4];
  bic_gpio_t bits;
} bic_gpio_u;

typedef union _bic_rc_gpio_u {
  uint8_t gpio[4];
  bic_rc_gpio_t bits;
} bic_rc_gpio_u;

typedef union _bic_ep_gpio_u {
  uint8_t gpio[5];
  bic_ep_gpio_t bits;
} bic_ep_gpio_u;

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

int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *id);

int bic_get_bic_config(uint8_t slot_id, bic_config_t *cfg);
int bic_set_bic_config(uint8_t slot_id, bic_config_t *cfg);

int bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio);
int bic_get_gpio_raw(uint8_t slot_id, uint8_t *gpio);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
int bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, bic_gpio_config_t *gpio_config);
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

int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid);

int bic_request_post_buffer_data(uint8_t slot_id, uint8_t *port_buff, uint8_t *len);

int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);

int bic_dump_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_update_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int me_recovery(uint8_t slot_id, uint8_t command);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result);
int bic_read_accuracy_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_accuracy_sensor_reading_t *sensor);
int bic_get_server_type(uint8_t fru, uint8_t *type);
int get_imc_version(uint8_t slot, uint8_t *ver);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
