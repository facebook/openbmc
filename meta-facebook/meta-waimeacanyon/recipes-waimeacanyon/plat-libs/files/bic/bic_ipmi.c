/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "bic_ipmi.h"
#include "bic_xfer.h"

#define SIZE_SYS_GUID 16

// OEM - Get Firmware Version
// Netfn: 0x38, Cmd: 0x0B
// It's an API and provided to fw-util
int
bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver) {
  uint8_t tbuf[4] = {0x00};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = BIC_STATUS_FAILURE;
  
  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = comp;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, tlen, rbuf, &rlen);

  // rlen should be greater than or equal to 4 (IANA + Data1 +...+ DataN)
  if ( ret < 0 || rlen < 4 ) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d, ", __func__, ret, rlen);
  } else {
    //Ignore IANA ID
    memcpy(ver, &rbuf[3], rlen-3);
    ret = BIC_STATUS_SUCCESS;
  }

  return ret;
}

// Update firmware for various components
int
bic_update_firmware(uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int retries = 3;
  int index = 3;
  int offset_size = sizeof(offset) / sizeof(tbuf[0]);

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the component for which firmware is requested
  tbuf[index++] = target;

  memcpy(&tbuf[index], (uint8_t *)&offset, offset_size);
  index += offset_size;

  tbuf[index++] = len & 0xFF;
  tbuf[index++] = (len >> 8) & 0xFF;

  memcpy(&tbuf[index], buf, len);

  tlen = len + index;

bic_send:
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen);
  if ((ret) && (retries--)) {
    sleep(1);
    printf("_update_fw: target %d, offset: %u, len: %d retrying..\
           \n",   target, offset, len);
    goto bic_send;
  }

  return ret;
}

int
bic_reset(uint8_t slot_id) {
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_COLD_RESET, NULL, 0, rbuf, &rlen);

  return ret;
}

// OEM - Get Post Code buffer
// Netfn: 0x38, Cmd: 0x12
int
bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen) {
  int ret = 0;
  uint8_t tbuf[3] = {0};
  uint8_t tlen = sizeof(tbuf);

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot get the postcode buffer from slot%d", __func__, slot_id);
  } else {
    *rlen -= 3;
    memmove(rbuf, &rbuf[3], *rlen);
  }

  return ret;
}

// APP - Get Self Test Results
// Netfn: 0x06, Cmd: 0x04
int
bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result) {
  uint8_t rlen = 0;

  return bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, (uint8_t *)self_test_result, &rlen);
}

// Storage - Get FRUID info
// Netfn: 0x0A, Cmd: 0x10
int
bic_get_fruid_info(uint8_t slot_id, uint8_t bic_fru_id, ipmi_fruid_info_t *info) {
  uint8_t rlen = 0;
  uint8_t fruid = 0;

  return bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fruid, 1, (uint8_t *) info, &rlen);
}

// Storage - Read FRU Data
// Netfn: 0x0A, Cmd: 0x11
int
bic_read_fru_data(uint8_t slot_id, uint8_t bic_fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen;

  tbuf[0] = bic_fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;
  tlen = 4;

  return bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, tlen, rbuf, rlen);
}

// Storage - Write FRU Data
// Netfn: 0x0A, Cmd: 0x12
int
bic_write_fru_data(uint8_t slot_id, uint8_t bic_fru_id, uint32_t offset, uint8_t count, uint8_t *buf) {
  int ret = 0;
  uint8_t tbuf[MAX_FRU_DATA_SIZE + 3] = {0};
  uint8_t rbuf[8] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = bic_fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  memcpy(&tbuf[3], buf, count);
  tlen = count + 3;

  ret = bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen);
  if (rbuf[0] != count) {
    syslog(LOG_WARNING, "%s() Failed to write fruid data. %d != %d \n", __func__, rbuf[0], count);
    return -1;
  }

  return ret;
}

// OEM_1S - Get Accuracy Sensor reading
// Netfn: 0x38, Cmd: 0x23
int
bic_get_accur_sensor(uint8_t slot_id, uint8_t sensor_num, ipmi_extend_sensor_reading_t *sensor) {
  uint8_t rlen = 0;
  int ret = 0;
  uint8_t tbuf[5] = {0, 0, 0, sensor_num, SNR_READ_CACHE};
  uint8_t rbuf[16] = {0};

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_ACCURACY_SENSOR_READING, tbuf, 5, rbuf, &rlen);
  if (ret != 0) {
    return ret;
  }

  if (rlen == 8) {
    // Byte 1 - 3 : IANA
    // Byte 4 - 7 : sensor raw data
    // Byte 8 : flags
    memcpy(sensor->iana_id, rbuf, 3);
    memcpy((uint8_t *)&sensor->value, rbuf+3, 4);
    sensor->flags = rbuf[7];
  } else {
    syslog(LOG_WARNING, "%s() invalid rlen=%d", __func__, rlen);
    return READING_NA;
  }

  return ret;
}

// APP - Get System GUID
// Netfn: 0x06, Cmd: 0x37
int
bic_get_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SYSTEM_GUID, NULL, 0, guid, &rlen);
  if (rlen != SIZE_SYS_GUID) {
    syslog(LOG_ERR, "bic_get_sys_guid: rlen:%d != %d", rlen, SIZE_SYS_GUID);
    return -1;
  }

  return ret;
}

// OEM - Set System GUID
// Netfn: 0x30, Cmd: 0xEF
int
bic_set_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};

  ret = bic_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_SET_SYSTEM_GUID, guid, SIZE_SYS_GUID, rbuf, &rlen);

  return ret;
}

int
bic_bypass_get_vr_device_id(uint8_t slot_id, uint8_t *devid, uint8_t *id_len, uint8_t bus, uint8_t addr) {
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = sizeof(rbuf);
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x07; // read back 7 bytes
  tbuf[3] = 0xAD; // get device id command
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
  } else {
    rlen = (*id_len > rbuf[0]) ? rbuf[0] : *id_len;
    *id_len = rbuf[0];
    memmove(devid, &rbuf[1], rlen);
  }

  return ret;
}

// OEM - BIC_CMD_OEM_GET_SET_GPIO
// Netfn: 0x38, Cmd: 0x41
int
bic_set_single_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t value) {
  uint8_t tbuf[6] = {0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;
  int ret = 0;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  tbuf[3] = 0x01;   // set
  tbuf[4] = gpio_num;
  tbuf[5] = value;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    syslog(LOG_WARNING, "%s() failed, ret:%d gpio_num:0x%02X  value:0x%02X", __func__, ret, gpio_num, value);
  }
  
  return ret;
}

int
bic_set_gpio_passthrough(uint8_t slot_id, uint8_t enable) {
  uint8_t tbuf[12] = {0x00, 0x00, 0x00, 0xb0, 0x24, 0x6e, 0x7e, 0x04, 0x00, 0x00, 0xc3, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = sizeof(tbuf);
  uint8_t rlen = 0;
  int ret = 0;

  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  if (enable == 0) {
    tbuf[10] = 0x00;   //disable
  }

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_SET_REGISTER, tbuf, tlen, rbuf, &rlen); 
  if (ret) {
    syslog(LOG_WARNING, "%s() failed, ret:%d enable:0x%02X", __func__, ret, enable);
  }
  
  return ret;
}

/*
    0x2E 0xDF: Force Intel ME Recovery
Request
  Byte 1:3 = Intel Manufacturer ID - 000157h, LS byte first.

  Byte 4 - Command
    = 01h Restart using Recovery Firmware
      (Intel ME FW configuration is not restored to factory defaults)
    = 02h Restore Factory Default Variable values and restart the Intel ME FW
    = 03h PTT Initial State Restore
Response
  Byte 1 - Completion Code
  = 00h - Success
  (Remaining standard Completion Codes are shown in Section 2.12)
  = 81h - Unsupported Command parameter value in the Byte 4 of the request.

  Byte 2:4 = Intel Manufacturer ID - 000157h, LS byte first.
*/
int
me_recovery(uint8_t slot_id, uint8_t command) {
#define RETRY_3_TIME 3
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry <= RETRY_3_TIME) {
    tbuf[0] = 0xB8;
    tbuf[1] = 0xDF;
    tbuf[2] = 0x57;
    tbuf[3] = 0x01;
    tbuf[4] = 0x00;
    tbuf[5] = command;
    tlen = 6;

    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret) {
      retry++;
      sleep(1);
      continue;
    }
    else
      break;
  }
  if (retry > RETRY_3_TIME) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restart using Recovery Firmware failed..., retried: %d", __func__,  retry);
    return -1;
  }

  sleep(10);
  retry = 0;
  memset(&tbuf, 0, 256);
  memset(&rbuf, 0, 256);
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
  while (retry <= RETRY_3_TIME) {
    tbuf[0] = 0x18;
    tbuf[1] = 0x04;
    tlen = 2;
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret) {
      retry++;
      sleep(1);
      continue;
    }

    //if Get Self-Test Results is 0x55 0x00, means No error. All Self-Tests Passed.
    //if Get Self-Test Results is 0x81 0x02, means Firmware entered Recovery bootloader mode
    if ( (command == RECOVERY_MODE) && (rbuf[1] == 0x81) && (rbuf[2] == 0x02) ) {
      return 0;
    }
    else if ( (command == RESTORE_FACTORY_DEFAULT) && (rbuf[1] == 0x55) && (rbuf[2] == 0x00) ) {
      return 0;
    }
    else {
      return -1;
    }
  }
  if (retry > RETRY_3_TIME) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restore Factory Default failed..., retried: %d", __func__,  retry);
    return -1;
  }

  return 0;
}

int
bic_get_fw_cksum_sha256(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *cksum) {
  int ret;
  struct bic_get_fw_cksum_sha256_req req = {
    .iana_id = {0x00},
    .target = target,
    .offset = offset,
    .length = len,
  };
  // Fill the IANA ID
  memcpy(req.iana_id, (uint8_t *)&IANA_ID, 3);
  struct bic_get_fw_cksum_sha256_res res = {0};
  uint8_t rlen = sizeof(res);

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_FW_CKSUM_SHA256, (uint8_t *) &req, sizeof(req), (uint8_t *) &res, &rlen);
  if (ret != 0) {
    return -1;
  }
  if (rlen != sizeof(res)) {
    return -2;
  }

  memcpy(cksum, res.cksum, sizeof(res.cksum));

  return 0;
}

int
bic_get_fw_cksum(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *cksum) {
  uint8_t tbuf[12] = {0x00}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;
  int retries = 3;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  // Fill the component for which firmware is requested
  tbuf[3] = target;

  // Fill the offset
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  // Fill the length
  tbuf[8] = (len) & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  tbuf[10] = (len >> 16) & 0xFF;
  tbuf[11] = (len >> 24) & 0xFF;

bic_send:
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_CKSUM, tbuf, 12, rbuf, &rlen);
  if ((ret || (rlen != 4+3)) && (retries--)) {  // checksum has to be 4 bytes
    sleep(1);
    syslog(LOG_ERR, "bic_get_fw_cksum: slot: %d, target %d, offset: %d, ret: %d, rlen: %d\n", slot_id, target, offset, ret, rlen);
    goto bic_send;
  }
  if (ret || (rlen != 4+3)) {
    return -1;
  }

#ifdef DEBUG
  printf("cksum returns: %x:%x:%x::%x:%x:%x:%x\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6]);
#endif

  //Ignore IANA ID
  memcpy(cksum, &rbuf[IANA_ID_SIZE], rlen-IANA_ID_SIZE);

  return ret;
}
