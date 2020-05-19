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
#include <openbmc/obmc-i2c.h>
#include <facebook/fbal_fruid.h>
#include <openbmc/ipmb.h>
#include "pal.h"

#define FBAL_PLATFORM_NAME "angelslanding"
#define LAST_KEY "last_key"

#define GPIO_ID_LED "FP_ID_LED_N"
#define GPIO_LOCATE_LED "FP_LOCATE_LED"
#define GPIO_FAULT_LED "FM_BMC_LED_CATERR_N"
#define GPIO_NIC0_PRSNT "HP_LVC3_OCP_V3_1_PRSNT2_N"
#define GPIO_NIC1_PRSNT "HP_LVC3_OCP_V3_2_PRSNT2_N"
#define GPIO_SKT_ID0 "FM_BMC_SKT_ID_0"
#define GPIO_SKT_ID2 "FM_BMC_SKT_ID_2"

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

#define MAX_CPU_NUM 8

const char pal_fru_list[] = "all, mb, nic0, nic1, pdb, bmc";
const char pal_server_list[] = "mb";

static int key_func_por_policy (int event, void *arg);
static int key_func_lps (int event, void *arg);

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

typedef enum {
  SV_LAST_PWR_ST = 0,
  SV_SYSFW_VER,
  SLED_IDENTIFY,
  SLED_TIMESTAMP,
  SV_POR_CFG,
  SV_SNR_HEALTH,
  NIC_SNR_HEALTH,
  SV_SEL_ERR,
  SV_BOOT_ORDER,
  CPU0_PPIN,
  CPU1_PPIN,
  CPU2_PPIN,
  CPU3_PPIN,
  CPU4_PPIN,
  CPU5_PPIN,
  CPU6_PPIN,
  CPU7_PPIN,
  NTP_SERVER,
  LAST_ID = 255
} key_cfg_id;

struct pal_key_cfg {
  key_cfg_id id;
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {SV_LAST_PWR_ST, "pwr_server_last_state", "on", key_func_lps},
  {SV_SYSFW_VER, "sysfw_ver_server", "0", NULL},
  {SLED_IDENTIFY, "identify_sled", "off", NULL},
  {SLED_TIMESTAMP, "timestamp_sled", "0", NULL},
  {SV_POR_CFG, "server_por_cfg", "lps", key_func_por_policy},
  {SV_SNR_HEALTH, "server_sensor_health", "1", NULL},
  {NIC_SNR_HEALTH, "nic_sensor_health", "1", NULL},
  {SV_SEL_ERR, "server_sel_error", "1", NULL},
  {SV_BOOT_ORDER, "server_boot_order", "0100090203ff", NULL},
  {CPU0_PPIN, "cpu0_ppin", "", NULL},
  {CPU1_PPIN, "cpu1_ppin", "", NULL},
  {CPU2_PPIN, "cpu2_ppin", "", NULL},
  {CPU3_PPIN, "cpu3_ppin", "", NULL},
  {CPU4_PPIN, "cpu4_ppin", "", NULL},
  {CPU5_PPIN, "cpu5_ppin", "", NULL},
  {CPU6_PPIN, "cpu6_ppin", "", NULL},
  {CPU7_PPIN, "cpu7_ppin", "", NULL},
  {NTP_SERVER, "ntp_server", "", NULL},
  /* Add more Keys here */
  {LAST_ID, LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

static int
pal_key_index(char *key) {
  int i;

  for (i = 0; key_cfg[i].id != LAST_ID; i++) {
    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;
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
fw_getenv(char *key, char *value) {
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
key_func_lps(int event, void *arg) {
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
  strcpy(name, FBAL_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = 1;
  return 0;
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
  case FRU_PDB:
    *status = 1;
    break;
  case FRU_BMC:
    *status = 1;
    break;
  case FRU_DBG:
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
  uint8_t sys_pwr;
  gpio_desc_t *gdesc_id = NULL, *gdesc_loc = NULL;
  gpio_value_t val;

  do {
    if (!(gdesc_id = gpio_open_by_shadow(GPIO_ID_LED)))
      break;

    if (status == 0xFF) {  // restore FP_ID_LED_N
      gpio_set_value(gdesc_id, GPIO_VALUE_LOW);
      break;
    }

    if (!(gdesc_loc = gpio_open_by_shadow(GPIO_LOCATE_LED)))
      break;

    if (!pal_get_server_power(fru, &sys_pwr)) {
      val = sys_pwr ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
      gpio_set_value(gdesc_id, val);
    }
    val = status ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    gpio_set_value(gdesc_loc, val);
  } while (0);

  if (gdesc_id) {
    gpio_close(gdesc_id);
  }
  if (gdesc_loc) {
    gpio_close(gdesc_loc);
  }

  return 0;
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
  } else if (!strcmp(str, "mb") || !strcmp(str, "cpld") || !strcmp(str, "vr")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "pdb")) {
    *fru = FRU_PDB;
  } else if (!strcmp(str, "nic0") || !strcmp(str, "nic")) {
    *fru = FRU_NIC0;
  } else if (!strcmp(str, "nic1")) {
    *fru = FRU_NIC1;
  } else if (!strcmp(str, "ocpdbg")) {
    *fru = FRU_DBG;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
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
    case FRU_PDB:
      strcpy(name, "pdb");
      break;
    case FRU_NIC0:
      strcpy(name, "nic0");
      break;
    case FRU_NIC1:
      strcpy(name, "nic1");
      break;
    case FRU_DBG:
      strcpy(name, "ocpdbg");
      break;
    case FRU_BMC:
      strcpy(name, "bmc");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }

  return 0;
}

void
pal_dump_key_value(void) {
  int i;
  uint8_t mode;
  char value[MAX_VALUE_LEN];

  for (i = 0; key_cfg[i].id != LAST_ID; i++) {
    if ((key_cfg[i].id >= CPU2_PPIN) && (key_cfg[i].id <= CPU7_PPIN)) {
      if (!pal_get_host_system_mode(&mode) && (mode == MB_2S_MODE)) {
        continue;
      }
    }

    printf("%s:", key_cfg[i].name);
    memset(value, 0, sizeof(value));
    if (kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST) < 0) {
      printf("\n");
    } else {
      printf("%s\n", value);
    }
  }
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

void
pal_update_ts_sled() {
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
  case FRU_MB:
    sprintf(fname, "mb");
    break;
  case FRU_NIC0:
    sprintf(fname, "nic0");
    break;
  case FRU_NIC1:
    sprintf(fname, "nic1");
    break;
  case FRU_PDB:
    sprintf(fname, "pdb");
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

void fru_eeprom_mb_check(char* mb_path) {
  uint8_t id=0;

  pal_get_board_rev_id(&id);

  if(id < REV_DVT) {
   sprintf(mb_path, FRU_EEPROM_MB_T, 54);
  } else {
   sprintf(mb_path, FRU_EEPROM_MB_T, 57);
  }
}


int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  char FRU_EEPROM_MB[64];

  switch(fru) {
  case FRU_MB:
    fru_eeprom_mb_check(FRU_EEPROM_MB);
    sprintf(path, "%s", FRU_EEPROM_MB);
    break;
  case FRU_NIC0:
    sprintf(path, FRU_EEPROM_NIC0);
    break;
  case FRU_NIC1:
    sprintf(path, FRU_EEPROM_NIC1);
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

  case FRU_PDB:
    sprintf(name, "PDB");
    break;

  case FRU_BMC:
    sprintf(name, "BMC");
    break;

  default:
    return -1;
  }
  return 0;
}

int
pal_fruid_write(uint8_t fru, char *path) {
  if (fru == FRU_PDB) {
    return fbal_write_pdb_fruid(0, path);
  }
  return -1;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
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

    case IPMI_CHANNEL_2:
      return I2C_BUS_2; // Slave BMC

    case IPMI_CHANNEL_6:
      return I2C_BUS_5; // ME

    case IPMI_CHANNEL_8:
      return I2C_BUS_8; // CM

    case IPMI_CHANNEL_9:
      return I2C_BUS_6; // EP
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

  for (i = 1; i <= MAX_NUM_FRUS; i++) {
    if (pal_is_fw_update_ongoing(i) == true)
      return true;
  }

  return false;
}

int
pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  char key[64] = {0};
  char value[64] = {0};
  struct timespec ts;
  int index;

  sprintf(key, "fru%d_fwupd", fruid);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
    return -1;
  }

  index = lib_cmc_get_block_index(fruid);
  if(index < 0) {
    return -1;
  }

  if (tmout == 0) {
    lib_cmc_set_block_command_flag(index, CM_COMMAND_UNBLOCK);
  } else {
    lib_cmc_set_block_command_flag(index, CM_COMMAND_BLOCK);
  }

  return 0;
}



int
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
pal_set_def_key_value() {
  int i;
  char key[MAX_KEY_LEN] = {0};

  for (i = 0; key_cfg[i].id != LAST_ID; i++) {
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
pal_get_blade_id(uint8_t *id) {
  static bool cached = false;
  static uint8_t cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_BLADE_ID_0",
      "FM_BLADE_ID_1"
    };
    if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return -1;
    }
    cached = true;
  }
  *id = cached_id;
  return 0;
}

int
pal_get_bmc_ipmb_slave_addr(uint16_t* slave_addr, uint8_t bus_id) {
  uint8_t val;
  int ret;
  static uint8_t addr=0;

  if (bus_id == I2C_BUS_2) {
    if (addr == 0) {
      ret = pal_get_blade_id(&val);
      if (ret != 0) {
        return -1;
      }
      addr = 0x10 | val;
      *slave_addr = addr;
    } else {
      *slave_addr = addr;
    }
  } else {
    *slave_addr = 0x10;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG,"%s BMC Slave Addr=%d bus=%d", __func__, *slave_addr, bus_id);
#endif
  return 0;
}

int
pal_ipmb_processing(int bus, void *buf, uint16_t size) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  struct timespec ts;
  static time_t last_time = 0;

  switch (bus) {
    case I2C_BUS_0:
      if (((uint8_t *)buf)[0] == 0x20) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if (ts.tv_sec >= (last_time + 5)) {
          last_time = ts.tv_sec;
          ts.tv_sec += 20;

          sprintf(key, "ocpdbg_lcd");
          sprintf(value, "%ld", ts.tv_sec);
          if (kv_set(key, value, 0, 0) < 0) {
            return -1;
          }
        }
      }
      break;
  }

  return 0;
}

int
pal_is_mcu_ready(uint8_t bus) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  switch (bus) {
    case I2C_BUS_0:
      sprintf(key, "ocpdbg_lcd");
      if (kv_get(key, value, NULL, 0)) {
        return false;
      }

      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (strtoul(value, NULL, 10) > ts.tv_sec) {
         return true;
      }
      break;

    case I2C_BUS_8:
      return true;
  }

  return false;
}

int
pal_get_mb_position(uint8_t* pos) {
  static bool cached = false;
  static uint8_t cached_pos = 0;

  if (!cached) {
    if(pal_get_blade_id (&cached_pos))
      return -1;

    switch (cached_pos) {
      case 0:
        cached_pos = MB_ID4;
        break;
      case 1:
        cached_pos = MB_ID3;
        break;
      case 2:
        cached_pos = MB_ID2;
        break;
      case 3:
        cached_pos = MB_ID1;
        break;
      default:
        return -1;
    }
    cached = true;
  }

  *pos = cached_pos;
#ifdef DEBUG
  syslog(LOG_DEBUG,"%s BMC Position ID =%d\n", __func__, cached_pos);
#endif
  return 0;
}

int
pal_get_mb_mode(uint8_t* mode) {
  int ret=0;

  ret = cmd_cmc_get_config_mode(mode);
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s mode=%d\n", __func__, *mode);
#endif
  if(ret != 0) {
    return ret;
  }

  return 0;
}

int
pal_get_config_is_master(void) {
  gpio_desc_t *gdesc;
  gpio_value_t val;
  static bool cached = false;
  static int status = 1;

  if (!cached) {
    if ((gdesc = gpio_open_by_shadow(GPIO_SKT_ID0))) {
      if (!gpio_get_value(gdesc, &val)) {
        status = (val == GPIO_VALUE_LOW) ? 1 : 0;
        cached = true;
      }
      gpio_close(gdesc);
    }
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s status=%x", __func__, status);
#endif
  return status;
}

int
pal_get_platform_id(uint8_t *id) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN]={0};

  sprintf(key, "mb_sku");
  if (kv_get(key, value, NULL, 0)) {
    return false;
  }

  *id = atoi(value);
  return 0;
}

int
pal_get_host_system_mode(uint8_t* mode) {
  gpio_desc_t *gdesc;
  gpio_value_t val;
  static bool cached = false;
  static uint8_t cached_pos = 0;

  if (!cached) {
    if ((gdesc = gpio_open_by_shadow(GPIO_SKT_ID2))) {
      if (!gpio_get_value(gdesc, &val)) {
        cached_pos = (val == GPIO_VALUE_LOW) ? MB_8S_MODE : MB_2S_MODE;
        cached = true;
      }
      gpio_close(gdesc);
    }
  }

  *mode = cached_pos;
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s: BMC System Mode = %u", __func__, cached_pos);
#endif
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
  int ret = -1;
  int i, j;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[8] = {0};
  uint8_t mode;

  ret = pal_get_host_system_mode(&mode);
  if(ret != 0) {
    return ret;
  }

  ret = pal_get_config_is_master();
  if(ret < 0) {
    return ret;
  }

  if( ret || (mode == MB_2S_MODE) ) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s true, mode=%d\n", __func__, mode);
#endif
    ret = pal_get_key_value("sysfw_ver_server", str);
    if (ret) {
      return ret;
    }

    for (i = 0, j = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
      sprintf(tstr, "%c%c", str[i], str[i+1]);
      ver[j++] = strtol(tstr, NULL, 16);
    }
  } else {
    return -1;
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

    snprintf(tstr, sizeof(tstr), "%02x", boot[i]);
    strncat(str, tstr, sizeof(str) - 1);
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


  ver[0] = dev_id.fw_rev1 & 0x7F;
  ver[1] = dev_id.fw_rev2 >> 4;
  ver[2] = dev_id.fw_rev2 & 0x0f;
  ver[3] = dev_id.aux_fw_rev[1];
  ver[4] = dev_id.aux_fw_rev[2] >> 4;
  return ret;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd;
  ssize_t bytes_rd;
  char FRU_EEPROM_MB[64];

  fru_eeprom_mb_check(FRU_EEPROM_MB);
  errno = 0;

  // check for file presence
  if (access(FRU_EEPROM_MB, F_OK)) {
    syslog(LOG_ERR, "pal_get_guid: unable to access %s: %s", FRU_EEPROM_MB, strerror(errno));
    return errno;
  }

  fd = open(FRU_EEPROM_MB, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "pal_get_guid: unable to open %s: %s", FRU_EEPROM_MB, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read from %s failed: %s", FRU_EEPROM_MB, strerror(errno));
  }

  close(fd);
  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd;
  ssize_t bytes_wr;
  char FRU_EEPROM_MB[64];

  fru_eeprom_mb_check(FRU_EEPROM_MB);
  errno = 0;

  // check for file presence
  if (access(FRU_EEPROM_MB, F_OK)) {
    syslog(LOG_ERR, "pal_set_guid: unable to access %s: %s", FRU_EEPROM_MB, strerror(errno));
    return errno;
  }

  fd = open(FRU_EEPROM_MB, O_WRONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "pal_set_guid: unable to open %s: %s", FRU_EEPROM_MB, strerror(errno));
    return errno;
  }

  lseek(fd, offset, SEEK_SET);

  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s failed: %s", FRU_EEPROM_MB, strerror(errno));
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

  return;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_SYS_GUID, guid);
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
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
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

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  int ret;
  uint8_t platform_id  = 0x00;
  uint8_t board_rev_id = 0x00;
  int completion_code=CC_UNSPECIFIED_ERROR;

  ret = pal_get_platform_id(&platform_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  ret = pal_get_board_rev_id(&board_rev_id);
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
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN];
  int i, comp_code = CC_SUCCESS;

  *res_len = 0;
  if (req_len > SIZE_CPU_PPIN*MAX_CPU_NUM)
    req_len = SIZE_CPU_PPIN*MAX_CPU_NUM;

  for (i = 0; i < req_len; i++) {
    sprintf(&str[(i%MAX_CPU_NUM)*2], "%02x", req_data[i]);

    if (!((i+1)%MAX_CPU_NUM)) {
      sprintf(key, "cpu%d_ppin", i/MAX_CPU_NUM);
      if (pal_set_key_value(key, str)) {
        comp_code = CC_UNSPECIFIED_ERROR;
        break;
      }
    }
  }

  return comp_code;
}

int
pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data)
{
  NM_RW_INFO info;
  uint8_t rbuf[8];
  uint8_t rlen;
  int ret;

  info.bus = NM_IPMB_BUS_ID;
  info.nm_addr = NM_SLAVE_ADDR;
  ret = pal_get_bmc_ipmb_slave_addr(&info.bmc_addr, info.bus);
  if (ret != 0) {
    return PAL_ENOTSUP;
  }

  ret = cmd_NM_get_self_test_result(&info, rbuf, &rlen);
  if (ret != 0) {
    return PAL_ENOTSUP;
  }

  memcpy(data, rbuf, rlen);
#ifdef DEBUG
  syslog(LOG_WARNING, "rbuf[0] =%x rbuf[1] =%x\n", rbuf[0], rbuf[1]);
#endif
  return ret;
}

static int get_dev_bridge_info(uint8_t slot, uint8_t* dev_addr,
                               uint8_t* bus_num, uint16_t* bmc_addr) {
  int ret=0;

  switch(slot) {
    case BYPASS_ME:
      *dev_addr = NM_SLAVE_ADDR;
      *bus_num = NM_IPMB_BUS_ID;
      break;

    case BRIDGE_2_CM:
      *dev_addr = CM_SLAVE_ADDR;
      *bus_num = CM_IPMB_BUS_ID;
      break;

    case BRIDGE_2_MB_BMC0:
      //BMC IPMB SLVAE ADDR MB0 0x20
      *dev_addr = BMC0_SLAVE_DEF_ADDR;
      *bus_num = BMC_IPMB_BUS_ID;
      break;

    case BRIDGE_2_MB_BMC1:
      //BMC IPMB SLVAE ADDR MB1:0x22
      *dev_addr = BMC1_SLAVE_DEF_ADDR;
      *bus_num = BMC_IPMB_BUS_ID;
      break;

    case BRIDGE_2_MB_BMC2:
      //BMC IPMB SLVAE ADDR MB2:0x24
      *dev_addr = BMC2_SLAVE_DEF_ADDR;
      *bus_num = BMC_IPMB_BUS_ID;
      break;

    case BRIDGE_2_MB_BMC3:
      //BMC IPMB SLVAE ADDR MB3:0x26
      *dev_addr = BMC3_SLAVE_DEF_ADDR;
      *bus_num = BMC_IPMB_BUS_ID;
      break;

    case BRIDGE_2_ASIC_BMC:
      *dev_addr = ASIC_BMC_SLAVE_ADDR;
      *bus_num = ASIC_IPMB_BUS_ID;
      break;

    default:
      return -1;
  }

  ret = pal_get_bmc_ipmb_slave_addr(bmc_addr, *bus_num);
  if(ret != 0) {
    return -1;
  }
  return 0;
}

static int pal_ipmb_bypass (uint8_t *req_data, uint8_t req_len,
                            uint8_t *res_data, uint8_t *res_len) {
  int ret;
  uint8_t slot, netfn, cmd;
  uint8_t dev_addr, bus_num;
  uint8_t txlen, rxlen;
  uint8_t txbuf[256] = {0x00};
  uint8_t rxbuf[256] = {0x00};
  uint16_t bmc_addr;

  //payload, netfn, cmd, data[0]:select, data[1]:bypass netfn, data[2]:bypass cmd
  txlen = req_len - 6;
  slot = req_data[0];
  netfn = req_data[1];
  cmd = req_data[2];

  ret = get_dev_bridge_info(slot, &dev_addr, &bus_num, &bmc_addr);
  if(ret != 0) {
    return CC_OEM_DEVICE_INFO_ERR;
  }

  //Don't allow bridge to self
  if(bmc_addr << 1 == dev_addr) {
    return CC_OEM_DEVICE_DESTINATION_ERR;
  }

  memcpy(txbuf, &req_data[3], txlen);

  ret = lib_ipmb_send_request(cmd, netfn,
                              txbuf, txlen,
                              rxbuf, &rxlen,
                              bus_num, dev_addr, bmc_addr);

  if(ret != CC_SUCCESS) {
    return ret;
  }

  memcpy(&res_data[0], &rxbuf[0], rxlen);
  *res_len = rxlen;

  return CC_SUCCESS;
}

// OEM Command "CMD_OEM_BYPASS_CMD" 0x34 return: CC Code
int pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len){
  int ret;
  uint8_t select;
  uint8_t status;

  *res_len = 0;
  ret = pal_is_fru_prsnt(slot, &status);
  if (ret < 0) {
    return CC_OEM_DEVICE_NOT_PRESENT;
  }
  if (status == 0) {
    return CC_UNSPECIFIED_ERROR;
  }
  select = req_data[0];

  switch (select) {
    case BYPASS_ME:          //ME
    case BRIDGE_2_CM:        //Chassis Manager
    case BRIDGE_2_MB_BMC0:   //MB BMC0
    case BRIDGE_2_MB_BMC1:   //MB BMC1
    case BRIDGE_2_MB_BMC2:   //MB BMC2
    case BRIDGE_2_MB_BMC3:   //MB BMC3
    case BRIDGE_2_ASIC_BMC:  //FBEP
      ret = pal_ipmb_bypass(req_data, req_len, res_data, res_len);

      if(ret != CC_SUCCESS) {
        return ret;
      }
      break;

    default:
      return CC_UNSPECIFIED_ERROR;
  }
  return CC_SUCCESS;
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
pal_get_pfr_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged) {
  if (fru != FRU_MB) {
    return -1;
  }
  *bus = PFR_MAILBOX_BUS;
  *addr = PFR_MAILBOX_ADDR;
  *bridged = false;
  return 0;
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
      buf[1] = UPDATE_BMC_ACTIVE;
    } else if (!strcmp(comp, "bios")) {
      buf[1] = UPDATE_UPDATE_DYNAMIC | UPDATE_PCH_ACTIVE;
      if (!pal_get_config_is_master()) {
        buf[1] |= UPDATE_AT_RESET;
      }
    } else if (!strcmp(comp, "pfr_cpld")) {
      buf[1] = UPDATE_CPLD_ACTIVE;
      if (!pal_get_config_is_master()) {
        buf[1] |= UPDATE_AT_RESET;
      }
    }

    sync();
    sleep(3);
    ret = system("sv stop sensord > /dev/null 2>&1");
    ret = system("sv stop ipmbd_0 > /dev/null 2>&1");
    ret = system("sv stop ipmbd_2 > /dev/null 2>&1");
    ret = system("sv stop ipmbd_5 > /dev/null 2>&1");
    ret = system("sv stop ipmbd_6 > /dev/null 2>&1");
    ret = system("sv stop ipmbd_8 > /dev/null 2>&1");

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
