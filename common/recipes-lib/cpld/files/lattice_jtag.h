#ifndef _LATTICE_JTAG_H_
#define _LATTICE_JTAG_H_

// #include "lattice.h"

enum
{
  Only_CF = 0,
  Both_CF_UFM = 1,
};

enum
{
  CHECK_BUSY = 0,
  CHECK_STATUS = 1,
};

int cpld_dev_open_jtag(void *_attr);
int cpld_dev_close_jtag(void);
int common_cpld_Get_id_jtag(unsigned int *dev_id);
int common_cpld_Get_Ver_jtag(unsigned int *ver);
int LCMXO2Family_cpld_update_jtag(FILE *jed_fd, char* key, char is_signed);


#endif // _LATTICE_JTAG_H_
