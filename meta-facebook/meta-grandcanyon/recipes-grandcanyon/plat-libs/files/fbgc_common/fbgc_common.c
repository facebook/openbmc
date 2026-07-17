/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <facebook/fbgc_gpio.h>
#include "fbgc_common.h"
#include <openbmc/kv.h>


#ifdef CONFIG_GRANDCANYON2
// Platform signature of image: GrandCanyon V2.0
const char platform_signature[PLAT_SIG_SIZE] = {0x47, 0x72, 0x61, 0x6e, 0x64, 0x43, 0x61, 0x6e, 0x79, 0x6f, 0x6e, 0x20, 0x56, 0x32, 0x2e, 0x30};
#else
// Platform signature of image: Grand Canyon
const char platform_signature[PLAT_SIG_SIZE] = {0x47, 0x72, 0x61, 0x6e, 0x64, 0x20, 0x43, 0x61, 0x6e, 0x79, 0x6f, 0x6e, 0x20, 0x20, 0x20, 0x20};
#endif
const char* board_stage[] = {"Pre EVT", "EVT", "DVT", "PVT", "MP"};

static int
get_chassis_type_by_gpio(uint8_t *type) {
  int chassis_type_value = 0;
  gpio_value_t uic_loc_type_in = GPIO_VALUE_INVALID;
  gpio_value_t uic_rmt_type_in = GPIO_VALUE_INVALID;
  gpio_value_t scc_loc_type_0 = GPIO_VALUE_INVALID;
  gpio_value_t scc_rmt_type_0 = GPIO_VALUE_INVALID;

  uic_loc_type_in = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_UIC_LOC_TYPE_IN));
  uic_rmt_type_in = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_UIC_RMT_TYPE_IN));
  scc_loc_type_0  = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_LOC_TYPE_0));
  scc_rmt_type_0  = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_RMT_TYPE_0));

  if ((uic_loc_type_in == GPIO_VALUE_INVALID) || (uic_rmt_type_in == GPIO_VALUE_INVALID) ||
      (scc_loc_type_0 == GPIO_VALUE_INVALID)  || (scc_rmt_type_0 == GPIO_VALUE_INVALID)) {
    syslog(LOG_ERR, "%s() failed to get chassis type gpio", __func__);
    return -1;
  }

  //                 UIC_LOC_TYPE_IN   UIC_RMT_TYPE_IN   SCC_LOC_TYPE_0   SCC_RMT_TYPE_0
  // Type 5                        0                 0                0                0
  // Type 7 Headnode               0                 1                0                1

  chassis_type_value = CHASSIS_TYPE_BIT_3(uic_loc_type_in) | CHASSIS_TYPE_BIT_2(uic_rmt_type_in) |
                       CHASSIS_TYPE_BIT_1(scc_loc_type_0)  | CHASSIS_TYPE_BIT_0(scc_rmt_type_0);

  if (chassis_type_value == CHASSIS_TYPE_5_VALUE) {
    *type = CHASSIS_TYPE5;
  } else if (chassis_type_value == CHASSIS_TYPE_7_VALUE) {
    *type = CHASSIS_TYPE7;
  } else {
    syslog(LOG_ERR, "%s() Unknown chassis type.", __func__);
    return -1;
  }

  return 0;
}

#ifdef CONFIG_GRANDCANYON2
#define CHASSIS_TYPE_FAIL_SEL_MARKER "/tmp/fbgc_chassis_type_fail_sel_logged"

static void
fbgc_common_log_chassis_type_fail_once(int sku_value) {
  static bool logged = false;
  FILE *fp = NULL;
  gpio_value_t uic_loc_type_in = GPIO_VALUE_INVALID;
  gpio_value_t uic_rmt_type_in = GPIO_VALUE_INVALID;
  gpio_value_t scc_loc_type_0 = GPIO_VALUE_INVALID;
  gpio_value_t scc_rmt_type_0 = GPIO_VALUE_INVALID;

  if (logged) {
    return;
  }

  fp = fopen(CHASSIS_TYPE_FAIL_SEL_MARKER, "r");
  if (fp != NULL) {
    fclose(fp);
    logged = true;
    return;
  }

  uic_loc_type_in = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_UIC_LOC_TYPE_IN));
  uic_rmt_type_in = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_UIC_RMT_TYPE_IN));
  scc_loc_type_0 = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_LOC_TYPE_0));
  scc_rmt_type_0 = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_RMT_TYPE_0));

  syslog(LOG_LOCAL0 | LOG_CRIT,
         "Chassis type detection failed. SKU=%d, GPIO pattern: "
         "UIC_LOC_TYPE_IN=%d, UIC_RMT_TYPE_IN=%d, "
         "SCC_LOC_TYPE_0=%d, SCC_RMT_TYPE_0=%d. "
         "Expected Type5=0,0,0,0 or Type7=0,1,0,1. "
         "Using default chassis type: Type5",
         sku_value,
         uic_loc_type_in,
         uic_rmt_type_in,
         scc_loc_type_0,
         scc_rmt_type_0);

  logged = true;

  fp = fopen(CHASSIS_TYPE_FAIL_SEL_MARKER, "w");
  if (fp != NULL) {
    fprintf(fp, "logged\n");
    fclose(fp);
  }
}
#endif /* CONFIG_GRANDCANYON2 */

int
fbgc_common_get_chassis_type(uint8_t *type) {
  char system_info[MAX_VALUE_LEN] = {0};
  int sku_value = 0;
  int ret = 0;

  if (type == NULL) {
    syslog(LOG_ERR, "%s(): Failed to get chassis type because of NULL parameter\n", __func__);
    return -1;
  }

  ret = kv_get(SYSTEM_INFO, system_info, NULL, KV_FPERSIST);

  if (ret < 0) { //cache not ready, get from gpio directly
    if (get_chassis_type_by_gpio(type) < 0) {
#ifdef CONFIG_GRANDCANYON2
      fbgc_common_log_chassis_type_fail_once(-1);
#else
      syslog(LOG_ERR, "%s(): Unknown chassis type.\n", __func__);
#endif
      goto error;
    } else {
      return 0;
    }
  }

  sku_value = atoi(system_info);

  if (sku_value >= MAX_SKU_VALUE) {
#ifdef CONFIG_GRANDCANYON2
    fbgc_common_log_chassis_type_fail_once(sku_value);
#else
    syslog(LOG_WARNING, "%s(): Failed to get chassis type because SKU value exceed limit, value: %d\n", __func__, sku_value);
#endif
    goto error;
  }

  //  SKU[5:0] = {UIC_ID0, UIC_ID1, UIC_TYPE0, UIC_TYPE1, UIC_TYPE2, UIC_TYPE3}
  switch ((sku_value & 0xF)) {
    case CHASSIS_TYPE_5_VALUE:
      *type = CHASSIS_TYPE5;
      break;
    case CHASSIS_TYPE_7_VALUE:
      *type = CHASSIS_TYPE7;
      break;
    default:
#ifdef CONFIG_GRANDCANYON2
      fbgc_common_log_chassis_type_fail_once(sku_value);
#else
      syslog(LOG_WARNING, "%s(): Failed to get chassis type because SKU value is wrong, value: %d\n", __func__, sku_value);
#endif
      goto error;
  }

  return 0;

error:
#ifndef CONFIG_GRANDCANYON2
  syslog(LOG_ERR, "%s(): Using default chassis type: Type5\n", __func__);
#endif
  *type = CHASSIS_TYPE5;
  return 0;
}

void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int
fbgc_common_server_stby_pwr_sts(uint8_t *val) {
  gpio_value_t pg_gpio = GPIO_VALUE_LOW;

  if (val == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *val", __func__);
    return -1;
  }

  pg_gpio = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_COMP_STBY_PG_IN));
  if (pg_gpio == GPIO_VALUE_INVALID) {
    syslog(LOG_WARNING, "%s() Can not get 12V power status via GPIO pin", __func__);
    return -1;
  } else if (pg_gpio == GPIO_VALUE_HIGH) {
    *val = STAT_12V_ON;
  } else {
    *val = STAT_12V_OFF;
  }

  return 0;
}

uint8_t
cal_crc8(uint8_t crc, uint8_t const *data, uint8_t len) {
  uint8_t const *end = NULL;
  static uint8_t const crc8_table[] =
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

  if (NULL == data) {
    return 0;
  } else {
    end = data + len;
  }

  crc &= 0xff;

  while (data < end) {
    crc = crc8_table[crc ^ *data++];
  }

  return crc;
}

uint8_t
hex_c2i(const char c) {
  if ((c >= '0') && (c <= '9')) {
    return (c - '0');
  }
  if ((c >= 'A') && (c <= 'F')) {
    return (c - 'A' + 10);
  }
  if ((c >= 'a') && (c <= 'f')) {
    return (c - 'a' + 10);
  }

  return 0xFF;
}

int
string_2_byte(const char* c) {
  uint8_t h_nibble = hex_c2i(c[0]);
  if (h_nibble > 0xF) {
    return -1;
  }
  uint8_t l_nibble = hex_c2i(c[1]);
  if (l_nibble > 0xF) {
    return -1;
  }

  return (h_nibble << 4) | (l_nibble);
}

bool
start_with(const char *s, const char *p) {
  if ((0 == s) || (0 == p)) {
    return false;
  }
  while (*p && *s && (*p == *s)) {
    p++;
    s++;
  }

  return (*p == '\0');
}

int
split(char **dst, char *src, char *delim, int max_size) {
  char *s = strtok(src, delim);
  int size = 0;

  while ((NULL != s) && (size < max_size) ) {
    *dst++ = s;
    size++;
    s = strtok(NULL, delim);
  }

  return (size == max_size) ? -1 : size;
}

int
fbgc_common_get_system_stage(uint8_t *stage) {
  if (stage == NULL) {
    syslog(LOG_WARNING, "%s(): system stage is missing", __func__);
  }
#ifdef CONFIG_GRANDCANYON2
  uint8_t board_id[BOARD_ID_PIN_NUM] =
        {GPIO_BOARD_REV_ID3, GPIO_BOARD_REV_ID2, GPIO_BOARD_REV_ID1, GPIO_BOARD_REV_ID0};
#else
  uint8_t board_id[BOARD_ID_PIN_NUM] = 
        {GPIO_BOARD_REV_ID2, GPIO_BOARD_REV_ID1, GPIO_BOARD_REV_ID0};
#endif
  int i = 0;
  int val = 0;

  *stage = 0;
  for (i = 0; i < BOARD_ID_PIN_NUM; i++) {
    val = gpio_get_value_by_shadow(fbgc_get_gpio_name(board_id[i]));
    if (val == GPIO_VALUE_INVALID) {
      syslog(LOG_WARNING, "%s(): failed to get board ID", __func__);
      return -1;
    }
    *stage = (*stage << 1) + val;
  }

  return 0;
}

int
check_image_md5_at_offset(const char* image_path, off_t start_offs,
                          int cal_size, uint32_t md5_offset, uint8_t is_masked)
{
  int fd = 0, sum = 0, byte_num = 0, ret = 0, read_bytes = 0, clear_size = 0, padding = 0;
  unsigned char read_buf[MD5_READ_BYTES] = {0};
  unsigned char md5_digest[MD5_DIGEST_LENGTH] = {0};
  EVP_MD_CTX *context = NULL;
  uint64_t sur_size = SUR_TOTAL_SIZE;

  if (image_path == NULL) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5 due to NULL parameters.", __func__);
    return -1;
  }

  if (cal_size <= 0) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5 due to wrong calculate size: %d.", __func__, cal_size);
    return -1;
  }

  fd = open(image_path, O_RDONLY);

  if (fd < 0) {
    syslog(LOG_WARNING, "%s(): failed to open %s to calculate MD5.", __func__, image_path);
    return -1;
  }

  if (lseek(fd, start_offs, SEEK_SET) != start_offs) {
    syslog(LOG_WARNING, "%s(): failed to seek %s to %lld.", __func__, image_path, (long long)start_offs);
    ret = -1;
    goto exit;
  }

  context = EVP_MD_CTX_create();
  if (!context) {
    syslog(LOG_WARNING, "%s(): failed to initialize message digest context.", __func__);
    ret = -1;
    goto exit;
  }

  if (!EVP_DigestInit_ex(context, EVP_md5(), NULL)) {
    syslog(LOG_WARNING, "%s(): failed to initialize MD5 context.", __func__);
    ret = -1;
    goto exit;
  }

  while (sum < cal_size) {
    read_bytes = MD5_READ_BYTES;
    if ((sum + MD5_READ_BYTES) > cal_size) {
      read_bytes = cal_size - sum;
    }

    byte_num = read(fd, read_buf, read_bytes);

    if (byte_num <= 0) {
      syslog(LOG_WARNING, "%s(): failed to read %s while calculating MD5.", __func__, image_path);
      ret = -1;
      goto exit;
    }

    /* Mask SUR region with 0xFF when calculating BIOS payload MD5 */
    if(is_masked){
      off_t cur_offs = start_offs + sum;

      /* Case 1: The current buffer starts before SUR but overlaps its beginning */
      if(cur_offs <= BIOS_SUR_OFFSET && (cur_offs+byte_num) > BIOS_SUR_OFFSET){

        padding = BIOS_SUR_OFFSET - cur_offs;

        if ((byte_num - padding) > sur_size){
          clear_size = sur_size;
        }else{
          clear_size = byte_num- padding;
        }

        memset(read_buf+padding, 0xFF, clear_size);
      /* Case 2: The current buffer is already inside SUR */
      }else if(cur_offs > BIOS_SUR_OFFSET && cur_offs < (BIOS_SUR_OFFSET+ sur_size)){

          clear_size = BIOS_SUR_OFFSET + sur_size - cur_offs;
          if(clear_size> byte_num){
            clear_size = byte_num;
          }

          memset(read_buf, 0xFF, clear_size);
      }
    }

    if (!EVP_DigestUpdate(context, read_buf, byte_num)) {
      syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5 of %s.", __func__, image_path);
      ret = -1;
      goto exit;
    }
    sum += byte_num;
  }

  if (!EVP_DigestFinal_ex(context, md5_digest, NULL)) {
    syslog(LOG_WARNING, "%s(): failed to calculate MD5 of %s.", __func__, image_path);
    ret = -1;
    goto exit;
  }

  // read expected MD5 from image
  memset(read_buf, 0, sizeof(read_buf));

  if (lseek(fd, md5_offset, SEEK_SET) != (off_t)md5_offset) {
    syslog(LOG_WARNING, "%s(): failed to seek expected MD5 at offset 0x%x of %s.",
           __func__, md5_offset, image_path);
    ret = -1;
    goto exit;
  }

  byte_num = read(fd, read_buf, MD5_DIGEST_LENGTH);

  if (byte_num != MD5_DIGEST_LENGTH) {
    syslog(LOG_WARNING, "%s(): failed to read the signature of  %s.", __func__, image_path);
    ret = -1;
    goto exit;
  }

  if (memcmp(md5_digest, read_buf, MD5_DIGEST_LENGTH) != 0) {
    ret = -1;
    goto exit;
  }

exit:
  EVP_MD_CTX_destroy(context);
  close(fd);
  return ret;
}

int check_image_md5(const char* image_path, int cal_size, uint32_t md5_offset)
{
  return check_image_md5_at_offset(image_path, 0, cal_size, md5_offset, 0);
}

int
check_image_signature(const char* image_path, uint32_t sig_offset) {
  int fd = 0, ret = 0, byte_num = 0;
  char read_buf[PLAT_SIG_SIZE] = {0};

  if ((image_path == NULL)) {
   syslog(LOG_WARNING, "%s(): failed to check platform signature of image due to NULL parameter.", __func__);
    ret = -1;
    goto exit;
  }

  fd = open(image_path, O_RDONLY);

  if (fd < 0 ) {
    syslog(LOG_WARNING, "%s(): failed to open %s to check platform signature.", __func__, image_path);
    ret = -1;
    goto exit;
  }

  lseek(fd, sig_offset, SEEK_SET);
  byte_num = read( fd, read_buf, sizeof(read_buf));

  if (byte_num != sizeof(read_buf)) {
    syslog(LOG_WARNING, "%s(): failed to check platform signature of %s because read failed.", __func__, image_path);
    ret = -1;
    goto exit;
  }

  if (strncmp(platform_signature, read_buf, sizeof(read_buf)) != 0) {
    ret = -1;
  }

exit:
  close(fd);
  return ret;
}

int
get_server_board_revision_id(uint8_t* board_rev_id, uint8_t board_rev_id_len) {
  int i2cfd = 0, ret = 0, retry = 0;
  uint8_t tbuf[1] = {0};
  uint8_t tlen = 0;

  if (board_rev_id == NULL) {
    syslog(LOG_WARNING, "%s(): fail to get board revision id due to NULL parameter", __func__);
    return -1;
  }

  i2cfd = i2c_cdev_slave_open(I2C_BS_FPGA_BUS, BS_FPGA_SLAVE_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "%s(): fail to open device: I2C BUS: %d", __func__, I2C_BS_FPGA_BUS);
    return i2cfd;
  }

  if (fbgc_common_is_grandcanyon2()){
    tbuf[0] = ES_FPGA_BOARD_REV_ID_OFFSET;
  }else{
    tbuf[0] = BS_FPGA_BOARD_REV_ID_OFFSET;
  }

  tlen = sizeof(tbuf);

  while (retry < MAX_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, BS_FPGA_SLAVE_ADDR, tbuf, tlen, board_rev_id, board_rev_id_len);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      ret = 0;
      break;
    }
  }

  if (fbgc_common_is_grandcanyon2()){
    *board_rev_id = (*board_rev_id >> 3) & 0x0F; //Bit[6:3]
  }


  if (retry == MAX_RETRY) {
    syslog(LOG_WARNING, "%s(): fail to read server FPGA offset: 0x%02X via i2c\n", __func__, tbuf[0]);
    ret = -1;
    goto exit;
  }

#ifndef CONFIG_GRANDCANYON2
  FILE* fp = NULL;
  char buf[MAX_SYS_CMD_RESP_LEN] = {0};
  // Add this workaround to handle the wrong BS Rev ID on PVT Barton Springs.
  if (*board_rev_id == (STAGE_DVT + 1)) {
    if ((fp = popen("/usr/local/bin/fruid-util server | grep 'Product Version' | grep 'PVT'", "r")) == NULL) {
      syslog(LOG_WARNING, "%s(): fail to get Product Version of Bartion Springs\n", __func__);
      ret = -1;
      goto exit;
    }
    if (fgets(buf, sizeof(buf), fp) != NULL) {
      *board_rev_id = (STAGE_PVT + 1);
    }
  }

  if (fp != NULL) {
    pclose(fp);
  }

#endif
exit:
  close(i2cfd);
  return ret;
}

int
fbgc_common_dev_id(char *str, uint8_t *dev) {
  if ((str == NULL) || (dev == NULL)) {
    syslog(LOG_WARNING, "%s(): Failed to get device id due to NULL parameter", __func__);
    return -1;
  }

  if (strcmp(str, "e1s0") == 0) {
    *dev = DEV_ID0_E1S;
  } else if (strcmp(str, "e1s1") == 0) {
    *dev = DEV_ID1_E1S;
  } else {
#ifdef DEBUG
    syslog(LOG_WARNING, "s%(): Wrong device name: %s", __func__, str);
#endif
    return -1;
  }

  return 0;
}

bool
fbgc_common_is_grandcanyon2() {
#ifdef CONFIG_GRANDCANYON2
  return true;
#else
  return false;
#endif
}

#define RETRY_CNT  5
#define RETRY_DELAY_SEC 1

static int
sv_status(const char *service) {
  char cmd[MAX_SYS_CMD_REQ_LEN] = {0};
  char buf[MAX_SYS_CMD_RESP_LEN] = {0};
  FILE *fp = NULL;

  snprintf(cmd, sizeof(cmd), "sv status %s 2>/dev/null", service);

  fp = popen(cmd, "r");
  if (!fp) {
    syslog(LOG_WARNING, "%s() popen failed for %s", __func__, service);
    return -1;
  }

  if (!fgets(buf, sizeof(buf), fp)) {
    pclose(fp);
    syslog(LOG_WARNING, "%s() no output for %s", __func__, service);
    return -1;
  }

  pclose(fp);

  if (strncmp(buf, "run:", 4) == 0) {
    return 1;   // service is running
  }
  if (strncmp(buf, "down:", 5) == 0) {
    return 0;   // service is stopped
  }

  syslog(LOG_WARNING, "%s() unknown status %s", __func__, service);
  return -1;
}

int
sv_control(const char *service, svc_mode_t mode) {
  char cmd[MAX_SYS_CMD_REQ_LEN] = {0};
  int ret, retry;

  if (!service) {
    return -1;
  }

  switch (mode) {
  case SV_STOP:
    snprintf(cmd, sizeof(cmd), "sv stop %s", service);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s() %s failed", __func__, cmd);
    }

    for (retry = 0; retry < RETRY_CNT; retry++) {
      sleep(RETRY_DELAY_SEC);
      ret = sv_status(service);
      if (ret == 0) {
        return SV_STOP;
      }
    }
    syslog(LOG_ERR, "service %s failed to stop", service);
    return -1;

  case SV_START:
    snprintf(cmd, sizeof(cmd), "sv start %s", service);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s() %s failed", __func__, cmd);
    }

    for (retry = 0; retry < RETRY_CNT; retry++) {
      sleep(RETRY_DELAY_SEC);
      ret = sv_status(service);
      if (ret == 1) {
        return SV_START; // running
      }
    }
    syslog(LOG_ERR, "service %s failed to start", service);
    return -1;

  case SV_STATUS:
    return sv_status(service);

  default:
    return -1;
  }
}

int
fbgc_common_get_img_sur_info(const char *img_path, uint8_t comp,
                             sur_error_proof_info_t *img_error_proof_info)
{
  FILE *fp = NULL;
  off_t sur_offset;
  struct stat file_info;
  sur_signed_info_t signed_info;
  int cal_size;
  size_t sur_size = SUR_TOTAL_SIZE;
  const char *platform = fbgc_common_is_grandcanyon2() ? GRANDCANYON2 : GRANDCANYON;

  if ((img_path == NULL) || (img_error_proof_info == NULL)) {
    return -1;
  }

  memset(img_error_proof_info, 0, sizeof(*img_error_proof_info));
  memset(&signed_info, 0, sizeof(signed_info));

  if (stat(img_path, &file_info) < 0) {
    printf("%s: failed to stat %s\n", __func__, img_path);
    return -1;
  }

  switch (comp) {
    case FW_BIOS:
      cal_size = file_info.st_size;
      sur_offset = BIOS_SUR_OFFSET;
      break;
    default:
      cal_size = file_info.st_size - sur_size;
      sur_offset = cal_size + MD5_OFFSET;
      break;
  }

  if ((size_t)file_info.st_size < (size_t)(sur_offset + sur_size)) {
    printf("%s: invalid file size\n", __func__);
    return -1;
  }

  fp = fopen(img_path, "rb");
  if (fp == NULL) {
    printf("Failed to open image: %s\n", img_path);
    return -1;
  }

  if (fseeko(fp, sur_offset, SEEK_SET) != 0) {
    printf("Failed to seek SUR offset\n");
    fclose(fp);
    return -1;
  }

  if (fread(&signed_info, 1, sur_size, fp) != sur_size) {
    printf("Failed to read Secure Update Region info\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  //MD5-2 check
  if (check_image_md5_at_offset(img_path,
                                sur_offset,
                                SUR_SIZE,
                                sur_offset+SUR_SIZE, NOT_MASKED) < 0) {
    printf("Image does not contain a valid signed SUR\n");
    return -1;
  }

  if (check_image_signature(img_path, sur_offset + MD5_SIZE) < 0) {
    printf("The image is not for %s\n", platform);
    return -1;
  }

  if (comp == FW_BIOS){
      if (check_image_md5_at_offset(img_path,
                                    0,
                                    cal_size,
                                    sur_offset, MASKED) < 0) {
        printf("[%d]Image file has corrupted\n", __LINE__);
        return -1;
      }
  }else {
      if (check_image_md5(img_path, cal_size, sur_offset) < 0) {
        printf("Image file has corrupted\n");
        return -1;
      }
  }

  uint32_t err = signed_info.err_proof[0] | (signed_info.err_proof[1] << 8) | (signed_info.err_proof[2] << 16);

  img_error_proof_info->board_id   = err & 0x1F;            //[4:0]
  img_error_proof_info->fru_stage  = (err >> 5) & 0x07;     //[7:5]
  img_error_proof_info->comp_id  = (err >> 8) & 0x07;       //[10:8]

  return 0;
}

int
fbgc_common_validate_img(const char *img_path, uint8_t comp, uint8_t expected_board_id, uint8_t board_rev_id)
{
  sur_error_proof_info_t sur;

  int board_rev_is_invalid = 0;
  int expected_comp = 0;
  const char *component[] = {"Unknown", "CPLD", "BIC", "BIOS"};
  const char *rev_sb[] = {"POC", "EVT", "DVT", "PVT", "MP"};
  const char *rev_bb[] = {"Pre-EVT", "EVT", "DVT", "DVT3", "PVT", "MP"};
  const char **stage = NULL;

  switch (comp) {
    case FW_BIC_FPGA:
      expected_comp = COMP_CPLD;
      stage = rev_sb;
      break;
    case FW_BIC_RECOVERY:
    case FW_BIC:
      expected_comp = COMP_BIC;
      stage = rev_sb;
      break;
    case FW_BIOS:
      expected_comp = COMP_BIOS;
      stage = rev_sb;
      break;
    case FW_BMC_FPGA:
      expected_comp = COMP_CPLD;
      stage = rev_bb;
      break;
    default:
      syslog(LOG_WARNING, "%s(): Unknown component %d\n", __func__, comp);
      return -1;
  }


  if (fbgc_common_get_img_sur_info(img_path, comp, &sur) < 0) {
    return -1;
  }

  if (sur.comp_id >= ARRAY_SIZE(component)) {
    printf("Invalid component id: %u\n", sur.comp_id);
    return -1;
  }

  if (sur.comp_id != expected_comp) {
    printf("Not a valid %s firmware image\n", component[expected_comp]);
    return -1;
  }

  if (sur.board_id != expected_board_id) {
    printf("Unknown board id: %u \n", sur.board_id);
    return -1;
  }

  if (sur.fru_stage > STAGE_MP) {
    printf("Wrong f/w REV ID: %u", sur.fru_stage);
    return -1;
  }

  if (board_rev_id > STAGE_MP) {
    printf("Wrong board REV ID: %u\n", board_rev_id);
    return -1;
  }

  if (board_rev_id < STAGE_PVT) {
    if (sur.fru_stage != board_rev_id) {
      if(sur.fru_stage == STAGE_EVT && board_rev_id == ES_STAGE_POC){
        board_rev_is_invalid = 0;
      }else{
        board_rev_is_invalid = 1;
      }
    }
  } else {
    if (sur.fru_stage < STAGE_PVT) {
      board_rev_is_invalid = 1;
    }
  }

  if (board_rev_is_invalid) {
    printf("If you want to update the %s f/w on the %s system, please use force update.\n", stage[sur.fru_stage], stage[board_rev_id]);
    return -1;
  }

  return 0;
}

#define GPIO_NAME_TYPE_KV_KEY "gpio_name_type"
/* Raw board-ID value that indicates MP stage */
#define GPIO_NAME_TYPE_STAGE_MP_RAW 6

/* UIC location values */
#define GPIO_NAME_TYPE_UIC_SIDEA 1
#define GPIO_NAME_TYPE_UIC_SIDEB 2

/* Internal: read GPIOH2/H1/H0 to determine board stage.
 * Returns 0 on success and sets *is_mp. */
static int
gpio_name_type_read_stage(bool *is_mp) {
#ifdef CONFIG_GRANDCANYON2
  const char *pins[BOARD_ID_PIN_NUM] = {"GPIOH3", "GPIOH2", "GPIOH1", "GPIOH0"};
#else
  const char *pins[BOARD_ID_PIN_NUM] = {"GPIOH2", "GPIOH1", "GPIOH0"};
#endif
  uint8_t raw = 0;
  int i = 0;

  if (is_mp == NULL) {
    return -1;
  }

  for (i = 0; i < BOARD_ID_PIN_NUM; i++) {
    gpio_desc_t *g = gpio_open_by_name(GPIO_CHIP_ASPEED, pins[i]);
    if (!g) {
      syslog(LOG_WARNING, "%s(): open %s failed: %m", __func__, pins[i]);
      return -1;
    }

    gpio_value_t v;
    if (gpio_get_value(g, &v) != 0) {
      syslog(LOG_WARNING, "%s(): get %s value failed: %m", __func__, pins[i]);
      gpio_close(g);
      return -1;
    }
    gpio_close(g);

    raw = (raw << 1) | ((v == GPIO_VALUE_HIGH) ? 1 : 0);
  }

  *is_mp = (raw == GPIO_NAME_TYPE_STAGE_MP_RAW);
  return 0;
}

// * Internal: read GPIOY3 to determine A/B side.
// * Returns 0 on success, sets *loc to UIC_SIDEA or UIC_SIDEB. */
static int
gpio_name_type_read_uic_location(uint8_t *loc) {
  int retry = 0;

  if (loc == NULL) {
    return -1;
  }

  while (retry < 5) {
    gpio_desc_t *g = gpio_open_by_name(GPIO_CHIP_ASPEED, "GPIOY3");
    if (!g) {
      syslog(LOG_WARNING, "%s(): open GPIOY3 failed, retry %d/5: %m",
             __func__, retry + 1);
      retry++;
      sleep(1);
      continue;
    }

    gpio_value_t v;
    if (gpio_get_value(g, &v) != 0) {
      syslog(LOG_WARNING, "%s(): get GPIOY3 value failed, retry %d/5: %m",
             __func__, retry + 1);
      gpio_close(g);
      retry++;
      sleep(1);
      continue;
    }
    gpio_close(g);

    /* LOW (0) → A side, HIGH (1) → B side */
    *loc = (v == GPIO_VALUE_HIGH) ? GPIO_NAME_TYPE_UIC_SIDEB
                                  : GPIO_NAME_TYPE_UIC_SIDEA;
    return 0;
  }

  syslog(LOG_WARNING, "%s(): failed after 5 retries", __func__);
  return -1;
}

int
fbgc_common_get_gpio_name_type(gpio_name_type_t *type) {
  static gpio_name_type_t cached_type = GPIO_NAME_TYPE_UNKNOWN;
  static bool cached_valid = false;
  char kv_val[MAX_VALUE_LEN] = {0};

  if (type == NULL) {
    syslog(LOG_WARNING, "%s(): NULL parameter", __func__);
    return -1;
  }

  /*first return from process-local cache */
  if (cached_valid) {
    *type = cached_type;
    return 0;
  }

  /* Second path: check kv_store persistent cache */
  if (kv_get(GPIO_NAME_TYPE_KV_KEY, kv_val, NULL, 0) == 0) {
    int val = atoi(kv_val);
    if (val >= GPIO_NAME_TYPE_HACK && val <= GPIO_NAME_TYPE_UIC_B) {
      cached_type = (gpio_name_type_t)val;
      cached_valid = true;
      *type = cached_type;
      syslog(LOG_INFO, "%s(): loaded from kv cache: %d", __func__, val);
      return 0;
    }
  }

  bool is_mp = false;
  int stage_ret = -1;
  int retry = 0;

  for (retry = 0; retry < 3; retry++) {
    stage_ret = gpio_name_type_read_stage(&is_mp);
    if (stage_ret == 0) {
      break;
    }
    syslog(LOG_WARNING, "%s(): GPIO stage read failed, retry %d/3",
           __func__, retry + 1);
    usleep(10000);
  }

  if (stage_ret < 0) {
    /* Cannot determine stage → default to HACK for safety */
    syslog(LOG_WARNING, "%s(): GPIO stage read failed → default HACK", __func__);
    cached_type = GPIO_NAME_TYPE_HACK;
    cached_valid = true;
    *type = cached_type;
    snprintf(kv_val, sizeof(kv_val), "%d", (int)cached_type);
    kv_set(GPIO_NAME_TYPE_KV_KEY, kv_val, 0, 0);
    return 0;
  }

  if (is_mp) {
    /* MP stage → use HACK (old shadow names) */
    cached_type = GPIO_NAME_TYPE_HACK;
    syslog(LOG_INFO, "%s(): stage=MP → HACK", __func__);
  } else {
    /* DVT/EVT/PVT (non-MP) → determine A/B side */
    uint8_t uic_loc = GPIO_NAME_TYPE_UIC_SIDEA;
    if (gpio_name_type_read_uic_location(&uic_loc) < 0) {
      syslog(LOG_WARNING, "%s(): GPIOY3 read failed → default UIC-A", __func__);
      uic_loc = GPIO_NAME_TYPE_UIC_SIDEA;
    }

    if (uic_loc == GPIO_NAME_TYPE_UIC_SIDEB) {
      cached_type = GPIO_NAME_TYPE_UIC_B;
      syslog(LOG_INFO, "%s():side=B → UIC-B", __func__);
    } else {
      cached_type = GPIO_NAME_TYPE_UIC_A;
      syslog(LOG_INFO, "%s():side=A → UIC-A", __func__);
    }
  }

  cached_valid = true;
  *type = cached_type;
  snprintf(kv_val, sizeof(kv_val), "%d", (int)cached_type);
  kv_set(GPIO_NAME_TYPE_KV_KEY, kv_val, 0, 0);
  return 0;
}

static const gpio_shadow_name_map_t gpio_shadow_name_map[GPIO_SHADOW_ID_MAX] = {
  [GPIO_SHADOW_ID_COMP_PRSNT_N] = {
    .hack_name  = "COMP_PRSNT_N",
    .uic_a_name = "COMP_A_PRSNT_N",
    .uic_b_name = "COMP_B_PRSNT_N",
  },

  [GPIO_SHADOW_ID_SCC_STBY_PGOOD] = {
    .hack_name  = "SCC_STBY_PGOOD",
    .uic_a_name = "SCC_A_STBY_PGOOD",
    .uic_b_name = "SCC_B_STBY_PGOOD",
  },

  [GPIO_SHADOW_ID_SCC_FULL_PGOOD] = {
    .hack_name  = "SCC_FULL_PGOOD",
    .uic_a_name = "SCC_A_FULL_PGOOD",
    .uic_b_name = "SCC_B_FULL_PGOOD",
  },

  [GPIO_SHADOW_ID_COMP_PGOOD] = {
    .hack_name  = "COMP_PGOOD",
    .uic_a_name = "COMP_A_PGOOD",
    .uic_b_name = "COMP_B_PGOOD",
  },

  [GPIO_SHADOW_ID_E1S_1_PRSNT_N] = {
    .hack_name  = "E1S_1_PRSNT_N",
    .uic_a_name = "E1SA_1_PRSNT_N",
    .uic_b_name = "E1SB_1_PRSNT_N",
  },

  [GPIO_SHADOW_ID_E1S_2_PRSNT_N] = {
    .hack_name  = "E1S_2_PRSNT_N",
    .uic_a_name = "E1SA_2_PRSNT_N",
    .uic_b_name = "E1SB_2_PRSNT_N",
  },

  [GPIO_SHADOW_ID_I2C_E1S_1_RST_N] = {
    .hack_name  = "I2C_E1S_1_RST_N",
    .uic_a_name = "I2C_E1SA_1_RST_N",
    .uic_b_name = "I2C_E1SB_1_RST_N",
  },

  [GPIO_SHADOW_ID_I2C_E1S_2_RST_N] = {
    .hack_name  = "I2C_E1S_2_RST_N",
    .uic_a_name = "I2C_E1SA_2_RST_N",
    .uic_b_name = "I2C_E1SB_2_RST_N",
  },

  [GPIO_SHADOW_ID_E1S_1_LED_ACT] = {
    .hack_name  = "E1S_1_LED_ACT",
    .uic_a_name = "E1SA_1_LED_ACT",
    .uic_b_name = "E1SB_1_LED_ACT",
  },

  [GPIO_SHADOW_ID_E1S_2_LED_ACT] = {
    .hack_name  = "E1S_2_LED_ACT",
    .uic_a_name = "E1SA_2_LED_ACT",
    .uic_b_name = "E1SB_2_LED_ACT",
  },

  [GPIO_SHADOW_ID_SCC_STBY_PWR_EN] = {
    .hack_name  = "SCC_STBY_PWR_EN",
    .uic_a_name = "SCC_A_STBY_PWR_EN",
    .uic_b_name = "SCC_B_STBY_PWR_EN",
  },

  [GPIO_SHADOW_ID_SCC_FULL_PWR_EN] = {
    .hack_name  = "SCC_FULL_PWR_EN",
    .uic_a_name = "SCC_A_FULL_PWR_EN",
    .uic_b_name = "SCC_B_FULL_PWR_EN",
  },

  [GPIO_SHADOW_ID_BMC_EXP_SOFT_RST_N] = {
    .hack_name  = "BMC_EXP_SOFT_RST_N",
    .uic_a_name = "BMC_A_EXP_A_SOFT_RST_N",
    .uic_b_name = "BMC_B_EXP_B_SOFT_RST_N",
  },

  [GPIO_SHADOW_ID_UIC_COMP_BIC_RST_N] = {
    .hack_name  = "UIC_COMP_BIC_RST_N",
    .uic_a_name = "UIC_A_COMP_A_BIC_RST_N",
    .uic_b_name = "UIC_B_COMP_B_BIC_RST_N",
  },

  [GPIO_SHADOW_ID_E1S_1_3V3EFUSE_PGOOD] = {
    .hack_name  = "E1S_1_3V3EFUSE_PGOOD",
    .uic_a_name = "E1SA_1_3V3EFUSE_PGOOD",
    .uic_b_name = "E1SB_1_3V3EFUSE_PGOOD",
  },

  [GPIO_SHADOW_ID_E1S_2_3V3EFUSE_PGOOD] = {
    .hack_name  = "E1S_2_3V3EFUSE_PGOOD",
    .uic_a_name = "E1SA_2_3V3EFUSE_PGOOD",
    .uic_b_name = "E1SB_2_3V3EFUSE_PGOOD",
  },

  [GPIO_SHADOW_ID_P12V_NIC_STATUS_N] = {
    .hack_name  = "P12V_NIC_FAULT_N",
    .uic_a_name = "PWRGD_P12V_NIC_A",
    .uic_b_name = "PWRGD_P12V_NIC_B",
  },

  [GPIO_SHADOW_ID_P3V3_NIC_STATUS_N] = {
    .hack_name  = "P3V3_NIC_FAULT_N",
    .uic_a_name = "PWRGD_P3V3_NIC_A",
    .uic_b_name = "PWRGD_P3V3_NIC_B",
  },

  [GPIO_SHADOW_ID_SCC_POR_RST_N] = {
    .hack_name  = "SCC_POR_RST_N",
    .uic_a_name = "SCC_A_POR_RST_N",
    .uic_b_name = "SCC_B_POR_RST_N",
  },

  [GPIO_SHADOW_ID_BMC_COMP_BLED] = {
    .hack_name  = "BMC_COMP_BLED",
    .uic_a_name = "BMC_A_COMP_A_BLED",
    .uic_b_name = "BMC_B_COMP_B_BLED",
  },
};

const char *
fbgc_common_get_gpio_shadow_name(gpio_shadow_id_t id) {
  gpio_name_type_t name_type = GPIO_NAME_TYPE_HACK;
  const gpio_shadow_name_map_t *map;

  if (id >= GPIO_SHADOW_ID_MAX) {
    return NULL;
  }

  map = &gpio_shadow_name_map[id];

  if (map->hack_name == NULL ||
      map->uic_a_name == NULL ||
      map->uic_b_name == NULL) {
    return NULL;
  }

  if (fbgc_common_get_gpio_name_type(&name_type) < 0) {
    return map->hack_name;
  }

  switch (name_type) {
    case GPIO_NAME_TYPE_UIC_A:
      return map->uic_a_name;
    case GPIO_NAME_TYPE_UIC_B:
      return map->uic_b_name;
    case GPIO_NAME_TYPE_HACK:
    default:
      return map->hack_name;
  }
}