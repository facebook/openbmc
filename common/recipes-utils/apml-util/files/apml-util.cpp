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

#define MAX_APML_DATA_LENGTH 4
#define POWER_CAPPING_OPTION_NUM 4
#define READ_MCA_REG_OPTION_NUM 8
#define POWER_CAPPING_COMMAND "apml-util slot%d 0xE0 0x2E 0x15 0xa0 0x00 0x02 0x%02x 0x%02x 0x%02x 0x%02x"
#define READ_MCA_REG_COMMAND  "apml-util slot%d 0xE0 0x2C 0x15 0xa0 0x00 0x02 %s %s %s %s %s"

static const char *option_list[] = {
  "--power_capping [limit_watts]",
  "--read_mca_reg [thread Addr] [4 bytes for MCA address i.e. 0x01 0x20 0x00 0xC0 for 0xC0002001]"
};

static void
print_usage_help(void) {
  printf("Usage: apml-util <slot1|slot2|slot3|slot4> <[0..n]data_bytes_to_send>\n");
  printf("Usage: apml-util <slot1|slot2|slot3|slot4> <option>\n");
  printf("       option:\n");
  for (size_t i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
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

int
get_fru_id(char *str, uint8_t *fru) {

  if (!strcmp(str, "slot1")) {
    *fru = 1;
  } else if (!strcmp(str, "slot2")) {
    *fru = 2;
  } else if (!strcmp(str, "slot3")) {
    *fru = 3;
  } else if (!strcmp(str, "slot4")) {
    *fru = 4;
  } else {
    return -1;
  }

  return 0;
}

static int
util_read_mca_reg(uint8_t slot_id, char* thread_addr, char* mca_reg_addr[]) {
  char read_mca_reg_command[128];

  snprintf(read_mca_reg_command, sizeof(read_mca_reg_command), READ_MCA_REG_COMMAND, slot_id, thread_addr, mca_reg_addr[0], mca_reg_addr[1], mca_reg_addr[2], mca_reg_addr[3]);
  if (0 != system(read_mca_reg_command))
  {
      return -1;
  }

  return 0;
}

static int
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

int parse_args(int argc, char *argv[]) {
  int ret;
  u_int8_t fru;
  u_int32_t limit_power;
  char fruname[32];
  char* thread_addr;
  char* mca_reg_addr[4];
  static struct option long_opts[] = {
    {"power_capping", required_argument, 0, 'p'},
    {"read_mca_reg", required_argument, 0, 'r'},
    {0,0,0,0},
  };

  if (argc < 2) {
    return -1;
  }

  strcpy(fruname, argv[1]);
  if(get_fru_id(fruname, &fru) != 0 )
  {
    return -1;
  }

  while(1) {
    ret = getopt_long(argc, argv, "p:r:", long_opts, NULL);
    if (ret == -1) {
      break;
    }

    switch (ret) {
      case 'p':
        if(argc == POWER_CAPPING_OPTION_NUM) {
          limit_power = atoi(optarg);
          //Watts converted to milliwatts
          ret = util_power_capping(fru, limit_power*1000);
          return ret;
        } else {
          return - 1;
        }
        break;
      case 'r':
        if (argc == READ_MCA_REG_OPTION_NUM) {
          int i = 0;
          int index = 0;

          thread_addr = optarg;
          for (i = optind; i < argc; i++) {
            mca_reg_addr[index] = argv[i];
            index++;
          }
          ret = util_read_mca_reg(fru, thread_addr, mca_reg_addr);
          return ret;
        } else {
          return -1;
        }
        break;
      default:
        return -1;
        break;
    }
  }

  if (argc >= 4) {
    process_command(fru, (argc - 2), (argv + 2));
  } else {
    return -1;
  }

  return 0;
}

int
main(int argc, char **argv) {
  int ret = 0;
  if (parse_args(argc, argv)) {
    print_usage_help();
    exit(-1);
  }

  return ret;
}
