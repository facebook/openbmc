#ifndef __HAL_FRUID_H__
#define __HAL_FRUID_H__

int hal_read_pldm_fruid(uint8_t fru_id, const char *path, int fru_size);
int hal_write_pldm_fruid(uint8_t fru_id, const char *path);
#endif

