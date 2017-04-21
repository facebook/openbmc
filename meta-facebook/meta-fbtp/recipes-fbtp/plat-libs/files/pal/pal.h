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
#include <stdbool.h>

#define MAX_KEY_LEN     64
#define MAX_NUM_FAN     2

#define FRU_STATUS_GOOD   1
#define FRU_STATUS_BAD    0

#define KV_STORE "/mnt/data/kv_store/%s"
#define KV_STORE_PATH "/mnt/data/kv_store"

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)

#define MAX_NODES 4

#define MAX_NUM_FRUS 6

#define MAX_SDR_LEN           64
#define MAX_SENSOR_NUM        0xFF
#define MAX_SENSOR_THRESHOLD  8
#define MAX_RETRIES_SDR_INIT  30
#define THERMAL_CONSTANT      255
#define ERR_NOT_READY         -2


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

enum {
  LED_STATE_OFF,
  LED_STATE_ON,
};

enum {
  USB_MUX_OFF,
  USB_MUX_ON,
};

enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_POWER_RESET,
  SERVER_GRACEFUL_SHUTDOWN,
};

enum {
  UART_TO_BMC,
  UART_TO_DEBUG,
};

enum {
  OS_BOOT = 0x1F,
};

enum {
  SYSTEM_EVENT = 0xE9,
  THERM_THRESH_EVT = 0x7D,
  BUTTON = 0xAA,
  POWER_STATE = 0xAB,
  CRITICAL_IRQ = 0xEA,
  POST_ERROR = 0x2B,
  MACHINE_CHK_ERR = 0x40,
  PCIE_ERR = 0x41,
  IIO_ERR = 0x43,
  MEMORY_ECC_ERR = 0X63,
  MEMORY_ERR_LOG_DIS = 0X87,
  PROCHOT_EXT = 0X51,
  PWR_ERR = 0X56,
  CATERR = 0xE6,
  CPU_DIMM_HOT = 0xB3,
  SOFTWARE_NMI = 0x90,
  CPU0_THERM_STATUS = 0x1C,
  CPU1_THERM_STATUS = 0x1D,
  SPS_FW_HEALTH = 0x17,
  NM_EXCEPTION = 0x18,
  PWR_THRESH_EVT = 0x3B,
  MSMI = 0xE7
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
};

/* Enum for type of Upper and Lower threshold values */
enum {
  SENSOR_VALID = 0x0,
  UCR_THRESH = 0x01,
  UNC_THRESH,
  UNR_THRESH,
  LCR_THRESH,
  LNC_THRESH,
  LNR_THRESH,
  POS_HYST,
  NEG_HYST,
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

//Used identify VR Chip info. there are 4 vr fw code in EVT3 and after
enum
{
    SS_Mix = 0x1,
    DS_Fairchild = 0x2,
    DS_IFX = 0x3,
};

enum {
  VR_CPU0_VCCIN = 0x90,
  VR_CPU0_VSA = 0x90,
  VR_CPU0_VCCIO = 0x94,
  VR_CPU0_VDDQ_ABC = 0xE0,
  VR_CPU0_VDDQ_DEF = 0xE4,
  VR_CPU0_VDDQ_ABC_EVT = 0x1C,
  VR_CPU0_VDDQ_DEF_EVT= 0x14,
  VR_CPU1_VCCIN = 0xB0,
  VR_CPU1_VSA = 0xB0,
  VR_CPU1_VCCIO = 0xB4,
  VR_CPU1_VDDQ_GHJ = 0xA0,
  VR_CPU1_VDDQ_KLM = 0xA4,
  VR_CPU1_VDDQ_GHJ_EVT = 0x0C,
  VR_CPU1_VDDQ_KLM_EVT = 0x04,
  VR_PCH_PVNN = 0xD0,
  VR_PCH_P1V05 = 0xD0,
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

typedef struct _sensor_info_t {
  bool valid;
  sdr_full_t sdr;
} sensor_info_t;

//VR Chip info
typedef struct
{
  uint8_t SlaveAddr;

  uint8_t CRC[4];

  int  DataLength;

  int IndexStartAddr;

  int IndexEndAddr;

} VRBasicInfo;

int pal_get_platform_name(char *name);
int pal_get_num_slots(uint8_t *num);
int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_is_fru_ready(uint8_t fru, uint8_t *status);
int pal_is_slot_server(uint8_t fru);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_sled_cycle(void);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_hand_sw(uint8_t *pos);
int pal_switch_usb_mux(uint8_t slot);
int pal_switch_uart_mux(uint8_t fru);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_get_pwr_btn(uint8_t *status);
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
int pal_sensor_read(uint8_t fru, uint8_t sensor_num, void *value);
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
int pal_get_vr_ver(uint8_t vr, uint8_t *ver);
int pal_get_vr_checksum(uint8_t vr, uint8_t *checksum);
int pal_get_vr_deviceId(uint8_t vr, uint8_t *deviceId);
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
int pal_get_poss_pcie_config(uint8_t *pcie_config);
int pal_get_pwm_value(uint8_t fan_num, uint8_t *value);
int pal_fan_dead_handle(int fan_num);
int pal_fan_recovered_handle(int fan_num);
static bool is_cpu0_socket_occupy(void);
static bool is_cpu1_socket_occupy(void);
int pal_get_vr_update_data(char *update_file, uint8_t **BinData);
int pal_vr_fw_update(uint8_t *BinData);
int pal_PBO(void);
void pal_sensor_assert_handle(uint8_t snr_num, float val, uint8_t thresh);
void pal_sensor_deassert_handle(uint8_t snr_num, float val, uint8_t thresh);
void pal_set_post_end(void);
void pal_post_end_chk(uint8_t *post_end_chk);
int pal_get_fw_info(unsigned char target, unsigned char* res, unsigned char* res_len);
void pal_add_cri_sel(char *str);
int pal_update_MCU(char *path);
int pal_update_MCU_bl(char *path);
int pal_bmc_err_enable(void);
int pal_bmc_err_disable(void);
int pal_set_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t list_length, uint8_t *cc);
int pal_get_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t *list_length);
int pal_set_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device);
int pal_get_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device);
int pal_set_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting);
int pal_get_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data);
uint8_t pal_get_status(void);
void pal_set_boot_option(unsigned char para,unsigned char* pbuff);
int pal_get_boot_option(unsigned char para,unsigned char* pbuff);
void pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
