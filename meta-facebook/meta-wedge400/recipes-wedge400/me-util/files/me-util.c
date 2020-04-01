/*
 * me-util
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <facebook/bic.h>
#include <openbmc/ipmi.h>

#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOGFILE "/tmp/me-util.log"
#endif

#define MAX_ARG_NUM 64
#define MAX_CMD_RETRY 2
#define MAX_TOTAL_RETRY 30

static int total_retry = 0;

static void
print_usage_help(void) {
  printf("Usage: me-util <--slot #id> <[0..n]data_bytes_to_send>\n");
  printf("Usage: me-util <--slot #id> <--file> <path>\n");
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
#ifdef ENABLE_LOG
  int logfd, len;
  char log[512] = {0};
#endif

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

#ifdef ENABLE_LOG
  log[0] = 0;
#endif
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }

  for (i = 1; i < rlen; i++) {
#ifdef ENABLE_LOG
    char tmp[8];
    snprintf(tmp, sizeof(tmp), "%02X ", rbuf[i]);
    strncat(log, tmp, sizeof(tmp));
#endif
    printf("%02X ", rbuf[i]);
  }
  printf("\n");

#ifdef ENABLE_LOG
  logfd = open(LOGFILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
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

int
main(int argc, char **argv) {
  unsigned int slot_id = 0xff;

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "--slot")) {
    errno = 0;
    int ret = sscanf(argv[2], "%u", &slot_id);
    if (slot_id == 0xff || errno != 0 || ret != 1) {
      goto err_exit;
    }
    printf("slot id: %d\n", slot_id);
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[3], "--file")) {
    if (argc < 4) {
      goto err_exit;
    }

    process_file((uint8_t)slot_id, argv[3]);
    return 0;
  }

  return process_command((uint8_t)slot_id, (argc - 3), (argv + 3));

err_exit:
  print_usage_help();
  return -1;
}
