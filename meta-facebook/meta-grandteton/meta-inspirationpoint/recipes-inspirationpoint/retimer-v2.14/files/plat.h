#include <syslog.h>

typedef struct {
  int bus;
  int addr;
} RETIMER_DEV;

RETIMER_DEV RT_DEV_INFO[] = {
  {60, 0x24},
  {61, 0x24},
  {62, 0x24},
  {63, 0x24},
  {64, 0x24},
  {65, 0x24},
  {66, 0x24},
  {67, 0x24},
};
