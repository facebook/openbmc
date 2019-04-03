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
#include <unistd.h>
#include <stdint.h>
#include <mqueue.h>
#include <semaphore.h>
#include <poll.h>
#include <assert.h>
#include <linux/limits.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/ipc.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>

//#define DEBUG 0

#define MAX_BYTES 300

/*
 * Message queue definitions.
 */
#define MQ_IPMB_REQ             "/mq_ipmb_req"
#define MQ_IPMB_RES             "/mq_ipmb_res"
#define MQ_MAX_MSG_SIZE         MAX_BYTES
#define MQ_MAX_NUM_MSGS         256
#define MQ_DESC_INVALID         ((mqd_t)-1)
#define MQ_DFT_FLAGS            (O_RDONLY | O_CREAT)
#define MQ_DFT_MODES            (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MQ_DFT_ATTR_INITIALIZER { \
  .mq_flags = 0,                  \
  .mq_maxmsg = MQ_MAX_NUM_MSGS,   \
  .mq_msgsize = MQ_MAX_MSG_SIZE,  \
  .mq_curmsgs = 0,                \
}

#define SEQ_NUM_MAX 64

#define I2C_RETRIES_MAX 15
#define I2C_RETRY_DELAY 20 /* unit: millisecond */

#define IPMB_PKT_MIN_SIZE 6

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */

// Structure for sequence number and buffer
typedef struct {
  bool in_use; // seq# is being used
  uint8_t len; // buffer size
  uint8_t *p_buf; // pointer to buffer
  sem_t seq_sem; // semaphore for thread sync.
} seq_buf_t;

// Structure for holding currently used sequence number and
// array of all possible sequence number
static struct {
  pthread_mutex_t seq_mutex;

  uint8_t curr_seq; // currently used seq#
  seq_buf_t seq[SEQ_NUM_MAX]; //array of all possible seq# struct.
} ipmb_seq_buf = {
  .seq_mutex = PTHREAD_MUTEX_INITIALIZER,
};

static pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_bus_id = 0; // store the i2c bus ID for debug print
static int g_payload_id = 1; // Store the payload ID we need to use
static int bic_up_flag = 0;

static char*
mq_name_gen(char *buf, size_t size, const char *prefix, uint8_t bus)
{
  snprintf(buf, size, "%s_%u", prefix, bus);
  return buf;
}

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

static void ipmb_seq_buf_init(void) {
  // Initialize ipmb_seq_buf structure
  int i;
  for (i = 0; i < ARRAY_SIZE(ipmb_seq_buf.seq); i++) {
    ipmb_seq_buf.seq[i].in_use = false;
    assert(sem_init(&ipmb_seq_buf.seq[i].seq_sem, 0, 0) == 0);
    ipmb_seq_buf.seq[i].len = 0;
    ipmb_seq_buf.seq[i].p_buf = NULL;
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
  pthread_mutex_lock(&ipmb_seq_buf.seq_mutex);
  s = &ipmb_seq_buf.seq[seq];
  if (s->in_use && s->p_buf) {
    // Copy the response to the requester's buffer
    memcpy(s->p_buf, buf, len);
    s->len = len;

    // Wake up the worker thread to receive the response
    sem_post(&s->seq_sem);
    rc = 0;
  }
  pthread_mutex_unlock(&ipmb_seq_buf.seq_mutex);
  return rc;
}

// Returns an unused seq# from all possible seq#
static int8_t
seq_get_new(unsigned char *resp) {
  int8_t ret = -1;
  uint8_t index;

  pthread_mutex_lock(&ipmb_seq_buf.seq_mutex);

  // Search for unused sequence number
  index = ipmb_seq_buf.curr_seq;
  do {
    if (ipmb_seq_buf.seq[index].in_use == false) {
      // Found it!
      ret = index;
      ipmb_seq_buf.seq[index].in_use = true;
      ipmb_seq_buf.seq[index].len = 0;
      ipmb_seq_buf.seq[index].p_buf = resp;
      break;
    }

    if (++index == SEQ_NUM_MAX) {
      index = 0;
    }
  } while (index != ipmb_seq_buf.curr_seq);

  // Update the current seq num
  if (ret >= 0) {
    if (++index == SEQ_NUM_MAX) {
      index = 0;
    }
    ipmb_seq_buf.curr_seq = index;
  }

  pthread_mutex_unlock(&ipmb_seq_buf.seq_mutex);

  return ret;
}

static int
ipmb_open_satellite(uint8_t bus_num) {
  int fd;
  char fn[32];
  int rc;

  snprintf(fn, sizeof(fn), "/dev/i2c-%u", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s: %s",
           fn, strerror(errno));
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, BRIDGE_SLAVE_ADDR);
  if (rc < 0) {
    syslog(LOG_WARNING, "Failed to open slave @ address %#x: %s",
           BRIDGE_SLAVE_ADDR, strerror(errno));
    close(fd);
    return -1;
  }

  return fd;
}

static int
ipmb_write_satellite(int fd, uint8_t *buf, uint16_t len) {
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

  pthread_mutex_lock(&i2c_mutex);

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    rc = ioctl(fd, I2C_RDWR, &data);
    if (rc < 0) {
      msleep(I2C_RETRY_DELAY);
      continue;
    } else {
      break;
    }
  }

  if (rc < 0) {
    syslog(LOG_WARNING, "bus: %d, Failed to do raw io", g_bus_id);
  }

  pthread_mutex_unlock(&i2c_mutex);

  return (rc < 0 ? -1 : 0);
}

static int
ipmb_open_bmc(uint8_t bus_num) {
  int fd;
  char fn[32];
  int rc;
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  uint8_t read_bytes[MAX_BYTES] = { 0 };

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s: %s",
           fn, strerror(errno));
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
    syslog(LOG_WARNING, "Failed to open slave @ address %#x: %s",
           BMC_SLAVE_ADDR, strerror(errno));
    close(fd);
  }

  return fd;
}

static int
ipmb_read_bmc(int fd, uint8_t *buf, uint8_t *len) {
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
ipmb_req_handler(void *args) {
  uint8_t bus_num = *((uint8_t*)args);
  mqd_t mq;
  int i, fd;
  uint8_t rlen = 0;
  uint16_t tlen = 0;
  char mq_name_req[NAME_MAX];

  //Buffers for IPMB transport
  uint8_t rxbuf[MQ_MAX_MSG_SIZE] = {0};
  uint8_t txbuf[MQ_MAX_MSG_SIZE] = {0};
  ipmb_req_t *p_ipmb_req = (ipmb_req_t*)rxbuf;
  ipmb_res_t *p_ipmb_res = (ipmb_res_t*)txbuf;

  //Buffers for IPMI Stack
  uint8_t rbuf[MQ_MAX_MSG_SIZE] = {0};
  uint8_t tbuf[MQ_MAX_MSG_SIZE] = {0};
  ipmi_mn_req_t *p_ipmi_mn_req = (ipmi_mn_req_t*)rbuf;
  ipmi_res_t *p_ipmi_res = (ipmi_res_t*)tbuf;

  // Open Queue to receive requests
  mq_name_gen(mq_name_req, sizeof(mq_name_req), MQ_IPMB_REQ, bus_num);
  mq = mq_open(mq_name_req, O_RDONLY);
  if (mq == MQ_DESC_INVALID) {
    return NULL;
  }

  // Open the i2c bus for sending response
  fd = ipmb_open_satellite(bus_num);
  if (fd < 0) {
    syslog(LOG_WARNING, "ipmb_open_satellite failure\n");
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

    memcpy(p_ipmi_mn_req->data, p_ipmb_req->data,
           rlen - IPMB_HDR_SIZE - IPMI_REQ_HDR_SIZE);

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
     ipmb_write_satellite(fd, txbuf, tlen+IPMB_HDR_SIZE);

     pal_ipmb_finished(g_bus_id, txbuf, tlen+IPMB_HDR_SIZE);
  }
}

// Thread to handle the incoming responses
static void*
ipmb_res_handler(void *args) {
  uint8_t bus_num = *((uint8_t*)args);
  uint8_t buf[MQ_MAX_MSG_SIZE] = { 0 };
  uint8_t len = 0;
  mqd_t mq;
  ipmb_res_t *p_res;
  uint8_t index;
  char mq_name_res[NAME_MAX];

  // Open the message queue
  mq_name_gen(mq_name_res, sizeof(mq_name_res), MQ_IPMB_RES, bus_num);
  mq = mq_open(mq_name_res, O_RDONLY);
  if (mq == MQ_DESC_INVALID) {
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
ipmb_rx_handler(void *args) {
  uint8_t bus_num = *((uint8_t*)args);
  int fd, ret;
  uint8_t len;
  uint8_t tlun;
  uint8_t buf[MAX_BYTES] = { 0 };
  mqd_t mq_req = MQ_DESC_INVALID;
  mqd_t mq_res = MQ_DESC_INVALID;
  ipmb_req_t *p_req;
  struct timespec req;
  char mq_name_req[NAME_MAX];
  char mq_name_res[NAME_MAX];
  uint8_t tbuf[MAX_BYTES] = { 0 };
  uint8_t fbyte;
  struct pollfd ufds[1];

  // Setup wait time
  req.tv_sec = 0;
  req.tv_nsec = 10000000;//10mSec

  // Open the i2c bus as a slave
  fd = ipmb_open_bmc(bus_num);
  if (fd < 0) {
    syslog(LOG_WARNING, "ipmb_open_bmc fails\n");
    goto cleanup;
  }

  // Open the message queues for post processing
  mq_name_gen(mq_name_req, sizeof(mq_name_req), MQ_IPMB_REQ, bus_num);
  mq_req = mq_open(mq_name_req, O_WRONLY);
  if (mq_req == MQ_DESC_INVALID) {
    syslog(LOG_WARNING, "mq_open req fails\n");
    goto cleanup;
  }

  mq_name_gen(mq_name_res, sizeof(mq_name_res), MQ_IPMB_RES, bus_num);
  mq_res = mq_open(mq_name_res, O_WRONLY);
  if (mq_res == MQ_DESC_INVALID) {
    syslog(LOG_WARNING, "mq_open res fails\n");
    goto cleanup;
  }

  ufds[0].fd= fd;
  ufds[0].events = POLLIN;
  // Loop that retrieves messages
  while (1) {
    // Read messages from i2c driver
    if (ipmb_read_bmc(fd, buf, &len) < 0) {
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
    // Post message to approriate Queue for further processing
    p_req = (ipmb_req_t*) buf;
    tlun = p_req->netfn_lun >> LUN_OFFSET;
    if (tlun % 2) {
      ret = mq_timedsend(mq_res, (char *)buf, len, 0, &req);
    } else {
      ret = mq_timedsend(mq_req, (char *)buf, len, 0, &req);
    }
    if (ret != 0) {
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
  int i, ret;
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
  request[req_len-1] = 0;
  for (i = IPMB_DATA_OFFSET; i < req_len-1; i++) {
    request[req_len-1] += request[i];
  }

  request[req_len-1] = ZERO_CKSUM_CONST - request[req_len-1];

  if (pal_ipmb_processing(g_bus_id, request, req_len)) {
    goto ipmb_handle_out;
  }

  // Send request over i2c bus
  if (ipmb_write_satellite(fd, request, req_len)) {
    goto ipmb_handle_out;
  }

  // Wait on semaphore for that sequence Number
  clock_gettime(CLOCK_REALTIME, &ts);

  ts.tv_sec += TIMEOUT_IPMB;

  ret = sem_timedwait(&ipmb_seq_buf.seq[index].seq_sem, &ts);
  if (ret == -1) {
    syslog(LOG_DEBUG, "bus: %d, No response for sequence number: %d\n", g_bus_id, index);
    *res_len = 0;
  }

ipmb_handle_out:
  // Reply to user with data
  pthread_mutex_lock(&ipmb_seq_buf.seq_mutex);
  *res_len = ipmb_seq_buf.seq[index].len;

  ipmb_seq_buf.seq[index].in_use = false;
  ipmb_seq_buf.seq[index].p_buf = NULL;
  pthread_mutex_unlock(&ipmb_seq_buf.seq_mutex);

  pal_ipmb_finished(g_bus_id, request, *res_len);

  return;
}


struct ipmb_svc_cookie {
  int i2c_fd;
};

static int
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

  ipmb_handle(svc->i2c_fd, req_buf, (unsigned short)req_len, res_buf, &res_len);

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
  svc->i2c_fd = ipmb_open_satellite(bus_num);
  if (svc->i2c_fd < 0) {
    syslog(LOG_WARNING, "ipmb_open_satellite failure\n");
    free(svc);
    return -1;
  }

  snprintf(sock_path, sizeof(sock_path), "%s_%d", SOCK_PATH_IPMB, bus_num);
  if (ipc_start_svc(sock_path, conn_handler, SEQ_NUM_MAX, svc, NULL)) {
    free(svc);
    return -1;
  }
  return 0;
}

int
main(int argc, char * const argv[]) {
  mqd_t mqd_req = MQ_DESC_INVALID;
  mqd_t mqd_res = MQ_DESC_INVALID;
  struct mq_attr attr = MQ_DFT_ATTR_INITIALIZER;
  char mq_name_req[NAME_MAX];
  char mq_name_res[NAME_MAX];
  int i, rc = 0;
  struct {
    const char *name;
    void* (*handler)(void *args);
    bool initialized;
    pthread_t tid;
  } ipmb_threads[3] = {
    {
      .name = "ipmb_rx_handler",
      .handler = ipmb_rx_handler,
      .initialized = false,
    },
    {
      .name = "ipmb_req_handler",
      .handler = ipmb_req_handler,
      .initialized = false,
    },
    {
      .name = "ipmb_res_handler",
      .handler = ipmb_res_handler,
      .initialized = false,
    },
  };

  if (argc < 3) {
    syslog(LOG_WARNING, "ipmbd: Usage: ipmbd <bus#> <payload#> [bicup = allow bic updates]");
    exit(1);
  }

  g_bus_id = (uint8_t)strtoul(argv[1], NULL, 0);

  g_payload_id = (uint8_t)strtoul(argv[2], NULL, 0);

  syslog(LOG_WARNING, "ipmbd: bus#:%d payload#:%u\n", g_bus_id, g_payload_id);

  if( (argc >= 4) && !(strcmp(argv[3] ,"bicup")) ){
    bic_up_flag = 1;
  } else {
    bic_up_flag = 0;
  }

  mq_name_gen(mq_name_req, sizeof(mq_name_req), MQ_IPMB_REQ, g_bus_id);
  mq_name_gen(mq_name_res, sizeof(mq_name_res), MQ_IPMB_RES, g_bus_id);

  // Remove the MQ if exists
  mq_unlink(mq_name_req);
  mqd_req = mq_open(mq_name_req, MQ_DFT_FLAGS, MQ_DFT_MODES, &attr);
  if (mqd_req == (mqd_t) -1) {
    rc = errno;
    syslog(LOG_WARNING, "ipmbd: mq_open request failed errno:%d\n", rc);
    goto cleanup;
  }

  // Remove the MQ if exists
  mq_unlink(mq_name_res);
  mqd_res = mq_open(mq_name_res, MQ_DFT_FLAGS, MQ_DFT_MODES, &attr);
  if (mqd_res == (mqd_t) -1) {
    rc = errno;
    syslog(LOG_WARNING, "ipmbd: mq_open response failed errno: %d\n", rc);
    goto cleanup;
  }

  ipmb_seq_buf_init();

  for (i = 0; i < ARRAY_SIZE(ipmb_threads); i++) {
    rc = pthread_create(&ipmb_threads[i].tid, NULL,
                        ipmb_threads[i].handler, &g_bus_id);
    if (rc != 0) {
      syslog(LOG_WARNING, "ipmbd: failed to create %s thread: %s\n",
             ipmb_threads[i].name, strerror(rc));
      goto cleanup;
    }
    ipmb_threads[i].initialized = true;
  }

  // Create thread to receive ipmb library requests from apps
  if (start_ipmb_lib_handler(g_bus_id) < 0) {
    syslog(LOG_WARNING, "ipmbd: pthread_create failed\n");
    goto cleanup;
  }

cleanup:
  for (i = ARRAY_SIZE(ipmb_threads) - 1; i >= 0; i--) {
    if (ipmb_threads[i].initialized) {
      pthread_join(ipmb_threads[i].tid, NULL);
      ipmb_threads[i].initialized = false;
    }
  }

  if (mqd_res != MQ_DESC_INVALID) {
    mq_close(mqd_res);
    mq_unlink(mq_name_res);
  }

  if (mqd_req != MQ_DESC_INVALID) {
    mq_close(mqd_req);
    mq_unlink(mq_name_req);
  }

  return 0;
}
