#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include "pal.h"
#include "pal_health.h"

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {
  return ERR_NOT_READY;
}

