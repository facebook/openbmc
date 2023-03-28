#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-pal.h>
#include "pal_def.h"
#include "pal_common.h"

#define LAST_KEY "last_key"

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
  SWB_SNR_HEALTH,
  HGX_SNR_HEALTH,
  NIC0_SNR_HEALTH,
  NIC1_SNR_HEALTH,
  VPDB_SNR_HEALTH,
  HPDB_SNR_HEALTH,
  FAN_BP1_SNR_HEALTH,
  FAN_BP2_SNR_HEALTH,
  SCM_SNR_HEALTH,
  HSC_SNR_HEALTH,
  SHSC_SNR_HEALTH,
  SV_SEL_ERR,
  SV_BOOT_ORDER,
  CPU0_PPIN,
  CPU1_PPIN,
  NTP_SERVER,
  LAST_ID = 255
} key_cfg_id;

struct pal_key_cfg {
  key_cfg_id id;
  char *name;
  char *def_val;
  int (*function)(int, void*);
  bool enable;
} key_cfg[] = {
  /* name, default value, function */
  {SV_LAST_PWR_ST,      "pwr_server_last_state", "on",           key_func_lps, true},
  {SV_SYSFW_VER,        "fru1_sysfw_ver",        "0",            NULL, true},
  {SLED_IDENTIFY,       "identify_sled",         "off",          NULL, true},
  {SLED_TIMESTAMP,      "timestamp_sled",        "0",            NULL, true},
  {SV_POR_CFG,          "server_por_cfg",        "on",           key_func_por_policy},
  {SV_SNR_HEALTH,       "server_sensor_health",  "1",            NULL, true},
  {SWB_SNR_HEALTH,      "swb_sensor_health",     "1",            NULL, true},
  {HGX_SNR_HEALTH,      "hgx_sensor_health",     "1",            NULL, true},
  {NIC0_SNR_HEALTH,     "nic0_sensor_health",    "1",            NULL, true},
  {NIC1_SNR_HEALTH,     "nic1_sensor_health",    "1",            NULL, true},
  {VPDB_SNR_HEALTH,     "vpdb_sensor_health",    "1",            NULL, true},
  {HPDB_SNR_HEALTH,     "hpdb_sensor_health",    "1",            NULL, true},
  {FAN_BP1_SNR_HEALTH,  "fan_bp1_sensor_health", "1",            NULL, true},
  {FAN_BP2_SNR_HEALTH,  "fan_bp2_sensor_health", "1",            NULL, true},
  {SCM_SNR_HEALTH,      "scm_sensor_health",     "1",            NULL, true},
  {HSC_SNR_HEALTH,      "hsc_sensor_health",     "1",            NULL, false},
  {SHSC_SNR_HEALTH,     "swb_hsc_sensor_health", "1",            NULL, false},
  {SV_SEL_ERR,          "server_sel_error",      "1",            NULL, true},
  {SV_BOOT_ORDER,       "server_boot_order",     "0100090203ff", NULL, true},
  {CPU0_PPIN,           "cpu0_ppin",             "",             NULL, true},
  {CPU1_PPIN,           "cpu1_ppin",             "",             NULL, true},
  {NTP_SERVER,          "ntp_server",            "",             NULL, true},
  /* Add more Keys here */
  {LAST_ID, LAST_KEY, LAST_KEY, NULL, true} /* This is the last key of the list */
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
pal_set_def_key_value(void) {
  int i;
  char key[MAX_KEY_LEN] = {0};

  for (i = 0; key_cfg[i].id != LAST_ID; i++) {
    if (kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) {
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed.");
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
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN];
  int i, comp_code = CC_SUCCESS;

  *res_len = 0;
  if (req_len > SIZE_CPU_PPIN*MAX_CPU_CNT)
    req_len = SIZE_CPU_PPIN*MAX_CPU_CNT;

  for (i = 0; i < req_len; i++) {
    sprintf(&str[(i%SIZE_CPU_PPIN)*2], "%02x", req_data[i]);

    if (!((i+1)%MAX_CPU_CNT)) {
      sprintf(key, "cpu%d_ppin", i/SIZE_CPU_PPIN);
      if (pal_set_key_value(key, str)) {
        comp_code = CC_UNSPECIFIED_ERROR;
        break;
      }
    }
  }

  return comp_code;
}

int
pal_get_syscfg_text(char *text) {
  int cnt=0;
  char key[MAX_KEY_LEN], value[MAX_VALUE_LEN], entry[MAX_VALUE_LEN];
  char *key_prefix = "sys_config/";
  char *buf = NULL;
  char str[16];

  uint8_t cpu_num=MAX_CPU_CNT;
  uint8_t cpu_index=0;
  uint8_t cpu_core_num=0;
  float cpu_speed=0;

  uint8_t dimm_num=MAX_DIMM_NUM;
  uint8_t dimm_index=0;
  uint16_t dimm_speed=0;
  uint32_t dimm_capacity=0;
  size_t ret;
  static char *dimm_label[32] = {
    "A0", "C0", "A1", "C1", "A2", "C2", "A3", "C3", "A4", "C4", "A5", "C5", "A6", "C6", "A7", "C7",
    "B0", "D0", "B1", "D1", "B2", "D2", "B3", "D3", "B4", "D4", "B5", "D5", "B6", "D6", "B7", "D7",
  };

  if (text == NULL)
    return -1;

  // Clear string buffer
  text[0] = '\0';

  // CPU information
  for (cpu_index = 0; cpu_index < cpu_num; cpu_index++) {
    if (!is_cpu_socket_occupy(cpu_index))
      continue;
    sprintf(entry, "CPU%d:", cpu_index);

    // Processor#
    snprintf(key, MAX_KEY_LEN, "%sfru1_cpu%d_product_name", key_prefix, cpu_index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 ) {
      cnt = 0;
      buf = strtok(value, " ");
      while( buf != NULL ) {
        if(cnt == 3) { //Full Name
        // strncpy(str, buf, sizeof(str));
         snprintf(&entry[strlen(entry)], sizeof(str), "%s", buf);
         break;
        }
        cnt++;
        buf = strtok(NULL, " ");
      }
    }

    // Frequency & Core Number
    snprintf(key, MAX_KEY_LEN, "%sfru1_cpu%d_basic_info", key_prefix, cpu_index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 5) {
      cpu_speed = (float)(value[4] << 8 | value[3])/1000;
      cpu_core_num = value[0];

      sprintf(&entry[strlen(entry)], "/%.1fG/%dc", cpu_speed, cpu_core_num);
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }


  for (dimm_index=0; dimm_index<dimm_num; dimm_index++) {
    sprintf(entry, "CPU%d_MEM%s:", dimm_index/(MAX_DIMM_NUM/2), dimm_label[dimm_index % MAX_DIMM_NUM]);

    // Check Present
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_location", key_prefix, dimm_index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      // Skip if not present
      if (value[0] != 0x01)
        continue;
    }

    // Module Manufacturer ID
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_manufacturer_id", key_prefix, dimm_index);
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
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_speed",key_prefix, dimm_index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 6) {
      dimm_speed =  value[1]<<8 | value[0];
      dimm_capacity = (value[5]<<24 | value[4]<<16 | value[3]<<8 | value[2])/1024;
      sprintf(&entry[strlen(entry)], "%dMhz/%uGB", dimm_speed, dimm_capacity);
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }

  return 0;
}

void
pal_dump_key_value(void) {
  int i;
  char value[MAX_VALUE_LEN];

  if(is_mb_hsc_module())
    key_cfg[HSC_SNR_HEALTH].enable = true;

  if(is_swb_hsc_module())
    key_cfg[SHSC_SNR_HEALTH].enable = true;

  for (i = 0; key_cfg[i].id != LAST_ID; i++) {
    if (key_cfg[i].enable) {
      printf("%s:", key_cfg[i].name);
      memset(value, 0, sizeof(value));
      if (kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST) < 0) {
        printf("\n");
      } else {
        printf("%s\n", value);
      }
    }
  }
}

