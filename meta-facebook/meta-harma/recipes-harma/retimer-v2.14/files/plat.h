#include <syslog.h>

typedef struct {
  int bus;
  int addr;
} RETIMER_DEV;

RETIMER_DEV RT_DEV_INFO[] = {
  {12, 0x24},
  {21, 0x24},
};
