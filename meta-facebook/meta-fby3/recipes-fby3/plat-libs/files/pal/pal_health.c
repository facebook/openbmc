#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include "pal.h"
#include "pal_health.h"

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {
  char val[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret = ERR_NOT_READY;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      snprintf(key, MAX_KEY_LEN, "slot%d_sel_error", fru);
      break;
    case FRU_CWC:
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sel_error", fru);
      break;
      /*it's supported*/
    default:
      return ERR_NOT_READY;
  }

  // get sel val
  ret = pal_get_key_value(key, val);
  if ( ret < 0 ) return ret;

  // store it
  *value = atoi(val);

  // reset
  memset(val, 0, MAX_VALUE_LEN);

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      snprintf(key, MAX_KEY_LEN, "slot%d_sensor_health", fru);
      break;
    case FRU_CWC:
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sensor_health", fru);
      break;
    default:
      return ERR_NOT_READY;
  }

  // get snr val
  ret = pal_get_key_value(key, val);
  if ( ret < 0 ) return ret;

  // is fru FRU_BAD?
  *value = *value & atoi(val);
  return PAL_EOK;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {
  char val[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      snprintf(key, MAX_KEY_LEN, "slot%d_sensor_health", fru);
      break;
    case FRU_CWC:
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      snprintf(key, MAX_KEY_LEN, "cwc_fru%d_sensor_health", fru);
      break;
      /*it's supported*/
    default:
      return ERR_NOT_READY;
  }

  snprintf(val, MAX_VALUE_LEN, (value > 0) ? "1": "0");

  return pal_set_key_value(key, val);
}
