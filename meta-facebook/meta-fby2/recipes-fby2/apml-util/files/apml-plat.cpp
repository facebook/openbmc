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
#define POWER_CAPPING_COMMAND "apml-util slot%d 0xE0 0x2E 0x15 0xa0 0x00 0x02 0x%02x 0x%02x 0x%02x 0x%02x"
#define READ_MCA_REG_COMMAND  "apml-util slot%d 0xE0 0x2C 0x15 0xa0 0x00 0x02 %s %s %s %s %s"
int
util_read_mca_reg(uint8_t slot_id, char* thread_addr, char* mca_reg_addr[]) {
  char read_mca_reg_command[128];

  snprintf(read_mca_reg_command, sizeof(read_mca_reg_command), READ_MCA_REG_COMMAND, slot_id, thread_addr, mca_reg_addr[0], mca_reg_addr[1], mca_reg_addr[2], mca_reg_addr[3]);
  if (0 != system(read_mca_reg_command))
  {
      return -1;
  }

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
