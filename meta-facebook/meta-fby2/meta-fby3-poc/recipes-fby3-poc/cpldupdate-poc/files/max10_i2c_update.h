#ifndef _MAX10_I2C_UPDATE_H_
#define _MAX10_I2C_UPDATE_H_

typedef enum{
	Sector_CFM0,
	Sector_CFM1,
	Sector_CFM2,
	Sector_UFM0,
	Sector_UFM1,
	Sector_NotSet,

}SectorType_t;

void Max10Update_Init(int i2c_file, uint8_t addr);

int Max10_Update_Rpd(unsigned char* rpd_file, SectorType_t sectorType , int cfm_start_addr, int cfm_end_addr );

typedef enum{
	CONFIG_0,
	CONFIG_1,
}Max10_Config_Sel_t;

int Max10_TrigReconfig(void);
Max10_Config_Sel_t Max10_DualBoot_ConfigSel_Get(void);
int Max10_DualBoot_ConfigSel_Set(Max10_Config_Sel_t img_no);








#endif //_MAX10_I2C_UPDATE_H_




