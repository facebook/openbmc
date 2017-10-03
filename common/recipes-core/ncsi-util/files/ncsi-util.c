/*
 * ncsi-util
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#include <openbmc/pal.h>

#define FTGMAC0_DIR "/sys/devices/platform/ftgmac100.0/net/eth0"
#define MAX_RETRY_CNT 3
#define LARGEST_DEVICE_NAME 128

static int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}


static int
write_ftgmac0_value(const char *device_name, unsigned char count, unsigned char *buf) {
  char full_name[LARGEST_DEVICE_NAME];
  char output_value[LARGEST_DEVICE_NAME];
  int value = 0;
  int err;
  int retry_cnt=0;
  int i=0;
  int index = 0;

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", FTGMAC0_DIR, device_name);

  for (i=0; i<count; i++)
     index += snprintf(&output_value[index], LARGEST_DEVICE_NAME-index, "%d ", buf[i]);

  do {
    err = write_device(full_name, output_value);
    if (err == ENOENT) {
      syslog(LOG_INFO, "Error ENOENT, Retry sending cmd [%d], err=%d", (retry_cnt+1), err);
      break;
    } else if (err != 0) {
      syslog(LOG_INFO, "Error, Retry sending cmd [%d], err=%d", (retry_cnt+1), err);
      sleep(1);
    }
  } while ((err != 0) && (retry_cnt++ < MAX_RETRY_CNT));

  return err;
}

static void
print_usage_help(void) {
  printf("Usage: ncsi-util <cmd> <[0..n] raw_payload_bytes_to_send>\n\n");
  printf("   e.g. \n");
  printf("       ncsi-util 80 0 0 129 25 0 0 27 0\n");
}


int
main(int argc, char **argv) {

  int i;
  unsigned char count;
  unsigned char buf[128];

  if (argc < 2)
    goto err_exit;

  count = argc-1;


  for (i=1; i<argc; ++i) {
      printf("0x%x ", atoi(argv[i]));
      buf[i-1] = atoi(argv[i]);
  }
  printf("\n");

  write_ftgmac0_value("cmd_payload", count, buf);


  goto err_exit;


err_exit:
  print_usage_help();
  return -1;
}
