#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

bool is_server_off(void);
int is_cpu_present(uint8_t fru, uint8_t cpu_id);
int pal_get_rst_btn(uint8_t *status);

#endif
