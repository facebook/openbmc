#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

bool is_server_off(void);
int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_get_server_power(uint8_t fru, uint8_t *status);
#endif
