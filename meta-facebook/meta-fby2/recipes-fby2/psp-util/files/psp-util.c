#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <facebook/bic.h>

#define MAX_APML_DATA_LENGTH 4
#define POWER_CAPPING_COMMAND "psp-util slot%d 0xE0 0x2E 0x15 0xa0 0x00 0x02 0x%02x 0x%02x 0x%02x 0x%02x"

static const char *option_list[] = {
  "--power_capping [limit_watts]"
};

static void
print_usage_help(void) {
  printf("Usage: psp-util <slot1|slot2|slot3|slot4> <[0..n]data_bytes_to_send>\n");
  printf("Usage: psp-util <slot1|slot2|slot3|slot4> <option>\n");
  printf("       option:\n");
  for (int i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = 2;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0)
      break;

    retry--;
  }
  if (ret) {
    printf("BIC no response!\n");
    return ret;
  }

  for (i = 0; i < rlen; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
}

static int
util_power_capping(uint8_t slot_id, uint32_t limitPower) {
  char power_capping_command[64];
  uint8_t limit_data[MAX_APML_DATA_LENGTH] = {0};

  memcpy(limit_data, &limitPower, MAX_APML_DATA_LENGTH*sizeof(uint8_t));
  snprintf(power_capping_command, sizeof(power_capping_command), POWER_CAPPING_COMMAND, slot_id, limit_data[0], limit_data[1], limit_data[2], limit_data[3]);
  system(power_capping_command);

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t slot_id;
  int ret = 0;

  if (argc < 3) {
    print_usage_help();
    return -1;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else {
    print_usage_help();
    return -1;
  }

  if (!strcmp(argv[2], "--power_capping")) {
    if (argc != 4) {
      print_usage_help();
      return -1;
    }
    //Watts converted to milliwatts
    ret = util_power_capping(slot_id, atoi(argv[3]) * 1000 );
  } else if (argc >= 4) {
      return process_command(slot_id, (argc - 2), (argv + 2));
  } else {
    print_usage_help();
    return -1;
  }

  return ret;
}