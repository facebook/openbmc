#ifndef _MAX10_I2C_UPDATE_H_
#define _MAX10_I2C_UPDATE_H_

#include <stdbool.h>

typedef enum{
  Sector_CFM0,
  Sector_CFM1,
  Sector_CFM2,
  Sector_UFM0,
  Sector_UFM1,
  Sector_NotSet,
}SectorType_t;

void Max10Update_Init(bool bypass_update, int slot_id, unsigned char intf, char *i2cdev);

int Max10_Update_Rpd(unsigned char *rpd_file, SectorType_t sectorType);
void Max10_Show_Ver(void);

typedef enum{
  CONFIG_0,
  CONFIG_1,
}Max10_Config_Sel_t;

#endif //_MAX10_I2C_UPDATE_H_




