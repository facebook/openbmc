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
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include "pal.h"


#define FBAL_PLATFORM_NAME "angelslanding"
#define LAST_KEY "last_key"

#define GPIO_POWER "FM_BMC_PWRBTN_OUT_R_N"
#define GPIO_POWER_GOOD "PWRGD_SYS_PWROK"
#define GPIO_POWER_RESET "RST_BMC_RSTBTN_OUT_R_N"
#define GPIO_RESET_BTN_IN "FP_BMC_RST_BTN_N" 
#define GPIO_BLADE_ID0 "FM_BLADE_ID_0"
#define GPIO_BLADE_ID1 "FM_BLADE_ID_1"
#define GPIO_LOCATE_LED "FP_LOCATE_LED"

#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10


const char pal_fru_list[] = "all, mb, nic0, nic1";
const char pal_server_list[] = "mb";

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

static int key_func_por_policy (int event, void *arg);
static int key_func_lps (int event, void *arg);
static int key_func_ntp (int event, void *arg);

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
  {"nic_sensor_health", "1", NULL},
  {"server_sel_error", "1", NULL},
  {"server_boot_order", "0100090203ff", NULL},
  {"ntp_server", "", key_func_ntp},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

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

static int fw_getenv(char *key, char *value)
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

static void fw_setenv(char *key, char *value)
{
  char old_value[MAX_VALUE_LEN] = {0};
  if (fw_getenv(key, old_value) != 0 ||
      strcmp(old_value, value) != 0) {
    /* Set the env key:value if either the key
     * does not exist or the value is different from
     * what we want set */
    char cmd[MAX_VALUE_LEN] = {0};
    snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv %s %s", key, value);
    system(cmd);
  }
}

//Overwrite the one in obmc-pal.c without systme call of flashcp check
bool
pal_is_fw_update_ongoing(uint8_t fruid) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);
  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

static int
key_func_por_policy (int event, void *arg)
{
  char value[MAX_VALUE_LEN] = {0};

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      // sync to env
      if ( !strcmp(arg,"lps") || !strcmp(arg,"on") || !strcmp(arg,"off")) {
        fw_setenv("por_policy", (char *)arg);
      }
      else
        return -1;
      break;
    case KEY_AFTER_INI:
      // sync to env
      kv_get("server_por_cfg", value, NULL, KV_FPERSIST);
      fw_setenv("por_policy", value);
      break;
  }

  return 0;
}

static int
key_func_lps (int event, void *arg)
{
  char value[MAX_VALUE_LEN] = {0};

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
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
key_func_ntp (int event, void *arg)
{
  char cmd[MAX_VALUE_LEN] = {0};
  char ntp_server_new[MAX_VALUE_LEN] = {0};
  char ntp_server_old[MAX_VALUE_LEN] = {0};

  switch (event) {
    case KEY_BEFORE_SET:
      // Remove old NTP server
      kv_get("ntp_server", ntp_server_old, NULL, KV_FPERSIST);
      if (strlen(ntp_server_old) > 2) {
        snprintf(cmd, MAX_VALUE_LEN, "sed -i '/^restrict %s$/d' /etc/ntp.conf", ntp_server_old);
        system(cmd);
        snprintf(cmd, MAX_VALUE_LEN, "sed -i '/^server %s$/d' /etc/ntp.conf", ntp_server_old);
        system(cmd);
      }
      // Add new NTP server
      snprintf(ntp_server_new, MAX_VALUE_LEN, "%s", (char *)arg);
      if (strlen(ntp_server_new) > 2) {
        snprintf(cmd, MAX_VALUE_LEN, "echo \"restrict %s\" >> /etc/ntp.conf", ntp_server_new);
        system(cmd);
        snprintf(cmd, MAX_VALUE_LEN, "echo \"server %s\" >> /etc/ntp.conf", ntp_server_new);
        system(cmd);
      }
      // Restart NTP server
      snprintf(cmd, MAX_VALUE_LEN, "/etc/init.d/ntpd restart > /dev/null &");
      system(cmd);
      break;
    case KEY_AFTER_INI:
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
    fscanf(fp, "%d", &por);
    fclose(fp);
  }

  return (por)?1:0;
}

// Power On the server in a given slot
static int
server_power_on(void) {
  int ret;
  gpio_desc_t *gdesc = NULL;

  gdesc = gpio_open_by_shadow(GPIO_POWER);
  if (gdesc == NULL)
    return -1;

  ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  if (ret != 0)
    goto error;

  ret = gpio_set_value(gdesc, GPIO_VALUE_LOW);
  if (ret != 0)
    goto error;

  sleep(1);

  ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  if (ret != 0)
    goto error;

  sleep(2);

  system("/usr/bin/sv restart fscd >> /dev/null");

error:
  gpio_close(gdesc);
  return ret;
}

// Power Off the server in given slot
static int
server_power_off(bool gs_flag) {
  int ret;
  gpio_desc_t *gdesc = NULL;

  gdesc = gpio_open_by_shadow(GPIO_POWER);
  if (gdesc == NULL)
    return -1;

  system("/usr/bin/sv stop fscd >> /dev/null");

  ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  if (ret != 0)
    goto error;

  sleep(1);

  ret = gpio_set_value(gdesc, GPIO_VALUE_LOW);
  if (ret != 0)
    goto error;

  if (gs_flag) {
    sleep(DELAY_GRACEFUL_SHUTDOWN);
  } else {
    sleep(DELAY_POWER_OFF);
  }

  ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  if (ret != 0)
    goto error;

error:
  gpio_close(gdesc);
  return ret;
}

int
pal_get_platform_name(char *name) {
  strcpy(name, FBAL_PLATFORM_NAME);

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  *status = 0;

  switch (fru) {
    case FRU_MB:
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

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int ret;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;

  if (fru != FRU_MB)
    return -1;

  gdesc = gpio_open_by_shadow(GPIO_POWER_GOOD);
  if (gdesc == NULL)
    return -1;

  ret = gpio_get_value(gdesc, &val);
  if (ret != 0)
    goto error;

  *status = (int)val;

error:
  gpio_close(gdesc);
  return ret;
}

bool
is_server_off(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_MB, &status);
  if (ret) {
    return false;
  }

  return status == SERVER_POWER_OFF? true: false;
}


// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  bool gs_flag = false;
  uint8_t ret;

  if (pal_get_server_power(fru, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return server_power_on();
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return server_power_off(gs_flag);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off(gs_flag))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on();

      } else if (status == SERVER_POWER_OFF) {

        return (server_power_on());
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return 1;
      gs_flag = true;
      return server_power_off(gs_flag);
      break;

   case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {
        ret = pal_set_rst_btn(fru, 0);
        if (ret < 0)
          return ret;
        msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high
        ret = pal_set_rst_btn(fru, 1);
        if (ret < 0)
          return ret;
      } else if (status == SERVER_POWER_OFF)
        return -1;
      break;

    default:
      return -1;
  }

  return 0;
}

int
pal_sled_cycle(void) {
  // Send command to HSC power cycle
  system("i2cset -y 7 0x11 0xd9 c &> /dev/null");

  return 0;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  int ret;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;

  if (slot != FRU_MB) {
    return -1;
  }

  gdesc = gpio_open_by_shadow(GPIO_POWER_RESET);
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

// Return the front panel's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {
  int ret = -1;
  gpio_value_t value;
  gpio_desc_t *desc = gpio_open_by_shadow(GPIO_RESET_BTN_IN);
  if (!desc) {
     return -1;
  }
  if (0 == gpio_get_value(desc, &value)) {
    *status = value == GPIO_VALUE_HIGH ? 0 : 1;
    ret = 0;
  }
  gpio_close(desc);
  return ret;
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

// Update the LED for the given slot with the status
int
pal_set_led(uint8_t fru, uint8_t status) {

// Controlled by pal_set_id_led()
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "mb")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "pdb")) {
    *fru = FRU_PDB;
  } else if (!strcmp(str, "nic")) {
    *fru = FRU_NIC0;
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
      strcpy(name, "nic");
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

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
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
  case FRU_MB:
    sprintf(fname, "mb");
    break;
  default:
    return -1;
  }

  sprintf(path, "/tmp/fruid_%s.bin", fname);

  return 0;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  switch(fru) {
  case FRU_MB:
    sprintf(path, FRU_EEPROM_MB);
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
      return I2C_BUS_6; // Riser
  }

  // Debug purpose, map to real bus number
  if (channel & 0x80) {
    return (channel & 0x7f);
  }

  return channel;
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
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return -1;
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return -1;
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

static int
get_gpio_shadow_array(const char **shadows, int num, uint8_t *mask)
{
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

int pal_get_bmc_ipmb_slave_addr(uint16_t* slave_addr, uint8_t bus_id)
{
  uint8_t val;
  int ret;
  static uint16_t addr=0;

  if ((bus_id == I2C_BUS_1) || (bus_id == I2C_BUS_5)) {

    if (addr == 0) {
      ret = pal_get_blade_id (&val);
      if (ret != 0) {
        return -1;
      }
      addr = 0x1010 | val;
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
