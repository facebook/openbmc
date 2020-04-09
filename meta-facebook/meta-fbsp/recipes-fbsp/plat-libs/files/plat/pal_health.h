#ifndef __PAL_HEALTH_H__
#define __PAL_HEALTH_H__

int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);

#endif

