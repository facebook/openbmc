/*
 * sms-kcsd
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * Daemon to monitor traffic coming from sms-kcs interface
 * and respond to the command using IPMI stack
 *
 * TODO:  Determine if the daemon is already started.
 * TODO:  Cache the file descriptors instead of fopen/fclose everytime
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
#include <string.h>
#include <getopt.h>
#include <linux/limits.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/log.h>

#include "alert_control.h"

#define PATH_SMS_KCS I2C_SYSFS_DEV_ENTRY(4-0040, sms_kcs)
#define MAX_ALERT_CONTROL_RETRIES 3

typedef struct {
  unsigned char fbid;
  unsigned char length;
  unsigned char buf[];
} kcs_msg_t;

static int verbose_logging;

#define KCSD_VERBOSE(fmt, args...)   \
  do {                               \
    if (verbose_logging)             \
      syslog(LOG_INFO, fmt, ##args); \
  } while (0)

/*
 * Function to sanity test if ipmid is ready for accepting messages.
 */
static bool
is_ipmid_ready(void)
{
  char path[PATH_MAX];

  /*
   * NOTE: socket file exists doesn't necessarily mean ipmid is ready for
   * accepting messages, but it's better than not checking anything.
   */
  snprintf(path, sizeof(path), "/tmp/%s", SOCK_PATH_IPMI);
  return path_exists(path);
}

/*
 * Function to check if there is any new KCS message available
 * TODO: Will be replaced by interrupt-driven approach
 */
static bool
is_new_kcs_msg(void) {
  return is_alert_present(FBID_SMS_KCS);
}

/*
 * KCS Message Handler:
 *  - Reads the incoming request on KCS channel
 *  - Invokes IPMI handler to provide response
 *  - Writes reply back to KCS channel
 */
static int
handle_kcs_msg(void) {
  FILE *fp;
  kcs_msg_t *msg;
  unsigned char rbuf[256] = {0};
  unsigned char tbuf[256] = {0};
  unsigned short tlen = 0;
  int count = 0;

  // Reads incoming request
  fp = fopen(PATH_SMS_KCS, "r");
  if (!fp) {
    syslog(LOG_ALERT, "failed to open file %s\n", PATH_SMS_KCS);
    return -1;
  }

  count = fread(rbuf, sizeof(rbuf[0]), sizeof(rbuf), fp);
  if (count == 0) {
    syslog(LOG_INFO, "fread returns zero bytes\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  msg = (kcs_msg_t*)rbuf;

  // Invoke IPMI handler
  lib_ipmi_handle(msg->buf, msg->length, &tbuf[1], &tlen);

  // Fill the length as returned by IPMI stack
  tbuf[0] = (unsigned char)tlen;

  //Write Reply back to KCS channel
  fp = fopen(PATH_SMS_KCS, "w");
  if (!fp) {
    syslog(LOG_ALERT, "failed to open file %s\n", PATH_SMS_KCS);
    return -1;
  }

  count = fwrite(tbuf, sizeof(tbuf[0]), tlen+1, fp);
  if (count != tlen+1) {
    syslog(LOG_ALERT, "fwrite returns: %d, expected: %d\n", count, tlen+1);
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

static void
dump_usage(const char *prog_name)
{
  int i;
  struct {
    const char *opt;
    const char *desc;
  } options[] = {
    {"-h|--help", "print this help message"},
    {"-v|--verbose", "enable verbose logging"},
    {NULL, NULL},
  };

  printf("Usage: %s [options]\n", prog_name);
  for (i = 0; options[i].opt != NULL; i++) {
    printf("    %-18s - %s\n", options[i].opt, options[i].desc);
  }
}

/*
 * Daemon Main loop
 */
int main(int argc, char **argv) {
  int i, ret;
  bool ipmid_ready = false;
  struct option long_opts[] = {
    {"help",       no_argument, NULL, 'h'},
    {"verbose",    no_argument, NULL, 'v'},
    {"foreground", no_argument, NULL, 'f'},
    {NULL,         0,           NULL, 0},
  };

  while (1) {
    int opt_index = 0;

    ret = getopt_long(argc, argv, "hv", long_opts, &opt_index);
    if (ret == -1)
      break; /* end of arguments */

    switch (ret) {
    case 'h':
      dump_usage(argv[0]);
      return 0;

    case 'v':
      verbose_logging = 1;
      break;

    default:
      return -1;
    }
  } /* while */

  obmc_log_init("sms-kcs", LOG_INFO, 0);
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  obmc_log_unset_std_stream();
  if (daemon(1, 0)) {
    OBMC_ERROR(errno, "failed to daemonize");
    exit(-1);
  }

  /*
   * Wait till ipmid is ready.
   */
  KCSD_VERBOSE("checking if ipmid is ready..");
  for (i = 0; i < MAX_ALERT_CONTROL_RETRIES; i++) {
    ipmid_ready = is_ipmid_ready();
    if (ipmid_ready) {
      /*
       * sleep another 50 milliseconds just in case ipmid was de-scheduled
       * right after creating the socket (but before setting it to passive
       * socket).
       */
      usleep(50000);
      break;
    }

    sleep(1);
  }
  if (!ipmid_ready) {
    OBMC_WARN("ipmid is not ready after %d retries. Exiting",
              MAX_ALERT_CONTROL_RETRIES);
    return -1;
  }

  // Enable alert for SMS KCS Function Block
  KCSD_VERBOSE("enabling SMS KCS function..");
  for (i = 0; i < MAX_ALERT_CONTROL_RETRIES; i++) {
    ret = alert_control(FBID_SMS_KCS, FLAG_ENABLE);
    if (!ret) {
      break;
    }
    sleep(2);
  }
  if (ret) {
    OBMC_ERROR(errno, "Can not enable SMS KCS Alert");
    exit(-1);
  }

  // Forever loop to poll and process KCS messages
  KCSD_VERBOSE("entering main loop..");
  while (1) {
    if (is_new_kcs_msg()) {
      KCSD_VERBOSE("handling new kcs messages..");
      handle_kcs_msg();
    }
    sleep(1);
  }

  return 0; /* Never reached at present. */
}
