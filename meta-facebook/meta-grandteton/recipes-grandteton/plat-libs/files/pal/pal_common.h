#ifndef __PAL_COMMON_H__
#define __PAL_COMMON_H__

bool get_bic_ready(void); 
bool is_cpu_socket_occupy(uint8_t cpu_idx);
bool pal_skip_access_me(void);
int read_device(const char *device, int *value);
bool check_pwron_time(int time);
bool pal_bios_completed(uint8_t fru);

#endif
