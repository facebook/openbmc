/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#ifndef __WEDGE400_PEM_H__
#define __WEDGE400_PEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>

#define ERR_PRINT(fmt, args...) \
        fprintf(stderr, fmt ": %s\n", ##args, strerror(errno));

#define msleep(n) usleep(n*1000)

#define PEM1_EEPROM  "/sys/bus/i2c/devices/22-0050/eeprom"
#define PEM2_EEPROM  "/sys/bus/i2c/devices/23-0050/eeprom"
/* define for DELTA PEM */
#define DELTA_MODEL         "ECD24020003"
#define DELTA_HDR_LENGTH    32
#define UNLOCK_UPGRADE      0xf0
#define BOOT_FLAG           0xf1
#define DATA_TO_RAM         0xf2
#define DATA_TO_FLASH       0xf3
#define CRC_CHECK           0xf4

#define FW_IDENTICAL        1

#define NORMAL_MODE         0x00
#define BOOT_MODE           0x01

#define DELTA_PRI_NUM_OF_BLOCK    32
#define DELTA_PRI_NUM_OF_PAGE     44
#define DELTA_PRI_PAGE_START      16
#define DELTA_PRI_PAGE_END        59
#define DELTA_SEC_NUM_OF_BLOCK    32
#define DELTA_SEC_NUM_OF_PAGE     92
#define DELTA_SEC_PAGE_START      36
#define DELTA_SEC_PAGE_END        127
/* end */

#define PEM_BLACKBOX(num, chip) "/mnt/data/pem/pem"#num"_"#chip
#define PEM_LTC4282_REG_CNT 0x50
#define PEM_MAX6615_REG_CNT 0x20
#define PEM_ARCHIVE_BUFF ( PEM_LTC4282_REG_CNT > PEM_MAX6615_REG_CNT ? \
                          PEM_LTC4282_REG_CNT : PEM_MAX6615_REG_CNT )

/* max6615 sensors registers addr */
#define MAX6615_REG_TEMP(ch)                (0x00 + (ch))
#define MAX6615_REG_TEMP_EXT(ch)            (0x1E + (ch))
#define MAX6615_REG_FAN_CNT(ch)             (0x18 + (ch))
#define MAX6615_REG_TARGT_PWM(ch)           (0x0B + (ch))
#define MAX6615_REG_INSTANTANEOUS_PWM(ch)   (0x0D + (ch))

enum pem_chip_t {
  LTC4282,
  MAX6615,
  EEPROM,
  PEM_CHIP_CNT,
};

typedef struct _i2c_info_t {
  int fd;
  uint8_t bus;
  uint8_t chip_addr[PEM_CHIP_CNT];
  uint8_t chip_reg_cnt[PEM_CHIP_CNT];
  const char *file_path[PEM_CHIP_CNT];
} i2c_info_t;

typedef struct _pmbus_info_t {
  const char *item;
  uint8_t reg;
} pmbus_info_t;

typedef struct _smbus_info_t {
  const char *item;
  uint8_t reg;
  uint8_t len;
} smbus_info_t;

enum {
  READ,
  WRITE
};

enum {
  STOP,
  START
};

enum {
  DELTA_PEM,
  UNKNOWN
};

enum register_t{
  CONTROL,
  ALERT,
  FAULT_LOG,
  ADC_ALERT_LOG,
  FET_BAD_FAULT_TIME,
  GPIO_CONGIG,
  VGPIO_ALARM_MIN,
  VGPIO_ALARM_MAX,
  VSOURCE_ALARM_MIN,
  VSOURCE_ALARM_MAX,
  VSENSE_ALARM_MIN,
  VSENSE_ALARM_MAX,
  POWER_ALARM_MIN,
  POWER_ALARM_MAX,
  CLOCK_DIVIDER,
  ILIM_ADJUST,
  ENERGY,
  TIME_COUNTER,
  ALERT_CONTROL,
  ADC_CONTROL,
  STATUS,
  EE_CONTROL,
  EE_ALERT,
  EE_FAULT,
  EE_ADC_ALERT_LOG,
  EE_FET_BAD_FAULT_TIME,
  EE_GPIO_CONFIG,
  EE_VGPIO_ALARM_MIN,
  EE_VGPIO_ALARM_MAX,
  EE_VSOURCE_ALARM_MIN,
  EE_VSOURCE_ALARM_MAX,
  EE_VSENSE_ALARM_MIN,
  EE_VSENSE_ALARM_MAX,
  EE_POWER_ALARM_MIN,
  EE_POWER_ALARM_MAX,
  EE_CLOCK_DECIMATOR,
  EE_ILIM_ADJUST,

  VGPIO,
  VGPIO_MIN,
  VGPIO_MAX,
  VIN,
  VOUT,
  VSOURCE_MIN,
  VSOURCE_MAX,
  VSENSE,
  VSENSE_MIN,
  VSENSE_MAX,
  POWER,
  POWER_MIN,
  POWER_MAX,
  EE_SCRATCH,

  APP_FW_MAJOR,
  APP_FW_MINOR,
  BL_FW_MAJOR,
  BL_FW_MINOR,

  REG_NUM,
};

typedef struct _blackbox_info_t {
  smbus_info_t regs[REG_NUM];
} blackbox_info_t;

typedef struct _pem_status_t {
  union {
    uint16_t value;
    struct {
    uint16_t meter_overflow_present:1;
    uint16_t ticker_overflow_present:1;
    uint16_t adc_idle:1;
    uint16_t eeprom_busy:1;
    uint16_t alert_status:1;
    uint16_t gpio1_status:1;
    uint16_t gpio2_status:1;
    uint16_t gpio3_status:1;
    uint16_t ov_status:1;
    uint16_t uv_status:1;
    uint16_t oc_cooldown_status:1;
    uint16_t power_good_status:1;
    uint16_t on_pin_status:1;
    uint16_t fet_short_present:1;
    uint16_t fet_bad_cooldown_status:1;
    uint16_t on_status:1;
    } values;
  } reg_val;
} pem_status_t;

typedef struct _pem_control_t {
  union {
    uint16_t value;
    struct {
      uint16_t vin_mode:2;
      uint16_t ov_mode:2;
      uint16_t uv_mode:2;
      uint16_t fb_mode:2;
      uint16_t ov_autoretry:1;
      uint16_t uv_autoretry:1;
      uint16_t oc_autoretry:1;
      uint16_t fet_on:1;
      uint16_t mass_write_enable:1;
      uint16_t on_enb:1;
      uint16_t on_delay:1;
      uint16_t on_fault_mask:1;
    } values;
  } reg_val;
} pem_control_t;

typedef struct _pem_alert_t {
  union {
    uint16_t value;
    struct {
      uint16_t vgpio_alarm_low:1;
      uint16_t vgpio_alarm_high:1;
      uint16_t vsource_alarm_low:1;
      uint16_t vsource_alarm_high:1;
      uint16_t vsense_alarm_low:1;
      uint16_t vsense_alarm_high:1;
      uint16_t power_alarm_low:1;
      uint16_t power_alarm_high:1;
      uint16_t ov_alert:1;
      uint16_t uv_alert:1;
      uint16_t oc_alert:1;
      uint16_t pb_alert:1;
      uint16_t on_alert:1;
      uint16_t fet_short_alert:1;
      uint16_t fet_bad_fault_alert:1;
      uint16_t eeprom_done_alert:1;
    } values;
  } reg_val;
} pem_alert_t;

typedef struct _pem_fault_t {
  union {
    uint8_t value;
    struct {
      uint8_t ov_fault:1;
      uint8_t uv_fault:1;
      uint8_t oc_fault:1;
      uint8_t power_bad_fault:1;
      uint8_t on_fault:1;
      uint8_t fet_short_fault:1;
      uint8_t fet_bad_fault:1;
      uint8_t eeprom_done:1;
    } values;
  } reg_val;
} pem_fault_t;

typedef struct _pem_adc_alert_t {
  union {
    uint8_t value;
    struct {
      uint8_t vgpio_alarm_low:1;
      uint8_t vgpio_alarm_high:1;
      uint8_t vsource_alarm_low:1;
      uint8_t vsource_alarm_high:1;
      uint8_t vsense_alarm_low:1;
      uint8_t vsense_alarm_high:1;
      uint8_t power_alarm_low:1;
      uint8_t power_alarm_high:1;
    } values;
  } reg_val;
} pem_adc_alert_t;

typedef struct _pem_gpio_config_t {
  union {
    uint8_t value;
    struct {
      uint8_t meter_overflow_alert:1;
      uint8_t stress_to_gpio2:1;
      uint8_t adc_conv_alert:1;
      uint8_t gpio1_output:1;
      uint8_t gpio1_config:2;
      uint8_t gpio2_pd:1;
      uint8_t gpio3_pd:1;
    } values;
  } reg_val;
} pem_gpio_alert_t;

typedef struct _pem_clock_divider_t {
  union {
    uint8_t value;
    struct {
      uint8_t clock_divider:5;
      uint8_t int_clk_out:1;
      uint8_t tick_out:1;
      uint8_t coulomb_meter:1;
    } values;
  } reg_val;
} pem_clock_divider_t;

typedef struct _pem_ilim_adjust_t {
  union {
    uint8_t value;
    struct {
      uint8_t adc_resolution:1; //1 16-bit mode, 0 12-bit mode
      uint8_t gpio_mode:1;
      uint8_t vource_vdd:1;
      uint8_t foldback_mode:2;
      uint8_t ilim_adjust:3;
    } values;
  } reg_val;
} pem_ilim_adjust_t;

typedef struct _pem_status_regs_t {
  pem_fault_t fault;
  pem_adc_alert_t adc_alert_log;
  pem_status_t status;
} pem_status_regs_t;

typedef struct _pem_firmware_regs_t {
  uint8_t app_fw_major;
  uint8_t app_fw_minor;
  uint8_t bl_fw_major;
  uint8_t bl_fw_minor;
} pem_firmware_regs_t;

typedef struct _pem_eeprom_reg_t {
  pem_control_t control;
  pem_alert_t alert;
  pem_fault_t fault;
  pem_adc_alert_t adc_alert_log;
  uint8_t fet_bad_fault_time;
  pem_gpio_alert_t gpio_config;
  uint8_t vgpio_alarm_min;
  uint8_t vgpio_alarm_max;
  uint8_t vsource_alarm_min;
  uint8_t vsource_alarm_max;
  uint8_t vsense_alarm_min;
  uint8_t vsense_alarm_max;
  uint8_t power_alarm_min;
  uint8_t power_alarm_max;
  pem_clock_divider_t clock_decimator;
  pem_ilim_adjust_t ilim_adjust;
} pem_eeprom_reg_t;

typedef struct _delta_hdr_t {
  uint8_t crc[2];
  uint16_t page_start;
  uint16_t page_end;
  uint16_t byte_per_blk;
  uint16_t blk_per_page;
  uint8_t uc;
  uint8_t app_fw_major;
  uint8_t app_fw_minor;
  uint8_t bl_fw_major;
  uint8_t bl_fw_minor;
  uint8_t fw_id_len;
  uint8_t fw_id[16];
  uint8_t compatibility;
} delta_hdr_t;

int is_pem_prsnt(uint8_t num, uint8_t *status);
int get_mfr_model(uint8_t num, uint8_t *block);
int do_update_pem(uint8_t num, const char *file, const char *vendor, _Bool force);
int archive_pem_chips(uint8_t num);
int log_pem_critical_regs(uint8_t num);
int get_eeprom_info(uint8_t mum, const char *option);
int get_pem_info(uint8_t num);
int get_blackbox_info(uint8_t num, const char *option);
int get_archive_log(uint8_t num, const char *option);

#ifdef __cplusplus
}
#endif

#endif
