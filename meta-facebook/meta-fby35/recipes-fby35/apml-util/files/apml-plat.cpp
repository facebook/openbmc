#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <facebook/bic.h>
#include "apml-extra.h"
#define MAX_APML_DATA_LENGTH 4

#define POWER_CAPPING_COMMAND "apml-util slot%d 0xE0 0x2E 0x15 0xa0 0x00 0x00 0x02 0x%02x 0x%02x 0x%02x 0x%02x"

int
util_read_mca_reg(uint8_t slot_id, char* thread_addr, char* mca_reg_addr[]) {
  int i, ret, retry = 3;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t thread = (uint8_t)strtoul(thread_addr, NULL, 0);

  tbuf[tlen++] = 0x15;
  tbuf[tlen++] = 0xa0;
  tbuf[tlen++] = 0x00;
  tbuf[tlen++] = 0x02; // MCR
  tbuf[tlen++] = thread << 1; //Bits[7:1] slecet the thread
  for (i = 0; i < 4; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(mca_reg_addr[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_data_wrapper(slot_id, 0x38, 0x2E, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    retry--;
  }
  if (ret) {
    printf("Send APML request, BIC no response!\n");
    return -1;
  }

  sleep(1);

  retry = 3;
  tlen = 0;
  tbuf[tlen++] = 0x15;
  tbuf[tlen++] = 0xa0;
  tbuf[tlen++] = 0x00;
  tbuf[tlen++] = rbuf[3];
  while (retry >= 0) {
    ret = bic_data_wrapper(slot_id, 0x38, 0x2F, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    retry--;
    sleep(1);
  }
  if (ret) {
    printf("Get APML response, BIC no response!\n");
    return -1;
  }
   for (i = 0; i < rlen; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", rbuf[i]);
  }
  printf("\n");


  return 0;
}

int
util_power_capping(uint8_t slot_id, uint32_t limitPower) {
  char power_capping_command[128];
  uint8_t limit_data[MAX_APML_DATA_LENGTH] = {0};

  memcpy(limit_data, &limitPower, MAX_APML_DATA_LENGTH*sizeof(uint8_t));
  snprintf(power_capping_command, sizeof(power_capping_command), POWER_CAPPING_COMMAND, slot_id, limit_data[0], limit_data[1], limit_data[2], limit_data[3]);
  if (0 != system(power_capping_command))
  {
    return -1;
  }

  return 0;
}

int
get_fru_id(char *str, uint8_t *fru) {

  if (!strcmp(str, "slot1")) {
    *fru = 1;
  } else if (!strcmp(str, "slot3")) {
    *fru = 3;
  } else {
    return -1;
  }

  return 0;
}

void
print_usage(void) {
  printf("Usage: apml-util <slot1|slot3> <[0..n]data_bytes_to_send>\n");
  printf("Usage: apml-util <slot1|slot3> <option>\n");
}
