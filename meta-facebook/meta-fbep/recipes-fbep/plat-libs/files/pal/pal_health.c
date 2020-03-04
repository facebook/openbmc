#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include "pal.h"

static int pal_get_sensor_health_key(uint8_t fru, char *key)
{
  if (fru != FRU_MB)
      return -1;

  sprintf(key, "fru_sensor_health");

  return 0;
}

int pal_set_sensor_health(uint8_t fru, uint8_t value)
{
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  if (pal_get_sensor_health_key(fru, key))
    return -1;

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int pal_get_fru_health(uint8_t fru, uint8_t *value)
{
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

  // If Baseboard, get SEL error status.
  sprintf(key, "fru_sel_error");
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
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

