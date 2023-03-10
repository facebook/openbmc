#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__
#include <stdint.h>

bool is_server_off(void);
int pal_is_bmc_por(void);
int pal_power_button_override(uint8_t fru_id);


#endif

