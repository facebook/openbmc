/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/nm.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

#define PLATFORM_NAME "sonorapass"
#define LAST_KEY "last_key"

#define REV_ID_FILE "/tmp/mb_rev"
#define SKU_ID_FILE "/tmp/mb_sku"

#define GPIO_LOCATE_LED "FP_LOCATE_LED"
#define GPIO_FAULT_LED "FP_FAULT_LED_N"
#define GPIO_NIC0_PRSNT "HP_LVC3_OCP_V3_1_PRSNT2_N"
#define GPIO_NIC1_PRSNT "HP_LVC3_OCP_V3_2_PRSNT2_N"

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

const char pal_fru_list[] = "all, mb, nic0, nic1, riser1, riser2, fcb, bmc";
const char pal_server_list[] = "mb";
const char pal_fru_list_print[] =  "all, mb, nic0, nic1, riser1, riser2, bmc";  // Cannot read fruid from "fcb"
const char pal_fru_list_rw[] = "mb, nic0, nic1, riser1, riser2, bmc";  // Cannot write fruid to "fcb"
const char pal_fru_list_sensor_history[] = "all, mb, nic0, nic1, riser1, riser2, fcb, bmc";

static int key_func_por_policy (int event, void *arg);
static int key_func_lps (int event, void *arg);
static bool is_cpu_socket_occupy(unsigned int cpu_id);

static char *dimm_label[24] = {
  "A0", "C0", "A1", "C1", "A2", "C2", "A3", "C3", "A4", "C4", "A5", "C5",
  "B0", "D0", "B1", "D1", "B2", "D2", "B3", "D3", "B4", "D4", "B5", "D5"};
struct dimm_map {
  unsigned char index;
  char *label;
};

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
  {"pwr_server_last_state", "on", key_func_lps},
  {"sysfw_ver_server", "0", NULL},
  {"identify_sled", "off", NULL},
  {"timestamp_sled", "0", NULL},
  {"server_por_cfg", "lps", key_func_por_policy},
  {"server_sensor_health", "1", NULL},
  {"nic0_sensor_health", "1", NULL},
  {"nic1_sensor_health", "1", NULL},
  {"server_sel_error", "1", NULL},
  {"server_boot_order", "0100090203ff", NULL},
  {"ntp_server", "", NULL},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

struct fsc_monitor
{
  uint8_t sensor_num;
  char *sensor_name;
  bool (*check_sensor_sts)(uint8_t);
  bool is_alive;
  uint8_t init_count;
  uint8_t retry;
};

static struct fsc_monitor fsc_monitor_basic_snr_list[] =
{
  {MB_SNR_INLET_TEMP            , "mb_inlet_temp"  , NULL, false, 5, 5},
  {MB_SNR_CPU0_THERM_MARGIN     , "mb_cpu0_therm_margin"     , NULL, false, 5, 5},
  {MB_SNR_CPU1_THERM_MARGIN     , "mb_cpu1_therm_margin"     , NULL, false, 5, 5},
  {MB_SNR_DATA0_DRIVER_TEMP     , "mb_data0_driver_temp"  , NULL, false, 5, 5},
  {MB_SNR_DATA1_DRIVER_TEMP     , "mb_data1_driver_temp"  , NULL, false, 5, 5},
  {NIC_MEZZ0_SNR_TEMP           , "nic_mezz0_temp"  , NULL, false, 5, 5},
  {NIC_MEZZ1_SNR_TEMP           , "nic_mezz1_temp"  , NULL, false, 5, 5},
  //dimm sensors wait for 240s. 240=80*3(fsc monitor interval)
  {MB_SNR_CPU0_DIMM_GRPA_TEMP, "mb_cpu0_dimm_a_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPB_TEMP, "mb_cpu0_dimm_b_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPC_TEMP, "mb_cpu0_dimm_c_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPD_TEMP, "mb_cpu0_dimm_d_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPE_TEMP, "mb_cpu0_dimm_e_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU0_DIMM_GRPF_TEMP, "mb_cpu0_dimm_f_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPA_TEMP, "mb_cpu1_dimm_a_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPB_TEMP, "mb_cpu1_dimm_b_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPC_TEMP, "mb_cpu1_dimm_c_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPD_TEMP, "mb_cpu1_dimm_d_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPE_TEMP, "mb_cpu1_dimm_e_temp", pal_dimm_present_check, false, 80, 5},
  {MB_SNR_CPU1_DIMM_GRPF_TEMP, "mb_cpu1_dimm_f_temp", pal_dimm_present_check, false, 80, 5},
};

static int fsc_monitor_basic_snr_list_size = sizeof(fsc_monitor_basic_snr_list) / sizeof(struct fsc_monitor);

static int
pal_key_index(char *key) {

  int i;

  i = 0;
  while(strcmp(key_cfg[i].name, LAST_KEY)) {

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

int
pal_get_key_value(char *key, char *value) {
  int index;

  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  return kv_get(key, value, NULL, KV_FPERSIST);
}

int
pal_set_key_value(char *key, char *value) {
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

static int
fw_getenv(char *key, char *value)
{
  char cmd[MAX_KEY_LEN + 32] = {0};
  char *p;
  FILE *fp;

  sprintf(cmd, "/sbin/fw_printenv -n %s", key);
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }
  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }
  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }
  pclose(fp);
  return 0;
}

static int
fw_setenv(char *key, char *value) {
  char old_value[MAX_VALUE_LEN] = {0};
  if (fw_getenv(key, old_value) != 0 ||
      strcmp(old_value, value) != 0) {
    /* Set the env key:value if either the key
     * does not exist or the value is different from
     * what we want set */
    char cmd[MAX_VALUE_LEN] = {0};
    snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv %s %s", key, value);
    return system(cmd);
  }
  return 0;
}

static int
key_func_por_policy(int event, void *arg) {
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  switch (event) {
    case KEY_BEFORE_SET:
      if (strcmp((char *)arg, "lps") && strcmp((char *)arg, "on") && strcmp((char *)arg, "off"))
        return -1;

      if (pal_is_fw_update_ongoing(FRU_BMC)) {
        syslog(LOG_WARNING, "key_func_por_policy: cannot setenv por_policy=%s", (char *)arg);
        break;
      }

      ret = fw_setenv("por_policy", (char *)arg);
      break;
    case KEY_AFTER_INI:
      kv_get("server_por_cfg", value, NULL, KV_FPERSIST);
      ret = fw_setenv("por_policy", value);
      break;
  }

  return ret;
}

static int
key_func_lps(int event, void *arg)
{
  char value[MAX_VALUE_LEN] = {0};

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_BMC)) {
        syslog(LOG_WARNING, "key_func_lps: cannot setenv por_ls=%s", (char *)arg);
        break;
      }
      fw_setenv("por_ls", (char *)arg);
      break;
    case KEY_AFTER_INI:
      kv_get("pwr_server_last_state", value, NULL, KV_FPERSIST);
      fw_setenv("por_ls", value);
      break;
  }

  return 0;
}

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
    syslog(LOG_INFO, "failed to open device %s", device);
    return err;
  }

  rc = fscanf(fp, "%d", value);

  fclose(fp);
  if (rc != 1) {
    syslog(LOG_INFO, "failed to read device %s", device);
    return ENOENT;
  } else {
    return 0;
  }
}

int
pal_is_bmc_por(void) {
  FILE *fp;
  int por = 0;

  fp = fopen("/tmp/ast_por", "r");
  if (fp != NULL) {
    if (fscanf(fp, "%d", &por) != 1) {
      por = 0;
    }
    fclose(fp);
  }

  return (por)?1:0;
}

int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = 1;
  return 0;
}

int
pal_get_platform_id(uint8_t id_type, uint8_t *id) {
  int ret = 0, retry = 3, val;
  uint8_t *id_cache = NULL;
  char *id_file = NULL;
  static uint8_t rev_id = 0xFF, sku_id = 0xFF;

  switch (id_type) {
    case BOARD_REV_ID:
      id_cache = &rev_id;
      id_file = REV_ID_FILE;
      break;
    case BOARD_SKU_ID:
      id_cache = &sku_id;
      id_file = SKU_ID_FILE;
      break;
    default:
      return -1;
  }

  if (*id_cache != 0xFF) {
    *id = *id_cache;
  } else {
    do {
      ret = read_device(id_file, &val);
      if (!ret) {
        *id_cache = (uint8_t)val;
        *id = *id_cache;
        break;
      }
      syslog(LOG_WARNING, "pal_get_platform_id failed, id_type: %u", id_type);
      msleep(10);
    } while (--retry);
  }

  return ret;
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  int ret;
  uint8_t platform_id  = 0x00;
  uint8_t board_rev_id = 0x00;
  int completion_code=CC_UNSPECIFIED_ERROR;

  ret = pal_get_platform_id(BOARD_SKU_ID, &platform_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  ret = pal_get_platform_id(BOARD_REV_ID, &board_rev_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  // Prepare response buffer
  completion_code = CC_SUCCESS;
  res_data[0] = platform_id;
  res_data[1] = board_rev_id;
  *res_len = 0x02;

  return completion_code;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
	gpio_desc_t *gdesc = NULL;
	gpio_value_t val;

  *status = 0;

  switch (fru) {
    case FRU_MB:
      *status = 1;
      break;
    case FRU_NIC0:
      if ((gdesc = gpio_open_by_shadow(GPIO_NIC0_PRSNT))) {
        if (!gpio_get_value(gdesc, &val)) {
          *status = !val;
        }
        gpio_close(gdesc);
      }
      break;
    case FRU_NIC1:
      if ((gdesc = gpio_open_by_shadow(GPIO_NIC1_PRSNT))) {
        if (!gpio_get_value(gdesc, &val)) {
          *status = !val;
        }
        gpio_close(gdesc);
      }
      break;
    case FRU_RISER1:
      *status = 1;
      break;
    case FRU_RISER2:
      *status = 1;
      break;
    case FRU_BMC:
      *status = 1;
      break;
    case FRU_FCB:
      *status = 1;
      break;
    default:
      return -1;
    }

  return 0;
}

int
pal_is_slot_server(uint8_t fru) {
  if (fru == FRU_MB)
    return 1;
  return 0;
}

// Update the Identification LED for the given fru with the status
int
pal_set_id_led(uint8_t fru, uint8_t status) {
  int ret;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;

  if (fru != FRU_MB)
    return -1;

  gdesc = gpio_open_by_shadow(GPIO_LOCATE_LED);
  if (gdesc == NULL)
    return -1;

  val = status? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  ret = gpio_set_value(gdesc, val);
  if (ret != 0)
    goto error;

error:
  gpio_close(gdesc);
  return ret;
}

int
pal_set_fault_led(uint8_t fru, uint8_t status) {
  int ret;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;

  if (fru != FRU_MB)
    return -1;

  gdesc = gpio_open_by_shadow(GPIO_FAULT_LED);
  if (gdesc == NULL)
    return -1;

  val = status? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  ret = gpio_set_value(gdesc, val);
  if (ret != 0)
    goto error;

  error:
    gpio_close(gdesc);
    return ret;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "mb")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "nic0") || !strcmp(str, "nic")) {
    *fru = FRU_NIC0;
  } else if (!strcmp(str, "nic1")) {
    *fru = FRU_NIC1;
  } else if (!strcmp(str, "riser1")) {
    *fru = FRU_RISER1;
  } else if (!strcmp(str, "riser2")) {
    *fru = FRU_RISER2;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else if (!strcmp(str, "fcb")) {
    *fru = FRU_FCB;
  } else {
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch (fru) {
    case FRU_MB:
      strcpy(name, "mb");
      break;
    case FRU_NIC0:
      strcpy(name, "nic0");
      break;
    case FRU_NIC1:
      strcpy(name, "nic1");
      break;
    case FRU_RISER1:
      strcpy(name, "riser1");
      break;
    case FRU_RISER2:
      strcpy(name, "riser2");
      break;
    case FRU_BMC:
      strcpy(name, "bmc");
      break;
    case FRU_FCB:
      strcpy(name, "fcb");
      break;
    default:
      syslog(LOG_WARNING, "[%s] unknown fruid %d", __func__, fru);
      return -1;
  }

  return 0;
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};

  switch(fru) {
  case FRU_ALL: //no bin for FRU_ALL, when fru id is 0 the default is set to FRU_MB
  case FRU_MB:
    sprintf(fname, "mb");
    break;
  case FRU_NIC0:
    sprintf(fname, "nic0");
    break;
  case FRU_NIC1:
    sprintf(fname, "nic1");
    break;
  case FRU_RISER1:
    sprintf(fname, "riser1");
    break;
  case FRU_RISER2:
    sprintf(fname, "riser2");
    break;
  case FRU_BMC:
    sprintf(fname, "bmc");
    break;
  default:
    return -1;
  }

  sprintf(path, "/tmp/fruid_%s.bin", fname);
  return 0;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  uint8_t rev_id = 0xFF;

  pal_get_platform_id(BOARD_REV_ID, &rev_id);

  switch(fru) {
  case FRU_MB:
    if (rev_id == PLATFORM_EVT) {
      sprintf(path, FRU_EEPROM_MB_EVT);
    }
    else if (rev_id == PLATFORM_DVT) {
      sprintf(path, FRU_EEPROM_MB_DVT);
    }
    break;
  case FRU_NIC0:
    sprintf(path, FRU_EEPROM_NIC0);
    break;
  case FRU_NIC1:
    sprintf(path, FRU_EEPROM_NIC1);
    break;
  case FRU_RISER1:
    sprintf(path, FRU_EEPROM_RISER1);
    break;
  case FRU_RISER2:
    sprintf(path, FRU_EEPROM_RISER2);
    break;
  case FRU_BMC:
    sprintf(path, FRU_EEPROM_BMC);
    break;
  default:
    return -1;
  }

  return 0;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  switch(fru) {
  case FRU_MB:
    sprintf(name, "Mother Board");
    break;
  case FRU_NIC0:
    sprintf(name, "Mezz Card 0");
    break;
  case FRU_NIC1:
    sprintf(name, "Mezz Card 1");
    break;
  case FRU_RISER1:
    sprintf(name, "Riser Card 1");
    break;
  case FRU_RISER2:
    sprintf(name, "Riser Card 2");
    break;
  case FRU_BMC:
    sprintf(name, "BMC");
    break;
  case FRU_FCB:
    sprintf(name, "FCB");
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd;
  ssize_t bytes_rd;
  char path[128];
  uint8_t rev_id = 0xFF;

  errno = 0;

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  if (rev_id == PLATFORM_EVT) {
    sprintf(path, FRU_EEPROM_MB_EVT);
  }
  else if (rev_id == PLATFORM_DVT) {
    sprintf(path, FRU_EEPROM_MB_DVT);
  }

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "pal_get_guid: unable to access %s: %s", path, strerror(errno));
    return errno;
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "pal_get_guid: unable to open %s: %s", path, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read from %s failed: %s", path, strerror(errno));
  }

  close(fd);
  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd;
  ssize_t bytes_wr;
  char path[128];
  uint8_t rev_id = 0xFF;

  errno = 0;

  pal_get_platform_id(BOARD_REV_ID, &rev_id);
  if (rev_id == PLATFORM_EVT) {
    sprintf(path, FRU_EEPROM_MB_EVT);
  }
  else if (rev_id == PLATFORM_DVT) {
    sprintf(path, FRU_EEPROM_MB_DVT);
  }

  // check for file presence
  if (access(path, F_OK)) {
    syslog(LOG_ERR, "pal_set_guid: unable to access %s: %s", path, strerror(errno));
    return errno;
  }

  fd = open(path, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "pal_set_guid: unable to open %s: %s", path, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s failed: %s", path, strerror(errno));
  }

  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(char *guid, char *str) {
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
  srand(time(NULL));
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

  return;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_SYS_GUID, guid);
  return 0;
}

int
pal_set_def_key_value() {
  int i;
  char key[MAX_KEY_LEN] = {0};

  for(i = 0; strcmp(key_cfg[i].name, LAST_KEY) != 0; i++) {
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
    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, "server_sel_error");

    /* Write the value "1" which means FRU_STATUS_GOOD */
    pal_set_key_value(key, "1");

    /* Clear all the sensor health files*/
    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, "server_sensor_health");

    /* Write the value "1" which means FRU_STATUS_GOOD */
    pal_set_key_value(key, "1");
  }

  return 0;
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(OFFSET_SYS_GUID, guid);
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_DEV_GUID, guid);
  return 0;
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char guid[GUID_SIZE] = {0};

  pal_populate_guid(guid, str);
  return pal_set_guid(OFFSET_DEV_GUID, guid);
}

int
pal_devnum_to_fruid(int devnum) {
  return FRU_MB;
}

int
pal_channel_to_bus(int channel) {
  switch (channel) {
    case IPMI_CHANNEL_0:
      return I2C_BUS_0; // USB (LCD Debug Board)

    case IPMI_CHANNEL_6:
      return I2C_BUS_5; // ME
  }

  // Debug purpose, map to real bus number
  if (channel & 0x80) {
    return (channel & 0x7f);
  }

  return channel;
}

bool
pal_is_fw_update_ongoing_system(void) {
  uint8_t i;

  for (i = FRU_MB; i <= FRU_BMC; i++) {
    if (pal_is_fw_update_ongoing(i) == true)
      return true;
  }

  return false;
}

bool
pal_check_boot_device_is_vaild(uint8_t device) {
  bool vaild = false;

  switch (device)
  {
    case BOOT_DEVICE_USB:
    case BOOT_DEVICE_IPV4:
    case BOOT_DEVICE_HDD:
    case BOOT_DEVICE_CDROM:
    case BOOT_DEVICE_OTHERS:
    case BOOT_DEVICE_IPV6:
    case BOOT_DEVICE_RESERVED:
      vaild = true;
      break; 
    default:
      break;
  }

  return vaild;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i, j, network_dev = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  *res_len = 0;
  sprintf(key, "server_boot_order");

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    //Byte 0 is boot mode, Byte 1~5 is boot order
    if ((i > 0) && (boot[i] != 0xFF)) {
      if(!pal_check_boot_device_is_vaild(boot[i]))
        return CC_INVALID_PARAM;
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        if ( boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      //If Bit 2:0 is 001b (Network), Bit3 is IPv4/IPv6 order
      //Bit3=0b: IPv4 first
      //Bit3=1b: IPv6 first
      if ( boot[i] == BOOT_DEVICE_IPV4 || boot[i] == BOOT_DEVICE_IPV6)
        network_dev++;
    }

    snprintf(tstr, 3, "%02x", boot[i]);
    strncat(str, tstr, 3);
  }

  //Not allow having more than 1 network boot device in the boot order.
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return pal_set_key_value(key, str);
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "server_boot_order");

  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;
  return 0;
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value("sysfw_ver_server", str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int ret;
  int i, j;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};

  ret = pal_get_key_value("sysfw_ver_server", str);
  if (ret) {
    return ret;
  }

  for (i = 0, j = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c%c", str[i], str[i+1]);
    ver[j++] = strtol(tstr, NULL, 16);
  }
  return 0;
}

// Get ME Firmware Version
int
pal_get_me_fw_ver(uint8_t bus, uint8_t addr, uint8_t *ver) {
  ipmi_dev_id_t dev_id;
  NM_RW_INFO info;
  int ret;

  info.bus = bus;
  info.nm_addr = addr;
  ret = pal_get_bmc_ipmb_slave_addr(&info.bmc_addr, info.bus);
  if (ret != 0) {
    return ret;
  }

  ret = cmd_NM_get_dev_id(&info, &dev_id);
  if (ret != 0) {
    return ret;
  }

  /*
    Major version number: byte 4[6:0]
    Minor version number: high 4 bits of byte 5
    Milestone version number: low 4 bits of byte 5
    Build version number: byte 14 and byte 15

    dev_id.fw_rev1 = byte 4
    dev_id.fw_rev2 = byte 5
    dev_id.aux_fw_rev[1] = byte 14
    dev_id.aux_fw_rev[2] = byte 15
  */
  ver[0] = dev_id.fw_rev1 & 0x7F ;
  ver[1] = dev_id.fw_rev2 >> 4;
  ver[2] = dev_id.fw_rev2 & 0x0F;
  ver[3] = dev_id.aux_fw_rev[1];
  ver[4] = dev_id.aux_fw_rev[2] >> 4;
  ver[5] = dev_id.aux_fw_rev[2] & 0x0F;

  return ret;
}

static int
get_gpio_shadow_array(const char **shadows, int num, uint8_t *mask) {
  int i;
  *mask = 0;

  for (i = 0; i < num; i++) {
    int ret;
    gpio_value_t value;
    gpio_desc_t *gpio = gpio_open_by_shadow(shadows[i]);
    if (!gpio) {
      return -1;
    }

    ret = gpio_get_value(gpio, &value);
    gpio_close(gpio);

    if (ret != 0) {
      return -1;
    }
    *mask |= (value == GPIO_VALUE_HIGH ? 1 : 0) << i;
  }
  return 0;
}

int
pal_uart_select (uint32_t base, uint8_t offset, int option, uint32_t para) {
  uint32_t mmap_fd;
  uint32_t ctrl;
  void *reg_base;
  void *reg_offset;

  mmap_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (mmap_fd < 0) {
    return -1;
  }

  reg_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mmap_fd, base);
  reg_offset = (char*) reg_base + offset;
  ctrl = *(volatile uint32_t*) reg_offset;

  switch(option) {
    case UARTSW_BY_BMC:                //UART Switch control by bmc
      ctrl &= 0x00ffffff;
      break;
    case UARTSW_BY_DEBUG:           //UART Switch control by debug card
      ctrl |= 0x01000000;
      break;
    case SET_SEVEN_SEGMENT:      //set channel on the seven segment display
      ctrl &= 0x00ffffff;
      ctrl |= para;
      break;
    default:
      syslog(LOG_WARNING, "pal_mmap: unknown option");
      break;
  }
  *(volatile uint32_t*) reg_offset = ctrl;

  munmap(reg_base, PAGE_SIZE);
  close(mmap_fd);

  return 0;
}

int
pal_uart_select_led_set(void) {
  static uint32_t pre_channel = 0xffffffff;
  uint8_t vals;
  uint32_t channel = 0;
  const char *shadows[] = {
    "FM_UARTSW_LSB_N",
    "FM_UARTSW_MSB_N"
  };

  //UART Switch control by bmc
  pal_uart_select(AST_GPIO_BASE, UARTSW_OFFSET, UARTSW_BY_BMC, 0);

  if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &vals)) {
    return -1;
  }
  // The GPIOs are active-low. So, invert it.
  channel = (uint32_t)(~vals & 0x3);
  // Shift to get to the bit position of the led.
  channel = channel << 24;

  // If the requested channel is the same as the previous, do nothing.
  if (channel == pre_channel) {
     return -1;
  }
  pre_channel = channel;

  //show channel on 7-segment display
  pal_uart_select(AST_GPIO_BASE, SEVEN_SEGMENT_OFFSET, SET_SEVEN_SEGMENT, channel);
  return 0;
}

int
parse_mem_error_sel(uint8_t fru, uint8_t snr_num, uint8_t *event_data, char *error_log) {
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};
  uint8_t sen_type = event_data[0];
  uint8_t chn_num, dimm_num;

  if (snr_num == MEMORY_ECC_ERR) {
    // SEL from MEMORY_ECC_ERR Sensor
    if ((ed[0] & 0x0F) == 0x0) {
      if (sen_type == 0x0C) {
        strcat(error_log, "Correctable");
        snprintf(temp_log, sizeof(temp_log), "DIMM%02X ECC err,FRU:%u", ed[2], fru);
        pal_add_cri_sel(temp_log);
      } else if (sen_type == 0x10)
        strcat(error_log, "Correctable ECC error Logging Disabled");
    } else if ((ed[0] & 0x0F) == 0x1) {
        strcat(error_log, "Uncorrectable");
        snprintf(temp_log, sizeof(temp_log), "DIMM%02X UECC err,FRU:%u", ed[2], fru);
        pal_add_cri_sel(temp_log);
    } else if ((ed[0] & 0x0F) == 0x5)
        strcat(error_log, "Correctable ECC error Logging Limit Reached");
      else
        strcat(error_log, "Unknown");
  } else if (snr_num == MEMORY_ERR_LOG_DIS) {
      // SEL from MEMORY_ERR_LOG_DIS Sensor
    if ((ed[0] & 0x0F) == 0x0)
      strcat(error_log, "Correctable Memory Error Logging Disabled");
    else
      strcat(error_log, "Unknown");
  }

  // Common routine for both MEM_ECC_ERR and MEMORY_ERR_LOG_DIS
  chn_num = (ed[2] & 0x1C) >> 2;
  bool support_mem_mapping = false;
  char mem_mapping_string[32];
  pal_parse_mem_mapping_string(chn_num, &support_mem_mapping, mem_mapping_string);
  if(support_mem_mapping) {
    snprintf(temp_log, sizeof(temp_log), " (DIMM %s)", mem_mapping_string);
  } else {
    snprintf(temp_log, sizeof(temp_log), " (DIMM %02X)", ed[2]);
  }
  strcat(error_log, temp_log);

  snprintf(temp_log, sizeof(temp_log), " Logical Rank %d", ed[1] & 0x03);
  strcat(error_log, temp_log);

  // DIMM number (ed[2]):
  // Bit[7:5]: Socket number  (Range: 0-7)
  // Bit[4:2]: Channel number (Range: 0-7)
  // Bit[1:0]: DIMM number    (Range: 0-3)
  if (((ed[1] & 0xC) >> 2) == 0x0) {
    /* All Info Valid */
    chn_num = (ed[2] & 0x1C) >> 2;
    dimm_num = ed[2] & 0x3;

    /* If critical SEL logging is available, do it */
    if (sen_type == 0x0C) {
      if ((ed[0] & 0x0F) == 0x0) {
        snprintf(temp_log, sizeof(temp_log), "DIMM%c%d ECC err,FRU:%u", 'A'+chn_num,
                dimm_num, fru);
        pal_add_cri_sel(temp_log);
      } else if ((ed[0] & 0x0F) == 0x1) {
        snprintf(temp_log, sizeof(temp_log), "DIMM%c%d UECC err,FRU:%u", 'A'+chn_num,
                dimm_num, fru);
        pal_add_cri_sel(temp_log);
      }
    }
      /* Then continue parse the error into a string. */
      /* All Info Valid                               */
    sprintf(temp_log, " (DIMM %02X) Logical Rank %d", ed[2], ed[1] & 0x03);
  } else if (((ed[1] & 0xC) >> 2) == 0x1) {
    /* DIMM info not valid */
    snprintf(temp_log, sizeof(temp_log), " (CPU# %d, CHN# %d)",
        (ed[2] & 0xE0) >> 5, (ed[2] & 0x1C) >> 2);
  } else if (((ed[1] & 0xC) >> 2) == 0x2) {
    /* CHN info not valid */
    snprintf(temp_log, sizeof(temp_log), " (CPU# %d, DIMM# %d)",
        (ed[2] & 0xE0) >> 5, ed[2] & 0x3);
  } else if (((ed[1] & 0xC) >> 2) == 0x3) {
    /* CPU info not valid */
    snprintf(temp_log, sizeof(temp_log), " (CHN# %d, DIMM# %d)",
        (ed[2] & 0x1C) >> 2, ed[2] & 0x3);
  }
  strcat(error_log, temp_log);
  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log) {
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  bool parsed = false;
  error_log[0] = '\0';

  switch(snr_num) {
    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      parse_mem_error_sel(fru, snr_num, event_data, error_log);
      parsed = true;
      break;
  }

  if (parsed == true) {
    if ((event_data[2] & 0x80) == 0) {
      strcat(error_log, " Assertion");
    } else {
      strcat(error_log, " Deassertion");
    }
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;
}

void
pal_dump_key_value(void) {
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

// OEM Command "CMD_OEM_BYPASS_CMD" 0x34
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len){
  int ret;
  int completion_code=CC_UNSPECIFIED_ERROR;
  uint8_t cmd, select;
  uint8_t tlen;
  uint8_t status;
  NCSI_NL_MSG_T *msg = NULL;
  NCSI_NL_RSP_T *rsp = NULL;
  uint8_t channel = 0;
  uint8_t netdev = 0;
  int i;

  *res_len = 0;

  ret = pal_is_fru_prsnt(slot, &status);
  if (ret < 0) {
    return -1;
  }
  if (status == 0) {
    syslog(LOG_ERR, "%s pal_is_fru_prsnt status == 0", __func__);
    return CC_UNSPECIFIED_ERROR;
  }

  ret = pal_get_server_power(FRU_MB, &status);
  if(ret < 0 || 0 == status) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  if(!pal_is_slot_server(slot)) {
    syslog(LOG_ERR, "%s pal_is_slot_server false", __func__);
    return CC_UNSPECIFIED_ERROR;
  }

  select = req_data[0];

  switch (select) {
    case BYPASS_NCSI:
      tlen = req_len - 7; // payload_id, netfn, cmd, data[0] (select), netdev, channel, cmd
      if (tlen < 0) {
        completion_code = CC_INVALID_LENGTH;
        break;
      }

      netdev = req_data[1];
      channel = req_data[2];
      cmd = req_data[3];

      msg = calloc(1, sizeof(NCSI_NL_MSG_T));
      if (!msg) {
        syslog(LOG_ERR, "%s Error: failed msg buffer allocation", __func__);
        break;
      }

      memset(msg, 0, sizeof(NCSI_NL_MSG_T));

      sprintf(msg->dev_name, "eth%d", netdev);
      msg->channel_id = channel;
      msg->cmd = cmd;
      msg->payload_length = tlen;

      for (i=0; i<msg->payload_length; i++) {
        msg->msg_payload[i] = req_data[4+i];
      }

      rsp = send_nl_msg_libnl(msg);
      if (rsp) {
        memcpy(&res_data[0], &rsp->msg_payload[0], rsp->hdr.payload_length);
        *res_len = rsp->hdr.payload_length;
        completion_code = CC_SUCCESS;
      } else {
        completion_code = CC_UNSPECIFIED_ERROR;
      }

      free(msg);
      if (rsp)
        free(rsp);

      break;
    default:
      return completion_code;
  }

  return completion_code;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  gpio_desc_t *desc = gpio_open_by_shadow("FM_POST_CARD_PRES_BMC_N");
  gpio_value_t value;
  int ret = -1;

  if (!desc) {
    return -1;
  }
  if (gpio_get_value(desc, &value) == 0) {
    *status = value == GPIO_VALUE_LOW ? 1 : 0;
    ret = 0;
  }
  gpio_close(desc);
  return ret;
}

static bool
is_cpu_socket_occupy(unsigned int cpu_idx) {
  static bool cached = false;
  static uint8_t cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_CPU0_SKTOCC_LVT3_PLD_N",
      "FM_CPU1_SKTOCC_LVT3_PLD_N"
    };
    if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return false;
    }
    cached = true;
  }

  // bit == 1 implies CPU is absent.
  if (cached_id & (1 << cpu_idx)) {
    return false;
  }
  return true;
}

int
pal_get_syscfg_text (char *text) {
  char key[MAX_KEY_LEN], value[MAX_VALUE_LEN], entry[MAX_VALUE_LEN];
  char *key_prefix = "sys_config/";
  int num_cpu=2, num_dimm, num_drive=14;
  int index, surface, bubble;
  size_t ret;
  char **dimm_labels;
  struct dimm_map map[48], temp_map;

  if (text == NULL)
    return -1;

  // Clear string buffer
  text[0] = '\0';

  // CPU information
  for (index = 0; index < num_cpu; index++) {
    if (!is_cpu_socket_occupy((unsigned int)index))
      continue;
    sprintf(entry, "CPU%d:", index);

    // Processor#
    snprintf(key, MAX_KEY_LEN, "%sfru1_cpu%d_product_name",
      key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 26) {
      // Read 4 bytes Processor#
      if (snprintf(&entry[strlen(entry)], 5, "%s", &value[22]) > 5) {
        syslog(LOG_ERR, "%s: CPU processor ID truncation detected!\n", __func__);
      }
    }

    // Frequency & Core Number
    snprintf(key, MAX_KEY_LEN, "%sfru1_cpu%d_basic_info",
      key_prefix, index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 5) {
      sprintf(&entry[strlen(entry)], "/%.1fG/%dc",
        (float) (value[4] << 8 | value[3])/1000, value[0]);
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }


  num_dimm = 24;
  dimm_labels = dimm_label;

  // Initialize map
  for (index = 0; index < num_dimm; index++) {
    map[index].index = index;
    map[index].label = dimm_labels[index];
  }
  // Bubble Sort the map according label string
  surface = num_dimm;
  for (surface = num_dimm; surface > 1;) {
    bubble = 0;
    for(index = 0; index < surface - 1; index++) {
      if (strcmp(map[index].label, map[index+1].label) > 0) {
        // Swap
        temp_map = map[index+1];
        map[index+1] = map[index];
        map[index] = temp_map;

        bubble = index + 1;
      }
    }
    surface = bubble;
  }
  // DIMM information
  for (index = 0; index < num_dimm; index++) {
    sprintf(entry, "MEM%s:", map[index].label);

    // Check Present
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_location",
      key_prefix, map[index].index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      // Skip if not present
      if (value[0] != 0x01)
        continue;
    }

    // Module Manufacturer ID
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_manufacturer_id",
      key_prefix, map[index].index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 2) {
      switch (value[1]) {
        case 0xce:
          sprintf(&entry[strlen(entry)], "Samsung");
          break;
        case 0xad:
          sprintf(&entry[strlen(entry)], "Hynix");
          break;
        case 0x2c:
          sprintf(&entry[strlen(entry)], "Micron");
          break;
        default:
          sprintf(&entry[strlen(entry)], "unknown");
          break;
      }
    }

    // Speed
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_speed",
      key_prefix, map[index].index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 6) {
      sprintf(&entry[strlen(entry)], "/%dMhz/%dGB",
        value[1]<<8 | value[0],
        (value[5]<<24 | value[4]<<16 | value[3]<<8 | value[2])/1024 );
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }

  // Drive information
  for (index = 0; index < num_drive; index++) {
    sprintf(entry, "HDD%d:", index);

    // Check Present
    snprintf(key, MAX_KEY_LEN, "%sfru1_B_drive%d_location",
      key_prefix, index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 3) {
      // Skip if not present
      if (value[2] == 0xff)
        continue;
    }

    // Model name
    snprintf(key, MAX_KEY_LEN, "%sfru1_B_drive%d_model_name",
      key_prefix, index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      snprintf(&entry[strlen(entry)], ret+1, "%s", value);
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }

  return 0;
}

int
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  char key[] = "mb_cpu_ppin";
  char str[MAX_VALUE_LEN] = {0};
  int i;
  int completion_code = CC_UNSPECIFIED_ERROR;
  *res_len = 0;

  if (req_len > SIZE_CPU_PPIN*2)
    req_len = SIZE_CPU_PPIN*2;

  for (i = 0; i < req_len; i++) {
    sprintf(str+(i*2), "%02x", req_data[i]);
  }

  if (kv_set(key, str, 0, KV_FPERSIST) != 0)
    return completion_code;

  completion_code = CC_SUCCESS;

  return completion_code;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {

   char str_server_por_cfg[64];
   char buff[MAX_VALUE_LEN];
   int policy = 3;
   unsigned char *data = res_data;

   // Platform Power Policy
   memset(str_server_por_cfg, 0 , sizeof(char) * 64);
   sprintf(str_server_por_cfg, "%s", "server_por_cfg");

   if (pal_get_key_value(str_server_por_cfg, buff) == 0)
   {
     if (!memcmp(buff, "off", strlen("off")))
       policy = 0;
     else if (!memcmp(buff, "lps", strlen("lps")))
       policy = 1;
     else if (!memcmp(buff, "on", strlen("on")))
       policy = 2;
     else
       policy = 3;
   }
   *data++ = ((is_server_off())?0x00:0x01) | (policy << 5);
   *data++ = 0x00;   // Last Power Event
   *data++ = 0x40;   // Misc. Chassis Status
   *data++ = 0x00;   // Front Panel Button Disable
   *res_len = data - res_data;
}

int
pal_fsc_get_target_snr(char *sname, struct fsc_monitor *fsc_fru_list, int fsc_fru_list_size)
{
  int i;
  for ( i=0;  i<fsc_fru_list_size; i++)
  {
    if ( 0 == strcmp(sname, fsc_fru_list[i].sensor_name) )
    {
#ifdef FSC_DEBUG
      syslog(LOG_WARNING,"[%s]sensor is found:%s, idx:%d", __func__, sname, i);
#endif
      return i;
    }
  }

  syslog(LOG_WARNING,"[%s]Unknown sensor name:%s", __func__, sname);
  return PAL_ENOTSUP;
}

bool
pal_sensor_is_valid(char *fru_name, char *sensor_name)
{
  uint8_t fru_id;
  struct fsc_monitor *fsc_fru_list;
  int fsc_fru_list_size;
  int ret;

  //check the fru name is valid or not
  ret = pal_get_fru_id(fru_name, &fru_id);
  if ( ret < 0 )
  {
    syslog(LOG_WARNING,"[%s] Wrong fru#%s", __func__, fru_name);
    return false;
  }

  fsc_fru_list = fsc_monitor_basic_snr_list;
  fsc_fru_list_size = fsc_monitor_basic_snr_list_size;

  //get the target sensor
  ret = pal_fsc_get_target_snr(sensor_name, fsc_fru_list, fsc_fru_list_size);
  if ( ret < 0 )
  {
    syslog(LOG_WARNING,"[%s] undefined sensor: %s", __func__, sensor_name);
    return false;
  }

  return true;
}

int
pal_convert_to_dimm_str(uint8_t cpu, uint8_t channel, uint8_t slot, char *str) {
  uint8_t idx;
  char label[] = {'A','C','B','D'};

  if ((idx = cpu*2+slot) < sizeof(label)) {
    sprintf(str, "%c%d", label[idx], channel);
  } else {
    sprintf(str, "NA");
  }

  return 0;
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

void
pal_post_end_chk(uint8_t *post_end_chk) {
  static uint8_t post_end = 1;

  if (*post_end_chk == 1) {
    post_end = 1;
  } else if (*post_end_chk == 0) {
    *post_end_chk = post_end;
    post_end = 0;
  }
}

void
pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
  uint8_t post_end = 1;

  *res_len = 0;

  //Set post end chk flag to update LCD info page
  pal_post_end_chk(&post_end);

  // log the post end event
  syslog (LOG_INFO, "POST End Event for Payload#%d\n", slot);

  // Sync time with system
  if (system("/etc/init.d/sync_date.sh &") != 0) {
    syslog(LOG_ERR, "Sync date failed!\n");
  }
}

int
pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data)
{
  uint8_t bus_id = 0x5;
  int rlen = 0;
  int ret = PAL_EOK;

  rlen = ipmb_send(
    bus_id,
    0x2c,
    NETFN_APP_REQ << 2,
    CMD_APP_GET_SELFTEST_RESULTS);

  if ( rlen < 2 ) {
    ret = PAL_ENOTSUP;
  }
  else {
    //get the response data
    memcpy(data, ipmb_rxb()->data, 2);
  }

  return ret;
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  return me_xmit(request, req_len, response, rlen);
}

int
pal_fw_update_finished(uint8_t fru, const char *comp, int status) {
  int ret = 0;
  int ifd, retry = 3;
  uint8_t buf[16];
  char dev_i2c[16];

  ret = status;
  if (ret == 0) {
    sprintf(dev_i2c, "/dev/i2c-%d", PFR_MAILBOX_BUS);
    ifd = open(dev_i2c, O_RDWR);
    if (ifd < 0) {
      return -1;
    }

    buf[0] = 0x13;  // BMC update intent
    if (!strcmp(comp, "bmc")) {
      buf[1] = 0x08;  // BMC_ACTIVE
    } else if (!strcmp(comp, "bios")) {
      buf[1] = 0x01;  // PCH_ACTIVE
    } else if (!strcmp(comp, "cpld")) {
      buf[1] = 0x04;  // CPLD_ACTIVE
    }

    sync();
    sleep(3);
    ret = system("sv stop sensord > /dev/null 2>&1");
    ret = system("sv stop ipmbd_0 > /dev/null 2>&1");
    ret = system("sv stop ipmbd_5 > /dev/null 2>&1");

    printf("sending update intent to CPLD...\n");
    fflush(stdout);
    sleep(1);
    do {
      ret = i2c_rdwr_msg_transfer(ifd, PFR_MAILBOX_ADDR, buf, 2, NULL, 0);
      if (ret) {
        syslog(LOG_WARNING, "i2c%u xfer failed, cmd: %02x %02x", PFR_MAILBOX_BUS, buf[0], buf[1]);
        if (--retry > 0) {
          msleep(100);
        }
      }
    } while (ret && retry > 0);
    close(ifd);
  }

  return ret;
}

int
pal_is_pfr_active(void) {
  int pfr_active = PFR_NONE;
  int ifd, retry = 3;
  uint8_t tbuf[8], rbuf[8];
  char dev_i2c[16];

  sprintf(dev_i2c, "/dev/i2c-%d", PFR_MAILBOX_BUS);
  ifd = open(dev_i2c, O_RDWR);
  if (ifd < 0) {
    return pfr_active;
  }

  tbuf[0] = 0x0A;
  do {
    if (!i2c_rdwr_msg_transfer(ifd, PFR_MAILBOX_ADDR, tbuf, 1, rbuf, 1)) {
      pfr_active = (rbuf[0] & 0x20) ? PFR_ACTIVE : PFR_UNPROVISIONED;
      break;
    }

#ifdef DEBUG
    syslog(LOG_WARNING, "i2c%u xfer failed, cmd: %02x", 4, tbuf[0]);
#endif
    if (--retry > 0)
      msleep(20);
  } while (retry > 0);
  close(ifd);

  return pfr_active;
}
