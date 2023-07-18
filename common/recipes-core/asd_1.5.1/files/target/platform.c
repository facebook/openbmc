#include <safe_str_lib.h>

#include "../include/logging.h"
#include "target_handler.h"
#include "jtag_handler.h"

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

#ifdef IPMB_JTAG_HNDLR
STATUS jtag_ipmb_wrapper(uint8_t fru, uint8_t netfn, uint8_t cmd,
                         uint8_t *txbuf, size_t txlen,
                         uint8_t *rxbuf, size_t *rxlen)
{
    return ST_ERR;
}
#endif

int pin_get_gpio(uint8_t fru, Target_Control_GPIO *gpio)
{
    return -1;
}

int pin_set_gpio(uint8_t fru, Target_Control_GPIO *gpio, int val)
{
    return -1;
}

STATUS platform_init(Target_Control_Handle *state)
{
    return ST_ERR;
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
  return NULL;
}
