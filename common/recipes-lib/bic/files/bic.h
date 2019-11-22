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

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_SENSOR_INFO_LEN  6

#define CC_BIC_RETRY 0x70
#define BIC_RETRY_ACTION  -2

// Server type
enum {
  SERVER_TYPE_ML = 0x0,
  SERVER_TYPE_TL = 0x0,
  SERVER_TYPE_RC = 0x1,
  SERVER_TYPE_EP = 0x2,
  SERVER_TYPE_ND = 0x4,
  SERVER_TYPE_NONE = 0xFF,
};

enum {
  SLOT_TYPE_SERVER = 0,
  SLOT_TYPE_CF     = 1,
  SLOT_TYPE_GP     = 2,
  SLOT_TYPE_NULL   = 3,
  SLOT_TYPE_GPV2   = 4,
};

enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_BIC,
  UPDATE_VR,
  UPDATE_PCIE_SWITCH,
};

enum {
  DUMP_BIOS = 0,
};

//Force Intel ME Recovery Command Options
enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
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

// Structure for Accuracy Sensor Reading (Bridge IC spec v0.6)
typedef struct {
  uint8_t int_value;
  uint8_t dec_value;
  uint8_t flags;
} ipmi_accuracy_sensor_reading_t;

typedef struct _ipmi_device_sensor_reading_t {
  ipmi_device_sensor_t data[MAX_NUM_DEV_SENSORS];
} ipmi_device_sensor_reading_t;

int bic_is_slot_12v_on(uint8_t slot_id);
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
int bic_update_fw(uint8_t slot_id, uint8_t comp, char *path);
int bic_imc_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int me_recovery(uint8_t slot_id, uint8_t command);
int bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result);
int bic_read_accuracy_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_accuracy_sensor_reading_t *sensor);
int bic_get_slot_type(uint8_t fru);
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
int bic_set_sdr_update_flag(uint8_t slot, uint8_t update);
int bic_get_sdr_update_flag(uint8_t slot);
int bic_set_sdr_threshold_update_flag(uint8_t slot, uint8_t update);
int bic_get_sdr_threshold_update_flag(uint8_t slot);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
