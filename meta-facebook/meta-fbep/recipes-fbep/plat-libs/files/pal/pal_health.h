#ifndef __PAL_HEALTH_H__
#define __PAL_HEALTH_H__

#define KEY_MB_SNR_HEALTH  "mb_sensor_health"
#define KEY_MB_SEL_ERROR   "mb_sel_error"
#define KEY_PDB_SNR_HEALTH "pdb_sensor_health"
#define KEY_ASIC0_SNR_HEALTH "asic0_sensor_health"
#define KEY_ASIC1_SNR_HEALTH "asic1_sensor_health"
#define KEY_ASIC2_SNR_HEALTH "asic2_sensor_health"
#define KEY_ASIC3_SNR_HEALTH "asic3_sensor_health"
#define KEY_ASIC4_SNR_HEALTH "asic4_sensor_health"
#define KEY_ASIC5_SNR_HEALTH "asic5_sensor_health"
#define KEY_ASIC6_SNR_HEALTH "asic6_sensor_health"
#define KEY_ASIC7_SNR_HEALTH "asic7_sensor_health"

int pal_set_sensor_health(uint8_t fru, uint8_t value);
int pal_get_fru_health(uint8_t fru, uint8_t *value);

#endif

