#ifndef _LATTICE_I2C_H_
#define _LATTICE_I2C_H_

// #include "lattice.h"

int cpld_dev_open_i2c(void *_attr);
int cpld_dev_close_i2c(void);
int common_cpld_Get_Ver_i2c(unsigned int *ver);
int common_cpld_Get_id_i2c(unsigned int *dev_id);
int XO2XO3Family_cpld_update_i2c(FILE *jed_fd, char* key, char is_signed);
int NXFamily_cpld_update_i2c(FILE *jed_fd, char* key, char is_signed);


#endif // _LATTICE_I2C_H_
