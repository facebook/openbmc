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
#include <poll.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/ipc.h>
#include "openbmc/ipmi.h"
#include "openbmc/ipmb.h"

//#define DEBUG 0

#define MAX_BYTES 300

#define MQ_IPMB_REQ "/mq_ipmb_req"
#define MQ_IPMB_RES "/mq_ipmb_res"
#define MQ_MAX_MSG_SIZE MAX_BYTES
#define MQ_MAX_NUM_MSGS 256

#define SEQ_NUM_MAX 64

#define I2C_RETRIES_MAX 15

#define IPMB_PKT_MIN_SIZE 6

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

static int g_bus_id = 0; // store the i2c bus ID for debug print
static int g_payload_id = 1; // Store the payload ID we need to use

static int i2c_slave_read(int fd, uint8_t *buf, uint8_t *len);
static int i2c_slave_open(uint8_t bus_num);
static int bic_up_flag = 0;

// Calculate checksum
static inline uint8_t
calc_cksum(uint8_t *buf, uint8_t len) {
  uint8_t i = 0;
  uint8_t cksum = 0;

  for (i = 0; i < len; i++) {
    cksum += buf[i];
  }

  return (ZERO_CKSUM_CONST - cksum);
}

static void seq_init() {
  // Initialize mutex to access global structure
  pthread_mutex_init(&m_seq, NULL);
  // Initialize g_seq structure
  int i;
  g_seq.curr_seq = 0;
  for (i = 0; i < SEQ_NUM_MAX; i++) {
    g_seq.seq[i].in_use = false;
    sem_init(&g_seq.seq[i].s_seq, 0, 0);
    g_seq.seq[i].len = 0;
    g_seq.seq[i].p_buf = NULL;
  }
}

static int seq_put(uint8_t seq, uint8_t *buf, uint8_t len)
{
  seq_buf_t *s;
  int rc = -1;

  if (seq >= SEQ_NUM_MAX) {
    return -1;
  }
  // Check if the response is being waited for
  pthread_mutex_lock(&m_seq);
  s = &g_seq.seq[seq];
  if (s->in_use && s->p_buf) {
    // Copy the response to the requester's buffer
    memcpy(s->p_buf, buf, len);
    s->len = len;

    // Wake up the worker thread to receive the response
    sem_post(&s->s_seq);
    rc = 0;
  }
  pthread_mutex_unlock(&m_seq);
  return rc;
}

// Returns an unused seq# from all possible seq#
static int8_t
seq_get_new(unsigned char *resp) {
  int8_t ret = -1;
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
      g_seq.seq[index].p_buf = resp;
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
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, BRIDGE_SLAVE_ADDR);
  if (rc < 0) {
    syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", BRIDGE_SLAVE_ADDR);
    close(fd);
    return -1;
  }

  return fd;
}

static int
i2c_write(int fd, uint8_t *buf, uint16_t len) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  int rc;
  int i;

  memset(&msg, 0, sizeof(msg));

  msg.addr = buf[0] >> 1;
  msg.flags = 0;
  msg.len = len - 1; // 1st byte in addr
  msg.buf = &buf[1];

  data.msgs = &msg;
  data.nmsgs = 1;

  pthread_mutex_lock(&m_i2c);

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    rc = ioctl(fd, I2C_RDWR, &data);
    if (rc < 0) {
      msleep(20);
      continue;
    } else {
      break;
    }
  }

  if (rc < 0) {
    syslog(LOG_WARNING, "bus: %d, Failed to do raw io", g_bus_id);
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
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
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
    syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", BMC_SLAVE_ADDR);
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
  uint16_t tlen = 0;

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
    syslog(LOG_WARNING, "i2c_open failure\n");
    mq_close(mq);
    return NULL;
  }

  // Loop to process incoming requests
  while (1) {
    if ((rlen = mq_receive(mq, (char *)rxbuf, MQ_MAX_MSG_SIZE, NULL)) <= 0) {
      sleep(1);
      continue;
    }

    pal_ipmb_processing(g_bus_id, rxbuf, rlen);

#ifdef DEBUG
    syslog(LOG_WARNING, "Received Request of %d bytes\n", rlen);
    for (i = 0; i < rlen; i++) {
      syslog(LOG_WARNING, "0x%X", rxbuf[i]);
    }
#endif

    // Create IPMI request from IPMB data
    p_ipmi_mn_req->payload_id = g_payload_id;
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
    syslog(LOG_WARNING, "Sending Response of %d bytes\n", tlen+IPMB_HDR_SIZE-1);
    for (i = 0; i < tlen+IPMB_HDR_SIZE; i++) {
      syslog(LOG_WARNING, "0x%X:", txbuf[i]);
    }
#endif

     // Send response back
     i2c_write(fd, txbuf, tlen+IPMB_HDR_SIZE);

     pal_ipmb_finished(g_bus_id, txbuf, tlen+IPMB_HDR_SIZE);
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
    syslog(LOG_WARNING, "mq_open fails\n");
    return NULL;
  }

  // Loop to wait for incomng response messages
  while (1) {
    if ((len = mq_receive(mq, (char *)buf, MQ_MAX_MSG_SIZE, NULL)) <= 0) {
      sleep(1);
      continue;
    }

    p_res = (ipmb_res_t *) buf;


    // Check the seq# of response
    index = p_res->seq_lun >> LUN_OFFSET;

    if (seq_put(index, buf, len)) {
      // Either the IPMB packet is corrupted or arrived late after client exits
      syslog(LOG_WARNING, "bus: %d, WRONG packet received with seq#%d\n", g_bus_id, index);
    }

#ifdef DEBUG
    syslog(LOG_WARNING, "Received Response of %d bytes\n", len);
    int i;
    for (i = 0; i < len; i++) {
      syslog(LOG_WARNING, "0x%X:", buf[i]);
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
  mqd_t mq_req = (mqd_t)-1, mq_res, tmq;
  ipmb_req_t *p_req;
  struct timespec req;
  char mq_ipmb_req[64] = {0};
  char mq_ipmb_res[64] = {0};
  uint8_t tbuf[MAX_BYTES] = { 0 };
  uint8_t fbyte;

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 10000000;//10mSec

  // Open the i2c bus as a slave
  fd = i2c_slave_open(*bnum);
  if (fd < 0) {
    syslog(LOG_WARNING, "i2c_slave_open fails\n");
    goto cleanup;
  }

  sprintf(mq_ipmb_req, "%s_%d", MQ_IPMB_REQ, *bnum);
  sprintf(mq_ipmb_res, "%s_%d", MQ_IPMB_RES, *bnum);

  // Open the message queues for post processing
  mq_req = mq_open(mq_ipmb_req, O_WRONLY);
  if (mq_req == (mqd_t) -1) {
    syslog(LOG_WARNING, "mq_open req fails\n");
    goto cleanup;
  }

  mq_res = mq_open(mq_ipmb_res, O_WRONLY);
  if (mq_res == (mqd_t) -1) {
    syslog(LOG_WARNING, "mq_open res fails\n");
    goto cleanup;
  }

  struct pollfd ufds[1];
  ufds[0].fd= fd;
  ufds[0].events = POLLIN;
  // Loop that retrieves messages
  while (1) {
    // Read messages from i2c driver
     if (i2c_slave_read(fd, buf, &len) < 0) {
      poll(ufds, 1, 10);
      continue;
    }

    // TODO: HACK: Due to i2cdriver issues, we are seeing two different type of packet corruptions
    // 1. The firstbyte(BMC's slave address) byte is same as second byte
    //    Workaround: Replace the first byte with correct slave address
    // 2. The missing slave address as first byte
    //    Workaround: move the buffer by one byte and add the correct slave address
    // Verify the IPMB hdr cksum: first two bytes are hdr and 3-rd byte cksum

    if (len < IPMB_PKT_MIN_SIZE) {
      syslog(LOG_WARNING, "bus: %d, IPMB Packet invalid size %d", g_bus_id, len);
      continue;
    }

    if (buf[2] != calc_cksum(buf, 2)) {
      //handle wrong slave address
      if (buf[0] != BMC_SLAVE_ADDR<<1) {
        // Store the first byte
        fbyte = buf[0];
        // Update the first byte with correct slave address
        buf[0] = BMC_SLAVE_ADDR<<1;
        // Check again if the cksum passes
        if (buf[2] != calc_cksum(buf,2)) {
          //handle missing slave address
          // restore the first byte
          buf[0] = fbyte;
          //copy the buffer to temporary
          memcpy(tbuf, buf, len);
          // correct the slave address
          buf[0] = BMC_SLAVE_ADDR<<1;
          // copy back from temp buffer
          memcpy(&buf[1], tbuf, len);
          // increase length as we added slave address byte
          len++;
          // Check if the above hacks corrected the header
          if (buf[2] != calc_cksum(buf,2)) {
            syslog(LOG_WARNING, "bus: %d, IPMB Header cksum error after correcting slave address\n", g_bus_id);
            continue;
          }
        }
      } else {
          syslog(LOG_WARNING, "bus: %d, IPMB Header cksum does not match\n", g_bus_id);
          continue;
      }
    }

    // Verify the IPMB data cksum: data starts from 4-th byte
    if (buf[len-1] != calc_cksum(&buf[3], len-4)) {
      syslog(LOG_WARNING, "bus: %d, IPMB Data cksum does not match\n", g_bus_id);
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
    if (mq_timedsend(tmq, (char *)buf, len, 0, &req)) {
      //syslog(LOG_WARNING, "mq_send failed for queue %d\n", tmq);
      msleep(10);
      continue;
    }
  }

cleanup:
  if (fd > 0) {
    close (fd);
  }

  if (mq_req > 0) {
    mq_close(mq_req);
  }

  if (mq_res > 0) {
    mq_close(mq_req);
  }
  return NULL;
}

/*
 * Function to handle all IPMB requests
 */
static void
ipmb_handle (int fd, unsigned char *request, unsigned short req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmb_req_t *req = (ipmb_req_t *) request;

  int8_t index;
  struct timespec ts;

  // Allocate right sequence Number
  index = seq_get_new(response);
  if (index < 0) {
    *res_len = 0;
    return ;
  }

  req->seq_lun = index << LUN_OFFSET;
  req->req_slave_addr = BMC_SLAVE_ADDR << 1;

  // Calculate/update header Cksum
  req->hdr_cksum = req->res_slave_addr +
                   req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  // Calculate/update dataCksum
  // Note: dataCkSum byte is last byte
  int i;
  request[req_len-1] = 0;
  for (i = IPMB_DATA_OFFSET; i < req_len-1; i++) {
    request[req_len-1] += request[i];
  }

  request[req_len-1] = ZERO_CKSUM_CONST - request[req_len-1];

  if (pal_ipmb_processing(g_bus_id, request, req_len)) {
    goto ipmb_handle_out;
  }

  // Send request over i2c bus
  if (i2c_write(fd, request, req_len)) {
    goto ipmb_handle_out;
  }

  // Wait on semaphore for that sequence Number
  clock_gettime(CLOCK_REALTIME, &ts);

  ts.tv_sec += TIMEOUT_IPMB;

  int ret;
  ret = sem_timedwait(&g_seq.seq[index].s_seq, &ts);
  if (ret == -1) {
    syslog(LOG_DEBUG, "bus: %d, No response for sequence number: %d\n", g_bus_id, index);
    *res_len = 0;
  }

ipmb_handle_out:
  // Reply to user with data
  pthread_mutex_lock(&m_seq);
  *res_len = g_seq.seq[index].len;

  g_seq.seq[index].in_use = false;
  g_seq.seq[index].p_buf = NULL;
  pthread_mutex_unlock(&m_seq);

  pal_ipmb_finished(g_bus_id, request, *res_len);

  return;
}


struct ipmb_svc_cookie {
  int i2c_fd;
};

int
conn_handler(client_t *cli) {
  struct ipmb_svc_cookie *svc = (struct ipmb_svc_cookie *)cli->svc_cookie;
  unsigned char req_buf[MAX_IPMB_RES_LEN];
  unsigned char res_buf[MAX_IPMB_RES_LEN];
  size_t req_len = MAX_IPMB_RES_LEN;
  unsigned char res_len;

  if (ipc_recv_req(cli, req_buf, &req_len, TIMEOUT_IPMB)) {
    syslog(LOG_WARNING, "ipmbd: recv() failed\n");
    return -1;
  }

  if(bic_up_flag){
    if(!((req_buf[1] == 0xe0) && (req_buf[5] == CMD_OEM_1S_ENABLE_BIC_UPDATE))){
      return -1;
	  }
  }

  ipmb_handle(svc->i2c_fd, req_buf, (unsigned char)req_len, res_buf, &res_len);

  if(ipc_send_resp(cli, res_buf, res_len) != 0) {
    syslog(LOG_WARNING, "ipmbd: send() failed!\n");
    return -1;
  }
  return 0;
}


// Thread to receive the IPMB lib messages from various apps
static int
start_ipmb_lib_handler(uint8_t bus_num) {
  char sock_path[64];
  struct ipmb_svc_cookie *svc = calloc(1, sizeof(*svc));
  if (!svc) {
    return -1;
  }

  // Open the i2c bus for sending request
  svc->i2c_fd = i2c_open(bus_num);
  if (svc->i2c_fd < 0) {
    syslog(LOG_WARNING, "i2c_open failure\n");
    free(svc);
    return -1;
  }

  // Initialize mutex to access global structure
  pthread_mutex_init(&m_seq, NULL);

  snprintf(sock_path, sizeof(sock_path), "%s_%d", SOCK_PATH_IPMB, bus_num);
  if (ipc_start_svc(sock_path, conn_handler, SEQ_NUM_MAX, svc, NULL)) {
    free(svc);
    return -1;
  }
  return 0;
}

int
main(int argc, char * const argv[]) {
  pthread_t tid_ipmb_rx;
  pthread_t tid_req_handler;
  pthread_t tid_res_handler;
  uint8_t ipmb_bus_num;
  mqd_t mqd_req = (mqd_t)-1, mqd_res = (mqd_t)-1;
  struct mq_attr attr;
  char mq_ipmb_req[64] = {0};
  char mq_ipmb_res[64] = {0};
  int rc = 0;

  if (argc < 3) {
    syslog(LOG_WARNING, "ipmbd: Usage: ipmbd <bus#> <payload#> [bicup = allow bic updates]");
    exit(1);
  }

  ipmb_bus_num = (uint8_t)strtoul(argv[1], NULL, 0);
  g_bus_id = ipmb_bus_num;

  g_payload_id = (uint8_t)strtoul(argv[2], NULL, 0);

  syslog(LOG_WARNING, "ipmbd: bus#:%d payload#:%u\n", ipmb_bus_num, g_payload_id);

  if( (argc >= 4) && !(strcmp(argv[3] ,"bicup")) ){
    bic_up_flag = 1;
  } else {
    bic_up_flag = 0;
  }

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

  mqd_req = mq_open(mq_ipmb_req, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  if (mqd_req == (mqd_t) -1) {
    rc = errno;
    syslog(LOG_WARNING, "ipmbd: mq_open request failed errno:%d\n", rc);
    goto cleanup;
  }

  // Remove the MQ if exists
  mq_unlink(mq_ipmb_res);

  mqd_res = mq_open(mq_ipmb_res, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  if (mqd_res == (mqd_t) -1) {
    rc = errno;
    syslog(LOG_WARNING, "ipmbd: mq_open response failed errno: %d\n", rc);
    goto cleanup;
  }

  seq_init();

  // Create thread to handle IPMB Requests
  if (pthread_create(&tid_req_handler, NULL, ipmb_req_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_WARNING, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

  // Create thread to handle IPMB Responses
  if (pthread_create(&tid_res_handler, NULL, ipmb_res_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_WARNING, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

  // Create thread to retrieve ipmb traffic from i2c bus as slave
  if (pthread_create(&tid_ipmb_rx, NULL, ipmb_rx_handler, (void*) &ipmb_bus_num) < 0) {
    syslog(LOG_WARNING, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

  // Create thread to receive ipmb library requests from apps
  if (start_ipmb_lib_handler(ipmb_bus_num) < 0) {
    syslog(LOG_WARNING, "ipmbd: pthread_create failed\n");
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
