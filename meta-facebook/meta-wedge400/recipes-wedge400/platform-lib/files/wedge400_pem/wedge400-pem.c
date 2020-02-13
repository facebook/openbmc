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

#include <signal.h>
#include <assert.h>
#include <libgen.h>
#include <openbmc/kv.h>
#include <openbmc/log.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pmbus.h>
#include <openbmc/pal.h>
#include <openbmc/fruid.h>
#include <facebook/wedge_eeprom.h>
#include "wedge400-pem.h"

// #define DEBUG

static delta_hdr_t delta_hdr;
static int g_fd = -1;

static void
exithandler(int signum) {
  printf("\nPEM update abort!\n");
  syslog(LOG_WARNING, "PEM update abort!");
  close(g_fd);
  run_command("rm /var/run/pem-util.pid");
  exit(0);
}

i2c_info_t pem[] = {
  {-1, 22, {0x58, 0x18, 0x50}, {PEM_LTC4282_REG_CNT, PEM_MAX6615_REG_CNT, 0},
   {PEM_BLACKBOX(1, ltc4282), PEM_BLACKBOX(1, max6615), PEM1_EEPROM}},
  {-1, 23, {0x58, 0x18, 0x50}, {PEM_LTC4282_REG_CNT, PEM_MAX6615_REG_CNT, 0},
   {PEM_BLACKBOX(2, ltc4282), PEM_BLACKBOX(2, max6615), PEM2_EEPROM}},
};

smbus_info_t smbus_ltc4282[] = {
  {"CONTROL", 0x00, 2},
  {"ALERT", 0x02, 2},
  {"FAULT_LOG", 0x04, 1},
  {"ADC_ALERT_LOG", 0x05, 1},
  {"FET_BAD_FAULT_TIME", 0x06, 1},
  {"GPIO_CONGIG", 0x07, 1},
  {"VGPIO_ALARM_MIN", 0x08, 1},
  {"VGPIO_ALARM_MAX", 0x09, 1},
  {"VSOURCE_ALARM_MIN", 0x0A, 1},
  {"VSOURCE_ALARM_MAX", 0x0B, 1},
  {"VSENSE_ALARM_MIN", 0x0C, 1},
  {"VSENSE_ALARM_MAX", 0x0D, 1},
  {"POWER_ALARM_MIN", 0x0E, 1},
  {"POWER_ALARM_MAX", 0x0F, 1},
  {"CLOCK_DIVIDER", 0x10, 1},
  {"ILIM_ADJUST", 0x11, 1},
  {"ENERGY", 0x12, 6},
  {"TIME_COUNTER", 0x18, 4},
  {"ALERT_CONTROL", 0x1C, 1},
  {"ADC_CONTROL", 0x1D, 1},
  {"STATUS", 0x1E, 2},

  {"EE_CONTROL", 0x20, 2},
  {"EE_ALERT", 0x22, 2},
  {"EE_FAULT_LOG", 0x24, 1},
  {"EE_ADC_ALERT_LOG", 0x25, 1},
  {"EE_FET_BAD_FAULT_TIME", 0x26, 1},
  {"EE_GPIO_CONFIG", 0x27, 1},
  {"EE_VGPIO_ALARM_MIN", 0x28, 1},
  {"EE_VGPIO_ALARM_MAX", 0x29, 1},
  {"EE_VSOURCE_ALARM_MIN", 0x2A, 1},
  {"EE_VSOURCE_ALARM_MAX", 0x2B, 1},
  {"EE_VSENSE_ALARM_MIN", 0x2C, 1},
  {"EE_VSENSE_ALARM_MAX", 0x2D, 1},
  {"EE_POWER_ALARM_MIN", 0x2E, 1},
  {"EE_POWER_ALARM_MAX", 0x2F, 1},
  {"EE_CLOCK_DECIMATOR", 0x30, 1},
  {"EE_ILIM_ADJUST", 0x31, 1},

  {"VGPIO", 0x34, 2},
  {"VGPIO_MIN", 0x36, 2},
  {"VGPIO_MAX", 0x38, 2},
  {"VIN", 0x3A, 2},
  {"VOUT", 0x3A, 2},
  {"VSOURCE_MIN", 0x3C, 2},
  {"VSOURCE_MAX", 0x3E, 2},
  {"VSENSE", 0x40, 2},
  {"VSENSE_MIN", 0x42, 2},
  {"VSENSE_MAX", 0x44, 2},
  {"POWER", 0x46, 2},
  {"POWER_MIN", 0x48, 2},
  {"POWER_MAX", 0x4A, 2},
  {"EE_SCRATCH", 0x4C, 4},

  {"APP_FW_MAJOR", 0x50, 1},
  {"APP_FW_MINOR", 0x51, 1},
  {"BL_FW_MAJOR", 0x52, 1},
  {"BL_FW_MINOR", 0x53, 1},
};

pem_eeprom_reg_t pem_default_config = {
  .control.reg_val.value = 0x3f02,
  .alert.reg_val.value = 0,
  .fault.reg_val.value = 0,
  .adc_alert_log.reg_val.value = 0,
  .fet_bad_fault_time = 0xff,
  .gpio_config.reg_val.value = 0x40,
  .vgpio_alarm_min = 0,
  .vgpio_alarm_max = 0xff,
  .vsource_alarm_min = 0,
  .vsource_alarm_max = 0xff,
  .vsense_alarm_min = 0,
  .vsense_alarm_max = 0xff,
  .power_alarm_min = 0,
  .power_alarm_max = 0xff,
  .clock_decimator.reg_val.value = 0x08,
  .ilim_adjust.reg_val.value = 0x55,
};

static void
sensord_operation(uint8_t num, uint8_t action) {

  if (action == STOP) {
    syslog(LOG_WARNING, "Stop monitor PEM%d sensor to update", num + 1);
    run_command("sv stop sensord > /dev/null");
    switch (num) {
      case 0:
        run_command("/usr/local/bin/sensord scm smb pem2 > /dev/null 2>&1 &");
        break;
      case 1:
        run_command("/usr/local/bin/sensord scm smb pem1> /dev/null 2>&1 &");
        break;
    }
  } else if (action == START) {
    run_command("killall sensord");
    run_command("sv start sensord > /dev/nul");
    syslog(LOG_WARNING, "Start monitor PEM%d sensor", num + 1);
  }
}

/*
 * for reading ltc4282 registers
 */
static int pem_reg_read(int fd, uint8_t reg, void *value) {
  uint8_t byte;
  uint16_t word = 0;

  if (fd <= 0 || value == NULL)
    return -1;

  switch (reg) {
    case ENERGY:
    case TIME_COUNTER:
    case EE_SCRATCH:
      return -1;

    case VGPIO:
      byte = i2c_smbus_read_byte_data(fd, smbus_ltc4282[ILIM_ADJUST].reg);
      /* Select GPIO3 */
      byte &= (~0x03);
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[ILIM_ADJUST].reg, byte);
      msleep(100);
      word = i2c_smbus_read_word_data(fd, smbus_ltc4282[reg].reg);
      *(uint16_t *)value = ((uint16_t)(word << 8) | word >> 8);
      break;
    case VIN:
      byte = i2c_smbus_read_byte_data(fd, smbus_ltc4282[ILIM_ADJUST].reg);
      /* Select Vin, Enable 12-bits mode */
      byte |= 0x05;
      byte &= (~0x03);
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[ILIM_ADJUST].reg, byte);
      msleep(100);
      word = i2c_smbus_read_word_data(fd, smbus_ltc4282[reg].reg);
      *(uint16_t *)value = ((uint16_t)(word << 8) | word >> 8);
      break;
    case VOUT:
      byte = i2c_smbus_read_byte_data(fd, smbus_ltc4282[EE_ILIM_ADJUST].reg);
      /* Select Vout, Enable 12-bits mode */
      byte |= 0x01;
      byte &= (~0x05);
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[ILIM_ADJUST].reg, byte);
      msleep(100);
      word = i2c_smbus_read_word_data(fd, smbus_ltc4282[reg].reg);
      *(uint16_t *)value = ((uint16_t)(word << 8) | word >> 8);
      break;

    case CONTROL:
    case ALERT:
    case STATUS:
    case VGPIO_MIN:
    case VGPIO_MAX:
    case VSOURCE_MIN:
    case VSOURCE_MAX:
    case VSENSE:
    case VSENSE_MIN:
    case VSENSE_MAX:
    case POWER:
    case POWER_MIN:
    case POWER_MAX:
    case EE_CONTROL:
    case EE_ALERT:
      word = i2c_smbus_read_word_data(fd, smbus_ltc4282[reg].reg);
      *(uint16_t *)value = ((uint16_t)(word << 8) | word >> 8);
      break;

    case FAULT_LOG:
    case ADC_ALERT_LOG:
    case FET_BAD_FAULT_TIME:
    case GPIO_CONGIG:
    case VGPIO_ALARM_MIN:
    case VGPIO_ALARM_MAX:
    case VSOURCE_ALARM_MIN:
    case VSOURCE_ALARM_MAX:
    case VSENSE_ALARM_MIN:
    case VSENSE_ALARM_MAX:
    case POWER_ALARM_MIN:
    case POWER_ALARM_MAX:
    case CLOCK_DIVIDER:
    case ILIM_ADJUST:
    case ALERT_CONTROL:
    case ADC_CONTROL:
    case EE_FAULT:
    case EE_ADC_ALERT_LOG:
    case EE_FET_BAD_FAULT_TIME:
    case EE_GPIO_CONFIG:
    case EE_VGPIO_ALARM_MIN:
    case EE_VGPIO_ALARM_MAX:
    case EE_VSOURCE_ALARM_MIN:
    case EE_VSOURCE_ALARM_MAX:
    case EE_VSENSE_ALARM_MIN:
    case EE_VSENSE_ALARM_MAX:
    case EE_POWER_ALARM_MIN:
    case EE_POWER_ALARM_MAX:
    case EE_CLOCK_DECIMATOR:
    case EE_ILIM_ADJUST:
    case APP_FW_MAJOR:
    case APP_FW_MINOR:
    case BL_FW_MAJOR:
    case BL_FW_MINOR:
      byte = i2c_smbus_read_byte_data(fd, smbus_ltc4282[reg].reg);
      *(uint8_t *)value = byte;
      break;
    default:
      return -1;
      break;
  }

  return 0;
}

/*
 * if write ltc4282 eeprom regs, check if write done
 */
static int wait_eeprom_done(int fd) {
  pem_status_t status;
  int retry = 10;

  if(fd <= 0)
    return -1;

  while(retry--) {
    if(pem_reg_read(fd, STATUS, &status.reg_val.value) == -1)
      return -1;
    if(status.reg_val.values.eeprom_busy == 0) {
      break;
    }
    msleep(100);
  }
  if(retry == 0)
    return -1;

  return 0;
}

/*
 * for writing ltc4282 registers
 */
static int pem_reg_write(int fd, uint8_t reg, int value) {
  uint8_t byte;
  uint16_t word;

  if (fd <= 0)
    return -1;

  switch (reg) {
    case ENERGY:
    case TIME_COUNTER:
    case EE_SCRATCH:
    case VIN:
    case VOUT:
    case VGPIO:
    case VSENSE:
    case POWER:
    case STATUS:
        break;
    case CONTROL:
    case ALERT:
    case VGPIO_MIN:
    case VGPIO_MAX:
    case VSOURCE_MIN:
    case VSOURCE_MAX:
    case VSENSE_MIN:
    case VSENSE_MAX:
    case POWER_MIN:
    case POWER_MAX:
      word = value & 0xFFFF;
      word = i2c_smbus_write_word_data(fd, smbus_ltc4282[reg].reg, word);
      break;
    case EE_CONTROL:
    case EE_ALERT:
      /* ltc4282 eeprom not support word write */
      word = value & 0xFFFF;
      if(wait_eeprom_done(fd) == -1)
        return -1;
      byte = (word & 0xFF00) >> 8;
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[reg].reg, byte);
      if(wait_eeprom_done(fd) == -1)
        return -1;
      byte = word & 0xFF;
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[reg].reg + 1, byte);
      break;

    case FAULT_LOG:
    case ADC_ALERT_LOG:
    case FET_BAD_FAULT_TIME:
    case GPIO_CONGIG:
    case VGPIO_ALARM_MIN:
    case VGPIO_ALARM_MAX:
    case VSOURCE_ALARM_MIN:
    case VSOURCE_ALARM_MAX:
    case VSENSE_ALARM_MIN:
    case VSENSE_ALARM_MAX:
    case POWER_ALARM_MIN:
    case POWER_ALARM_MAX:
    case CLOCK_DIVIDER:
    case ILIM_ADJUST:
    case ALERT_CONTROL:
    case ADC_CONTROL:
      byte = value & 0xFF;
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[reg].reg, byte);
      break;
    case EE_FAULT:
    case EE_ADC_ALERT_LOG:
    case EE_FET_BAD_FAULT_TIME:
    case EE_GPIO_CONFIG:
    case EE_VGPIO_ALARM_MIN:
    case EE_VGPIO_ALARM_MAX:
    case EE_VSOURCE_ALARM_MIN:
    case EE_VSOURCE_ALARM_MAX:
    case EE_VSENSE_ALARM_MIN:
    case EE_VSENSE_ALARM_MAX:
    case EE_POWER_ALARM_MIN:
    case EE_POWER_ALARM_MAX:
    case EE_CLOCK_DECIMATOR:
    case EE_ILIM_ADJUST:
      if(wait_eeprom_done(fd) == -1)
        return -1;
      byte = value & 0xFF;
      byte = i2c_smbus_write_byte_data(fd, smbus_ltc4282[reg].reg, byte);
      break;
    default:
      break;
  }

  return 0;
}

/* 
 * ltc4282 Vin/Vout 16 bits adc convert to Volts 
 * refer to ltc4282 datasheet page 23
 */
static inline float ltc4282_vsourve_calculate(int reg_val) {
  return (reg_val * 16.64 / 65535);
}

/* 
 * ltc4282 Current 16 bits adc convert to Amps 
 * refer to ltc4282 datasheet page 23
 * Rsense = 0.0001875 ohm
 */
static inline float ltc4282_vsense_calculate(int reg_val) {
  return (reg_val * 0.04 / 65535 / 0.0001875);
}

/* 
 * ltc4282 Power 16 bits adc convert to Watts 
 * refer to ltc4282 datasheet page 23
 * Rsense = 0.0001875 ohm
 */
static inline float ltc4282_power_calculate(int reg_val) {
  return (reg_val * 16.64 * 0.04 / 65535 / 0.0001875);
}

/* 
 * ltc4282 Hot Swap Temp 16 bits adc convert to degree C 
 * linear correction
 */
static inline float ltc4282_temp_calculate(int reg_val) {
  float temp = ((reg_val * 14412.0 - 15000000) / 10000) / 1000;
  return temp > 0 ? temp : 0;
}

/*
 * From datesheet reg addr 0x1E & 0x1F, 
 * the temps LSBs(decimal part) are three bits.
 * So the unit is 1/8C, need to multiply by 125 = 1/8*1000.
 */
static inline float max6615_temp_calculate(int reg_val) {
  return reg_val * 0.125;
}

/*
 * formula come from recalibration
 */
static inline float max6615_fan_calculate(int reg_val) {
  return ((reg_val == 0xff || reg_val == 0) ? 0 : 57600.0 / reg_val);
}

/*
 * refer to MAX6615 datasheet page 13th
 */
static inline float max6615_pwm_calculate(int reg_val) {
  return reg_val / 240.0 * 100;
}

int is_pem_prsnt(uint8_t num, uint8_t *status) {
  return pal_is_fru_prsnt(num + FRU_PEM1, status);
}

static void mkdir_recurse(const char *dir, mode_t mode)
{
  if (access(dir, F_OK) == -1) {
    char curr[MAX_KEY_PATH_LEN];
    char parent[MAX_KEY_PATH_LEN];
    strcpy(curr, dir);
    strcpy(parent, dirname(curr));
    mkdir_recurse(parent, mode);
    mkdir(dir, mode);
  }
}

/*
 *  file_write
 *  value is pointer of write buffer
 *  len is the size of value. If 0, it is assumed value is
 *      a string and strlen() is used to determine the length.
 *  flags is bitmask of options.
 *
 *  return 0 on success, negative error code on failure.
 */
int file_write(const char *file, char *value, size_t len, unsigned int flags) {
  FILE *fp;
  int rc, ret = -1;
  bool present = true;
  char *curr_value;

  /* If the key already exists, and user wants to create it,
   * deny the access */
  if ((flags & KV_FCREATE) && access(file, F_OK) != -1) {
    return -1;
  }

  /* Optimizes a lot of things */
  if (len == 0) {
    len = strlen(value);
  }

  curr_value = malloc(len);
  if(curr_value == NULL) {
    return -1;
  }
  memset(curr_value, 0, len);

  fp = fopen(file, "rb+");
  if (!fp && (errno == ENOENT)) {
    fp = fopen(file, "wb");
    present = false;
  }
  if (!fp) {
    ret = -1;
    goto err_exit;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
    goto close_bail;
  }

  // Check if we are writing the same value. If so, exit early
  // to save on number of times flash is updated.
  if (present && (flags && KV_FPERSIST)) {
    rc = (int)fread(curr_value, 1, PEM_ARCHIVE_BUFF, fp);
    if (len == rc && !memcmp(value, curr_value, len)) {
      ret = 0;
      goto unlock_bail;
    }
    rc = fseek(fp, 0, SEEK_SET);
    if (rc < 0) {
      goto close_bail;
    }
  }

  if (ftruncate(fileno(fp), 0) < 0) {  //truncate cache file after getting flock
    goto unlock_bail;
  }

  rc = fwrite(value, 1, len, fp);
  if (rc < 0) {
    goto unlock_bail;
  }
  rc= fflush(fp);
  if (rc != 0) {
    goto unlock_bail;
  }

  if (rc != len) {
    goto unlock_bail;
  }
  ret = 0;
unlock_bail:
  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
    ret = -1;
  }
close_bail:
  fclose(fp);
err_exit:
  free(curr_value);
  return ret;
}

/*
 *  file_read
 *  value is pointer of read buffer
 *  len is the return size of value. If NULL then this information
 *      is not returned to the user (And assumed to be a string).
 *  flags is bitmask of options.
 *
 *  return 0 on success, negative error code on failure.
 */
int file_read(const char *file, char *value, size_t *len) {
  FILE *fp;
  int rc, ret=-1;

  fp = fopen(file, "rb");
  if (!fp) {
    return -1;
  }

  rc = flock(fileno(fp), LOCK_EX);
  if (rc < 0) {
    goto close_bail;
  }

  rc = (int) fread(value, 1, PEM_ARCHIVE_BUFF, fp);
  if (rc < 0 || ferror(fp)) {
    goto unlock_bail;
  }
  if (len)
    *len = rc;
  ret = 0;
unlock_bail:
  rc = flock(fileno(fp), LOCK_UN);
  if (rc < 0) {
    ret = -1;
  }
close_bail:
  fclose(fp);
  return ret;
}

/*
 * archive ltc4282 & max6615 registers to /mnt/data/pem folder
 */
int archive_pem_chips(uint8_t num) {
  int ret = 0;
  char *chip_regs;
  char path[MAX_KEY_PATH_LEN];

  for(int chip = LTC4282; chip <= MAX6615; chip++) {
    memset(path, 0, MAX_KEY_PATH_LEN);
    strcpy(path, pem[num].file_path[chip]);
    mkdir_recurse(dirname(path), 0777);

    pem[num].fd = i2c_cdev_slave_open(pem[num].bus, pem[num].chip_addr[chip],
                                      I2C_SLAVE_FORCE_CLAIM);
    if (pem[num].fd < 0) {
      ERR_PRINT("Fail to open i2c");
      ret |= (1 << chip);
      continue;
    }

    chip_regs = malloc(pem[num].chip_reg_cnt[chip]);
    if(chip_regs == NULL) {
      close(pem[num].fd);
      ret |= (1 << chip);
      continue;
    }

    for(int reg = 0; reg < pem[num].chip_reg_cnt[chip]; reg++) {
      *(chip_regs + reg) = i2c_smbus_read_byte_data(pem[num].fd, reg);
    }

    file_write(pem[num].file_path[chip], chip_regs, pem[num].chip_reg_cnt[chip], KV_FPERSIST);
    free(chip_regs);
  }

  return ret;
}

static void print_pem_status_regs(pem_status_regs_t *status_regs);
static int print_ltc4282_crit_sensors(char *chip_regs) {
  int ret = 0;
  int reg_val;
  int threshold_sensors[] = { VIN, VOUT, VSENSE, POWER, VGPIO };
  pem_status_regs_t discrete_sensors;
  pem_status_regs_t discrete_sensors_ee;

  if(chip_regs == NULL) {
    return -1;
  }

  for (int sensor = 0; sensor < sizeof(threshold_sensors) / sizeof(int); sensor++) {
    int reg = threshold_sensors[sensor];
    reg_val = 0;
    reg_val = (*(chip_regs + smbus_ltc4282[reg].reg)) << 8 | 
              (*(chip_regs + smbus_ltc4282[reg].reg + 1));
    switch(reg) {
      case VIN:
        printf("%-32s: %.2f Volts\n", "IN_VOLT", ltc4282_vsourve_calculate(reg_val));
        break;
      case VOUT:
        printf("%-32s: %.2f Volts\n", "OUT_VOLT", ltc4282_vsourve_calculate(reg_val));
        break;
      case VSENSE:
        printf("%-32s: %.2f Amps\n", "CURR", ltc4282_vsense_calculate(reg_val));
        break;
      case POWER:
        printf("%-32s: %.2f Watts\n", "POWER", ltc4282_power_calculate(reg_val));
        break;
      case VGPIO:
        printf("%-32s: %.2f C\n", "HOT_SWAP_TEMP", ltc4282_temp_calculate(reg_val));
        break;
    }
  }

  discrete_sensors.fault.reg_val.value = *(chip_regs + smbus_ltc4282[FAULT_LOG].reg);
  discrete_sensors.adc_alert_log.reg_val.value = *(chip_regs + smbus_ltc4282[ADC_ALERT_LOG].reg);
  discrete_sensors.status.reg_val.value = (*(chip_regs + smbus_ltc4282[STATUS].reg)) << 8 | 
                                          (*(chip_regs + smbus_ltc4282[STATUS].reg + 1));
  printf("\nDecode ltc4282 STATUS/FAULT/ADC_ALERT_LOG:");
  print_pem_status_regs(&discrete_sensors);

  discrete_sensors_ee.fault.reg_val.value = *(chip_regs + smbus_ltc4282[FAULT_LOG].reg);
  discrete_sensors_ee.adc_alert_log.reg_val.value = *(chip_regs + smbus_ltc4282[ADC_ALERT_LOG].reg);
  discrete_sensors_ee.status.reg_val.value = (*(chip_regs + smbus_ltc4282[STATUS].reg)) << 8 | 
                                             (*(chip_regs + smbus_ltc4282[STATUS].reg + 1));
  printf("\nDecode ltc4282 EE_STATUS/EE_FAULT_LOG/EE_ADC_ALERT_LOG:");
  print_pem_status_regs(&discrete_sensors_ee);

  return ret;
}

static int print_max6615_crit_sensors(char *chip_regs) {
  int ret = 0;
  int temp;
  int fan_cnt;
  int pwm_target;
  int pwm_output;

  if(chip_regs == NULL)
    return -1;

  temp = *(chip_regs + MAX6615_REG_TEMP(0)) << 3 | (*(chip_regs + MAX6615_REG_TEMP_EXT(0)) >> 5);
  fan_cnt = *(chip_regs + MAX6615_REG_FAN_CNT(0));
  pwm_target = *(chip_regs + MAX6615_REG_TARGT_PWM(0));
  pwm_output = *(chip_regs + MAX6615_REG_INSTANTANEOUS_PWM(0));
  printf("%-32s: %.2f C\n", "AIR_INLET_TEMP", max6615_temp_calculate(temp));
  printf("%-32s: %.2f RPM\n", "FAN1_SPEED", max6615_fan_calculate(fan_cnt));
  printf("%-32s: %.2f %%\n", "FAN1_TARGET_PWM", max6615_pwm_calculate(pwm_target));
  printf("%-32s: %.2f %%\n", "FAN1_PWM_OUTPUT", max6615_pwm_calculate(pwm_output));

  temp = *(chip_regs + MAX6615_REG_TEMP(1)) << 3 | (*(chip_regs + MAX6615_REG_TEMP_EXT(1)) >> 5);
  fan_cnt = *(chip_regs + MAX6615_REG_FAN_CNT(1));
  pwm_target = *(chip_regs + MAX6615_REG_TARGT_PWM(1));
  pwm_output = *(chip_regs + MAX6615_REG_INSTANTANEOUS_PWM(1));
  printf("%-32s: %.2f C\n", "AIR_OUTLET_TEMP", max6615_temp_calculate(temp));
  printf("%-32s: %.2f RPM\n", "FAN2_SPEED", max6615_fan_calculate(fan_cnt));
  printf("%-32s: %.2f %%\n", "FAN2_TARGET_PWM", max6615_pwm_calculate(pwm_target));
  printf("%-32s: %.2f %%\n", "FAN2_PWM_OUTPUT", max6615_pwm_calculate(pwm_output));

  return ret;
}

static int decode_pem_backup_log(uint8_t num) {
  int ret = 0;
  char *chip_regs;

  for(int chip = LTC4282; chip <= MAX6615; chip++) {
    chip_regs = malloc(pem[num].chip_reg_cnt[chip]);
    if(chip_regs == NULL) {
      ret |= (1 << chip);
      continue;
    }

    size_t len = 0;
    if(file_read(pem[num].file_path[chip], chip_regs, &len)) {
      printf("Failed to read %s\n", pem[num].file_path[chip]);
      free(chip_regs);
      ret |= (1 << chip);
      continue;
    }

    printf("\nDecode %s: \n", pem[num].file_path[chip]);
    switch(chip) {
      case LTC4282:
        print_ltc4282_crit_sensors(chip_regs);
        break;
      case MAX6615:
        print_max6615_crit_sensors(chip_regs);
        break;
    }

    free(chip_regs);
  }

  return ret;
}

/*
 * get pem sensors
 * refer to pal.c
 */
static void print_pem_sensors(uint8_t num) {
  for(int i = PEM1_SENSOR_IN_VOLT; i <= PEM1_SENSOR_TEMP3; i++) {
    float value = 0;
    int sensor_num = num * PEM1_SENSOR_STATUS_METER_OVERFLOW + i;
    char sensor_name[64] = {0};
    char sensor_units[8] = {0};

    pal_get_sensor_name(num + FRU_PEM1, sensor_num, sensor_name);
    pal_get_sensor_units(num + FRU_PEM1, sensor_num, sensor_units);
    if (!pal_sensor_read_raw(num + FRU_PEM1, sensor_num, &value)) {
      printf("\n%-32s: %0.2f %s", sensor_name, value, sensor_units);
    } else {
      printf("\n%-32s: %s", sensor_name, "NA");
    }
  }
  printf("\n");
}

static void print_pem_control_reg(pem_control_t *control) {
  const char *mode[][4] = {
    {"external", "5%", "10%", "15%"},
    {"3.3V", "5V", "12V", "24V"}
  };

  printf("\n%-32s: %d", "On Fault Mask", control->reg_val.values.on_fault_mask);
  printf("\n%-32s: %d", "On Delay", control->reg_val.values.on_delay);
  printf("\n%-32s: %d", "On/Enb", control->reg_val.values.on_enb);
  printf("\n%-32s: %d", "Mass Write Enable", control->reg_val.values.mass_write_enable);
  printf("\n%-32s: %d", "Fet on", control->reg_val.values.fet_on);
  printf("\n%-32s: %d", "OC Autoretry", control->reg_val.values.oc_autoretry);
  printf("\n%-32s: %d", "UV Autoretry", control->reg_val.values.uv_autoretry);
  printf("\n%-32s: %d", "OV Autoretry", control->reg_val.values.ov_autoretry);
  printf("\n%-32s: %s", "On FB Mode", mode[0][control->reg_val.values.fb_mode]);
  printf("\n%-32s: %s", "On UV Mode", mode[0][control->reg_val.values.uv_mode]);
  printf("\n%-32s: %s", "On OV Mode", mode[0][control->reg_val.values.ov_mode]);
  printf("\n%-32s: %s", "On Vin Mode", mode[1][control->reg_val.values.vin_mode]);
  printf("\n");
}

static void print_pem_alert_reg(pem_alert_t *alert) {
  printf("\n%-32s: %d", "EEPROM Done Alert", alert->reg_val.values.eeprom_done_alert);
  printf("\n%-32s: %d", "FET Bad Fault Alert", alert->reg_val.values.fet_bad_fault_alert);
  printf("\n%-32s: %d", "FET Short Alert", alert->reg_val.values.fet_short_alert);
  printf("\n%-32s: %d", "On Alert", alert->reg_val.values.on_alert);
  printf("\n%-32s: %d", "PB Alert", alert->reg_val.values.pb_alert);
  printf("\n%-32s: %d", "OC Alert", alert->reg_val.values.oc_alert);
  printf("\n%-32s: %d", "UV Alert", alert->reg_val.values.uv_alert);
  printf("\n%-32s: %d", "OV Alert", alert->reg_val.values.ov_alert);
  printf("\n%-32s: %d", "Power Alarm High", alert->reg_val.values.power_alarm_high);
  printf("\n%-32s: %d", "Power Alarm Low", alert->reg_val.values.power_alarm_low);
  printf("\n%-32s: %d", "Vsense Alarm High", alert->reg_val.values.vsense_alarm_high);
  printf("\n%-32s: %d", "Vsense Alarm Low", alert->reg_val.values.vsense_alarm_low);
  printf("\n%-32s: %d", "VSourve Alarm High", alert->reg_val.values.vsource_alarm_high);
  printf("\n%-32s: %d", "VSourve Alarm Low", alert->reg_val.values.vsource_alarm_low);
  printf("\n%-32s: %d", "VGPIO Alarm High", alert->reg_val.values.vgpio_alarm_high);
  printf("\n%-32s: %d", "VGPIO Alarm Low", alert->reg_val.values.vgpio_alarm_low);
  printf("\n");
}

static void print_pem_fault_reg(pem_fault_t *fault) {
  printf("\n%-32s: %d", "EEPROM_Done", fault->reg_val.values.eeprom_done);
  printf("\n%-32s: %d", "FET_Bad_Fault", fault->reg_val.values.fet_bad_fault);
  printf("\n%-32s: %d", "FET_Short_Fault", fault->reg_val.values.fet_short_fault);
  printf("\n%-32s: %d", "On_Fault", fault->reg_val.values.on_fault);
  printf("\n%-32s: %d", "Power_Bad_Fault", fault->reg_val.values.power_bad_fault);
  printf("\n%-32s: %d", "OC_Fault", fault->reg_val.values.oc_fault);
  printf("\n%-32s: %d", "UV_Fault", fault->reg_val.values.uv_fault);
  printf("\n%-32s: %d", "OV_Fault", fault->reg_val.values.ov_fault);
  printf("\n");
}

static void print_pem_adc_alert_reg(pem_adc_alert_t *adc_alert_log) {
  printf("\n%-32s: %d", "Power_Alarm_High", adc_alert_log->reg_val.values.power_alarm_high);
  printf("\n%-32s: %d", "Power_Alarm_Low", adc_alert_log->reg_val.values.power_alarm_low);
  printf("\n%-32s: %d", "Vsense_Alarm_High", adc_alert_log->reg_val.values.vsense_alarm_high);
  printf("\n%-32s: %d", "Vsense_Alarm_Low", adc_alert_log->reg_val.values.vsense_alarm_low);
  printf("\n%-32s: %d", "VSourve_Alarm_High", adc_alert_log->reg_val.values.vsource_alarm_high);
  printf("\n%-32s: %d", "VSourve_Alarm_Low", adc_alert_log->reg_val.values.vsource_alarm_low);
  printf("\n%-32s: %d", "VGPIO_Alarm_High", adc_alert_log->reg_val.values.vgpio_alarm_high);
  printf("\n%-32s: %d", "VGPIO_Alarm_Low", adc_alert_log->reg_val.values.vgpio_alarm_low);
  printf("\n");
}

static void print_pem_gpio_config_reg(pem_gpio_alert_t *gpio_config) {
  const char *gpio1_config[] = {
    "Power Good", "Power Bad", "Output", "Input"
  };
  printf("\n%-32s: %d", "GPIO3 PD", gpio_config->reg_val.values.gpio3_pd);
  printf("\n%-32s: %d", "GPIO2 PD", gpio_config->reg_val.values.gpio2_pd);
  printf("\n%-32s: %s", "GPIO1 Config", gpio1_config[gpio_config->reg_val.values.gpio1_config]);
  printf("\n%-32s: %d", "GPIO1 Output", gpio_config->reg_val.values.gpio1_output);
  printf("\n%-32s: %d", "ADC Conv Alert", gpio_config->reg_val.values.adc_conv_alert);
  printf("\n%-32s: %d", "Stress to GPIO2", gpio_config->reg_val.values.stress_to_gpio2);
  printf("\n%-32s: %d", "Meter Overflow Alert", gpio_config->reg_val.values.meter_overflow_alert);
  printf("\n");
}

static void print_pem_clock_divider_reg(pem_clock_divider_t *clock_decimator) {
  printf("\n%-32s: %d", "Coulomb Meter", clock_decimator->reg_val.values.coulomb_meter);
  printf("\n%-32s: %d", "Tick Out", clock_decimator->reg_val.values.tick_out);
  printf("\n%-32s: %d", "Int Clock Out", clock_decimator->reg_val.values.int_clk_out);
  printf("\n%-32s: %d", "Clock Divider", clock_decimator->reg_val.values.clock_divider);
  printf("\n");
}

static void print_pem_ilim_adjust_reg(pem_ilim_adjust_t *ilim_adjust) {
  printf("\n%-32s: %d", "ILIM Adjust", ilim_adjust->reg_val.values.ilim_adjust);
  printf("\n%-32s: %d", "Foldback Mode", ilim_adjust->reg_val.values.foldback_mode);
  printf("\n%-32s: %d", "Vsource/VDD", ilim_adjust->reg_val.values.vource_vdd);
  printf("\n%-32s: %d", "GPIO Mode", ilim_adjust->reg_val.values.gpio_mode);
  printf("\n%-32s: %d-bit", "ADC 16-BIT/12-BIT", ilim_adjust->reg_val.values.adc_resolution ? 16 : 12);
  printf("\n");
}

static void print_pem_status_reg(pem_status_t *status) {
  printf("\n%-32s: %d", "ON_STATUS", status->reg_val.values.on_status);
  printf("\n%-32s: %d", "FET_BAD_COOLDOWN_STATUS", status->reg_val.values.fet_bad_cooldown_status);
  printf("\n%-32s: %d", "FET_SHORT_PRESENT", status->reg_val.values.fet_short_present);
  printf("\n%-32s: %d", "ON_PIN_STATUS", status->reg_val.values.on_pin_status);
  printf("\n%-32s: %d", "POWER_GOOD_STATUS", status->reg_val.values.power_good_status);
  printf("\n%-32s: %d", "OC_COOLDOWN_STATUS", status->reg_val.values.oc_cooldown_status);
  printf("\n%-32s: %d", "UV_STATUS", status->reg_val.values.uv_status);
  printf("\n%-32s: %d", "OV_STATUS", status->reg_val.values.ov_status);
  printf("\n%-32s: %d", "GPIO3_STATUS", status->reg_val.values.gpio3_status);
  printf("\n%-32s: %d", "GPIO2_STATUS", status->reg_val.values.gpio2_status);
  printf("\n%-32s: %d", "GPIO1_STATUS", status->reg_val.values.gpio1_status);
  printf("\n%-32s: %d", "ALERT_STATUS", status->reg_val.values.alert_status);
  printf("\n%-32s: %d", "EEPROM_BUSY", status->reg_val.values.eeprom_busy);
  printf("\n%-32s: %d", "ADC_IDLE", status->reg_val.values.adc_idle);
  printf("\n%-32s: %d", "TICKER_OVERFLOW_PRESENT", status->reg_val.values.ticker_overflow_present);
  printf("\n%-32s: %d", "METER_OVERFLOW_PRESENT", status->reg_val.values.meter_overflow_present);
  printf("\n");
}

static void print_pem_status_regs(pem_status_regs_t *status_regs) {
  if(!status_regs)
    return;
  print_pem_status_reg(&status_regs->status);
  print_pem_fault_reg(&status_regs->fault);
  print_pem_adc_alert_reg(&status_regs->adc_alert_log);
}

static void print_pem_eeprom_regs(pem_eeprom_reg_t *eeprom_reg) {
  if(!eeprom_reg)
    return;
  print_pem_control_reg(&eeprom_reg->control);
  print_pem_alert_reg(&eeprom_reg->alert);
  print_pem_fault_reg(&eeprom_reg->fault);
  print_pem_adc_alert_reg(&eeprom_reg->adc_alert_log);
  print_pem_gpio_config_reg(&eeprom_reg->gpio_config);
  print_pem_clock_divider_reg(&eeprom_reg->clock_decimator);
  print_pem_ilim_adjust_reg(&eeprom_reg->ilim_adjust);
}

static void print_pem_fru_eeprom(struct wedge_eeprom_st fruid) {
  printf("\n%-32s: %d", "Version", fruid.fbw_version);
  printf("\n%-32s: %s", "Product Name", fruid.fbw_product_name);
  printf("\n%-32s: %s", "Product Part Number", fruid.fbw_product_number);
  printf("\n%-32s: %s", "System Assembly Part Number", fruid.fbw_assembly_number);
  printf("\n%-32s: %s", "Facebook PCBA Part Number", fruid.fbw_facebook_pcba_number);
  printf("\n%-32s: %s", "Facebook PCB Part Number", fruid.fbw_facebook_pcb_number);
  printf("\n%-32s: %s", "ODM PCBA Part Number", fruid.fbw_odm_pcba_number);
  printf("\n%-32s: %s", "ODM PCBA Serial Number", fruid.fbw_odm_pcba_serial);
  printf("\n%-32s: %d", "Product Production State", fruid.fbw_production_state);
  printf("\n%-32s: %d", "Product Version", fruid.fbw_product_version);
  printf("\n%-32s: %d", "Product Sub-Version", fruid.fbw_product_subversion);
  printf("\n%-32s: %s", "Product Serial Number", fruid.fbw_product_serial);
  printf("\n%-32s: %s", "Product Asset Tag", fruid.fbw_product_asset);
  printf("\n%-32s: %s", "System Manufacturer", fruid.fbw_system_manufacturer);
  printf("\n%-32s: %s", "System Manufacturing Date",
         fruid.fbw_system_manufacturing_date);
  printf("\n%-32s: %s", "PCB Manufacturer", fruid.fbw_pcb_manufacturer);
  printf("\n%-32s: %s", "Assembled At", fruid.fbw_assembled);
  printf("\n%-32s: %02X:%02X:%02X:%02X:%02X:%02X", "Local MAC",
         fruid.fbw_local_mac[0], fruid.fbw_local_mac[1],
         fruid.fbw_local_mac[2], fruid.fbw_local_mac[3],
         fruid.fbw_local_mac[4], fruid.fbw_local_mac[5]);
  printf("\n%-32s: %02X:%02X:%02X:%02X:%02X:%02X", "Extended MAC Base",
         fruid.fbw_mac_base[0], fruid.fbw_mac_base[1],
         fruid.fbw_mac_base[2], fruid.fbw_mac_base[3],
         fruid.fbw_mac_base[4], fruid.fbw_mac_base[5]);
  printf("\n%-32s: %d", "Extended MAC Address Size", fruid.fbw_mac_size);
  printf("\n%-32s: %s", "Location on Fabric", fruid.fbw_location);
  printf("\n%-32s: 0x%x", "CRC8", fruid.fbw_crc8);
  printf("\n");
}

static int parse_pem_fru_eeprom(uint8_t num, struct wedge_eeprom_st *fruid) {
  int ret = -1;
  char eeprom[16];

  snprintf(eeprom, sizeof(eeprom), "%s", "24c02");
  pal_add_i2c_device(pem[num].bus, pem[num].chip_addr[EEPROM], eeprom);
  ret = wedge_eeprom_parse(pem[num].file_path[EEPROM], fruid);
  pal_del_i2c_device(pem[num].bus, pem[num].chip_addr[EEPROM]);

  if (ret) {
    snprintf(eeprom, sizeof(eeprom), "%s", "24c64");
    pal_add_i2c_device(pem[num].bus, pem[num].chip_addr[EEPROM], eeprom);
    ret = wedge_eeprom_parse(pem[num].file_path[EEPROM], fruid);
    pal_del_i2c_device(pem[num].bus, pem[num].chip_addr[EEPROM]);
    if (ret) {
      printf("Failed print EEPROM info!\n");
      return -1;
    }
  }
  return 0;
}

/*
 * dump ltc4282 FAULT_LOG/ADC_ALERT_LOG/STATUS regs
 */
static int pem_status_regs(uint8_t num, int option, pem_status_regs_t *status_regs) {
  int value = 0;

  if (option != READ)
    return -1;

  pem[num].fd = i2c_cdev_slave_open(pem[num].bus, pem[num].chip_addr[LTC4282],
                                    I2C_SLAVE_FORCE_CLAIM);
  if (pem[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return pem[num].fd;
  }

  pem_reg_read(pem[num].fd, FAULT_LOG, &value);
  status_regs->fault.reg_val.value = value;
  pem_reg_read(pem[num].fd, ADC_ALERT_LOG, &value);
  status_regs->adc_alert_log.reg_val.value = value;
  pem_reg_read(pem[num].fd, STATUS, &value);
  status_regs->status.reg_val.value = value;

  close(pem[num].fd);

  return 0;
}

static
int pem_firmware_regs(int fd, int option, pem_firmware_regs_t *firmware_regs) {
  int value = 0;

  if (option != READ)
    return -1;

  if (fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return fd;
  }

  pem_reg_read(fd, APP_FW_MAJOR, &value);
  firmware_regs->app_fw_major = value;
  pem_reg_read(fd, APP_FW_MINOR, &value);
  firmware_regs->app_fw_minor = value;
  pem_reg_read(fd, BL_FW_MAJOR, &value);
  firmware_regs->bl_fw_major = value;
  pem_reg_read(fd, BL_FW_MINOR, &value);
  firmware_regs->bl_fw_minor = value;

  return 0;
}

int log_pem_critical_regs(uint8_t num) {
  pem_status_regs_t status_regs;
  int ret = 0;

  ret = pem_status_regs(num, READ, &status_regs);

  if(ret) {
    OBMC_ERROR(ret, "Failed to read PEM %d FAULT_LOG/ADC_ALERT_LOG/STATUS registers", num + 1);
    return ret;
  }

  OBMC_CRIT("PEM %d [FAULT_LOG addr: 0x%02x, value: 0x%02x] " \
                   "[ADC_ALERT_LOG addr: 0x%02x, value: 0x%02x] " \
                   "[STATUS addr: 0x%02x, value: 0x%04x]", num + 1,
                    smbus_ltc4282[FAULT_LOG].reg, status_regs.fault.reg_val.value,
                    smbus_ltc4282[ADC_ALERT_LOG].reg, status_regs.adc_alert_log.reg_val.value,
                    smbus_ltc4282[STATUS].reg, status_regs.status.reg_val.value);

  return ret;
}

/*
 * dump ltc4282 eeprom regs
 */
static int pem_eeprom_regs(uint8_t num, int option, pem_eeprom_reg_t *eeprom_reg) {
  if (option != READ && option != WRITE)
    return -1;

  pem[num].fd = i2c_cdev_slave_open(pem[num].bus, pem[num].chip_addr[LTC4282],
                                    I2C_SLAVE_FORCE_CLAIM);
  if (pem[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  for(int reg = EE_CONTROL; reg <= EE_ILIM_ADJUST; reg++) {
    int value = 0;
    if (option == READ) {
      pem_reg_read(pem[num].fd, reg, &value);
      switch(reg) {
        case EE_CONTROL:
          eeprom_reg->control.reg_val.value = value;
          break;
        case EE_ALERT:
          eeprom_reg->alert.reg_val.value = value;
          break;
        case EE_FAULT:
          eeprom_reg->fault.reg_val.value = value;
          break;
        case EE_ADC_ALERT_LOG:
          eeprom_reg->adc_alert_log.reg_val.value = value;
          break;
        case EE_FET_BAD_FAULT_TIME:
          eeprom_reg->fet_bad_fault_time = value;
          break;
        case EE_GPIO_CONFIG:
          eeprom_reg->gpio_config.reg_val.value = value;
          break;
        case EE_VGPIO_ALARM_MIN:
          eeprom_reg->vgpio_alarm_min = value;
          break;
        case EE_VGPIO_ALARM_MAX:
          eeprom_reg->vgpio_alarm_max = value;
          break;
        case EE_VSOURCE_ALARM_MIN:
          eeprom_reg->vsource_alarm_min = value;
          break;
        case EE_VSOURCE_ALARM_MAX:
          eeprom_reg->vsource_alarm_max = value;
          break;
        case EE_VSENSE_ALARM_MIN:
          eeprom_reg->vsense_alarm_min = value;
          break;
        case EE_VSENSE_ALARM_MAX:
          eeprom_reg->vsense_alarm_max = value;
          break;
        case EE_POWER_ALARM_MIN:
          eeprom_reg->power_alarm_min = value;
          break;
        case EE_POWER_ALARM_MAX:
          eeprom_reg->power_alarm_max = value;
          break;
        case EE_CLOCK_DECIMATOR:
          eeprom_reg->clock_decimator.reg_val.value = value;
          break;
        case EE_ILIM_ADJUST:
          eeprom_reg->ilim_adjust.reg_val.value = value;
          break;
        default:
          continue;
      }
    } else if (option == WRITE) {
      switch(reg) {
        case EE_CONTROL:
          value = eeprom_reg->control.reg_val.value;
          break;
        case EE_ALERT:
          value = eeprom_reg->alert.reg_val.value;
          break;
        case EE_FAULT:
          value = eeprom_reg->fault.reg_val.value;
          break;
        case EE_ADC_ALERT_LOG:
          value = eeprom_reg->adc_alert_log.reg_val.value;
          break;
        case EE_FET_BAD_FAULT_TIME:
          value = eeprom_reg->fet_bad_fault_time;
          break;
        case EE_GPIO_CONFIG:
          value = eeprom_reg->gpio_config.reg_val.value;
          break;
        case EE_VGPIO_ALARM_MIN:
          value = eeprom_reg->vgpio_alarm_min;
          break;
        case EE_VGPIO_ALARM_MAX:
          value = eeprom_reg->vgpio_alarm_max;
          break;
        case EE_VSOURCE_ALARM_MIN:
          value = eeprom_reg->vsource_alarm_min;
          break;
        case EE_VSOURCE_ALARM_MAX:
          value = eeprom_reg->vsource_alarm_max;
          break;
        case EE_VSENSE_ALARM_MIN:
          value = eeprom_reg->vsense_alarm_min;
          break;
        case EE_VSENSE_ALARM_MAX:
          value = eeprom_reg->vsense_alarm_max;
          break;
        case EE_POWER_ALARM_MIN:
          value = eeprom_reg->power_alarm_min;
          break;
        case EE_POWER_ALARM_MAX:
          value = eeprom_reg->power_alarm_max;
          break;
        case EE_CLOCK_DECIMATOR:
          value = eeprom_reg->clock_decimator.reg_val.value;
          break;
        case EE_ILIM_ADJUST:
          value = eeprom_reg->ilim_adjust.reg_val.value;
          break;
        default:
          continue;
      }
      pem_reg_write(pem[num].fd, reg, value);
    }
  }

  close(pem[num].fd);

  return 0;
}

/* 
 * Populate and print fruid_info by parsing the fru's binary dump and
 * populate and print ltc4282 eeprom regs
 */
int get_eeprom_info(uint8_t num, const char *option) {
  struct wedge_eeprom_st fruid;
  pem_eeprom_reg_t eeprom_reg;

  if ((!strncmp(option, "--print", strlen("--print")))) {
    printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "FRU Information",
           num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
    printf("\n%-32s: %s", "---------------", "-----------------------");
    if(!parse_pem_fru_eeprom(num, &fruid)) {
      print_pem_fru_eeprom(fruid);
    }

    printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "Hot Swap EEPROM Information",
           num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
    printf("\n%-32s: %s", "---------------", "-----------------------");
    pem_eeprom_regs(num, READ, &eeprom_reg);
    print_pem_eeprom_regs(&eeprom_reg);
  } else if ((!strncmp(option, "--clear", strlen("--clear")))) {
    printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "Hot Swap EEPROM Default Information",
           num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
    printf("\n%-32s: %s", "---------------", "-----------------------");
    print_pem_eeprom_regs(&pem_default_config);
    pem_eeprom_regs(num, WRITE, &pem_default_config);
  } else {
    printf("Invalid option!\n");
    return -1;
  }

  return 0;
}

/* 
 * Show pem current status & some basic info
 */
int get_pem_info(uint8_t num) {
  struct wedge_eeprom_st fruid;
  pem_status_regs_t status_regs;
  pem_firmware_regs_t firmware_regs = {0};
  int ret;
  uint8_t pem_model[I2C_SMBUS_BLOCK_MAX + 1] = {0};

  printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "PEM Information",
         num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
  printf("\n%-32s: %s", "---------------", "-----------------------");
  if(!parse_pem_fru_eeprom(num, &fruid)) {
    printf("\n%-32s: %d", "Version", fruid.fbw_version);
    printf("\n%-32s: %s", "Product Name", fruid.fbw_product_name);
    printf("\n%-32s: %s", "Product Part Number", fruid.fbw_product_number);
    printf("\n%-32s: %d", "Product Version", fruid.fbw_product_version);
    printf("\n%-32s: %d", "Product Sub-Version", fruid.fbw_product_subversion);
    printf("\n%-32s: %s", "Product Serial Number", fruid.fbw_product_serial);
    printf("\n%-32s: %s", "System Manufacturer", fruid.fbw_system_manufacturer);
    printf("\n%-32s: %s", "System Manufacturing Date",
           fruid.fbw_system_manufacturing_date);
    printf("\n");
  }

  ret = get_mfr_model(num, pem_model);
  if (ret == 0) {
    if (!strncmp((const char *)pem_model, DELTA_MODEL, strlen(DELTA_MODEL))) {
      if (pem[num].fd < 0) {
        pem[num].fd = i2c_cdev_slave_open(pem[num].bus,
                                          pem[num].chip_addr[LTC4282],
                                          I2C_SLAVE_FORCE_CLAIM);
      }
      ret = pem_firmware_regs(pem[num].fd, READ, &firmware_regs);
      if (ret == 0) {
        printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "PEM Firmware Info",
            num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
        printf("\n%-32s: %s", "---------------", "-----------------------");
        printf("\n%-32s: %d.%d", "Application Version", firmware_regs.app_fw_major,
              firmware_regs.app_fw_minor);
        printf("\n%-32s: %d.%d", "Bootloader Version", firmware_regs.bl_fw_major,
              firmware_regs.bl_fw_minor);
        printf("\n");
      }
    }
  }

  printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "PEM Hot Swap status",
         num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
  printf("\n%-32s: %s", "---------------", "-----------------------");
  print_pem_sensors(num);
  pem_status_regs(num, READ, &status_regs);
  print_pem_status_regs(&status_regs);

  return 0;
}

/* 
 * Show pem's info & status fully
 */
int get_blackbox_info(uint8_t num, const char *option) {
  struct wedge_eeprom_st fruid;
  pem_eeprom_reg_t eeprom_reg;
  pem_status_regs_t status_regs;

  pem[num].fd = i2c_cdev_slave_open(pem[num].bus, pem[num].chip_addr[LTC4282],
                                    I2C_SLAVE_FORCE_CLAIM);
  if (pem[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if ((!strncmp(option, "--print", strlen("--print")))) {
    printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "FRU Information",
           num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
    printf("\n%-32s: %s", "---------------", "-----------------------");
    if(!parse_pem_fru_eeprom(num, &fruid)) {
      print_pem_fru_eeprom(fruid);
    }

    printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "Hot Swap EEPROM Information",
           num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
    printf("\n%-32s: %s", "---------------", "-----------------------");
    pem_eeprom_regs(num, READ, &eeprom_reg);
    print_pem_eeprom_regs(&eeprom_reg);

    printf("\n%-32s: PEM%d (Bus:%d Addr:0x%x)", "PEM Hot Swap status",
           num + 1, pem[num].bus, pem[num].chip_addr[LTC4282]);
    printf("\n%-32s: %s", "---------------", "-----------------------");
    print_pem_sensors(num);
    pem_status_regs(num, READ, &status_regs);
    print_pem_status_regs(&status_regs);
    printf("\n");
  } else if ((!strncmp(option, "--clear", strlen("--clear")))) {
    printf("Not support!\n");
  } else {
    printf("Invalid option!\n");
    close(pem[num].fd);
    return -1;
  }
  close(pem[num].fd);

  return 0;
}

int get_archive_log(uint8_t num, const char *option) {

  if ((!strncmp(option, "--print", strlen("--print")))) {
    return decode_pem_backup_log(num);
  } else if ((!strncmp(option, "--clear", strlen("--clear")))) {
    printf("Not support!\n");
    return 0;
  } else {
    printf("Invalid option!\n");
    close(pem[num].fd);
    return -1;
  }

}

static int
delta_check_fw_version(uint8_t num) {
  pem_firmware_regs_t firmware_regs = {0};
  int ret;
  uint16_t app_fw_ver, bl_fw_ver;
  uint16_t eep_app_fw_ver, eep_bl_fw_ver;

  ret = pem_firmware_regs(pem[num].fd, READ, &firmware_regs);
  if (ret != 0) {
    ERR_PRINT("delta_check_fw_version()");
    return -1;
  }

  app_fw_ver = (delta_hdr.app_fw_major << 8) | delta_hdr.app_fw_minor;
  bl_fw_ver = (delta_hdr.bl_fw_major << 8) | delta_hdr.bl_fw_minor;

  eep_app_fw_ver = (firmware_regs.app_fw_major << 8) | firmware_regs.app_fw_minor;
  eep_bl_fw_ver = (firmware_regs.bl_fw_major << 8) | firmware_regs.bl_fw_minor;
  if (app_fw_ver == eep_app_fw_ver && bl_fw_ver == eep_bl_fw_ver) {
    /* FW is identical */
    return FW_IDENTICAL;
  }

  return 0;

}

static int
check_file_len(const char *file_path) {
  int size;
  struct stat st;

  if (stat(file_path, &st) != 0) {
    ERR_PRINT("check_file_len()");
    return -1;
  }

  size = st.st_size;

  return size;
}

static int
delta_img_hdr_parse(const char *file_path) {
  int ret;
  int i;
  int fd_file = -1;
  int index = 0;
  uint8_t hdr_buf[DELTA_HDR_LENGTH];

  fd_file = open(file_path, O_RDONLY, 0666);
  if (fd_file < 0) {
    ERR_PRINT("delta_img_hdr_parse(): open()");
    return -1;
  }
  ret = read(fd_file, hdr_buf, DELTA_HDR_LENGTH);
  if (ret != DELTA_HDR_LENGTH) {
      ERR_PRINT("delta_img_hdr_parse(): read()");
      if(close(fd_file)) {
        ERR_PRINT("delta_img_hdr_parse(): close()");
      }
      return -1;
  }
  if(close(fd_file)) {
    ERR_PRINT("delta_img_hdr_parse(): close()");
    return -1;
  }

  delta_hdr.crc[0] = hdr_buf[index++];
  delta_hdr.crc[1] = hdr_buf[index++];
  delta_hdr.page_start = hdr_buf[index++];
  delta_hdr.page_start = ( delta_hdr.page_start << 8 ) | hdr_buf[index++];
  delta_hdr.page_end = hdr_buf[index++];
  delta_hdr.page_end = ( delta_hdr.page_end << 8 ) | hdr_buf[index++];
  delta_hdr.byte_per_blk = hdr_buf[index++];
  delta_hdr.byte_per_blk =( delta_hdr.byte_per_blk << 8 ) | hdr_buf[index++];
  delta_hdr.blk_per_page = hdr_buf[index++];
  delta_hdr.blk_per_page = ( delta_hdr.blk_per_page << 8 ) | hdr_buf[index++];
  delta_hdr.uc = hdr_buf[index++];
  delta_hdr.app_fw_major = hdr_buf[index++];
  delta_hdr.app_fw_minor = hdr_buf[index++];
  delta_hdr.bl_fw_major = hdr_buf[index++];
  delta_hdr.bl_fw_minor = hdr_buf[index++];
  delta_hdr.fw_id_len = hdr_buf[index++];

  for (i = 0; i < delta_hdr.fw_id_len; i++) {
    delta_hdr.fw_id[i] = hdr_buf[index++];
  }
  delta_hdr.compatibility = hdr_buf[index];

  if (!strncmp((const char *)delta_hdr.fw_id, DELTA_MODEL, strlen(DELTA_MODEL))) {
    printf("Vendor: Delta\n");
    printf("Model: %s\n", delta_hdr.fw_id);
    printf("HW Compatibility: %d\n", delta_hdr.compatibility);
    if (delta_hdr.uc == 0x10) {
      printf("MCU: primary\n");
    } else if (delta_hdr.uc == 0x20) {
      printf("MCU: secondary\n");
    } else {
      printf("MCU: unknown number 0x%x\n", delta_hdr.uc);
      return -1;
    }
    printf("Ver: %d.%d\n", delta_hdr.app_fw_major, delta_hdr.app_fw_minor);
    return DELTA_PEM;
  } else {
    printf("Get Image Header Fail!\n");
    return -1;
  }
}

static int
delta_unlock_upgrade(uint8_t num) {
  uint8_t block[I2C_SMBUS_BLOCK_MAX] = {delta_hdr.uc, 
                                        delta_hdr.fw_id[10], delta_hdr.fw_id[9],
                                        delta_hdr.fw_id[8], delta_hdr.fw_id[7],
                                        delta_hdr.fw_id[6], delta_hdr.fw_id[5],
                                        delta_hdr.fw_id[4], delta_hdr.fw_id[3],
                                        delta_hdr.fw_id[2], delta_hdr.fw_id[1],
                                        delta_hdr.fw_id[0], delta_hdr.compatibility};

  printf("-- Unlock Upgrade --\n");
  int ret = i2c_smbus_write_block_data(pem[num].fd, UNLOCK_UPGRADE, 13, block);
  if (ret < 0) {
    printf("Unlock upgrade failure\n");
    return -1;
  }
  else
    return 0;
}

static int
delta_boot_flag(uint8_t num, uint16_t mode) {
  int ret;
  uint16_t word = (mode << 8) | delta_hdr.uc;

  printf("-- %s --\n", mode == BOOT_MODE ? "Bootloader Mode" : "Reset PEM");
  ret = i2c_smbus_write_word_data(pem[num].fd, BOOT_FLAG, word);
  if (ret < 0) {
    printf("Failed to %s\n", mode == BOOT_MODE ? "set to bootloader mode" : "reset PEM");
    return -1;
  }
  else
    return 0;
}

static int
delta_fw_transmit(uint8_t num, const char *path) {
  int ret;
  FILE* fp;
  int fw_len = 0;
  int block_total = 0;
  int byte_index = 0;
  int fw_block = (delta_hdr.uc == 0x10)
                 ? DELTA_PRI_NUM_OF_BLOCK * DELTA_PRI_NUM_OF_PAGE
                 : DELTA_SEC_NUM_OF_BLOCK * DELTA_SEC_NUM_OF_PAGE;
  uint8_t page_num_lo = (delta_hdr.uc == 0x10) ? DELTA_PRI_PAGE_START
                                               : DELTA_SEC_PAGE_START;
  uint8_t block_size = (delta_hdr.uc == 0x10) ? DELTA_PRI_NUM_OF_BLOCK
                                              : DELTA_SEC_NUM_OF_BLOCK;
  uint8_t page_num_max = (delta_hdr.uc == 0x10) ? DELTA_PRI_PAGE_END
                                                : DELTA_SEC_PAGE_END;
  uint8_t block[I2C_SMBUS_BLOCK_MAX] = {0};
  uint8_t fw_buf[16];

  fw_len = check_file_len(path) - 32;

  uint8_t fw_data[fw_len];

  fp = fopen(path, "rb");
  if (fp == NULL) {
    ERR_PRINT("delta_fw_transmit(): fopen()");
    return -1;
  }
  ret = fseek(fp, 32, SEEK_SET);
  if (ret < 0) {
    ERR_PRINT("delta_fw_transmit(): fseek()");
    goto err_exit;
  }
  ret = fread(fw_data, sizeof(uint8_t), fw_len, fp);
  if (ret != fw_len) {
    ret = feof(fp);
    if (ret < 0) {
      ERR_PRINT("delta_fw_transmit(): fread()");
      goto err_exit;
    }
  }

  if (delta_hdr.uc == 0x10) {
    printf("-- Transmit Primary Firmware --\n");
  } else if (delta_hdr.uc == 0x20){
    printf("-- Transmit Secondary Firmware --\n");
  }

  /* Send date to PEM, use 16bytes mode */
  while (block_total <= fw_block) {
    block[0] = delta_hdr.uc;

    /* block[1] - Block Num LO
       block[2] - Block Num HI */
    if (block[1] < block_size) {
      memcpy(&fw_buf[0], &fw_data[byte_index], 16);
      memcpy(&block[3], &fw_buf, 16);
      ret = i2c_smbus_write_block_data(pem[num].fd, DATA_TO_RAM, 19, block);
      if (ret < 0) {
        printf("Send data to RAM failure\n");
        goto err_exit;
      }
      /*
       * The delay between two blocks of data.
       * It should be no less than 5ms, 10ms is recommended.
       */
      if (delta_hdr.uc == 0x10) {
        msleep(10);
      } else if (delta_hdr.uc == 0x20) {
        msleep(10);
      }

      block[1]++;
      block[2] = 0;
      block_total++;
      byte_index = byte_index + 16;
      printf("-- (%d/%d) (%d%%/100%%) --\r",
                  block_total, fw_block, (100 * block_total) / fw_block);
    } else {
      block[1] = page_num_lo;
      block[2] = 0;
      ret = i2c_smbus_write_block_data(pem[num].fd, DATA_TO_FLASH, 3, block);
      if (ret < 0) {
        printf("Send data to flash failure\n");
        goto err_exit;
      }
      /*
       * The delay between write data to flash command F3h and write CRC value command F4h.
       * It should be no less than 50ms, 90ms is recommended.
       */
      msleep(90);
      if (page_num_lo == page_num_max) {
        printf("\n");
        goto exit;
      } else {
        page_num_lo++;
        block[1] = 0;
      }
    }
  }

exit:
  ret = 0;

err_exit:
  ret = fclose(fp);
  if (ret < 0) {
    ERR_PRINT("delta_fw_transmit(): fclose()");
  }
  return ret;
}

static int
delta_crc_transmit(uint8_t num) {
  uint8_t block[I2C_SMBUS_BLOCK_MAX] =
                     {delta_hdr.uc, delta_hdr.crc[0], delta_hdr.crc[1]};

  printf("-- Transmit CRC --\n");
  int ret = i2c_smbus_write_block_data(pem[num].fd, CRC_CHECK, 3, block);
  if (ret < 0) {
    printf("CRC check failure\n");
    return -1;
  }

  return 0;
}

static int
update_delta_pem(uint8_t num, const char *file_path, _Bool force) {
  int ret = 0;
  if (delta_img_hdr_parse(file_path) == DELTA_PEM) {
    if (!force) {
      printf("-- Check Version --\n");
      ret = delta_check_fw_version(num);
      if (ret != 0) {
        return ret;
      }
    }
    ret = delta_unlock_upgrade(num);
    if (ret < 0)
      goto err_exit;
    /*
     * The delay between unlock command F0h and set boot flag command F1h.
     * It should be no less than 10ms, 20ms is recommended.
     */
    msleep(20);
    ret = delta_boot_flag(num, BOOT_MODE);
    if (ret < 0)
      goto err_exit;
    /*
     * The delay between unlock command F1h and send data command F2h.
     * It should be no less than 1500ms, 2500ms is recommended.
     */
    msleep(2500);
    ret = delta_fw_transmit(num, file_path);
    if (ret < 0)
      goto err_exit;
    ret = delta_crc_transmit(num);
    if (ret < 0)
      goto err_exit;
    /*
     * The delay between write CRC value command F4h and reset boot flag command F1h.
     * It should be no less than 800, 1500ms is recommended.
     */
    msleep(1500);
    ret = delta_boot_flag(num, NORMAL_MODE);

    /*
     * The delay from reset boot flag command to power supply back to normal working status,
     * which allowed to process the next controller.
     * It should be no less than 1000, 2000ms is recommended.
     */
    if (delta_hdr.uc == 0x10) {
      msleep(2000);
    } else if (delta_hdr.uc == 0x20) {
      msleep(2000);
    }
    printf("-- Upgrade Done --\n");
    return 0;
  }

err_exit:
  delta_boot_flag(num, NORMAL_MODE);
  return -1;
}

int
get_mfr_model(uint8_t num, uint8_t *block) {
  int rc = -1;
  struct wedge_eeprom_st fruid;

  rc = i2c_smbus_read_block_data(pem[num].fd, PMBUS_MFR_MODEL, block);
  if (rc < 0) {
    rc = parse_pem_fru_eeprom(num, &fruid);
    if (rc < 0) {
      ERR_PRINT("get_mfr_model()");
      return -1;
    }
    snprintf((char *)block, 33 , "%s", fruid.fbw_odm_pcba_number);
  }

  return 0;
}

int
do_update_pem(uint8_t num, const char *file_path, const char *vendor, _Bool force) {
  int ret = -1;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1] = {0};

  signal(SIGHUP, exithandler);
  signal(SIGINT, exithandler);
  signal(SIGTERM, exithandler);
  signal(SIGQUIT, exithandler);

  sensord_operation(num, STOP);
  pal_del_i2c_device(pem[num].bus, pem[num].chip_addr[LTC4282]);
  pal_del_i2c_device(pem[num].bus, pem[num].chip_addr[MAX6615]);

  pem[num].fd = i2c_cdev_slave_open(pem[num].bus, pem[num].chip_addr[LTC4282],
                                    I2C_SLAVE_FORCE_CLAIM);
  g_fd = pem[num].fd;
  if (pem[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if (vendor == NULL) {
    ret = get_mfr_model(num, block);
    if (ret < 0) {
      printf("Cannot Get PEM Model\n");
      return -1;
    }
  }

  if ((vendor == NULL && !strncmp((const char *)block, DELTA_MODEL, strlen(DELTA_MODEL))) ||
      !strncasecmp(vendor, "delta", strlen("delta"))) {
      ret = update_delta_pem(num, file_path, force);
    } else {
      printf("Unsupported vendor: %s\n", vendor);
      return -1;
  }

  if (ret == 0 || ret == FW_IDENTICAL) {
    sensord_operation(num, START);
    pal_add_i2c_device(pem[num].bus, pem[num].chip_addr[LTC4282], PEM_LTC4282_DRIVER);
    pal_add_i2c_device(pem[num].bus, pem[num].chip_addr[MAX6615], PEM_MAX6615_DRIVER);
  }
  close(pem[num].fd);

  return ret;
}
