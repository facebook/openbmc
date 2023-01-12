/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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

/*
 * This file contains functions and logics that depends on Yamp specific
 * hardware and kernel drivers. Here, some of the empty "default" functions
 * are overridden with simple functions that returns non-zero error code.
 * This is for preventing any potential escape of failures through the
 * default functions that will return 0 no matter what.
 */

#include "pal.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <openbmc/obmc-i2c.h>

const uint8_t pim_bus[] = { 16, 17, 18, 19, 20, 21, 22, 23 };
const uint8_t pim_bus_p1[] = { 16, 17, 18, 23, 20, 21, 22, 19 };

const char pal_fru_list[] = "all, scm, smb, pim2, pim3, pim4, pim5 \
pim6, pim7, pim8, pim9, psu1, psu2, psu3, psu4, fan";

char* key_list[] = {
    "pwr_server_last_state",
    "sysfw_ver_server",
    "scm_sensor_health",
    "smb_sensor_health",
    "pim2_sensor_health",
    "pim3_sensor_health",
    "pim4_sensor_health",
    "pim5_sensor_health",
    "pim6_sensor_health",
    "pim7_sensor_health",
    "pim8_sensor_health",
    "pim9_sensor_health",
    "psu1_sensor_health",
    "psu2_sensor_health",
    "psu3_sensor_health",
    "psu4_sensor_health",
    "fan_sensor_health",
    "server_boot_order",
    "server_restart_cause",
    "server_cpu_ppin",
    /* Add more Keys here */
    LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server_last_state */
  "0", /* sysfw_ver_server */
  "1", /* scm_sensor_health */
  "1", /* smb_sensor_health */
  "1", /* pim2_sensor_health */
  "1", /* pim3_sensor_health */
  "1", /* pim4_sensor_health */
  "1", /* pim5_sensor_health */
  "1", /* pim6_sensor_health */
  "1", /* pim7_sensor_health */
  "1", /* pim8_sensor_health */
  "1", /* pim9_sensor_health */
  "1", /* psu1_sensor_health */
  "1", /* psu2_sensor_health */
  "1", /* psu3_sensor_health */
  "1", /* psu4_sensor_health */
  "1", /* fan_sensor_health */
  "0000000", /* server_boot_order */
  "3", /* server_restart_cause */
  "0", /* server_cpu_ppin */
  /* Add more def values for the corresponding keys*/
  LAST_KEY /* Same as last entry of the key_list */
};

// Elbert specific Platform Abstraction Layer (PAL) Functions
int pal_get_platform_name(char* name) {
  // Return Elbert Specific value
  strcpy(name, ELBERT_PLATFORM_NAME);
  return 0;
}

int pal_get_num_slots(uint8_t* num) {
  // Return Elbert Specific Value
  *num = ELBERT_MAX_NUM_SLOTS;
  return 0;
}

static int pal_key_check(char* key) {
  int i;

  i = 0;
  while (strcmp(key_list[i], LAST_KEY)) {
    // If key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int pal_get_key_value(char* key, char* value) {
  int ret;
  // Check if key is defined and valid
  if (pal_key_check(key))
    return -1;

  ret = kv_get(key, value, NULL, KV_FPERSIST);
  return ret;
}

int pal_set_key_value(char* key, char* value) {
  // Check if key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value, 0, KV_FPERSIST);
}

int
pal_set_def_key_value(void) {
  int i;

  for (i = 0; strncmp(key_list[i], LAST_KEY, strlen(LAST_KEY)) != 0; i++) {
    if (kv_set(key_list[i], def_val_list[i], 0, KV_FPERSIST | KV_FCREATE)) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed.");
#endif
    }
  }

  return 0;
}

int pal_set_sysfw_ver(uint8_t slot, uint8_t* ver) {
  int i;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10];
  sprintf(key, "%s", "sysfw_ver_server");

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int pal_get_sysfw_ver(uint8_t slot, uint8_t* ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];

  sprintf(key, "%s", "sysfw_ver_server");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2 * SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i + 1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

int pal_get_boot_order(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t* boot,
    uint8_t* res_len) {
  int ret, msb, lsb, i, j = 0;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];

  ret = pal_get_key_value("server_boot_order", str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2 * SIZE_BOOT_ORDER; i += 2) {
    snprintf(tstr, sizeof(tstr), "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    snprintf(tstr, sizeof(tstr), "%c\n", str[i + 1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;

  return 0;
}

int pal_set_boot_order(
    uint8_t slot,
    uint8_t* boot,
    uint8_t* res_data,
    uint8_t* res_len) {
  int i, j, offset, network_dev = 0;
  char str[MAX_VALUE_LEN] = {0};
  enum {
    BOOT_DEVICE_IPV4 = 0x1,
    BOOT_DEVICE_IPV6 = 0x9,
  };

  *res_len = 0;

  for (i = offset = 0; i < SIZE_BOOT_ORDER && offset < sizeof(str); i++) {
    if (i > 0) { // byte[0] is boot mode, byte[1:5] are boot order
      for (j = i + 1; j < SIZE_BOOT_ORDER; j++) {
        if (boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      // If bit[2:0] is 001b (Network), bit[3] is IPv4/IPv6 order
      // bit[3]=0b: IPv4 first
      // bit[3]=1b: IPv6 first
      if ((boot[i] == BOOT_DEVICE_IPV4) || (boot[i] == BOOT_DEVICE_IPV6))
        network_dev++;
    }

    offset += snprintf(str + offset, sizeof(str) - offset, "%02x", boot[i]);
  }

  // not allow having more than 1 network boot device in the boot order
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return pal_set_key_value("server_boot_order", str);
}

int pal_set_last_pwr_state(uint8_t fru, char* state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(
        LOG_WARNING,
        "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u",
        fru);
#endif
  }
  return ret;
}

int pal_get_last_pwr_state(uint8_t fru, char* state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(
        LOG_WARNING,
        "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u",
        fru);
#endif
  }
  return ret;
}

int pal_set_restart_cause(uint8_t slot, uint8_t restart_cause) {
  char value[MAX_VALUE_LEN] = {0};

  sprintf(value, "%d", restart_cause);
  if (kv_set("server_restart_cause", value, 0, KV_FPERSIST)) {
    return -1;
  }
  return 0;
}

int pal_set_ppin_info(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  char key[] = "server_cpu_ppin";
  char str[MAX_VALUE_LEN] = {0};
  int i;
  int completion_code = CC_UNSPECIFIED_ERROR;
  *res_len = 0;

  if (req_len > SIZE_CPU_PPIN * 2)
    req_len = SIZE_CPU_PPIN * 2;

  for (i = 0; i < req_len; i++) {
    sprintf(str + (i * 2), "%02x", req_data[i]);
  }

  if (kv_set(key, str, 0, KV_FPERSIST) != 0)
    return completion_code;

  completion_code = CC_SUCCESS;

  return completion_code;
}

// Read output from sysfs into buffer
int pal_read_sysfs(char* path, char* data, int data_size) {
  int fd;
  int ret;
  int bytes_read;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    ret = errno;
    close(fd);
    return ret;
  }

  bytes_read = read(fd, data, data_size);
  if (bytes_read == data_size) {
    ret = 0;
  } else {
    ret = -1;
  }

  close(fd);
  return ret;
}

// Power on/off/reset the server using wedge_power.sh
static int run_wedge_power(const char* action) {
  int ret;
  char cmd[MAX_WEDGE_POWER_CMD_SIZE] = {0};

  sprintf(cmd, "%s %s", WEDGE_POWER, action);
  ret = run_command(cmd);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: Failed to power %s server", __func__, action);
#endif
    return -1;
  }
  return 0;
}

int pal_get_server_power(uint8_t slot_id, uint8_t* status) {
  int ret;
  uint8_t retry = MAX_READ_RETRY;
  char value[MAX_VALUE_LEN];
  char sysfs_value[4] = {0};
  char path[LARGEST_DEVICE_NAME + 1];

  snprintf(path, LARGEST_DEVICE_NAME, SCMCPLD_PATH_FMT, CPU_CTRL);

  /* Check if the SCM is turned on or not */
  while (retry) {
    ret = pal_read_sysfs(path, sysfs_value, sizeof(sysfs_value) - 1);
    if (!ret)
      break;
    msleep(50);
    retry--;
  }

  if (ret) {
    // Check previous power state
    syslog(
        LOG_INFO,
        "pal_get_server_power: pal_read_sysfs returned error hence"
        " reading the kv_store for last power state for fru %d",
        slot_id);
    pal_get_last_pwr_state(slot_id, value);
    if (!(strncmp(value, "off", strlen("off")))) {
      *status = SERVER_POWER_OFF;
    } else if (!(strncmp(value, "on", strlen("on")))) {
      *status = SERVER_POWER_ON;
    } else {
      return ret;
    }

    return 0;
  }

  if (!strcmp(sysfs_value, ELBERT_SCM_PWR_ON)) {
    *status = SERVER_POWER_ON;
  } else {
    *status = SERVER_POWER_OFF;
  }

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  uint8_t status;

  if (pal_get_server_power(slot_id, &status) < 0) {
    return -1;
  }

  switch (cmd) {
     case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return run_wedge_power(WEDGE_POWER_ON);
      break;

     case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return run_wedge_power(WEDGE_POWER_OFF);
      break;

     case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (run_wedge_power(WEDGE_POWER_OFF))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return run_wedge_power(WEDGE_POWER_ON);
      } else if (status == SERVER_POWER_OFF) {
        return run_wedge_power(WEDGE_POWER_ON);
      }
      break;

     case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {
        if (run_wedge_power(WEDGE_POWER_RESET))
          return -1;
      } else if (status == SERVER_POWER_OFF) {
        syslog(
            LOG_CRIT,
            "Micro-server power status is off, "
            "ignore power reset action");
        return -2;
      }
      break;

     default:
      return -1;
  }

  return 0;
}

bool pal_is_fw_update_ongoing_system(void) {
  bool ret = false;
  uint8_t retry = MAX_READ_RETRY;
  char buf[PS_BUF_SIZE];
  FILE* fp;

  while (retry) {
    fp = popen(PS_CMD, "r");
    if (fp)
      break;
    msleep(50);
    retry--;
  }

  if (fp) {
    while (fgets(buf, PS_BUF_SIZE, fp) != NULL) {
      if (strstr(buf, ELBERT_BIOS_UTIL) || strstr(buf, ELBERT_FPGA_UTIL) ||
          strstr(buf, ELBERT_BMC_FLASH) || strstr(buf, ELBERT_PSU_UTIL)) {
        ret = true;
        break;
      }
    }
    pclose(fp);
  } else {
    // Assume we're updating if we can't be sure
    ret = true;
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: failed to get ps process output", __func__);
#endif
  }

  return ret;
}

// Used as part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG"
// 0xF4
int pal_get_poss_pcie_config(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x02; // Elbert
  uint8_t* data = res_data;

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
}

// For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int pal_get_plat_sku_id(void) {
  return 0x06; // Elbert
}

int pal_get_board_id(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  int board_sku_id = 0, board_rev = 0, ret = 0;
  unsigned char* data = res_data;
  struct wedge_eeprom_st eeprom;

  board_sku_id = pal_get_plat_sku_id();

  ret = elbert_eeprom_parse(SCM_TARGET, &eeprom);
  if (ret)
    return CC_NODE_BUSY;
  board_rev = eeprom.fbw_product_version;

  *data++ = board_sku_id;
  *data++ = board_rev;
  *data++ = slot;
  *data++ = 0x00; // 1S Server.
  *res_len = data - res_data;

  return CC_SUCCESS;
}

void pal_get_eth_intf_name(char* intf_name) {
  snprintf(intf_name, MAX_INTF_SIZE, ELBERT_INTF);
}

// GUID for System and Device
static int pal_set_guid(char* guid) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "bmc_guid");

  ret = kv_set(key, guid, GUID_SIZE, KV_FPERSIST);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_guid: pal_set_key_value failed");
#endif
  }
  return ret;
}

static int pal_get_guid(char* guid) {
  int ret;
  unsigned int size;
  char key[MAX_KEY_LEN];
  struct wedge_eeprom_st eeprom;
  uint8_t retry = MAX_READ_RETRY;

  sprintf(key, "%s", "bmc_guid");

  ret = kv_get(key, guid, &size, KV_FPERSIST);
  if (ret < 0 || size != GUID_SIZE) {
    // On Elbert, GUID is stored in kv rather than EEPROM, so GUID must be
    // generated from serial number when first accessed.
    while (retry) {
      ret = elbert_eeprom_parse(BMC_TARGET, &eeprom);
      if (!ret)
        break;
      msleep(50);
      retry--;
    }

    if (ret)
      return CC_NODE_BUSY;

    pal_populate_guid(guid, eeprom.fbw_product_serial);
    syslog(
        LOG_INFO,
        "pal_get_guid: generating GUID with UID %s",
        eeprom.fbw_product_serial);
    return pal_set_guid(guid);
  }

  return ret;
}

int pal_get_sys_guid(uint8_t fru, char* guid) {
  return pal_get_guid(guid);
}

int pal_set_sys_guid(uint8_t fru, char* str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(guid);
}

int pal_get_dev_guid(uint8_t fru, char* guid) {
  return pal_get_guid(guid);
}

int pal_set_dev_guid(uint8_t fru, char* str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(guid);
}

// Helper Functions
static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%i", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static bool
smb_is_p1(void) {
  if (access(ELBERT_SMB_P1_BOARD_PATH, F_OK) != -1)
    return true;
  else
    return false;
}

uint8_t
get_pim_i2cbus(uint8_t fru) {
  uint8_t i2cbus;
  uint8_t pimid = fru - FRU_PIM2;

  // P1 SMB detected, use pim_bus_p1 smbus channel mapping.
  if (smb_is_p1() == true)
    i2cbus = pim_bus_p1[pimid];
  else
    i2cbus = pim_bus[pimid];

  return i2cbus;
}

static int
pal_is_pim_prsnt(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char prsnt_path[LARGEST_DEVICE_NAME + 1];
  uint8_t i2cbus = get_pim_i2cbus(fru);

  // Check present bit and EEPROM.
  snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PIM_PRSNT);
  snprintf(prsnt_path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PIM2 + 2);
  if ((read_device(prsnt_path, &val) == 0 && val == 0x1) ||
       i2c_detect_device(i2cbus, 0x50) == 0) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

int
pal_is_pim_fpga_rev_valid(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char prsnt_path[LARGEST_DEVICE_NAME + 1];

  // Check present bit.
  snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PIM_FPGA_REV_MAJOR);
  snprintf(prsnt_path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PIM2 + 2);
  if (read_device(prsnt_path, &val) == 0 && val != 0xff) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

int
pal_is_pim_reset(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char prsnt_path[LARGEST_DEVICE_NAME + 1];

  // Check present bit.
  snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PIM_RESET);
  snprintf(prsnt_path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PIM2 + 2);
  if (read_device(prsnt_path, &val) == 0 && val == 0x0 ) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

int
pal_get_fru_count() {
  return MAX_NUM_FRUS;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_CHASSIS:
    case FRU_BMC:
    case FRU_SCM:
    case FRU_SMB:
    case FRU_SMB_EXTRA:
    case FRU_FAN:
      *status = 1;
      return 0;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      return pal_is_pim_prsnt(fru, status);
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PSU_PRSNT);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PSU1 + 1);
      break;
    default:
      return -1;
    }

  if (read_device(path, &val)) {
    return -1;
  }

  // Only consider present bit.
  if ((val & 0x1) == 0x0) {
    *status = 0;
  } else {
    *status = 1;
  }
  return 0;
}

int
pal_is_psu_power_ok(uint8_t fru, uint8_t *status) {
  const char *targets[] = { PSU_INPUT_OK, PSU_OUTPUT_OK };
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  int i, val;

  /* Check that both PSU input and output power are OK. */
  for (i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
    snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, targets[i]);
    snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PSU1 + 1);

    if (read_device(path, &val)) {
      syslog(LOG_ERR, "%s cannot get value from %s", __func__, path);
      return -1;
    }

    if (val == 0x0) {
      *status = 0;
      return 0;
    } else {
      *status = 1;
    }
  }

  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int val;
  int expected_val = 0x0;
  uint8_t expected_status = 0;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_CHASSIS:
    case FRU_BMC:
    case FRU_SCM:
    case FRU_SMB:
    case FRU_SMB_EXTRA:
    case FRU_FAN:
      *status = 1;
      return 0;
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
    case FRU_PIM9:
      return pal_is_pim_prsnt(fru, status);
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      /* Use PSU presence to determine readiness. This is because we want to
         be able to view sensor data even when PSU is not supplying power. */
      snprintf(tmp, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, PSU_PRSNT);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - FRU_PSU1 + 1);
      break;
    default:
      return -1;
    }

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == expected_val) {
    *status = expected_status;
  } else {
    *status = !expected_status;
  }
  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SMB:
      strcpy(name, "smb");
      break;
    case FRU_SCM:
      strcpy(name, "scm");
      break;
    case FRU_PIM2:
      strcpy(name, "pim2");
      break;
    case FRU_PIM3:
      strcpy(name, "pim3");
      break;
    case FRU_PIM4:
      strcpy(name, "pim4");
      break;
    case FRU_PIM5:
      strcpy(name, "pim5");
      break;
    case FRU_PIM6:
      strcpy(name, "pim6");
      break;
    case FRU_PIM7:
      strcpy(name, "pim7");
      break;
    case FRU_PIM8:
      strcpy(name, "pim8");
      break;
    case FRU_PIM9:
      strcpy(name, "pim9");
      break;
    case FRU_PSU1:
      strcpy(name, "psu1");
      break;
    case FRU_PSU2:
      strcpy(name, "psu2");
      break;
    case FRU_PSU3:
      strcpy(name, "psu3");
      break;
    case FRU_PSU4:
      strcpy(name, "psu4");
      break;
    case FRU_FAN:
      strcpy(name, "fan");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "smb")) {
    *fru = FRU_SMB;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else if (!strcmp(str, "scm")) {
    *fru = FRU_SCM;
  } else if (!strcmp(str, "pim2")) {
    *fru = FRU_PIM2;
  } else if (!strcmp(str, "pim3")) {
    *fru = FRU_PIM3;
  } else if (!strcmp(str, "pim4")) {
    *fru = FRU_PIM4;
  } else if (!strcmp(str, "pim5")) {
    *fru = FRU_PIM5;
  } else if (!strcmp(str, "pim6")) {
    *fru = FRU_PIM6;
  } else if (!strcmp(str, "pim7")) {
    *fru = FRU_PIM7;
  } else if (!strcmp(str, "pim8")) {
    *fru = FRU_PIM8;
  } else if (!strcmp(str, "pim9")) {
    *fru = FRU_PIM9;
  } else if (!strcmp(str, "psu1")) {
    *fru = FRU_PSU1;
  } else if (!strcmp(str, "psu2")) {
    *fru = FRU_PSU2;
  } else if (!strcmp(str, "psu3")) {
    *fru = FRU_PSU3;
  } else if (!strcmp(str, "psu4")) {
    *fru = FRU_PSU4;
  } else if (!strcmp(str, "fan")) {
    *fru = FRU_FAN;
  } else {
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }
  return 0;
}

int
pal_get_pim_type(uint8_t fru, int retry) {
  int ret = -1;
  char fru_name[16];
  struct wedge_eeprom_st eeprom;

  pal_get_fru_name(fru, fru_name);

  ret = elbert_eeprom_parse(fru_name, &eeprom);
  while (ret != 0 || !strstr(eeprom.fbw_product_asset, "88-") && retry--) {
    msleep(500);
    ret = elbert_eeprom_parse(fru_name, &eeprom);
  }

  if (ret) {
    return -1;
  }

  // Elbert uses product_asset instead of product_number for SKU field
  if (strstr(eeprom.fbw_product_asset, "88-16CD2")) {
    ret = PIM_TYPE_16Q2;
  } else if (strstr(eeprom.fbw_product_asset, "88-16CD")) {
    ret = PIM_TYPE_16Q;
  } else if (strstr(eeprom.fbw_product_asset, "88-8D")) {
    ret = PIM_TYPE_8DDM;
  } else {
    return -1;
  }

  return ret;
}

int
pal_set_pim_type_to_file(uint8_t fru, char *type) {
  char fru_name[16];
  char key[MAX_KEY_LEN];

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_type", fru_name);

  return kv_set(key, type, 0, 0);
}

int
pal_get_pim_type_from_file(uint8_t fru) {
  char fru_name[16];
  char key[MAX_KEY_LEN];
  char type[12] = {0};

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_type", fru_name);

  if (kv_get(key, type, NULL, 0)) {
    return -1;
  }

  if (!strncmp(type, "16q2", sizeof("16q2"))) {
    return PIM_TYPE_16Q2;
  } else if (!strncmp(type, "16q", sizeof("16q"))) {
    return PIM_TYPE_16Q;
  } else if (!strncmp(type, "8ddm", sizeof("8ddm"))) {
    return PIM_TYPE_8DDM;
  } else if (!strncmp(type, "unplug", sizeof("unplug"))) {
    return PIM_TYPE_UNPLUG;
  } else {
    return PIM_TYPE_NONE;
  }
}
