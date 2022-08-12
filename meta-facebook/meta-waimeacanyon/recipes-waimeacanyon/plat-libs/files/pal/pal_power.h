#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

#include <stdint.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/libgpio.h>

int pal_get_server_12v_power(uint8_t fru, uint8_t *status);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_set_server_power(uint8_t fru, uint8_t cmd);



#endif //__PAL_POWER_H__
