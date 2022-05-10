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
#include "bic_vr_fwupdate.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"

//#define DEBUG

/****************************/
/*       VR fw update       */
/****************************/
#define WARNING_REMAINING_WRITES 3
#define MAX_RETRY 3
#define VR_SB_BUS 0x4
#define VR_2OU_BUS 0x1
#define REVD_MAX_REVISION_NUM 5
#define REVISION_D 0
#define REVISION_F 1

typedef struct {
  uint8_t command;
  uint8_t data_len;
  uint8_t data[64];
} vr_data;

typedef struct {
  uint8_t addr;
  uint8_t bus;
  uint8_t intf;
  uint8_t devid[6];
  uint8_t devid_len;
  uint8_t dev_rev;
  int data_cnt;
  vr_data pdata[2048];
} vr;

static int vr_cnt = 0;

//4 VRs are on the server board
static vr vr_list[4] = {0};

static struct {
  uint8_t slot_id;
  uint8_t intf;
} dis_bic_vr = {0, NONE_INTF};

__attribute__((destructor))
static void en_bic_vr_monitor(void) {
  if ( dis_bic_vr.slot_id ) {
    bic_disable_sensor_monitor(dis_bic_vr.slot_id, 0, dis_bic_vr.intf);
  }
}

static uint8_t
cal_crc8(uint8_t crc, uint8_t const *data, uint8_t len)
{
  uint8_t const crc8_table[] =
  { 0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3};

  if ( NULL == data )
  {
    return 0;
  }

  crc &= 0xff;
  uint8_t const *end = data + len;

  while ( data < end )
  {
    crc = crc8_table[crc ^ *data++];
  }

  return crc;
}

static uint8_t
char_to_hex(char c) {
  if (c >= 'A')
    return c - 'A' + 10;
  else
    return c - '0';
}

static uint8_t
string_to_byte(char *c) {
  return char_to_hex(c[0]) * 16 + char_to_hex(c[1]);
}

static int
start_with(const char *str, const char *c) {
  int len = strlen(c);
  int i;

  for ( i=0; i<len; i++ )
  {
    if ( str[i] != c[i] )
    {
       return 0;
    }
  }
  return 1;
}

static int
split(char **dst, char *src, char *delim, int maxsz) {
  char *s = strtok(src, delim);
  int size = 0;

  while ( NULL != s ) {
    *dst++ = s;
    if ( (++size) >= maxsz ) {
      break;
    }
    s = strtok(NULL, delim);
  }

  return size;
}

static void
show_progress(uint8_t slot_id, int current_progress, int total) {
  printf("Progress: %.0f %%\r", (float) (((current_progress+1)/(float)total)*100));
  _set_fw_update_ongoing(slot_id, 60);
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

static int
vr_ISL_polling_status(uint8_t slot_id, uint8_t bus, uint8_t addr, uint8_t intf) {
  int ret = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = 0xC7; //command code
  tbuf[4] = 0x7E; //data0
  tbuf[5] = 0x00; //data1
  tlen = 6;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Failed to send PROGRAMMER_STATUS command", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x04; //read cnt
  tbuf[3] = 0xC5; //command code
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Failed to get PROGRAMMER_STATUS", __func__);
    goto error_exit;
  }

  //bit1 is held to 1, it means the action is successful.
  return rbuf[0] & 0x1;

error_exit:

  return ret;
}

static int
vr_ISL_program(uint8_t slot_id, vr *dev, uint8_t force) {
  int i = 0;
  int ret = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t remaining_writes = 0x00;
  int retry = MAX_RETRY;
  vr_data *list = dev->pdata;
  uint8_t addr = dev->addr;
  uint8_t bus = dev->bus;
  uint8_t intf = dev->intf;
  int len = dev->data_cnt;

  //get the remaining of the VR
  ret = bic_get_isl_vr_remaining_writes(slot_id, bus, addr, &remaining_writes, intf);
  if ( ret < 0 ) {
    goto error_exit;
  }

  //check it
  ret = vr_remaining_writes_check(remaining_writes, force);
  if ( ret < 0 ) {
    goto error_exit;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  for ( i=0; i<len; i++ ) {
    //prepare data
    tbuf[3] = list[i].command ;//command code
    memcpy(&tbuf[4], list[i].data, list[i].data_len);
    tlen = 4 + list[i].data_len;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "[%s] Failed to send data...%d", __func__, i);
      break;
    }
    msleep(100);
    show_progress(slot_id, i, len);
  }

  //check the status
  retry = MAX_RETRY;
  do {
    if ( vr_ISL_polling_status(slot_id, bus, addr, intf) > 0 ) {
      break;
    } else {
      retry--;
      sleep(2);
    }
  } while ( retry > 0 );

  if ( retry == 0 ) {
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
  char tmp_buf[64] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int i = 0, j = 0;
  int data_cnt = 0;
  uint8_t addr = 0;
  uint8_t cnt = 0;
  uint8_t crc8 = 0;
  uint8_t crc8_check = 0;
  uint8_t data[16] = {0};
  uint8_t data_end = 0;

  if ( (fp = fopen(image, "r") ) == NULL ) {
    printf("Invalid file: %s\n", image);
    ret = -1;
    goto error_exit;
  }

  while( NULL != fgets(tmp_buf, tmp_len, fp) ) {
    /*search for 0xAD and 0xAE command*/
    if ( start_with(tmp_buf, "49") ) {
      if ( string_to_byte(&tmp_buf[6]) == 0xad ) {
          if (vr_list[vr_cnt].addr != 0x0 ) vr_cnt++; //move on to the next addr if 4 VRs are included a file
        vr_list[vr_cnt].addr = string_to_byte(&tmp_buf[4]);
        vr_list[vr_cnt].devid[0] = string_to_byte(&tmp_buf[14]);
        vr_list[vr_cnt].devid[1] = string_to_byte(&tmp_buf[12]);
        vr_list[vr_cnt].devid[2] = string_to_byte(&tmp_buf[10]);
        vr_list[vr_cnt].devid[3] = string_to_byte(&tmp_buf[8]);
        vr_list[vr_cnt].devid_len = 4;
#ifdef DEBUG
        printf("[Get] ID ");
        for ( i = 0; i < vr_list[vr_cnt].devid_len; i++ ) {
          printf("%x ", vr_list[vr_cnt].devid[i]);
        }
        printf(",Addr %x\n", vr_list[vr_cnt].addr);
#endif
      }
      else if (string_to_byte(&tmp_buf[6]) == 0xae) {
        vr_list[vr_cnt].dev_rev = (string_to_byte(&tmp_buf[8]) <= REVD_MAX_REVISION_NUM)? REVISION_D:REVISION_F; // <= 5 means revision D, >= 6 means revision F
#ifdef DEBUG
        printf("[Get] revision ");
        printf("%s ", (vr_list[vr_cnt].dev_rev == REVISION_D)? "D":"F");
        printf(",Addr %x\n", vr_list[vr_cnt].addr);
#endif
      }
    } else if ( start_with(tmp_buf, "00") ) {
      //initialize the metadata
      j = 0;
      data_cnt = vr_list[vr_cnt].data_cnt;
      cnt = string_to_byte(&tmp_buf[2]);
      addr = string_to_byte(&tmp_buf[4]);
      data_end = cnt * 2;
      crc8 = string_to_byte(&tmp_buf[data_end+2]);
      data[j++] = addr; //put addr to buffer to calculate crc8

      //printf("cnt %x, addr %x, data_end %x, crc8 %x\n", cnt, addr, data_end, crc8);
      if ( addr != vr_list[vr_cnt].addr ) {
        syslog(LOG_WARNING, "[%s] Failed to parse this line since the addr is not match. 0x%x != 0x%x\n", __func__, addr, vr_list[vr_cnt].addr);
        syslog(LOG_WARNING, "[%s] %s\n", __func__, tmp_buf);
        ret = -1;
        break;
      }

      //get the data
      for ( i = 6; i <= data_end; i += 2, j++ ) {
        data[j] = string_to_byte(&tmp_buf[i]);
      }

      //calculate crc8
      crc8_check = 0;
      crc8_check = cal_crc8(crc8_check, data, cnt-1);

      if ( crc8_check != crc8 ) {
        syslog(LOG_WARNING, "[%s] CRC8 is not match. Expected CRC8: 0x%x, Acutal CRC8: 0x%x\n", __func__, crc8, crc8_check);
        ret = -1;
        break;
      }

      vr_list[vr_cnt].pdata[data_cnt].command = data[1];
      vr_list[vr_cnt].pdata[data_cnt].data_len = cnt - 3;
      memcpy(vr_list[vr_cnt].pdata[data_cnt].data, &data[2], vr_list[vr_cnt].pdata[data_cnt].data_len);
#ifdef DEBUG
      printf(" cmd: %x, data_len: %x, data: ", vr_list[vr_cnt].pdata[data_cnt].command, vr_list[vr_cnt].pdata[data_cnt].data_len);
      for ( i = 0; i < vr_list[vr_cnt].pdata[data_cnt].data_len; i++){
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
  if ( fp != NULL ) fclose(fp);

  return ret;
}

//CRC16 lookup table
unsigned int CRC16_LUT[256] = {
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

static int
cal_TI_crc16(vr *dev) {
  uint8_t data[254] = {0};
  uint8_t data_index = 0;
  uint8_t crc16_result[2] = {0};
  uint32_t crc16_accum = 0;
  uint32_t crc_shift = 0;
  uint8_t index = 0;
  int i = 0;

  for (i = 0; i < dev->data_cnt; i++) {
    if ( i == 0 ) {
      memcpy(crc16_result, &dev->pdata[i].data[9], 2);
      memcpy(&data[data_index], &dev->pdata[i].data[11], 21); //get data
      data_index += 21;
    } else if ( i == 8 ) { //get the last data
      memcpy(&data[data_index], dev->pdata[i].data, 9);
      data_index += 9;
    } else {
      memcpy(&data[data_index], dev->pdata[i].data, 32);
      data_index += 32;
    }
  }

  for(i=0; i<254; i++) {
    index = ((crc16_accum >> 8) ^ data[i]) & 0xFF;
    crc_shift = (crc16_accum << 8) & 0xFFFF;
    crc16_accum = (crc_shift ^ CRC16_LUT[index]) & 0xFFFF;
  }

  return ( crc16_accum == (crc16_result[1] << 8 | crc16_result[0]))?0:-1;
}

static int
vr_TI_csv_parser(char *image) {
#define IC_DEVICE_ID "IC_DEVICE_ID"
#define BLOCK_READ "BlockRead"
#define BLOCK_WRITE "BlockWrite"
  int ret = 0;
  FILE *fp = NULL;
  char *token = NULL;
  char tmp_buf[128] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int data_cnt = 0;
  int i = 0;
  uint8_t data_index = 0;

  if ( (fp = fopen(image, "r") ) == NULL ) {
    printf("Invalid file: %s\n", image);
    ret = -1;
    goto error_exit;
  }

  while ( NULL != fgets(tmp_buf, tmp_len, fp) ) {
    if ( (token = strstr(tmp_buf, IC_DEVICE_ID)) != NULL ) { //get device id from the string
      token = strstr(token, "0x"); //get the pointer
      vr_list[vr_cnt].devid_len = 6;
      vr_list[vr_cnt].devid[0] = string_to_byte(&token[2]);
      vr_list[vr_cnt].devid[1] = string_to_byte(&token[4]);
      vr_list[vr_cnt].devid[2] = string_to_byte(&token[6]);
      vr_list[vr_cnt].devid[3] = string_to_byte(&token[8]);
      vr_list[vr_cnt].devid[4] = string_to_byte(&token[10]);
      vr_list[vr_cnt].devid[5] = string_to_byte(&token[12]);
    } else if ( (token = strstr(tmp_buf, BLOCK_READ)) != NULL ) { //get block read
      if ( vr_list[vr_cnt].addr != 0x00 ) {
        continue;
      }

      token = strstr(token, ",");
      vr_list[vr_cnt].addr = string_to_byte(&token[3]) << 1;
    } else if ( (token = strstr(tmp_buf, BLOCK_WRITE)) != NULL ) { //get block write
      token = strstr(token, ",");
      vr_list[vr_cnt].pdata[data_cnt].command  = string_to_byte(&token[8]);
      vr_list[vr_cnt].pdata[data_cnt].data_len = string_to_byte(&token[13]);
      int data_len = vr_list[vr_cnt].pdata[data_cnt].data_len;
      for (i = 0, data_index = 15; i < data_len; data_index+=2, i++) {
        vr_list[vr_cnt].pdata[data_cnt].data[i] = string_to_byte(&token[data_index]);
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
  ret = cal_TI_crc16(&vr_list[vr_cnt]);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] CRC16 is error!", __func__);
  }

error_exit:
  if ( fp != NULL ) fclose(fp);

  return ret;
}

static int
vr_TI_program(uint8_t slot_id, vr *dev, uint8_t force) {
#define TI_USER_NVM_INDEX   0xF5
#define TI_USER_NVM_EXECUTE 0xF6
#define TI_NVM_INDEX_00     0x00
  int i = 0;
  int ret = 0;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int check_cnt = 0;
  uint8_t addr = dev->addr;
  uint8_t bus = dev->bus;
  uint8_t intf = dev->intf;
  uint8_t skip_offs = 0;
  int len = dev->data_cnt;
  vr_data *list = dev->pdata;

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt

  //step 1- Set page to 0x00 first
  tbuf[3] = TI_USER_NVM_INDEX;
  tbuf[4] = TI_NVM_INDEX_00;
  tlen = 5;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot initialize the page to 0x00!", __func__);
    goto error_exit;
  }

  //step 2 - program a VR
  printf("programming to NVM\n");
  for ( i=0; i<len; i++ ) {
    //prepare data
    tbuf[3] = list[i].command ;//command code
    tbuf[4] = list[i].data_len;//counts
    memcpy(&tbuf[5], list[i].data, list[i].data_len);
    tlen = 5 + list[i].data_len;

    //send it
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "[%s] Failed to send data...%d", __func__, i);
      break;
    }

    msleep(50);
  }

  //step 3 - verify data
  tbuf[3] = TI_USER_NVM_INDEX;
  tbuf[4] = TI_NVM_INDEX_00;
  tlen = 5;
  msleep(300);
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot initialize the page to 0x00 again.!", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x21;
  tbuf[3] = TI_USER_NVM_EXECUTE;
  tlen = 4;
  for ( i=0; i<len; i++ ) {
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "[%s] Failed to read data. index:%d", __func__, i);
    } else {
      if ( rbuf[0] == 0x20 ) {
        memmove(rbuf, &rbuf[1], 0x20);
      } else {
        syslog(LOG_WARNING, "[%s] The count of data is incorrect. index:%d, data_len:%d", __func__, i, rbuf[0]);
        ret = -1;
        goto error_exit;
      }

      // The first 9 bytes are all 0xFF in VR file, so skip the comparison
      // byte 0~5: IC_DEVICE_ID; byte 6~7: IC_DEVICE_REV; byte 8: current slave address
      skip_offs = (i == 0) ? 9 : 0;
      ret = memcmp(rbuf + skip_offs, list[i].data + skip_offs, list[i].data_len - skip_offs);
      if ( ret == 0 ) {
        check_cnt++;
      }
    }
    msleep(50);
    show_progress(slot_id, i, len);
  }
  printf("\n");

  if ( check_cnt != len ) {
    ret = -1;
  }

error_exit:
  return ret;
}

static uint32_t
cal_IFX_crc32(vr_data *list, int len) {
  uint32_t crc = 0xFFFFFFFF;
  int i, j, b;

  for (i = 0; i < len; i++) {
    for (j = 0; j < 4; j++) {
      crc ^= list[i].data[j];
      for (b = 0; b < 8; b++) {
        if (crc & 0x01) {
          crc = (crc >> 1) ^ 0xEDB88320;  // polynomial 0xEDB88320 (lsb-first)
        } else {
          crc >>= 1;
        }
      }
    }
  }

  return ~crc;
}

static int
vr_IFX_mic_parser(char *image) {
#define ADDRESS_FIELD  "PMBus Address :"
#define CHECKSUM_FIELD "Checksum :"
#define DATA_START_TAG "[Configuration Data]"
#define DATA_END_TAG   "[End Configuration Data]"
#define DATA_COMMENT   "//"
#define DATA_XV        "XV"
#define DATA_LEN_IN_LINE 5
#define SEC_TRIM       0x02
  const size_t len_start = strlen(DATA_START_TAG);
  const size_t len_end = strlen(DATA_END_TAG);
  const size_t len_comment = strlen(DATA_COMMENT);
  const size_t len_xv = strlen(DATA_XV);
  const uint16_t ifx_devid = 0x8A03;
  FILE *fp = NULL;
  char *token = NULL;
  char tmp_buf[128] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int data_cnt = 0, start_idx = 0;
  int i, ret = 0;
  uint8_t sec_type = 0x0;
  uint16_t offset, sec_size = 0x0;
  uint32_t sum = 0, sum_exp = 0;
  uint32_t dword, crc;
  bool is_data = false;

  if ( (fp = fopen(image, "r") ) == NULL ) {
    printf("Invalid file: %s\n", image);
    return BIC_STATUS_FAILURE;
  }

  while ( NULL != fgets(tmp_buf, tmp_len, fp) ) {
    if ( !strncmp(tmp_buf, DATA_COMMENT, len_comment) ) {
      token = tmp_buf + len_comment;
      if ( !strncmp(token, DATA_XV, len_xv) ) {  // find "//XV..."
        printf("Parsing %s", token);
      }
      continue;
    } else if ( !strncmp(tmp_buf, DATA_END_TAG, len_end) ) {
      // dummy data (end of data)
      vr_list[vr_cnt].pdata[data_cnt].command = 0xFF;
      vr_list[vr_cnt].pdata[data_cnt].data_len = 0;
      data_cnt++;
      break;
    } else if ( is_data == true ) {
      char *token_list[8] = {0};
      int token_size = split(token_list, tmp_buf, " ", DATA_LEN_IN_LINE);

      if ( token_size < 1 ) {  // unexpected data line
        continue;
      }
      offset = (uint16_t)strtol(token_list[0], NULL, 16);
      if ( sec_type == SEC_TRIM && offset != 0x0 ) {  // skip Trim
        continue;
      }

      for ( i = 1; i < token_size; i++ ) {  // DWORD0 ~ DWORD3
        dword = (uint32_t)strtoul(token_list[i], NULL, 16);
        if ( offset == 0x0 ) {
          switch (i) {
            case 1:  // start of section header
              start_idx = data_cnt;
              sec_type = (uint8_t)dword;  // section type
              break;
            case 2:  // section size
              sec_size = (uint16_t)dword;
              break;
            case 3:  // section header CRC
              // check CRC of section header
              crc = cal_IFX_crc32(&vr_list[vr_cnt].pdata[start_idx], data_cnt - start_idx);
              if ( dword != crc ) {
                syslog(LOG_WARNING, "CRC mismatched! Expected: %08X, Actual: %08X", dword, crc);
                ret = BIC_STATUS_FAILURE;
                break;
              }

              sum += dword;
              start_idx = data_cnt + 1;  // start of section data
              break;
          }
          if ( sec_type == SEC_TRIM ) {
            break;
          }
        } else if ( (offset + i*4) == sec_size ) {  // section data CRC
          // check CRC of section data
          crc = cal_IFX_crc32(&vr_list[vr_cnt].pdata[start_idx], data_cnt - start_idx);
          if ( dword != crc ) {
            syslog(LOG_WARNING, "CRC mismatched! Expected: %08X, Actual: %08X", dword, crc);
            ret = BIC_STATUS_FAILURE;
            break;
          }
          sum += dword;
        }
        if ( ret != 0 ) {
          break;
        }

        vr_list[vr_cnt].pdata[data_cnt].command = sec_type;
        vr_list[vr_cnt].pdata[data_cnt].data_len = 4;
        memcpy(vr_list[vr_cnt].pdata[data_cnt].data, &dword, sizeof(dword));
        data_cnt++;
      }
      if ( ret != 0 ) {
        break;
      }
    } else {
      if ( (token = strstr(tmp_buf, ADDRESS_FIELD)) != NULL ) {
        if ( (token = strstr(token, "0x")) != NULL ) {
          vr_list[vr_cnt].addr = (uint8_t)(strtoul(token, NULL, 16) << 1);
        }
      } else if ( (token = strstr(tmp_buf, CHECKSUM_FIELD)) != NULL ) {
        if ( (token = strstr(token, "0x")) != NULL ) {
          sum_exp = (uint32_t)strtoul(token, NULL, 16);
          printf("Configuration Checksum: %08X\n", sum_exp);
        }
        // set DEVID manually
        vr_list[vr_cnt].devid_len = 2;
        memcpy(vr_list[vr_cnt].devid, (uint8_t *)&ifx_devid, vr_list[vr_cnt].devid_len);
      } else if ( !strncmp(tmp_buf, DATA_START_TAG, len_start) ) {
        is_data = true;
        continue;
      }
    }
  }
  fclose(fp);

  if ( ret == 0 ) {
    vr_list[vr_cnt].data_cnt = data_cnt;
    if ( sum_exp != sum ) {
      syslog(LOG_WARNING, "Checksum mismatched! Expected: %08X, Actual: %08X", sum_exp, sum);
      ret = BIC_STATUS_FAILURE;
    }
  }

  return ret;
}

static int
vr_IFX_program(uint8_t slot_id, vr *dev, uint8_t force) {
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t curr_sec = 0xFF;
  uint8_t remaining_writes = 0;
  uint8_t addr = dev->addr;
  uint8_t bus = dev->bus;
  uint8_t intf = dev->intf;
  int ret = BIC_STATUS_FAILURE;
  int i, j, len = dev->data_cnt;
  int size, start_idx = 0;
  vr_data *list = dev->pdata;

  // get the remaining writes of the VR
  if ( bic_get_ifx_vr_remaining_writes(slot_id, bus, addr, &remaining_writes, intf) < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to get vr remaining writes", __func__);
    return BIC_STATUS_FAILURE;
  }

  // check remaining writes
  if ( vr_remaining_writes_check(remaining_writes, force) < 0 ) {
    return BIC_STATUS_FAILURE;
  }

  // to stop bic polling VR sensors
  if ( bic_disable_sensor_monitor(slot_id, 1, intf) < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to disable vr monitor", __func__);
    return BIC_STATUS_FAILURE;
  }
  dis_bic_vr.slot_id = slot_id;
  sleep(2);

  for ( i = 0; i < len; i++ ) {
    if ( curr_sec != list[i].command ) {  // start of section
      if ( curr_sec != 0xFF ) {  // when it's not the 1st
        // check scratchpad data (read and compare to list data)
        tbuf[0] = (bus << 1) + 1;
        tbuf[1] = addr;
        tbuf[2] = 0;    // read count
        tbuf[3] = IFX_MFR_AHB_ADDR;
        tbuf[4] = 4;
        tbuf[5] = 0x00;
        tbuf[6] = 0xe0;
        tbuf[7] = 0x05;
        tbuf[8] = 0x20;
        tlen = 9;
        ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to set scratchpad addr", __func__);
          break;
        }
        msleep(10);

        tbuf[2] = 6;    // read count
        tbuf[3] = IFX_MFR_REG_READ;
        tlen = 4;
        for ( j = start_idx; j < i; j++ ) {
          ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
          if ( ret < 0 ) {
            syslog(LOG_WARNING, "%s() Failed to read data %d", __func__, j);
            break;
          }
          if ( memcmp(&rbuf[1], list[j].data, 4) != 0 ) {
            syslog(LOG_WARNING, "%s() Scratchpad(%d) mismatched!", __func__, j);
            ret = BIC_STATUS_FAILURE;
            break;
          }
        }
        if ( ret < 0 ) {
          break;  // TBD (enhancement): re-program the section
        }

        // upload scratchpad to OTP
        size = (i - start_idx) * 4;
        memcpy(tbuf, &size, 2);
        tbuf[2] = 0x00;
        tbuf[3] = 0x00;
        ret = bic_ifx_vr_mfr_fw(slot_id, bus, addr, OTP_CONF_STO, tbuf, NULL, intf);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to upload data to OTP %02X", __func__, curr_sec);
          break;
        }

        // wait for programming soak (2ms/byte, at least 200ms)
        // ex: Config (604 bytes): (604 / 50) + 2 = 14 (1400 ms)
        size = (size / 50) + 2;
        for ( j = 0; j < size; j++ ) {
          msleep(100);
        }

        tbuf[0] = (bus << 1) + 1;
        tbuf[1] = addr;
        tbuf[2] = 1;    // read count
        tbuf[3] = PMBUS_STS_CML;
        tlen = 4;
        ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
        if ( ret < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to read PMBUS_STS_CML", __func__);
          break;
        }

        if ( rbuf[0] & 0x01 ) {
          syslog(LOG_WARNING, "%s() CML Other Memory Fault: %02X (%d)", __func__, rbuf[0], start_idx);
          ret = BIC_STATUS_FAILURE;
          break;
        }

        // dummy data (end of data)
        if ( list[i].data_len == 0 ) {
          break;
        }
      }

      // clear bit0 of PMBUS_STS_CML
      tbuf[0] = (bus << 1) + 1;
      tbuf[1] = addr;
      tbuf[2] = 0;    // read count
      tbuf[3] = PMBUS_STS_CML;
      tbuf[4] = 0x01;
      tlen = 5;
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
      if ( ret < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to write PMBUS_STS_CML", __func__);
        break;
      }

      // invalidate existing data
      tbuf[0] = list[i].command;  // section type
      tbuf[1] = 0x00;             // xv0
      tbuf[2] = 0x00;
      tbuf[3] = 0x00;
      ret = bic_ifx_vr_mfr_fw(slot_id, bus, addr, OTP_FILE_INVD, tbuf, NULL, intf);
      if ( ret < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to invalidate %02X", __func__, list[i].command);
        break;
      }
      msleep(10);

      // set scratchpad addr
      tbuf[0] = (bus << 1) + 1;
      tbuf[1] = addr;
      tbuf[2] = 0;    // read count
      tbuf[3] = IFX_MFR_AHB_ADDR;
      tbuf[4] = 4;
      tbuf[5] = 0x00;
      tbuf[6] = 0xe0;
      tbuf[7] = 0x05;
      tbuf[8] = 0x20;
      tlen = 9;
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
      if ( ret < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to set scratchpad addr", __func__);
        break;
      }
      msleep(10);

      start_idx = i;  // start of the section
      curr_sec = list[i].command;
    }

    // program data into scratch
    tbuf[3] = IFX_MFR_REG_WRITE;
    tbuf[4] = 4;
    memcpy(&tbuf[5], list[i].data, 4);
    tlen = 9;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to write data %08X (%d)", __func__, *(uint32_t *)list[i].data, i);
      break;
    }
    msleep(10);

    show_progress(slot_id, i, len);
  }
  printf("\n");
  if ( ret < 0 ) {
    ret = BIC_STATUS_FAILURE;
  }

  if ( bic_disable_sensor_monitor(slot_id, 0, intf) < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to enable vr monitor", __func__);
  }
  dis_bic_vr.slot_id = 0;

  return ret;
}

static bool
is_valid_data(char c) {
  if ( (c >= 'A' && c <= 'F') || \
       (c >= '0' && c <= '9') ) {
    return true;
  } else if ( (c >= 'a' && c <= 'f')) {
    printf("Please all caps\n");
  }
  return false;
}

static int
vr_VY_txt_parser(char *image) {
  int ret = BIC_STATUS_FAILURE;
  FILE *fp = NULL;
  char tmp_buf[64] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int i = 0, j = 0;
  uint8_t char_cnt = 0;
  int data_cnt = 0;
  bool is_cmd = true;

  if ( (fp = fopen(image, "r") ) == NULL ) {
    printf("Invalid file: %s\n", image);
    goto error_exit;
  }

  while( NULL != fgets(tmp_buf, tmp_len, fp) ) {
    if (  is_valid_data(tmp_buf[0]) == false ) continue;

    //check line in char
    for ( i = 0, j = 0, is_cmd = true; tmp_buf[i] != '\n'; i++ ) {
      //is valid char
      if ( is_valid_data(tmp_buf[i]) == true ) {
        //is data end in the next?
        if ( is_valid_data(tmp_buf[i+1]) == false ) {
          //cmd
          if ( is_cmd == true ) {
            //store cmd
            vr_list[vr_cnt].pdata[data_cnt].command  = string_to_byte(&tmp_buf[i-1]);
            is_cmd = false;
          } else {
            //store data
            if ( char_cnt == 0 ) vr_list[vr_cnt].pdata[data_cnt].data[j] = char_to_hex(tmp_buf[i]);
            else vr_list[vr_cnt].pdata[data_cnt].data[j] = string_to_byte(&tmp_buf[i-1]);
            j++;
          }
          char_cnt = 0;
        } else char_cnt++;
      }
    }
    vr_list[vr_cnt].pdata[data_cnt].data_len = j;
    data_cnt++;
  }
  vr_list[vr_cnt].data_cnt = data_cnt;
  vr_list[vr_cnt].addr = 0xff;   //set it manually
  vr_list[vr_cnt].devid_len = 2;
  vr_list[vr_cnt].devid[0] = 0x0;
  vr_list[vr_cnt].devid[1] = 0x0;
  ret = BIC_STATUS_SUCCESS;
error_exit:
  if ( fp != NULL ) fclose(fp);

  return ret;
}

static int
vr_VY_program(uint8_t slot_id, vr *dev, uint8_t force) {
  /*not supported*/
  return BIC_STATUS_FAILURE;
}

#include "bic_bios_fwupdate.h"
static int
vr_usb_program(uint8_t slot_id, uint8_t sel_vendor, uint8_t comp, vr *dev, uint8_t force) {
#define DEFAULT_DATA_SIZE 16
#define USB_LAST_DATA_FLAG  0x80
#define USB_2OU_ISL_VR_IDX  0x04
#define USB_2OU_VY_VR_STBY1 0x06
#define USB_2OU_VY_VR_STBY2 0x07
#define USB_2OU_VY_VR_STBY3 0x08
#define USB_2OU_VY_VR_P1V8  0x09

  int ret = BIC_STATUS_FAILURE;
  int i = 0;
  int len = dev->data_cnt;
  vr_data *list = dev->pdata;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;
  uint16_t bic_usb_pid = 0x0, bic_usb_vid = 0x0;
  uint8_t usb_idx = 0;
  uint8_t *buf = NULL;

  //select PID/VID
  switch(dev->intf) {
    case REXP_BIC_INTF:
      bic_usb_pid = EXP2_TI_PRODUCT_ID;
      bic_usb_vid = EXP2_TI_VENDOR_ID;
      break;
    default:
      printf("%s() intf(0x%02X) didn't support USB firmware update\n", __func__, dev->intf);
      return BIC_STATUS_FAILURE;
  }

  //select INDEX
  switch(comp) {
    case FW_2OU_PESW_VR:
      usb_idx = USB_2OU_ISL_VR_IDX;
      break;
    case FW_2OU_3V3_VR1:
      usb_idx = USB_2OU_VY_VR_STBY1;
      break;
    case FW_2OU_3V3_VR2:
      usb_idx = USB_2OU_VY_VR_STBY2;
      break;
    case FW_2OU_3V3_VR3:
      usb_idx = USB_2OU_VY_VR_STBY3;
      break;
    case FW_2OU_1V8_VR:
      usb_idx = USB_2OU_VY_VR_P1V8;
      break;
    default:
      printf("%s() comp(0x%02X) didn't support USB firmware update\n", __func__, comp);
      return BIC_STATUS_FAILURE;
  }

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  if ( (ret = bic_init_usb_dev(slot_id, udev, bic_usb_pid, bic_usb_vid)) < 0 ) goto error_exit;

  // allocate mem
  buf = malloc(USB_PKT_HDR_SIZE + DEFAULT_DATA_SIZE);
  if ( buf == NULL ) {
    printf("%s() failed to allocate memory\n", __func__);
    goto error_exit;
  }
  uint8_t *file_buf = buf + USB_PKT_HDR_SIZE;
  for ( i = 0; i < len; i++ ) {
    bic_usb_packet *pkt = (bic_usb_packet *) buf;
    // if it's the last one, append the flag
    if ( i + 1 == len ) pkt->target = USB_LAST_DATA_FLAG|usb_idx;
    else pkt->target  = usb_idx; // component

    pkt->offset = 0x0;         // dummy
    pkt->length = list[i].data_len + 2; // read bytes
    file_buf[0] = list[i].data_len; // data length
    file_buf[1] = list[i].command;  // command code
    memcpy(&file_buf[2], list[i].data, list[i].data_len); // data
    ret = send_bic_usb_packet(udev, pkt);
    if (ret < 0) {
      printf("Failed to write data to the device\n");
      goto error_exit;
    }
    show_progress(slot_id, i, len);
  }
  printf("\n");

  switch(comp) {
    case FW_2OU_PESW_VR:
      {
        int retry = MAX_RETRY;
        sleep(1);
        do {
          if ( vr_ISL_polling_status(slot_id, dev->bus, dev->addr, dev->intf) > 0 ) {
            break;
          } else {
            retry--;
            sleep(2);
          }
        } while ( retry > 0 );
        if ( retry == 0 ) ret = BIC_STATUS_FAILURE;
        else ret = BIC_STATUS_SUCCESS;
      }
      break;
    case FW_2OU_3V3_VR1:
    case FW_2OU_3V3_VR2:
    case FW_2OU_3V3_VR3:
    case FW_2OU_1V8_VR:
      printf("please wait 15 seconds for programming to complete\n");
      break;
  }

error_exit:
  if ( buf != NULL ) free(buf);
  bic_close_usb_dev(udev);
  return ret;
}

struct dev_table {
  uint8_t intf;
  uint8_t bus;
  uint8_t addr;
  char *dev_name;
  uint8_t comp;
} dev_list[] = {
  {NONE_INTF,     VR_SB_BUS,  VCCIN_ADDR,        "VCCIN/VCCFA_EHV_FIVRA", FW_VR_VCCIN},
  {NONE_INTF,     VR_SB_BUS,  VCCD_ADDR,         "VCCD",                  FW_VR_VCCD},
  {NONE_INTF,     VR_SB_BUS,  VCCINFAON_ADDR,    "VCCINFAON/VCCFA_EHV",   FW_VR_VCCINFAON},
  {REXP_BIC_INTF, VR_2OU_BUS, VR_PESW_ADDR,      "VR_P0V84/P0V9", FW_2OU_PESW_VR},
  {REXP_BIC_INTF, VR_2OU_BUS, VR_2OU_P3V3_STBY1, "VR_P3V3_STBY1", FW_2OU_3V3_VR1},
  {REXP_BIC_INTF, VR_2OU_BUS, VR_2OU_P3V3_STBY2, "VR_P3V3_STBY2", FW_2OU_3V3_VR2},
  {REXP_BIC_INTF, VR_2OU_BUS, VR_2OU_P3V3_STBY3, "VR_P3V3_STBY3", FW_2OU_3V3_VR3},
  {REXP_BIC_INTF, VR_2OU_BUS, VR_2OU_P1V8,       "VR_P1V8",       FW_2OU_1V8_VR}
};

int dev_table_size = (sizeof(dev_list)/sizeof(struct dev_table));

static struct tool {
  int (*parser)(char *);
  int (*program)(uint8_t, vr *, uint8_t);
} vr_tool[] = {
  [VR_ISL] = {vr_ISL_hex_parser, vr_ISL_program},
  [VR_TI]  = {vr_TI_csv_parser , vr_TI_program},
  [VR_IFX] = {vr_IFX_mic_parser, vr_IFX_program},
  [VR_VY]  = {vr_VY_txt_parser,  vr_VY_program},
};

static char*
get_vr_name(uint8_t intf, uint8_t bus, uint8_t addr, uint8_t comp) {
  int i;

  for ( i = 0; i < dev_table_size; i++ ) {
    // check comp first, if it's false, check addr further
    if ( comp == dev_list[i].comp && addr == dev_list[i].addr &&
         bus == dev_list[i].bus && intf == dev_list[i].intf ) {
      return dev_list[i].dev_name;
    }
  }

  return NULL;
}

static int
_lookup_vr_devid(uint8_t slot_id, uint8_t comp, uint8_t intf, uint8_t *rbuf,
                 uint8_t *sel_vendor, uint8_t *vr_bus, uint8_t *vr_addr) {
  int i, ret = 0;
  uint8_t rlen = TI_DEVID_LEN;

  for ( i = 0; i < dev_table_size; i++ ) {
    if ( dev_list[i].intf == intf && dev_list[i].comp == comp ) {
      printf("Find: ");
      ret = bic_get_vr_device_id(slot_id, rbuf, &rlen, dev_list[i].bus, dev_list[i].addr, intf);
      if ( ret == BIC_STATUS_SUCCESS && rlen <= TI_DEVID_LEN/*the longest length of dev id*/ ) {
        *vr_bus = dev_list[i].bus;
        *vr_addr = dev_list[i].addr;
        break;
      }
    }
  }
  if ( i == dev_table_size ) {
    printf("Couldn't get the devid from VRs\n");
    return BIC_STATUS_FAILURE;
  }

  if ( rlen == IFX_DEVID_LEN ) {
    *sel_vendor = VR_IFX;
    // the length of IFX and VY is the same, check it further
    if ( intf == REXP_BIC_INTF && dev_list[i].comp != FW_2OU_PESW_VR ) {
      *sel_vendor = VR_VY;
    }
  } else if (rlen == TI_DEVID_LEN ) {
    *sel_vendor = VR_TI;
  } else {
    *sel_vendor = VR_ISL;
  }

  printf("VR vendor=%s(%x.%x)\n", (*sel_vendor == VR_IFX)?"IFX": \
                                  (*sel_vendor == VR_TI)?"TI": \
                                  (*sel_vendor == VR_ISL)?"ISL":"VY", rlen, *sel_vendor);
  return BIC_STATUS_SUCCESS;
}

int
get_vr_revision(uint8_t slot_id, uint8_t comp, uint8_t intf, uint8_t *dev_rev,
                 uint8_t sel_vendor){
  int i, ret = 0;
  uint8_t rbuf[TI_DEVID_LEN] = {0};

  //For now only ISL need to check FW&IC revision
  if(sel_vendor == VR_ISL) {
    for (i = 0; i < dev_table_size; i++) {
      if (dev_list[i].intf == intf && dev_list[i].comp == comp) {
        printf("Find: ");
        ret = bic_get_vr_device_revision(slot_id, rbuf, dev_list[i].bus, dev_list[i].addr, intf);
        if (ret == BIC_STATUS_SUCCESS) {
          break;
        }
      }
    }

    if(i == dev_table_size) {
      printf("Couldn't get the revision from VRs\n");
      return BIC_STATUS_FAILURE;
    }

    if(dev_rev != NULL) {
      if(rbuf[3] <= REVD_MAX_REVISION_NUM) { // <= 5 means revision D
        *dev_rev = REVISION_D;
      } else { // >= 6 means revision F
        *dev_rev = REVISION_F;
      }
      return BIC_STATUS_SUCCESS;
    }
    else {
      return BIC_STATUS_FAILURE;
    }
  }
  else {
    return BIC_STATUS_SUCCESS;
  }
}

int
update_bic_vr(uint8_t slot_id, uint8_t comp, char *image, uint8_t intf, uint8_t force, bool usb_update) {
  int i, ret = 0;
  uint8_t devid[TI_DEVID_LEN] = {0};
  uint8_t dev_rev = 0;
  uint8_t sel_vendor = 0;
  uint8_t vr_bus = 0x0, vr_addr = 0x0;

  ret = _lookup_vr_devid(slot_id, comp, intf, devid, &sel_vendor, &vr_bus, &vr_addr);
  if ( ret < 0 ) {
    return ret;
  }

  ret = get_vr_revision(slot_id, comp, intf, &dev_rev, sel_vendor);
  if ( ret < 0 ) {
    return ret;
  }

  ret = vr_tool[sel_vendor].parser(image);
  if ( ret < 0 ) {
    printf("Cannot parse the file!\n");
    return ret;
  }

  // slave address is unspecified in IFX mic file
  if ( sel_vendor == VR_IFX && !vr_list[0].addr ) {
    vr_list[0].addr = vr_addr;
  }

  //step 2.5 - check if data is existed.
  //data_cnt, addr, and devid_len cannot be 0.
  for ( i = 0; i <= vr_cnt; i++ ) {
    if ( vr_list[i].data_cnt == 0 || vr_list[i].addr == 0 || vr_list[i].devid_len == 0 ) {
      printf("data, addr, or devid_len is not caught!\n");
      return BIC_STATUS_FAILURE;
    }
  }

  //step 3 - check DEVID and version
  if ( memcmp(vr_list[0].devid, devid, vr_list[0].devid_len) != 0 ) {
    printf("Device ID is not match!\n");
    printf(" Expected ID: ");
    for ( i = 0 ; i < vr_list[0].devid_len; i++ ) printf("%02X ", vr_list[0].devid[i]);
    printf("\n");
    printf(" Actual ID: ");
    for ( i = 0 ; i < vr_list[0].devid_len; i++ ) printf("%02X ", devid[i]);
    printf("\n");
    return BIC_STATUS_FAILURE;
  }

  // IC & FW revision should match, revD device can only update with revD firmware, same as revF.
  if (dev_rev != vr_list[vr_cnt].dev_rev) {
    printf("Device revision is not match!\n");
    printf(" Expected revision: ");
    printf("%s \n", (vr_list[vr_cnt].dev_rev == REVISION_D)? "D":"F");
    printf(" Actual revision: ");
    printf("%s \n", (dev_rev == REVISION_D)? "D":"F");
    return BIC_STATUS_FAILURE;
  }

  if ( vr_list[0].addr != vr_addr ) {
    printf("Device slave address mismatched!\n");
    if ( !force ) {
      return BIC_STATUS_FAILURE;
    }
  }

  // bus/intf info is not included in the fw file.
  vr_list[0].bus = vr_bus;
  vr_list[0].intf = intf;

  //step 4 - program
  //For now, we only support to be input 1 file.
  printf("Update %s...", get_vr_name(intf, vr_bus, vr_list[0].addr, comp));
  ret = (usb_update == true)?vr_usb_program(slot_id, sel_vendor, comp, &vr_list[0], force): \
                             vr_tool[sel_vendor].program(slot_id, &vr_list[0], force);

  return ret;
}
