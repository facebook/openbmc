/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <openbmc/libgpio.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

#define REG_STATUS 		0x1E
#define REG_STATUS_ON		(1 << 7)
#define REG_STATUS_ON_PIN	(1 << 4)
#define REG_STATUS_POWER_GOOD 	(1 << 3)

#define DELAY_POWER_CYCLE 3
#define MAX_RETRY 10

#define BMC_IPMB_SLAVE_ADDR 0x16

#define GUID_SIZE 16
#define OFFSET_DEV_GUID 0x1800

#define LAST_KEY "last_key"

const char pal_fru_list[] = "all, fru";
const char pal_server_list[] = "fru";

char g_dev_guid[GUID_SIZE] = {0};

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"identify_sled", "off", NULL},
  {"timestamp_sled", "0", NULL},
  {"base_sensor_health", "1", NULL},
  //{"base_sel_error", "1", NULL},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

static int pal_key_index(char *key)
{
  int i = 0;

  while (strcmp(key_cfg[i].name, LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_index: invalid key - %s", key);
#endif
  return -1;
}

int pal_get_key_value(char *key, char *value)
{
  int index;

  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  return kv_get(key, value, NULL, KV_FPERSIST);
}

int pal_set_key_value(char *key, char *value)
{
  int index, ret;
  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  if (key_cfg[index].function) {
    ret = key_cfg[index].function(KEY_BEFORE_SET, value);
    if (ret < 0)
      return ret;
  }

  return kv_set(key, value, 0, KV_FPERSIST);
}

int pal_set_def_key_value()
{
  int i;

  for (i = 0; strcmp(key_cfg[i].name, LAST_KEY) != 0; i++) {
    if (kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed.");
#endif
    }
    if (key_cfg[i].function) {
      key_cfg[i].function(KEY_AFTER_INI, key_cfg[i].name);
    }
  }

  return 0;
}

int pal_channel_to_bus(int channel)
{
  switch (channel) {
    case 0:
      return 13; // USB (LCD Debug Board)
    case 1:
      return 1; // MB#1
    case 2:
      return 0; // MB#2
    case 3:
      return 3; // MB#3
    case 4:
      return 2; // MB#4
  }

  // Debug purpose, map to real bus number
  if (channel & 0x80) {
    return (channel & 0x7f);
  }

  return channel;
}

int pal_get_fruid_path(uint8_t fru, char *path)
{

  sprintf(path, FRU_BIN);
  return 0;
}

int pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{

  sprintf(path, FRU_EEPROM);
  return 0;
}

int pal_get_fru_id(char *str, uint8_t *fru)
{
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "fru")) {
    *fru = FRU_BASE;
  } else {
    syslog(LOG_WARNING, "%s: Wrong fru name %s", __func__, str);
    return -1;
  }

  return 0;
}

int pal_get_fruid_name(uint8_t fru, char *name)
{

  sprintf(name, "Base Board");
  return 0;
}

int pal_is_fru_ready(uint8_t fru, uint8_t *status)
{

  *status = 1;
  return 0;
}

int pal_get_fru_name(uint8_t fru, char *name)
{
  if (fru != FRU_BASE) {
    syslog(LOG_WARNING, "%s: Wrong fruid %d", __func__, fru);
    return -1;
  }

  strcpy(name, "fru");

  return 0;
}

int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  *slave_addr = BMC_IPMB_SLAVE_ADDR;
  return 0;
}

// GUID for System and Device
static int pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;

  errno = 0;

  // Check if file is present
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_get_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(FRU_EEPROM, O_RDONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_get_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

static int pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;

  errno = 0;

  // Check for file presence
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_set_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(FRU_EEPROM, O_WRONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_set_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void pal_populate_guid(char *guid, char *str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  //getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  //memcpy(&guid[8], rand(), 2);
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r>>8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str)-1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15-count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15-count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6-count);
  }

}

int pal_set_dev_guid(uint8_t fru, char *str) {
  pal_populate_guid(g_dev_guid, str);

  return pal_set_guid(OFFSET_DEV_GUID, g_dev_guid);
}

int pal_get_dev_guid(uint8_t fru, char *guid) {

  pal_get_guid(OFFSET_DEV_GUID, g_dev_guid);

  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

static int pal_read_hsc_reg(uint8_t id, uint8_t reg, uint8_t bytes, uint32_t *value)
{
  int ret;
  int fd;
  int retry = MAX_RETRY;
  uint8_t mux_addr = 0xe0;
  uint8_t addr, chan;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  switch (id) {
    case HSC_1:
      addr = 0xa6;
      chan = 0x4;
      break;
    case HSC_2:
      addr = 0x80;
      chan = 0x5;
      break;
    case HSC_AUX:
      addr = 0x86;
      chan = 0x6;
      break;
    default:
      return -1;
  }

  fd = open("/dev/i2c-5", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING,"[%s] Cannot open i2c bus", __func__);
    return -1;
  }

  // switch mux to target channel
  tbuf[0] = 0xa5; tbuf[1] = chan;
  while (retry > 0) {
    ret = i2c_rdwr_msg_transfer(fd, mux_addr, tbuf, 2, rbuf, 0);
    if (PAL_EOK == ret)
      break;

    msleep(50);
    retry--;
  }
  if (ret < 0) {
    syslog(LOG_WARNING,"[%s] Failed to switch the mux to target channel", __func__);
    goto exit;
  }

  // Read from register of LTC4282
  memset(tbuf, 0x0, 8);
  tbuf[0] = reg;
  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, bytes);
  if (ret < 0) {
    syslog(LOG_WARNING,"[%s] Failed to read 0x%x from addr 0x%x on i2c-5",
	__func__, reg, addr);
    goto exit;
  }

  *value = 0x0;
  memcpy(value, rbuf, bytes);

exit:
  close(fd);
  return ret;
}

#if 0
static int pal_write_hsc_reg(uint8_t id, uint8_t reg, uint8_t bytes, uint64_t *value)
{
  int i;
  int ret;
  int fd;
  uint8_t addr, chan;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};
  uint8_t *data = (uint8_t*)value;
  char cmd[64] = {0};

  switch (id) {
    case HSC_1:
      addr = 0xa6;
      chan = 0x4;
      break;
    case HSC_2:
      addr = 0x80;
      chan = 0x5;
      break;
    case HSC_AUX:
      addr = 0x86;
      chan = 0x6;
      break;
    default:
      return -1;
  }

  fd = open("/dev/i2c-5", O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING,"[%s] Cannot open i2c-5", __func__);
    return -1;
  }

  // switch mux
  sprintf(cmd, "i2cset -y 5 0x70 0xa5 0x%x", chan);
  system(cmd);

  // Write register of LTC4282
  tbuf[0] = reg;
  for (i = 1; i < bytes+1; i++)
    tbuf[i] = data[i-1];

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, bytes+1, rbuf, 0);
  if (ret < 0) {
    syslog(LOG_WARNING,"[%s] Failed to read 0x%x from addr 0x%x on i2c-5",
	__func__, reg, addr);
    goto exit;
  }

exit:
  close(fd);
  return ret;
}
#endif

int pal_get_server_power(uint8_t fru, uint8_t *status)
{
  int ret;
  uint32_t val;
  uint32_t POWER_ON = (REG_STATUS_ON | REG_STATUS_ON_PIN | REG_STATUS_POWER_GOOD);

  if (fru != FRU_BASE)
    return -1;

  ret = pal_read_hsc_reg(HSC_1, REG_STATUS, 1, &val);
  if (ret < 0)
    return -1;

  *status = val == POWER_ON? 1: 0;

  ret = pal_read_hsc_reg(HSC_2, REG_STATUS, 1, &val);
  if (ret < 0)
    return -1;

  *status &= val == POWER_ON? 1: 0;
  *status = *status == 1? SERVER_POWER_ON: SERVER_POWER_OFF;

  return 0;
}

int pal_set_usb_path(uint8_t slot, uint8_t endpoint)
{
  int ret = CC_SUCCESS;
  char gpio_name[16] = {0};
  gpio_desc_t *desc;
  gpio_value_t value;

  if (slot > SERVER_4 || endpoint > PCH) {
    ret = CC_PARAM_OUT_OF_RANGE;
    goto exit;
  }

  desc = gpio_open_by_shadow("SEL_USB_MUX");
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = endpoint == BMC? GPIO_VALUE_LOW: GPIO_VALUE_HIGH;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }
  gpio_close(desc);

  strcpy(gpio_name, endpoint == BMC? "USB2_SEL0_U43": "USB2_SEL0_U42");
  desc = gpio_open_by_shadow(gpio_name);
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = slot%2 == 0? GPIO_VALUE_LOW: GPIO_VALUE_HIGH;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }
  gpio_close(desc);

  strcpy(gpio_name, endpoint == BMC? "USB2_SEL1_U43": "USB2_SEL1_U42");
  desc = gpio_open_by_shadow(gpio_name);
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = slot/2 == 0? GPIO_VALUE_LOW: GPIO_VALUE_HIGH;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }

bail:
  gpio_close(desc);
exit:
  return ret;
}

static int server_power_on()
{
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("BMC_IPMI_PWR_ON");

  if (!gpio) {
    return -1;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  sleep(2);
  system("/usr/bin/sv restart fscd >> /dev/null");
  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

static int server_power_off()
{
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("BMC_IPMI_PWR_ON");

  if (!gpio) {
    return -1;
  }

  system("/usr/bin/sv stop fscd >> /dev/null");

  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

static bool is_server_off()
{
  uint8_t status;

  if (pal_get_server_power(FRU_BASE, &status) < 0)
    return false;

  return status == SERVER_POWER_OFF? true: false;
}

// Power Off, Power On, or Power Cycle
int pal_set_server_power(uint8_t fru, uint8_t cmd)
{
  uint8_t status;

  if (pal_get_server_power(fru, &status) < 0) {
    return -1;
  }

  switch (cmd) {
    case SERVER_POWER_ON:
        return status == SERVER_POWER_ON? 1: server_power_on();
      break;

    case SERVER_POWER_OFF:
        return status == SERVER_POWER_OFF? 1: server_power_off();
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off())
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on();
      } else if (status == SERVER_POWER_OFF) {
        return server_power_on();
      }
      break;

    default:
      return -1;
  }

  return 0;
}

int pal_chassis_control(uint8_t fru, uint8_t *req_data, uint8_t req_len)
{
  int ret;

  if (req_len != 1) {
    return CC_INVALID_LENGTH;
  }

  switch (req_data[0]) {
    case 0x00:  // power off
      ret = pal_set_server_power(fru, SERVER_POWER_OFF);
      break;
    case 0x01:  // power on
      ret = pal_set_server_power(fru, SERVER_POWER_ON);
      break;
    case 0x02:  // power cycle
      ret = pal_set_server_power(fru, SERVER_POWER_CYCLE);
      break;
    default:
      return CC_INVALID_DATA_FIELD;
  }

  return ret < 0? CC_UNSPECIFIED_ERROR: CC_SUCCESS;
}

void pal_get_chassis_status(uint8_t fru, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
   unsigned char *data = res_data;

   *data++ = is_server_off()? 0x00:0x01;
   *res_len = data - res_data;
}

int pal_sled_cycle(void)
{
  // switch the mux and reboot LTC4282 for 12V
  system("i2cset -y 5 0x70 0xa5 0x6");
  system("i2cset -y 5 0x43 0x1d 0x80");

  return 0;
}

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status)
{
  if (fru != FRU_BASE)
    return -1;

  *status = 1;
  return 0;
}

