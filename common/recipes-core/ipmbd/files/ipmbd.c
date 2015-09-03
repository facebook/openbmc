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
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <mqueue.h>
#include <semaphore.h>

#include "facebook/i2c-dev.h"
#include "openbmc/ipmi.h"
#include "openbmc/ipmb.h"

//#define DEBUG 0

#define MAX_BYTES 255

#define MQ_IPMB_REQ "/mq_ipmb_req"
#define MQ_IPMB_RES "/mq_ipmb_res"
#define MQ_MAX_MSG_SIZE MAX_BYTES
#define MQ_MAX_NUM_MSGS 10

#define SEQ_NUM_MAX 64

#define I2C_RETRIES_MAX 3

// Structure for i2c file descriptor and socket
typedef struct _ipmb_sfd_t {
  int fd;
  int sock;
} ipmb_sfd_t;

// Structure for sequence number and buffer
typedef struct _seq_buf_t {
  bool in_use; // seq# is being used
  uint8_t len; // buffer size
  uint8_t *p_buf; // pointer to buffer
  sem_t s_seq; // semaphore for thread sync.
} seq_buf_t;

// Structure for holding currently used sequence number and
// array of all possible sequence number
typedef struct _ipmb_sbuf_t {
  uint8_t curr_seq; // currently used seq#
  seq_buf_t seq[SEQ_NUM_MAX]; //array of all possible seq# struct.
} ipmb_sbuf_t;

// Global storage for holding IPMB sequence number and buffer
ipmb_sbuf_t g_seq;

// mutex to protect global data access
pthread_mutex_t m_seq;

pthread_mutex_t m_i2c;

#ifdef CONFIG_YOSEMITE
// Returns the payload ID from IPMB bus routing
// Slot#1: bus#3, Slot#2: bus#1, Slot#3: bus#7, Slot#4: bus#5
static uint8_t
get_payload_id(uint8_t bus_id) {
  uint8_t payload_id = 0xFF; // Invalid payload ID

  switch(bus_id) {
  case 1:
    payload_id = 2;
    break;
  case 3:
    payload_id = 1;
    break;
  case 5:
    payload_id = 4;
    break;
  case 7:
    payload_id = 3;
    break;
  default:
    syslog(LOG_ALERT, "get_payload_id: Wrong bus ID\n");
    break;
  }

  return payload_id;
}
#endif

// Returns an unused seq# from all possible seq#
static uint8_t
seq_get_new(void) {
  uint8_t ret = -1;
  uint8_t index;

  pthread_mutex_lock(&m_seq);

  // Search for unused sequence number
  index = g_seq.curr_seq;
  do {
    if (g_seq.seq[index].in_use == false) {
      // Found it!
      ret = index;
      g_seq.seq[index].in_use = true;
      g_seq.seq[index].len = 0;
      break;
    }

    if (++index == SEQ_NUM_MAX) {
      index = 0;
    }
  } while (index != g_seq.curr_seq);

  // Update the current seq num
  if (ret >= 0) {
    if (++index == SEQ_NUM_MAX) {
      index = 0;
    }
    g_seq.curr_seq = index;
  }

  pthread_mutex_unlock(&m_seq);

  return ret;
}

static int
i2c_open(uint8_t bus_num) {
  int fd;
  char fn[32];
  int rc;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_ALERT, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, BRIDGE_SLAVE_ADDR);
  if (rc < 0) {
    syslog(LOG_ALERT, "Failed to open slave @ address 0x%x", BRIDGE_SLAVE_ADDR);
    close(fd);
    return -1;
  }

  return fd;
}

static int
i2c_write(int fd, uint8_t *buf, uint8_t len) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  int rc;
  int i;

  memset(&msg, 0, sizeof(msg));

  msg.addr = BRIDGE_SLAVE_ADDR;
  msg.flags = 0;
  msg.len = len;
  msg.buf = buf;

  data.msgs = &msg;
  data.nmsgs = 1;

  pthread_mutex_lock(&m_i2c);

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    rc = ioctl(fd, I2C_RDWR, &data);
    if (rc < 0) {
      sleep(1);
      continue;
    } else {
      break;
    }
  }

  if (rc < 0) {
    syslog(LOG_ALERT, "Failed to do raw io");
    pthread_mutex_unlock(&m_i2c);
    return -1;
  }

  pthread_mutex_unlock(&m_i2c);

  return 0;
}

static int
i2c_slave_open(uint8_t bus_num) {
  int fd;
  char fn[32];
  int rc;
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  uint8_t read_bytes[MAX_BYTES] = { 0 };

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_ALERT, "Failed to open i2c device %s", fn);
    return -1;
  }

  memset(&msg, 0, sizeof(msg));

  msg.addr = BMC_SLAVE_ADDR;
  msg.flags = I2C_S_EN;
  msg.len = 1;
  msg.buf = read_bytes;
  msg.buf[0] = 1;

  data.msgs = &msg;
  data.nmsgs = 1;

  rc = ioctl(fd, I2C_SLAVE_RDWR, &data);
  if (rc < 0) {
    syslog(LOG_ALERT, "Failed to open slave @ address 0x%x", BMC_SLAVE_ADDR);
    close(fd);
  }

  return fd;
}

static int
i2c_slave_read(int fd, uint8_t *buf, uint8_t *len) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  int rc;

  memset(&msg, 0, sizeof(msg));

  msg.addr = BMC_SLAVE_ADDR;
  msg.flags = 0;
  msg.len = MAX_BYTES;
  msg.buf = buf;

  data.msgs = &msg;
  data.nmsgs = 1;

  rc = ioctl(fd, I2C_SLAVE_RDWR, &data);
  if (rc < 0) {
    return -1;
  }

  *len = msg.len;

  return 0;
}

// Thread to handle new requests
static void*
ipmb_req_handler(void *bus_num) {
  uint8_t *bnum = (uint8_t*) bus_num;
  mqd_t mq;
  int fd;
  int i;

  //Buffers for IPMB transport
  uint8_t rxbuf[MQ_MAX_MSG_SIZE] = {0};
  uint8_t txbuf[MQ_MAX_MSG_SIZE] = {0};
  ipmb_req_t *p_ipmb_req;
  ipmb_res_t *p_ipmb_res;

  p_ipmb_req = (ipmb_req_t*) rxbuf;
  p_ipmb_res = (ipmb_res_t*) txbuf;

  //Buffers for IPMI Stack
  uint8_t rbuf[MQ_MAX_MSG_SIZE] = {0};
  uint8_t tbuf[MQ_MAX_MSG_SIZE] = {0};
  ipmi_mn_req_t *p_ipmi_mn_req;
  ipmi_res_t *p_ipmi_res;

  p_ipmi_mn_req = (ipmi_mn_req_t*) rbuf;
  p_ipmi_res = (ipmi_res_t*) tbuf;

  uint8_t rlen = 0;
  uint8_t tlen = 0;

  char mq_ipmb_req[64] = {0};

  sprintf(mq_ipmb_req, "%s_%d", MQ_IPMB_REQ, *bnum);

  // Open Queue to receive requests
  mq = mq_open(mq_ipmb_req, O_RDONLY);
  if (mq == (mqd_t) -1) {
    return NULL;
  }

  // Open the i2c bus for sending response
  fd = i2c_open(*bnum);
  if (fd < 0) {
    syslog(LOG_ALERT, "i2c_open failure\n");
    close(mq);
    return NULL;
  }

  // Loop to process incoming requests
  while (1) {
    if ((rlen = mq_receive(mq, rxbuf, MQ_MAX_MSG_SIZE, NULL)) < 0) {
      sleep(1);
      continue;
    }

#ifdef DEBUG
    syslog(LOG_ALERT, "Received Request of %d bytes\n", rlen);
    for (i = 0; i < rlen; i++) {
      syslog(LOG_ALERT, "0x%X", rxbuf[i]);
    }
#endif

    // Create IPMI request from IPMB data
#ifdef CONFIG_YOSEMITE
    p_ipmi_mn_req->payload_id = get_payload_id(*bnum);
#else
    // For single node systems use payload ID as 1
    p_ipmi_mn_req->payload_id = 0x1;
#endif
    p_ipmi_mn_req->netfn_lun = p_ipmb_req->netfn_lun;
    p_ipmi_mn_req->cmd = p_ipmb_req->cmd;

    memcpy(p_ipmi_mn_req->data, p_ipmb_req->data, rlen - IPMB_HDR_SIZE - IPMI_REQ_HDR_SIZE);

    // Send to IPMI stack and get response
    // Additional byte as we are adding and passing payload ID for MN support
    lib_ipmi_handle(rbuf, rlen - IPMB_HDR_SIZE + 1, tbuf, &tlen);

    // Populate IPMB response data from IPMB request
    p_ipmb_res->req_slave_addr = p_ipmb_req->req_slave_addr;
    p_ipmb_res->res_slave_addr = p_ipmb_req->res_slave_addr;
    p_ipmb_res->cmd = p_ipmb_req->cmd;
    p_ipmb_res->seq_lun = p_ipmb_req->seq_lun;

    // Add IPMI response data
    p_ipmb_res->netfn_lun = p_ipmi_res->netfn_lun;
    p_ipmb_res->cc = p_ipmi_res->cc;

    memcpy(p_ipmb_res->data, p_ipmi_res->data, tlen - IPMI_RESP_HDR_SIZE);

    // Calculate Header Checksum
    p_ipmb_res->hdr_cksum = p_ipmb_res->req_slave_addr +
                           p_ipmb_res->netfn_lun;
    p_ipmb_res->hdr_cksum = ZERO_CKSUM_CONST - p_ipmb_res->hdr_cksum;

    // Calculate Data Checksum
    p_ipmb_res->data[tlen-IPMI_RESP_HDR_SIZE] = p_ipmb_res->res_slave_addr +
                            p_ipmb_res->seq_lun +
                            p_ipmb_res->cmd +
                            p_ipmb_res->cc;

    for (i = 0; i < tlen-IPMI_RESP_HDR_SIZE; i++) {
      p_ipmb_res->data[tlen-IPMI_RESP_HDR_SIZE] += p_ipmb_res->data[i];
    }

    p_ipmb_res->data[tlen-IPMI_RESP_HDR_SIZE] = ZERO_CKSUM_CONST -
                                      p_ipmb_res->data[tlen-IPMI_RESP_HDR_SIZE];

#ifdef DEBUG
    syslog(LOG_ALERT, "Sending Response of %d bytes\n", tlen+IPMB_HDR_SIZE-1);
    for (i = 1; i < tlen+IPMB_HDR_SIZE; i++) {
      syslog(LOG_ALERT, "0x%X:", txbuf[i]);
    }
#endif

     // Send response back
     i2c_write(fd, &txbuf[1], tlen+IPMB_HDR_SIZE-1);
  }
}

// Thread to handle the incoming responses
static void*
ipmb_res_handler(void *bus_num) {
  uint8_t *bnum = (uint8_t*) bus_num;
  uint8_t buf[MQ_MAX_MSG_SIZE] = { 0 };
  uint8_t len = 0;
  mqd_t mq;
  ipmb_res_t *p_res;
  uint8_t index;
  char mq_ipmb_res[64] = {0};

  sprintf(mq_ipmb_res, "%s_%d", MQ_IPMB_RES, *bnum);

  // Open the message queue
  mq = mq_open(mq_ipmb_res, O_RDONLY);
  if (mq == (mqd_t) -1) {
    syslog(LOG_ALERT, "mq_open fails\n");
    return NULL;
  }

  // Loop to wait for incomng response messages
  while (1) {
    if ((len = mq_receive(mq, buf, MQ_MAX_MSG_SIZE, NULL)) < 0) {
      sleep(1);
      continue;
    }

    p_res = (ipmb_res_t *) buf;

    // Check the seq# of response
    index = p_res->seq_lun >> LUN_OFFSET;

    // Check if the response is being waited for
    pthread_mutex_lock(&m_seq);
    if (g_seq.seq[index].in_use) {
      // Copy the response to the requester's buffer
      memcpy(g_seq.seq[index].p_buf, buf, len);
      g_seq.seq[index].len = len;

      // Wake up the worker thread to receive the response
      sem_post(&g_seq.seq[index].s_seq);
    }
    pthread_mutex_unlock(&m_seq);

#ifdef DEBUG
    syslog(LOG_ALERT, "Received Response of %d bytes\n", len);
    int i;
    for (i = 0; i < len; i++) {
      syslog(LOG_ALERT, "0x%X:", buf[i]);
    }
#endif
  }
}

// Thread to receive the IPMB messages over i2c bus as a slave
static void*
ipmb_rx_handler(void *bus_num) {
  uint8_t *bnum = (uint8_t*) bus_num;
  int fd;
  uint8_t len;
  uint8_t tlun;
  uint8_t buf[MAX_BYTES] = { 0 };
  mqd_t mq_req, mq_res, tmq;
  ipmb_req_t *p_req;
  struct timespec req;
  struct timespec rem;
  char mq_ipmb_req[64] = {0};
  char mq_ipmb_res[64] = {0};

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 10000000;//10mSec

  // Open the i2c bus as a slave
  fd = i2c_slave_open(*bnum);
  if (fd < 0) {
    syslog(LOG_ALERT, "i2c_slave_open fails\n");
    goto cleanup;
  }

  sprintf(mq_ipmb_req, "%s_%d", MQ_IPMB_REQ, *bnum);
  sprintf(mq_ipmb_res, "%s_%d", MQ_IPMB_RES, *bnum);

  // Open the message queues for post processing
  mq_req = mq_open(mq_ipmb_req, O_WRONLY);
  if (mq_req == (mqd_t) -1) {
    syslog(LOG_ALERT, "mq_open req fails\n");
    goto cleanup;
  }

  mq_res = mq_open(mq_ipmb_res, O_WRONLY);
  if (mq_res == (mqd_t) -1) {
    syslog(LOG_ALERT, "mq_open res fails\n");
    goto cleanup;
  }

  // Loop that retrieves messages
  while (1) {
    // Read messages from i2c driver
    if (i2c_slave_read(fd, buf, &len) < 0) {
      nanosleep(&req, &rem);
      continue;
    }

    // Check if the messages is request or response
    // Even NetFn: Request, Odd NetFn: Response
    p_req = (ipmb_req_t*) buf;
    tlun = p_req->netfn_lun >> LUN_OFFSET;
    if (tlun%2) {
      tmq = mq_res;
    } else {
      tmq = mq_req;
    }
    // Post message to approriate Queue for further processing
    if (mq_send(tmq, buf, len, 0)) {
      syslog(LOG_ALERT, "mq_send failed\n");
      sleep(1);
      continue;
    }
  }

cleanup:
  if (fd > 0) {
    close (fd);
  }

  if (mq_req > 0) {
    close(mq_req);
  }

  if (mq_res > 0) {
    close(mq_req);
  }
}

/*
 * Function to handle all IPMB requests
 */
static void
ipmb_handle (int fd, unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmb_req_t *req = (ipmb_req_t *) request;
  ipmb_res_t *res = (ipmb_res_t *) response;

  uint8_t index;
  struct timespec ts;

  // Allocate right sequence Number
  index = seq_get_new();
  if (index < 0) {
    *res_len = 0;
    return ;
  }

  req->seq_lun = index << LUN_OFFSET;

  // Calculate/update dataCksum
  // Note: dataCkSum byte is last byte
  int i;
  for (i = IPMB_DATA_OFFSET; i < req_len-1; i++) {
    request[req_len-1] += request[i];
  }

  request[req_len-1] = ZERO_CKSUM_CONST - request[req_len-1];

  // Setup response buffer
  pthread_mutex_lock(&m_seq);
  g_seq.seq[index].p_buf = response;
  pthread_mutex_unlock(&m_seq);

  // Send request over i2c bus
  // Note: Need not send first byte SlaveAddress automatically added by driver
  if (i2c_write(fd, &request[1], req_len-1)) {
    goto ipmb_handle_out;
  }

  // Wait on semaphore for that sequence Number
  clock_gettime(CLOCK_REALTIME, &ts);

  ts.tv_sec += TIMEOUT_IPMI;

  int ret;
  ret = sem_timedwait(&g_seq.seq[index].s_seq, &ts);
  if (ret == -1) {
    syslog(LOG_ALERT, "No response for sequence number: %d\n", index);
    *res_len = 0;
  }

ipmb_handle_out:
  // Reply to user with data
  pthread_mutex_lock(&m_seq);
  *res_len = g_seq.seq[index].len;

  g_seq.seq[index].in_use = false;
  pthread_mutex_unlock(&m_seq);

  return;
}

void
*conn_handler(void *sfd) {
  ipmb_sfd_t *p_sfd = (ipmb_sfd_t *) sfd;

  int sock = p_sfd->sock;
  int fd = p_sfd->fd;
  int n;
  unsigned char req_buf[MAX_IPMI_MSG_SIZE];
  unsigned char res_buf[MAX_IPMI_MSG_SIZE];
  unsigned char res_len = 0;

  n = recv(sock, req_buf, sizeof(req_buf), 0);
  if (n <= 0) {
      syslog(LOG_ALERT, "ipmbd: recv() failed with %d\n", n);
      goto conn_cleanup;
  }

  ipmb_handle(fd, req_buf, n, res_buf, &res_len);

  if (send(sock, res_buf, res_len, MSG_NOSIGNAL) < 0) {
    syslog(LOG_ALERT, "ipmbd: send() failed\n");
  }

conn_cleanup:
  close(sock);
  free(p_sfd);

  pthread_exit(NULL);
  return 0;
}

// Thread to receive the IPMB lib messages from various apps
static void*
ipmb_lib_handler(void *bus_num) {
  int s, s2, t, len;
  struct sockaddr_un local, remote;
  pthread_t tid;
  ipmb_sfd_t *sfd;
  int fd;
  uint8_t *bnum = (uint8_t*) bus_num;
  char sock_path[20] = {0};

  // Open the i2c bus for sending request
  fd = i2c_open(*bnum);
  if (fd < 0) {
    syslog(LOG_ALERT, "i2c_open failure\n");
    return NULL;
  }

  // Initialize g_seq structure
  int i;
  for (i = 0; i < SEQ_NUM_MAX; i++) {
    g_seq.seq[i].in_use = false;
    sem_init(&g_seq.seq[i].s_seq, 0, 0);
    g_seq.seq[i].len = 0;
  }

  // Initialize mutex to access global structure
  pthread_mutex_init(&m_seq, NULL);

  if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    syslog(LOG_ALERT, "ipmbd: socket() failed\n");
    exit (1);
  }

  sprintf(sock_path, "%s_%d", SOCK_PATH_IPMB, *bnum);

  local.sun_family = AF_UNIX;
  strcpy (local.sun_path, sock_path);
  unlink (local.sun_path);
  len = strlen (local.sun_path) + sizeof (local.sun_family);
  if (bind (s, (struct sockaddr *) &local, len) == -1)
  {
    syslog(LOG_ALERT, "ipmbd: bind() failed\n");
    exit (1);
  }

  if (listen (s, 5) == -1)
  {
    syslog(LOG_ALERT, "ipmbd: listen() failed\n");
    exit (1);
  }

  while(1) {
    int n;
    t = sizeof (remote);
    if ((s2 = accept (s, (struct sockaddr *) &remote, &t)) < 0) {
      syslog(LOG_ALERT, "ipmbd: accept() failed\n");
      break;
    }

    // Creating a worker thread to handle the request
    // TODO: Need to monitor the server performance with higher load and
    // see if we need to create pre-defined number of workers and schedule
    // the requests among them.
    sfd = (ipmb_sfd_t *) malloc(sizeof(ipmb_sfd_t));
    sfd->fd = fd;
    sfd->sock = s2;
    if (pthread_create(&tid, NULL, conn_handler, (void*) sfd) < 0) {
        syslog(LOG_ALERT, "ipmbd: pthread_create failed\n");
        close(s2);
        continue;
    }

    pthread_detach(tid);
  }

  close(s);
  pthread_mutex_destroy(&m_seq);

  return 0;
}

int
main(int argc, char * const argv[]) {
  pthread_t tid_ipmb_rx;
  pthread_t tid_req_handler;
  pthread_t tid_res_handler;
  pthread_t tid_lib_handler;
  uint8_t ipmb_bus_num;
  mqd_t mqd_req, mqd_res;
  struct mq_attr attr;
  char mq_ipmb_req[64] = {0};
  char mq_ipmb_res[64] = {0};

  daemon(1, 0);
  openlog("ipmbd", LOG_CONS, LOG_DAEMON);

  if (argc != 2) {
    syslog(LOG_ALERT, "ipmbd: Usage: ipmbd <bus#>");
    exit(1);
  }

  ipmb_bus_num = atoi(argv[1]);
syslog(LOG_ALERT, "ipmbd: bus#:%d\n", ipmb_bus_num);

  pthread_mutex_init(&m_i2c, NULL);

  // Create Message Queues for Request Messages and Response Messages
  attr.mq_flags = 0;
  attr.mq_maxmsg = MQ_MAX_NUM_MSGS;
  attr.mq_msgsize = MQ_MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  sprintf(mq_ipmb_req, "%s_%d", MQ_IPMB_REQ, ipmb_bus_num);
  sprintf(mq_ipmb_res, "%s_%d", MQ_IPMB_RES, ipmb_bus_num);

  // Remove the MQ if exists
  mq_unlink(mq_ipmb_req);

  errno = 0;
  mqd_req = mq_open(mq_ipmb_req, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  if (mqd_req == (mqd_t) -1) {
    syslog(LOG_ALERT, "ipmbd: mq_open request failed errno:%d\n", errno);
    goto cleanup;
  }

  // Remove the MQ if exists
  mq_unlink(mq_ipmb_res);

  errno = 0;
  mqd_res = mq_open(mq_ipmb_res, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  if (mqd_res == (mqd_t) -1) {
    syslog(LOG_ALERT, "ipmbd: mq_open response failed errno: %d\n", errno);
    goto cleanup;
  }

  // Create thread to handle IPMB Requests
  if (pthread_create(&tid_req_handler, NULL, ipmb_req_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_ALERT, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

  // Create thread to handle IPMB Responses
  if (pthread_create(&tid_res_handler, NULL, ipmb_res_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_ALERT, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

  // Create thread to retrieve ipmb traffic from i2c bus as slave
  if (pthread_create(&tid_ipmb_rx, NULL, ipmb_rx_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_ALERT, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

  // Create thread to receive ipmb library requests from apps
  if (pthread_create(&tid_lib_handler, NULL, ipmb_lib_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_ALERT, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

cleanup:
  if (tid_ipmb_rx > 0) {
    pthread_join(tid_ipmb_rx, NULL);
  }

  if (tid_req_handler > 0) {
    pthread_join(tid_req_handler, NULL);
  }

  if (tid_res_handler > 0) {
    pthread_join(tid_res_handler, NULL);
  }

  if (tid_lib_handler > 0) {
    pthread_join(tid_lib_handler, NULL);
  }

  if (mqd_res > 0) {
    mq_close(mqd_res);
    mq_unlink(mq_ipmb_res);
  }

  if (mqd_req > 0) {
    mq_close(mqd_req);
    mq_unlink(mq_ipmb_req);
  }

  pthread_mutex_destroy(&m_i2c);

  return 0;
}
