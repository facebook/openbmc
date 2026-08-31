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

//FRU
#define FRUID_READ_COUNT_MAX 0x20
#define FRUID_WRITE_COUNT_MAX 0x20
#define FRUID_SIZE 256

//SDR
#define SDR_READ_COUNT_MAX 0x1A
#pragma pack(push, 1)

#define MASTER_WRITE_READ_HEADER_LEN          3
#define MASTER_WRITE_READ_MAX_WRITE_BUF_SIZE  (MAX_IPMB_BUFFER - MASTER_WRITE_READ_HEADER_LEN)

#define GET_BIC_GPIO_CFG_CMD_LEN 13
#define SET_BIC_GPIO_CFG_CMD_LEN 14

#define GET_SYS_FW_VER_CMD_LEN   4
#define SYS_FW_VER_PARAM_SEL     0x01

#ifdef CONFIG_GRANDCANYON2
#define XDPE15284C_CONF_SIZE 1344
#define XDPE19283B_CONF_SIZE 1416
#define PRODUCT_ID_XDPE19283 0x95
#endif

typedef struct _sdr_rec_hdr_t {
  uint16_t rec_id;
  uint8_t ver;
  uint8_t type;
  uint8_t len;
} sdr_rec_hdr_t;
#pragma pack(pop)

//BIC and BIC bootloader firmware version
typedef struct _bic_get_fw_ver_req {
  uint8_t IANA[SIZE_IANA_ID];
  uint8_t component;
} bic_get_fw_ver_req;

typedef struct _bic_get_fw_ver_resp {
  uint8_t IANA[SIZE_IANA_ID];
  uint8_t ver_response[MAX_BIC_VER_STR_LEN];
} bic_get_fw_ver_resp;

#ifdef CONFIG_GRANDCANYON2
typedef struct _bic_get_fw_rev_ver_req {
  uint8_t IANA[SIZE_IANA_ID];
} bic_get_fw_rev_ver_req;

typedef struct _bic_get_fw_rev_ver_resp {
  uint8_t IANA[SIZE_IANA_ID];
  uint8_t ver_response[MAX_BIC_VER_STR_LEN];
} bic_get_fw_rev_ver_resp;

enum revision_code{
  REV_A = 0x00,
  REV_B,
  REV_C,
  REV_D,
};
#endif

typedef struct {
  uint8_t IANA[SIZE_IANA_ID];
  uint8_t postcode_response[MAX_POSTCODE_LEN];
} bic_get_post_code_resp;

typedef struct {
  uint8_t bus_id;
  uint8_t slave_addr;
  uint8_t read_count;
  uint8_t write_buffer[MASTER_WRITE_READ_MAX_WRITE_BUF_SIZE];
} bic_master_write_read_req;

typedef struct {
  uint8_t param_revision;
  uint8_t set_selector;
  uint8_t encoding;
  uint8_t str_len;
  uint8_t str_data[SIZE_SYSFW_VER];
} bic_get_sys_fw_ver_resp;

#ifdef CONFIG_GRANDCANYON2
int
bic_get_fw_rev_ver(uint8_t slot_id, uint8_t *ver) {
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0;
  int ret = BIC_STATUS_FAILURE;
  bic_get_fw_rev_ver_req ver_req;
  bic_get_fw_rev_ver_resp ver_resp;

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: pointer is NULL\n", __func__);
    return BIC_STATUS_FAILURE;
  }

  memset(&ver_req, 0, sizeof(ver_req));
  memset(&ver_resp, 0, sizeof(ver_resp));

  // Fill the IANA ID
  memcpy(&ver_req, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
  // Fill the component for which firmware is requested

  // NETFN: 0x38, CMD: 0x31 => for get fw revision id from openbic 
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, 0x31, (uint8_t *)&ver_req, sizeof(ver_req), rbuf, &rlen);

  // rlen should be greater than or equal to 4 (IANA + Data1 +...+ DataN)
  if ((ret < 0) || (rlen < 4) || (rlen > (MAX_BIC_VER_STR_LEN + SIZE_IANA_ID))) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d\n", __func__, ret, rlen);
    ret = BIC_STATUS_FAILURE;
  } else {
    memcpy(&ver_resp, rbuf, rlen);
    //Ignore IANA ID
    memcpy(ver, &(ver_resp.ver_response), sizeof(ver_resp.ver_response));
  }

  return ret;
}
#endif

int
bic_get_fw_ver(uint8_t slot_id, uint8_t comp, uint8_t *ver) {
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0;
  int ret = BIC_STATUS_FAILURE;
  bic_get_fw_ver_req ver_req;
  bic_get_fw_ver_resp ver_resp;

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: pointer is NULL\n", __func__);
    return BIC_STATUS_FAILURE;
  }

  memset(&ver_req, 0, sizeof(ver_req));
  memset(&ver_resp, 0, sizeof(ver_resp));

  // Fill the IANA ID
#ifdef CONFIG_GRANDCANYON2
  memcpy(&ver_req, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID);
#else
  memcpy(&ver_req, (uint8_t *)&IANA_ID, SIZE_IANA_ID);
#endif
  // Fill the component for which firmware is requested
  ver_req.component = comp;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER, (uint8_t *)&ver_req, sizeof(ver_req), rbuf, &rlen);

  // rlen should be greater than or equal to 4 (IANA + Data1 +...+ DataN)
  if ((ret < 0) || (rlen < 4) || (rlen > (MAX_BIC_VER_STR_LEN + SIZE_IANA_ID))) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d, comp: %d\n", __func__, ret, rlen, comp);
    ret = BIC_STATUS_FAILURE;
  } else {
    memcpy(&ver_resp, rbuf, rlen);
    //Ignore IANA ID
    memcpy(ver, &(ver_resp.ver_response), sizeof(ver_resp.ver_response));
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

#ifndef CONFIG_GRANDCANYON2
  // set VR page
  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = CMD_INF_VR_SET_PAGE; //command code
  tbuf[4] = 0x01;
  tlen = 5;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s():%d Failed to send command code to switch VR page. ret=%d", __func__,__LINE__, ret);
    return ret;
  }
#endif

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

#ifdef CONFIG_GRANDCANYON2
// Pause (enable=0) or resume (enable=1) BIC firmware's background VR sensor-monitor
int
bic_set_vr_sensor_monitor(uint8_t enable) {
  uint8_t sm_tbuf[8] = {0};
  uint8_t sm_rbuf[8] = {0};
  uint8_t sm_rlen = sizeof(sm_rbuf);
  int ret;

  memcpy(sm_tbuf, (uint8_t *)&META_IANA_ID, IANA_ID_SIZE);
  sm_tbuf[3] = enable ? 0x1 : 0x0;
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_VR_MON_ENABLE, sm_tbuf, 4, sm_rbuf, &sm_rlen);
  if (ret == 0 && !enable) {
    // Give BIC time to finish any in-flight VR PMBus transaction (e.g. a page switch)
    // started before disable took effect.
    msleep(10);
  }
  return ret;
}

int
bic_get_ifx_vr_version_mfr(uint8_t bus, uint8_t addr, uint8_t *ver_data) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  if (ver_data == NULL) {
    syslog(LOG_ERR, "%s: pointer is NULL\n", __func__);
    return -1;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;

  tbuf[2] = 0x00; // read cnt
  tbuf[3] = 0x10; // write protect
  tbuf[4] = 0x00; // unlock
  tlen = 5;
  
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to unlock WP", __func__);
    return ret;
  }

  usleep(300);

  // Retry the init(0xFD)->execute(0xFE)->read(0xFD) sequence up to MAX_RETRY times on failure
  for (int attempt = 0; attempt < MAX_RETRY; attempt++) {
    // Step 1 : Explicitly initialize MFR_FW_COMMAND_DATA (0xFD)
    tbuf[2] = 0x00; // read cnt
    tbuf[3] = CMD_INF_VR_MFR_WRITE; // 0xFD (MFR_FW_COMMAND_DATA)
    tbuf[4] = 0x04; // block write byte count = 4
    tbuf[5] = 0x00; // header_code = 0 (query total checksum)
    tbuf[6] = 0x00; // XVcode = 0
    tbuf[7] = 0x00; // reserved
    tbuf[8] = 0x00; // partition_number = 0
    tlen = 9;

    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() attempt %d/%d failed to initialize MFR_FW_COMMAND_DATA (0xFD), retrying",
             __func__, attempt + 1, MAX_RETRY);
      continue;
    }

    usleep(300);

    // Step 2 : Execute GET_CRC command (0x2D).
    tbuf[2] = 0x00; // read cnt
    tbuf[3] = CMD_INF_VR_MFR_EXECUTE; // 0xFE
    tbuf[4] = INF_VR_CMD_GET_VERSION; // 0x2D (GET_CRC)
    tlen = 5;

    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() attempt %d/%d failed to execute GET_CRC command, retrying",
             __func__, attempt + 1, MAX_RETRY);
      continue;
    }

    // Wait for command execution completion (GET_CRC needs 20ms)
    usleep(20000); // 20ms

    // Step 3 : Read CRC result.
    tbuf[2] = 0x05; // read cnt
    tbuf[3] = CMD_INF_VR_MFR_WRITE; // 0xFD (MFR_FW_COMMAND_DATA)
    tlen = 4;

    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() attempt %d/%d failed to read CRC data, retrying",
             __func__, attempt + 1, MAX_RETRY);
      continue;
    }

    break; // full sequence succeeded
  }
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed after %d retries", __func__, MAX_RETRY);
    return ret;
  }

  // Copy result
  if (rlen >= 5) {
    memcpy(ver_data, rbuf, 5);
    // ver_data[0] = 0x04 (length)
    // ver_data[1] = CRC byte 0
    // ver_data[2] = CRC byte 1
    // ver_data[3] = CRC byte 2
    // ver_data[4] = CRC byte 3
  } else {
    syslog(LOG_WARNING, "%s() Invalid response length: %d, expected >= 5", __func__, rlen);
    ret = -1;
    return ret;
  }

  // Report whatever CRC value comes back, even all-zero -- the IPMB transaction already
  // succeeded, so the VR really is reporting this value (e.g. an invalidated/not-yet
  // -committed section), not a failed read.
  if (ver_data[1] == 0 && ver_data[2] == 0 && ver_data[3] == 0 && ver_data[4] == 0) {
    syslog(LOG_WARNING, "%s() Read back all-zero CRC, bus=%u addr=0x%02X", __func__, bus, addr);
  }

  return ret;
}

 // Config size decision consistent with xdpe152xx.c
static int get_xdpe152xx_config_size_by_devid(uint8_t product_id, uint8_t rev_code) {
  int size = XDPE15284C_CONF_SIZE;

  switch (product_id) {
    case PRODUCT_ID_XDPE19283:
      if (rev_code == REV_B) {
        size = XDPE19283B_CONF_SIZE;
      }
      break;
  }

  return size;
}

// Read Infineon VR remaining writes using MFR_SPECIFIC command (for second source)
int
bic_get_ifx_vr_remaining_writes_mfr(uint8_t bus, uint8_t addr, uint8_t *writes) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  if (!writes) return -1;

  // Read Device ID first to determine config size
  uint8_t devid[8] = {0};
  uint8_t dlen = 0;
  ret = bic_get_vr_device_id(devid, &dlen, bus, addr);
  if (ret < 0 || dlen < 2) {
    syslog(LOG_WARNING, "%s() Failed to read device id", __func__);
    return -1;
  }
  uint8_t rev_code = devid[0];
  uint8_t product_id = devid[1];
  int conf_size = get_xdpe152xx_config_size_by_devid(product_id, rev_code);
  if (conf_size <= 0) conf_size = XDPE15284C_CONF_SIZE; // fallback

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;

  tbuf[2] = 0x00; // read cnt
  tbuf[3] = 0x10; // write protect
  tbuf[4] = 0x00; // unlock
  tlen = 5;
  
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to unlock WP", __func__);
    return ret;
  }
  
  usleep(300);

  // Retry the init(0xFD)->execute(0xFE)->read(0xFD) sequence up to MAX_RETRY times on failure
  for (int attempt = 0; attempt < MAX_RETRY; attempt++) {
    // Step 1 : Explicitly initialize MFR_FW_COMMAND_DATA (0xFD).
    tbuf[2] = 0x00; // read cnt
    tbuf[3] = CMD_INF_VR_MFR_WRITE; // 0xFD
    tbuf[4] = 0x04; // block write byte count = 4
    tbuf[5] = 0x00;
    tbuf[6] = 0x00;
    tbuf[7] = 0x00;
    tbuf[8] = 0x00; // partition_number = 0
    tlen = 9;

    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() attempt %d/%d failed to initialize MFR_FW_COMMAND_DATA (0xFD), retrying",
             __func__, attempt + 1, MAX_RETRY);
      continue;
    }

    usleep(300);

    // Step 2 : Execute OTP_PARTITION_SIZE_REMAINING (0x10).
    tbuf[2] = 0x00; // read cnt
    tbuf[3] = CMD_INF_VR_MFR_EXECUTE; // 0xFE
    tbuf[4] = INF_VR_CMD_GET_REM_WRITES; // 0x10
    tlen = 5;

    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() attempt %d/%d failed to execute OTP_PARTITION_SIZE_REMAINING, retrying",
             __func__, attempt + 1, MAX_RETRY);
      continue;
    }

    // Wait for command execution completion (OTP_PARTITION_SIZE_REMAINING needs 1ms)
    usleep(1000);

    // Step 3 : Read remaining space result
    tbuf[2] = 0x06; // read cnt
    tbuf[3] = CMD_INF_VR_MFR_WRITE; // 0xFD (MFR_FW_COMMAND_DATA)
    tlen = 4;

    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() attempt %d/%d failed to read remaining writes data, retrying",
             __func__, attempt + 1, MAX_RETRY);
      continue;
    }

    break; // full sequence succeeded
  }
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed after %d retries", __func__, MAX_RETRY);
    return ret;
  }

  if (rlen < 5 || rbuf[0] != 0x04) {
    syslog(LOG_WARNING, "%s() Invalid block read: rlen=%u, len=%u", __func__, rlen, rbuf[0]);
    return -1;
  }

  // Little-endian 16-bit remaining bytes (match original xdpe: only take lower 16 bits)
  uint16_t remaining_size = (uint16_t)(rbuf[1] | (rbuf[2] << 8));

  // Report whatever value comes back, even zero -- see bic_get_ifx_vr_version_mfr() above.
  if (remaining_size == 0) {
    syslog(LOG_WARNING, "%s() Read back zero remaining size, bus=%u addr=0x%02X",
           __func__, bus, addr);
  }

  // Convert to "remaining full-program counts"
  *writes = (uint8_t)(remaining_size / conf_size);

  return 0;
}
#endif // CONFIG_GRANDCANYON2

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

#ifdef CONFIG_GRANDCANYON2  
  ret = bic_get_ifx_vr_remaining_writes_mfr(bus, addr, writes);
  if (ret == 0) {
    return ret;
  }
    
  syslog(LOG_INFO, "%s() MFR method failed, trying legacy Page method", __func__);
#endif

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
#ifdef CONFIG_GRANDCANYON2
  tbuf[4] = 0x35;
#else
  tbuf[4] = 0xC2; //data1
#endif
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

#ifdef CONFIG_GRANDCANYON2
// Calculate zero-sum checksum: 2's complement of the sum of all data bytes,
// such that sum(data) + checksum = 0 (mod 256)
static uint8_t
zero_checksum_calculate(uint8_t *buf, uint8_t len) {
  uint8_t i, ret = 0;
  for (i = 0; i < len; i++) {
    ret += *(buf++);
  }
  ret = (~ret) + 1;
  return ret;
}

// Validate zero-sum checksum: 2's complement of sum(data) + checksum should be 0
static bool
zero_checksum_valid(uint8_t *buf, uint8_t len) {
  uint8_t i, ret = 0;
  for (i = 0; i < len; i++) {
    ret += *(buf++);
  }
  ret += *(buf++);   // include checksum byte
  ret = (~ret) + 1;
  return (ret == 0) ? true : false;
}

// Read TI VR remaining writes from BIC EEPROM
int
bic_get_ti_vr_remaining_wr(uint8_t addr, uint16_t *remain) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  if (remain == NULL) {
    syslog(LOG_WARNING, "%s: NULL pointer", __func__);
    return -1;
  }

  tbuf[0] = BIC_EEPROM_BUS;
  tbuf[1] = BIC_EEPROM_ADDR;
  tbuf[2] = 3;                                   // read 3 bytes (high, low, checksum)
  tbuf[3] = VR_REMAINING_WRITE_START_ADDR;
  tbuf[4] = TI_VR_REMAINING_WRITE_OFFSET(addr);
  tlen = 5;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                         tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to read EEPROM, addr=0x%02X, ret=%d",
           __func__, addr, ret);
    return ret;
  }

  *remain = ((uint16_t)rbuf[0] << 8) | rbuf[1];  // higher byte first

  // UNINITIALIZED_EEPROM means not yet provisioned; let caller auto-init
  if (*remain == UNINITIALIZED_EEPROM) {
    syslog(LOG_INFO, "%s() EEPROM uninitialized for VR addr=0x%02X, will auto-init",
           __func__, addr);
    return 0;
  }

  if (!zero_checksum_valid(rbuf, VR_REMAIN_WR_SIZE)) {
    syslog(LOG_WARNING, "%s() Checksum invalid for VR addr=0x%02X "
           "(0x%02X 0x%02X 0x%02X)",
           __func__, addr, rbuf[0], rbuf[1], rbuf[2]);
    return -1;
  }

  return 0;
}

// Write TI VR remaining writes back to BIC EEPROM
int
bic_set_ti_vr_remaining_wr(uint8_t addr, uint16_t remain) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  tbuf[0] = BIC_EEPROM_BUS;
  tbuf[1] = BIC_EEPROM_ADDR;
  tbuf[2] = 0;                                   // write only
  tbuf[3] = VR_REMAINING_WRITE_START_ADDR;
  tbuf[4] = TI_VR_REMAINING_WRITE_OFFSET(addr);
  tbuf[5] = (remain >> 8) & 0xFF;                // higher byte
  tbuf[6] = remain & 0xFF;                       // lower byte
  tbuf[7] = zero_checksum_calculate(&tbuf[5], VR_REMAIN_WR_SIZE);
  tlen = 8;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                         tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to write EEPROM, addr=0x%02X, remain=%u, ret=%d",
           __func__, addr, remain, ret);
  }

  return ret;
}
#endif // CONFIG_GRANDCANYON2

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

#ifdef CONFIG_GRANDCANYON2
static const char *
vr_addr_to_name(uint8_t addr) {
  size_t i;

  for (i = 0; i < bic_vr_list_size; i++) {
    if (bic_vr_list[i].addr == addr) {
      return bic_vr_list[i].name;
    }
  }
  return "unknown";
}
#endif

int
bic_get_vr_ver(uint8_t bus, uint8_t addr, char *key, char *ver_str) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rlen = 0;
  uint8_t remaining_writes = 0;
  int fd = 0;
  int ret = 0;

#ifdef CONFIG_GRANDCANYON2
  // Pause BIC firmware's background VR sensor-monitor task
  if (bic_set_vr_sensor_monitor(VR_SENSOR_MONITOR_DISABLE) < 0) {
    syslog(LOG_WARNING, "%s: failed to disable BIC VR sensor monitor, aborting", __func__);
    return -1;
  }
#endif

  ret = bic_get_vr_device_id(rbuf, &rlen, bus, addr);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get vr device id, ret=%d", __func__, ret);
    goto vr_mon_exit;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;

  //The length of GET_DEVICE_ID is different.
  if (rlen == 2) {
    //Infineon
    //to avoid sensord changing the page of VRs, so use the LOCK file
    //to stop monitoring sensors of VRs
    fd = open(SERVER_SENSOR_LOCK, O_CREAT | O_RDWR, 0666);
    ret = flock(fd, LOCK_EX | LOCK_NB);
    if (ret != 0) {
      if (EWOULDBLOCK == errno) {
        syslog(LOG_WARNING, "%s():%d Failed to lock VR sensor reading", __func__,__LINE__);
        remove(SERVER_SENSOR_LOCK);
        close(fd);
        goto vr_mon_exit;
      }
    }

#ifdef CONFIG_GRANDCANYON2  
    uint8_t ver_data[5] = {0};
    ret = bic_get_ifx_vr_version_mfr(bus, addr, ver_data);
    
    if (ret == 0) {
      uint8_t byte_count = ver_data[0];      
      
      ret = bic_get_ifx_vr_remaining_writes_mfr(bus, addr, &remaining_writes);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s():%d Failed to get remaining writes via MFR method, ret=%d",
               __func__, __LINE__, ret);

        remaining_writes = 0xFF;
        // Failing to read remaining-writes is non-fatal -- the VR version
        // itself was already read successfully above. Reset ret to 0 so
        // this success path doesn't return a stale failure code.
        ret = 0;
      }

      if (byte_count >= 4) {
        snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X, Remaining Writes: %d", 
                 ver_data[4], ver_data[3], ver_data[2], ver_data[1], 
                 (remaining_writes == 0xFF) ? 0 : remaining_writes);
      } else {
        snprintf(ver_str, MAX_VALUE_LEN, "Infineon (unknown format), Remaining Writes: %d", 
                 (remaining_writes == 0xFF) ? 0 : remaining_writes);
      }
      
      if (kv_set(key, ver_str, 0, 0) != 0) {
        syslog(LOG_WARNING, "%s(): failed to write VR version to cache, key=%s", __func__, key);
        // VR version was already read successfully above -- ver_str is
        // valid. Report the cache-write failure distinctly instead of a
        // generic failure, so callers don't discard a good ver_str.
        ret = BIC_VR_VER_CACHE_WRITE_FAILED;
      }

      syslog(LOG_INFO, "%s() Successfully read Infineon VR via MFR method: %s", __func__, ver_str);
      goto cleanup;
    }

    syslog(LOG_INFO, "%s() MFR method failed, trying legacy Page method", __func__);
#endif

    ret = bic_get_ifx_vr_remaining_writes(bus, addr, &remaining_writes);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr remaining writes. ret=%d", __func__,__LINE__, ret);
#ifdef CONFIG_GRANDCANYON2      
      remaining_writes = 0xFF;      
#else
      goto error_exit;
#endif
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
#ifdef CONFIG_GRANDCANYON2      
      if (remaining_writes == 0xFF) {
        snprintf(ver_str, MAX_VALUE_LEN, "Infineon (version unavailable)");
      } else {
        snprintf(ver_str, MAX_VALUE_LEN, "Infineon (version unavailable), Remaining Writes: %d", 
                 remaining_writes);
      }
      if (kv_set(key, ver_str, 0, 0) != 0) {
        syslog(LOG_WARNING, "%s(): failed to write VR version to cache, key=%s", __func__, key);
        ret = -1;
      }
#endif
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
    
#ifdef CONFIG_GRANDCANYON2    
    if (remaining_writes == 0xFF) {
      snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X", 
               rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
    } else {
      snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X, Remaining Writes: %d", 
               rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
    }
    if (kv_set(key, ver_str, 0, 0) != 0) {
      syslog(LOG_WARNING, "%s(): failed to write VR version to cache, key=%s", __func__, key);
      // VR version was already read successfully above -- ver_str is
      // valid. Report the cache-write failure distinctly instead of a
      // generic failure, so callers don't discard a good ver_str.
      ret = BIC_VR_VER_CACHE_WRITE_FAILED;
    }

#else
    snprintf(ver_str, MAX_VALUE_LEN, "Infineon %02X%02X%02X%02X, Remaining Writes: %d", 
             rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
    kv_set(key, ver_str, 0, 0);
#endif

#ifdef CONFIG_GRANDCANYON2
cleanup:
#endif
  error_exit:
    // Do NOT assign flock()'s own return value into "ret" here -- ret is
    // still holding the actual VR-read result (0 on success, negative on
    // any of the failure paths above via goto error_exit). Overwriting it
    // with the unlock's result silently turned real read failures into a
    // reported "success" (flock(LOCK_UN) essentially never fails).
    if (flock(fd, LOCK_UN) == -1) {
      syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, SERVER_SENSOR_LOCK);
    }
    close(fd);
    remove(SERVER_SENSOR_LOCK);
    goto vr_mon_exit;

  } else if (rlen > 4) {
    //TI
    tbuf[2] = 0x02; //read cnt
    tbuf[3] = CMD_TI_VR_NVM_CHECKSUM; //command code
    tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto vr_mon_exit;
    }

#ifdef CONFIG_GRANDCANYON2
    // TI VR has no remaining-writes query command; read software counter from BIC EEPROM
    {
      uint16_t remaining = 0;
      if (bic_get_ti_vr_remaining_wr(addr, &remaining) < 0) {
        snprintf(ver_str, MAX_VALUE_LEN,
                 "Texas Instruments %02X%02X, Remaining Writes: Unknown",
                 rbuf[1], rbuf[0]);
      } else {
        if (remaining == UNINITIALIZED_EEPROM) {
          // Auto-init on first use
          remaining = MAX_TI_VR_REMAIN_WR;
          bic_set_ti_vr_remaining_wr(addr, remaining);
        }
        snprintf(ver_str, MAX_VALUE_LEN,
                 "Texas Instruments %02X%02X, Remaining Writes: %u",
                 rbuf[1], rbuf[0], remaining);
      }
    }
    if (kv_set(key, ver_str, 0, 0) != 0) {
      syslog(LOG_WARNING, "%s(): failed to write VR version to cache, key=%s", __func__, key);
      // VR version was already read successfully above -- ver_str is
      // valid. Report the cache-write failure distinctly instead of a
      // generic failure, so callers don't discard a good ver_str.
      ret = BIC_VR_VER_CACHE_WRITE_FAILED;
    }
#else
    snprintf(ver_str, MAX_VALUE_LEN, "Texas Instruments %02X%02X", rbuf[1], rbuf[0]);
    kv_set(key, ver_str, 0, 0);
#endif
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
#ifdef CONFIG_GRANDCANYON2
    tbuf[4] = 0x94; //reg
#else
    tbuf[4] = 0x3F; //reg
#endif
    tbuf[5] = 0x00; //dummy data
    tlen = 6;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto vr_mon_exit;
    }

    tbuf[2] = 0x04; //read cnt
    tbuf[3] = CMD_ISL_VR_DMAFIX; //command code
    tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s():%d Failed to send command code to get vr ver. ret=%d", __func__,__LINE__, ret);
      goto vr_mon_exit;
    }
    snprintf(ver_str, MAX_VALUE_LEN, "Renesas %02X%02X%02X%02X, Remaining Writes: %d", rbuf[3], rbuf[2], rbuf[1], rbuf[0], remaining_writes);
#ifdef CONFIG_GRANDCANYON2
    if (kv_set(key, ver_str, 0, 0) != 0) {
      syslog(LOG_WARNING, "%s(): failed to write VR version to cache, key=%s", __func__, key);
      // VR version was already read successfully above -- ver_str is
      // valid. Report the cache-write failure distinctly instead of a
      // generic failure, so callers don't discard a good ver_str.
      ret = BIC_VR_VER_CACHE_WRITE_FAILED;
    }
#else
    kv_set(key, ver_str, 0, 0);
#endif
  }

vr_mon_exit:
#ifdef CONFIG_GRANDCANYON2
  if (bic_set_vr_sensor_monitor(VR_SENSOR_MONITOR_ENABLE) < 0) {
    syslog(LOG_WARNING, "%s: failed to re-enable BIC VR sensor monitor", __func__);
  }

  if (ret >= 0) {
    syslog(LOG_INFO, "Get %s version from bic. bus=0x%X addr=0x%X", vr_addr_to_name(addr), bus, addr);
  }
#endif
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
    if (bic_get_vr_ver(bus, addr, key, tmp_str) < 0) {
      return -1;
    }
  } else {
#ifdef CONFIG_GRANDCANYON2
    syslog(LOG_INFO, "Get %s version from cache. bus=0x%X addr=0x%X", vr_addr_to_name(addr), bus, addr);
#endif
  }
  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN - 1)) {
    return -1;
  }
  return 0;
}

#ifdef CONFIG_GRANDCANYON2
#define VR_VER_CACHE_BUS 0x4
const bic_vr_info_t bic_vr_list[] = {
  {0xC0, "PVCCIN_FIVRA"},
  {0xC4, "PVCCD_HV"},
  {0xEC, "PVCCINFAON"},
};
const size_t bic_vr_list_size = sizeof(bic_vr_list) / sizeof(bic_vr_list[0]);

int
bic_refresh_all_vr_ver_cache(void) {
  char key[MAX_KEY_LEN] = {0};
  char ver_str[MAX_VALUE_LEN] = {0};
  size_t i;
  int ret = 0;
  int r;

  for (i = 0; i < bic_vr_list_size; i++) {
    memset(key, 0, sizeof(key));
    memset(ver_str, 0, sizeof(ver_str));
    snprintf(key, sizeof(key), "vr_%02xh_crc", bic_vr_list[i].addr);
    r = bic_get_vr_ver(VR_VER_CACHE_BUS, bic_vr_list[i].addr, key, ver_str);
    if (r < 0) {
      // Actually failed to read the VR version from hardware.
      syslog(LOG_WARNING, "%s(): failed to refresh VR 0x%02X version cache", __func__, bic_vr_list[i].addr);
      ret = -1;
    } else if (r == BIC_VR_VER_CACHE_WRITE_FAILED) {
      // VR version WAS read OK, only the cache persist step failed.
      syslog(LOG_WARNING, "%s(): VR 0x%02X version read OK but cache write failed", __func__, bic_vr_list[i].addr);
      ret = -1;
    }
  }
  return ret;
}
#endif

static int
_read_fruid(uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *rbuf, uint8_t *rlen) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;

  tbuf[0] = fru_id;
  tbuf[1] = offset & 0xFF;
  tbuf[2] = (offset >> 8) & 0xFF;
  tbuf[3] = count;
  tlen = 4;

  return bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA, tbuf, tlen, rbuf, rlen);
}

// Storage - Get FRUID info
// Netfn: 0x0A, Cmd: 0x10
int
bic_get_fruid_info(uint8_t fru_id, ipmi_fruid_info_t *info) {
  uint8_t rlen = 0;
  uint8_t fruid = 0;
  return bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO, &fruid, 1, (uint8_t *) info, &rlen);
}

int
bic_read_fruid(uint8_t fru_id, char *path, int *fru_size) {
  int ret = 0;
  uint32_t nread = 0;
  uint32_t offset = 0;
  uint8_t count = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  int fd = 0;
  ssize_t bytes_wr = 0;
  ipmi_fruid_info_t info = {0};

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s open fails for path: %s\n", __func__, path);
    goto error_exit;
  }

  // Read the FRUID information
  ret = bic_get_fruid_info(fru_id, &info);
  if (ret) {
    syslog(LOG_ERR, "%s bic_read_fruid_info returns %d\n", __func__, ret);
    goto error_exit;
  }

  // Indicates the size of the FRUID
  nread = (info.size_msb << 8) | info.size_lsb;
  if (nread > FRUID_SIZE) {
    nread = FRUID_SIZE;
  }

  *fru_size = nread;
  if (*fru_size == 0) {
     goto error_exit;
  }

  // Read chunks of FRUID binary data in a loop
  offset = 0;
  while (nread > 0) {
    if (nread > FRUID_READ_COUNT_MAX) {
      count = FRUID_READ_COUNT_MAX;
    } else {
      count = nread;
    }

    ret = _read_fruid(fru_id, offset, count, rbuf, &rlen);
    if (ret) {
      syslog(LOG_ERR, "%s _read_fruid failed, ret: %d\n", __func__, ret);
      goto error_exit;
    }

    // Ignore the first byte as it indicates length of response
    bytes_wr = write(fd, &rbuf[1], rlen-1);
    if (bytes_wr != rlen-1) {
      syslog(LOG_ERR, "%s write fruid failed, write bytes: %d\n", __func__, bytes_wr);
      return -1;
    }

    // Update offset
    offset += (rlen-1);
    nread -= (rlen-1);
  }

error_exit:
  if (fd >= 0 ) {
    close(fd);
  }

  return ret;
}

static int
_write_fruid(uint8_t fru_id, uint32_t offset, uint8_t count, uint8_t *buf) {
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = fru_id;
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

int
bic_write_fruid(uint8_t fru_id, const char *path) {
  int ret = -1;
  uint32_t offset = 0;
  uint8_t count = 0;
  uint8_t buf[64] = {0};
  int fd = 0;

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
    ret = _write_fruid(fru_id, offset, count, buf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

error_exit:
  if (fd >= 0 ) {
    close(fd);
  }

  return ret;
}

static int
_get_sdr_rsv(uint8_t *rsv) {
  uint8_t rlen = 0;
  return bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_RSV_SDR, NULL, 0, (uint8_t *) rsv, &rlen);
}

static int
_get_sdr(ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen) {
  return bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR, (uint8_t *)req, sizeof(ipmi_sel_sdr_req_t), (uint8_t*)res, rlen);
}

int
bic_get_sdr(ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen) {
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t len = 0;
  ipmi_sel_sdr_res_t *tres = NULL;
  sdr_rec_hdr_t *hdr = NULL;

  tres = (ipmi_sel_sdr_res_t *) tbuf;

  // Get SDR reservation ID for the given record
  ret = _get_sdr_rsv(rbuf);
  if (ret != 0) {
    syslog(LOG_ERR, "%s() _get_sdr_rsv returns %d\n", __func__, ret);
    return ret;
  }

  req->rsv_id = (rbuf[1] << 8) | rbuf[0];

  // Initialize the response length to zero
  *rlen = 0;

  // Read SDR Record Header
  req->offset = 0;
  req->nbytes = sizeof(sdr_rec_hdr_t);

  ret = _get_sdr(req, (ipmi_sel_sdr_res_t *)tbuf, &tlen);
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

    ret = _get_sdr(req, (ipmi_sel_sdr_res_t *)tbuf, &tlen);
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

  return 0;
}

int
bic_get_sensor_reading(uint8_t sensor_num, ipmi_sensor_reading_t *sensor) {
  uint8_t rlen = 0;
  return bic_ipmb_wrapper(NETFN_SENSOR_REQ, CMD_SENSOR_GET_SENSOR_READING, &sensor_num, 1, (uint8_t *)sensor, &rlen);
}

int
bic_get_self_test_result(uint8_t *self_test_result) {
  uint8_t rlen = 0;
  int ret = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SELFTEST_RESULTS, NULL, 0, rbuf, &rlen);
  if (ret == 0 && rlen == 2) {
    memcpy(self_test_result, rbuf, rlen);
  }

  return ret;
}

int
bic_get_sys_guid(uint8_t slot_id, uint8_t *guid, uint8_t guid_size) {
  int ret = 0;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};

  if (guid == NULL) {
    syslog(LOG_ERR, "%s: get pointer guid failed\n", __func__);
    
    return BIC_STATUS_FAILURE;
  }
 
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SYSTEM_GUID, NULL, 0, rbuf, &rlen);
  if ((ret < 0) || (rlen != guid_size)) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d\n", __func__, ret, rlen);
    ret = BIC_STATUS_FAILURE;
  } else {
    memcpy(guid, rbuf, guid_size);
  } 

  return ret;
}

int
bic_set_sys_guid(uint8_t slot_id, uint8_t *guid, uint8_t guid_size) {
  int ret = 0;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};

  if (guid == NULL) {
    syslog(LOG_ERR, "%s: get pointer guid failed\n", __func__);
    
    return BIC_STATUS_FAILURE;
  }
  
  ret = bic_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_SET_SYSTEM_GUID, guid, guid_size, rbuf, &rlen);

  if (ret < 0) {
    syslog(LOG_ERR, "%s: ret: %d\n", __func__, ret);
    
    ret = BIC_STATUS_FAILURE;
  }

  return ret;
}

// OEM - Get Post Code buffer
// Netfn: 0x38, Cmd: 0x12
int
bic_get_80port_record(uint16_t max_len, uint8_t *rbuf, uint8_t *rlen) {
  int ret = 0;
  bic_get_post_code_resp postcode_resp;

  if ((rbuf == NULL) || (rlen == NULL)) {
    syslog(LOG_WARNING, "%s Cannot get the postcode buffer from server because of NULL parameters of response buffer and response length", __func__);
    return -1;
  }

  memset(&postcode_resp, 0, sizeof(postcode_resp));

#ifdef CONFIG_GRANDCANYON2
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, (uint8_t *)&META_IANA_ID, SIZE_IANA_ID, (uint8_t *)&postcode_resp, rlen);
#else
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_POST_BUF, (uint8_t *)&IANA_ID, SIZE_IANA_ID, (uint8_t *)&postcode_resp, rlen);
#endif

  if (ret < 0) {
    syslog(LOG_WARNING, "%s Cannot get the postcode buffer from server", __func__);
  } else if (*rlen < SIZE_IANA_ID) {
    syslog(LOG_WARNING, "%s Cannot get the postcode buffer from server because the response length: %d should greater or equal to length: %d.", __func__, *rlen, SIZE_IANA_ID);
    return -1;
  } else if ((*rlen - SIZE_IANA_ID) > max_len) {
    syslog(LOG_WARNING, "%s Cannot get the postcode buffer from server because the response length: %d is larger than max length: %d.", __func__, (*rlen - SIZE_IANA_ID), max_len);
    return -1;
  } else {
    *rlen -= SIZE_IANA_ID;
    memcpy(rbuf, &(postcode_resp.postcode_response), MIN(*rlen, sizeof(postcode_resp.postcode_response)));
  }

  return ret;
}

int
bic_master_write_read(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t *wbuf, uint8_t wcnt, uint8_t *rbuf, uint8_t rcnt) {
  uint8_t tlen = MASTER_WRITE_READ_HEADER_LEN, rlen = 0;
  int ret = 0;
  bic_master_write_read_req tbuf;

  if ((wbuf == NULL) || (rbuf == NULL)) {
    syslog(LOG_ERR, "%s: failed to send ipmb wrapper because parameters are NULL\n", __func__);
    return BIC_STATUS_FAILURE;
  }

  memset(&tbuf, 0, sizeof(tbuf));

  tbuf.bus_id     = bus;
  tbuf.slave_addr = addr;
  tbuf.read_count = rcnt;

  if ((wcnt != 0) && (wcnt <= sizeof(tbuf.write_buffer))) {
    memcpy(&tbuf.write_buffer, wbuf, wcnt);
    tlen += wcnt;
  } else {
    syslog(LOG_ERR, "%s: failed to set write buffer because write buffer length: %d is wrong, max write buffer length: %d\n", __func__, wcnt, sizeof(tbuf.write_buffer));
    return BIC_STATUS_FAILURE;
  }

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, (uint8_t *)&tbuf, tlen, rbuf, &rlen);

  if ((ret < 0) || (rlen != rcnt)) {
    syslog(LOG_ERR, "%s: failed to send ipmb wrapper. ret: %d, response length: %d, expected length: %d\n", __func__, ret, rlen, rcnt);
    return BIC_STATUS_FAILURE;
  }

  return ret;
}

int
bic_asd_init(uint8_t cmd) {
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

#ifdef CONFIG_GRANDCANYON2
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, 3);
#else
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
#endif
  tbuf[3] = cmd;
  return bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_INIT, tbuf, tlen, rbuf, &rlen);
}

// Get one GPIO pin status
int
bic_get_one_gpio_status(uint8_t gpio_num, uint8_t *value){
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret = 0;

  if (value == NULL) {
    syslog(LOG_ERR, "%s(): gpio value should not be NULL", __func__);
    return -1;
  }
  // File the IANA ID
#ifdef CONFIG_GRANDCANYON2
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, 3);
#else
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
#endif
  tbuf[3] = 0x00;
  tbuf[4] = gpio_num;
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen);
  *value = rbuf[4] & 0x01;
  return ret;
}

int
bic_set_gpio(uint8_t gpio_num, uint8_t value) {
#ifdef CONFIG_GRANDCANYON2
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x15, 0xA0, 0x00}; // IANA ID
#else
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x9c, 0x9c, 0x00}; // IANA ID
#endif
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;
  int ret = 0;

  // IANA ID
  if (fbgc_common_is_grandcanyon2()) {
      tbuf[0] = 0x15;
      tbuf[1] = 0xA0;
      tbuf[2] = 0x00;
  } else {
      tbuf[0] = 0x9c;
      tbuf[1] = 0x9c;
      tbuf[2] = 0x00;
  }

  tbuf[3] = 0x01;
  tbuf[4] = gpio_num;
  tbuf[5] = value;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen);

  if (ret < 0) {
    return -1;
  }

  return 0;
}

// APP - Get Device ID
// Netfn: 0x06, Cmd: 0x01
int
bic_get_dev_id(ipmi_dev_id_t *dev_id) {
  uint8_t rlen = 0;

  if (dev_id == NULL) {
    syslog(LOG_ERR, "%s(): device ID is missing", __func__);
  }
  return bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_DEVICE_ID, NULL, 0, (uint8_t *) dev_id, &rlen);
}

int
bic_reset() {
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  int ret;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_COLD_RESET, NULL, 0, rbuf, &rlen);
  return ret;
}

int
bic_clear_cmos() {
#ifdef CONFIG_GRANDCANYON2
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x15, 0xA0, 0x00}; // IANA ID
#else
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x9c, 0x9c, 0x00}; // IANA ID
#endif
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;

  return bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_CLEAR_CMOS, tbuf, 3, rbuf, &rlen);
}

int
bic_get_gpio_config(uint8_t gpio, uint8_t *data) {
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = GET_BIC_GPIO_CFG_CMD_LEN;
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  if (data == NULL) {
    syslog(LOG_ERR, "%s(): data is missing", __func__);
  }
  // File the IANA ID
#ifdef CONFIG_GRANDCANYON2
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, 3);
#else
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
#endif

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);
  if (rlen < 4) {
    syslog(LOG_ERR, "%s(): Wrong response length: %d", __func__, rlen);
    return -1;
  }
  *data = rbuf[3];
  return ret;
}

int
bic_set_gpio_config(uint8_t gpio, uint8_t data) {
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  uint8_t tlen = SET_BIC_GPIO_CFG_CMD_LEN;
  uint8_t index = 0;
  uint8_t pin = 0;
  int ret = 0;

  // File the IANA ID
#ifdef CONFIG_GRANDCANYON2
  memcpy(tbuf, (uint8_t *)&META_IANA_ID, 3);
#else
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
#endif

  //get the buffer index
  index = (gpio / 8) + 3; //3 is the size of IANA ID
  pin = 1 << (gpio % 8);
  tbuf[index] = pin;
  tbuf[13] = data & 0x1f;

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen);
  return ret;
}

// Get all GPIO pin status
int
bic_get_gpio(bic_gpio_t *gpio) {
#ifdef CONFIG_GRANDCANYON2
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x15, 0xA0, 0x00}; // IANA ID
#else
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x9c, 0x9c, 0x00}; // IANA ID
#endif
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;

  if (gpio == NULL) {
    syslog(LOG_WARNING, "%s(): gpio status is missing", __func__);
    return -1;
  }
  memset(gpio, 0, sizeof(*gpio));

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf, 3, rbuf, &rlen);
  if ((ret != 0) || (rlen < 3)) {
    return -1;
  }

  rlen -= 3;
  if (rlen > sizeof(bic_gpio_t)) {
    rlen = sizeof(bic_gpio_t);
  }
  // Ignore first 3 bytes of IANA ID
  memcpy((uint8_t*) gpio, &rbuf[3], rlen);

  return ret;
}

// Get BIC Configuration
int
bic_get_config(bic_config_t *cfg) {
#ifdef CONFIG_GRANDCANYON2
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x15, 0xA0, 0x00}; // IANA ID
#else
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x9c, 0x9c, 0x00}; // IANA ID
#endif
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;

  if (cfg == NULL) {
    syslog(LOG_ERR, "%s(): Configuration is missing", __func__);
    return -1;
  }
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_CONFIG, tbuf, 3, rbuf, &rlen);
  // Ignore IANA ID
  if (rlen < 4) {
    syslog(LOG_ERR, "%s(): Get BIC config failed, rlen: %d", __func__, rlen);
    return -1;
  }
  *(uint8_t *) cfg = rbuf[3];

  return ret;
}

// Read Sensor Data Records (SDR)
int
bic_get_sdr_info(ipmi_sel_sdr_info_t *info) {
  int ret = 0;
  uint8_t rlen = 0;

  if (info == NULL) {
    syslog(LOG_ERR, "%s(): SEL information is missing", __func__);
    return -1;
  }
  ret = bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_SDR_INFO, NULL, 0, (uint8_t *) info, &rlen);

  return ret;
}

int
bic_get_sel(ipmi_sel_sdr_req_t *req, ipmi_sel_sdr_res_t *res, uint8_t *rlen) {
  int ret = 0;

  if (req == NULL) {
    syslog(LOG_ERR, "%s(): requset is missing", __func__);
    return -1;
  }
  if (res == NULL) {
    syslog(LOG_ERR, "%s(): response is missing", __func__);
    return -1;
  }
  if (rlen == NULL) {
    syslog(LOG_ERR, "%s(): requset length is missing", __func__);
    return -1;
  }
  ret = bic_ipmb_wrapper(NETFN_STORAGE_REQ, CMD_STORAGE_GET_SEL, (uint8_t *)req, sizeof(ipmi_sel_sdr_req_t), (uint8_t*)res, rlen);

  return ret;
}

int
bic_get_sys_fw_ver(uint8_t *ver) {
  int ret = 0;
  int data_index = 0;
  uint8_t tlen = 0, rlen = 0;
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  bic_get_sys_fw_ver_resp sys_fw_resp;

  if (ver == NULL) {
    syslog(LOG_ERR, "%s: failed to get system firmware version due to NULL pointer\n", __func__);
    return BIC_STATUS_FAILURE;
  }

  memset(&sys_fw_resp, 0, sizeof(sys_fw_resp));

  tlen = GET_SYS_FW_VER_CMD_LEN;
  tbuf[0] = 0x00;                 // parameter revision
  tbuf[1] = SYS_FW_VER_PARAM_SEL; // parameter selector
  tbuf[2] = 0x00;                 // set selector
  tbuf[3] = 0x00;                 // block selector

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_GET_SYS_INFO_PARAMS, tbuf, tlen, (uint8_t *)&sys_fw_resp, &rlen);
  if ((ret < 0) || ((rlen - 1) != SIZE_SYSFW_VER)) {
    syslog(LOG_ERR, "%s: ret: %d, rlen: %d\n", __func__, ret, rlen);
    ret = BIC_STATUS_FAILURE;
  } else {
    // set remaining data byte to 0x00
    if ((SIZE_SYSFW_VER - sys_fw_resp.str_len - 3) > 0) {
      data_index = (int) sys_fw_resp.str_len;
      memset(&sys_fw_resp.str_data[data_index], 0x00, (SIZE_SYSFW_VER - sys_fw_resp.str_len - 3));
    }
    memcpy(ver, &(sys_fw_resp.set_selector), (rlen-1));
  }

  return ret;
}