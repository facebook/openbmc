#ifndef _MAX10_I2C_UPDATE_H_
#define _MAX10_I2C_UPDATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MAX10_RETRY_TIMEOUT  (100)

typedef enum {
  CONFIG_0,
  CONFIG_1,
} MAX10_CFG_SEL_T;


/******************************************************************************/
int max10_status_read(void);
int max10_unprotect_sector(int sector_id);
int max10_erase_sector(int sector_id);
int max10_flash_write(int address, int data);
int max10_protect_sectors(void);
extern struct cpld_dev_info altera_dev_list[4];

#ifdef __cplusplus
} // extern "C"
#endif

#endif //_MAX10_I2C_UPDATE_H_
