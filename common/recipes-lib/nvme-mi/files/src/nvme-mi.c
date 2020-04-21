/* Copyright 2017-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openbmc/obmc-i2c.h>
#include <time.h>
#include <errno.h>
#include "nvme-mi.h"

// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

/* Read a byte from NVMe-MI 0x6A. Need to give a bus and a byte address for reading. */
int
nvme_read_byte(const char *i2c_bus_device, uint8_t item, uint8_t *value) {
  int dev;
  int ret;
  int32_t res;
  int retry = 0;

  dev = open(i2c_bus_device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_NVME_INTF_ADDR);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
    close(dev);
    return -1;
  }

  res = i2c_smbus_read_byte_data(dev, item);
  retry = 0;
  while ((retry < 5) && (res < 0)) {
    msleep(100);
    res = i2c_smbus_read_byte_data(dev, item);
    if (res < 0)
      retry++;
    else
      break;
  }

  if (res < 0) {
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_byte_data failed", __func__);
    close(dev);
    return -1;
  }

  *value = (uint8_t) res;

  close(dev);

  return 0;
}

/* Read a word from NVMe-MI 0x6A. Need to give a bus and a byte address for reading. */
int
nvme_read_word(const char *i2c_bus_device, uint8_t item, uint16_t *value) {
  int dev;
  int ret;
  int32_t res;
  int retry = 0;

  dev = open(i2c_bus_device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
    return -1;
  }

  ret = ioctl(dev, I2C_SLAVE, I2C_NVME_INTF_ADDR);
  if (ret < 0) {
    syslog(LOG_DEBUG, "%s(): ioctl() assigning i2c addr failed", __func__);
    close(dev);
    return -1;
  }

  res = i2c_smbus_read_word_data(dev, item);
  retry = 0;
  while ((retry < 5) && (res < 0)) {
    msleep(100);
    res = i2c_smbus_read_word_data(dev, item);
    if (res < 0)
      retry++;
    else
      break;
  }

  if (res < 0) {
    syslog(LOG_DEBUG, "%s(): i2c_smbus_read_byte_data failed", __func__);
    close(dev);
    return -1;
  }

  *value = (uint16_t) res;

  close(dev);

  return 0;
}

/* Read NVMe-MI Status Flags. Need to give a bus for reading. */
int
nvme_sflgs_read(const char *i2c_bus_device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(i2c_bus_device, NVME_SFLGS_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI SMART Warnings. Need to give a bus for reading. */
int
nvme_smart_warning_read(const char *i2c_bus_device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(i2c_bus_device, NVME_WARNING_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI Composite Temperature. Need to give a bus for reading. */
int
nvme_temp_read(const char *i2c_bus_device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(i2c_bus_device, NVME_TEMP_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI Percentage Drive Life Used. Need to give a bus for reading. */
int
nvme_pdlu_read(const char *i2c_bus_device, uint8_t *value) {
  int ret;

  ret = nvme_read_byte(i2c_bus_device, NVME_PDLU_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  return 0;
}

/* Read NVMe-MI Vendor ID. Need to give a bus for reading. */
int
nvme_vendor_read(const char *i2c_bus_device, uint16_t *value) {
  int ret;

  ret = nvme_read_word(i2c_bus_device, NVME_VENDOR_REG, value);

  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
    return -1;
  }

  *value = (*value & 0xFF00) >> 8 | (*value & 0xFF) << 8;

  return 0;
}

/* Read NVMe-MI Serial Number. Need to give a bus for reading. */
int
nvme_serial_num_read(const char *i2c_bus_device, uint8_t *value, int size) {
  int ret;
  uint8_t reg = NVME_SERIAL_NUM_REG;
  int count;

  if(size != SERIAL_NUM_SIZE) {
    syslog(LOG_DEBUG, "%s(): the array size is wrong", __func__);
    return -1;
  }

  for(count = 0; count < SERIAL_NUM_SIZE; count++) {
    ret = nvme_read_byte(i2c_bus_device, reg + count, value + count);
    if(ret < 0) {
      syslog(LOG_DEBUG, "%s(): nvme_read_byte failed", __func__);
      return -1;
    }
  }
  return 0;
}

int
check_nvme_fileds_valid(uint8_t block_len, t_key_value_pair *tmp_decoding) {
  // If block length is 0xFF, it means this block is non-supported field.
  if (block_len == 0xFF) {
    sprintf(tmp_decoding->value, "%s", "NA");
    return INVALID;
  } else {
    return VALID;
  }
}

int
nvme_sflgs_decode(uint8_t value, t_status_flags *status_flag_decoding) {

  if (!status_flag_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(status_flag_decoding->self.key, "Status Flags");
  sprintf(status_flag_decoding->self.value, "0x%02X", value);

  sprintf(status_flag_decoding->read_complete.key, "SMBUS block read complete");
  sprintf(status_flag_decoding->read_complete.value, (value & 0x80)?"OK":"No");

  sprintf(status_flag_decoding->ready.key, "Drive Ready");
  sprintf(status_flag_decoding->ready.value, (value & 0x40)?"Not ready":"Ready");

  sprintf(status_flag_decoding->functional.key, "Drive Functional");
  sprintf(status_flag_decoding->functional.value, (value & 0x20)?"Functional":"Unrecoverable Failure");

  sprintf(status_flag_decoding->reset_required.key, "Reset Required");
  sprintf(status_flag_decoding->reset_required.value, (value & 0x10)?"No":"Required");

  sprintf(status_flag_decoding->port0_link.key, "Port 0 PCIe Link Active");
  sprintf(status_flag_decoding->port0_link.value, (value & 0x08)?"Up":"Down");

  sprintf(status_flag_decoding->port1_link.key, "Port 1 PCIe Link Active");
  sprintf(status_flag_decoding->port1_link.value, (value & 0x04)?"Up":"Down");

  return 0;
}

int
nvme_smart_warning_decode(uint8_t value, t_smart_warning *smart_warning_decoding) {

  if (!smart_warning_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(smart_warning_decoding->self.key, "SMART Critical Warning");
  sprintf(smart_warning_decoding->self.value, "0x%02X", value);

  sprintf(smart_warning_decoding->spare_space.key, "Spare Space");
  sprintf(smart_warning_decoding->spare_space.value, (value & 0x01)?"Normal":"Low");

  sprintf(smart_warning_decoding->temp_warning.key, "Temperature Warning");
  sprintf(smart_warning_decoding->temp_warning.value, (value & 0x02)?"Normal":"Abnormal");

  sprintf(smart_warning_decoding->reliability.key, "NVM Subsystem Reliability");
  sprintf(smart_warning_decoding->reliability.value, (value & 0x04)?"Normal":"Degraded");

  sprintf(smart_warning_decoding->media_status.key, "Media Status");
  sprintf(smart_warning_decoding->media_status.value, (value & 0x08)?"Normal":"Read Only mode");

  sprintf(smart_warning_decoding->backup_device.key, "Volatile Memory Backup Device");
  sprintf(smart_warning_decoding->backup_device.value, (value & 0x10)?"Normal":"Failed");

  return 0;
}

int
nvme_temp_decode(uint8_t value, t_key_value_pair *temp_decoding) {

  if (!temp_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(temp_decoding->key, "Composite Temperature");
  if (value <= TEMP_HIGHER_THAN_127)
    sprintf(temp_decoding->value, "%d C", value);
  else if (value >= TEPM_LOWER_THAN_n60)
    sprintf(temp_decoding->value, "%d C", -(0x100 - value));
  else if (value == TEMP_NO_UPDATE)
    sprintf(temp_decoding->value, "No data or data is too old");
  else if (value == TEMP_SENSOR_FAIL)
    sprintf(temp_decoding->value, "Sensor failure");

  return 0;
}

int
nvme_lower_threshold_temp_decode(uint8_t block_len, uint8_t value, t_key_value_pair *temp_decoding) {

  if (!temp_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (check_nvme_fileds_valid(block_len, temp_decoding) == VALID) {
    if (nvme_temp_decode(value, temp_decoding) < 0) {
      syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
      return -1;
    }
  }
  sprintf(temp_decoding->key, "Lower Thermal Threshold");

  return 0;
}

int
nvme_upper_threshold_temp_decode(uint8_t block_len, uint8_t value, t_key_value_pair *temp_decoding) {

  if (!temp_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (check_nvme_fileds_valid(block_len, temp_decoding) == VALID) {
    if (nvme_temp_decode(value, temp_decoding) < 0) {
      syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
      return -1;
    }
  }
  sprintf(temp_decoding->key, "Upper Thermal Threshold");

  return 0;
}

int
nvme_max_asic_temp_decode(uint8_t block_len, uint8_t value, t_key_value_pair *temp_decoding) {

  if (!temp_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (check_nvme_fileds_valid(block_len, temp_decoding) == VALID) {
    if (nvme_temp_decode(value, temp_decoding) < 0) {
      syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
      return -1;
    }
  }
  sprintf(temp_decoding->key, "Historical Max ASIC Temperature");

  return 0;
}

int
nvme_pdlu_decode(uint8_t value, t_key_value_pair *pdlu_decoding) {

  if (!pdlu_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(pdlu_decoding->key, "Percentage Drive Life Used");
  sprintf(pdlu_decoding->value, "%d", value);

  return 0;
}

int
nvme_vendor_decode(uint16_t value, t_key_value_pair *vendor_decoding) {

  if (!vendor_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(vendor_decoding->key, "Vendor");
  switch (value) {
    case VENDOR_ID_HGST:
      sprintf(vendor_decoding->value, "HGST(0x%04X)", value);
      break;
    case VENDOR_ID_HYNIX:
      sprintf(vendor_decoding->value, "Hynix(0x%04X)", value);
      break;
    case VENDOR_ID_INTEL:
      sprintf(vendor_decoding->value, "Intel(0x%04X)", value);
      break;
    case VENDOR_ID_LITEON:
      sprintf(vendor_decoding->value, "Lite-on(0x%04X)", value);
      break;
    case VENDOR_ID_MICRON:
      sprintf(vendor_decoding->value, "Micron(0x%04X)", value);
      break;
    case VENDOR_ID_SAMSUNG:
      sprintf(vendor_decoding->value, "Samsung(0x%04X)", value);
      break;
    case VENDOR_ID_SEAGATE:
      sprintf(vendor_decoding->value, "Seagate(0x%04X)", value);
      break;
    case VENDOR_ID_TOSHIBA:
      sprintf(vendor_decoding->value, "Toshiba(0x%04X)", value);
      break;
    case VENDOR_ID_FACEBOOK:
      sprintf(vendor_decoding->value, "Facebook(0x%04X)", value);
      break;
    case VENDOR_ID_BROARDCOM:
      sprintf(vendor_decoding->value, "Broadcom(0x%04X)", value);
      break;
    case VENDOR_ID_QUALCOMM:
      sprintf(vendor_decoding->value, "Qualcomm(0x%04X)", value);
      break;
    default:
      sprintf(vendor_decoding->value, "Unknown(0x%04X)", value);
      break;
  }

  return 0;
}

int
nvme_serial_num_decode(uint8_t *value, t_key_value_pair *sn_decoding) {

  if (!value || !sn_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(sn_decoding->key, "Serial Number");
  memcpy(sn_decoding->value, value, SERIAL_NUM_SIZE);
  sn_decoding->value[SERIAL_NUM_SIZE] = '\0';

  return 0;
}

int
nvme_part_num_decode(uint8_t block_len, uint8_t *value, t_key_value_pair *pn_decoding) {

  if (!value || !pn_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(pn_decoding->key, "Module Product Part Number");
  if (check_nvme_fileds_valid(block_len, pn_decoding) == VALID) {
    if (value[0] == 0xFF) { // RosePeak does not have Module Product Part Number
      sprintf(pn_decoding->value, "NA");
    } else {
      memcpy(pn_decoding->value, value, PART_NUM_SIZE);
      pn_decoding->value[PART_NUM_SIZE] = '\0';
    }
  }

  return 0;
}

int
nvme_meff_decode (uint8_t block_len, uint8_t value, t_key_value_pair *meff_decoding) {

  if (!value || !meff_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(meff_decoding->key, "Management End Point Form Factor");
  if (check_nvme_fileds_valid(block_len, meff_decoding) == VALID) {
    switch (value) {
      case MEFF_M_2_22110:
        sprintf(meff_decoding->value, "M.2 22110 (0x%02X)", value);
        break;
      case MEFF_Dual_M_2:
        sprintf(meff_decoding->value, "Dual M.2 (0x%02X)", value);
        break;
      default:
        sprintf(meff_decoding->value, "Unknown (0x%02X)", value);
        break;
    }
  }

  return 0;
}

int
nvme_ffi_0_decode (uint8_t block_len, uint8_t value, t_key_value_pair *ffi_0_decoding) {

  if (!ffi_0_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(ffi_0_decoding->key, "Form Factor Information 0 Register");
  if (check_nvme_fileds_valid(block_len, ffi_0_decoding) == VALID) {
    switch (value) {
      case FFI_0_STORAGE:
        sprintf(ffi_0_decoding->value, "Storage (0x%02X)", value);
        break;
      case FFI_0_ACCELERATOR:
        sprintf(ffi_0_decoding->value, "Accelerator (0x%02X)", value);
        break;
      default:
        sprintf(ffi_0_decoding->value, "Unknown (0x%02X)", value);
        break;
    }
  }

  return 0;
}

int
nvme_power_state_decode (uint8_t block_len, uint8_t value, t_key_value_pair *power_state_decoding) {

  if (!power_state_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(power_state_decoding->key, "Power State");
  if (check_nvme_fileds_valid(block_len, power_state_decoding) == VALID) {
    switch (value) {
      case POWER_STATE_FULL_MODE:
        sprintf(power_state_decoding->value, "Full Power Mode (0x%02X)", value);
        break;
      case POWER_STATE_REDUCED_MODE:
        sprintf(power_state_decoding->value, "Reduced Power Mode (0x%02X)", value);
        break;
      case POWER_STATE_LOWER_MODE:
        sprintf(power_state_decoding->value, "Lower Power Mode (0x%02X)", value);
        break;
      default:
        sprintf(power_state_decoding->value, "Unknown (0x%02X)", value);
        break;
    }
  }

  return 0;
}

int
nvme_i2c_freq_decode (uint8_t block_len, uint8_t value, t_key_value_pair *i2c_freq_decoding) {

  if (!i2c_freq_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(i2c_freq_decoding->key, "SMBus/I2C Frquency");
  if (check_nvme_fileds_valid(block_len, i2c_freq_decoding) == VALID) {
    switch (value) {
      case I2C_FREQ_100K:
        sprintf(i2c_freq_decoding->value, "100 kHz");
        break;
      case I2C_FREQ_400K:
        sprintf(i2c_freq_decoding->value, "400 kHz");
        break;
      case I2C_FREQ_1M:
        sprintf(i2c_freq_decoding->value, "1 MHz");
        break;
      default:
        sprintf(i2c_freq_decoding->value, "Unknown(0x%02X)", value);
        break;
    }
  }

  return 0;
}

int
nvme_tdp_level_decode (uint8_t block_len, uint8_t value, t_key_value_pair *tdp_level_decoding) {

  if (!tdp_level_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(tdp_level_decoding->key, "Module Static TDP level setting");
  if (check_nvme_fileds_valid(block_len, tdp_level_decoding) == VALID) {
    switch (value) {
      case TDP_LEVEL1:
      case TDP_LEVEL2:
      case TDP_LEVEL3:
        sprintf(tdp_level_decoding->value, "level %d", value);
        break;
      default:
        sprintf(tdp_level_decoding->value, "Unknown (0x%02X)", value);
        break;
    }
  }

  return 0;
}

int
nvme_fw_version_decode (uint8_t block_len, uint8_t major_value, uint8_t minor_value, t_key_value_pair *fw_version_decoding) {

  if (!fw_version_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(fw_version_decoding->key, "%s", "FW version");
  if (check_nvme_fileds_valid(block_len, fw_version_decoding) == VALID) {
    sprintf(fw_version_decoding->value, "v%d.%d", major_value, minor_value);
  }

  return 0;
}

int
nvme_monitor_area_decode (char *key, uint8_t block_len, uint16_t value, float unit, t_key_value_pair *monitor_area_decoding) {

  if (!monitor_area_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(monitor_area_decoding->key, "%s", key);
  if (check_nvme_fileds_valid(block_len, monitor_area_decoding) == VALID) {
    sprintf(monitor_area_decoding->value, "%.4f V", (value * unit));
  }

  return 0;
}

int
nvme_total_int_mem_err_count_decode(uint8_t block_len, uint8_t value, t_key_value_pair *total_int_mem_err_count_decoding) {

  if (!total_int_mem_err_count_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(total_int_mem_err_count_decoding->key, "Total internal memory error count");
  if (check_nvme_fileds_valid(block_len, total_int_mem_err_count_decoding) == VALID) {
    if (value == 0xFF) { // SpringHill does not have Total internal memory error count
      sprintf(total_int_mem_err_count_decoding->value, "NA");
    } else {
      sprintf(total_int_mem_err_count_decoding->value, "%d", value);
    }
  }

  return 0;
}

int
nvme_total_ext_mem_err_count_decode(uint8_t block_len, uint8_t value, t_key_value_pair *total_ext_mem_err_count_decoding) {

  if (!total_ext_mem_err_count_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(total_ext_mem_err_count_decoding->key, "Total external memory error count");
  if (check_nvme_fileds_valid(block_len, total_ext_mem_err_count_decoding) == VALID) {
    if (value == 0xFF) { // SpringHill does not have Total external memory error count
      sprintf(total_ext_mem_err_count_decoding->value, "NA");
    } else {
      sprintf(total_ext_mem_err_count_decoding->value, "%d", value);
    }
  }

  return 0;
}

int
nvme_smbus_err_decode(uint8_t block_len, uint8_t value, t_key_value_pair *smbus_err_decoding) {

  if (!smbus_err_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(smbus_err_decoding->key, "SMBus error");
  if (check_nvme_fileds_valid(block_len, smbus_err_decoding) == VALID) {
    if (value == 0xFF) { // SpringHill does not have SMBus Error
      sprintf(smbus_err_decoding->value, "NA");
    } else {
      sprintf(smbus_err_decoding->value, "0x%02X", value);
    }
  }

  return 0;
}


int
nvme_raw_data_prase (char *key, uint8_t block_len, uint8_t value, t_key_value_pair *raw_data_prasing) {

  if (!raw_data_prasing) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(raw_data_prasing->key, "%s", key);
  if (check_nvme_fileds_valid(block_len, raw_data_prasing) == VALID) {
    sprintf(raw_data_prasing->value, "0x%02X", value);
  }

  return 0;
}

int
nvme_sinfo_0_decode (uint8_t value, t_key_value_pair *sinfo_0_decoding) {

  if (!sinfo_0_decoding) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  sprintf(sinfo_0_decoding->key, "Storage Information 0");

  switch (value & 0x3) {
    case SINFO_0_PLP_NOT_DEFINED:
      sprintf(sinfo_0_decoding->value, "Power Loss Protection not defined");
      break;
    case SINFO_0_PLP_DEFINED:
      sprintf(sinfo_0_decoding->value, "Power Loss Protection supported");
      break;
    default:
      sprintf(sinfo_0_decoding->value, "Unknown (0x%02X)", value);
      break;
  }

  return 0;
}

/* Read NVMe-MI Status Flags and decode it. */
int
nvme_sflgs_read_decode(const char *i2c_bus_device, uint8_t *value, t_status_flags *status_flag_decoding) {

  if ((i2c_bus_device == NULL) | (value == NULL) | (status_flag_decoding == NULL)) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (nvme_sflgs_read(i2c_bus_device, value)) {
    syslog(LOG_DEBUG, "%s(): nvme_sflgs_read failed", __func__);
    sprintf(status_flag_decoding->self.key, "Status Flags");
    sprintf(status_flag_decoding->self.value, "Fail on reading");
    return -1;
  }
  else {
    nvme_sflgs_decode(*value, status_flag_decoding);
  }

  return 0;
}

/* Read NVMe-MI SMART Warnings and decode it. */
int
nvme_smart_warning_read_decode(const char *i2c_bus_device, uint8_t *value, t_smart_warning *smart_warning_decoding) {

  if ((i2c_bus_device == NULL) | (value == NULL) | (smart_warning_decoding == NULL)) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (nvme_smart_warning_read(i2c_bus_device, value)) {
    syslog(LOG_DEBUG, "%s(): nvme_smart_warning_read failed", __func__);
    sprintf(smart_warning_decoding->self.key, "SMART Critical Warning");
    sprintf(smart_warning_decoding->self.value, "Fail on reading");
    return -1;
  }
  else {
    nvme_smart_warning_decode(*value, smart_warning_decoding);
  }

  return 0;
}

/* Read NVMe-MI Composite Temperature and decode it. */
int
nvme_temp_read_decode(const char *i2c_bus_device, uint8_t *value, t_key_value_pair *temp_decoding) {

  if ((i2c_bus_device == NULL) | (value == NULL) | (temp_decoding == NULL)) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (nvme_temp_read(i2c_bus_device, value)) {
    syslog(LOG_DEBUG, "%s(): nvme_temp_read failed", __func__);
    sprintf(temp_decoding->key, "Composite Temperature");
    sprintf(temp_decoding->value, "Fail on reading");
    return -1;
  }
  else {
    nvme_temp_decode(*value, temp_decoding);
  }

  return 0;
}

/* Read NVMe-MI Percentage Drive Life Used and decode it. */
int
nvme_pdlu_read_decode(const char *i2c_bus_device, uint8_t *value, t_key_value_pair *pdlu_decoding) {

  if ((i2c_bus_device == NULL) | (value == NULL) | (pdlu_decoding == NULL)) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (nvme_pdlu_read(i2c_bus_device, value)) {
    syslog(LOG_DEBUG, "%s(): nvme_pdlu_read failed", __func__);
    sprintf(pdlu_decoding->key, "Percentage Drive Life Used");
    sprintf(pdlu_decoding->value, "Fail on reading");
    return -1;
  }
  else {
    nvme_pdlu_decode(*value, pdlu_decoding);
  }

  return 0;
}

/* Read NVMe-MI Vendor ID and decode it. */
int
nvme_vendor_read_decode(const char *i2c_bus_device, uint16_t *value, t_key_value_pair *vendor_decoding) {

  if ((i2c_bus_device == NULL) | (value == NULL) | (vendor_decoding == NULL)) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (nvme_vendor_read(i2c_bus_device, value)) {
    syslog(LOG_DEBUG, "%s(): nvme_vendor_read failed", __func__);
    sprintf(vendor_decoding->key, "Vendor");
    sprintf(vendor_decoding->value, "Fail on reading");
    return -1;
  }
  else {
    nvme_vendor_decode(*value, vendor_decoding);
  }

  return 0;
}

/* Read NVMe-MI Serial Number and decode it. */
int
nvme_serial_num_read_decode(const char *i2c_bus_device, uint8_t *value, int size, t_key_value_pair *sn_decoding) {

  if ((i2c_bus_device == NULL) | (value == NULL) | (sn_decoding == NULL)) {
    syslog(LOG_ERR, "%s(): invalid parameter (null)", __func__);
    return -1;
  }

  if (nvme_serial_num_read(i2c_bus_device, value, SERIAL_NUM_SIZE)) {
    syslog(LOG_DEBUG, "%s(): nvme_serial_num_read failed", __func__);
    sprintf(sn_decoding->key, "Serial Number");
    sprintf(sn_decoding->value, "Fail on reading");
    return -1;
  }
  else {
    nvme_serial_num_decode(value, sn_decoding);
  }

  return 0;
}

