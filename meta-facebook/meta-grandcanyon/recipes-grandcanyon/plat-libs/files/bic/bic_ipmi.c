/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "bic_xfer.h"

#define MAX_VER_STR_LEN 80


int
bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = BIC_STATUS_FAILURE;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the component for which firmware is requested
  tbuf[3] = FW_BIC;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, tlen, rbuf, &rlen);
  // rlen should be greater than or equal to 4 (IANA + Data1 +...+ DataN)
  if ( ret < 0 || rlen < 4 ) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d, comp: %d\n", __func__, ret, rlen, comp);
    ret = BIC_STATUS_FAILURE;
  } else {
    //Ignore IANA ID
    memcpy(ver, &rbuf[3], rlen - 3);
  }

  return ret;
}

int
bic_me_recovery(uint8_t command) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry <= MAX_RETRY) {
    tbuf[0] = 0xB8;
    tbuf[1] = 0xDF;
    tbuf[2] = 0x57;
    tbuf[3] = 0x01;
    tbuf[4] = 0x00;
    tbuf[5] = command;
    tlen = 6;

    ret = bic_me_xmit(tbuf, tlen, rbuf, &rlen);
    if (ret != 0) {
      retry++;
      sleep(1);
      continue;
    }
    else {
      break;
    }
  }
  if (retry == MAX_RETRY + 1) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restart using Recovery Firmware failed..., retried: %d", __func__,  retry);
    return -1;
  }

  sleep(10);
  retry = 0;
  memset(&tbuf, 0, sizeof(tbuf));
  memset(&rbuf, 0, sizeof(rbuf));
  /*
      0x6 0x4: Get Self-Test Results
    Byte 1 - Completion Code
    Byte 2
      = 55h - No error. All Self-Tests Passed.
      = 81h - Firmware entered Recovery bootloader mode
    Byte 3 For byte 2 = 55h, 56h, FFh:
      =00h
      =02h - recovery mode entered by IPMI command "Force ME Recovery"
  */
  //Using ME self-test result to check if the ME Recovery Command Success or not
  while (retry <= MAX_RETRY) {
    tbuf[0] = 0x18;
    tbuf[1] = 0x04;
    tlen = 2;
    ret = bic_me_xmit(tbuf, tlen, rbuf, &rlen);
    if (ret != 0) {
      retry++;
      sleep(1);
      continue;
    }

    //if Get Self-Test Results is 0x55 0x00, means No error. All Self-Tests Passed.
    //if Get Self-Test Results is 0x81 0x02, means Firmware entered Recovery bootloader mode
    if ((command == RECOVERY_MODE) && (rbuf[1] == 0x81) && (rbuf[2] == 0x02)) {
      return 0;
    } else if ((command == RESTORE_FACTORY_DEFAULT) && (rbuf[1] == 0x55) && (rbuf[2] == 0x00)) {
      return 0;
    } else {
      return -1;
    }
  }
  if (retry == MAX_RETRY + 1) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restore Factory Default failed..., retried: %d", __func__,  retry);
    return -1;
  }

  return 0;
}

// Custom Command for getting vr version/device id
int
bic_get_vr_device_id(uint8_t *rbuf, uint8_t *rlen, uint8_t bus, uint8_t addr) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x07; //read back 7 bytes
  tbuf[3] = 0xAD; //get device id command
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
  } else {
    *rlen = rbuf[0]; //read cnt
    memmove(rbuf, &rbuf[1], *rlen);
  }

  return ret;
}

//Refer "AN001-XDPE122xx-V3.0_XDPE122xx Programming Guide" 9
int
bic_get_ifx_vr_remaining_writes(uint8_t bus, uint8_t addr, uint8_t *writes) {
// The data residing in bit11~bit6 is the number of the remaining writes. 
#define REMAINING_TIMES(x) (((x[1] << 8) + x[0]) & 0xFC0) >> 6
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = VR_PAGE;
  tbuf[4] = VR_PAGE50;
  tlen = 5;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[2] = 0x02; //read cnt
  tbuf[3] = 0x82;
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Couldn't get data from 0x%02X", __func__, tbuf[3]);
    goto error_exit;
  }

  *writes = REMAINING_TIMES(rbuf);

error_exit:
  return ret;
}

// Refer "isl69259_ds_Aug_21_2019" 10.71 & 10.73
int
bic_get_isl_vr_remaining_writes(uint8_t bus, uint8_t addr, uint8_t *writes) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = CMD_ISL_VR_DMAADDR; //command code
  tbuf[4] = 0xC2; //data1
  tbuf[5] = 0x00; //data2
  tlen = 6;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to send command and data...", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x04; //read cnt
  tbuf[3] = CMD_ISL_VR_DMAFIX; //command code
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to read NVM slots...", __func__);
    goto error_exit;
  }

  *writes = rbuf[0];

error_exit:
  return ret;
}

int
bic_switch_mux_for_bios_spi(uint8_t mux) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x42; //slave addr
  tbuf[2] = 0x01; //read 1 byte
  tbuf[3] = 0x00; //register offset
  tbuf[4] = mux;  //mux setting

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  if (ret < 0) {
    return -1;
  }

  return 0;
}

int
bic_get_vr_ver(uint8_t bus, uint8_t addr, char *key, char *ver_str) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rlen = 0;
  uint8_t remaining_writes = 0;
  char path[MAX_PATH_LEN] = {0};
  int fd = 0;
  int ret = 0, rc = 0;

  ret = bic_get_vr_device_id(rbuf, &rlen, bus, addr);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
    return ret;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;

  //The length of GET_DEVICE_ID is different.
  if (rlen == 2) {
    //Infineon
    //to avoid sensord changing the page of VRs, so use the LOCK file
    //to stop monitoring sensors of VRs
    fd = open(SERVER_SENSOR_LOCK, O_CREAT | O_RDWR, 0666);
    rc = flock(fd, LOCK_EX | LOCK_NB);
    if (rc != 0) {
      if (EWOULDBLOCK == errno) {
        return -1;
      }
    }

    //get the remaining writes of VRs
    ret = bic_get_ifx_vr_remaining_writes(bus, addr, &remaining_writes);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr remaining writes. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }

    //get the CRC32 of the VR
    //Refer "AN001-XDPE122xx-V3.0_XDPE122xx Programming Guide" 8.2.1
    tbuf[2] = 0x00; //read cnt
    tbuf[3] = CMD_INF_VR_SET_PAGE; //command code
    tbuf[4] = 0x62;
    tlen = 5;
    
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }
    
    tbuf[2] = 0x2; //read cnt
    tbuf[3] = CMD_INF_VR_READ_DATA_LOW; //command code
    tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }
    
    tbuf[2] = 0x2; //read cnt
    tbuf[3] = CMD_INF_VR_READ_DATA_HIGH; //command code
    tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, &rbuf[2], &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }
    snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X, Remaining Writes: %d", rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
    kv_set(key, ver_str, 0, 0);
    
  error_exit:
    rc = flock(fd, LOCK_UN);
    if (rc == -1) {
      syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
      close(fd);
      return rc;
    }
    close(fd);
    remove(path);
    return ret;
    
  } else if (rlen > 4) {
    //TI
    tbuf[2] = 0x02; //read cnt
    tbuf[3] = CMD_TI_VR_NVM_CHECKSUM; //command code
    tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      return ret;
    }
    
    snprintf(ver_str, MAX_VALUE_LEN, "Texas Instruments %02X%02X", rbuf[1], rbuf[0]);
    kv_set(key, ver_str, 0, 0);
  } else {
    //ISL
    //get the reamaining write
    // Refer "isl69259_ds_Aug_21_2019" 10.71 & 10.73
    ret = bic_get_isl_vr_remaining_writes(bus, addr, &remaining_writes);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr remaining writes. ret=%d", __func__,__LINE__, ret);
      goto error_exit;
    }

    //get the CRC32 of the VR
    tbuf[2] = 0x00; //read cnt
    tbuf[3] = CMD_ISL_VR_DMAADDR; //command code
    tbuf[4] = 0x3F; //reg
    tbuf[5] = 0x00; //dummy data
    tlen = 6;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      return ret;
    }

    tbuf[2] = 0x04; //read cnt
    tbuf[3] = CMD_ISL_VR_DMAFIX; //command code
    tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      return ret;
    }    
    snprintf(ver_str, MAX_VALUE_LEN, "Renesas %02X%02X%02X%02X, Remaining Writes: %d", rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
    kv_set(key, ver_str, 0, 0);
  }

  return ret;
}

int
bic_get_vr_ver_cache(uint8_t bus, uint8_t addr, char *ver_str) {
  char key[MAX_KEY_LEN] = {0};
  char tmp_str[MAX_VALUE_LEN] = {0};

  memset(key, 0, MAX_KEY_LEN);
  memset(tmp_str, 0, MAX_VALUE_LEN);
  snprintf(key, sizeof(key), "vr_%02xh_crc", addr);
  if (kv_get(key, tmp_str, NULL, 0) != 0) {
    if (bic_get_vr_ver(bus, addr, key, tmp_str) != 0) {
      return -1;
    }
  }
  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN - 1)) {
    return -1;
  }

  return 0;  
}
