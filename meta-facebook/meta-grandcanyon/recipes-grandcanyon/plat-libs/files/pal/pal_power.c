#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {

  uint8_t completion_code = CC_SUCCESS; // Fill response with default values
  char key[MAX_KEY_LEN] = {0};
  unsigned char policy = *pwr_policy & 0x07;  // Power restore policy
  
  snprintf(key, MAX_KEY_LEN, "server_por_cfg");
  switch (policy)
  {
    case POWER_CFG_OFF:
      if (pal_set_key_value(key, "off") != 0) {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case POWER_CFG_LPS:
      if (pal_set_key_value(key, "lps") != 0) {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case POWER_CFG_ON:
      if (pal_set_key_value(key, "on") != 0) {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case POWER_CFG_UKNOWN:
      // no change (just get present policy support)
      break;
    default:
      completion_code = CC_PARAM_OUT_OF_RANGE;
      break;
  }
  return completion_code;
}
