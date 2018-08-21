#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>

#define MAX_CMD_RETRY 2

static total_retry = 0;

static void
print_usage_help(void) {
  printf("Usage: imc-util <slot1|slot2|slot3|slot4> <[0..n]data_bytes_to_send>\n");
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_imc_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("IMC no response!\n");
    return ret;
  }

  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }

  for (i = 1; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t slot_id;

  if (argc < 2) {
    goto err_exit;
  }

  uint8_t server_type = 0xFF;
  uint8_t status;
  int ret;

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else {
    goto err_exit;
  }

  ret = pal_is_fru_prsnt(slot_id, &status);
  if (ret < 0) {
    printf("Check slot present failed for slot%u\n", slot_id);
    return ret;
  }
  if (status == 0) {
    printf("slot%u is not present!\n\n", slot_id);
    return -1;
  }

  ret = fby2_get_server_type(slot_id, &server_type);
  if (ret) {
    printf("Get server type failed for slot%u\n", slot_id);
    return -1;
  }
  switch (server_type) {
    case SERVER_TYPE_TL:
      printf("Error: imc-util isn't supported on TL platform\n");
      return -1;
    case SERVER_TYPE_EP:
      printf("Error: imc-util isn't supported on EP platform\n");
      return -1;
    case SERVER_TYPE_RC:
      if (argc < 3) {
        goto err_exit;
      }
      return process_command(slot_id, (argc - 2), (argv + 2));
    default:
      printf("Error: Block imc-util due to unknown server type\n");
      return -1;
  }

err_exit:
  print_usage_help();
  return -1;
}
