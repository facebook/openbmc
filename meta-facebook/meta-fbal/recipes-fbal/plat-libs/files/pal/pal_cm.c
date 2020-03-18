#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/ipmb.h>
#include "pal.h"

//#define DEBUG
CMC_FAN_INFO cm_fan_list[] = {
  {FAN_ID0, CM_SNR_FAN0_INLET_SPEED},
  {FAN_ID1, CM_SNR_FAN0_OUTLET_SPEED},
  {FAN_ID2, CM_SNR_FAN1_INLET_SPEED},
  {FAN_ID3, CM_SNR_FAN1_OUTLET_SPEED},
  {FAN_ID4, CM_SNR_FAN2_INLET_SPEED}, 
  {FAN_ID5, CM_SNR_FAN2_OUTLET_SPEED},
  {FAN_ID6, CM_SNR_FAN3_INLET_SPEED},
  {FAN_ID7, CM_SNR_FAN3_OUTLET_SPEED},
};


static int
cmc_ipmb_process(uint8_t ipmi_cmd, uint8_t netfn,
              uint8_t *txbuf, uint8_t txlen, 
              uint8_t *rxbuf, uint8_t *rxlen ) {
  int ret=0;
  uint16_t bmc_addr;
  

  ret = pal_get_bmc_ipmb_slave_addr(&bmc_addr, CM_I2C_BUS_NUMBER);
  if(ret != 0) {
    return ret;
  }

  ret = lib_ipmb_send_request(ipmi_cmd, netfn, 
                         txbuf, txlen, 
                         rxbuf, rxlen, 
                         CM_I2C_BUS_NUMBER, CM_I2C_SLAVE_ADDR, bmc_addr);

  if(ret != 0) {
    return ret;
  }

  return 0;
}

int
cmd_cmc_get_dev_id(ipmi_dev_id_t *dev_id) {
  uint8_t ipmi_cmd = CMD_APP_GET_DEVICE_ID;
  uint8_t netfn = NETFN_APP_REQ;
  uint8_t rlen;
  int ret;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, NULL, 0, (uint8_t *)dev_id, &rlen);
  return ret;
}

int
cmd_cmc_get_config_mode(uint8_t *mode) {
  uint8_t ipmi_cmd = CMD_CMC_GET_CONFIG_MODE;
  uint8_t netfn = NETFN_OEM_REQ;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, NULL, 0, rbuf, &rlen);
  *mode = rbuf[0];
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s mode=%d\n", __func__, *mode);
#endif
  return ret;
}

int
cmd_cmc_get_mb_position(uint8_t *position) {
  uint8_t ipmi_cmd = CMD_CMC_GET_MB_POSITION;
  uint8_t netfn = NETFN_OEM_Q_REQ;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, NULL, 0, rbuf, &rlen);
  *position = rbuf[0];
#ifdef DEBUG 
  syslog(LOG_DEBUG, "%s position=%d\n", __func__, *position);
#endif
  return ret;
}

int
cmd_cmc_get_sensor_value(uint8_t snr_num, uint8_t *rbuf, uint8_t* rlen) {
  uint8_t ipmi_cmd = CMD_CMC_OEM_GET_SENSOR_READING;
  uint8_t netfn = NETFN_OEM_Q_REQ;
  uint8_t tbuf[8] = {0x00};
  uint8_t tlen=0;
  int ret;

  tbuf[0] = snr_num; 
  tlen = 1;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, tbuf, tlen, rbuf, rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<*rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf=%d\n", __func__, *(rbuf+i));
  }
}  
#endif  
  return ret;
}

int
lib_cmc_get_fan_speed(uint8_t fan_num, uint16_t* speed) {
  int ret = 0;
  uint8_t rlen=0;
  uint8_t rbuf[16];
   
  ret = cmd_cmc_get_sensor_value(cm_fan_list[fan_num].sdr, rbuf, &rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf[%d]=%d\n", __func__, i, *(rbuf+i));
  }
}  
#endif  
  if(ret != 0) {
    return -1;
  }

  *speed = rbuf[0] | rbuf[1]<<8;
  return 0;
}


int
lib_cmd_set_fan_ctrl(uint8_t fan_mode, uint8_t* status) {
  uint8_t ipmi_cmd = CMD_CMC_SET_FAN_CONTROL_STATUS;
  uint8_t netfn = NETFN_OEM_Q_REQ;
  uint8_t tbuf[8] = {0x00};
  uint8_t tlen=0;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;

  tbuf[0] = fan_mode;
  tlen = 1;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, tbuf, tlen, rbuf, &rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf[%d]=%d\n", __func__, i, *(rbuf+i));
  }
}  
#endif  
  if(ret != 0) {
    return -1;
  }
  *status = rbuf[0];
  return 0;
}

int
lib_cmc_set_fan_pwm(uint8_t fan_num, uint8_t pwm) {
  uint8_t ipmi_cmd = CMD_CMC_SET_FAN_DUTY;
  uint8_t netfn = NETFN_OEM_Q_REQ;
  uint8_t tbuf[8] = {0x00};
  uint8_t tlen=0;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;

  tbuf[0] = pwm;
  tbuf[1] = fan_num; 
  tlen = 2;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, tbuf, tlen, rbuf, &rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf[%d]=%d\n", __func__, i, *(rbuf+i));
  }
}  
#endif  
  if(ret != 0) {
    return -1;
  }
  return 0;
}

int
lib_cmc_get_fan_pwm(uint8_t fan_num, uint8_t* pwm) {
  uint8_t ipmi_cmd = CMD_CMC_GET_FAN_DUTY;
  uint8_t netfn = NETFN_OEM_Q_REQ;
  uint8_t tbuf[8] = {0x00};
  uint8_t tlen=0;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;

  tbuf[0] = fan_num; 
  tlen = 1;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, tbuf, tlen, rbuf, &rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf[%d]=%d\n", __func__, i, *(rbuf+i));
  }
}  
#endif  
  if(ret != 0) {
    return -1;
  }

  *pwm = rbuf[1];
  return 0;
}

int lib_cmc_get_fan_id(uint8_t fan_sdr) {
  int fan_num=-1;

  switch(fan_sdr) {
    case PDB_EVENT_FAN0_PRESENT: 
      fan_num = FAN_ID0; 
      break;
    case PDB_EVENT_FAN1_PRESENT: 
      fan_num = FAN_ID1; 
      break;
    case PDB_EVENT_FAN2_PRESENT: 
      fan_num = FAN_ID2; 
      break;
    case PDB_EVENT_FAN3_PRESENT: 
      fan_num = FAN_ID3; 
      break;
  }

  return fan_num;
}

int
lib_cmc_power_cycle(void) {
  uint8_t ipmi_cmd = CMD_CMC_POWER_CYCLE;
  uint8_t netfn = NETFN_OEM_REQ;
  uint8_t tlen=0;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, NULL, tlen, rbuf, &rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf[%d]=%d\n", __func__, i, *(rbuf+i));
  }
}  
#endif  
  if(ret != 0) {
    return -1;
  }
  return 0;
}

int
lib_cmc_get_block_index(uint8_t fru) {
  uint8_t index;

  switch(fru) {
    case FRU_MB:
      index = COMP_BIOS;
      break;
    case FRU_BMC:
      index = COMP_BMC;
      break;
    case FRU_NIC0:
    case FRU_NIC1:
      index = COMP_NIC;
      break;
    case FRU_DBG:
      index = COMP_MCU;
      break;
    default :
      return -1;
  }

  return index;
}

int
lib_cmc_set_block_command_flag(uint8_t index, uint8_t flag) {
  uint8_t ipmi_cmd = CMD_CMC_OEM_SET_BLOCK_COMMON_FLAG;
  uint8_t netfn = NETFN_OEM_Q_REQ;
  uint8_t tbuf[8] = {0x00};
  uint8_t tlen=0;
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  int ret;
  
  tbuf[0] = CM_SET_FLAG_STATUS;
  tbuf[1] = index;
  tbuf[2] = flag;

  tlen = 3;

  ret = cmc_ipmb_process(ipmi_cmd, netfn, tbuf, tlen, rbuf, &rlen);
#ifdef DEBUG
{    
  int i;
  for(i=0; i<rlen; i++) {
    syslog(LOG_DEBUG, "%s rbuf[%d]=%d\n", __func__, i, *(rbuf+i));
  }
}  
#endif  
  if(ret != 0) {
    return -1;
  }
  return 0;
}

