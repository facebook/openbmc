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
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "bic_ipmi.h"

//FRU
#define FRUID_READ_COUNT_MAX 0x20
#define FRUID_WRITE_COUNT_MAX 0x20
#define FRUID_SIZE 256

//SDR
#define SDR_READ_COUNT_MAX 0x1A
#pragma pack(push, 1)
typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

#define SIZE_SYS_GUID 16

// S/E - Get Sensor reading
// Netfn: 0x04, Cmd: 0x2d
int
bic_get_sensor_reading(uint8_t slot_id, uint8_t sensor_num, ipmi_sensor_reading_t *sensor, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_SENSOR_REQ, CMD_SENSOR_GET_SENSOR_READING, &sensor_num, 1, (uint8_t *)sensor, &rlen, intf);
}

// APP - Get Device ID
// Netfn: 0x06, Cmd: 0x01
int
bic_get_dev_id(uint8_t slot_id, ipmi_dev_id_t *dev_id, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, (uint8_t *) dev_id, &rlen, intf);
}

// APP - Get Device ID
// Netfn: 0x06, Cmd: 0x04
int
bic_get_self_test_result(uint8_t slot_id, uint8_t *self_test_result, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, (uint8_t *)self_test_result, &rlen, intf);
}

// Storage - Get FRUID info
// Netfn: 0x0A, Cmd: 0x10
int
bic_get_fruid_info(uint8_t slot_id, uint8_t fru_id, ipmi_fruid_info_t *info, uint8_t intf) {
  uint8_t rlen = 0;
  uint8_t fruid = 0;
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fruid, 1, (uint8_t *) info, &rlen, intf);
}

// Storage - Reserve SDR
// Netfn: 0x0A, Cmd: 0x22
static int
_get_sdr_rsv(uint8_t slot_id, uint8_t *rsv, uint8_t intf) {
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_RSV_SDR, NULL, 0, (uint8_t *) rsv, &rlen, intf);
}

// Storage - Get SDR
// Netfn: 0x0A, Cmd: 0x23
static int
_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf) {
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR, (uint8_t *)req, sizeof(ipmi_sel_sdr_req_t), (uint8_t*)res, rlen, intf);
}

int
bic_get_sdr(uint8_t slot_id, ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen, uint8_t intf) {
  int ret;
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen;
  uint8_t len;
  ipmi_sel_sdr_res_t *tres;
  sdr_rec_hdr_t *hdr;

  tres = (ipmi_sel_sdr_res_t *) tbuf;

  // Get SDR reservation ID for the given record
  ret = _get_sdr_rsv(slot_id, rbuf, intf);
  if (ret) {
    syslog(LOG_ERR, "%s() _get_sdr_rsv returns %d\n", __func__, ret);
    return ret;
  }

  req->rsv_id = (rbuf[1] << 8) | rbuf[0];

  // Initialize the response length to zero
  *rlen = 0;

  // Read SDR Record Header
  req->offset = 0;
  req->nbytes = sizeof(sdr_rec_hdr_t);

  ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t *)tbuf, &tlen, intf);
  if (ret) {
    syslog(LOG_ERR, "%s() _get_sdr returns %d\n", __func__, ret);
    return ret;
  }

  // Copy the next record id to response
  res->next_rec_id = tres->next_rec_id;

  // Copy the header excluding first two bytes(next_rec_id)
  memcpy(res->data, tres->data, tlen-2);

  // Update response length and offset for next requesdr_rec_hdr_t
  *rlen += tlen-2;
  req->offset = tlen-2;

  // Find length of data from header info
  hdr = (sdr_rec_hdr_t *) tres->data;
  len = hdr->len;

  // Keep reading chunks of SDR record in a loop
  while (len > 0) {
    if (len > SDR_READ_COUNT_MAX) {
      req->nbytes = SDR_READ_COUNT_MAX;
    } else {
      req->nbytes = len;
    }

    ret = _get_sdr(slot_id, req, (ipmi_sel_sdr_res_t *)tbuf, &tlen, intf);
    if (ret) {
      syslog(LOG_ERR, "%s() _get_sdr returns %d\n", __func__, ret);
      return ret;
    }

    // Copy the data excluding the first two bytes(next_rec_id)
    memcpy(&res->data[req->offset], tres->data, tlen-2);

    // Update response length, offset for next request, and remaining length
    *rlen += tlen-2;
    req->offset += tlen-2;
    len -= tlen-2;
  }

  return BIC_STATUS_SUCCESS;
}

// Storage - Get SDR
// Netfn: 0x0A, Cmd: 0x11
static int
_read_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen;

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;
  tlen = 4;
  return bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, tlen, rbuf, rlen, intf);
}

int
bic_read_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, int *fru_size, uint8_t intf) {
  int ret = 0;
  uint32_t nread;
  uint32_t offset;
  uint8_t count;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int fd;
  ssize_t bytes_wr;
  ipmi_fruid_info_t info;

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "bic_read_fruid: open fails for path: %s\n", path);
    goto error_exit;
  }

  // Read the FRUID information
  ret = bic_get_fruid_info(slot_id, fru_id, &info, intf);
  if (ret) {
    syslog(LOG_ERR, "bic_read_fruid: bic_read_fruid_info returns %d\n", ret);
    goto error_exit;
  }

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) | info.size_lsb;
  if (nread > FRUID_SIZE) {
    nread = FRUID_SIZE;
  }

  *fru_size = nread;
  if (*fru_size == 0)
     goto error_exit;

  // Read chunks of FRUID binary data in a loop
  offset = 0;
  while (nread > 0) {
    if (nread > FRUID_READ_COUNT_MAX) {
      count = FRUID_READ_COUNT_MAX;
    } else {
      count = nread;
    }

    ret = _read_fruid(slot_id, fru_id, offset, count, rbuf, &rlen, intf);
    if (ret) {
      syslog(LOG_ERR, "bic_read_fruid: ipmb_wrapper fails\n");
      goto error_exit;
    }

    // Ignore the first byte as it indicates length of response
    bytes_wr = write(fd, &rbuf[1], rlen-1);
    if (bytes_wr != rlen-1) {
      syslog(LOG_ERR, "bic_read_fruid: write to FRU failed\n");
      return -1;
    }

    // Update offset
    offset += (rlen-1);
    nread -= (rlen-1);
  }

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

// Storage - Get SDR
// Netfn: 0x0A, Cmd: 0x12
static int
_write_fruid(uint8_t slot_id, uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *buf, uint8_t intf) {
  int ret;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  memcpy(&tbuf[3], buf, count);
  tlen = count + 3;

  ret = bic_ipmb_send(slot_id, NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA, tbuf, tlen, rbuf, &rlen, intf);
  if ( rbuf[0] != count ) {
    syslog(LOG_WARNING, "%s() Failed to write fruid data. %d != %d \n", __func__, rbuf[0], count);
    return -1;
  }

  return ret;
}

int
bic_write_fruid(uint8_t slot_id, uint8_t fru_id, const char *path, uint8_t intf) {
  int ret = -1;
  uint32_t offset;
  uint8_t count;
  uint8_t buf[64] = {0};
  int fd;

  // Open the file exclusively for read
  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s() Failed to open %s\n", __func__, path);
    goto error_exit;
  }

  // Write chunks of FRUID binary data in a loop
  offset = 0;
  while (1) {
    // Read from file
    count = read(fd, buf, FRUID_WRITE_COUNT_MAX);
    if (count <= 0) {
      break;
    }

    // Write to the FRUID
    ret = _write_fruid(slot_id, fru_id, offset, count, buf, intf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

error_exit:
  if (fd > 0 ) {
    close(fd);
  }

  return ret;
}

// OEM - Get Firmware Version
// Netfn: 0x38, Cmd: 0x0B
int
bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t intf) {
  uint8_t tbuf[4] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  int ret;
  uint8_t bmc_location = 0;

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    ret = BIC_STATUS_FAILURE;
    goto error_exit;
  }

  if ( bmc_location == BB_BMC && (intf == BB_BIC_INTF) ) {
    ret = BIC_STATUS_FAILURE;
    goto error_exit;
  } else if ( bmc_location == NIC_BMC && (intf == FEXP_BIC_INTF) ) {
    ret = BIC_STATUS_FAILURE;
    goto error_exit;
  }

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the component for which firmware is requested
  tbuf[3] = comp;

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, tbuf, 0x04, rbuf, &rlen, intf);

  // rlen should be greater than or equal to 4 (IANA + Data1 +...+ DataN)
  if ( ret < 0 || rlen < 4 ) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d\n", __func__, ret, rlen);
    ret = BIC_STATUS_FAILURE;
    goto error_exit;
  }

  //Ignore IANA ID
  memcpy(ver, &rbuf[3], rlen-3);

error_exit:
  return ret;
}

// OEM - Get Post Code buffer
// Netfn: 0x38, Cmd: 0x12
int
bic_get_80port_record(uint8_t slot_id, uint8_t *rbuf, uint8_t *rlen, uint8_t intf) {
  int ret = 0;
  uint8_t tbuf[3] = {0};
  uint8_t tlen = sizeof(tbuf);

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, tbuf, tlen, rbuf, rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot get the postcode buffer from slot%d", __func__, slot_id);
  } else {
    *rlen -= 3;
    memmove(rbuf, &rbuf[3], *rlen);
  }

  return ret;
}

// Custom Command for getting cpld version
int
bic_get_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  const uint32_t reg = 0x00200028; //for altera cpld

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x04; //read back 4 bytes
  tbuf[3] = (reg >> 24) & 0xff;
  tbuf[4] = (reg >> 16) & 0xff;
  tbuf[5] = (reg >> 8) & 0xff;
  tbuf[6] = (reg >> 0) & 0xff;
  tlen = 7;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, ver, &rlen, intf);
}

// Custom Command for getting vr version/device id
int
bic_get_vr_device_id(uint8_t slot_id, uint8_t comp, uint8_t *rbuf, uint8_t *rlen, uint8_t bus, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t tlen = 0;
  int ret = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x07; //read back 7 bytes
  tbuf[3] = 0xAD; //get device id command
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
  } else {
    *rlen = rbuf[0];//read cnt
    memmove(rbuf, &rbuf[1], *rlen);
  }

  return ret;
}

int
bic_get_vr_ver(uint8_t slot_id, uint8_t comp, uint8_t *rbuf, uint8_t *rlen, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t tlen = 0;
  int ret = 0;
  const uint8_t bus = 0x8;

  ret = bic_get_vr_device_id(slot_id, comp, rbuf, rlen, bus, addr, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
    return ret;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;

  //The length of GET_DEVICE_ID of ISL is different to TI.
  if ( *rlen > 4 ) {
    //TI
    tbuf[2] = 0x02; //read cnt
    tbuf[3] = 0xF0; //command code
    tlen = 4;
  } else {
    //ISL
    tbuf[2] = 0x00; //read cnt
    tbuf[3] = 0xC7; //command code
    tbuf[4] = 0x3F; //reg
    tbuf[5] = 0x00; //dummy data
    tlen = 6;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to send command code to get vr ver. ret=%d", __func__, ret);
      return ret;
    }

    tbuf[2] = 0x04; //read cnt
    tbuf[3] = 0xC5; //command code
    tlen = 4;
  }

  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, rlen, intf);
}

int
bic_get_exp_cpld_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver, uint8_t bus, uint8_t addr, uint8_t intf) {
  uint8_t tbuf[32] = {0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int i = 0;
  int ret = 0;

  //mux
  tbuf[0] = (bus << 1) + 1; //bus
  tbuf[1] = 0xE2; //mux addr
  tbuf[2] = 0x00;
  tbuf[3] = 0x02;
  tlen = 4;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to send the command to switch the mux. ret=%d", __func__, ret);
    goto error_exit;
  }

  //read cpld
  tbuf[1] = addr;
  tbuf[2] = 0x04; //read cnt
  tbuf[3] = 0xC0; //data 1
  tbuf[4] = 0x00; //data 2
  tbuf[5] = 0x00; //data 3
  tbuf[6] = 0x00; //data 4
  tlen = 7;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to send the command code to get cpld ver. ret=%d", __func__, ret);
  }

  for (i = 0; i < 4; i++) ver[i] = rbuf[3-i];

error_exit:
  return ret;
}

int
bic_is_m2_exp_prsnt(uint8_t slot_id) {
  uint8_t tbuf[4] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;
  int present = 0;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x42; //slave addr
  tbuf[2] = 0x01; //read 1 byte
  tbuf[3] = 0x0D; //register offset

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  present = rbuf[0] & 0xC;

  if ( ret < 0 ) {
    return -1;
  } else {
    if ( present == 0) {
      return 3; //1OU+2OU present
    } else if ( present == 8) {
      return 1; //1OU present
    } else if ( present == 4) {
      return 2; //2OU present
    }
  }

  return 0;
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
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retry = 0;

  while (retry <= 3) {
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
  if (retry == 4) { //if the third retry still failed, return -1
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
  while (retry <= 3) {
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
  if (retry == 4) { //if the third retry still failed, return -1
    syslog(LOG_CRIT, "%s: Restore Factory Default failed..., retried: %d", __func__,  retry);
    return -1;
  }
  return 0;
}

int
bic_switch_mux_for_bios_spi(uint8_t slot_id, uint8_t mux) {
  uint8_t tbuf[5] = {0};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x42; //slave addr
  tbuf[2] = 0x01; //read 1 byte
  tbuf[3] = 0x00; //register offset
  tbuf[4] = mux;  //mux setting

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  if ( ret < 0 ) {
    return -1;
  }

  return 0;
}

int
bic_get_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t *data) {
  uint8_t tbuf[13] = {0x00};
  uint8_t rbuf[6] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = sizeof(tbuf);
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  
  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);
  *data = rbuf[3];
  return ret;
}

int
bic_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t data) {
  uint8_t tbuf[14] = {0x00};
  uint8_t rbuf[6] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = sizeof(tbuf);
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  tbuf[13] = data & 0x1f;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);
  return ret;
}

int
bic_set_gpio(uint8_t slot_id, uint8_t gpio_num,uint8_t value) {
  uint8_t tbuf[6] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[3] = 0x01;
  tbuf[4] = gpio_num;
  tbuf[5] = value;

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen);

  if ( ret < 0 ) {
    return -1;
  }

  return 0;
}

int
bic_get_gpio(uint8_t slot_id, bic_gpio_t *gpio) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[13] = {0x00};
  uint8_t rlen = 0;
  int ret;

  memset(gpio, 0, sizeof(*gpio));

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf, 3, rbuf, &rlen);
  if (ret != 0 || rlen < 3)
    return -1;

  rlen -= 3;
  if (rlen > sizeof(bic_gpio_t))
    rlen = sizeof(bic_gpio_t);

  // Ignore first 3 bytes of IANA ID
  memcpy((uint8_t*) gpio, &rbuf[3], rlen);

  return ret;
}

int
bic_get_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  uint8_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_SYSTEM_GUID, NULL, 0, guid, &rlen);
  if (rlen != SIZE_SYS_GUID) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_get_sys_guid: returned rlen of %d\n");
#endif
    return -1;
  }

  return ret;
}

int
bic_set_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN]={0x00};

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_REQ, CMD_OEM_SET_SYSTEM_GUID, guid, SIZE_SYS_GUID, rbuf, &rlen);

  return ret;
}

int
bic_do_sled_cycle(uint8_t slot_id) {
  uint8_t tbuf[4] = {0};
  uint8_t tlen = 4;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x80; //slave addr
  tbuf[2] = 0x00; //read 0 byte
  tbuf[3] = 0xd9; //register offset
  tlen = 4;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, NULL, 0, BB_BIC_INTF);
}



// Only For Class 2
int
bic_set_fan_speed(uint8_t fan_id, uint8_t pwm) {
  uint8_t tbuf[5] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[3] = fan_id;
  tbuf[4] = pwm;

  ret = bic_ipmb_send(1, NETFN_OEM_1S_REQ, CMD_OEM_SET_FAN_DUTY, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
  if ( ret < 0 ) {
    return -1;
  }

  return 0;
}

// Only For Class 2
int
bic_get_fan_speed(uint8_t fan_id, float *value) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[5] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[3] = fan_id;

  ret = bic_ipmb_send(1, NETFN_OEM_1S_REQ, CMD_OEM_GET_FAN_RPM, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
  if ( ret < 0 ) {
    return -1;
  }

  *value = (float)((rbuf[3] << 8)+ rbuf[4]);

  return 0;
}

// Only For Class 2
int
bic_get_fan_pwm(uint8_t fan_id, float *value) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[3] = fan_id;

  ret = bic_ipmb_send(1, NETFN_OEM_1S_REQ, CMD_OEM_GET_FAN_DUTY, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
  if ( ret < 0 ) {
    return -1;
  }

  *value = (float)rbuf[3];

  return 0;
}
