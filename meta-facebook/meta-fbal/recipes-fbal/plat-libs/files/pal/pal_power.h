#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

bool is_server_off(void);
int pal_sled_cycle(void);
int pal_bios_update_ac(void);
int pal_check_cpld_power_fail(void);
int pal_recfg_sled_cycle(void);
uint8_t pal_set_power_policy(uint8_t *pwr_policy, uint8_t *res_data);


#endif

