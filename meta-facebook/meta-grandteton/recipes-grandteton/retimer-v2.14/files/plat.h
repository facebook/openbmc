#include <syslog.h>

typedef struct {
  int bus;
  int addr;
} RETIMER_DEV;

RETIMER_DEV HGX_RT_DEV_INFO[] = {
  { 9, 0x33},
  { 9, 0x44},
  { 9, 0x45},
  { 9, 0x46},
  { 9, 0x47},
  { 9, 0x48},
  { 9, 0x49},
  { 9, 0x0B},
};
