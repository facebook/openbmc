#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <plat.h>

int get_dev_bus(const char *fru, uint8_t retimerId)
{
  if ( !strcmp(fru, "mb") )
    return RT_DEV_INFO[retimerId].bus;
  else
    return -1;
}

int get_dev_addr(const char *fru, uint8_t retimerId)
{
  if ( !strcmp(fru, "mb") )
    return RT_DEV_INFO[retimerId].addr;
  else
    return -1;
}

