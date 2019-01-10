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

#ifndef __PAL_H__
#define __PAL_H__

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/me.h>
#include <stdbool.h>

#define MAX_NUM_FAN     2

#define FRU_STATUS_GOOD   1
#define FRU_STATUS_BAD    0

#define MAX_NODES 4

#define MAX_NUM_FRUS 5

#define MAX_SDR_LEN           64
#define MAX_SENSOR_NUM        0xFF
#define MAX_SENSOR_THRESHOLD  8
#define MAX_RETRIES_SDR_INIT  30
#define THERMAL_CONSTANT      255
#define ERR_NOT_READY         -2

#define PWR_OPTION_LIST "status, graceful-shutdown, off, on, reset, cycle"

#define AST_GPIO_BASE 0x1e780000
#define UARTSW_OFFSET 0x68
#define SEVEN_SEGMENT_OFFSET 0x20

extern char * key_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];

extern const uint8_t mb_sensor_list[];
extern const uint8_t nic_sensor_list[];
//extern float mb_sensor_threshold[][];
//extern float nic_sensor_threshold[][];

enum
{
  FOUND_AVA_DEVICE = 0x1,
  FOUND_RETIMER_DEVICE = 0x2,
};

enum {
  USB_MUX_OFF,
  USB_MUX_ON,
};

enum {
  UART_TO_BMC,
  UART_TO_DEBUG,
};

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  FAN_0 = 0,
  FAN_1,
};

enum {
  FRU_ALL   = 0,
  FRU_MB = 1,
  FRU_NIC = 2,
  FRU_RISER_SLOT2 = 3,
  FRU_RISER_SLOT3 = 4,
  FRU_RISER_SLOT4 = 5,
};

enum {
  UARTSW_BY_BMC,
  UARTSW_BY_DEBUG,
  SET_SEVEN_SEGMENT,
};

// Sensors Under Side Plane
enum {
  MB_SENSOR_HSC_IN_POWER = 0x29,
  MB_SENSOR_HSC_IN_VOLT = 0x2A,
  MB_SENSOR_CPU0_PKG_POWER = 0x2C,
  MB_SENSOR_CPU1_PKG_POWER = 0x2D,

  MB_SENSOR_CPU0_TJMAX = 0x30,
  MB_SENSOR_CPU1_TJMAX = 0x31,

  MB_SENSOR_PCH_TEMP = 0x08,
  MB_SENSOR_CPU0_THERM_MARGIN = 0x09,
  MB_SENSOR_CPU1_THERM_MARGIN = 0x0A,

  MB_SENSOR_C2_NVME_CTEMP = 0x7B,
  MB_SENSOR_C3_NVME_CTEMP = 0x7C,
  MB_SENSOR_C4_NVME_CTEMP = 0x7D,

  MB_SENSOR_C4_AVA_FTEMP = 0x7E,
  MB_SENSOR_C4_AVA_RTEMP = 0x7F,

  MB_SENSOR_C2_AVA_FTEMP = 0x80,
  MB_SENSOR_C2_AVA_RTEMP = 0x81,
  MB_SENSOR_C2_1_NVME_CTEMP = 0x82,
  MB_SENSOR_C2_2_NVME_CTEMP = 0x83,
  MB_SENSOR_C2_3_NVME_CTEMP = 0x84,
  MB_SENSOR_C2_4_NVME_CTEMP = 0x85,
  MB_SENSOR_C3_AVA_FTEMP = 0x86,
  MB_SENSOR_C3_AVA_RTEMP = 0x87,
  MB_SENSOR_C3_1_NVME_CTEMP = 0x88,
  MB_SENSOR_C3_2_NVME_CTEMP = 0x89,
  MB_SENSOR_C3_3_NVME_CTEMP = 0x8A,
  MB_SENSOR_C3_4_NVME_CTEMP = 0x8B,
  MB_SENSOR_C4_1_NVME_CTEMP = 0x8C,
  MB_SENSOR_C4_2_NVME_CTEMP = 0x8D,
  MB_SENSOR_C4_3_NVME_CTEMP = 0x8E,
  MB_SENSOR_C4_4_NVME_CTEMP = 0x8F,

  MB_SENSOR_C2_P12V_INA230_VOL = 0x90,
  MB_SENSOR_C2_P12V_INA230_CURR = 0x91,
  MB_SENSOR_C2_P12V_INA230_PWR = 0x92,
  MB_SENSOR_C3_P12V_INA230_VOL = 0x93,
  MB_SENSOR_C3_P12V_INA230_CURR = 0x94,
  MB_SENSOR_C3_P12V_INA230_PWR = 0x95,
  MB_SENSOR_C4_P12V_INA230_VOL = 0x96,
  MB_SENSOR_C4_P12V_INA230_CURR = 0x97,
  MB_SENSOR_C4_P12V_INA230_PWR = 0x98,
  MB_SENSOR_CONN_P12V_INA230_VOL = 0x99,
  MB_SENSOR_CONN_P12V_INA230_CURR = 0x9A,
  MB_SENSOR_CONN_P12V_INA230_PWR = 0x9B,

  MB_SENSOR_INLET_TEMP = 0xA0,
  MB_SENSOR_OUTLET_TEMP = 0xA1,
  MB_SENSOR_INLET_REMOTE_TEMP = 0xA3,
  MB_SENSOR_OUTLET_REMOTE_TEMP = 0xA4,
  MB_SENSOR_CPU0_TEMP = 0xAA,
  MB_SENSOR_CPU1_TEMP = 0xAB,
  MB_SENSOR_CPU0_DIMM_GRPA_TEMP = 0xAC,
  MB_SENSOR_CPU0_DIMM_GRPB_TEMP = 0xAD,
  MB_SENSOR_CPU1_DIMM_GRPC_TEMP = 0xAE,
  MB_SENSOR_CPU1_DIMM_GRPD_TEMP = 0xAF,

  MB_SENSOR_VR_CPU0_VCCIN_VOLT = 0xB0,
  MB_SENSOR_VR_CPU0_VCCIN_TEMP = 0xB1,
  MB_SENSOR_VR_CPU0_VCCIN_CURR = 0xB2,
  MB_SENSOR_VR_CPU0_VCCIN_POWER = 0xB3,
  MB_SENSOR_VR_CPU0_VSA_VOLT = 0xB4,
  MB_SENSOR_VR_CPU0_VSA_TEMP = 0xB5,
  MB_SENSOR_VR_CPU0_VSA_CURR = 0xB6,
  MB_SENSOR_VR_CPU0_VSA_POWER = 0xB7,
  MB_SENSOR_VR_CPU0_VCCIO_VOLT = 0xB8,
  MB_SENSOR_VR_CPU0_VCCIO_TEMP = 0xB9,
  MB_SENSOR_VR_CPU0_VCCIO_CURR = 0xBA,
  MB_SENSOR_VR_CPU0_VCCIO_POWER = 0xBB,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT = 0xBC,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP = 0xBD,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR = 0xBE,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER = 0xBF,

  MB_SENSOR_FAN0_TACH = 0xC0,
  MB_SENSOR_HSC_OUT_CURR = 0xC1,
  MB_SENSOR_HSC_TEMP = 0xC2,
  MB_SENSOR_FAN1_TACH = 0xC3,
  MB_SENSOR_VR_PCH_P1V05_VOLT = 0xC4,
  MB_SENSOR_VR_PCH_P1V05_TEMP = 0xC5,
  MB_SENSOR_VR_PCH_P1V05_CURR = 0xC6,
  MB_SENSOR_VR_PCH_P1V05_POWER = 0xC7,
  MB_SENSOR_VR_PCH_PVNN_VOLT = 0xC8,
  MB_SENSOR_VR_PCH_PVNN_TEMP = 0xC9,
  MB_SENSOR_VR_PCH_PVNN_CURR = 0xCA,
  MB_SENSOR_VR_PCH_PVNN_POWER = 0xCB,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT = 0xCC,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP = 0xCD,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR = 0xCE,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER = 0xCF,

  MB_SENSOR_P3V_BAT = 0xD0,
  MB_SENSOR_P3V3 = 0xD1,
  MB_SENSOR_P5V = 0xD2,
  MB_SENSOR_P12V = 0xD3,
  MB_SENSOR_P1V05 = 0xD4,
  MB_SENSOR_PVNN_PCH_STBY = 0xD5,
  MB_SENSOR_P3V3_STBY = 0xD6,
  MB_SENSOR_P5V_STBY = 0xD7,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER = 0xD8,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT = 0xD9,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP = 0xDA,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR = 0XDB,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER = 0XDC,

  MB_SENSOR_VR_CPU1_VCCIN_VOLT = 0xF0,
  MB_SENSOR_VR_CPU1_VCCIN_TEMP = 0xF1,
  MB_SENSOR_VR_CPU1_VCCIN_CURR = 0xF2,
  MB_SENSOR_VR_CPU1_VCCIN_POWER = 0xF3,
  MB_SENSOR_VR_CPU1_VSA_VOLT = 0xF4,
  MB_SENSOR_VR_CPU1_VSA_TEMP = 0xF5,
  MB_SENSOR_VR_CPU1_VSA_CURR = 0xF6,
  MB_SENSOR_VR_CPU1_VSA_POWER = 0xF7,
  MB_SENSOR_VR_CPU1_VCCIO_VOLT = 0xF8,
  MB_SENSOR_VR_CPU1_VCCIO_TEMP = 0xF9,
  MB_SENSOR_VR_CPU1_VCCIO_CURR = 0xFA,
  MB_SENSOR_VR_CPU1_VCCIO_POWER = 0xFB,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT = 0xFC,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP = 0xFD,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR = 0xFE,

//Discrete sensor
  MB_SENSOR_POWER_FAIL = 0x9C,
  MB_SENSOR_MEMORY_LOOP_FAIL = 0x9D,
  MB_SENSOR_PROCESSOR_FAIL = 0x65,
};

enum{
  MEZZ_SENSOR_TEMP = 0xA2,
};

enum {
  FAN0 = 0,
  FAN1 = 2,
};

enum {
  ADC_PIN0 = 0,
  ADC_PIN1,
  ADC_PIN2,
  ADC_PIN3,
  ADC_PIN4,
  ADC_PIN5,
  ADC_PIN6,
  ADC_PIN7,
};

enum {
  BOARD_REV_POWERON,
  BOARD_REV_EVT,
  BOARD_REV_PREDVT,
  BOARD_REV_DVT,
  BOARD_REV_PVT,
  BOARD_REV_MP,
};

enum {
  SLOT_CFG_SS_2x16  = 0x00,
  SLOT_CFG_DS_2x8   = 0x01,
  SLOT_CFG_SS_3x8   = 0x02,
  SLOT_CFG_EMPTY    = 0x03,
};

enum {
  BOOT_DEVICE_IPV4     = 0x1,
  BOOT_DEVICE_HDD      = 0x2,
  BOOT_DEVICE_CDROM    = 0x3,
  BOOT_DEVICE_OTHERS   = 0x4,
  BOOT_DEVICE_IPV6     = 0x9,
  BOOT_DEVICE_RESERVED = 0xff,
};

typedef struct _sensor_info_t {
  bool valid;
  sdr_full_t sdr;
} sensor_info_t;

int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_is_slot_server(uint8_t fru);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_sled_cycle(void);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_switch_uart_mux(uint8_t fru);
int pal_get_rst_btn(uint8_t *status);
int pal_set_rst_btn(uint8_t slot, uint8_t status);
int pal_set_led(uint8_t slot, uint8_t status);
int pal_set_hb_led(uint8_t status);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_list(char *list);
int pal_get_fru_id(char *fru_str, uint8_t *fru);
int pal_get_fru_name(uint8_t fru, char *name);
int pal_get_fruid_path(uint8_t fru, char *path);
int pal_get_fruid_eeprom_path(uint8_t fru, char *path);
int pal_get_fruid_name(uint8_t fru, char *name);
int pal_get_fru_sdr_path(uint8_t fru, char *path);
int pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units);
int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value);
int pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag);
int pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name);
int pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh,
    void *value);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_def_key_value();
void pal_dump_key_value(void);
int pal_get_fru_devtty(uint8_t fru, char *devtty);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_sys_guid(uint8_t fru, char *guid);
int pal_set_sys_guid(uint8_t fru, char *guid);
int pal_get_dev_guid(uint8_t fru, char *guid);
int pal_set_dev_guid(uint8_t fru, char *guid);
int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver);
int pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len);
int pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len);
int pal_fruid_write(uint8_t slot, char *path);
int pal_is_bmc_por(void);
int pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val);
int pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name);
int pal_parse_sel(uint8_t fru, uint8_t *sel,
    char *error_log);
int pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data);
void msleep(int msec);
int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);
int pal_set_fan_speed(uint8_t fan, uint8_t pwm);
int pal_get_fan_speed(uint8_t fan, int *rpm);
int pal_get_fan_name(uint8_t num, char *name);
void pal_update_ts_sled();
int pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen);
int pal_get_platform_id(uint8_t *id);
int pal_get_board_rev_id(uint8_t *id);
int pal_get_mb_slot_id(uint8_t *id);
int pal_get_slot_cfg_id(uint8_t *id);
void pal_log_clear(char *fru);
int pal_get_plat_sku_id(void);
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);
int pal_fan_dead_handle(int fan_num);
int pal_fan_recovered_handle(int fan_num);
int pal_PBO(void);
void pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh);
void pal_post_end_chk(uint8_t *post_end_chk);
int pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_add_cri_sel(char *str);
uint8_t pal_get_status(void);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
void pal_sensor_sts_check(uint8_t snr_num, float val, uint8_t *thresh);
int pal_get_syscfg_text(char *text);
int pal_add_i2c_device(uint8_t bus, char *device_name, uint8_t slave_addr);
int pal_del_i2c_device(uint8_t bus, uint8_t slave_addr);
int pal_is_fru_on_riser_card(uint8_t riser_slot, uint8_t *device_type);
bool pal_is_ava_card(uint8_t riser_slot);
bool pal_is_retimer_card ( uint8_t riser_slot );
bool pal_is_pcie_ssd_card( uint8_t riser_slot );
int pal_get_machine_configuration(char *conf);
void pal_check_power_sts(void);
int notify_BBV_ipmb_offline_online(uint8_t on_off, int off_sec);
bool pal_is_BBV_prsnt();
int pal_CPU_error_num_chk(bool is_caterr);
void pal_second_crashdump_chk(void);
int pal_mmap (uint32_t base, uint8_t offset, int option, uint32_t para);
int pal_control_mux_to_target_ch(uint8_t channel, uint8_t bus, uint8_t mux_addr);
int pal_uart_switch_for_led_ctrl (void);
int pal_riser_mux_switch (uint8_t riser_slot);
int pal_riser_mux_release (void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
