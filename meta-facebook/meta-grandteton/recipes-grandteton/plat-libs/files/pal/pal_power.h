#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__
#include <stdint.h>

enum DEV_POWER_ACTION {
  GET_DEV_POWER = 0x0,
  SET_DEV_POWER = 0x1,
};

enum DEV_POWER_STATUS {
  DEVICE_POWER_OFF = 0x0,
  DEVICE_POWER_ON = 0x1,
};

bool is_server_off(void);
int pal_is_bmc_por(void);
int pal_power_button_override(uint8_t fru_id);
int pal_set_fru_power(uint8_t fru, uint8_t cmd);
int pal_get_fru_power(uint8_t fru, uint8_t *status);


#endif

