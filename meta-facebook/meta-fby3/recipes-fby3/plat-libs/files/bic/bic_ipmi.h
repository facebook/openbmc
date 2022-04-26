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

#ifndef __BIC_IPMI_H__
#define __BIC_IPMI_H__

#include "bic_xfer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  DEV_TYPE_UNKNOWN,
  DEV_TYPE_M2,
  DEV_TYPE_SSD,
  DEV_TYPE_BRCM_ACC,
  DEV_TYPE_SPH_ACC,
  DEV_TYPE_DUAL_M2,
};

enum {
  FFI_STORAGE,
  FFI_ACCELERATOR,
};

enum {
  VENDOR_SAMSUNG = 0x144D,
  VENDOR_VSI = 0x1D9B,
  VENDOR_BRCM = 0x14E4,
  VENDOR_SPH = 0x8086,
};

enum {
  SET_LED_OFF = 0x0,
  SET_LED_ON = 0x1, 
  SET_LED_BLINK_START = 0x2,
  SET_LED_BLINK_STOP = 0x3,
  GET_LED_STAT = 0x4,
  CMD_UNKNOWN = 0xff,
};

#define MAX_READ_RETRY 5

#define CONFIG_M2_SINGLE 0x08
#define CONFIG_M2_DUAL 0x04

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id, uint8_t intf);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result, uint8_t intf);
int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info, uint8_t intf);
int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size, uint8_t intf, uint8_t less_retry);
int bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, uint8_t intf);
int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf);
int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);
int bic_get_vr_vendor_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t *rlen);
int bic_get_1ou_type(uint8_t slot_id, uint8_t *type);
int bic_get_1ou_type_cache(uint8_t slot_id, uint8_t *type);
int bic_set_amber_led(uint8_t slot_id, uint8_t dev_id, uint8_t status);
int bic_get_amber_led_status(uint8_t slot_id, uint8_t dev_id, uint8_t *status);
int bic_spe_led_ctrl(uint8_t dev_id, uint8_t option, uint8_t* status);
int bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf);
int bic_get_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_vr_device_id(uint8_t slot_id, uint8_t comp, uint8_t *rbuf, uint8_t *rlen, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_vr_ver(uint8_t slot_id, uint8_t intf, uint8_t bus, uint8_t addr, char *key, char *ver_str);
int bic_get_vr_ver_cache(uint8_t slot_id, uint8_t intf, uint8_t bus, uint8_t addr, char *ver_str);
int bic_get_exp_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_sensor_reading(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor, uint8_t intf);
int bic_is_m2_exp_prsnt(uint8_t slot_id);
int bic_is_m2_exp_prsnt_cache(uint8_t slot_id);
int me_recovery(uint8_t slot_id, uint8_t command);
int me_reset(uint8_t slot_id);
int bic_switch_mux_for_bios_spi(uint8_t slot_id, uint8_t mux);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio_num,uint8_t value);
int remote_bic_set_gpio(uint8_t slot_id, uint8_t gpio_num,uint8_t value, uint8_t intf);
int bic_get_one_gpio_status(uint8_t slot_id, uint8_t gpio_num, uint8_t *value);
int bic_asd_init(uint8_t slot_id, uint8_t cmd);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t *data);
int remote_bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t *data, uint8_t intf);
int bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t data);
int remote_bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t data, uint8_t intf);
int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid);
int bic_do_sled_cycle(uint8_t slot_id);
int bic_set_fan_speed(uint8_t fan_id, uint8_t pwm);
int bic_manual_set_fan_speed(uint8_t fan_id, uint8_t pwm);
int bic_get_fan_speed(uint8_t fan_id, float *value);
int bic_get_fan_pwm(uint8_t fan_id, float *value);
int bic_do_12V_cycle(uint8_t slot_id);
int bic_get_dev_info(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, uint8_t *type);
int bic_get_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t *nvme_ready, uint8_t *status, \
                             uint8_t *ffi, uint8_t *meff, uint16_t *vendor_id, uint8_t *major_ver, uint8_t *minor_ver,uint8_t *additional_ver, uint8_t intf);
int bic_set_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t status, uint8_t intf);
int bic_get_ifx_vr_remaining_writes(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *writes, uint8_t intf);
int bic_get_isl_vr_remaining_writes(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *writes, uint8_t intf);
int bic_reset(uint8_t slot_id, uint8_t intf);
int bic_usb_hub_reset(uint8_t slot_id, uint8_t board_type, uint8_t intf);
int bic_clear_cmos(uint8_t slot_id);
int bic_inform_sled_cycle(void);
int bic_get_gpv3_pci_link(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf);
int bic_enable_ssd_sensor_monitor(uint8_t slot_id, bool enable, uint8_t intf);
int bic_enable_vr_fault_monitor(uint8_t slot_id, bool enable, uint8_t intf) ;
uint8_t get_gpv3_bus_number(uint8_t dev_id);
uint8_t get_gpv3_channel_number(uint8_t dev_id);
int bic_get_dp_pcie_config(uint8_t slot_id, uint8_t *pcie_config);
bool bic_is_fw_update_ongoing(uint8_t fruid);
bool bic_is_crit_act_ongoing(uint8_t fruid);
int bic_get_mb_index(uint8_t *index);
int bic_bypass_to_another_bmc(uint8_t* data, uint8_t len);
int bic_set_crit_act_flag(uint8_t dir_type);
int bic_is_2u_top_bot_prsnt(uint8_t slot_id);
int bic_is_2u_top_bot_prsnt_cache(uint8_t slot_id);
int bic_enable_ina233_alert(uint8_t fru, bool enable);
int bb_fw_update_prepare(uint8_t slot_id);
int bb_fw_update_finish(uint8_t slot_id);
int bic_get_oem_sensor_reading(uint8_t slot_id, uint8_t index, ipmi_sensor_reading_t *sensor, uint8_t mul, uint8_t intf);
int bic_get_m2_config(uint8_t *config, uint8_t slot, uint8_t intf);
void bic_open_cwc_usb(uint8_t slot);
void bic_close_cwc_usb(uint8_t slot);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_IPMI_H__ */
