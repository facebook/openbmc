#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include "nm.h"

int
cmd_NM_pmbus_read_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *rbuf) {
  uint8_t tbuf[64] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  ipmb_req_t *req;
 
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = info.nm_addr;
  req->netfn_lun = NETFN_NM_REQ << 2;
  req->cmd = CMD_NM_SEND_RAW_PMBUS;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  req->req_slave_addr = info.bmc_addr << 1;

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = SMBUS_EXTEND_READ_WORD;
  req->data[4] = dev_addr;
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x01;
  req->data[8] = 0x02;
  req->data[9] = info.nm_cmd;
  tlen = 10 + MIN_IPMB_REQ_LEN;


  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, &rlen);
  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_hsc_value: Zero bytes received\n");
  #endif  
    return -1;
  }

  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_hsc_value: rbuf[10]=%x rbuf[11]=%x\n", rbuf[10], rbuf[11]);
  #endif  
  return ret;
}

int
cmd_NM_pmbus_write_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *data){
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t data_len = 2;
  int ret = 0;
  ipmb_req_t *req;
 
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = info.nm_addr;
  req->netfn_lun = NETFN_NM_REQ << 2;
  req->cmd = CMD_NM_SEND_RAW_PMBUS;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  req->req_slave_addr = info.bmc_addr << 1;

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = SMBUS_EXTEND_WRITE_WORD;
  req->data[4] = dev_addr;
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x03;
  req->data[8] = 0x00;
  req->data[9] = info.nm_cmd;
  memcpy(req->data+10, data, data_len);

  #ifdef DEBUG  
    syslog(LOG_DEBUG, "Tx data[0]=%x data[1]=%x\n", req->data[10], data[11]);
  #endif  

  tlen = MIN_IPMB_REQ_LEN + 10 + data_len;

  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, &rlen);
  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "write_hsc_value: Zero bytes received\n");
  #endif
    return -1; 
  }

  return ret;
}


int
cmd_NM_sensor_reading(NM_RW_INFO info, uint8_t snr_num, uint8_t* rbuf, uint8_t* rlen) {
  uint8_t tbuf[64] = {0x00};
  uint8_t tlen = 0;
  ipmb_req_t *req;
  int ret = 0;

  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = info.nm_addr; //ME's Slave Address 0x2C
  req->netfn_lun = NETFN_SENSOR_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  req->req_slave_addr = info.bmc_addr << 1;
  req->seq_lun = 0x00;
//req->cmd = CMD_SENSOR_GET_SENSOR_READING;
  req->cmd = info.nm_cmd;
  req->data[0] = snr_num;
  tlen = 7;

  // Invoke IPMB library handler
  ret = lib_ipmb_handle(info.bus, tbuf, tlen+1, rbuf, rlen);
  if (ret != 0) {
    return ret;
  }

  if (rlen == 0) {
  //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "read NM sensor=%x: Zero bytes received\n", snr_num);
    return -1;
#endif
  } else {
    if (rbuf[6] == 0) {
      if (rbuf[8] & 0x20) {
        return -1;
      }
    }
  }
  return ret;
}

