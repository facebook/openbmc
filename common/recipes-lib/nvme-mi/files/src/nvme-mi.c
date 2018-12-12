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

#define I2C_NVME_INTF_ADDR 0x6A

#define NVME_SFLGS_REG 0x01
#define NVME_WARNING_REG 0x02
#define NVME_TEMP_REG 0x03
#define NVME_PDLU_REG 0x04
#define NVME_VENDOR_REG 0x09
#define NVME_SERIAL_NUM_REG 0x0B
#define SERIAL_NUM_SIZE 20

/* NVMe-MI Temperature Definition Code */
#define TEMP_HIGHER_THAN_127 0x7F
#define TEPM_LOWER_THAN_n60 0xC4
#define TEMP_NO_UPDATE 0x80
#define TEMP_SENSOR_FAIL 0x81

/* NVMe-MI Vendor ID Code */
#define VENDOR_ID_HGST 0x1C58
#define VENDOR_ID_HYNIX 0x1C5C
#define VENDOR_ID_INTEL 0x8086
#define VENDOR_ID_LITEON 0x14A4
#define VENDOR_ID_MICRON 0x1344
#define VENDOR_ID_SAMSUNG 0x144D
#define VENDOR_ID_SEAGATE 0x1BB1
#define VENDOR_ID_TOSHIBA 0x1179

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

