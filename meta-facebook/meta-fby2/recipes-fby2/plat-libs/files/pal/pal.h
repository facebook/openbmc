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
#include <openbmc/kv.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <facebook/bic.h>
#include <facebook/fby2_common.h>
#include <facebook/fby2_fruid.h>
#include <facebook/fby2_sensor.h>

#define MAX_NUM_FAN     2

#define MAX_NODES 4

#define MAX_NIC_TEMP_RETRY 3

#define SOCK_PATH_ASD_BIC "/tmp/asd_bic_socket"
#define SOCK_PATH_JTAG_MSG "/tmp/jtag_msg_socket"

#define YV250_NVMe_Temp_Dev_UCR 75

#define MAX_DEV_JTAG_GPIO 4

extern char * key_list[];
extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;
extern char pal_pwm_list[];
extern char pal_tach_list[];
extern const char pal_fru_list[];
extern const char pal_server_list[];
extern const char pal_dev_fru_list[];
extern const char pal_dev_pwr_list[];
extern const char pal_dev_pwr_option_list[];

enum {
  USB_MUX_OFF,
  USB_MUX_ON,
};

enum {
  DEVICE_POWER_OFF,
  DEVICE_POWER_ON,
};

enum {
  FFI_STORAGE,
  FFI_ACCELERATOR,
};

enum {
  MEFF_M2_22110 = 0x35,
  MEFF_DUAL_M2 = 0xF0,
};


enum {
  DEV_FRU_NOT_COMPLETE,
  DEV_FRU_COMPLETE,
  DEV_FRU_IGNORE,
};

enum {
  HAND_SW_SERVER1 = 1,
  HAND_SW_SERVER2,
  HAND_SW_SERVER3,
  HAND_SW_SERVER4,
  HAND_SW_BMC
};

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  FAN_0 = 0,
  FAN_1,
  FAN_2,
  FAN_3,
};

enum {
  POST_RESET = 0,
  POST_SET
};

enum {
  POST_END_CHECK = 0,
  NVME_READY_CHECK
};

enum {
  IMC_DUMP_END = 0,
  IMC_DUMP_START,
  IMC_DUMP_PROCESS,
};

enum {
  TRANS_TYPE_VALID = 0,
  OPER_VALID,
  LEVEL_VALID,
  PROC_CONTEXT_CORRUPT_VALID,
  CORR_VALID,
  PRECISE_PC_VALID,
  RESTART_PC_VALID,
  PARTICIPATION_TYPE_VALID,
  TIMEOUT_VALID,
  ADDRESS_SPACE_VALID,
  MEM_ATTR_VALID,
  ACCESS_MODE_VALID
};

enum {
  CACHE_ERROR = 0,
  TLB_ERROR,
  BUS_ERROR,
  MICRO_ARCH_ERROR
};

enum {
  DRIVE_NOT_READY = 0,
  DRIVE_READY
};

int pal_is_slot_latch_closed(uint8_t slot_id, uint8_t *status);
int pal_get_dev_info(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *type, uint8_t force);
void pal_power_policy_control(uint8_t slot_id, char *last_ps, bool force);
int pal_baseboard_clock_control(uint8_t slot_id, int ctrl);
int pal_is_server_12v_on(uint8_t slot_id, uint8_t *status);
int pal_slot_pair_12V_off(uint8_t slot_id);
bool pal_is_hsvc_ongoing(uint8_t slot_id);
int pal_set_hsvc_ongoing(uint8_t slot_id, uint8_t status, uint8_t ident);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_get_hand_sw_physically(uint8_t *pos);
int pal_get_hand_sw(uint8_t *pos);
int pal_get_usb_sw(uint8_t *pos);
int pal_enable_usb_mux(uint8_t state);
int pal_switch_vga_mux(uint8_t slot);
int pal_switch_usb_mux(uint8_t slot);
int pal_switch_uart_mux(uint8_t slot);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *post);
int pal_get_pwr_btn(uint8_t *status);
int pal_get_rst_btn(uint8_t *status);
int pal_set_sled_led(uint8_t status);
int pal_set_id_led(uint8_t slot, uint8_t status);
int pal_set_slot_id_led(uint8_t slot, uint8_t status);
int pal_get_fru_sdr_path(uint8_t fru, char *path);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_get_pair_slot_type(uint8_t fru);
int pal_system_config_check(uint8_t slot_id);
int pal_read_nic_fruid(const char *path, int size);
void pal_notify_nic(uint8_t slot);
int pal_get_fan_latch(uint8_t *status);
int pal_get_fru_post(uint8_t fru, uint8_t *value);
uint8_t pal_is_post_ongoing();
int pal_set_nvme_ready(uint8_t fru, uint8_t value);
int pal_get_nvme_ready(uint8_t fru, uint8_t *value);
uint8_t pal_is_nvme_ready();
int pal_ignore_thresh(uint8_t fru, uint8_t snr_num, uint8_t thresh);
long pal_get_fscd_counter();
int pal_set_fscd_counter(long value);
int pal_set_ignore_thresh(int value);
int pal_get_ignore_thresh(int *value);
int pal_set_post_start_timestamp(uint8_t fru, uint8_t method);
int pal_get_post_start_timestamp(uint8_t fru, long *value);
int pal_set_post_end_timestamp(uint8_t fru);
int pal_get_post_end_timestamp(uint8_t fru, long *value);
int pal_set_nvme_ready_timestamp(uint8_t fru);
int pal_get_nvme_ready_timestamp(uint8_t fru, long *value);
uint8_t pal_is_post_time_out();
uint8_t pal_is_nvme_time_out();
void pal_check_fscd_watchdog();
uint8_t pal_get_server_type(uint8_t fru);
int pal_set_tpm_timeout(uint8_t slot, int timeout);
int pal_set_tpm_physical_presence_reset(uint8_t slot, uint8_t reset);
int pal_get_sensor_util_timeout(uint8_t fru);
int pal_set_m2_prsnt(uint8_t slot_id, uint8_t dev_id, uint8_t present);
int pal_is_ocp30_nic(void);
int pal_set_dev_config_setup(uint8_t value);
int pal_get_dev_config_setup(uint8_t *value);
int pal_set_dev_sdr_setup(uint8_t fru, uint8_t value);
int pal_get_dev_sdr_setup(uint8_t fru, uint8_t *value);
int pal_get_fan_config();
int pal_set_update_sdr_flag(uint8_t fru, uint8_t value);
int pal_get_update_sdr_flag(uint8_t fru, uint8_t *value);
int8_t pal_init_dev_jtag_gpio(uint8_t fru, uint8_t dev);
int8_t pal_is_dev_com_sel_en (uint8_t fru);
int8_t pal_dev_jtag_gpio_to_bus(uint8_t fru);
bool pal_is_all_fan_fail();
int pal_set_last_postcode(uint8_t slot, uint32_t postcode);
int pal_get_last_postcode(uint8_t slot, char* postcode);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
