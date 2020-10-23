/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
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
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <openbmc/obmc-sensors.h>
#include "pal.h"

#define NUM_SERVER_FRU  1
#define NUM_NIC_FRU     1
#define NUM_BMC_FRU     1
#define MAX_FAN_NAME    32

const char pal_fru_list[] = "all, server, bmc, uic, dpb, scc, nic, iocm";
const char pal_fru_list_print[] = "all, server, bmc, uic, dpb, scc, nic, iocm";
const char pal_fru_list_rw[] = "server, bmc, uic, nic, iocm";
const char pal_fru_list_sensor_history[] = "all, server, uic, nic, iocm";
const char pal_server_list[] = "server";
const char *fru_str_list[] = {"all", "server", "bmc", "uic", "dpb", "scc", "nic", "iocm"};
const char *pal_server_fru_list[NUM_SERVER_FRU] = {"server"};
const char *pal_nic_fru_list[NUM_NIC_FRU] = {"nic"};
const char *pal_bmc_fru_list[NUM_BMC_FRU] = {"bmc"};

size_t server_fru_cnt = NUM_SERVER_FRU;
size_t nic_fru_cnt  = NUM_NIC_FRU;
size_t bmc_fru_cnt  = NUM_BMC_FRU;

const char pal_pwm_list[] = "0";
const char pal_tach_list[] = "0...7";

size_t pal_pwm_cnt = 1;
size_t pal_tach_cnt = 8;

// TODO: temporary mapping table, will get from fan config after fan table is ready
uint8_t fanid2pwmid_mapping[] = {0, 0, 0, 0, 0, 0, 0, 0};

int
pal_get_fru_id(char *str, uint8_t *fru) {
  int fru_id = -1;
  bool found_id = false;

  for (fru_id = FRU_ALL; fru_id <= MAX_NUM_FRUS; fru_id++) {
    if ( strncmp(str, fru_str_list[fru_id], MAX_FRU_CMD_STR) == 0 ) {
      *fru = fru_id;
      found_id = true;
      break;
    }
  }

  return found_id ? 0 : -1;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  switch (fru) {
    case FRU_SERVER:
    // TODO: Get server power status and BIC ready pin
    case FRU_BMC:
    case FRU_UIC:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
    case FRU_E1S_IOCM:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  // TODO: implement SERVER, SCC, NIC, E1S/IOCM present determination
  switch (fru) {
    case FRU_SERVER:
    case FRU_BMC:
    case FRU_UIC:
    case FRU_DPB:
    case FRU_SCC:
    case FRU_NIC:
    case FRU_E1S_IOCM:
      *status = 1;
      break;
    default:
      *status = 0;
      syslog(LOG_WARNING, "%s() wrong fru id 0x%02x", __func__, fru);
      return PAL_ENOTSUP;
  }

  return 0;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  return fbgc_get_fruid_name(fru, name);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  return fbgc_get_fruid_path(fru, path);
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  return fbgc_get_fruid_eeprom_path(fru, path);
}

int
pal_get_fru_list(char *list) {
  snprintf(list, sizeof(pal_fru_list), pal_fru_list);
  return 0;
}

int
pal_get_fan_name(uint8_t fan_id, char *name) {
  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }
  snprintf(name, MAX_FAN_NAME, "Fan %d %s", (fan_id / 2) + 1, fan_id % 2 == 0 ? "Front" : "Rear");

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  uint8_t type = 0;

  switch(fru) {
    case FRU_SERVER:
      snprintf(name, MAX_FRU_CMD_STR, "server");
      break;
    case FRU_BMC:
      snprintf(name, MAX_FRU_CMD_STR, "bmc");
      break;
    case FRU_UIC:
      snprintf(name, MAX_FRU_CMD_STR, "uic");
      break;
    case FRU_DPB:
      snprintf(name, MAX_FRU_CMD_STR, "dpb");
      break;
    case FRU_SCC:
      snprintf(name, MAX_FRU_CMD_STR, "scc");
      break;
    case FRU_NIC:
      snprintf(name, MAX_FRU_CMD_STR, "nic");
      break;
    case FRU_E1S_IOCM:
      fbgc_common_get_chassis_type(&type);
      if (type == CHASSIS_TYPE7) {
        snprintf(name, MAX_FRU_CMD_STR, "iocm");
      } else {
        snprintf(name, MAX_FRU_CMD_STR, "e1s");
      }
      break;
    default:
      if (fru > MAX_NUM_FRUS) {
        return -1;
      }
      snprintf(name, MAX_FRU_CMD_STR, "fru%d", fru);
      break;
  }

  return 0;
}

int
pal_set_fan_speed(uint8_t fan_id, uint8_t pwm) {
  char label[32] = {0};
  int zone = 0;

  if (fan_id >= pal_pwm_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }
  zone = fanid2pwmid_mapping[fan_id];
  snprintf(label, sizeof(label), "pwm%d", zone);

  return sensors_write_fan(label, (float)pwm);
}

int
pal_get_pwm_value(uint8_t fan_id, uint8_t *pwm) {
  char label[32] = {0};
  float value = 0;
  int ret = 0;
  int zone = 0;

  if (fan_id >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d", __func__, fan_id);
    return -1;
  }
  zone = fanid2pwmid_mapping[fan_id];
  snprintf(label, sizeof(label), "pwm%d", zone);
  ret = sensors_read_fan(label, &value);
  if (ret == 0) {
    *pwm = (uint8_t)value;
  }

  return ret;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(char *guid, char *str) {
  unsigned int secs = 0;
  unsigned int usecs = 0;
  struct timeval tv;
  uint8_t count = 0;
  uint8_t lsb = 0, msb = 0;
  int i = 0, clock_seq = 0;
  
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
  srand(time(NULL));
  clock_seq = rand();
  guid[8] = clock_seq & 0xFF;
  guid[9] = (clock_seq >> 8) & 0xFF;

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

  return;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  char path[MAX_FILE_PATH] = {0};
  int fd = 0;
  int ret = 0;
  ssize_t bytes_rd = 0;

  // Set path for UIC
  snprintf(path, MAX_FILE_PATH, EEPROM_PATH, UIC_FRU_BUS, UIC_FRU_ADDR);

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "%s() unable to access %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() unable to open %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "%s() read from %s failed: %s", __func__, path, strerror(errno));
    ret = -1;
  }

  close(fd);
  
  return ret;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  char path[MAX_FILE_PATH] = {0};
  int fd = 0;
  int ret = 0;
  ssize_t bytes_wr = 0;

  // Set path for UIC
  snprintf(path, MAX_FILE_PATH, EEPROM_PATH, UIC_FRU_BUS, UIC_FRU_ADDR);

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "%s() unable to access %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  fd = open(path, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() unable to open %s: %s", __func__, path, strerror(errno));
    return -1;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "%s() write to %s failed: %s", __func__, path, strerror(errno));
    ret = -1;
  }

  close(fd);
  
  return ret;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  // TODO: the system GUID will get from BIC
  return -1;
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  // TODO: the system GUID will be set from BIC
  return -1;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  return pal_get_guid(OFFSET_DEV_GUID, guid);
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};
  
  pal_populate_guid(guid, str);
  return pal_set_guid(OFFSET_DEV_GUID, guid);
}
