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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <openbmc/ipmi.h>

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
  STANDARD_CMD = 0,
  ACCURATE_CMD = 1, // legacy BIC definition, 2 byte value
  ACCURATE_CMD_4BYTE = 2, // lattest BIC definition, 4 byte value

  UNKNOWN_CMD = 0xFF,
};

typedef struct
{
  uint8_t iana_id[3];
  uint32_t value;
  uint8_t flags;
  uint8_t status;
  uint8_t ext_status;
  uint8_t read_type;
} ipmi_extend_sensor_reading_t;

#define ME_NETFN_OEM       0x2E
#define ME_CMD_SMBUS_READ  0x47
#define ME_CMD_SMBUS_WRITE 0x48
#define MAX_ME_SMBUS_WRITE_LEN 32
#define MAX_ME_SMBUS_READ_LEN  32
#define ME_SMBUS_WRITE_HEADER_LEN 14
#define MAX_POST_CODE_PAGE 17
#define MAX_POSTCODE_NUM  1024
#define PSB_EEPROM_BUS 0x03

//TI VR remaining writes
#define VR_REMAINING_WRITE_START_ADDR 0x0A
#define VR_REMAINING_WRITE_OFFSET(addr) ((addr & 0xf) | 0x40)
#define BIC_EEPROM_BUS 0x2
#define BIC_EEPROM_ADDR 0xA8
#define UNINITIALIZED_EEPROM 0xFFFF
#define TI_VR_REMAIN_WR_SIZE 2

//Halfdome TI VR remaining writes
#define HD_VR_REMAINING_WRITE_OFFSET(addr) ((addr << 1 & 0xf) | 0x40)

typedef struct {
  uint8_t bus_id;
  uint8_t addr;
} smbus_info;

typedef struct {
  uint8_t net_fn;
  uint8_t cmd;
  uint8_t mfg_id[3];
  uint8_t cpu_id;
  uint8_t smbus_id;
  uint8_t smbus_addr;
  uint8_t addr_size;
  uint8_t rlen;
  uint8_t addr[4];
} me_smb_read;

typedef struct {
  uint8_t net_fn;
  uint8_t cmd;
  uint8_t mfg_id[3];
  uint8_t cpu_id;
  uint8_t smbus_id;
  uint8_t smbus_addr;
  uint8_t addr_size;
  uint8_t tlen;
  uint8_t addr[4];
  uint8_t data[MAX_ME_SMBUS_WRITE_LEN];
} me_smb_write;

#define MAX_READ_RETRY 5

int bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id, uint8_t intf);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result, uint8_t intf);
int bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info, uint8_t intf);
int bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size, uint8_t intf);
int bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, uint8_t intf);
int bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf);
int bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver);
int bic_get_1ou_type(uint8_t slot_id, uint8_t *type);
int bic_set_amber_led(uint8_t slot_id, uint8_t dev_id, uint8_t status);
int bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf);
int bic_get_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_vr_device_id(uint8_t slot_id, uint8_t *devid, uint8_t *id_len, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_exp_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf);
int bic_get_sensor_reading(uint8_t slot_id, uint8_t sensor_num, ipmi_extend_sensor_reading_t *sensor, uint8_t intf);
int bic_is_exp_prsnt(uint8_t slot_id);
int me_recovery(uint8_t slot_id, uint8_t command);
int me_reset(uint8_t slot_id);
int bic_switch_mux_for_bios_spi(uint8_t slot_id, uint8_t mux);
int bic_set_gpio(uint8_t slot_id, uint8_t gpio_num,uint8_t value);
int remote_bic_set_gpio(uint8_t slot_id, uint8_t gpio_num,uint8_t value, uint8_t intf);
int bic_get_one_gpio_status(uint8_t slot_id, uint8_t gpio_num, uint8_t *value);
int bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t *data);
int bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t data);
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
                             uint8_t *ffi, uint8_t *meff, uint16_t *vendor_id, uint8_t *major_ver, uint8_t *minor_ver, uint8_t intf);
int bic_set_dev_power_status(uint8_t slot_id, uint8_t dev_id, uint8_t status, uint8_t intf);
int bic_disable_sensor_monitor(uint8_t slot_id, uint8_t dis, uint8_t intf);
int bic_reset(uint8_t slot_id);
int bic_inform_sled_cycle(void);
int bic_enable_ssd_sensor_monitor(uint8_t slot_id, bool enable, uint8_t intf);
int bic_set_vr_monitor_enable(uint8_t slot_id, bool enable, uint8_t intf);
uint8_t get_gpv3_bus_number(uint8_t dev_id);
uint8_t get_gpv3_channel_number(uint8_t dev_id);
int bic_notify_fan_mode(int mode);
int bic_get_dp_pcie_config(uint8_t slot_id, uint8_t *pcie_config);
int bic_set_bb_fw_update_ongoing(uint8_t component, uint8_t option);
int bic_check_bb_fw_update_ongoing();
int me_smbus_read(uint8_t slot_id, smbus_info info, uint8_t addr_size, uint32_t addr, uint8_t rlen, uint8_t *data);
int me_smbus_write(uint8_t slot_id, smbus_info info, uint8_t addr_size, uint32_t addr, uint8_t tlen, uint8_t *data);
void get_pmic_err_str(uint8_t err_type, char* str, uint8_t len);
int bic_check_cable_status();
int bic_get_card_type(uint8_t slot_id, uint8_t card_config, uint8_t *type);
int bic_get_tps_remaining_wr(uint8_t fru_id, uint8_t addr, uint16_t *remain);
int bic_set_tps_remaining_wr(uint8_t fru_id, uint8_t addr, uint16_t remain);
int bic_request_post_buffer_dword_data(uint8_t slot_id, uint32_t *port_buff, uint32_t input_len, uint32_t *output_len);
int bic_request_post_buffer_page_data(uint8_t slot_id, uint8_t page_num, uint8_t *port_buff, uint8_t *len);
int bic_get_mb_index(uint8_t *index);
int bic_get_prot_spare_pins(uint8_t slot_id, uint8_t* value) ;
bool bic_is_prot_bypass(uint8_t fru);
int bic_get_sys_fw_ver(uint8_t slot_id, uint8_t *ver);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_IPMI_H__ */
