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
  uint32_t region;
} key_cfg[] = {
  /* name, default value, function */
  {SV_LAST_PWR_ST,      "pwr_server_last_state", "on",           key_func_lps,        true,  KV_FPERSIST},
  {SV_SYSFW_VER,        "fru1_sysfw_ver",        "0",            NULL,                true,  KV_FPERSIST},
  {SLED_IDENTIFY,       "identify_sled",         "off",          NULL,                true,  KV_FPERSIST},
  {SLED_TIMESTAMP,      "timestamp_sled",        "0",            NULL,                true,  KV_FPERSIST},
  {SV_POR_CFG,          "server_por_cfg",        "on",           key_func_por_policy, true,  KV_FPERSIST},
  {SV_SNR_HEALTH,       "server_sensor_health",  "1",            NULL,                true,  0},
  {SWB_SNR_HEALTH,      "swb_sensor_health",     "1",            NULL,                true,  0},
  {HGX_SNR_HEALTH,      "hgx_sensor_health",     "1",            NULL,                true,  0},
  {NIC0_SNR_HEALTH,     "nic0_sensor_health",    "1",            NULL,                true,  0},
  {NIC1_SNR_HEALTH,     "nic1_sensor_health",    "1",            NULL,                true,  0},
  {VPDB_SNR_HEALTH,     "vpdb_sensor_health",    "1",            NULL,                true,  0},
  {HPDB_SNR_HEALTH,     "hpdb_sensor_health",    "1",            NULL,                true,  0},
  {FAN_BP1_SNR_HEALTH,  "fan_bp1_sensor_health", "1",            NULL,                true,  0},
  {FAN_BP2_SNR_HEALTH,  "fan_bp2_sensor_health", "1",            NULL,                true,  0},
  {SCM_SNR_HEALTH,      "scm_sensor_health",     "1",            NULL,                true,  0},
  {HSC_SNR_HEALTH,      "hsc_sensor_health",     "1",            NULL,                false, 0},
  {SHSC_SNR_HEALTH,     "swb_hsc_sensor_health", "1",            NULL,                false, 0},
  {SV_SEL_ERR,          "server_sel_error",      "1",            NULL,                true,  KV_FPERSIST},
  {SV_BOOT_ORDER,       "server_boot_order",     "0100090203ff", NULL,                true,  KV_FPERSIST},
  {CPU0_PPIN,           "cpu0_ppin",             "",             NULL,                true,  KV_FPERSIST},
  {CPU1_PPIN,           "cpu1_ppin",             "",             NULL,                true,  KV_FPERSIST},
  {NTP_SERVER,          "ntp_server",            "",             NULL,                true,  KV_FPERSIST},
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

int
pal_get_key_value(char *key, char *value) {
  int index;

  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  return kv_get(key, value, NULL, key_cfg[index].region);
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

  return kv_set(key, value, 0, key_cfg[index].region);
}

static int
key_func_por_policy(int event, void *arg) {
  switch (event) {
    case KEY_BEFORE_SET:
      if (strcmp((char *)arg, "lps") && strcmp((char *)arg, "on") && strcmp((char *)arg, "off")) {
        return -1;
      }
      break;
  }

  return 0;
}

static int
key_func_lps(int event, void *arg) {
  switch (event) {
    case KEY_BEFORE_SET:
      if (strcmp((char *)arg, "on") && strcmp((char *)arg, "off")) {
        return -1;
      }
      break;
  }

  return 0;
}

int
pal_set_def_key_value(void) {
  int i;
  char key[MAX_KEY_LEN] = {0};

  for (i = 0; key_cfg[i].id != LAST_ID; i++) {
    if (kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | key_cfg[i].region)) {
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
  static const char *key_prefix = "sys_config/";
  static const char *label_gt[32] = {
    "A0", "C0", "A1", "C1", "A2", "C2", "A3", "C3", "A4", "C4", "A5", "C5", "A6", "C6", "A7", "C7",
    "B0", "D0", "B1", "D1", "B2", "D2", "B3", "D3", "B4", "D4", "B5", "D5", "B6", "D6", "B7", "D7",
  };
  static const char *label_gti[24] = {
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11",
    "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10", "B11",
  };
  const char **dimm_label = label_gt;
  char key[MAX_KEY_LEN], value[MAX_VALUE_LEN], entry[MAX_VALUE_LEN];
  char *token;
  uint8_t index, pos;
  uint8_t cpu_name_pos = 3;  // Intel(R) Xeon(R) Platinum "8480C"
  uint8_t dimm_num = sizeof(label_gt)/sizeof(label_gt[0]);
  uint8_t cpu_core;
  uint16_t dimm_speed;
  uint32_t dimm_capacity;
  float cpu_speed;
  int slen;
  size_t ret;

  if (text == NULL) {
    return -1;
  }

  // Clear string buffer
  text[0] = '\0';

  // CPU information
  for (index = 0; index < MAX_CPU_CNT; index++) {
    if (!is_cpu_socket_occupy(index)) {
      continue;
    }
    slen = snprintf(entry, sizeof(entry), "CPU%d:", index);

    // Processor#
    snprintf(key, sizeof(key), "%sfru1_cpu%d_product_name", key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0) {
      if (strncmp(value, "AMD", strlen("AMD")) == 0) {
        cpu_name_pos = 2;  // AMD EPYC "9654" 96-Core Processor
        dimm_label = label_gti;
        dimm_num = sizeof(label_gti)/sizeof(label_gti[0]);
      }

      token = strtok(value, " ");
      for (pos = 0; token && pos < cpu_name_pos; pos++) {
        token = strtok(NULL, " ");
      }
      if (token) {
        slen += snprintf(&entry[slen], sizeof(entry) - slen, "%s", token);
      }
    }

    // Frequency & Core Number
    snprintf(key, sizeof(key), "%sfru1_cpu%d_basic_info", key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 5) {
      cpu_speed = (float)(value[4] << 8 | value[3])/1000;
      cpu_core = value[0];
      slen += snprintf(&entry[slen], sizeof(entry) - slen, "/%.1fG/%dc", cpu_speed, cpu_core);
    }

    snprintf(&entry[slen], sizeof(entry) - slen, "\n");
    strcat(text, entry);
  }

  // DIMM information
  for (index = 0; index < dimm_num; index++) {
    slen = snprintf(entry, sizeof(entry), "CPU%d_MEM%s:", index/(dimm_num/2), dimm_label[index]);

    // Check Present
    snprintf(key, sizeof(key), "%sfru1_dimm%d_location", key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      // Skip if not present
      if (value[0] != 0x01)
        continue;
    }

    // Module Manufacturer ID
    snprintf(key, sizeof(key), "%sfru1_dimm%d_manufacturer_id", key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 2) {
      switch (value[1]) {
        case 0xce:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "Samsung");
          break;
        case 0xad:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "Hynix");
          break;
        case 0x2c:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "Micron");
          break;
        default:
          slen += snprintf(&entry[slen], sizeof(entry) - slen, "unknown");
          break;
      }
    }

    // Speed & Capacity
    snprintf(key, sizeof(key), "%sfru1_dimm%d_speed", key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 6) {
      dimm_speed = value[1]<<8 | value[0];
      dimm_capacity = (value[5]<<24 | value[4]<<16 | value[3]<<8 | value[2])/1024;
      slen += snprintf(&entry[slen], sizeof(entry) - slen, "/%uMhz/%uGB", dimm_speed, dimm_capacity);
    }

    snprintf(&entry[slen], sizeof(entry) - slen, "\n");
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
      if (kv_get(key_cfg[i].name, value, NULL, key_cfg[i].region) < 0) {
        printf("\n");
      } else {
        printf("%s\n", value);
      }
    }
  }
}
