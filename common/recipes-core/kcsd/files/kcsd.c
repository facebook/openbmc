/*
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

/*
 * This module handles all the IPMB communication protocol
 * Refer http://www.intel.com/content/www/us/en/servers/ipmi/ipmp-spec-v1-0.html
 * for more information.
 *
 * IPMB packet format is described here for quick reference
 * Request:
 *   <Responder Slave Address(rsSA)>
 *   <NetFn/ResponderLUN(Netfn/rsLUN)>
 *   <Header Checksum(hdrCksum)>
 *   <Requester Slave Address(rqSA)>
 *   <Requester Sequence Number/RequesterLUN(rqSeq/rqLUN>
 *   <Command>
 *   <Data[0..n]>
 *   <Data Checksum(dataCksum)>
 * Response:
 *   <Requester Slave Address(rqSA)>
 *   <NetFn/RequesterLUN(Netfn/rqLUN)>
 *   <Header Checksum(hdrCksum)>
 *   <Responder Slave Address(rsSA)>
 *   <Requester Sequence Number/ResponderLUN(rqSeq/rsLUN>
 *   <Command>
 *   <Completion Code(CC)>
 *   <Data[0..n]>
 *   <Data Checksum(dataCksum)>
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <openbmc/gpio.h>
#include "openbmc/ipmi.h"

unsigned char req_buf[256];
unsigned char res_buf[300];
uint8_t debug = 0;
uint8_t fm_bmc_ready_n = 145;
int kcs_fd;

#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

void set_bmc_ready(bool ready)
{
  gpio_st gpio;
  gpio_open(&gpio, fm_bmc_ready_n);
  /* Active low */
  if (ready)
    gpio_write(&gpio, GPIO_VALUE_LOW);
  else
    gpio_write(&gpio, GPIO_VALUE_HIGH);
  gpio_close(&gpio);
}

void *kcs_thread(void *unused) {
  struct timespec req;
  struct timespec rem;

  unsigned char req_len;
  unsigned short res_len;
  int i = 0;

  set_bmc_ready(true);

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 10000000;//10mSec
  while(1) {
    req_len = read(kcs_fd, req_buf, sizeof(req_buf));
    if (req_len > 0) {
      //dump read data
      if(debug) {
        syslog(LOG_WARNING, "Req [%d] : ", req_len);
        for(i=0;i<req_len;i++) {
          syslog(LOG_WARNING, "%x ", req_buf[i]);
        }
        syslog(LOG_WARNING, "\n");
      }

      // Add payload_id as 1 to  pass to ipmid
      for (i = req_len; i >=0; i--) {
        req_buf[i+1] = req_buf[i];
      }

      req_buf[0] = 0x01;

      TOUCH("/tmp/kcs_touch");

    // Send to IPMI stack and get response
    // Additional byte as we are adding and passing payload ID for MN support
    lib_ipmi_handle(req_buf, req_len + 1, res_buf, &res_len);

    } else {
      //sleep(0);
      nanosleep(&req, &rem);
      continue;
    }

    res_len = write(kcs_fd, res_buf, res_len);
  }

}

int
main(int argc, char * const argv[]) {
  pthread_t thread;
  char cmd[256], device[256];
  uint8_t kcs_channel_num = 2;

  daemon(1, 0);
  openlog("kcsd", LOG_CONS, LOG_DAEMON);

  if (argc > 2) {
    kcs_channel_num = (uint8_t)strtoul(argv[1], NULL, 0);
    fm_bmc_ready_n = (uint8_t)strtoul(argv[2], NULL, 0);
  }
  sprintf(cmd, "echo 1 > /sys/devices/platform/ast-kcs.%d/enable", kcs_channel_num);
  system(cmd);

  sprintf(device, "/dev/ast-kcs.%d", kcs_channel_num);
  kcs_fd = open(device, O_RDWR);
  if (!kcs_fd) {
    syslog(LOG_WARNING, "kcsd: can not open kcs device\n");
    exit(-1);
  }

  sleep(1);

  if (pthread_create(&thread, NULL, kcs_thread, NULL) < 0) {
    syslog(LOG_WARNING, "kcsd: pthread_create failed for kcs_thread\n");
    exit(-1);
  }

  sleep(1);

  pthread_join(thread, NULL);

  close(kcs_fd);

  return 0;
}
