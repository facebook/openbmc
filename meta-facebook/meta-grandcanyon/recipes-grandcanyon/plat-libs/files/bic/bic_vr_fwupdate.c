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
#include "bic_vr_fwupdate.h"
#include "bic_ipmi.h"

//#define DEBUG

/****************************/
/*       VR fw update       */                            
/****************************/
#define WARNING_REMAINING_WRITES 3
#ifdef CONFIG_GRANDCANYON2
#define VR_BUS 0x4
#else
#define VR_BUS 0x8
#endif
#define VR_DATA_LEN 64
#define VR_PACKET_DATA_LEN 2048
#define DEVID_BYTE_CNT 6
#ifdef CONFIG_GRANDCANYON2
#define VR_CNT 3
#else
#define VR_CNT 4
#endif

#define MAX_ISL_IMG_BUF 64
#define MAX_IFX_IMG_BUF 128

#define CRC8_BUF_LEN 16
#define CRC16_BUF_LEN 254
#define CRC16_LUT_LEN 256
#define CRC16_RESULT_LEN 2

#ifdef CONFIG_GRANDCANYON2
// ISL Gen3 CRC record location
#define ISL_GEN3_FILE_HEAD           5
#define ISL_GEN3_LEGACY_CRC_IDX      (276 - ISL_GEN3_FILE_HEAD)   // 271
#define ISL_GEN3_PRODUCTION_CRC_IDX  (290 - ISL_GEN3_FILE_HEAD)   // 285
#define ISL_GEN3_SW_REV_MIN          0x06
#define CMD_ISL_VR_DEV_REV           0xAE  // DEVREV record marker (vs 0xAD for DEVID)

#endif

typedef struct {
  uint8_t command;
  uint8_t data_len;
  uint8_t data[VR_DATA_LEN];
} vr_data;

typedef struct {
  uint8_t addr;
  uint8_t devid[DEVID_BYTE_CNT];
  uint8_t devid_len;
  int data_cnt;
  vr_data pdata[VR_PACKET_DATA_LEN];
  uint32_t expected_crc;
} vr;

static int vr_cnt = 0;

//4 VRs are on the server board
static vr vr_list[VR_CNT] = {0};


static void
show_progress(int current_progress, int total) {
  printf("Progress: %.0f %%\r", (float) (((current_progress+1) / (float)total)*100));
  fflush(stdout);
}

static int
vr_remaining_writes_check(uint8_t cnt, uint8_t force) {
  int ret = BIC_STATUS_SUCCESS;

  printf("remaining writes %d.\n", cnt);
  if ( cnt == 0 ) {
    printf("The device cannot be programmed since the remaining writes is 0.\n");
    return BIC_STATUS_FAILURE;
  }

  switch (force) {
    case FORCE_UPDATE_UNSET:
      ret = BIC_STATUS_FAILURE;
      if ( cnt <= WARNING_REMAINING_WRITES ) {
        printf("WARNING: the remaining writes is below the threshold value %d!\n", WARNING_REMAINING_WRITES);
        printf("Please use `--force` option to try again.\n");
      } else {
        ret = BIC_STATUS_SUCCESS;
      }
    case FORCE_UPDATE_SET:
      /*fall through*/
    default:
     break;
  }
  fflush(stdout);

  return ret;
}

#ifdef CONFIG_GRANDCANYON2
static int
vr_already_flashed_check(uint32_t dev_val, uint32_t exp_val, uint8_t force, const char *label, const char *value_fmt) {
  char hexstr[16];

  if (force == FORCE_UPDATE_SET) {
    return 0;
  }
  if (dev_val != exp_val) {
    return 0;
  }

  snprintf(hexstr, sizeof(hexstr), value_fmt, dev_val);
  printf("WARNING: the %s is the same as used now %s!\n", label, hexstr);
  printf("Please use \"--force\" option to try again.\n");
  syslog(LOG_WARNING, "%s: redundant programming", __func__);
  return -1;
}
#endif

// Refer "isl69259_ds_Aug_21_2019" 10.71 & 10.73
static int
vr_ISL_polling_status(uint8_t addr) {
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = CMD_ISL_VR_DMAADDR; //command code
#ifdef CONFIG_GRANDCANYON2
  tbuf[4] = 0x7E;               // isl69260 Gen3
  tbuf[5] = 0x00;
#else
  tbuf[4] = 0x07; //data0
  tbuf[5] = 0x07; //data1
#endif
  tlen = 6;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Failed to send PROGRAMMER_STATUS command", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x04; //read cnt
  tbuf[3] = CMD_ISL_VR_DMAFIX; //command code
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Failed to get PROGRAMMER_STATUS", __func__);
    goto error_exit;
  }

#ifdef CONFIG_GRANDCANYON2
  // bit5=1 fail(OTP consumed), bit4=1 CRC fail OTP
  // bit3=1 CRC mismatch, bit2=1 too many configs
  // bit1=1 fail, bit0=1 complete
  if (rbuf[0] & 0x26) {
    syslog(LOG_WARNING, "[%s] PROGRAMMER_STATUS failure: 0x%02X", __func__, rbuf[0]);
    return -1;
  }
#endif

  //bit1 is held to 1, it means the action is successful.
  return rbuf[0] & 0x1;

error_exit:
  return ret;
}

#ifdef CONFIG_GRANDCANYON2
static int
vr_ISL_get_crc(uint8_t addr, uint32_t *crc) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0, rlen = 0;
  int ret;

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00;
  tbuf[3] = CMD_ISL_VR_DMAADDR;
  tbuf[4] = 0x94; // per Renesas DMP Gen3 CRC Check guide, Step 6c
  tbuf[5] = 0x00;
  tlen = 6;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Failed to set DMA address 0x94", __func__);
    return ret;
  }

  tbuf[2] = 0x04;
  tbuf[3] = CMD_ISL_VR_DMAFIX;
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Failed to read CRC via DMA", __func__);
    return ret;
  }

  // little-endian: rbuf[0] is LSB ... rbuf[3] is MSB (per Renesas doc Step 6c)
  *crc = (uint32_t)rbuf[0] | ((uint32_t)rbuf[1] << 8) | ((uint32_t)rbuf[2] << 16) | ((uint32_t)rbuf[3] << 24);
  return 0;
}
#endif

static int
vr_ISL_program(vr *dev, uint8_t force) {
  int i = 0;
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t remaining_writes = 0x00;
  int retry = MAX_RETRY;
  vr_data *list = dev->pdata;
  uint8_t addr = dev->addr;
  int len = dev->data_cnt;

#ifdef CONFIG_GRANDCANYON2
  if (dev->expected_crc != 0) {
    uint32_t crc_now = 0;
    ret = vr_ISL_get_crc(addr, &crc_now);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Failed to read current CRC before programming, skip pre-check", __func__);
    } else {
      int chk = vr_already_flashed_check(crc_now, dev->expected_crc, force, "CRC", "%08X");
      if (chk < 0) {
        return -1;
      }
    }
  } else {
    syslog(LOG_WARNING, "[%s] No expected_crc parsed from file, skip pre-flash guard", __func__);
  }
#endif

  //get the remaining of the VR
  ret = bic_get_isl_vr_remaining_writes(VR_BUS, addr, &remaining_writes);
  if ( ret < 0 ) {
    goto error_exit;
  }

  //check it
  ret = vr_remaining_writes_check(remaining_writes, force);
  if ( ret < 0 ) {
    goto error_exit;
  }

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  for (i = 0; i < len; i++) {
    //prepare data
    tbuf[3] = list[i].command ;//command code
    memcpy(&tbuf[4], list[i].data, list[i].data_len);
    tlen = 4 + list[i].data_len;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Failed to send data...%d", __func__, i);
      break;
    }
    msleep(100);
    show_progress(i, len);
  }

  //check the status
  retry = MAX_RETRY;
  do {
    if (vr_ISL_polling_status(addr) > 0) {
      break;
    } else {
      retry--;
      sleep(2);
    }
  } while ( retry > 0 );

  if (retry == 0) {
    syslog(LOG_WARNING, "[%s] Failed to program the device", __func__);
    ret = -1;
  }
  printf("\n");

error_exit:
  return ret;
}

static int 
vr_ISL_hex_parser(char *image) {
  int ret = 0;
  FILE *fp = NULL;
  char tmp_buf[MAX_ISL_IMG_BUF] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int i = 0, j = 0;
  int data_cnt = 0;
  int byte_ret = 0;
  uint8_t addr = 0;
  uint8_t cnt = 0;
  uint8_t crc8 = 0;
  uint8_t crc8_check = 0;
  uint8_t data[CRC8_BUF_LEN] = {0};
  uint8_t data_end = 0;
#ifdef CONFIG_GRANDCANYON2
  int crc_record_idx = -1;   // which "00" data line holds the overall config CRC
#endif

  fp = fopen(image, "r");
  if (fp == NULL) {
    printf("Invalid file: %s\n", image);
    ret = -1;
    goto error_exit;
  }
 
  while (NULL != fgets(tmp_buf, tmp_len, fp)) {
    /*search for ISL manufacture ID*/
    if (start_with(tmp_buf, ISL_MFR_CODE)) {
      if (string_2_byte(&tmp_buf[6]) == CMD_ISL_VR_DEVICE_ID) {
        if (vr_list[vr_cnt].addr != 0x0) {
          vr_cnt++; //move on to the next addr if 4 VRs are included a file
        }
        byte_ret = string_2_byte(&tmp_buf[4]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].addr = byte_ret;
        }

        byte_ret = string_2_byte(&tmp_buf[14]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].devid[0] = byte_ret;
        }

        byte_ret = string_2_byte(&tmp_buf[12]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].devid[1] = byte_ret;
        }

        byte_ret = string_2_byte(&tmp_buf[10]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].devid[2] = byte_ret;
        }

        byte_ret = string_2_byte(&tmp_buf[8]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].devid[3] = byte_ret;
        }
        vr_list[vr_cnt].devid_len = 4;
#ifdef DEBUG
        printf("[Get] ID ");
        for (i = 0; i < vr_list[vr_cnt].devid_len; i++) {
          printf("%x ", vr_list[vr_cnt].devid[i]); 
        }
        printf(", Addr %x\n", vr_list[vr_cnt].addr);
#endif
      }
#ifdef CONFIG_GRANDCANYON2
      else if (string_2_byte(&tmp_buf[6]) == CMD_ISL_VR_DEV_REV) {
        // DEVREV record: determine Gen3 Legacy vs Production, which decides
        // where the overall configuration CRC is located in the file.
        int sw_rev = string_2_byte(&tmp_buf[8]);
        if (sw_rev < 0) {
          ret = -1;
          goto error_exit;
        }
        if (sw_rev < ISL_GEN3_SW_REV_MIN) {
          crc_record_idx = ISL_GEN3_LEGACY_CRC_IDX;
        } else {
          crc_record_idx = ISL_GEN3_PRODUCTION_CRC_IDX;
        }
      }
#endif
    } else if (start_with(tmp_buf, "00")) { // Not inittialized 
      //initialize the metadata
      j = 0;
      data_cnt = vr_list[vr_cnt].data_cnt;

      byte_ret = string_2_byte(&tmp_buf[2]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        cnt = byte_ret;
      }

      byte_ret = string_2_byte(&tmp_buf[4]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        addr = byte_ret;
      }
      data_end = cnt * 2;

      byte_ret = string_2_byte(&tmp_buf[data_end + 2]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        crc8 = byte_ret;
      }
      data[j++] = addr; //put addr to buffer to calculate crc8

      //printf("cnt %x, addr %x, data_end %x, crc8 %x\n", cnt, addr, data_end, crc8);
      if ( addr != vr_list[vr_cnt].addr ) {
        syslog(LOG_WARNING, "[%s] Failed to parse this line since the addr is not match. 0x%x != 0x%x\n", __func__, addr, vr_list[vr_cnt].addr);
        syslog(LOG_WARNING, "[%s] %s\n", __func__, tmp_buf);
        ret = -1;
        break;
      }

      //get the data
      for (i = 6; i <= data_end; i += 2, j++) {
        byte_ret = string_2_byte(&tmp_buf[i]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          data[j] = byte_ret;
        }
      }

      //calculate crc8
      crc8_check = 0;
      crc8_check = cal_crc8(crc8_check, data, cnt-1);

      if (crc8_check != crc8) {
        syslog(LOG_WARNING, "[%s] CRC8 is not match. Expected CRC8: 0x%x, Acutal CRC8: 0x%x\n", __func__, crc8, crc8_check);
        ret = -1;
        break;
      }

      vr_list[vr_cnt].pdata[data_cnt].command = data[1];
      vr_list[vr_cnt].pdata[data_cnt].data_len = cnt - 3;
      memcpy(vr_list[vr_cnt].pdata[data_cnt].data, &data[2], vr_list[vr_cnt].pdata[data_cnt].data_len);
#ifdef CONFIG_GRANDCANYON2
      // If this is the record holding the overall configuration CRC, capture it.
      if (data_cnt == crc_record_idx && vr_list[vr_cnt].pdata[data_cnt].data_len >= 4) {
        memcpy(&vr_list[vr_cnt].expected_crc, vr_list[vr_cnt].pdata[data_cnt].data, sizeof(uint32_t));
        printf("File CRC : %08X\n", vr_list[vr_cnt].expected_crc);
      }
#endif
#ifdef DEBUG
      printf(" cmd: %x, data_len: %x, data: ", vr_list[vr_cnt].pdata[data_cnt].command, vr_list[vr_cnt].pdata[data_cnt].data_len);
      for (i = 0; i < vr_list[vr_cnt].pdata[data_cnt].data_len; i++){
        printf("%x ", vr_list[vr_cnt].pdata[data_cnt].data[i]);
      }
      printf("\n");
#endif
      vr_list[vr_cnt].data_cnt++;
    } 
  }

#ifdef DEBUG
  printf("\n\n");
  for ( i = 0; i < vr_list[vr_cnt].data_cnt; i++) {
    printf(" cmd: %x, data_len: %x, data: ", vr_list[vr_cnt].pdata[i].command, vr_list[vr_cnt].pdata[i].data_len);
    for ( j = 0; j < vr_list[vr_cnt].pdata[i].data_len; j++)
      printf("%x ", vr_list[vr_cnt].pdata[i].data[j]);
    printf("\n");
  }
#endif

error_exit:
  if (fp != NULL) {
    fclose(fp);
  }

  return ret;
}

//CRC16 lookup table
unsigned int CRC16_LUT[CRC16_LUT_LEN] = {
  0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011, 0x8033, 0x0036, 0x003c, 0x8039,
  0x0028, 0x802d, 0x8027, 0x0022, 0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
  0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041, 0x80c3, 0x00c6, 0x00cc, 0x80c9,
  0x00d8, 0x80dd, 0x80d7, 0x00d2, 0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
  0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1, 0x8093, 0x0096, 0x009c, 0x8099,
  0x0088, 0x808d, 0x8087, 0x0082, 0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
  0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1, 0x01e0, 0x81e5, 0x81ef, 0x01ea,
  0x81fb, 0x01fe, 0x01f4, 0x81f1, 0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
  0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151, 0x8173, 0x0176, 0x017c, 0x8179,
  0x0168, 0x816d, 0x8167, 0x0162, 0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
  0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101, 0x8303, 0x0306, 0x030c, 0x8309,
  0x0318, 0x831d, 0x8317, 0x0312, 0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
  0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371, 0x8353, 0x0356, 0x035c, 0x8359,
  0x0348, 0x834d, 0x8347, 0x0342, 0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
  0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2, 0x83a3, 0x03a6, 0x03ac, 0x83a9,
  0x03b8, 0x83bd, 0x83b7, 0x03b2, 0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
  0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291, 0x82b3, 0x02b6, 0x02bc, 0x82b9,
  0x02a8, 0x82ad, 0x82a7, 0x02a2, 0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
  0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1, 0x8243, 0x0246, 0x024c, 0x8249,
  0x0258, 0x825d, 0x8257, 0x0252, 0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
  0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231, 0x8213, 0x0216, 0x021c, 0x8219,
  0x0208, 0x820d, 0x8207, 0x0202
};

// Refer Raw non-volatile memory (NVM) programming for "Catshark" family multiphase controllers Appendix III & Chapter 1
static int 
TI_cal_crc16(vr *dev) {
  uint8_t data[CRC16_BUF_LEN] = {0};
  uint8_t data_index = 0;
  uint8_t crc16_result[CRC16_RESULT_LEN] = {0};
  uint32_t crc16_accum = 0;
  uint32_t crc_shift = 0;
  uint8_t index = 0;
  int i = 0;

  for (i = 0; i < dev->data_cnt; i++) {
    if ( i == 0 ) {
      memcpy(crc16_result, &dev->pdata[i].data[9], 2);
      memcpy(&data[data_index], &dev->pdata[i].data[11], 21); //get data
      data_index += 21;
    } else if (i == 8) { //get the last data
      memcpy(&data[data_index], dev->pdata[i].data, 9);
      data_index += 9;
    } else {
      memcpy(&data[data_index], dev->pdata[i].data, 32);
      data_index += 32;
    }
  }

  for (i = 0; i < 254; i++) {
    index = ((crc16_accum >> 8) ^ data[i]) & 0xFF;
    crc_shift = (crc16_accum << 8) & 0xFFFF;
    crc16_accum = (crc_shift ^ CRC16_LUT[index]) & 0xFFFF;
  }

  return (crc16_accum == (crc16_result[1] << 8 | crc16_result[0])) ? 0 : -1;
}

static int
vr_TI_csv_parser(char *image) {
#define IC_DEVICE_ID "IC_DEVICE_ID"
#define BLOCK_READ "BlockRead"
#define BLOCK_WRITE "BlockWrite"
#ifdef CONFIG_GRANDCANYON2
#define DEVID_STR_LEN 1024
#else
#define DEVID_STR_LEN 128
#endif
  int ret = 0;
  FILE *fp = NULL;
  char *token = NULL;
  char tmp_buf[DEVID_STR_LEN] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int data_cnt = 0;
  int i = 0;
  uint8_t data_index = 0;
  uint8_t devid_2_token[DEVID_BYTE_CNT] = {2, 4, 6, 8, 10, 12};
  int byte_ret = 0;

  if ((fp = fopen(image, "r") ) == NULL) {
    printf("Invalid file: %s\n", image);
    ret = -1;
    goto error_exit;
  }

  while (NULL != fgets(tmp_buf, tmp_len, fp)) {
    if ((token = strstr(tmp_buf, IC_DEVICE_ID)) != NULL) { //get device id from the string
      token = strstr(token, "0x"); //get the pointer
      vr_list[vr_cnt].devid_len = 6;
      for (i = 0; i <= 5; i++) {
        byte_ret = string_2_byte(&token[devid_2_token[i]]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].devid[i] = byte_ret;
        }
      }
    } else if ( (token = strstr(tmp_buf, BLOCK_READ)) != NULL ) { //get block read
      if (vr_list[vr_cnt].addr != 0x00 ) {
        continue;
      }

      token = strstr(token, ",");
      byte_ret = string_2_byte(&token[3]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
       vr_list[vr_cnt].addr = byte_ret << 1;
      }
    } else if ( (token = strstr(tmp_buf, BLOCK_WRITE)) != NULL ) { //get block write
      token = strstr(token, ",");
      if (token == NULL) {
        continue;
      }
      byte_ret = string_2_byte(&token[8]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        vr_list[vr_cnt].pdata[data_cnt].command = byte_ret;
      }

      byte_ret = string_2_byte(&token[13]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        vr_list[vr_cnt].pdata[data_cnt].data_len = byte_ret;
      }

      int data_len = vr_list[vr_cnt].pdata[data_cnt].data_len;
      for (i = 0, data_index = 15; i < data_len; data_index+=2, i++) {
        byte_ret = string_2_byte(&token[data_index]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].pdata[data_cnt].data[i] = byte_ret;
        }
      } 
      vr_list[vr_cnt].data_cnt++;  
      data_cnt++;
    }
  }

#ifdef DEBUG
  int j = 0;
  printf("ID: ");
  for (i = 0; i < vr_list[vr_cnt].devid_len; i++) {
    printf("%02X ", vr_list[vr_cnt].devid[i]);
  }
  printf("\n Addr: %02X, Datalen:%d\n", vr_list[vr_cnt].addr, vr_list[vr_cnt].data_cnt);
  for (i = 0; i < vr_list[vr_cnt].data_cnt; i++){
    printf("[%d] %02X ", i, vr_list[vr_cnt].pdata[i].command);
    for (j = 0; j < vr_list[vr_cnt].pdata[i].data_len;j++) {
      printf("%02X ", vr_list[vr_cnt].pdata[i].data[j]);
    }
    printf("\n");
  }
#endif

  //calculate the checksum
  ret = TI_cal_crc16(&vr_list[vr_cnt]);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] CRC16 is error!", __func__);
  }
#ifdef CONFIG_GRANDCANYON2
  else {
    uint16_t file_crc = ((uint16_t)vr_list[vr_cnt].pdata[0].data[10] << 8) | vr_list[vr_cnt].pdata[0].data[9];
    printf("File CRC : %04X\n", file_crc);
  }
#endif

error_exit:
  if (fp != NULL) {
    fclose(fp);
  }

  return ret;
}

static int
vr_TI_program(vr *dev, uint8_t force) {
#define TI_USER_NVM_INDEX   0xF5
#define TI_USER_NVM_EXECUTE 0xF6
#define TI_NVM_CHECKSUM     0xF0
#define TI_NVM_INDEX_00     0x00
#define TI_NVM_DATA_BYTE_TLEN 0x21
#define TI_NVM_DATA_BYTE_RLEN 0x20

#define TI_CMD_RESTORE_USER   0x16  // RESTORE_USER_ALL
#define TI_BLOCK0_BYPASS_LEN  9     // bytes[0..8]: ID(6)+REV(2)+addr(1)
  int i = 0;
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
#ifndef CONFIG_GRANDCANYON2
  int check_cnt = 0;
#endif
  uint8_t addr = dev->addr;
  int len = dev->data_cnt;
  vr_data *list = dev->pdata;

#ifdef CONFIG_GRANDCANYON2
  // step 0 - Already-flashed guard (shared across vendors, aligned with
  // common/tps53688.c program_tps(); same NVM_CHECKSUM register used by the
  // post-write verification below, just read once before programming too).
  {
    uint8_t chk_tbuf[MAX_IPMB_BUFFER] = {0};
    uint8_t chk_rbuf[MAX_IPMB_BUFFER] = {0};
    uint8_t chk_tlen = 0, chk_rlen = 0;
    uint8_t dev_lo, dev_hi;

    chk_tbuf[0] = (VR_BUS << 1) + 1;
    chk_tbuf[1] = addr;
    chk_tbuf[2] = 0x04; // read 4 bytes to detect format (same as post-write verify)
    chk_tbuf[3] = CMD_TI_VR_NVM_CHECKSUM;
    chk_tlen = 4;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, chk_tbuf, chk_tlen, chk_rbuf, &chk_rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Failed to read NVM_CHECKSUM before programming", __func__);
      goto error_exit;
    }

    // Detect response format:
    //   BlockRead: rbuf[0]=count(0x02), rbuf[1]=CRC_lo, rbuf[2]=CRC_hi
    //   Direct:    rbuf[0]=CRC_lo,      rbuf[1]=CRC_hi
    if (chk_rlen == 3 && chk_rbuf[0] == 0x02) {
      dev_lo = chk_rbuf[1];
      dev_hi = chk_rbuf[2];
    } else {
      dev_lo = chk_rbuf[0];
      dev_hi = chk_rbuf[1];
    }

    uint32_t dev_crc = ((uint32_t)dev_hi << 8) | dev_lo;
    uint32_t exp_crc = ((uint32_t)list[0].data[10] << 8) | list[0].data[9];

    int chk = vr_already_flashed_check(dev_crc, exp_crc, force, "CRC", "%04X");
    if (chk < 0) {
      ret = -1;
      goto error_exit;
    }
  }

#endif

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt

  //step 1- Set page to 0x00 first
  tbuf[3] = TI_USER_NVM_INDEX;
  tbuf[4] = TI_NVM_INDEX_00;
  tlen = 5;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot initialize the page to 0x00!", __func__);
    goto error_exit;
  }

  //step 2 - program VR
  for (i = 0; i < len; i++) {
    // prepare data
    tbuf[3] = list[i].command ; //command code
    tbuf[4] = list[i].data_len; //counts
    memcpy(&tbuf[5], list[i].data, list[i].data_len);

#ifdef CONFIG_GRANDCANYON2
    if (i == 0) {
      // TI Datasheet 7.9.6 Step 4:
      // Replace Block 0 bytes[0..8] with 0xFF to bypass device validation:
      //   bytes[0..5] = IC_DEVICE_ID
      //   bytes[6..7] = IC_DEVICE_REV (file may differ from silicon rev)
      //   bytes[8]    = PMBus address
      // Verification is done via NVM_CHECKSUM (0xF4) after programming.
      memset(&tbuf[5], 0xFF, TI_BLOCK0_BYPASS_LEN);
    }
#endif

    tlen = 5 + list[i].data_len;

    // send it
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "[%s] Failed to send data...%d", __func__, i);
      break;
    }
    msleep(50);
  }

#ifdef CONFIG_GRANDCANYON2
  //step 3 (GC2) - verify via NVM_CHECKSUM (0xF4)
  // CSV declares "Perform_Read_Back_Validation: False":
  //   block readback is unreliable because bytes[0..8] were written as FF
  //   and device may auto-update fields after NVM store.
  // Expected CRC16: Block 0 data[9]=CRC_lo, data[10]=CRC_hi.
  msleep(300); // wait for NVM store (Datasheet 7.9.6 Step 13: 100ms min)

  // CSV declares "Reset_Device_When_Done: True"
  // Issue RESTORE_USER_ALL per Datasheet 7.9.6 Step 15
  memset(&tbuf[2], 0, sizeof(tbuf) - 2);
  tbuf[2] = 0x00;
  tbuf[3] = TI_CMD_RESTORE_USER;
  tlen = 4;
  rlen = 0;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    // non-fatal: device may have auto-restored after NVM store
    syslog(LOG_WARNING, "[%s] RESTORE_USER_ALL failed (non-fatal)", __func__);
  }
  msleep(100);

  memset(&tbuf[2], 0, sizeof(tbuf) - 2);
  tbuf[2] = 0x04; // read 4 bytes to detect format
  tbuf[3] = CMD_TI_VR_NVM_CHECKSUM;
  tlen = 4;
  rlen = 0;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Failed to read NVM_CHECKSUM", __func__);
    goto error_exit;
  }

#ifdef DEBUG
  printf("[TI VR 0x%02X] NVM_CHECKSUM raw: rlen=%d, %02X %02X %02X %02X\n",
         addr, rlen, rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
  printf("[TI VR 0x%02X] Expected CRC: lo=0x%02X hi=0x%02X (0x%02X%02X)\n",
         addr, list[0].data[9], list[0].data[10],
         list[0].data[10], list[0].data[9]);
#endif

  {
    uint8_t exp_lo = list[0].data[9];
    uint8_t exp_hi = list[0].data[10];
    uint8_t dev_lo, dev_hi;

    // Detect response format:
    //   BlockRead: rbuf[0]=count(0x02), rbuf[1]=CRC_lo, rbuf[2]=CRC_hi
    //   Direct:    rbuf[0]=CRC_lo,      rbuf[1]=CRC_hi
    if (rlen == 3 && rbuf[0] == 0x02) {
      dev_lo = rbuf[1];
      dev_hi = rbuf[2];
    } else {
      dev_lo = rbuf[0];
      dev_hi = rbuf[1];
    }

#ifdef DEBUG
    printf("[TI VR 0x%02X] CRC16: expected=0x%02X%02X, device=0x%02X%02X  %s\n",
           addr, exp_hi, exp_lo, dev_hi, dev_lo,
           (dev_lo == exp_lo && dev_hi == exp_hi) ? "MATCH" : "MISMATCH");
#endif

    if (dev_lo != exp_lo || dev_hi != exp_hi) {
      syslog(LOG_WARNING, "[%s] NVM CRC16 mismatch! expected=0x%02X%02X device=0x%02X%02X",
             __func__, exp_hi, exp_lo, dev_hi, dev_lo);
      ret = -1;
      goto error_exit;
    }
    ret = 0;
  }

#else  // !CONFIG_GRANDCANYON2
  //step 3 - verify data
  tbuf[3] = TI_USER_NVM_INDEX;
  tbuf[4] = TI_NVM_INDEX_00;
  tlen = 5;
  msleep(300);
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Cannot initialize the page to 0x00 again.!", __func__);
    goto error_exit;
  }

  tbuf[2] = TI_NVM_DATA_BYTE_TLEN;
  tbuf[3] = TI_USER_NVM_EXECUTE;
  tlen = 4;
  for ( i=0; i<len; i++ ) {
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "[%s] Failed to read data. index:%d", __func__, i);
    } else {
      if ( rbuf[0] == TI_NVM_DATA_BYTE_RLEN ) {
        memmove(rbuf, &rbuf[1], TI_NVM_DATA_BYTE_RLEN);
      } else {
        syslog(LOG_WARNING, "[%s] The count of data is incorrect. index:%d, data_len:%d", __func__, i, rbuf[0]);
        ret = -1;
        goto error_exit;
      }

      ret = memcmp(rbuf, list[i].data, list[i].data_len);
      if ( ret == 0 ) {
        check_cnt++;
      }
    }
    msleep(50);
  }

  if (check_cnt != len) {
    ret = -1;
  }
#endif // CONFIG_GRANDCANYON2

error_exit:
  return ret;
}

static int
vr_IFX_xsf_parser(char *image) {
#define CHECKSUM_FIELD "Checksum :"
#define ADDR_FIELD "i2c Address :"
#define DATA_START_TAG "[Config Data]"
#define DATA_END_TAG   "4000"
#define DATA_LEN_IN_LINE 0x11
#define MAX_TOKEN_LEN 32
  const uint16_t DEV_ID = 0x0279;
  int ret = 0;
  FILE *fp = NULL;
  char *token = NULL;
  char tmp_buf[MAX_IFX_IMG_BUF] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int data_cnt = 0;
  int i = 0;
  bool is_data = false;
  int token_size = 0;
  uint8_t page_addr = 0x0;
  uint8_t index_addr = 0x0;
  int byte_ret = 0;

  if ( (fp = fopen(image, "r") ) == NULL ) {
    printf("Invalid file: %s\n", image);
    ret = -1;
    goto error_exit;
  }

  while (NULL != fgets(tmp_buf, tmp_len, fp)) {
    if ((token = strstr(tmp_buf, CHECKSUM_FIELD)) != NULL) { //get the checksum
      //set DEVID manually
      memcpy(vr_list[vr_cnt].devid, (uint8_t *)&DEV_ID, 2);
      vr_list[vr_cnt].devid_len = 2;
    } else if ((token = strstr(tmp_buf, ADDR_FIELD)) != NULL) {
      token = strstr(tmp_buf, "0x");
      byte_ret = string_2_byte(&token[2]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        vr_list[vr_cnt].addr = byte_ret;
      }
    } else if ((token = strstr(tmp_buf, DATA_START_TAG)) != NULL) {
      is_data = true;
      continue;
    } else if (start_with(tmp_buf, DATA_END_TAG) > 0) {
      break;
    } else if (is_data == true) {
      char *token_list[MAX_TOKEN_LEN] = {NULL};
      token_size = split(token_list, tmp_buf, " ", MAX_TOKEN_LEN); //it should be 17 only
      page_addr = 0x0;
      index_addr = 0x0;

      if (token_size != DATA_LEN_IN_LINE) {
        printf("the len of token is not expected!\n");
        ret = -1;
        break;
      }

      byte_ret = string_2_byte(&token_list[0][0]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        page_addr = byte_ret;
      }

      byte_ret = string_2_byte(&token_list[0][2]);
      if (byte_ret < 0) {
        ret = -1;
        goto error_exit;
      } else {
        index_addr = byte_ret;
      }

      for (i = 1; i < DATA_LEN_IN_LINE; i++) {
        if (start_with(token_list[i], "----") > 0) {
          continue;
        }
        vr_list[vr_cnt].pdata[data_cnt].command = page_addr;
        vr_list[vr_cnt].pdata[data_cnt].data_len = 2; //write word
        vr_list[vr_cnt].pdata[data_cnt].data[0] = index_addr + i - 1; //put register offset to data[0]
        byte_ret = string_2_byte(&token_list[0][2]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].pdata[data_cnt].data[1] = byte_ret;
        }

        byte_ret = string_2_byte(&token_list[i][2]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].pdata[data_cnt].data[1] = byte_ret;
        }

        byte_ret = string_2_byte(&token_list[i][0]);
        if (byte_ret < 0) {
          ret = -1;
          goto error_exit;
        } else {
          vr_list[vr_cnt].pdata[data_cnt].data[2] = byte_ret;
        }
        data_cnt++;
      }
    }
  }
  vr_list[vr_cnt].data_cnt += data_cnt;

error_exit:
  if (fp != NULL) {
    fclose(fp);
  }

  return ret;
}

//Refer "AN001-XDPE122xx-V3.0_XDPE122xx Programming Guide" 9
static int
vr_IFX_program(vr *dev, uint8_t force) {
#define VR_CONF_REG    0x1A
#define VR_CLR_FAULT   0x03
#define REG1_STS_CHECK 0x01
#define REG2_STS_CHECK 0x0A
  int i = 0;
  int ret = 0;
  int fd = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t current_page = 0xff;
  uint8_t remaining_writes = 0x00;
  uint8_t addr = dev->addr;
  int len = dev->data_cnt;
  vr_data *list = dev->pdata;
  
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
      return -1;
    }
  }

  // get the remaining writes of the VR
  ret = bic_get_ifx_vr_remaining_writes(VR_BUS, addr, &remaining_writes);
  if ( ret < 0 ) {
    goto error_exit;
  }

  // check it
  ret = vr_remaining_writes_check(remaining_writes, force);
  if (ret < 0) {
    goto error_exit;
  }

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  //step 1 - write data to data register
  for (i = 0; i < len; i++) {
    if (list[i].command != current_page) {
      //set page
      tbuf[2] = 0x00; //read cnt
      tbuf[3] = VR_PAGE;
      tbuf[4] = list[i].command;
      tlen = 5;
      ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, list[i].command);
        goto error_exit;
      }
      current_page = list[i].command;
      msleep(10);//sleep 10ms and carry on to write the data
    }

    //write data
    memcpy(&tbuf[3], list[i].data, 3);
    tlen = 6;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Couldn't write data to page %02X and offset %02X", __func__, list[i].command, list[i].data[0]);
      goto error_exit;
    }
    msleep(100);
    show_progress(i, len);
  }
  printf("\n");

  //step 2 - upload data to config
  tbuf[3] = VR_PAGE;
  tbuf[4] = VR_PAGE32;
  tlen = 5;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[3] = VR_CONF_REG;
  tbuf[4] = 0xa1;
  tbuf[5] = 0x08;
  tlen = 6;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Couldn't write data to VR_CONF_REG", __func__);
    goto error_exit;
  }

  tbuf[3] = VR_CLR_FAULT;
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Couldn't sent CLR FAULT", __func__);
    goto error_exit;
  }

  tbuf[3] = 0x1d;
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Couldn't sent 0x1d", __func__);
    goto error_exit;
  }

  tbuf[3] = 0x24;
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Couldn't sent 0x1d", __func__);
    goto error_exit;
  }

  sleep(1); //wait for uploadprocess complete

  tbuf[3] = VR_CONF_REG;
  tbuf[4] = 0x00;
  tbuf[5] = 0x00;
  tlen = 6;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't write data to VR_CONF_REG", __func__);
    goto error_exit;
  }

  // step 3 - checking for a successful upload
  tbuf[3] = VR_PAGE;
  tbuf[4] = VR_PAGE60;
  tlen = 5;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[2] = 0x01; //read cnt
  tbuf[3] = 0x01; //sts reg1
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  if ((rbuf[0] & REG1_STS_CHECK) != 0) {
    ret = -1;
    printf("Failed to upload data. rbuf[0]=%02X from reg1\n", rbuf[0]);
    goto error_exit;
  }

  tbuf[3] = 0x02; //sts reg2
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  if ((rbuf[0] & REG2_STS_CHECK) != 0) {
    ret = -1;
    printf("Failed to upload data. rbuf[0]=%02X from reg2\n", rbuf[0]);
    printf("a defective bit is held or the mem space is full!\n");
    goto error_exit;
  }

error_exit:
  ret = flock(fd, LOCK_UN);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, SERVER_SENSOR_LOCK);
  }
  close(fd);
  remove(SERVER_SENSOR_LOCK);

  return ret;
}

#ifdef CONFIG_GRANDCANYON2

#define CRC32_POLY 0xEDB88320
#define SCRATCHPAD_ADDR 0x2005E000
#define PMBUS_STS_CML   0x7E
#define MFR_FW_CMD_DATA 0xFD
#define MFR_FW_CMD      0xFE
#define RPTR            0xCE
#define MFR_REG_WRITE   0xDE
#define REG_LOCK_PASSWORD 0x7F48680C
#define GET_CRC_CMD 0x2D
#define FW_RESET_CMD 0x0E
#define OTP_CONF_STO 0x11
#define OTP_FILE_INVD 0x12

// ===== Unlock/Lock Functions =====
static int
vr_XDPE152XX_unlock_reg(uint8_t addr) {
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00;
  tbuf[3] = 0xCB;  // IFX_MFR_DISABLE_SECURITY_ONCE
  tbuf[4] = 4;
  uint32_t password = REG_LOCK_PASSWORD;
  memcpy(&tbuf[5], (uint8_t *)&password, 4);
  tlen = 9;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, 
                         tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Failed to unlock registers (0xCB)", __func__);
    return ret;
  }

  msleep(10);
  
#ifdef DEBUG
  printf("[XDPE152XX] Registers unlocked\n");
#endif

  return 0;
}

static int
vr_XDPE152XX_lock_reg(uint8_t addr) {
  int ret = 0;
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00;
  tbuf[3] = 0xCB;
  tbuf[4] = 4;
  tbuf[5] = 0x01;
  tbuf[6] = 0x00;
  tbuf[7] = 0x00;
  tbuf[8] = 0x00;
  tlen = 9;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, 
                         tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Failed to lock registers", __func__);
    return ret;
  }

#ifdef DEBUG
  printf("[XDPE152XX] Registers locked\n");
#endif

  return 0;
}

// ===== CRC-32 Functions =====
// CRC32 (reflected/LSB-first) helpers
static inline uint32_t crc32_init(void) {
  return 0xFFFFFFFFu;
}

static inline uint32_t crc32_update_byte(uint32_t crc, uint8_t byte) {
  crc ^= byte;
  for (int b = 0; b < 8; ++b) {
    crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
  }
  return crc;
}

static inline uint32_t crc32_final(uint32_t crc) {
  return ~crc;
}

static uint32_t crc32_over_dwords_le(const uint32_t *data, int dw_count) {
  if (!data || dw_count <= 0) return 0;
  uint32_t crc = crc32_init();
  for (int i = 0; i < dw_count; ++i) {
    uint8_t b0 = (uint8_t)((data[i] >>  0) & 0xFF);
    uint8_t b1 = (uint8_t)((data[i] >>  8) & 0xFF);
    uint8_t b2 = (uint8_t)((data[i] >> 16) & 0xFF);
    uint8_t b3 = (uint8_t)((data[i] >> 24) & 0xFF);
    crc = crc32_update_byte(crc, b0);
    crc = crc32_update_byte(crc, b1);
    crc = crc32_update_byte(crc, b2);
    crc = crc32_update_byte(crc, b3);
  }
  return crc32_final(crc);
}

static int
check_xdpe152xx_image(struct xdpe152xx_config *config) {
  if (!config) return -1;

  uint32_t sum = 0;

  for (uint8_t i = 0; i < config->sect_cnt; i++) {
    struct config_sect *sect = &config->section[i];
    if (!sect) return -1;

    // A section must have at least: header(1) + size(1) + headerCRC(1) + dataCRC(1)
    if (sect->data_cnt < 4) {
      syslog(LOG_WARNING, "%s: Section %u too short (data_cnt=%u)", __func__, i, sect->data_cnt);
      return -1;
    }

    // 1) Header CRC (first two DWORDs)
    uint32_t header_crc_calc = crc32_over_dwords_le(&sect->data[0], 2);
    uint32_t header_crc_file = sect->data[2];
    if (header_crc_calc != header_crc_file) {
      syslog(LOG_WARNING, "%s: Header CRC mismatch in section %u: calc=0x%08X, file=0x%08X",
             __func__, i, header_crc_calc, header_crc_file);
      return -1;
    }

    // 2) Data CRC (data body = excluding header[0..2] and the final dataCRC)
    int data_body_dw = (int)sect->data_cnt - 4; // data[3..last-1]
    uint32_t data_crc_calc = 0;
    if (data_body_dw > 0) {
      data_crc_calc = crc32_over_dwords_le(&sect->data[3], data_body_dw);
    } else {
      // Empty data body; CRC(empty) is 0x00000000
      uint32_t crc = crc32_init();
      data_crc_calc = crc32_final(crc);
    }
    uint32_t data_crc_file = sect->data[sect->data_cnt - 1];
    if (data_crc_calc != data_crc_file) {
      syslog(LOG_WARNING, "%s: Data CRC mismatch in section %u: calc=0x%08X, file=0x%08X",
             __func__, i, data_crc_calc, data_crc_file);
      return -1;
    }

    // 3) File sum: exclude Trim (header code 0x02)
    if (sect->type != 0x02) {
      sum += header_crc_file;
      sum += data_crc_file;
    }
  }

  printf("File CRC : %08X\n", sum);
  if (sum != config->sum_exp) {
    syslog(LOG_WARNING, "%s: Checksum mismatched! Expected: 0x%08X, Actual: 0x%08X",
           __func__, config->sum_exp, sum);
    return -1;
  }

  return 0;
}

// ===== VR Communication Functions =====
int vr_xdpe152xx_get_crc(uint8_t addr, uint32_t *sum) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0, rlen = 0;
  int ret;

  if (!sum) return -1;

  // (Optional) RPTR initialization, if the platform requires it
  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00;
  tbuf[3] = 0x10;     // RPTR
  tbuf[4] = 0x00;
  tlen = 5;
  (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  // 0xFD: block write 4 bytes of zeros
  tbuf[2] = 0x00;
  tbuf[3] = MFR_FW_CMD_DATA; // 0xFD
  tbuf[4] = 0x04;            // count
  tbuf[5] = 0x00;
  tbuf[6] = 0x00;
  tbuf[7] = 0x00;
  tbuf[8] = 0x00;
  tlen = 9;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) return ret;

  // 0xFE: execute GET_CRC (0x2D)
  tbuf[2] = 0x00;
  tbuf[3] = MFR_FW_CMD;      // 0xFE
  tbuf[4] = GET_CRC_CMD;     // 0x2D
  tlen = 5;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) return ret;

  usleep(20000); // 20ms

  // Read 0xFD result
  tbuf[2] = 0x05;            // read count: want 1(len)+4(data)
  tbuf[3] = MFR_FW_CMD_DATA; // 0xFD
  tlen = 4;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) return ret;

  if (rlen < 5 || rbuf[0] != 4) {
    syslog(LOG_WARNING, "%s: invalid block length: rlen=%u, blk=%u", __func__, rlen, rbuf[0]);
    return -1;
  }
  memcpy(sum, &rbuf[1], 4);

  // (Optional) RPTR cleanup
  tbuf[2] = 0x00;
  tbuf[3] = 0x10;
  tbuf[4] = 0x80;
  tlen = 5;
  (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  return 0;
}

// ===== Parser Function =====
// Helpers for parsing
static inline uint32_t get_bits(uint32_t v, int start, int width) {
  if (width <= 0 || start < 0 || start >= 32) return 0;
  if (width > 32 - start) width = 32 - start;
  uint32_t mask = (width == 32) ? 0xFFFFFFFFU : ((1U << width) - 1U);
  return (v >> start) & mask;
}

static int ws_split(char *line, char *out[], int max_tokens) {
  int n = 0;
  char *p = line;

  // trim leading spaces/tabs
  while (*p == ' ' || *p == '\t') p++;

  while (*p && n < max_tokens) {
    // start of token
    out[n++] = p;
    // advance to next space/tab or end
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
    if (!*p) break;
    // terminate current token
    *p++ = '\0';
    // skip subsequent spaces/tabs
    while (*p == ' ' || *p == '\t') p++;
  }
  return n;
}

// ===== Parser Function (Rev D forced-address only) =====
static struct xdpe152xx_config*
vr_XDPE152XX_parse_file(char *image) {
  #define XDPE_DATA_START_TAG "[Configuration Data]"
  #define XDPE_DATA_END_TAG   "[End Configuration Data]"
  #define XDPE_DATA_COMMENT   "//"
  #define XDPE_CHECKSUM_FIELD "Checksum :"
  #define XDPE_PART_FIELD     "Part Number :"

  FILE *fp = NULL;
  char line[512];
  char *token = NULL;
  bool in_data = false;
  bool is_revD = false;     // Only allow XDPE152xx Rev D
  struct xdpe152xx_config *config = NULL;

  int sect_idx = -1;
  uint8_t current_type = 0x00;

  if (!image) {
    printf("ERROR: invalid file path (NULL)!\n");
    return NULL;
  }

  fp = fopen(image, "r");
  if (!fp) {
    printf("ERROR: invalid file path: %s\n", image);
    return NULL;
  }

  config = (struct xdpe152xx_config *)calloc(1, sizeof(struct xdpe152xx_config));
  if (!config) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    // Skip lines that start with // (including //XV0 ...)
    if (!strncmp(line, XDPE_DATA_COMMENT, strlen(XDPE_DATA_COMMENT))) {
      continue;
    }

    // Parse Part Number: only allow XDPE152xx Rev D
    if ((token = strstr(line, XDPE_PART_FIELD)) != NULL) {
      // Heuristic: contains "XDPE152" and has a 'D' near the end or around parentheses/whitespace
      if (strstr(line, "XDPE152") && strchr(line, 'D')) {
        is_revD = true;
      }
      continue;
    }

    // Parse Checksum (Configuration Checksum or the System Design File CRC32 will have: "Checksum : 0xXXXXXXXX")
    if ((token = strstr(line, XDPE_CHECKSUM_FIELD)) != NULL) {
      token = strstr(token, "0x");
      if (token) {
        config->sum_exp = (uint32_t)strtoul(token, NULL, 16);
      }
      continue;
    }

    // Check [Configuration Data] / [End Configuration Data]
    if (!strncmp(line, XDPE_DATA_START_TAG, strlen(XDPE_DATA_START_TAG))) {
      in_data = true;
      continue;
    }
    if (!strncmp(line, XDPE_DATA_END_TAG, strlen(XDPE_DATA_END_TAG))) {
      break;
    }

    // Only parse within the data section
    if (!in_data) {
      continue;
    }

    // Remove trailing newline(s)
    {
      size_t L = strlen(line);
      while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) line[--L] = '\0';
    }

    // Skip empty lines
    {
      char *p = line;
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '\0') continue;
    }

    // Tokenize (offset + up to 4 DWORDs)
    char *tok[8] = {0};
    int ntok = ws_split(line, tok, 5); // offset + 4 dwords
    if (ntok <= 0) continue;

    // The first token should be a 3-hex-digit offset (e.g., 000, 010, 1D0)
    // Use strtol to allow variable length
    uint16_t offset = (uint16_t)strtol(tok[0], NULL, 16);

    // Parse the DWORDs on this line
    uint32_t line_dw[4] = {0};
    for (int i = 1; i < ntok; i++) {
      line_dw[i-1] = (uint32_t)strtoul(tok[i], NULL, 16);
    }

    // Detect a new section: offset is 0x000 and at least 1 dword present
    if (offset == 0x000 && ntok >= 2) {
      uint32_t hdr = line_dw[0];
      current_type = (uint8_t)(hdr & 0xFF); // The LSB of the header code is the type

      // Skip storing data for Trim (0x02) sections; still update the section index to keep consistency
      if ((++sect_idx) >= MAX_SECT_NUM) {
        syslog(LOG_WARNING, "%s: Exceed max section number", __func__);
        fclose(fp);
        free(config);
        return NULL;
      }
      config->section[sect_idx].type = current_type;
      config->section[sect_idx].data_cnt = 0;
      config->sect_cnt = sect_idx + 1;
    }

    // Store this line's DWORDs into the current section (store Trim as well for complete validation later)
    if (sect_idx >= 0) {
      for (int i = 1; i < ntok; i++) {
        if (config->section[sect_idx].data_cnt >= MAX_SECT_DATA_NUM) {
          syslog(LOG_WARNING, "%s: Exceed max data count", __func__);
          fclose(fp);
          free(config);
          return NULL;
        }
        config->section[sect_idx].data[config->section[sect_idx].data_cnt++] = line_dw[i-1];
        config->total_cnt++;
      }
    }
  } // while fgets

  fclose(fp);

  // Basic field checks
  if (!config->sum_exp) {
    printf("ERROR: Configuration Checksum not found!\n");
    free(config);
    return NULL;
  }
  if (!is_revD) {
    printf("ERROR: Only XDPE152xx Rev D is supported by current parser address logic.\n");
    free(config);
    return NULL;
  }

  // Find the XV0 Config section (header type 0x04)
  const struct config_sect *cfg_sect = NULL;
  for (int s = 0; s < config->sect_cnt; s++) {
    if (config->section[s].type == 0x04) // Config
    {
      cfg_sect = &config->section[s];
      break;
    }
  }
  if (!cfg_sect) {
    printf("ERROR: XV0 Config section not found.\n");
    free(config);
    return NULL;
  }

  // Directly index into //XV0 Config to get row 1D0's DW3:
  // row = 0x1D (=29), DW3 → index = row*4 + 3 = 119
  const int row = 0x1D;
  const int dw  = 3;
  const int idx = row * 4 + dw;
  if (cfg_sect->data_cnt <= idx) {
    printf("ERROR: Failed to determine forced PMBus address from image (row 1D0/DW3 missing or invalid). "
           "Need index %d, but data_cnt=%d\n", idx, cfg_sect->data_cnt);
    free(config);
    return NULL;
  }

  uint32_t dw3 = cfg_sect->data[idx];

  // Extract Addr_base_force_en(bit23) and Addr_base_value(bits[22:16])
  uint8_t addr_base_force_en = (uint8_t)get_bits(dw3, 23, 1);
  uint8_t addr_base_value    = (uint8_t)get_bits(dw3, 16, 7);

  if (addr_base_force_en == 0) {
    printf("ERROR: Addr_base_force_en=0 (not forced). XVcode addressing disabled by policy.\n");
    free(config);
    return NULL;
  }

  // Forced address: 7-bit = addr_base_value; 8-bit write = <<1
  uint8_t addr7 = (uint8_t)(addr_base_value & 0x7F);
  config->addr = (uint8_t)(addr7 << 1); // 8-bit write address

  return config;
}

// ===== Program Function =====
// Program per section
static int vr_XDPE152XX_program(uint8_t addr, struct xdpe152xx_config *config, uint8_t force) {
  if (!config) return -1;

  int ret = 0;
  int fd = -1;  // lock file fd
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t tlen = 0, rlen = 0;

  uint8_t remain = 0;
  uint32_t crc_now = 0;
  int prog_cnt = 0;
  int upload_total_cnt = 0;
  int did_global_invalidate = 0;

  // 0) Stop monitor: acquire VR sensor polling lock to prevent polling from changing PAGE
  fd = open(SERVER_SENSOR_LOCK, O_CREAT | O_RDWR, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open lock file %s failed (%d)", __func__, SERVER_SENSOR_LOCK, errno);
    return -1;
  }
  ret = flock(fd, LOCK_EX | LOCK_NB);
  if (ret != 0) {
    if (errno == EWOULDBLOCK) {
      syslog(LOG_WARNING, "%s: failed to lock VR sensor reading (locked by others)", __func__);
      // Mimic your ifx example: if lock cannot be obtained, remove the lock file and return
      remove(SERVER_SENSOR_LOCK);
      close(fd);
      return -1;
    } else {
      syslog(LOG_WARNING, "%s: flock failed (%d)", __func__, errno);
      close(fd);
      return -1;
    }
  }

  // Calculate the total number of DWORDs to actually program (skip Trim)
  for (int i = 0; i < config->sect_cnt; i++) {
    const struct config_sect *s = &config->section[i];
    if (!s) continue;
    uint8_t header_code = (uint8_t)(s->data[0] & 0xFF);
    if (header_code == 0x02) continue; // Trim
    upload_total_cnt += s->data_cnt;
  }
  if (upload_total_cnt == 0) {
    printf("Nothing to program (no non-Trim sections)\n");
    return 0;
  }

  // 1) Already-flashed guard (aligned with common/xdpe152xx.c program_xdpe152xx(),
  //    which checks CRC before remaining writes; also matches ISL/TI ordering)
  ret = vr_xdpe152xx_get_crc(addr, &crc_now);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: failed to read current CRC before programming", __func__);
    goto cleanup_unlock;
  }
  {
    int chk = vr_already_flashed_check(crc_now, config->sum_exp, force, "Checksum", "%08X");
    if (chk < 0) {
      ret = -1;               // blocked = failure, no longer misreported as success
      goto cleanup_unlock;
    }
  }

  // 2) Get remaining write count
  ret = bic_get_ifx_vr_remaining_writes_mfr(VR_BUS, addr, &remain);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: failed to get remaining writes", __func__);
    goto cleanup_unlock;
  }
  ret = vr_remaining_writes_check(remain, force);
  if (ret < 0) {
    goto cleanup_unlock;
  }

  // 3) Unlock registers
  ret = vr_XDPE152XX_unlock_reg(addr);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: unlock failed", __func__);
    goto cleanup_unlock;
  }

  // 4) Global invalidate (FE/FE/00/00 -> 0x12); if it fails, do per-section surgical invalidate later
  // Bridge init before 0xFD
  tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
  tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
  (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  // 0xFD: [FE FE 00 00]
  tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD_DATA; tbuf[4] = 0x04;
  tbuf[5] = 0xFE; tbuf[6] = 0xFE; tbuf[7] = 0x00; tbuf[8] = 0x00; tlen = 9;
  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  // Bridge init before 0xFE
  tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
  tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
  (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

  // 0xFE: 0x12
  if (ret == 0) {
    tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD; tbuf[4] = OTP_FILE_INVD; tlen = 5;
    ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  }
  if (ret == 0) {
    did_global_invalidate = 1;
    msleep(500);
  } else {
    syslog(LOG_WARNING, "%s: global invalidate not supported or failed, will invalidate per-section", __func__);
    did_global_invalidate = 0;
  }

  // 5) Upload each section
  for (int i = 0; i < config->sect_cnt; i++) {
    const struct config_sect *s = &config->section[i];
    if (!s) { ret = -1; break; }

    uint32_t hdr        = s->data[0];
    uint8_t  header_code= (uint8_t)(hdr & 0xFF);
    uint8_t  xvcode     = (uint8_t)((hdr >> 8) & 0xFF);
    if (header_code == 0x02) { // Trim
      continue;
    }

    // Section size (low 16 bits of the second DWORD), needed for OTP_CONF_STO.
    uint16_t sec_size = (uint16_t)(s->data[1] & 0xFFFF);

#ifdef DEBUG
    printf("[DEBUG] sec=%d hc=0x%02X xv=%u size=%u\n", i, header_code, xvcode, sec_size);
#endif

    // Per AN001-XDPE1x2xx sec 6.5: on CML Other Memory Fault, re-invalidate
    // and rewrite the section rather than aborting (OTP writes are permanent,
    // so a failed attempt still consumes write budget while leaving a
    // corrupted/unrecognized section).
    int section_ok = 0;
    for (int attempt = 0; attempt < 5 && !section_ok; attempt++) {
      if (attempt > 0) {
        printf("WARNING: section %d upload failed, retrying (attempt %d/5)...\n", i, attempt + 1);
        syslog(LOG_WARNING, "%s: retrying section (sec=%d, hc=0x%02X xv=%u)", __func__, i, header_code, xvcode);

        // Re-invalidate before rewriting; a failed store may have left an
        // incomplete/corrupted header in place (AN001 sec 6.2).
        tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
        tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
        (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

        tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD_DATA; tbuf[4] = 0x04;
        tbuf[5] = header_code; tbuf[6] = xvcode; tbuf[7] = 0x00; tbuf[8] = 0x00; tlen = 9;
        (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

        tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
        tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
        (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

        tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD; tbuf[4] = OTP_FILE_INVD; tlen = 5;
        (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
        msleep(10);
      }

      // Clear STATUS_CML bit0
      tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr;
      tbuf[2] = 0x00; tbuf[3] = PMBUS_STS_CML; tbuf[4] = 0x01; tlen = 5;
      ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: clear STATUS_CML failed (ret=%d)\n", i, attempt + 1, ret);
#endif
        syslog(LOG_WARNING, "%s: clear STATUS_CML failed", __func__);
        goto program_fail;
      }

      // If global invalidate didn't succeed, surgically invalidate this section (header_code + XVcode)
      if (!did_global_invalidate && attempt == 0) {
        // Bridge init before 0xFD
        tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
        tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
        (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

        // 0xFD: [header_code, xvcode, 0, 0]
        tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD_DATA; tbuf[4] = 0x04;
        tbuf[5] = header_code; tbuf[6] = xvcode; tbuf[7] = 0x00; tbuf[8] = 0x00; tlen = 9;
        ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
        if (ret < 0) {
#ifdef DEBUG
          printf("[DEBUG] sec=%d attempt=%d: surgical invalidate (0xFD) failed (ret=%d)\n", i, attempt + 1, ret);
#endif
          syslog(LOG_WARNING, "%s: surgical invalidate data failed (hc=0x%02X xv=%u)", __func__, header_code, xvcode);
          goto program_fail;
        }

        // Bridge init before 0xFE
        tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
        tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
        (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

        // 0xFE: 0x12
        tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD; tbuf[4] = OTP_FILE_INVD; tlen = 5;
        ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
        if (ret < 0) {
#ifdef DEBUG
          printf("[DEBUG] sec=%d attempt=%d: execute OTP_FILE_INVD failed (ret=%d)\n", i, attempt + 1, ret);
#endif
          syslog(LOG_WARNING, "%s: execute OTP_FILE_INVD failed (hc=0x%02X xv=%u)", __func__, header_code, xvcode);
          goto program_fail;
        }
        msleep(10);
      }

      // At the start of each section: reset scratchpad start address to 0x2005E000
      tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
      tbuf[3] = RPTR; tbuf[4] = 0x04;
      tbuf[5] = 0x00; tbuf[6] = 0xE0; tbuf[7] = 0x05; tbuf[8] = 0x20; tlen = 9;
      ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: set scratchpad address failed (ret=%d)\n", i, attempt + 1, ret);
#endif
        syslog(LOG_WARNING, "%s: set scratchpad address failed", __func__);
        goto program_fail;
      }
      msleep(10);

      // Stream all DWORDs of this section to the scratchpad
      for (int k = 0; k < s->data_cnt; k++) {
        tbuf[0] = (VR_BUS << 1) + 1;
        tbuf[1] = addr;
        tbuf[2] = 0x00;                // read cnt
        tbuf[3] = MFR_REG_WRITE;       // 0xDE
        tbuf[4] = 0x04;                // block count
        memcpy(&tbuf[5], &s->data[k], 4);  // little endian
        tlen = 9;
        ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
        if (ret < 0) {
#ifdef DEBUG
          printf("[DEBUG] sec=%d attempt=%d: write data failed at dword %d/%d (ret=%d)\n",
                 i, attempt + 1, k, s->data_cnt, ret);
#endif
          syslog(LOG_WARNING, "%s: write data failed (sec=%d, dword=%d, hc=0x%02X xv=%u)", __func__, i, k, header_code, xvcode);
          goto program_fail;
        }
        msleep(10);  // VR_WRITE_DELAY
      }

      // Bridge init before 0xFD
      tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
      tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
      (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

      // 0xFD: [size_lo, size_hi, 0, 0]
      tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD_DATA; tbuf[4] = 0x04;
      tbuf[5] = (uint8_t)(sec_size & 0xFF);
      tbuf[6] = (uint8_t)((sec_size >> 8) & 0xFF);
      tbuf[7] = 0x00; tbuf[8] = 0x00; tlen = 9;
      ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: write section size failed (size=%u, ret=%d)\n", i, attempt + 1, sec_size, ret);
#endif
        syslog(LOG_WARNING, "%s: write section size failed (sec=%d, size=%u)", __func__, i, sec_size);
        goto program_fail;
      }

      // Bridge init before 0xFE
      tbuf[0] = (VR_BUS << 1) + 1; tbuf[1] = addr; tbuf[2] = 0x00;
      tbuf[3] = 0x10; tbuf[4] = 0x00; tlen = 5;
      (void)bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);

      // 0xFE: 0x11 (OTP_CONF_STO to commit this section)
      tbuf[2] = 0x00; tbuf[3] = MFR_FW_CMD; tbuf[4] = OTP_CONF_STO; tlen = 5;
      ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: execute OTP_CONF_STO failed (ret=%d)\n", i, attempt + 1, ret);
#endif
        syslog(LOG_WARNING, "%s: execute OTP_CONF_STO failed (sec=%d)", __func__, i);
        goto program_fail;
      }

      // Soak: based on section size (2 ms/byte, at least 200 ms)
      {
        int loops = (sec_size / 50) + 2;
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: soaking ~%d ms\n", i, attempt + 1, loops * 100);
#endif
        for (int sloop = 0; sloop < loops; sloop++) {
          msleep(100);
        }
      }

      // Check STATUS_CML bit0
      tbuf[0] = (VR_BUS << 1) + 1;
      tbuf[1] = addr;
      tbuf[2] = 0x01;                  // read 1 byte
      tbuf[3] = PMBUS_STS_CML;         // 0x7E
      tlen = 4;
      ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: read STATUS_CML failed (ret=%d)\n", i, attempt + 1, ret);
#endif
        syslog(LOG_WARNING, "%s: read STATUS_CML failed after section (sec=%d)", __func__, i);
        goto program_fail;
      }
      if (rbuf[0] & 0x01) {
#ifdef DEBUG
        printf("[DEBUG] sec=%d attempt=%d: CML fault, STATUS_CML=0x%02X\n", i, attempt + 1, rbuf[0]);
#endif
        syslog(LOG_WARNING, "%s: CML Other Memory Fault after section (sec=%d, sts=0x%02X, attempt=%d)",
               __func__, i, rbuf[0], attempt + 1);
        continue; // retry this section (or fall through to failure below if out of attempts)
      }

#ifdef DEBUG
      printf("[DEBUG] sec=%d attempt=%d: OK, STATUS_CML=0x%02X\n", i, attempt + 1, rbuf[0]);
#endif
      section_ok = 1;
    }

    if (!section_ok) {
      syslog(LOG_WARNING, "%s: section %d (hc=0x%02X xv=%u) failed after 2 attempts, giving up",
             __func__, i, header_code, xvcode);
      ret = -1;
      goto program_fail;
    }

    // Progress
    prog_cnt += s->data_cnt;
    printf("\rupdated: %d %%  ", (prog_cnt * 100) / upload_total_cnt);
    fflush(stdout);
  }

  printf("\rupdated: 100 %%  \n");
  ret = 0;
  goto cleanup_lockback;

program_fail:
  printf("\n");

cleanup_lockback:
  // Attempt to re-lock (per your existing flow)
  vr_XDPE152XX_lock_reg(addr);

cleanup_unlock:
  // Release monitoring lock
  if (fd >= 0) {
    if (flock(fd, LOCK_UN) == -1) {
      syslog(LOG_WARNING, "%s: failed to unlock %s", __func__, SERVER_SENSOR_LOCK);
    }
    close(fd);
    remove(SERVER_SENSOR_LOCK);
  }

  return ret;
}

// ===== Device Detection =====
static int
vr_detect_device_type(uint8_t addr, uint8_t *rbuf, uint8_t rlen) {
  int vendor_type = VR_UNKNOWN;
  
  switch (rlen) {
    case 2: {
      uint8_t product_id = rbuf[1];
      uint8_t revision = rbuf[0];
      
      switch (product_id) {
        case XDPE15254_PRODUCT_ID:
        case XDPE15284_PRODUCT_ID:
        case XDPE152C4_PRODUCT_ID:
          vendor_type = VR_XDPE152XX;
          printf("Detected XDPE152xx (Product ID: 0x%02X, Rev: 0x%02X)\n", 
                 product_id, revision);
          break;
        default:
          vendor_type = VR_IFX;
          printf("Detected other Infineon VR (Product ID: 0x%02X)\n", product_id);
          break;
      }
      break;
    }
    
    case 6:
      vendor_type = VR_TI;
      printf("Detected TI VR\n");
      break;
      
    case 4:
    default:
      vendor_type = VR_ISL;
      printf("Detected Renesas/ISL VR\n");
      break;
  }
  
  return vendor_type;
}

#endif // CONFIG_GRANDCANYON2

struct dev_table {
  uint8_t addr;
  char *dev_name;
} 
#ifdef CONFIG_GRANDCANYON2
dev_list[] = {
  {PVCCIN_FIVRA_ADDR,    "PVCCIN_FIVRA"},
  {PVCCD_HV_ADDR,    "PVCCD_HV"},
  {PVCCINFAON_ADDR, "PVCCINFAON"},
};
#else
dev_list[] = {
  {VCCIN_ADDR,    "VCCIN/VSA"},
  {VCCIO_ADDR,    "VCCIO"},
  {VDDQ_AB_ADDR, "VDDQ_AB"},
  {VDDQ_DE_ADDR, "VDDQ_DE"},
};
#endif

int dev_table_size = (sizeof(dev_list)/sizeof(struct dev_table));

static struct tool {
  uint8_t vendor;
  int (*parser)(char *);
  int (*program)(vr *, uint8_t);
} vr_tool[] = {
  {VR_ISL, vr_ISL_hex_parser, vr_ISL_program},
  {VR_TI , vr_TI_csv_parser , vr_TI_program},
  {VR_IFX, vr_IFX_xsf_parser, vr_IFX_program},
#ifdef CONFIG_GRANDCANYON2
  {VR_XDPE152XX, NULL, NULL},
#endif
};

// ===== Helper Function =====
static __attribute__((unused)) const char *
get_vr_name(uint8_t addr) {
  int i = 0;

  for (i = 0; i< dev_table_size; i++) {
    if (addr == dev_list[i].addr) {
      return dev_list[i].dev_name;
    }
  }

  return "Unknown VR component";
}

#ifdef CONFIG_GRANDCANYON2
static int addr_in_dev_list(uint8_t addr) {
  for (int i = 0; i < dev_table_size; i++) {
    if (dev_list[i].addr == addr) {
      return 1; // found
    }
  }
  return 0; // not found
}

int update_bic_vr(char *image, uint8_t force) {
  int ret = -1;
  int i = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rlen = 0;
  uint8_t sel_vendor = VR_UNKNOWN;

  if (!image) {
    printf("Invalid parameter: image is NULL\n");
    return -1;
  }

  // Step 1: Read the Device ID from any device in dev_list
  do {
    ret = bic_get_vr_device_id(rbuf, &rlen, VR_BUS, dev_list[i].addr);
    if (ret == 0 && rlen <= TI_DEVID_LEN) { // the longest length of dev id
      break;
    }
  } while (i++ < dev_table_size);

  if (i == dev_table_size) {
    printf("Couldn't get the devid from VRs\n");
    goto error_exit;
  }

  // Use vr_detect_device_type to further distinguish (especially IFX vs XDPE152xx)
  sel_vendor = vr_detect_device_type(dev_list[i].addr, rbuf, rlen);
  if (sel_vendor == VR_UNKNOWN) {
    printf("Unknown VR type at 0x%02X (devid_len=%u)\n", dev_list[i].addr, rlen);
    goto error_exit;
  }
  printf("VR vendor=%s (devid_len=%u) at 0x%02X\n",
         (sel_vendor == VR_XDPE152XX) ? "XDPE152XX" :
         (sel_vendor == VR_IFX) ? "IFX" :
         (sel_vendor == VR_TI) ? "TI" : "ISL",
         rlen, dev_list[i].addr);

  // Step 2/3/4/5: Branch by vendor
  if (sel_vendor == VR_XDPE152XX) {
    // Step 2: XDPE-specific parser
    struct xdpe152xx_config *xdpe_config = vr_XDPE152XX_parse_file(image);
    if (!xdpe_config) {
      printf("Cannot parse the XDPE152xx configuration file!\n");
      goto error_exit;
    }

    // Step 3: File integrity validation
    ret = check_xdpe152xx_image(xdpe_config);
    if (ret < 0) {
      printf("Configuration file validation failed!\n");
      free(xdpe_config);
      goto error_exit;
    }

    if (xdpe_config->addr == 0) {
      printf("ERROR: Parsed image does not provide a forced PMBus address.\n");
      free(xdpe_config);
      goto error_exit;
    }

    // Step 4: Ensure the address extracted by the parser is in dev_list (address is 8-bit write)
    if (!addr_in_dev_list(xdpe_config->addr)) {
      printf("ERROR: Image PMBus write address 0x%02X not in dev_list\n", xdpe_config->addr);
      free(xdpe_config);
      goto error_exit;
    }

    printf("Update VR: %s\n", get_vr_name(xdpe_config->addr));

    // Step 5: Update XDPE
    ret = vr_XDPE152XX_program(xdpe_config->addr, xdpe_config, force);
    if (ret < 0) {
      printf("ERROR: VR Firmware update fail!\n");
      free(xdpe_config);
      goto error_exit;
    }

    printf("Please do power cycle to reset VR to reload configuration\n");

    free(xdpe_config);
    return 0;
  }

  // Other vendors (ISL/TI/IFX) use vr_tool's parser/program
  ret = vr_tool[sel_vendor].parser(image);
  if (ret < 0) {
    printf("Cannot parse the file!\n");
    goto error_exit;
  }

  // Basic data presence check
  for (int k = 0; k < vr_cnt + 1; k++) {
    if (vr_list[k].data_cnt == 0 || vr_list[k].addr == 0 || vr_list[k].devid_len == 0) {
      printf("data, addr, or devid_len is not caught!\n");
      ret = -1;
      goto error_exit;
    }
  }

  // Step 4: Ensure the address extracted by the parser appears in dev_list
  if (!addr_in_dev_list(vr_list[0].addr)) {
    printf("ERROR: Image VR address (0x%02X) not in dev_list\n", vr_list[0].addr);
    ret = -1;
    goto error_exit;
  }

  // More rigorous: re-read and compare the Device ID on the actual target (the address provided by the parser)
  {
    uint8_t abuf[MAX_IPMB_BUFFER] = {0};
    uint8_t alen = 0;
    ret = bic_get_vr_device_id(abuf, &alen, VR_BUS, vr_list[0].addr);
    if (ret < 0 || alen == 0) {
      printf("Couldn't get the devid from target VR at 0x%02X\n", vr_list[0].addr);
      goto error_exit;
    }
    if (vr_list[0].devid_len != alen || memcmp(vr_list[0].devid, abuf, alen) != 0) {
      printf("Device ID is not match on target 0x%02X!\n", vr_list[0].addr);
      printf(" Expected ID: ");
      for (int j = 0; j < vr_list[0].devid_len; j++) printf("%02X ", vr_list[0].devid[j]);
      printf("\n Actual ID: ");
      for (int j = 0; j < alen; j++) printf("%02X ", abuf[j]);
      printf("\n");
      ret = -1;
      goto error_exit;
    }
  }

  // Step 5: Perform the update
  printf("Update VR: %s\n", get_vr_name(vr_list[0].addr));
  ret = vr_tool[sel_vendor].program(&vr_list[0], force);
  if (ret < 0) {
    printf("ERROR: VR Firmware update fail!\n");
    goto error_exit;
  }

  return 0;

error_exit:
  return ret;
}
#else
int 
update_bic_vr(char *image, uint8_t force) {
  int ret = 0;
  int i = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t rlen = 0;
  uint8_t sel_vendor = 0;

  //step 1 - read the dev id of one of them.
  do {
    ret = bic_get_vr_device_id(rbuf, &rlen, VR_BUS, dev_list[i].addr);
    if ( ret == 0 && rlen <= TI_DEVID_LEN) { /*the longest length of dev id*/
      break;
    } 
  } while (i++ < dev_table_size);

  if (i == dev_table_size) {
    printf("Couldn't get the devid from VRs\n");
    goto error_exit;
  }

  //step 2 - parse the image file.
  if (rlen == IFX_DEVID_LEN)  {
    sel_vendor = VR_IFX;
  } else if (rlen == TI_DEVID_LEN) {
    sel_vendor = VR_TI;
  } else {
    sel_vendor = VR_ISL;
  }

  printf("VR vendor=%s(%x.%x) \n", (sel_vendor == VR_IFX) ? "IFX" : (sel_vendor == VR_TI) ? "TI" : "ISL", rlen, sel_vendor);

  ret = vr_tool[sel_vendor].parser(image);
  if (ret < 0) {
    printf("Cannot parse the file!\n");
    goto error_exit;
  }

  //step 2.5 - check if data is existed.
  //data_cnt, addr, and devid_len cannot be 0.
  for (i = 0; i < vr_cnt + 1; i++) {
    if (vr_list[i].data_cnt == 0 || vr_list[i].addr == 0 || vr_list[i].devid_len == 0) {
      printf("data, addr, or devid_len is not caught!\n");
      ret = -1;
      goto error_exit;
    }
  }

  //step 3 - check DEVID
  if (memcmp(vr_list[0].devid, rbuf, vr_list[0].devid_len) != 0) {
    printf("Device ID is not match!\n");
    printf(" Expected ID: ");
    for (i = 0 ; i < vr_list[0].devid_len; i++) {
      printf("%02X ", vr_list[0].devid[i]);
    }
    printf("\n");
    printf(" Actual ID: ");
    for (i = 0 ; i < vr_list[0].devid_len; i++) {
      printf("%02X ", rbuf[i]);
    }
    printf("\n");
    ret = -1;
    goto error_exit;
  }

  //step 4 - program
  //For now, we only support to be input 1 file.
  printf("Update %s...", get_vr_name(vr_list[0].addr));
  ret = vr_tool[sel_vendor].program(&vr_list[0], force);

error_exit:
  return ret;
}
#endif
