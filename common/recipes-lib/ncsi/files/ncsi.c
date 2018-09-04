/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "ncsi.h"



// reload kernel NC-SI driver and trigger NC-SI interface initialization
int
ncsi_init_if(int inv_addr)
{
  char cmd[80] = {0};
  int ret = 0;
  syslog(LOG_CRIT, "ncsid: re-configure NC-SI and restart eth0 interface");

  if (inv_addr) {
    sprintf(cmd, "ifdown eth0; ifconfig eth0 hw ether 00:00:00:00:00:e0; ifup eth0");
  } else {
    sprintf(cmd, "ifdown eth0; ifup eth0");
  }
  ret = system(cmd);

  syslog(LOG_CRIT, "ncsid: re-start eth0 interface done! ret=%d", ret);

  return ret;
}


// Handles AEN type 0x01 - Cnfiguration Required
// Steps
//    1. ifdown eth0;ifup eth0    # re-init NIC NC-SI interface
//    2. restart ncsid
void
handle_ncsi_config(void)
{
// NCSI Reset time. NCSI Spec specfies NIC to finish reset within 2second max,
// here we add an extra 1 sec to provide extra buffer
#define NCSI_RESET_TIMEOUT  3

  // Give NIC some time to finish its reset opeartion before  BMC sends
  // NCSI commands to re-initialize the interface
  sleep(NCSI_RESET_TIMEOUT);

  ncsi_init_if(0);
}


int
getMacAddr(int *values) {
  FILE *fp;

  fp = fopen("/sys/class/net/eth0/address", "rt");
  if (!fp) {
    int err = errno;
    return err;
  }
  if( 6 == fscanf(fp, "%x:%x:%x:%x:%x:%x%*c",
                  &values[0], &values[1], &values[2],
                  &values[3], &values[4], &values[5] ) )
  {
#ifdef DEBUG
    syslog(LOG_CRIT, "mac check %x:%x:%x:%x:%x:%x",
               values[0], values[1], values[2], values[3],
               values[4], values[5]);
#endif
  }

  fclose(fp);
  return 0;
}



#define ZERO_OCTET_THRESHOLD 5

// rudimentary check for valid MAC:
//    invalid MAC == (number of 0 octet >= 5)
//    valid MAC   == everything else
int
checkValidMacAddr(int *value) {
  int numZeroOctet = 0;
  int valid = 1;

  if ((value[0] & 0x01))  // broadcast or multicast address
    return 0;

  for (int i=0; i<6; ++i) {
    if (value[i] == 0)
      numZeroOctet++;
  }

  if (numZeroOctet >= ZERO_OCTET_THRESHOLD) {
    valid = 0;
  }

  return valid;
}

// check if MAC address was successfully obtained from NIC via NC-SI
// will re-init/retry if needed
// return values
//     0 - success
//     1 - generic Error
//     NCSI_IF_REINIT - NCSI reinited, restart required
int
check_ncsi_status(void)
{
  int values[6] = {0};
  int ret = 0;

  ret = getMacAddr(values);

  // if fail to obtain MAC addr, do not proceed further
  if (ret)
    return 1;

  // if invalid MAC was obtained, re-init NCSI interface and
  //     check the MAC addr again
  if (!checkValidMacAddr(values)) {
    /* invalid mac */
    syslog(LOG_CRIT, "invalid MAC(%x:%x:%x:%x:%x:%x) detected, reinit NCSI",
             values[0], values[1], values[2], values[3],
             values[4], values[5]);

    // re-init NCSI interface and get MAC address again
    ncsi_init_if(1);
    ret = getMacAddr(values);
    if (!ret && checkValidMacAddr(values)) {
       syslog(LOG_CRIT, "Valid MAC(%x:%x:%x:%x:%x:%x) obtained after NCSI re-init, restarting ncsid",
       values[0], values[1], values[2], values[3],
             values[4], values[5]);
      return NCSI_IF_REINIT;
    } else {
      syslog(LOG_CRIT, "Invalid MAC(%x:%x:%x:%x:%x:%x), after NCSI re-init",
            values[0], values[1], values[2], values[3],
            values[4], values[5]);
    }
  }

  return 0;
}

void
print_ncsi_resp(NCSI_NL_RSP_T *rcv_buf)
{
  uint8_t *pbuf = rcv_buf->msg_payload;
  int i = 0;

  printf("NC-SI Command Response:\n");
  printf("Response Code: 0x%04x  Reason Code: 0x%04x\n", (pbuf[0]<<8)+pbuf[1], (pbuf[2]<<8)+pbuf[3]);
  for (i = 4; i < rcv_buf->payload_length; ++i) {
		if (i && !(i%4))
			printf("\n%d: ", 16+i);
    printf("0x%02x ", rcv_buf->msg_payload[i]);
  }
  printf("\n");
}
