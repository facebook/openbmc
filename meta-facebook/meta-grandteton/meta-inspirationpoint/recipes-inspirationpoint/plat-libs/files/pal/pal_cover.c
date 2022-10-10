#include <stdio.h>
#include <syslog.h>
#include "pal.h"

int
parse_mce_error_sel(uint8_t fru, uint8_t *event_data, char *error_log) {
  uint8_t *ed = &event_data[3];
  uint8_t bank_num;
  uint8_t error_type = ((ed[1] & 0x60) >> 5);
  char temp_log[512] = {0};

  strcpy(error_log, "");
  if ((ed[0] & 0x0F) == 0x0B) { //Uncorrectable
    switch (error_type) {
      case 0x00:
        strcat(error_log, "Uncorrected Recoverable Error, ");
        break;
      case 0x01:
        strcat(error_log, "Uncorrected Thread Fatal Error, ");
        break;
      case 0x02:
        strcat(error_log, "Uncorrected System Fatal Error, ");
        break;
      default:
        strcat(error_log, "Unknown, ");
        break;
    }
  } else if((ed[0] & 0x0F) == 0x0C) { //Correctable
    switch (error_type) {
      case 0x00:
        strcat(error_log, "Correctable Error, ");
        break;
      case 0x01:
        strcat(error_log, "Deferred Error, ");
        break;
      default:
        strcat(error_log, "Unknown, ");
        break;
    }
  }
  bank_num = ed[1] & 0x1F;
  snprintf(temp_log, sizeof(temp_log), "Bank Number %d, ", bank_num);
  strcat(error_log, temp_log);

  snprintf(temp_log, sizeof(temp_log), "CPU %d, Core %d", ((ed[2] & 0xF0) >> 4), (ed[2] & 0x0F));
  strcat(error_log, temp_log);

  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    case MACHINE_CHK_ERR:
      parse_mce_error_sel(fru, event_data, error_log);
      parsed = true;
      break;
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