/*
 * me-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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

#define LOGFILE "/tmp/me-util.log"

#define MAX_ARG_NUM 64
#define MAX_CMD_RETRY 2
#define MAX_TOTAL_RETRY 30

static int total_retry = 0;

static void
print_usage_help(void) {
  printf("\nUsage: me-util <slot1|slot2|slot3|slot4|all> <[0..n]data_bytes_to_send>\n");
  printf("                                             <--file> <path>\n");
  printf("                                             <cmd>\n");
  printf("\ncmd list:\n");
  printf("   --get_dev_id   - get device ID\n");
  printf("   --cold_reset   - performs cold reset\n");
  printf("   --warm_reset   - perofrms warm reset\n");
  printf("   --check_status - reports self-test status\n");
  printf("   --restore      - restore ME factory default\n");
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return ret;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }

  for (i = 1; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
    //sprintf(log, "%s%02X ", log, rbuf[i]);
  }
  printf("\n");

#if 0
  sprintf(log, "%s\n", log);

  logfd = open(LOGFILE, O_CREAT | O_WRONLY);
  if (logfd < 0) {
    syslog(LOG_WARNING, "Opening a tmp file failed. errno: %d", errno);
    return -1;
  }

  len = write(logfd, log, strlen(log));
  if (len != strlen(log)) {
    syslog(LOG_WARNING, "Error writing the log to the file");
    close(logfd);
    return -1;
  }
  close(logfd);
#endif

  return 0;
}

static int
process_file(uint8_t slot_id, char *path) {
  FILE *fp;
  int argc;
  char buf[1024];
  char *str, *next, *del=" \n";
  char *argv[MAX_ARG_NUM];

  if (!(fp = fopen(path, "r"))) {
    syslog(LOG_WARNING, "Failed to open %s", path);
    return -1;
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    str = strtok_r(buf, del, &next);
    for (argc = 0; argc < MAX_ARG_NUM && str; argc++, str = strtok_r(NULL, del, &next)) {
      if (str[0] == '#')
        break;

      if (!argc && !strcmp(str, "echo")) {
        printf("%s", (*next) ? next : "\n");
        break;
      }
      argv[argc] = str;
    }
    if (argc < 1)
      continue;

    process_command(slot_id, argc, argv);
    if (total_retry > MAX_TOTAL_RETRY) {
      printf("Maximum retry count exceeded\n");
      fclose(fp);
      return -1;
    }
  }
  fclose(fp);

  return 0;
}

static int
util_get_dev_id(uint8_t slot_id) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];
  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_DEVICE_ID;
  tlen = 2;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }
  if (rlen < 16) {
    printf("return incomplete len=%d\n", rlen);
  }

  printf("Device ID:                 %02x\n", rbuf[1]);
  printf("Device Revision:           %02x\n", rbuf[2]);
  printf("Firmware Revision:         %02x %02x\n", rbuf[3], rbuf[4]);
  printf("IPMI Version:              %02x\n", rbuf[5]);
  printf("Additional Device Support: %02x\n", rbuf[6]);
  printf("Manufacturer ID:           %02x %02x %02x\n", rbuf[7], rbuf[8], rbuf[9]);
  printf("Product ID:                %02x %02x\n", rbuf[10], rbuf[11]);
  printf("Aux Firmware Revision:     %02x %02x %02x\n", rbuf[12], rbuf[13], rbuf[14]);

  return 0;
}

static int
util_cold_reset(uint8_t slot_id) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];
  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_COLD_RESET;
  tlen = 2;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
    return -1;
  }
  if (rlen < 3) {
    printf("return incomplete len=%d\n", rlen);
  }

  return 0;
}

static int
util_warm_reset(uint8_t slot_id) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];
  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_WARM_RESET;
  tlen = 2;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
    return -1;
  }
  if (rlen < 3) {
    printf("return incomplete len=%d\n", rlen);
  }

  return 0;
}

static int
util_check_status(uint8_t slot_id) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];
  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_SELFTEST_RESULTS;
  tlen = 2;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
    return -1;
  }
  if (rlen < 3) {
    printf("return incomplete len=%d\n", rlen);
    return -1;
  }

  printf("%02x %02x\n", rbuf[1], rbuf[2]);
  switch (rbuf[1]) {
    case 0x55:
      printf("No Error\n");
      ret = 0;
      break;
    case 0x56:
      printf("Slef test not supported\n");
      ret = 0;
      break;
    case 0x57:
      printf("Corrupted or inaccessible deivce\n");
      ret = -1;
      break;
    case 0x58:
      printf("Fatal Hardware Error\n");
      ret = -1;
      break;
    default:
      printf("Unknown Status\n");
      ret = 0;
      break;
  }

  return ret;
}

static int
util_restore_default(uint8_t slot_id) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];
  tbuf[0] = NETFN_NM_REQ << 2;
  tbuf[1] = CMD_NM_FORCE_ME_RECOVERY;
  tbuf[2] = 0x57;
  tbuf[3] = 0x01;
  tbuf[4] = 0x00;
  tbuf[5] = 0x02;
  tlen = 6;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return -1;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
    return -1;
  }
  if (rlen < 3) {
    printf("return incomplete len=%d\n", rlen);
    return -1;
  }
  for (i = 1; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
    //sprintf(log, "%s%02X ", log, rbuf[i]);
  }
  printf("\n");

  return ret;
}

int
main(int argc, char **argv) {
#define SLOT_MIN 1
#define SLOT_MAX 4
#define SLOT_ALL 5

  uint8_t slot_id, i;

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else if (!strcmp(argv[1] , "all")) {
    slot_id = SLOT_ALL;
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--file")) {
    if (argc < 4) {
      goto err_exit;
    }

    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        process_file(i, argv[3]);
      }
      return 0;
    } else {
      return process_file(slot_id, argv[3]);
    }
  } else if (!strcmp(argv[2], "--get_dev_id")) {
    if (argc < 3) {
      goto err_exit;
    }
    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        util_get_dev_id(i);
      }
      return 0;
    } else {
      return util_get_dev_id(slot_id);
    }
  } else if (!strcmp(argv[2], "--cold_reset")) {
    if (argc < 3) {
      goto err_exit;
    }
    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        util_cold_reset(i);
      }
      return 0;
    } else {
      return util_cold_reset(slot_id);
    }
  } else if (!strcmp(argv[2], "--warm_reset")) {
    if (argc < 3) {
      goto err_exit;
    }

    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        util_warm_reset(i);
      }
      return 0;
    } else {
      return util_warm_reset(slot_id);
    }
  } else if (!strcmp(argv[2], "--check_status")) {
    if (argc < 3) {
      goto err_exit;
    }
    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        util_check_status(i);
      }
      return 0;
    } else {
      return util_check_status(slot_id);
    }
  } else if (!strcmp(argv[2], "--restore")) {
    if (argc < 3) {
      goto err_exit;
    }
    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        util_restore_default(i);
      }
      return 0;
    } else {
      return util_restore_default(slot_id);
    }
  } else {
    if (slot_id == SLOT_ALL) {
      for (i = SLOT_MIN; i <= SLOT_MAX; ++i) {
        process_command(i, (argc - 2), (argv + 2));
      }
      return 0;
    } else {
      return process_command(slot_id, (argc - 2), (argv + 2));
    }
  }

err_exit:
  print_usage_help();
  return -1;
}
