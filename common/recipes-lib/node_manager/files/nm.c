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
#include "nm.h"

static void 
set_NM_head(NM_RW_INFO* info, uint8_t netfn, ipmb_req_t *req, uint8_t ipmi_cmd) {

  req->res_slave_addr = info->nm_addr;
  req->netfn_lun = netfn << 2;
  req->cmd = ipmi_cmd;
  req->seq_lun = 0x00;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;
  req->req_slave_addr = info->bmc_addr << 1;
}

static int
me_ipmb_process(NM_RW_INFO* info, uint8_t ipmi_cmd, uint8_t netfn, 
              uint8_t *txbuf, uint8_t txlen, 
              uint8_t *rxbuf, uint8_t *rxlen ) {
  uint8_t rdata[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t wdata[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  ipmb_res_t *res;

  
  res = (ipmb_res_t*) rdata;
  req = (ipmb_req_t*) wdata;

  set_NM_head(info, netfn, req, ipmi_cmd);

  if (txlen) {
    memcpy(req->data, txbuf, txlen);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + txlen;

  // Invoke IPMB library handler
  lib_ipmb_handle(info->bus, wdata, tlen, rdata, &rlen);

  if (rlen == 0) {
    syslog(LOG_DEBUG, "%s: Zero bytes received\n", __func__);
    return -1;
  }

  // Handle IPMB response
  if (res->cc) {
    syslog(LOG_ERR, "%s: Completion Code: 0x%X\n", __func__, res->cc);
    return -1;
  }

  // copy the received data back to caller
  *rxlen = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;
  memcpy(rxbuf, res->data, *rxlen);

#ifdef DEBUG
{
  int i;
  for(i=0; i< *rxlen; i++)
  syslog(LOG_WARNING, "rbuf[%d]=%x\n", i, rxbuf[i]);
}
#endif

  return 0;
}

// Get Device ID
int
cmd_NM_get_dev_id(NM_RW_INFO* info, ipmi_dev_id_t *dev_id) {
  uint8_t ipmi_cmd = CMD_APP_GET_DEVICE_ID;
  uint8_t netfn = NETFN_APP_REQ;
  uint8_t rlen;
  int ret;

  ret = me_ipmb_process(info, ipmi_cmd, netfn, NULL, 0, (uint8_t *)dev_id, &rlen);
  
  return ret;
}

// Get Self Test Result
int
cmd_NM_get_self_test_result(NM_RW_INFO* info, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t ipmi_cmd = CMD_APP_GET_SELFTEST_RESULTS;
  uint8_t netfn = NETFN_APP_REQ;
  int ret;

  ret = me_ipmb_process(info, ipmi_cmd, netfn, NULL, 0, rbuf, rlen);
  if(ret !=0 || *rlen < 2) {
    return -1;
  } 
  return 0;
}

int
cmd_NM_send_pmbus_raw(NM_RW_INFO info, uint8_t *rbuf, uint8_t *wbuf, uint8_t wr_len) {
  uint8_t tbuf[64] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  ipmb_req_t *req;
 
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  set_NM_head(&info, NETFN_NM_REQ, req, CMD_NM_SEND_RAW_PMBUS);
  memcpy(req->data, wbuf, wr_len);

  tlen = MIN_IPMB_REQ_LEN + wr_len;

  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, &rlen);


  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_value: Zero bytes received\n");
  #endif  
    return -1;
  } else {
  #ifdef DEBUG
    int i=0;
    for(i=0; i<rlen; i++) {
      syslog(LOG_WARNING, "read_value: rbuf[%d]=%x\n", rbuf[10+i]);
    }
  #endif  
  }

  return ret;
}

int
cmd_NM_pmbus_standard_read_word(NM_RW_INFO info, uint8_t* buf, uint8_t *rdata) {
  uint8_t tbuf[64] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  ipmb_req_t *req;
  NM_PMBUS_STANDAR_DEV dev;
 
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  set_NM_head(&info, NETFN_NM_REQ, req, CMD_NM_SEND_RAW_PMBUS);

  dev.psu_cmd = buf[0];
  dev.psu_addr = buf[1];  

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = SMBUS_STANDARD_READ_WORD;
  req->data[4] = dev.psu_addr;
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x01;
  req->data[8] = 0x02;
  req->data[9] = dev.psu_cmd;
  tlen = 10 + MIN_IPMB_REQ_LEN;


  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rdata, &rlen);
  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_value: Zero bytes received\n");
  #endif  
    return -1;
  }

  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_value: rdata[10]=%x rdata[11]=%x\n", rdata[10], rdata[11]);
  #endif  
  return ret;
}

int
cmd_NM_pmbus_standard_write_word(NM_RW_INFO info, uint8_t* buf, uint8_t *wdata) {
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t data_len = 2;
  int ret = 0;
  ipmb_req_t *req;
  NM_PMBUS_STANDAR_DEV dev;


#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  set_NM_head(&info, NETFN_NM_REQ, req, CMD_NM_SEND_RAW_PMBUS);

  dev.psu_cmd = buf[0];
  dev.psu_addr = buf[1];  

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = SMBUS_STANDARD_WRITE_WORD;
  req->data[4] = dev.psu_addr;
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x03;
  req->data[8] = 0x00;
  req->data[9] = dev.psu_cmd;
  memcpy(req->data+10, wdata, data_len);

  #ifdef DEBUG  
    syslog(LOG_DEBUG, "Tx wdata[0]=%x wdata[1]=%x\n", req->data[10], data[11]);
  #endif  

  tlen = MIN_IPMB_REQ_LEN + 10 + data_len;

  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, &rlen);
  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "write_value: Zero bytes received\n");
  #endif
    return -1; 
  }

  return ret;
}

int
cmd_NM_pmbus_extend_read_word(NM_RW_INFO info, uint8_t* buf, uint8_t *rdata) {
  uint8_t tbuf[64] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  ipmb_req_t *req;
  NM_PMBUS_EXTEND_DEV dev;
 
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  set_NM_head(&info, NETFN_NM_REQ, req, CMD_NM_SEND_RAW_PMBUS);

  dev.psu_cmd = buf[0];
  dev.psu_addr = buf[1];
  dev.mux_addr = buf[2];
  dev.mux_ch = buf[3];
  dev.sensor_bus = buf[4];

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = SMBUS_EXTENDED_READ_WORD;
  req->data[4] = dev.sensor_bus; 
  req->data[5] = dev.psu_addr;  
  req->data[6] = dev.mux_addr;
  req->data[7] = dev.mux_ch;
  req->data[8] = EXTENDED_MUX_ENABLE; 
  req->data[9] = TRANS_PROTOCOL_PMBUS;
  req->data[10] = 1;
  req->data[11] = 2;
  req->data[12] = dev.psu_cmd;

  tlen = 13 + MIN_IPMB_REQ_LEN;

  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rdata, &rlen);
  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_value: Zero bytes received\n");
  #endif  
    return -1;
  }

  #ifdef DEBUG  
    syslog(LOG_WARNING, "read_value: rdata[10]=%x rdata[11]=%x\n", rdata[10], rdata[11]);
  #endif  
  return ret;
}

int
cmd_NM_pmbus_extend_write_word(NM_RW_INFO info, uint8_t* buf,  uint8_t *wdata) {
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  ipmb_req_t *req;
  NM_PMBUS_EXTEND_DEV dev;
 
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s\n", __func__);
#endif
   
  req = (ipmb_req_t*)tbuf;
  set_NM_head(&info, NETFN_NM_REQ, req, CMD_NM_SEND_RAW_PMBUS);

  dev.psu_cmd = buf[0];
  dev.psu_addr = buf[1];
  dev.mux_addr = buf[2];
  dev.mux_ch = buf[3];
  dev.sensor_bus = buf[4];

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = SMBUS_EXTENDED_WRITE_WORD;
  req->data[4] = dev.sensor_bus; 
  req->data[5] = dev.psu_addr;  
  req->data[6] = dev.mux_addr;
  req->data[7] = dev.mux_ch;
  req->data[8] = EXTENDED_MUX_ENABLE; 
  req->data[9] = TRANS_PROTOCOL_PMBUS;
  req->data[10] = 3;
  req->data[11] = 0;
  req->data[12] = dev.psu_cmd;
  req->data[13] = wdata[0];
  req->data[14] = wdata[1];

  tlen = MIN_IPMB_REQ_LEN + 15;
  #ifdef DEBUG  
    syslog(LOG_DEBUG, "Tx wdata[0]=%x wdata[1]=%x\n", wdata[0], wdata[1]);
  #endif  

  // Invoke IPMB library handler
  lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, &rlen);
  if (rlen == 0) {
  #ifdef DEBUG  
    syslog(LOG_WARNING, "write_value: Zero bytes received\n");
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
  set_NM_head(&info, NETFN_SENSOR_REQ, req, CMD_SENSOR_GET_SENSOR_READING);

  req->data[0] = snr_num;
  tlen = 1+MIN_IPMB_REQ_LEN;

  // Invoke IPMB library handler
  ret = lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, rlen);
  if (ret != 0) {
    return ret;
  }

  if (rlen == 0) {
  //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "read NM sensor=%x: Zero bytes received\n", snr_num);
#endif
    return -1;
  } else {
    if (rbuf[6] == 0) {
      if (rbuf[8] & 0x20) {
        return -1;
      }
    } else {
      return -1;
    }
  }
  return ret;
}

int
cmd_NM_cpu_err_num_get(NM_RW_INFO info, bool is_caterr)
{
  int cpu_num = -1;
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[64] = {0x00};
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t tlen;
  uint8_t rlen;
  int ret=0;

  req = (ipmb_req_t*)tbuf;
  res = (ipmb_res_t*)rbuf;
  set_NM_head(&info, NETFN_NM_REQ, req, CMD_NM_SEND_RAW_PECI);

  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x30;
  req->data[4] = 0x05;
  req->data[5] = 0x05;
  req->data[6] = 0xa1;
  req->data[7] = 0x00;
  req->data[8] = 0x00;
  req->data[9] = 0x05;
  req->data[10] = 0x00;

  tlen = 11+MIN_IPMB_REQ_LEN; 
  ret = lib_ipmb_handle(info.bus, tbuf, tlen, rbuf, &rlen);
  if (ret != 0) {
    return ret;
  }

  if (rlen == 0) {
  //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "read NM sensor=%x: Zero bytes received\n", snr_num);
    return -1;
#endif
  } 

  if ( ( res->cc == 0 ) && ( res->data[3] == 0x40 ) ) {
    if( is_caterr ) {
      if(((res->data[7] & 0xE0) > 0) && ((res->data[7] & 0x1C) > 0))
        cpu_num = 2; //Both
      else if((res->data[7] & 0xE0) > 0)
        cpu_num = 1; //CPU1
      else if((res->data[7] & 0x1C) > 0)
        cpu_num = 0; // CPU0
    } else {
      if(((res->data[6] & 0xE0) > 0) && ((res->data[6] & 0x1C) > 0))
        cpu_num = 2; //Both
      else if((res->data[6] & 0xE0) > 0)
        cpu_num = 1; //CPU1
      else if((res->data[6] & 0x1C) > 0)
        cpu_num = 0; // CPU0
    }
  }
  return cpu_num;
}
