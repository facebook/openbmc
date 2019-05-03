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
#include <openbmc/libgpio.h>
#include "openbmc/ipmi.h"
#include <time.h>

//#define DEBUG
#define DEFAULT_BMC_READY_GPIO_SHADOW  "BMC_READY_N"
uint8_t req_buf[256];
uint8_t res_buf[300];
gpio_desc_t *bmc_ready_n = NULL;
int kcs_fd;

#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

#define FRU_SERVER 0x1  //payload id for FRU_SERVER

const uint8_t default_add_sel_resp[5]={0x2c, 0x44, 0x00, 0x00, 0x00};
const uint8_t default_add_sel_res_len = sizeof(default_add_sel_resp) * sizeof(uint8_t);

typedef struct
{
  pthread_mutex_t acquire_lock;
  pthread_cond_t done;
}lock;

lock sel_lock = 
{
  .acquire_lock = PTHREAD_MUTEX_INITIALIZER, 
  .done = PTHREAD_COND_INITIALIZER,
};
 
typedef struct sel_node
{
  uint8_t *sel;
  int sel_len;
  struct sel_node *next;
}sel_list;

sel_list *head_sel = NULL;

void handle_sel_list(uint8_t *buff, int buff_len)
{

  static sel_list *curr_sel = NULL;

  sel_list *temp_sel = calloc(1, sizeof(sel_list));

  //the total length. payload_id + request data
  temp_sel->sel_len = buff_len + 1;

  //allocate the space to store the payload id and request data
  temp_sel->sel = calloc(temp_sel->sel_len, sizeof(uint8_t)); 

  //copy the request data
  memcpy(&temp_sel->sel[1], buff, buff_len); 

  //fill the payload id
  temp_sel->sel[0] = FRU_SERVER;
  
#ifdef DEBUG
  char data[200]={0};
  int i;

  for(i=0; i < temp_sel->sel_len; i++)
  {
    sprintf(data, "%s [%d]=%02X", data, i, temp_sel->sel[i]);
  }

  syslog(LOG_WARNING,"[%s] Add SEL Get: %s", __func__, data);
#endif

  if ( NULL == head_sel )
  {
    head_sel = temp_sel;
    curr_sel = head_sel;
  }
  else
  {
    curr_sel->next = temp_sel;
    curr_sel = curr_sel->next;
  }  
}

bool is_add_sel_req(uint8_t *buff)
{
  return (buff[0] == (NETFN_STORAGE_REQ << 2)) && (buff[1] == CMD_STORAGE_ADD_SEL);
}

void set_bmc_ready(bool ready)
{
  /* Active low */
  if (ready)
    gpio_set_value(bmc_ready_n, GPIO_VALUE_LOW);
  else
    gpio_set_value(bmc_ready_n, GPIO_VALUE_HIGH);
}

void *handle_add_sel(void *unused)
{
  uint8_t sel_res[default_add_sel_res_len];
  uint16_t sel_res_len = 0;
  sel_list *send_to_ipmi = NULL;

  while (1)
  {

    //acquire the lock
    pthread_mutex_lock(&sel_lock.acquire_lock);

    while ( NULL == head_sel )
    {
      pthread_cond_wait(&sel_lock.done, &sel_lock.acquire_lock);
    }
 
    //send_to_ipmi pointer the head sel
    send_to_ipmi = head_sel;

    //head_sel pointer to the next
    head_sel = head_sel->next;

    //release the lock         
    pthread_mutex_unlock(&sel_lock.acquire_lock);
    
    memset(sel_res, 0, default_add_sel_res_len);
    
    lib_ipmi_handle(send_to_ipmi->sel, send_to_ipmi->sel_len, sel_res, &sel_res_len);

    if ( CC_SUCCESS != sel_res[2] )
    {
      char data[200] = {0};
      int i;

      for(i=0; i < send_to_ipmi->sel_len; i++)
      {
        sprintf(data, "%s %02X", data, send_to_ipmi->sel[i]);
      }

      syslog(LOG_WARNING,"[Fail] Add SEL Fail. Completion Code = 0x%02X", sel_res[2]);
      syslog(LOG_WARNING,"[Fail] SEL Raw: %s", data);
    }

    //free the sel buffer
    free(send_to_ipmi->sel);

    //free the node
    free(send_to_ipmi);
  } 
}

void *kcs_thread(void *unused) {
  struct timespec req;
  struct timespec rem;
  unsigned char req_len;
  unsigned short res_len;

#ifdef DEBUG
  struct timespec req_tv;
  struct timespec res_tv;
  double temp=0;
  char cmd[200]={0};
  int i = 0;
#endif

  set_bmc_ready(true);

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 10000000;//10mSec

  while(1) {
    req_len = read(kcs_fd, req_buf, sizeof(req_buf));
    if (req_len > 0) {

#ifdef DEBUG
      //dump read data
      memset(cmd, 0, 200);
      clock_gettime(CLOCK_REALTIME, &req_tv);
      for(i=0; i < req_len; i++) {
        sprintf(cmd, "%s %02x", cmd, req_buf[i]);
      }
      syslog(LOG_WARNING, "[ %ld.%ld ] KCS Req: %s, len=%d", req_tv.tv_sec, req_tv.tv_nsec, cmd, req_len);
#endif

      TOUCH("/tmp/kcs_touch");

      if ( true == is_add_sel_req(req_buf) )
      {
        //get the lock
        pthread_mutex_lock(&sel_lock.acquire_lock);

        //add the request data to list
        handle_sel_list(req_buf, req_len);

        pthread_cond_signal(&sel_lock.done);

        //release the lock
        pthread_mutex_unlock(&sel_lock.acquire_lock);

        res_len = default_add_sel_res_len;
        memcpy(res_buf, default_add_sel_resp, default_add_sel_res_len);
      }
      else
      {
        memmove(&req_buf[1], req_buf, req_len);
        req_buf[0] = FRU_SERVER;

        // Send to IPMI stack and get response
        // Additional byte as we are adding and passing payload ID for MN support
        lib_ipmi_handle(req_buf, req_len + 1, res_buf, &res_len);
      }
    } else {
      //sleep(0);
      nanosleep(&req, &rem);
      continue;
    }

    res_len = write(kcs_fd, res_buf, res_len);

#ifdef DEBUG
    memset(cmd, 0, 200);
    clock_gettime(CLOCK_REALTIME, &res_tv);
    for(i=0; i < res_len; i++)
      sprintf(cmd, "%s %02x", cmd, res_buf[i]);
    syslog(LOG_WARNING, "[ %ld.%ld ] KCS Res: %s", res_tv.tv_sec, res_tv.tv_nsec, cmd);

    temp =  (res_tv.tv_sec - req_tv.tv_sec -1) *1000 + (1000 + (float) ((res_tv.tv_nsec - req_tv.tv_nsec)/1000000) );
    syslog(LOG_WARNING, "KCS transaction time: %f ms ", temp);
#endif

  }
}

int
main(int argc, char * const argv[]) {
  pthread_t thread;
  pthread_t add_sel_tid;
  char cmd[256], device[256];
  uint8_t kcs_channel_num = 2;
  const char *bmc_ready_n_shadow;

  daemon(1, 0);
  openlog("kcsd", LOG_CONS, LOG_DAEMON);

  if (argc > 2) {
    kcs_channel_num = (uint8_t)strtoul(argv[1], NULL, 0);
    bmc_ready_n_shadow = argv[2];
  } else {
    bmc_ready_n_shadow = DEFAULT_BMC_READY_GPIO_SHADOW;
  }
  bmc_ready_n = gpio_open_by_shadow(bmc_ready_n_shadow);
  if (!bmc_ready_n) {
    syslog(LOG_WARNING, "BMC Ready PIN not open");
  }

  sprintf(device, "/dev/ipmi-kcs%d", kcs_channel_num+1);  // 1-based channel number
  kcs_fd = open(device, O_RDWR);
  if (kcs_fd < 0) {
    sprintf(cmd, "echo 1 > /sys/devices/platform/ast-kcs.%d/enable", kcs_channel_num);
    system(cmd);

    sprintf(device, "/dev/ast-kcs.%d", kcs_channel_num);
    kcs_fd = open(device, O_RDWR);
    if (kcs_fd < 0) {
      syslog(LOG_WARNING, "kcsd: can not open kcs device");
      exit(-1);
    }
  }
  syslog(LOG_INFO, "opened %s", device);

  sleep(1);

  if (pthread_create(&thread, NULL, kcs_thread, NULL) < 0) {
    syslog(LOG_WARNING, "kcsd: pthread_create failed for kcs_thread\n");
    exit(-1);
  }

  if (pthread_create(&add_sel_tid, NULL, handle_add_sel, NULL) < 0) {
    syslog(LOG_WARNING, "kcsd: pthread_create failed for add_sel_thread\n");
    exit(-1);
  }

  sleep(1);

  pthread_join(thread, NULL);

  pthread_join(add_sel_tid, NULL);

  close(kcs_fd);

  return 0;
}
