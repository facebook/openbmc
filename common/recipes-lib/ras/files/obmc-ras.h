#ifndef _OBMC_RAS_H_
#define _OBMC_RAS_H_

#include <openbmc/libgpio.h>


void log_gpio_change(gpiopoll_pin_t *gp, gpio_value_t value, useconds_t log_delay, bool active_low);
void fast_prochot_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr);
void fast_prochot_init(gpiopoll_pin_t *gp, gpio_value_t value);

#endif	//_OBMC_RAS_H_
