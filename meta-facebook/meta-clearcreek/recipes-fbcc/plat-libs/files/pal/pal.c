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
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

#define MB_BIN "/tmp/fruid_mb.bin"
#define MB_EEPROM "/sys/class/i2c-dev/i2c-6/device/6-0054/eeprom"
#define PDB_BIN "/tmp/fruid_pdb.bin"
#define PDB_EEPROM "/sys/class/i2c-dev/i2c-5/device/5-0054/eeprom"
#define BSM_BIN "/tmp/fruid_bsm.bin"
#define BSM_EEPROM "/sys/class/i2c-dev/i2c-23/device/23-0056/eeprom"
#define AVA1_BIN "/tmp/fruid_ava1.bin"
#define AVA1_EEPROM "/sys/class/i2c-dev/i2c-21/device/21-0050/eeprom"
#define AVA2_BIN "/tmp/fruid_ava2.bin"
#define AVA2_EEPROM "/sys/class/i2c-dev/i2c-22/device/22-0050/eeprom"
#define E1S1_BIN "/tmp/fruid_e1s1.bin"
#define E1S1_EEPROM "/sys/class/i2c-dev/i2c-21/device/21-0050/eeprom"
#define E1S2_BIN "/tmp/fruid_e1s2.bin"
#define E1S2_EEPROM "/sys/class/i2c-dev/i2c-22/device/22-0050/eeprom"
#define FIO_EEPROM "/sys/class/i2c-dev/i2c-8/device/8-0051/eeprom"
#define FIO_BIN "/tmp/fruid_fio.bin"

#define BMC_IPMB_SLAVE_ADDR 0x17

#define GUID_SIZE 16
#define OFFSET_DEV_GUID 0x1800

#define LAST_KEY "last_key"

const char pal_fru_list[] = "all, mb, bsm, pdb, carrier1, carrier2, fio";
const char pal_server_list[] = "";

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"identify_sled", "off", NULL},
  {"timestamp_sled", "0", NULL},
  {KEY_MB_SNR_HEALTH, "1", NULL},
  {KEY_MB_SEL_ERROR, "1", NULL},
  {KEY_PDB_SNR_HEALTH, "1", NULL},
  {KEY_CARRIER1_SNR_HEALTH, "1", NULL},
  {KEY_CARRIER2_SNR_HEALTH, "1", NULL},
  {"ntp_server", "", NULL},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

char g_dev_guid[GUID_SIZE] = {0};

int pal_check_carrier_type(int index)
{
  char type[64] = {0};

  if(index == 0) {
    if(kv_get("carrier_0", type, NULL, 0) < 0) {
      syslog(LOG_WARNING, "%s: not find carrier type", __func__);
    }
  } else if (index == 1) {
    if(kv_get("carrier_1", type, NULL, 0) < 0) {
      syslog(LOG_WARNING, "%s: not find carrier type", __func__);
    }
  }

  if (!strcmp(type, "m.2")) {
    return M2;
  } else if (!strcmp(type, "e1.s")) {
    return E1S;
  }
  return 0;
}

int pal_get_fru_id(char *str, uint8_t *fru)
{
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "mb") || !strcmp(str, "vr") ||
             !strcmp(str, "bmc") || !strcmp(str, "nic")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "bsm")) {
    *fru = FRU_BSM;
  } else if (!strcmp(str, "pdb")) {
    *fru = FRU_PDB;
  } else if (!strcmp(str, "carrier1")) {
    *fru = FRU_CARRIER1;
  } else if (!strcmp(str, "carrier2")) {
    *fru = FRU_CARRIER2;
  } else if (!strcmp(str, "fio")) {
    *fru = FRU_FIO;
  } else if (!strcmp(str, "nic0")) {
    *fru = FRU_NIC0;
  } else if (!strcmp(str, "nic1")) {
    *fru = FRU_NIC1;
  } else if (!strcmp(str, "nic2")) {
    *fru = FRU_NIC2;
  } else if (!strcmp(str, "nic3")) {
    *fru = FRU_NIC3;
  } else if (!strcmp(str, "nic4")) {
    *fru = FRU_NIC4;
  } else if (!strcmp(str, "nic5")) {
    *fru = FRU_NIC5;
  } else if (!strcmp(str, "nic6")) {
    *fru = FRU_NIC6;
  } else if (!strcmp(str, "nic7")) {
    *fru = FRU_NIC7;
  } else {
    syslog(LOG_WARNING, "%s: Wrong fru name %s", __func__, str);
    return -1;
  }

  return 0;
}

int pal_get_fruid_name(uint8_t fru, char *name)
{
  if (fru == FRU_MB)
    sprintf(name, "Base Board");
  else if (fru == FRU_BSM)
    sprintf(name, "BSM");
  else if (fru == FRU_PDB)
    sprintf(name, "PDB");
  else if (fru == FRU_CARRIER1) {
    if( pal_check_carrier_type(0) == M2 )
      sprintf(name, "M2 Carrier1");
    else
      sprintf(name, "E1.s Carrier1");
  }
  else if (fru == FRU_CARRIER2) {
    if( pal_check_carrier_type(1) == M2 )
      sprintf(name, "M2 Carrier2");
    else
      sprintf(name, "E1.s Carrier2");
  }
  else if (fru == FRU_FIO)
    sprintf(name, "Front IO");
  else
    return -1;

  return 0;
}

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status)
{
  int ret = -1;
  gpio_desc_t *desc = NULL;
  gpio_value_t value;

  switch (fru) {
    case FRU_MB:
    case FRU_PDB:
    case FRU_BSM:
    case FRU_CARRIER1:
    case FRU_CARRIER2:
    case FRU_FIO:
      *status = 1;
      return 0;
    case FRU_NIC0:
      desc = gpio_open_by_shadow("OCP_V3_0_PRSNTB_R_N");
      break;
    case FRU_NIC1:
      desc = gpio_open_by_shadow("OCP_V3_4_PRSNTB_R_N");
      break;
    case FRU_NIC2:
      desc = gpio_open_by_shadow("OCP_V3_1_PRSNTB_R_N");
      break;
    case FRU_NIC3:
      desc = gpio_open_by_shadow("OCP_V3_5_PRSNTB_R_N");
      break;
    case FRU_NIC4:
      desc = gpio_open_by_shadow("OCP_V3_2_PRSNTB_R_N");
      break;
    case FRU_NIC5:
      desc = gpio_open_by_shadow("OCP_V3_6_PRSNTB_R_N");
      break;
    case FRU_NIC6:
      desc = gpio_open_by_shadow("OCP_V3_3_PRSNTB_R_N");
      break;
    case FRU_NIC7:
      desc = gpio_open_by_shadow("OCP_V3_7_PRSNTB_R_N");
      break;
    default:
      return -1;
  }
  if (desc == NULL)
    return -1;
  if (gpio_get_value(desc, &value) == 0 && value == GPIO_VALUE_LOW) {
    *status = 1;
    ret = 0;
  }

  gpio_close(desc);
  return ret;
}

int pal_is_fru_ready(uint8_t fru, uint8_t *status)
{
  *status = 1;
  return 0;
}

int pal_get_fru_name(uint8_t fru, char *name)
{
  if (fru == FRU_MB) {
    strcpy(name, "mb");
  } else if (fru == FRU_BSM) {
    strcpy(name, "bsm");
  } else if (fru == FRU_PDB) {
    strcpy(name, "pdb");
  } else if (fru == FRU_NIC0) {
    strcpy(name, "nic0");
  } else if (fru == FRU_NIC1) {
    strcpy(name, "nic1");
  } else if (fru == FRU_NIC2) {
    strcpy(name, "nic2");
  } else if (fru == FRU_NIC3) {
    strcpy(name, "nic3");
  } else if (fru == FRU_NIC4) {
    strcpy(name, "nic4");
  } else if (fru == FRU_NIC5) {
    strcpy(name, "nic5");
  } else if (fru == FRU_NIC6) {
    strcpy(name, "nic6");
  } else if (fru == FRU_NIC7) {
    strcpy(name, "nic7");
  } else if (fru == FRU_CARRIER1) {
    strcpy(name, "carrier1");
  } else if (fru == FRU_CARRIER2) {
    strcpy(name, "carrier2");
  } else {
    syslog(LOG_WARNING, "%s: Wrong fruid %d", __func__, fru);
    return -1;
  }

  return 0;
}

int pal_get_fruid_path(uint8_t fru, char *path)
{
  if (fru == FRU_MB)
    sprintf(path, MB_BIN);
  else if (fru == FRU_PDB)
    sprintf(path, PDB_BIN);
  else if (fru == FRU_BSM)
    sprintf(path, BSM_BIN);
  else if (fru == FRU_CARRIER1) {
    if( pal_check_carrier_type(0) == M2 )
      sprintf(path, AVA1_BIN);
    else
      sprintf(path, E1S1_BIN);
  }
  else if (fru == FRU_CARRIER2) {
    if( pal_check_carrier_type(1) == M2 )
      sprintf(path, AVA2_BIN);
    else
      sprintf(path, E1S2_BIN);
  }
  else if (fru == FRU_FIO)
    sprintf(path, FIO_BIN);
  else
    return -1;

  return 0;
}

void pal_get_eth_intf_name(char* intf_name)
{
  snprintf(intf_name, 8, "usb0");
}

int pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{
  if (fru == FRU_MB)
    sprintf(path, MB_EEPROM);
  else if (fru == FRU_PDB)
    sprintf(path, PDB_EEPROM);
  else if (fru == FRU_BSM)
    sprintf(path, BSM_EEPROM);
  else if (fru == FRU_CARRIER1) {
    if( pal_check_carrier_type(0) == M2 )
      sprintf(path, AVA1_EEPROM);
    else
      sprintf(path, E1S1_EEPROM);
  }
  else if (fru == FRU_CARRIER2) {
    if( pal_check_carrier_type(1) == M2 )
      sprintf(path, AVA2_EEPROM);
    else
      sprintf(path, E1S2_EEPROM);
  }
  else if (fru == FRU_FIO)
    sprintf(path, FIO_EEPROM);
  else
    return -1;

  return 0;
}

int pal_get_fru_list(char *list)
{
  strcpy(list, pal_fru_list);
  return 0;
}

int pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  if (bus_id == 0) {
    *slave_addr = BMC_IPMB_SLAVE_ADDR;
  } else {
    *slave_addr = 0x10;
  }

  return 0;
}

int pal_set_usb_path(uint8_t slot, uint8_t endpoint)
{
  int ret = CC_SUCCESS;
  char gpio_name[16] = {0};
  gpio_desc_t *desc;
  gpio_value_t value;

  if (slot > SERVER_2 || endpoint > PCH) {
    ret = CC_PARAM_OUT_OF_RANGE;
    goto exit;
  }

  strcpy(gpio_name ,"USB2_SEL0");
  desc = gpio_open_by_shadow(gpio_name);
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = endpoint == PCH? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }
  gpio_close(desc);

  strcpy(gpio_name ,"USB2_SEL1");
  desc = gpio_open_by_shadow(gpio_name);
  if (!desc) {
    ret = CC_UNSPECIFIED_ERROR;
    goto exit;
  }
  value = slot%2 == 0? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  if (gpio_set_value(desc, value)) {
    ret = CC_UNSPECIFIED_ERROR;
    goto bail;
  }

bail:
  gpio_close(desc);
exit:
  return ret;
}

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

int
pal_is_bmc_por(void) {
  char por[MAX_VALUE_LEN] = {0};
  int rc;
 
  rc = kv_get("ast_por", por, NULL, 0);
  if (rc < 0) {
    syslog(LOG_DEBUG, "%s(): kv_get for ast_por failed", __func__);
    return -1;
  }

  if (!strcmp(por, "1"))
    return 1;
  else if (!strcmp(por, "0"))
    return 0;
  else {
    syslog(LOG_WARNING, "%s(): Cannot find corresponding ast_por", __func__);
    return -1;
  }

  return 0;
}

int pal_set_id_led(uint8_t status)
{
  int ret;
  int fd;
  char fn[32] = "/dev/i2c-8";
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t addr = 0xEE;

  fd = open(fn, O_RDWR);
  if(fd < 0) {
    syslog(LOG_WARNING,"[%s]Cannot open i2c bus", __func__);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  tbuf[0] = 0x03;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 1);

  if(ret < 0) {
    syslog(LOG_WARNING,"[%s] Cannot read i2c", __func__);
    goto error_exit;
  }

  tbuf[0] = 0x03;
  if(status)
    tbuf[1] = rbuf[0] | 0x40;
  else
    tbuf[1] = rbuf[0] & 0xBF;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 2, rbuf, 0);

  if(ret < 0) {
    syslog(LOG_WARNING,"[%s] Cannot write i2c", __func__);
    goto error_exit;
  }

error_exit:
  if (fd > 0){
    close(fd);
  }

  return ret;
}

int pal_set_def_key_value()
{
  int i;
  char key[MAX_KEY_LEN] = {0};

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

  /* Actions to be taken on Power On Reset */
  if (pal_is_bmc_por()) {
    /* Clear all the SEL errors */
    /* Write the value "1" which means FRU_STATUS_GOOD */
    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, KEY_MB_SEL_ERROR);
    pal_set_key_value(key, "1");

    /* Clear all the sensor health files*/
    /* Write the value "1" which means FRU_STATUS_GOOD */
    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, KEY_MB_SNR_HEALTH);
    pal_set_key_value(key, "1");

    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, KEY_PDB_SNR_HEALTH);
    pal_set_key_value(key, "1");

    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, KEY_CARRIER1_SNR_HEALTH);
    pal_set_key_value(key, "1");

    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, KEY_CARRIER2_SNR_HEALTH);
    pal_set_key_value(key, "1");

  }
  return 0;
}

int pal_get_sysfw_ver(uint8_t fru, uint8_t *ver)
{
  // No BIOS
  return -1;
}

void pal_dump_key_value(void)
{
  int ret;
  int i = 0;
  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_cfg[i].name, LAST_KEY)) {
    printf("%s:", key_cfg[i].name);
    if ((ret = kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST)) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int pal_slotid_to_fruid(int slotid)
{
  // FRU ID remapping, we start from FRU_MB = 1
  return slotid + 1;
}

int pal_get_platform_id(uint8_t *id)
{
  static bool cached = false;
  static unsigned int cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "BOARD_ID0",
      "BOARD_ID1",
      "BOARD_ID2"
    };
    if (gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return -1;
    }
    cached = true;
  }
  *id = (uint8_t)cached_id;
  return 0;
}

int
pal_get_board_rev_id(uint8_t *id) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "mb_rev");
  if (kv_get(key, value, NULL, 0)) {
    return false;
  }

  *id = atoi(value);

  return 0;
}

int pal_devnum_to_fruid(int devnum)
{
  // 1 = server, 2 = BMC ifself
  return devnum == 0? 2: devnum;
}

int
pal_control_mux_to_target_ch(uint8_t channel, uint8_t bus, uint8_t mux_addr)
{
  int ret;
  int fd;
  char fn[32];
  uint8_t retry;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if(fd < 0) {
    syslog(LOG_WARNING,"[%s]Cannot open bus %d", __func__, bus);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  tbuf[0] = channel;

  retry = MAX_READ_RETRY;
  while(retry > 0) {
    ret = i2c_rdwr_msg_transfer(fd, mux_addr, tbuf, 1, rbuf, 0);
    if (PAL_EOK == ret){
      break;
    }

    msleep(50);
    retry--;
  }

  if(ret < 0) {
    syslog(LOG_WARNING,"[%s] Cannot switch the mux on bus %d", __func__, bus);
    goto error_exit;
  }

error_exit:
  if (fd > 0){
    close(fd);
  }

  return ret;
}

// GUID for System and Device
static int pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;

  errno = 0;

  // Check if file is present
  if (access(MB_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_get_guid: unable to access the %s file: %s",
          MB_EEPROM, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(MB_EEPROM, O_RDONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_get_guid: unable to open the %s file: %s",
        MB_EEPROM, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read to %s file failed: %s",
        MB_EEPROM, strerror(errno));
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
  if (access(MB_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_set_guid: unable to access the %s file: %s",
          MB_EEPROM, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(MB_EEPROM, O_WRONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_set_guid: unable to open the %s file: %s",
        MB_EEPROM, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s file failed: %s",
        MB_EEPROM, strerror(errno));
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
