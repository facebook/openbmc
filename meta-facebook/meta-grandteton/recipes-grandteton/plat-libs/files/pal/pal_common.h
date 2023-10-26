#ifndef __PAL_COMMON_H__
#define __PAL_COMMON_H__

#include <openbmc/hal_fruid.h>

enum {
  MAIN_SOURCE = 0,
  SECOND_SOURCE = 1,
  THIRD_SOURCE = 2,
  DISCRETE_SOURCE = 3,
};

enum {
  MB_HSC_SOURCE,
  MB_VR_SOURCE,
  MB_ADC_SOURCE,
};

enum {
  SWB_HSC_SOURCE,
  SWB_VR_SOURCE,
  SWB_NIC_SOURCE,
};

enum {
  VPDB_HSC_SOURCE,
  VPDB_BRICK_SOURCE,
  VPDB_ADC_SOURCE,
  VPDB_RSENSE_SOURCE,
};

enum {
  HPDB_HSC_SOURCE,
  HPDB_ADC_SOURCE,
  HPDB_RSENSE_SOURCE,
};

enum {
  FAN_BP1_FAN_SOURCE,
  FAN_BP1_LED_SOURCE,

};

enum {
  FAN_BP2_FAN_SOURCE,
  FAN_BP2_LED_SOURCE,
};

bool fru_presence(uint8_t fru_id, uint8_t *status);
bool is_cpu_socket_occupy(uint8_t cpu_idx);
bool pal_skip_access_me(void);
int read_device(const char *device, int *value);
bool check_pwron_time(int time);
bool pal_bios_completed(uint8_t fru);
bool is_dimm_present(uint8_t dimm_id);
int get_comp_source(uint8_t fru, uint8_t comp_id, uint8_t* source);
int get_acb_vr_source(uint8_t* source);
bool is_mb_hsc_module(void);
bool is_swb_hsc_module(void);
bool sgpio_valid_check(void);
int read_cpld_health(uint8_t fru, uint8_t sensor_num, float *value);
int mb_retimer_lock(void);
void mb_retimer_unlock(int lock);
int pal_get_board_rev_id(uint8_t fru, uint8_t *id);
int pal_get_board_sku_id(uint8_t fru, uint8_t *id);
bool gta_expansion_board_present(uint8_t fru_id, uint8_t *status);
bool gta_check_exmax_prsnt(uint8_t cable_id);
void get_dimm_present_info(uint8_t fru, bool *dimm_sts_list);
#endif
