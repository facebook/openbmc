#ifndef __FBAL_FRUID_H__
#define __FBAL_FRUID_H__

int fbal_read_pdb_fruid(uint8_t fru_id, const char *path, int fru_size);
int fbal_write_pdb_fruid(uint8_t fru_id, const char *path);

#endif

