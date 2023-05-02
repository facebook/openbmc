#include <stdio.h>
#include <syslog.h>
#include "pal.h"

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    default:
      parsed = false;
      break;
  }

  if (parsed == true) {
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

int
pal_clear_psb_cache(void) {
  return 0;
}

int
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t index = sel[11] & 0x01;
  char* yellow_track[] = {
    "Corrected Cache Error",
    "Persistent Cache Fault"
  };

  // error_log size 256
  snprintf(error_log, 256, "%s", yellow_track[index]);

  return 0;
}