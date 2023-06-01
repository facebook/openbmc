#ifndef __HAL_FRUID_H__
#define __HAL_FRUID_H__

typedef struct {
  uint8_t fru_id;
  uint8_t root_fru;
  uint8_t bus_id;
  uint8_t bic_eid;
  uint8_t dev_id;
} bic_intf;

int hal_read_pldm_fruid(bic_intf fru_bic_info, const char *path, int fru_size);
int hal_write_pldm_fruid(bic_intf fru_bic_info, const char *path);
#endif

