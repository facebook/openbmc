/*
 *
 * Copyright 2022-present Meta PLatfrom Inc. All Rights Reserved.
 *
 */

#include <ctype.h>
#include <errno.h>
#include <facebook/elbert_eeprom.h>
#include <facebook/wedge_eeprom.h>
#include <fcntl.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "cfg-parse.h"
#include "common-handles.h"
#include "plat.h"

const uint8_t pim_bus[] = {16, 17, 18, 19, 20, 21, 22, 23};
const uint8_t pim_bus_p1[] = {16, 17, 18, 23, 20, 21, 22, 19};

static bool smb_is_p1(void) {
  if (access(ELBERT_SMB_P1_BOARD_PATH, F_OK) != -1)
    return true;
  else
    return false;
}

static uint8_t get_pim_i2cbus(uint8_t fru) {
  uint8_t i2cbus;
  uint8_t pimid = fru - FRU_PIM2;

  // P1 SMB detected, use pim_bus_p1 smbus channel mapping.
  if (smb_is_p1() == true)
    i2cbus = pim_bus_p1[pimid];
  else
    i2cbus = pim_bus[pimid];

  return i2cbus;
}

static int read_device(const char* device, int* value) {
  FILE* fp;
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

static int is_pim_prsnt(uint8_t fru, uint8_t* status) {
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

// Power Off, Power On, or Power Reset the server in given slot
int elbert_set_server_power(uint8_t slot_id, uint8_t cmd) {
  uint8_t status;

  if (ipmi_get_server_power(slot_id, &status) < 0) {
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

bool elbert_is_fw_update_ongoing_system(void) {
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

//  the eeprom format is  differebnt from wedge400
static int plat_get_guid(uint8_t fru, char* guid) {
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

    populate_guid(guid, eeprom.fbw_product_serial);
    syslog(
        LOG_INFO,
        "plat_get_guid: generating GUID with UID %s",
        eeprom.fbw_product_serial);
    return set_guid(guid);
  }
  return ret;
}

int elbert_get_plat_guid(uint8_t fru, char* guid) {
  return plat_get_guid(fru, guid);
}

int elbert_is_fru_prsnt(uint8_t fru, uint8_t* status) {
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
      return is_pim_prsnt(fru, status);
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

int elbert_get_board_id(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  int board_sku_id = 0, board_rev = 0, ret = 0;
  unsigned char* data = res_data;
  struct wedge_eeprom_st eeprom;

  get_cfg_item_num("platform_board_id", &board_sku_id);

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

int elbert_get_fru_name(uint8_t fru, char* name) {
  switch (fru) {
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

int elbert_set_plat_guid(uint8_t fru, char* str) {
  char guid[GUID_SIZE] = {0};

  populate_guid(guid, str);
  return set_guid(guid);
}

void set_plat_funcs() {
  g_fv->get_plat_guid = elbert_get_plat_guid;
  g_fv->set_plat_guid = elbert_set_plat_guid;
  g_fv->is_fru_prsnt = elbert_is_fru_prsnt;
  g_fv->get_board_id = elbert_get_board_id;
  g_fv->get_fru_name = elbert_get_fru_name;
  g_fv->set_server_power = elbert_set_server_power;
  g_fv->is_fw_update_ongoing_system = elbert_is_fw_update_ongoing_system;
}
