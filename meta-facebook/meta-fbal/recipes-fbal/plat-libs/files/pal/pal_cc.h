#ifndef __PAL_CC_H__
#define __PAL_CC_H__

#include <openbmc/ipmi.h>

#define CC_I2C_BUS_NUMBER                (6)
#define CC_I2C_SLAVE_ADDR                (0x2E)

int pal_cc_sled_cycle(void);
bool is_cc_present(void);

#endif