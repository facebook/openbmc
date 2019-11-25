#ifndef _MAX10_I2C_UPDATE_H_
#define _MAX10_I2C_UPDATE_H_

#ifdef __cplusplus
extern "C" {
#endif


#define MAX10_RETRY_TIMEOUT  (100)

typedef enum{
	CONFIG_0,
	CONFIG_1,
} MAX10_CFG_SEL_T;


/******************************************************************************/
extern struct cpld_dev_info altera_dev_list[2];

int max10_iic_get_fw_version(unsigned char id, unsigned char* ver);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //_MAX10_I2C_UPDATE_H_

