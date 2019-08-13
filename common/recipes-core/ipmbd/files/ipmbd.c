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
#include <getopt.h>
#include <linux/limits.h>
#include <openbmc/log.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/ipc.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>

/*
 * IPMB packet sizes.
 */
#define IPMB_PKT_MIN_SIZE 6
#define IPMB_PKT_MAX_SIZE 300

/*
 * I2C transaction retry parameters.
 */
#define I2C_RETRIES_MAX   15
#define I2C_RETRY_DELAY   20 /* unit: millisecond */

/*
 * Message queue definitions.
 */
#define MQ_DESC_INVALID         ((mqd_t)-1)
#define MQ_IPMB_REQ             "/mq_ipmb_req"
#define MQ_IPMB_RES             "/mq_ipmb_res"
#define MQ_MAX_NUM_MSGS         256
#define MQ_DFT_FLAGS            (O_RDONLY | O_CREAT)
#define MQ_DFT_MODES            (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MQ_DFT_ATTR_INITIALIZER {  \
  .mq_flags = 0,                   \
  .mq_maxmsg = MQ_MAX_NUM_MSGS,    \
  .mq_msgsize = IPMB_PKT_MAX_SIZE, \
  .mq_curmsgs = 0,                 \
}

#define SEQ_NUM_MAX 64

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */

#define IPMBD_RX_THREAD  "rx_handler"
#define IPMBD_REQ_THREAD "req_handler"
#define IPMBD_RES_THREAD "res_handler"
#define IPMBD_SVC_THREAD "svc_handler"
#define __VERBOSE(fmt, args...)       \
  do {                                \
    if (ipmbd_config.verbose_enabled) \
      OBMC_INFO(fmt, ##args);         \
  } while (0)
#define IPMBD_VERBOSE(fmt, args...) __VERBOSE(fmt, ##args)
#define RX_VERBOSE(fmt, args...)  __VERBOSE(IPMBD_RX_THREAD ": " fmt, ##args)
#define REQ_VERBOSE(fmt, args...) __VERBOSE(IPMBD_REQ_THREAD ": " fmt, ##args)
#define RES_VERBOSE(fmt, args...) __VERBOSE(IPMBD_RES_THREAD ": " fmt, ##args)
#define SVC_VERBOSE(fmt, args...) __VERBOSE(IPMBD_SVC_THREAD ": " fmt, ##args)

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

static struct {
  int bus_id;
  int payload_id;

  /* global flags */
  unsigned int bic_update_enabled:1;
  unsigned int verbose_enabled:1;
} ipmbd_config = {
  .bus_id = -1,
  .payload_id = -1,
};

/*
 * Name format: ipmbd_<bus-id>. Mainly for logging purpose.
 */
static char ipmbd_name[NAME_MAX];

static void
ipmbd_name_init(int bus_num)
{
  snprintf(ipmbd_name, sizeof(ipmbd_name), "ipmbd_%d", bus_num);
}

static char*
ipc_name_gen(char *buf, size_t size, const char *prefix, int bus_num)
{
  snprintf(buf, size, "%s_%d", prefix, bus_num);
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

  if (seq >= ARRAY_SIZE(ipmb_seq_buf.seq)) {
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
    if (!ipmb_seq_buf.seq[index].in_use) {
      // Found it!
      ret = index;
      ipmb_seq_buf.seq[index].in_use = true;
      ipmb_seq_buf.seq[index].len = 0;
      ipmb_seq_buf.seq[index].p_buf = resp;
      break;
    }

    if (++index == ARRAY_SIZE(ipmb_seq_buf.seq)) {
      index = 0;
    }
  } while (index != ipmb_seq_buf.curr_seq);

  // Update the current seq num
  if (ret >= 0) {
    if (++index == ARRAY_SIZE(ipmb_seq_buf.seq)) {
      index = 0;
    }
    ipmb_seq_buf.curr_seq = index;
  }

  pthread_mutex_unlock(&ipmb_seq_buf.seq_mutex);

  return ret;
}

static int
ipmb_write_satellite(int fd, uint8_t *buf, uint16_t len) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg;
  int rc;
  int i = 0;

  memset(&msg, 0, sizeof(msg));

  msg.addr = buf[0] >> 1;
  msg.flags = 0;
  msg.len = len - 1; // 1st byte in addr
  msg.buf = &buf[1];

  data.msgs = &msg;
  data.nmsgs = 1;

  pthread_mutex_lock(&i2c_mutex);
  while ((rc = ioctl(fd, I2C_RDWR, &data)) < 0 &&
         ++i < I2C_RETRIES_MAX) {
    msleep(I2C_RETRY_DELAY);
  }
  if (rc < 0) {
    OBMC_ERROR(errno, "Failed to send %u bytes to device @%#x",
               len, msg.addr);
  }
  pthread_mutex_unlock(&i2c_mutex);

  return (rc < 0 ? -1 : 0);
}

// Thread to handle new requests
static void*
ipmb_req_handler(void *args) {
  int bus_num = *((int*)args);
  mqd_t mq;
  int i, fd;
  uint8_t rlen = 0;
  uint16_t tlen = 0;
  char mq_name_req[NAME_MAX];
  uint16_t addr=0;
  int ret=0;

  //Buffers for IPMB transport
  uint8_t rxbuf[IPMB_PKT_MAX_SIZE] = {0};
  uint8_t txbuf[IPMB_PKT_MAX_SIZE] = {0};
  ipmb_req_t *p_ipmb_req = (ipmb_req_t*)rxbuf;
  ipmb_res_t *p_ipmb_res = (ipmb_res_t*)txbuf;

  //Buffers for IPMI Stack
  uint8_t rbuf[IPMB_PKT_MAX_SIZE] = {0};
  uint8_t tbuf[IPMB_PKT_MAX_SIZE] = {0};
  ipmi_mn_req_t *p_ipmi_mn_req = (ipmi_mn_req_t*)rbuf;
  ipmi_res_t *p_ipmi_res = (ipmi_res_t*)tbuf;

  REQ_VERBOSE("thread starts execution");

  // Open Queue to receive requests
  ipc_name_gen(mq_name_req, sizeof(mq_name_req), MQ_IPMB_REQ, bus_num);
  mq = mq_open(mq_name_req, O_RDONLY);
  if (mq == MQ_DESC_INVALID) {
    return NULL;
  }

  ret = pal_get_bmc_ipmb_slave_addr(&addr, bus_num);
  if(ret < 0) {
    return NULL;
  }
#ifdef DEBUG
  syslog(LOG_WARNING, "%s ADDR=%x BUS_ID=%x\n", __func__, addr, bus_num);
#endif

  // Open the i2c bus for sending response
  fd = i2c_cdev_slave_open(bus_num, addr<<1, 0);
  if (fd < 0) {
    mq_close(mq);
    return NULL;
  }
  REQ_VERBOSE("bic opened successfully, fd=%d", fd);

  // Loop to process incoming requests
  while (1) {
    if ((rlen = mq_receive(mq, (char *)rxbuf, IPMB_PKT_MAX_SIZE, NULL)) <= 0) {
      sleep(1);
      continue;
    }
    REQ_VERBOSE("received %d bytes from message queue", rlen);

    if (ipmbd_config.bic_update_enabled) {
      continue;
    }

    pal_ipmb_processing(bus_num, rxbuf, rlen);

#ifdef DEBUG
    syslog(LOG_WARNING, "Received Request of %d bytes\n", rlen);
    for (i = 0; i < rlen; i++) {
      syslog(LOG_WARNING, "0x%X", rxbuf[i]);
    }
#endif

    // Create IPMI request from IPMB data
    p_ipmi_mn_req->payload_id = (unsigned char)ipmbd_config.payload_id;
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

     pal_ipmb_finished(bus_num, txbuf, tlen+IPMB_HDR_SIZE);
  }
}

// Thread to handle the incoming responses
static void*
ipmb_res_handler(void *args) {
  int bus_num = *((int*)args);
  uint8_t buf[IPMB_PKT_MAX_SIZE] = { 0 };
  uint8_t len = 0;
  mqd_t mq;
  ipmb_res_t *p_res;
  uint8_t index;
  char mq_name_res[NAME_MAX];

  RES_VERBOSE("thread starts execution");

  // Open the message queue
  ipc_name_gen(mq_name_res, sizeof(mq_name_res), MQ_IPMB_RES, bus_num);
  mq = mq_open(mq_name_res, O_RDONLY);
  if (mq == MQ_DESC_INVALID) {
    OBMC_ERROR(errno, "failed to open message queue %s", mq_name_res);
    return NULL;
  }

  // Loop to wait for incomng response messages
  while (1) {
    if ((len = mq_receive(mq, (char *)buf, IPMB_PKT_MAX_SIZE, NULL)) <= 0) {
      sleep(1);
      continue;
    }
    RES_VERBOSE("received %d bytes from message queue", len);

    p_res = (ipmb_res_t *) buf;

    // Check the seq# of response
    index = p_res->seq_lun >> LUN_OFFSET;

    if (seq_put(index, buf, len)) {
      // Either the IPMB packet is corrupted or arrived late after client exits
      OBMC_WARN("%s: WRONG packet received with seq #%d\n",
                IPMBD_RES_THREAD, index);
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
  i2c_mslave_t *bmc_slave;
  mqd_t mq_req = MQ_DESC_INVALID;
  mqd_t mq_res = MQ_DESC_INVALID;
  struct timespec req = {
    .tv_sec = 0,
    .tv_nsec = 10000000, //10mSec
  };
  char mq_name_req[NAME_MAX], mq_name_res[NAME_MAX];
  int bus_num = *((int*)args);
  uint16_t addr=0;
  int ret=0;

  RX_VERBOSE("thread starts execution");

  ret = pal_get_bmc_ipmb_slave_addr(&addr, bus_num);
  if (ret < 0) {
    return NULL;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "%s ADDR=%x BUS_ID=%x\n", __func__, addr, ipmbd_config.bus_id);
#endif
  // Open the i2c bus as a slave
  bmc_slave = i2c_mslave_open(bus_num, addr);
  if (bmc_slave == NULL) {
    OBMC_ERROR(errno, "%s: failed to open bmc as slave",
               IPMBD_RX_THREAD);
    goto cleanup;
  }
  RX_VERBOSE("opened bmc i2c-%d master as slave successfully",
             bus_num);

  // Open the message queues for post processing
  ipc_name_gen(mq_name_req, sizeof(mq_name_req), MQ_IPMB_REQ, bus_num);
  mq_req = mq_open(mq_name_req, O_WRONLY);
  if (mq_req == MQ_DESC_INVALID) {
    OBMC_ERROR(errno, "%s: failed to open message queue %s",
               IPMBD_RX_THREAD, mq_name_req);
    goto cleanup;
  }
  RX_VERBOSE("message queue %s opened", mq_name_req);

  ipc_name_gen(mq_name_res, sizeof(mq_name_res), MQ_IPMB_RES, bus_num);
  mq_res = mq_open(mq_name_res, O_WRONLY);
  if (mq_res == MQ_DESC_INVALID) {
    OBMC_ERROR(errno, "%s: failed to open message queue %s",
               IPMBD_RX_THREAD, mq_name_res);
    goto cleanup;
  }
  RX_VERBOSE("message queue %s opened", mq_name_res);

  // Loop that retrieves messages
  while (1) {
    int ret;
    ipmb_req_t *p_req;
    uint8_t len, tlun, fbyte;
    uint8_t buf[IPMB_PKT_MAX_SIZE], tbuf[IPMB_PKT_MAX_SIZE];

    // Read messages from i2c driver
    ret = i2c_mslave_read(bmc_slave, buf, sizeof(buf));
    if (ret <= 0) {
      i2c_mslave_poll(bmc_slave, 10);
      continue;
    }
    len = (uint8_t)ret;
    RX_VERBOSE("read %u bytes from ipmb bus %d", len, bus_num);

    // TODO: HACK: Due to i2cdriver issues, we are seeing two different type of packet corruptions
    // 1. The firstbyte(BMC's slave address) byte is same as second byte
    //    Workaround: Replace the first byte with correct slave address
    // 2. The missing slave address as first byte
    //    Workaround: move the buffer by one byte and add the correct slave address
    // Verify the IPMB hdr cksum: first two bytes are hdr and 3-rd byte cksum

    if (len < IPMB_PKT_MIN_SIZE) {
      OBMC_WARN("%s: IPMB Packet invalid size %d", IPMBD_RX_THREAD, len);
      continue;
    }

    if (buf[2] != calc_cksum(buf, 2)) {
      //handle wrong slave address
      if (buf[0] != addr<<1) {
        // Store the first byte
        fbyte = buf[0];
        // Update the first byte with correct slave address
        buf[0] = addr<<1;
        // Check again if the cksum passes
        if (buf[2] != calc_cksum(buf,2)) {
          //handle missing slave address
          // restore the first byte
          buf[0] = fbyte;
          //copy the buffer to temporary
          memcpy(tbuf, buf, len);
          // correct the slave address
          buf[0] = addr<<1;
          // copy back from temp buffer
          memcpy(&buf[1], tbuf, len);
          // increase length as we added slave address byte
          len++;
          // Check if the above hacks corrected the header
          if (buf[2] != calc_cksum(buf,2)) {
            OBMC_WARN("%s: IPMB Header cksum error after fixup",
                      IPMBD_RX_THREAD);
            continue;
          }
        }
      } else {
          OBMC_WARN("%s: IPMB Header cksum does not match", IPMBD_RX_THREAD);
          continue;
      }
    }

    // Verify the IPMB data cksum: data starts from 4-th byte
    if (buf[len-1] != calc_cksum(&buf[3], len-4)) {
      OBMC_WARN("%s: IPMB Data cksum does not match\n", IPMBD_RX_THREAD);
      continue;
    }

    // Check if the messages is request or response
    // Even NetFn: Request, Odd NetFn: Response
    // Post message to approriate Queue for further processing
    p_req = (ipmb_req_t*)buf;
    tlun = p_req->netfn_lun >> LUN_OFFSET;
    if (tlun % 2) {
      RX_VERBOSE("sending packet to %s", mq_name_res);
      ret = mq_timedsend(mq_res, (char *)buf, len, 0, &req);
    } else {
      RX_VERBOSE("sending packet to %s", mq_name_req);
      ret = mq_timedsend(mq_req, (char *)buf, len, 0, &req);
    }
    if (ret != 0) {
      //syslog(LOG_WARNING, "mq_send failed for queue %d\n", tmq);
      msleep(10);
      continue;
    }
  }

cleanup:
  if (bmc_slave != NULL) {
    i2c_mslave_close(bmc_slave);
  }

  if (mq_req != MQ_DESC_INVALID) {
    mq_close(mq_req);
  }

  if (mq_res != MQ_DESC_INVALID) {
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
  uint16_t addr=0;

  // Allocate right sequence Number
  index = seq_get_new(response);
  if (index < 0) {
    *res_len = 0;
    return ;
  }

  ret = pal_get_bmc_ipmb_slave_addr(&addr, ipmbd_config.bus_id);
  if (ret < 0) {
    return ;
  }
#ifdef DEBUG
  syslog(LOG_WARNING, "%s ADDR=%x BUS_ID=%x\n", __func__, addr, ipmbd_config.bus_id);
#endif
  req->seq_lun = index << LUN_OFFSET;
  req->req_slave_addr = addr << 1;

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

  if (pal_ipmb_processing(ipmbd_config.bus_id, request, req_len)) {
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
    IPMBD_VERBOSE("No response for sequence number: %d\n", index);
    *res_len = 0;
  }

ipmb_handle_out:
  // Reply to user with data
  pthread_mutex_lock(&ipmb_seq_buf.seq_mutex);
  *res_len = ipmb_seq_buf.seq[index].len;

  ipmb_seq_buf.seq[index].in_use = false;
  ipmb_seq_buf.seq[index].p_buf = NULL;
  pthread_mutex_unlock(&ipmb_seq_buf.seq_mutex);

  pal_ipmb_finished(ipmbd_config.bus_id, request, *res_len);

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
  unsigned char res_len=0;

  SVC_VERBOSE("entering svc handler");
  if (ipc_recv_req(cli, req_buf, &req_len, TIMEOUT_IPMB)) {
    OBMC_ERROR(errno, "%s: ipc_recv_req() failed", IPMBD_SVC_THREAD);
    return -1;
  }

  if(ipmbd_config.bic_update_enabled) {
    if(!((req_buf[1] == 0xe0) &&
        (req_buf[5] == CMD_OEM_1S_ENABLE_BIC_UPDATE))) {
      return -1;
    }
  }

  ipmb_handle(svc->i2c_fd, req_buf,
              (unsigned short)req_len, res_buf, &res_len);

  if(ipc_send_resp(cli, res_buf, res_len) != 0) {
    OBMC_ERROR(errno, "%s: ipc_send_resp() failed", IPMBD_SVC_THREAD);
    return -1;
  }
  return 0;
}


// Thread to receive the IPMB lib messages from various apps
static int
start_ipmb_lib_handler(int bus_num) {
  char sock_path[64];
  struct ipmb_svc_cookie *svc = calloc(1, sizeof(*svc));
  if (!svc) {
    OBMC_ERROR(errno, "failed to allocate svc cookie");
    return -1;
  }

  // Open the i2c bus for sending request
  svc->i2c_fd = i2c_cdev_slave_open(bus_num, BRIDGE_SLAVE_ADDR, 0);
  if (svc->i2c_fd < 0) {
    free(svc);
    return -1;
  }
  IPMBD_VERBOSE("bic opened successfully, fd=%d", svc->i2c_fd);

  ipc_name_gen(sock_path, sizeof(sock_path), SOCK_PATH_IPMB, bus_num);
  if (ipc_start_svc(sock_path, conn_handler, SEQ_NUM_MAX, svc, NULL)) {
    OBMC_ERROR(errno, "failed to start svc thread");
    free(svc);
    return -1;
  }

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
    {"-u|--enable-bic-update", "enable/allow bic update"},
    {NULL, NULL},
  };

  printf("Usage: %s [options] <bus-id> <payload-id>\n", prog_name);
  for (i = 0; options[i].opt != NULL; i++) {
    printf("    %-24s - %s\n", options[i].opt, options[i].desc);
  }
}

static int
parse_cmdline_args(int argc, char* const argv[])
{
  struct option long_opts[] = {
    {"help",              no_argument, NULL, 'h'},
    {"verbose",           no_argument, NULL, 'v'},
    {"enable-bic-update", no_argument, NULL, 'u'},
    {NULL,               0,           NULL, 0},
  };

  while (1) {
    int opt_index = 0;
    int ret = getopt_long(argc, argv, "hvu", long_opts, &opt_index);
    if (ret == -1)
      break; /* end of arguments */

    switch (ret) {
    case 'h':
      dump_usage(argv[0]);
      exit(0);

    case 'v':
      ipmbd_config.verbose_enabled = true;
      break;

    case 'u':
      ipmbd_config.bic_update_enabled = true;
      break;

    default:
      return -1;
    }
  } /* while */

  if ((optind + 1) >= argc) {
    fprintf(stderr, "Error: <bus-id> and/or <payload-id> is missing!\n\n");
    dump_usage(argv[0]);
    return -1;
  }
  ipmbd_config.bus_id = (int)strtoul(argv[optind], NULL, 0);
  ipmbd_config.payload_id = (int)strtoul(argv[optind + 1], NULL, 0);
  ipmbd_name_init(ipmbd_config.bus_id);

  return 0;
}

int
main(int argc, char * const argv[]) {
  int i, rc = 0;
  mqd_t mqd_req = MQ_DESC_INVALID;
  mqd_t mqd_res = MQ_DESC_INVALID;
  struct mq_attr attr = MQ_DFT_ATTR_INITIALIZER;
  char mq_name_req[NAME_MAX], mq_name_res[NAME_MAX];
  struct {
    const char *name;
    void* (*handler)(void *args);
    bool initialized;
    pthread_t tid;
  } ipmb_threads[3] = {
    {
      .name = IPMBD_RX_THREAD,
      .handler = ipmb_rx_handler,
      .initialized = false,
    },
    {
      .name = IPMBD_REQ_THREAD,
      .handler = ipmb_req_handler,
      .initialized = false,
    },
    {
      .name = IPMBD_RES_THREAD,
      .handler = ipmb_res_handler,
      .initialized = false,
    },
  };

  /*
   * Parse command line arguments.
   */
  if (parse_cmdline_args(argc, argv) != 0) {
    return -1;
  }

  /*
   * Initializing logging facility.
   */
  rc = obmc_log_init(ipmbd_name, LOG_INFO, 0);
  if (rc != 0) {
    fprintf(stderr, "%s: failed to initialize logger: %s\n",
            ipmbd_name, strerror(errno));
    return -1;
  }
  rc = obmc_log_set_syslog(0, LOG_DAEMON);
  if (rc != 0) {
    fprintf(stderr, "%s: failed to setup syslog: %s\n",
            ipmbd_name, strerror(errno));
    return -1;
  }
  obmc_log_unset_std_stream();
  if (ipmbd_config.verbose_enabled) {
    obmc_log_set_prio(LOG_DEBUG); /* ignore errors */
  }

  OBMC_INFO("%s started: bus#:%d, payload#:%d",
            ipmbd_name, ipmbd_config.bus_id, ipmbd_config.payload_id);

  /*
   * Initialize message queues.
   */
  ipc_name_gen(mq_name_req, sizeof(mq_name_req),
               MQ_IPMB_REQ, ipmbd_config.bus_id);
  mq_unlink(mq_name_req);
  mqd_req = mq_open(mq_name_req, MQ_DFT_FLAGS, MQ_DFT_MODES, &attr);
  if (mqd_req == MQ_DESC_INVALID) {
    rc = errno;
    OBMC_ERROR(rc, "failed to open message queue %s", mq_name_req);
    goto cleanup;
  }
  IPMBD_VERBOSE("message queue %s created", mq_name_req);

  ipc_name_gen(mq_name_res, sizeof(mq_name_res),
               MQ_IPMB_RES, ipmbd_config.bus_id);
  mq_unlink(mq_name_res);
  mqd_res = mq_open(mq_name_res, MQ_DFT_FLAGS, MQ_DFT_MODES, &attr);
  if (mqd_res == MQ_DESC_INVALID) {
    rc = errno;
    OBMC_ERROR(rc, "failed to open message queue %s", mq_name_res);
    goto cleanup;
  }
  IPMBD_VERBOSE("message queue %s created", mq_name_res);

  ipmb_seq_buf_init();
  IPMBD_VERBOSE("sequence buffer initialized");

  for (i = 0; i < ARRAY_SIZE(ipmb_threads); i++) {
    IPMBD_VERBOSE("creating thread %s", ipmb_threads[i].name);
    rc = pthread_create(&ipmb_threads[i].tid, NULL,
                        ipmb_threads[i].handler, &ipmbd_config.bus_id);
    if (rc != 0) {
      OBMC_ERROR(rc, "ipmbd: failed to create %s thread: %s\n",
                 ipmb_threads[i].name, strerror(rc));
      goto cleanup;
    }
    ipmb_threads[i].initialized = true;
  }

  // Create thread to receive ipmb library requests from apps
  if (start_ipmb_lib_handler(ipmbd_config.bus_id) < 0) {
    rc = errno;
    goto cleanup;
  }
  IPMBD_VERBOSE("ipmb lib handler started successfully");

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

  return rc;
}
