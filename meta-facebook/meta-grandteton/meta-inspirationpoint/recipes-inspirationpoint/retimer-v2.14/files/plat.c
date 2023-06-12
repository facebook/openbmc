#include <syslog.h>
#include <stdint.h>
#include <plat.h>

int get_dev_bus(uint8_t retimerId)
{
  return RT_DEV_INFO[retimerId].bus;
}

int get_dev_addr(uint8_t retimerId)
{
  return RT_DEV_INFO[retimerId].addr;
}

