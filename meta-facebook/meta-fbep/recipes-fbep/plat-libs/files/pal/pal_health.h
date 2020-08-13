#ifndef __PAL_HEALTH_H__
#define __PAL_HEALTH_H__

#define KEY_MB_SNR_HEALTH  "mb_sensor_health"
#define KEY_MB_SEL_ERROR   "mb_sel_error"
#define KEY_PDB_SNR_HEALTH "pdb_sensor_health"

int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);

#endif

