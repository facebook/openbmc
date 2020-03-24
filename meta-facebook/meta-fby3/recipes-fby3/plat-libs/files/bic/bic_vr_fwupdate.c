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
#include "bic_vr_fwupdate.h"
#include "bic_ipmi.h"

//#define DEBUG

/****************************/
/*       VR fw update       */                            
/****************************/
#define WARNING_REMAINING_WRITES 3
#define MAX_RETRY 3
#define VR_BUS 0x8

typedef struct {
  uint8_t command;
  uint8_t data_len;
  uint8_t data[64];
} vr_data;

typedef struct {
  uint8_t addr;
  uint8_t devid[6];
  uint8_t devid_len;
  int data_cnt;
  vr_data pdata[2048];
} vr;

static int vr_cnt = 0;

//4 VRs are on the server board
static vr vr_list[4] = {0};

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
split(char **dst, char *src, char *delim) {
  char *s = strtok(src, delim);
  int size = 0;

  while ( NULL != s ) {
    *dst++ = s;
    size++;
    s = strtok(NULL, delim);
  }

  return size;
}

static void
show_progress(int current_progress, int total) {
  printf("Progress: %.0f %%\r", (float) (((current_progress+1)/(float)total)*100));
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
vr_ISL_polling_status(uint8_t slot_id, uint8_t addr) {
  int ret = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = 0xC7; //command code
  tbuf[4] = 0x07; //data0
  tbuf[5] = 0x07; //data1
  tlen = 6;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Failed to send PROGRAMMER_STATUS command", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x04; //read cnt
  tbuf[3] = 0xC5; //command code
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
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
  int len = dev->data_cnt;

  //get the remaining of the VR
  ret = bic_get_isl_vr_remaining_writes(slot_id, VR_BUS, addr, &remaining_writes);
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
  for ( i=0; i<len; i++ ) {
    //prepare data
    tbuf[3] = list[i].command ;//command code
    memcpy(&tbuf[4], list[i].data, list[i].data_len);
    tlen = 4 + list[i].data_len;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "[%s] Failed to send data...%d", __func__, i);
      break;
    }
    msleep(100);
    show_progress(i, len);
  }

  //check the status
  retry = MAX_RETRY;
  do {
    if ( vr_ISL_polling_status(slot_id, addr) > 0 ) {
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
    /*search for 0xAD command*/
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
#define TI_NVM_CHECKSUM     0xF0
#define TI_NVM_INDEX_00     0x00
  int i = 0;
  int ret = 0;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int check_cnt = 0;
  uint8_t addr = dev->addr;
  int len = dev->data_cnt;
  vr_data *list = dev->pdata;

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  tbuf[2] = 0x00; //read cnt

  //step 1- Set page to 0x00 first
  tbuf[3] = TI_USER_NVM_INDEX;
  tbuf[4] = TI_NVM_INDEX_00;
  tlen = 5;

  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot initialize the page to 0x00!", __func__);
    goto error_exit;
  }

  //step 2 - program a VR
  for ( i=0; i<len; i++ ) {
    //prepare data
    tbuf[3] = list[i].command ;//command code
    tbuf[4] = list[i].data_len;//counts
    memcpy(&tbuf[5], list[i].data, list[i].data_len);
    tlen = 5 + list[i].data_len;

    //send it
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
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
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "[%s] Cannot initialize the page to 0x00 again.!", __func__);
    goto error_exit;
  }

  tbuf[2] = 0x21;
  tbuf[3] = TI_USER_NVM_EXECUTE;
  tlen = 4;
  for ( i=0; i<len; i++ ) {
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
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

      ret = memcmp(rbuf, list[i].data, list[i].data_len);
      if ( ret == 0 ) {
        check_cnt++;
      }
    }
    msleep(50);
  }

  if ( check_cnt != len ) {
    ret = -1;
  }

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
  const uint16_t DEV_ID = 0x0279;
  int ret = 0;
  FILE *fp = NULL;
  char *token = NULL;
  char tmp_buf[128] = "\0";
  int tmp_len = sizeof(tmp_buf);
  int data_cnt = 0;
  int i = 0;
  bool is_data = false;

  if ( (fp = fopen(image, "r") ) == NULL ) {
    printf("Invalid file: %s\n", image);
    ret = -1;
    goto error_exit;
  }

  while ( NULL != fgets(tmp_buf, tmp_len, fp) ) {
    if ( (token = strstr(tmp_buf, CHECKSUM_FIELD)) != NULL ) { //get the checksum
      //set DEVID manually
      memcpy(vr_list[vr_cnt].devid, (uint8_t *)&DEV_ID, 2);
      vr_list[vr_cnt].devid_len = 2;
    } else if ( (token = strstr(tmp_buf, ADDR_FIELD)) != NULL ) {
      token = strstr(tmp_buf, "0x");
      vr_list[vr_cnt].addr = string_to_byte(&token[2]);
    } else if ( (token = strstr(tmp_buf, DATA_START_TAG)) != NULL ) {
      is_data = true;
      continue;
    } else if ( start_with(tmp_buf, DATA_END_TAG) > 0 ) {
      break;
    } else if ( is_data == true ) {
      char *token_list[32] = {NULL};
      int token_size = split(token_list, tmp_buf, " "); //it should be 17 only
      uint8_t page_addr = 0x0;
      uint8_t index_addr = 0x0;

      if ( token_size != DATA_LEN_IN_LINE ) {
        printf("the len of token is not expected!\n");
        ret = -1;
        break;
      }

      page_addr = string_to_byte(&token_list[0][0]);
      index_addr = string_to_byte(&token_list[0][2]);

      for (i = 1; i < DATA_LEN_IN_LINE; i++) {
        if ( start_with(token_list[i], "----") > 0 ) {
          continue;
        }
        vr_list[vr_cnt].pdata[data_cnt].command = page_addr;
        vr_list[vr_cnt].pdata[data_cnt].data_len = 2;//write word
        vr_list[vr_cnt].pdata[data_cnt].data[0] = index_addr + i - 1; //put register offset to data[0]
        vr_list[vr_cnt].pdata[data_cnt].data[1] = string_to_byte(&token_list[i][2]);
        vr_list[vr_cnt].pdata[data_cnt].data[2] = string_to_byte(&token_list[i][0]);
        data_cnt++;
      }
    }
  }
  vr_list[vr_cnt].data_cnt += data_cnt;

error_exit:
  if ( fp != NULL ) fclose(fp);

  return ret;
}

static int
vr_IFX_program(uint8_t slot_id, vr *dev, uint8_t force) {
#define VR_CONF_REG 0x1A
#define VR_CLR_FAULT 0x03
#define REG1_STS_CHECK 0x1
#define REG2_STS_CHECK 0xA
  int i = 0;
  int ret = 0;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t current_page = 0xff;
  uint8_t remaining_writes = 0x00;
  uint8_t addr = dev->addr;
  int len = dev->data_cnt;
  vr_data *list = dev->pdata;

  // get the remaining writes of the VR
  ret = bic_get_ifx_vr_remaining_writes(slot_id, VR_BUS, addr, &remaining_writes);
  if ( ret < 0 ) {
    goto error_exit;
  }

  // check it
  ret = vr_remaining_writes_check(remaining_writes, force);
  if ( ret < 0 ) {
    goto error_exit;
  }

  tbuf[0] = (VR_BUS << 1) + 1;
  tbuf[1] = addr;
  //step 1 - write data to data register
  for (i = 0; i < len; i++ ) {
    if ( list[i].command != current_page ) {
      //set page
      tbuf[2] = 0x00; //read cnt
      tbuf[3] = VR_PAGE;
      tbuf[4] = list[i].command;
      tlen = 5;
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
      if ( ret < 0 ) {
        syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, list[i].command);
        goto error_exit;
      }
      current_page = list[i].command;
      msleep(10);//sleep 10ms and carry on to write the data
    }

    //write data
    memcpy(&tbuf[3], list[i].data, 3);
    tlen = 6;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
    if ( ret < 0 ) {
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
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[3] = VR_CONF_REG;
  tbuf[4] = 0xa1;
  tbuf[5] = 0x08;
  tlen = 6;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't write data to VR_CONF_REG", __func__);
    goto error_exit;
  }

  tbuf[3] = VR_CLR_FAULT;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't sent CLR FAULT", __func__);
    goto error_exit;
  }

  tbuf[3] = 0x1d;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't sent 0x1d", __func__);
    goto error_exit;
  }

  tbuf[3] = 0x24;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't sent 0x1d", __func__);
    goto error_exit;
  }

  sleep(1); //wait for uploadprocess complete

  tbuf[3] = VR_CONF_REG;
  tbuf[4] = 0x00;
  tbuf[5] = 0x00;
  tlen = 6;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't write data to VR_CONF_REG", __func__);
    goto error_exit;
  }

  // step 3 - checking for a successful upload
  tbuf[3] = VR_PAGE;
  tbuf[4] = VR_PAGE60;
  tlen = 5;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[2] = 0x01; //read cnt
  tbuf[3] = 0x01; //sts reg1
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  if ( (rbuf[0] & REG1_STS_CHECK) != 0 ) {
    ret = -1;
    printf("Failed to upload data. rbuf[0]=%02X from reg1\n", rbuf[0]);
    goto error_exit;
  }

  tbuf[3] = 0x02; //sts reg2
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  if ( (rbuf[0] & REG2_STS_CHECK) != 0 ) {
    ret = -1;
    printf("Failed to upload data. rbuf[0]=%02X from reg2\n", rbuf[0]);
    printf("a defective bit is held or the mem space is full!\n");
    goto error_exit;
  }

#if 0
  //need to do sled-cycle to make the config reload
  //step 4 - read crc from the dev
  tbuf[2] = 0x00; //read cnt
  tbuf[3] = VR_PAGE;
  tbuf[4] = VR_PAGE62;
  tlen = 5;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't set page to 0x%02X", __func__, tbuf[4]);
    goto error_exit;
  }

  tbuf[2] = 0x02; //read 2 bytes
  tbuf[3] = 0x42;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't read crc1 from %02X", __func__, tbuf[3]);
    goto error_exit;
  }

  tbuf[3] = 0x43;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, NONE_INTF);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Couldn't read crc2 from %02X", __func__, tbuf[3]);
    goto error_exit;
  }
#endif

error_exit:
  return ret;
}

struct dev_table {
  uint8_t addr;
  char *dev_name;
} dev_list[] = {
  {VCCIN_ADDR,    "VCCIN/VSA"},
  {VCCIO_ADDR,    "VCCIO"},
  {VDDQ_ABC_ADDR, "VDDQ_ABC"},
  {VDDQ_DEF_ADDR, "VDDQ_DEF"},
};

int dev_table_size = (sizeof(dev_list)/sizeof(struct dev_table));

static struct tool {
  uint8_t vendor;
  int (*parser)(char *);
  int (*program)(uint8_t, vr *, uint8_t);
} vr_tool[] = {
  {VR_ISL, vr_ISL_hex_parser, vr_ISL_program},
  {VR_TI , vr_TI_csv_parser , vr_TI_program},
  {VR_IFX, vr_IFX_xsf_parser, vr_IFX_program},
};

static char*
get_vr_name(uint8_t addr) {
  int i;
  for ( i = 0; i< dev_table_size; i++ ) {
    if ( addr == dev_list[i].addr ) {
      return dev_list[i].dev_name;
    }
  }
  return NULL;
}

int 
update_bic_vr(uint8_t slot_id, char *image, uint8_t force) {
  int ret = 0;
  int i = 0;
  uint8_t rbuf[6] = {0};
  uint8_t rlen = 0;
  uint8_t sel_vendor = 0;

  //step 1 - read the dev id of one of them.
  do {
    ret = bic_get_vr_device_id(slot_id, FW_CPLD, rbuf, &rlen, VR_BUS, dev_list[i].addr, NONE_INTF);
    if ( ret == 0 && rlen <= TI_DEVID_LEN/*the longest length of dev id*/) break;
  } while ( i++ < dev_table_size );

  if ( i == dev_table_size ) {
    printf("Couldn't get the devid from VRs\n");
    goto error_exit;
  }

  //step 2 - parse the image file.
  if ( rlen == IFX_DEVID_LEN ) {
    sel_vendor = VR_IFX;
  } else if (rlen == TI_DEVID_LEN ){
    sel_vendor = VR_TI;
  } else {
    sel_vendor = VR_ISL;
  }

  printf("VR vendor=%s(%x.%x) \n", (sel_vendor == VR_IFX)?"IFX":(sel_vendor == VR_TI)?"TI":"ISL", rlen, sel_vendor);

  ret = vr_tool[sel_vendor].parser(image);
  if ( ret < 0 ) {
    printf("Cannot parse the file!\n");
    goto error_exit;
  }

  //step 2.5 - check if data is existed.
  //data_cnt, addr, and devid_len cannot be 0.
  for ( i = 0; i < vr_cnt + 1; i++ ) {
    if ( vr_list[i].data_cnt == 0 || vr_list[i].addr == 0 || vr_list[i].devid_len == 0 ) {
      printf("data, addr, or devid_len is not caught!\n");
      ret = -1;
      goto error_exit;
    }
  }

  //step 3 - check DEVID
  if ( memcmp(vr_list[0].devid, rbuf, vr_list[0].devid_len) != 0 ) {
    printf("Device ID is not match!\n");
    printf(" Expected ID: ");
    for ( i = 0 ; i < vr_list[0].devid_len; i++ ) printf("%02X ", vr_list[0].devid[i]);
    printf("\n");
    printf(" Actual ID: ");
    for ( i = 0 ; i < vr_list[0].devid_len; i++ ) printf("%02X ", rbuf[i]);
    printf("\n");
    ret = -1;
    goto error_exit;
  }


  //step 4 - program
  //For now, we only support to be input 1 file.
  printf("Update %s...", get_vr_name(vr_list[0].addr));
  ret = vr_tool[sel_vendor].program(slot_id, &vr_list[0], force);

error_exit:

  return ret;
}
