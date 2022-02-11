#ifndef _EEPROM_H_
#define _EEPROM_H_
#include <openbmc/obmc-i2c.h>

#define EEPROM_PATH I2C_SYSFS_DEV_ENTRY(6-0054, eeprom)
#define EEPROM_OFFSET 1024

#endif
