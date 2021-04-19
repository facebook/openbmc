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
#include <time.h>
#include <getopt.h>

#include <openbmc/ipmi.h>
#include <openbmc/libgpio.h>
#include <openbmc/log.h>

//#define DEBUG
#define DEFAULT_BMC_READY_GPIO_SHADOW  "BMC_READY_N"
static gpio_desc_t *bmc_ready_n = NULL;
static int kcs_fd = -1;

#define TOUCH(path)            \
do {                           \
  int _fd = creat(path, 0644); \
  if (_fd >= 0)                \
    close(_fd);                \
} while (0)

#define FRU_SERVER 0x1  //payload id for FRU_SERVER

static const uint8_t default_add_sel_resp[5] = {0x2c, 0x44, 0x00, 0x00, 0x00};
static const uint8_t default_add_sel_res_len = (sizeof(default_add_sel_resp) *
                                                sizeof(uint8_t));

static struct {
  pthread_mutex_t acquire_lock;
  pthread_cond_t done;
} sel_lock =
{
  .acquire_lock = PTHREAD_MUTEX_INITIALIZER, 
  .done = PTHREAD_COND_INITIALIZER,
};
 
typedef struct sel_node
{
  uint8_t *sel;
  int sel_len;
  struct sel_node *next;
} sel_list;

static sel_list *head_sel = NULL;

static int verbose_logging = 0;
#define KCSD_VERBOSE(fmt, args...) \
  do {                             \
    if (verbose_logging)           \
      OBMC_INFO(fmt, ##args);      \
  } while (0)

static void handle_sel_list(uint8_t *buff, int buff_len)
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
  char data[200] = {0};
  int i;

  for(i=0; i < temp_sel->sel_len; i++)
  {
    snprintf(data, sizeof(data), "%s [%d]=%02X", data, i, temp_sel->sel[i]);
  }

  OBMC_WARN("[%s] Add SEL Get: %s", __func__, data);
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

static bool is_add_sel_req(uint8_t *buff)
{
  return (buff[0] == (NETFN_STORAGE_REQ << 2)) &&
         (buff[1] == CMD_STORAGE_ADD_SEL);
}

static void set_bmc_ready(bool ready)
{
  /* Active low */
  if (ready)
    gpio_set_init_value(bmc_ready_n, GPIO_VALUE_LOW);
  else
    gpio_set_init_value(bmc_ready_n, GPIO_VALUE_HIGH);
}

static void *handle_add_sel(void *unused)
{
  uint8_t sel_res[default_add_sel_res_len];
  uint16_t sel_res_len = 0;
  sel_list *send_to_ipmi = NULL;

  KCSD_VERBOSE("sel_handler thread started");

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

    KCSD_VERBOSE("sel_handler: processing new SEL entry");
    
    memset(sel_res, 0, default_add_sel_res_len);
    
    lib_ipmi_handle(send_to_ipmi->sel, send_to_ipmi->sel_len, sel_res,
                    &sel_res_len);

    if ( CC_SUCCESS != sel_res[2] )
    {
      char data[200] = {0};
      int i;

      for(i=0; i < send_to_ipmi->sel_len; i++)
      {
        snprintf(data, sizeof(data), "%02X ", send_to_ipmi->sel[i]);
      }

      OBMC_WARN("[Fail] Add SEL Fail. Completion Code = 0x%02X", sel_res[2]);
      OBMC_WARN("[Fail] SEL Raw: %s", data);
    }

    //free the sel buffer
    free(send_to_ipmi->sel);

    //free the node
    free(send_to_ipmi);
  } 

  return NULL;
}

static void *kcs_thread(void *unused) {
  struct timespec req;
  struct timespec rem;
  unsigned char req_len;
  unsigned short res_len;
  uint8_t req_buf[256];
  uint8_t res_buf[300];

#ifdef DEBUG
  struct timespec req_tv;
  struct timespec res_tv;
  double temp=0;
  char cmd[200]={0};
  int i = 0;
#endif

  KCSD_VERBOSE("kcs_dev thread started");

  set_bmc_ready(true);

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 10000000;//10mSec

  while (1) {
    req_len = read(kcs_fd, req_buf, sizeof(req_buf));
    if (req_len <= 0) {
      nanosleep(&req, &rem);
      continue;
    }

#ifdef DEBUG
    //dump read data
    memset(cmd, 0, 200);
    clock_gettime(CLOCK_REALTIME, &req_tv);
    for(i=0; i < req_len; i++) {
      snprintf(cmd, sizeof(cmd), "%s %02x", cmd, req_buf[i]);
    }
    OBMC_WARN("[ %ld.%ld ] KCS Req: %s, len=%d",
              req_tv.tv_sec, req_tv.tv_nsec, cmd, req_len);
#endif

    TOUCH("/tmp/kcs_touch");

    if ( true == is_add_sel_req(req_buf)) {
      KCSD_VERBOSE("kcs_dev: new SEL entry received");

      pthread_mutex_lock(&sel_lock.acquire_lock);

      //add the request data to list
      handle_sel_list(req_buf, req_len);

      pthread_cond_signal(&sel_lock.done);

      pthread_mutex_unlock(&sel_lock.acquire_lock);

      res_len = default_add_sel_res_len;
      memcpy(res_buf, default_add_sel_resp, default_add_sel_res_len);
    } else {
      KCSD_VERBOSE("kcs_dev: send message (netfn=0x%02x, cmd=0x%02x) to ipmid",
                   req_buf[0], req_buf[1]);

      memmove(&req_buf[1], req_buf, req_len);
      req_buf[0] = FRU_SERVER;

      // Send to IPMI stack and get response
      // Additional byte as we are adding and passing payload ID for MN support
      lib_ipmi_handle(req_buf, req_len + 1, res_buf, &res_len);
    }

    res_len = write(kcs_fd, res_buf, res_len);

#ifdef DEBUG
    memset(cmd, 0, 200);
    clock_gettime(CLOCK_REALTIME, &res_tv);
    for(i=0; i < res_len; i++)
      snprintf(cmd, sizeof(cmd), "%s %02x", cmd, res_buf[i]);
    OBMC_WARN("[ %ld.%ld ] KCS Res: %s", res_tv.tv_sec, res_tv.tv_nsec, cmd);

    temp = (res_tv.tv_sec - req_tv.tv_sec -1) * 1000 +
           (1000 + (float) ((res_tv.tv_nsec - req_tv.tv_nsec)/1000000) );
    OBMC_WARN("KCS transaction time: %f ms ", temp);
#endif
  } /* while (1) */

  return NULL;
}

/*
 * "kcs_open_new" opens kcs device in upstream kernel.
 */
static int kcs_open_new(uint8_t channel)
{
  int fd;
  char path[PATH_MAX];

  channel++; /* convert to 1-based channel number */
  snprintf(path, sizeof(path), "/dev/ipmi-kcs%d", channel);
  fd = open(path, O_RDWR);
  if (fd < 0) {
    OBMC_ERROR(errno, "failed to open kcs device %s", path);
  } else {
    OBMC_INFO("opened kcs device %s, fd=%d", path, fd);
  }

  return fd;
}

/*
 * "kcs_open_legacy" enables and opens kcs device in kernel 4.1.
 */
static int kcs_open_legacy(uint8_t channel)
{
  int fd;
  char cmd[256], path[PATH_MAX];

  snprintf(cmd, sizeof(cmd),
           "echo 1 > /sys/devices/platform/ast-kcs.%d/enable", channel);
  if (system(cmd) != 0) {
    OBMC_WARN("KCSD Enabling channel %d failed\n", channel);
  }

  snprintf(path, sizeof(path), "/dev/ast-kcs.%d", channel);
  fd = open(path, O_RDWR);
  if (fd < 0) {
    OBMC_ERROR(errno, "failed to open kcs device %s", path);
  } else {
    OBMC_INFO("opened kcs device %s, fd=%d", path, fd);
  }

  return fd;
}

static void dump_usage(const char *prog_name)
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

int
main(int argc, char * const argv[]) {
  int ret;
  pthread_t kcs_tid;
  pthread_t add_sel_tid;
  uint8_t kcs_channel_num = 2;
  const char *bmc_ready_n_shadow = DEFAULT_BMC_READY_GPIO_SHADOW;
  struct option long_opts[] = {
    {"help",       no_argument, NULL, 'h'},
    {"verbose",    no_argument, NULL, 'v'},
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

  /*
   * Parse remaining command line arguments.
   */
  if (optind < argc) {
    kcs_channel_num = (uint8_t)strtoul(argv[optind++], NULL, 0);
  }
  if (optind < argc) {
    bmc_ready_n_shadow = argv[optind];
  }

  obmc_log_init("kcsd", LOG_INFO, 0);
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  obmc_log_unset_std_stream();
  if (daemon(1, 0) != 0) {
    OBMC_ERROR(errno, "failed to enter daemon mode");
    return -1;
  }

  OBMC_INFO("kcsd started: kcs channel %d, bmc_ready_gpio %s",
            kcs_channel_num, bmc_ready_n_shadow);

  bmc_ready_n = gpio_open_by_shadow(bmc_ready_n_shadow);
  if (!bmc_ready_n) {
    OBMC_ERROR(errno, "failed to open BMC Ready PIN %s", bmc_ready_n_shadow);
  }

  kcs_fd = kcs_open_new(kcs_channel_num);
  if (kcs_fd < 0) {
    kcs_fd = kcs_open_legacy(kcs_channel_num);
    if (kcs_fd < 0) {
      return -1;
    }
  }

  sleep(1);

  KCSD_VERBOSE("creating kcs_dev thread");
  ret = pthread_create(&kcs_tid, NULL, kcs_thread, NULL);
  if (ret != 0) {
    OBMC_ERROR(ret, "failed to create kcs_dev_thread");
    return -1;
  }

  KCSD_VERBOSE("creating sel_handler thread");
  ret = pthread_create(&add_sel_tid, NULL, handle_add_sel, NULL);
  if (ret != 0) {
    OBMC_ERROR(ret, "failed to create add_sel_thread");
    return -1;
  }

  sleep(1);

  pthread_join(kcs_tid, NULL);

  pthread_join(add_sel_tid, NULL);

  close(kcs_fd);

  OBMC_INFO("kcsd terminated!");
  return 0;
}
