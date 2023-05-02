#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include <algorithm>
#include <cstdio>

uint8_t plat_num_cpu(uint8_t /* fru_id */) {
  return 1;
}

int plat_read_psb_eeprom(uint8_t fru, uint8_t /*cpu*/, uint8_t *rbuf, size_t rsize) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};

  if (fru != 1)
    return -1;

  snprintf(key, sizeof(key), "slot%d_psb_config_raw", fru);
  if ((kv_get(key, value, NULL, KV_FPERSIST) < 0) && (errno == ENOENT)) {

  } else {
    memcpy(rbuf, value, rsize);
  }

  return 0;
}
