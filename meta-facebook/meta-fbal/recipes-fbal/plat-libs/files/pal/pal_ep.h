#ifndef __PAL_EP_H__
#define __PAL_EP_H__

#include <openbmc/ipmi.h>

#define EP_I2C_BUS_NUMBER                (6)
#define EP_I2C_SLAVE_ADDR                (0x2C)

int pal_ep_set_system_mode(uint8_t);
int pal_ep_sled_cycle(void);
bool is_ep_present(void);

#endif

