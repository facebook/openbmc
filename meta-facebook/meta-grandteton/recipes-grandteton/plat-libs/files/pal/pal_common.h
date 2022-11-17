#ifndef __PAL_COMMON_H__
#define __PAL_COMMON_H__

enum {
  MAIN_SOURCE = 0,
  SECOND_SOURCE = 1,
  THIRD_SOURCE = 2,
};

enum {
  MB_HSC_SOURCE,
  MB_VR_SOURCE,
  MB_ADC_SOURCE,
};

enum {
  VPDB_HSC_SOURCE,
  VPDB_BRICK_SOURCE,
};

enum {
  HPDB_HSC_SOURCE,
};

enum {
  FAN_BP0_FAN_SOURCE,
};

enum {
  FAN_BP1_FAN_SOURCE,
};

bool swb_presence(void);
bool hgx_presence(void);
bool fru_presence(void);
bool is_cpu_socket_occupy(uint8_t cpu_idx);
bool pal_skip_access_me(void);
int read_device(const char *device, int *value);
bool check_pwron_time(int time);
bool pal_bios_completed(uint8_t fru);
bool is_dimm_present(uint8_t dimm_id);
int get_comp_source(uint8_t fru, uint8_t comp_id, uint8_t* source);
bool is_mb_hsc_module(void);
bool is_swb_hsc_module(void);
bool sgpio_valid_check(void);
#endif
