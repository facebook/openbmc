#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_get_server_12v_power(uint8_t slot_id, uint8_t *status);

#endif
