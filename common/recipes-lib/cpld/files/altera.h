#ifndef _MAX10_I2C_UPDATE_H_
#define _MAX10_I2C_UPDATE_H_

#define MAX10_RETRY_TIMEOUT  (100)

enum {
	CFM_IMAGE_NONE = 0,
	CFM_IMAGE_1,
	CFM_IMAGE_2,
};

typedef enum{
	CONFIG_0,
	CONFIG_1,
} MAX10_CFG_SEL_T;


/******************************************************************************/
extern struct cpld_dev_info altera_dev_list[2];

#endif //_MAX10_I2C_UPDATE_H_

