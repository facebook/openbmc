/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#include "alert_control.h"

#define PATH_ALERT_STATUS "/sys/bus/i2c/drivers/fb_panther_plus/4-0040/alert_status"
#define PATH_ALERT_CONTROL "/sys/bus/i2c/drivers/fb_panther_plus/4-0040/alert_control"
#define MASK_ALERT_SMS_KCS 0x01

/*
 * Function to enable/disable alert signal generation for a given Function Block
 */
int
alert_control(e_fbid_t id, e_flag_t cflag) {
  FILE *fp;
  unsigned char rbuf[5] = {0};
  unsigned char tbuf[3] = {0};
  int count = 0;

  fp = fopen(PATH_ALERT_CONTROL, "r+");
  if (!fp) {
    return -1;
  }

  count = fread(rbuf, sizeof(unsigned char), sizeof(rbuf), fp);
  if (count == 0x0) {
    fclose(fp);
    return -1;
  }

  // Size of the request
  tbuf[0] = 0x02;

  switch(id) {
  case FBID_SMS_KCS:
    if (cflag == FLAG_ENABLE)
      tbuf[1] = rbuf[2] | (0x01 << FBID_SMS_KCS);
    else
      tbuf[1] = rbuf[2] & (~(0x01 << FBID_SMS_KCS));

    tbuf[2] = rbuf[3];
    break;
    // TODO: Add logic for other Function Blocks here
  default:
    tbuf[0] = rbuf[2];
    tbuf[1] = rbuf[3];
    break;
  }

  count = fwrite(tbuf, sizeof(unsigned char), sizeof(tbuf), fp);
  if (count != sizeof(tbuf)) {
    fclose(fp);
    return (-1);
  }

  fclose(fp);

  return 0;
}

/*
 * Function to check if the alert for a given Function Block is asserted or not
 */
bool
is_alert_present(e_fbid_t id) {
  FILE *fp;
  unsigned char buf[5] = {0};
  int count = 0;

  fp = fopen(PATH_ALERT_STATUS, "r");

  if (!fp) {
    return false;
  }

  count = fread(buf, sizeof(unsigned char), sizeof(buf), fp);
  if (count == 0x0) {
    fclose(fp);
    sleep(2);
    return false;
  }

  fclose(fp);

  switch(id) {
  case FBID_SMS_KCS:
    if (buf[2] & (0x01 << FBID_SMS_KCS))
      return true;
    else
      return false;
    //TODO: Add logic for other Function Blocks here
  default:
    return false;
  }
}
