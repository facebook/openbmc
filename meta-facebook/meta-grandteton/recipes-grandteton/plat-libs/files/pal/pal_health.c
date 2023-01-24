#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>
#include <openbmc/kv.h>
#include "pal_cfg.h"
#include "pal_def.h"

static int
pal_get_sensor_health_key(uint8_t fru, char *key) {

  switch (fru) {
    case FRU_MB:
      sprintf(key, "server_sensor_health");
      break;
    case FRU_SWB:
      sprintf(key, "swb_sensor_health");
      break;
    case FRU_HGX:
      sprintf(key, "hgx_sensor_health");
      break;
    case FRU_NIC0:
      sprintf(key, "nic0_sensor_health");
      break;
    case FRU_NIC1:
      sprintf(key, "nic1_sensor_health");
      break;
    case FRU_VPDB:
      sprintf(key, "vpdb_sensor_health");
      break;
    case FRU_HPDB:
      sprintf(key, "hpdb_sensor_health");
      break;
    case FRU_FAN_BP1:
      sprintf(key, "fan_bp1_sensor_health");
      break;
    case FRU_FAN_BP2:
      sprintf(key, "fan_bp2_sensor_health");
    break;
    case FRU_SCM:
      sprintf(key, "scm_sensor_health");
    break;
    default:
      return -1;
  }

  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  if (pal_get_sensor_health_key(fru, key))
    return -1;

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  if (pal_get_sensor_health_key(fru, key)) {
    return ERR_NOT_READY;
  }
  ret = pal_get_key_value(key, cvalue);

  if (ret) {
    syslog(LOG_INFO, "err0 pal_get_fru_health(%d): getting value for %s failed\n", fru, key);
    return ret;
  }

  *value = atoi(cvalue);

  if (fru != FRU_MB)
    return 0;

  // If MB, get SEL error status.
  sprintf(key, "server_sel_error");
  memset(cvalue, 0, MAX_VALUE_LEN);

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    syslog(LOG_INFO, "err1 pal_get_fru_health(%d): getting value for %s failed\n", fru, key);
    return ret;
  }

  *value = *value & atoi(cvalue);
  return 0;
}

