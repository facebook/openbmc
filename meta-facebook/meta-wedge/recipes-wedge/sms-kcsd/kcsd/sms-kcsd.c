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
#include <facebook/alert_control.h>
#include <facebook/ipmi.h>


#define PATH_SMS_KCS "/sys/bus/i2c/drivers/fb_panther_plus/4-0040/sms_kcs"
#define MAX_ALERT_CONTROL_RETRIES 3

typedef struct {
  unsigned char fbid;
  unsigned char length;
  unsigned char buf[];
} kcs_msg_t;

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
  unsigned char tlen = 0;
  int count = 0;
  int i = 0;

  // Reads incoming request
  fp = fopen(PATH_SMS_KCS, "r");
  if (!fp) {
    syslog(LOG_ALERT, "failed to open file %s\n", PATH_SMS_KCS);
    return -1;
  }

  count = fread(rbuf, sizeof(unsigned char), sizeof(rbuf), fp);
  if (count == 0) {
    syslog(LOG_INFO, "fread returns zero bytes\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  msg = (kcs_msg_t*)rbuf;

  // Invoke IPMI handler
  ipmi_handle(msg->buf, msg->length, &tbuf[1], &tlen);

  // Fill the length as returned by IPMI stack
  tbuf[0] = tlen;

  //Write Reply back to KCS channel
  fp = fopen(PATH_SMS_KCS, "w");
  if (!fp) {
    syslog(LOG_ALERT, "failed to open file %s\n", PATH_SMS_KCS);
    return -1;
  }

  count = fwrite(tbuf, sizeof(unsigned char), tlen+1, fp);
  if (count != tlen+1) {
    syslog(LOG_ALERT, "fwrite returns: %d, expected: %d\n", count, tlen+1);
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

/*
 * Daemon Main loop
 */
int main(int argc, char **argv) {
  int i;
  int ret;
  daemon(1, 0);
  openlog("sms-kcs", LOG_CONS, LOG_DAEMON);

  // Enable alert for SMS KCS Function Block
  for (i = 0; i < MAX_ALERT_CONTROL_RETRIES; i++) {
    ret = alert_control(FBID_SMS_KCS, FLAG_ENABLE);
    if (!ret) {
      break;
    }
    sleep(2);
  }

  // Exit with error in case we can not set the Alert
  if(ret) {
    syslog(LOG_ALERT, "Can not enable SMS KCS Alert\n");
    exit(-1);
  }

  // Forever loop to poll and process KCS messages
  while (1) {
    if (is_new_kcs_msg()) {
      handle_kcs_msg();
    }
    sleep(1);
  }
}
