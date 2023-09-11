#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include "pal.h"
#include "pal_health.h"

static int pal_get_sensor_health_key(uint8_t fru, char *key)
{
  switch (fru) {
    case FRU_MB:
      sprintf(key, KEY_MB_SNR_HEALTH);
      break;
    case FRU_PDB:
      sprintf(key, KEY_PDB_SNR_HEALTH);
      break;
    case FRU_ASIC0:
      sprintf(key, KEY_ASIC0_SNR_HEALTH);
      break;
    case FRU_ASIC1:
      sprintf(key, KEY_ASIC1_SNR_HEALTH);
      break;
    case FRU_ASIC2:
      sprintf(key, KEY_ASIC2_SNR_HEALTH);
      break;
    case FRU_ASIC3:
      sprintf(key, KEY_ASIC3_SNR_HEALTH);
      break;
    case FRU_ASIC4:
      sprintf(key, KEY_ASIC4_SNR_HEALTH);
      break;
    case FRU_ASIC5:
      sprintf(key, KEY_ASIC5_SNR_HEALTH);
      break;
    case FRU_ASIC6:
      sprintf(key, KEY_ASIC6_SNR_HEALTH);
      break;
    case FRU_ASIC7:
      sprintf(key, KEY_ASIC7_SNR_HEALTH);
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_set_sensor_health(uint8_t fru, uint8_t value)
{
  char key[MAX_KEY_LEN] = {0};

  if (pal_get_sensor_health_key(fru, key) < 0)
    return -1;

  return pal_set_key_value(key, value == FRU_STATUS_GOOD? "1": "0");
}

int pal_get_fru_health(uint8_t fru, uint8_t *value)
{
  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  if (pal_get_sensor_health_key(fru, key))
    return ERR_NOT_READY;

  ret = pal_get_key_value(key, cvalue);

  if (ret) {
    syslog(LOG_INFO, "err0 pal_get_fru_health(%d): getting value for %s failed\n", fru, key);
    return ret;
  }

  *value = atoi(cvalue);

  if (fru != FRU_MB)
    return 0;

  // If Baseboard, get SEL error status.
  sprintf(key, KEY_MB_SEL_ERROR);
  memset(cvalue, 0, MAX_VALUE_LEN);

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    syslog(LOG_INFO, "err1 pal_get_fru_health(%d): getting value for %s failed\n", fru, key);
    return ret;
  }

  *value = *value & atoi(cvalue);
  return 0;
}

int pal_is_bmc_por(void) {
  FILE *fp;
  int por = 0;

  fp = fopen("/tmp/ast_por", "r");
  if (fp != NULL) {
    if (fscanf(fp, "%d", &por) != 1) {
      por = 0;
    }
    fclose(fp);
  }

  return (por)? 1:0;
}

void pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%lld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

