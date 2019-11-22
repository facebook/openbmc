/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
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
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "vr.h"

#define VR_BUS_ID 1

#define VR_REG_PAGE 0x00

// PXE1110C
#define VR_PXE_PAGE_20 0x20
#define VR_PXE_PAGE_3F 0x3F
#define VR_PXE_PAGE_50 0x50
#define VR_PXE_PAGE_60 0x60
#define VR_PXE_PAGE_6F 0x6F

#define VR_PXE_REG_REMAIN_WR 0x82  // page 0x50

#define VR_PXE_REG_CRC_L 0x3D  // page 0x6F
#define VR_PXE_REG_CRC_H 0x3E  // page 0x6F

#define VR_PXE_TOTAL_RW_SIZE 2040

// TPS53688
#define VR_TPS_REG_DEVID   0xAD
#define VR_TPS_REG_CRC     0xF0
#define VR_TPS_REG_NVM_IDX 0xF5
#define VR_TPS_REG_NVM_EXE 0xF6

#define VR_TPS_OFFS_CRC 10

#define VR_TPS_FIRST_BLK_LEN 21
#define VR_TPS_BLK_LEN       32
#define VR_TPS_BLK_WR_LEN    (VR_TPS_BLK_LEN+1)
#define VR_TPS_LAST_BLK_LEN  9
#define VR_TPS_NVM_IDX_NUM   9
#define VR_TPS_DEVID_LEN     6
#define VR_TPS_TOTAL_WR_SIZE (VR_TPS_BLK_WR_LEN * VR_TPS_NVM_IDX_NUM)

#define MAX_IMAGE_BUF_SIZE 2048
#define MAX_RETRY 3

static int get_pxe_crc(uint8_t addr, char *key, char *checksum);
static int get_tps_crc(uint8_t addr, char *key, char *checksum);
static int check_exp_image(uint32_t crc_exp, uint8_t *data);
static int check_tps_image(uint32_t crc_exp, uint8_t *data);
static int program_pxe(uint8_t addr, uint64_t devid_exp, uint32_t crc_exp, uint8_t *data);
static int program_tps(uint8_t addr, uint64_t devid_exp, uint32_t crc_exp, uint8_t *data);

struct vr_dev {
  uint8_t addr;
  char *dev_name;
  int (*get_checksum)(uint8_t, char *, char *);
  int (*validate_image)(uint32_t, uint8_t *);
  int (*program_image)(uint8_t, uint64_t, uint32_t, uint8_t *);
};

static struct vr_dev m_vr_list[] = {
  {ADDR_PCH_PVNN,      "PVNN/P1V05",       get_pxe_crc, check_exp_image, program_pxe},
  {ADDR_CPU0_VCCIN,    "CPU0_VCCIN/VCCSA", get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU0_VCCIO,    "CPU0_VCCIO",       get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU0_VDDQ_ABC, "CPU0_VDDQ_ABC",    get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU0_VDDQ_DEF, "CPU0_VDDQ_DEF",    get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU1_VCCIN,    "CPU1_VCCIN/VCCSA", get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU1_VCCIO,    "CPU1_VCCIO",       get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU1_VDDQ_ABC, "CPU1_VDDQ_ABC",    get_tps_crc, check_tps_image, program_tps},
  {ADDR_CPU1_VDDQ_DEF, "CPU1_VDDQ_DEF",    get_tps_crc, check_tps_image, program_tps},
};

static size_t m_vr_cnt = sizeof(m_vr_list)/sizeof(m_vr_list[0]);

static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while (nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

static int
i2c_open(void) {
  int fd;
  char fn[16];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed", __func__, fn);
  }

  return fd;
}

static int
i2c_io(int fd, uint8_t addr, uint8_t *tbuf, uint8_t tcnt, uint8_t *rbuf, uint8_t rcnt) {
  int ret = -1;
  int retry = MAX_RETRY;
  uint8_t buf[64];

  if (tcnt > sizeof(buf)) {
    syslog(LOG_WARNING, "%s: unexpected write length %u", __func__, tcnt);
    return -1;
  }

  while (retry > 0) {
    memcpy(buf, tbuf, tcnt);
    ret = i2c_rdwr_msg_transfer(fd, addr, buf, tcnt, rbuf, rcnt);
    if (ret) {
      syslog(LOG_WARNING, "%s: i2c rw failed for bus#%d, dev 0x%x", __func__, VR_BUS_ID, addr);
      retry--;
      msleep(100);
      continue;
    }

    msleep(1);  // delay between transactions
    break;
  }

  return ret;
}

static int
get_pxe_crc(uint8_t addr, char *key, char *checksum) {
  int fd, ret = -1;
  uint8_t tbuf[16], rbuf[16];

  if ((fd = i2c_open()) < 0) {
    return -1;
  }

  do {
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_6F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_PXE_REG_CRC_L;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      break;
    }

    tbuf[0] = VR_PXE_REG_CRC_H;
    if ((ret = i2c_io(fd, addr, tbuf, 1, &rbuf[2], 2))) {
      break;
    }

    snprintf(checksum, MAX_VALUE_LEN, "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
    kv_set(key, checksum, 0, 0);
  } while (0);

  close(fd);
  return ret;
}

static int
get_tps_crc(uint8_t addr, char *key, char *checksum) {
  int fd, ret = -1;
  uint8_t tbuf[16], rbuf[16];

  if ((fd = i2c_open()) < 0) {
    return -1;
  }

  do {
    tbuf[0] = VR_TPS_REG_CRC;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      break;
    }

    snprintf(checksum, MAX_VALUE_LEN, "%02X%02X", rbuf[1], rbuf[0]);
    kv_set(key, checksum, 0, 0);
  } while (0);

  close(fd);
  return ret;
}

int
vr_fw_version(uint8_t vr, char *ver_str) {
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  if (vr >= m_vr_cnt) {
    return -1;
  }

  snprintf(key, sizeof(key), "vr_%02xh_crc", m_vr_list[vr].addr);
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (m_vr_list[vr].get_checksum(m_vr_list[vr].addr, key, tmp_str))
      return -1;
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "Version: %s", tmp_str) >
      (MAX_VER_STR_LEN-1))
    return -1;

  return 0;
}

static int
parse_file(char *path, uint8_t *addr, uint64_t *devid_exp, uint32_t *crc_exp, uint8_t *data) {
  FILE *fp;
  char line[200], xdigit[8] = {0};
  char *token, *str;
  int i, idx = 0;

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return -1;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    token = strtok(line, ",");
    if (!token) {
      continue;
    }

    if (!strcmp(token, "Comment")) {
      if ((token = strtok(NULL, ",")) && (token = strstr(token, "IC_DEVICE_ID"))) {
        if ((token = strstr(token, "0x")) && (str = strtok(token, " ")))
          *devid_exp = strtoll(str, NULL, 16);
      }
    } else if (!strcmp(token, "BlockWrite")) {
      for (i = 0; i < 3; i++) {
        if (!(token = strtok(NULL, ",")))
          break;

        if (*addr == 0x00) {
          *addr = strtol(token, NULL, 16);
        }
      }
      if (!token) {
        continue;
      }

      token += 2;
      for (i = 0; i <= (VR_TPS_BLK_LEN*2); i += 2) {
        strncpy(xdigit, &token[i], 2);
        data[idx++] = strtol(xdigit, NULL, 16);
        if (idx > VR_TPS_TOTAL_WR_SIZE) {
          syslog(LOG_WARNING, "%s: unexpected image size", __func__);
          fclose(fp);
          return -1;
        }
      }

      if (idx == VR_TPS_BLK_WR_LEN) {  // USER_NVM_INDEX 0
        memcpy(crc_exp, &data[VR_TPS_OFFS_CRC], 2);
      }
    } else if (!strncmp(token, "//E1110C", 8)) {
      if ((str = strtok(token, " -")) && (str = strtok(NULL, " -"))) {
        *addr = strtol(str, NULL, 16) >> 1;
      }

      if (str && (str = strtok(NULL, " -"))) {
        *crc_exp = strtoll(str, NULL, 16);
      }
    } else if (strstr(token, "//ReadWriteWord")) {
      if ((str = strtok(token, " \t"))) {
        i = strtol(str, NULL, 16);
        memcpy(&data[idx], &i, 2);
      }

      if (str && (str = strtok(NULL, " \t"))) {
        i = strtol(str, NULL, 2);
        memcpy(&data[idx+2], &i, 2);
      }

      idx += 4;
      if (idx > VR_PXE_TOTAL_RW_SIZE) {
        syslog(LOG_WARNING, "%s: unexpected image size", __func__);
        fclose(fp);
        return -1;
      }
    }
  }

  fclose(fp);
  return 0;
}

static uint32_t
cal_crc32(uint8_t *data, int len) {
  uint32_t crc = 0xFFFFFFFF;
  int i, b;

  for (i = 0; i < len; i++) {
    crc ^= data[i];
    for (b = 0; b < 32; b++) {
      if (crc & 0x80000000) {
        crc = (crc << 1) ^ 0x04C11DB7;  // polynomial 0x04C11DB7
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

static uint16_t
cal_crc16(uint8_t *data, int len) {
  uint16_t crc = 0x0000;
  int i, b;

  for (i = 0; i < len; i++) {
    for (b = 0; b < 8; b++) {
      if (((crc & 0x8000) >> 8) ^ ((data[i] << b) & 0x80)) {
        crc = (crc << 1) ^ 0x8005;  // polynomial 0x8005
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

static int
check_exp_image(uint32_t crc_exp, uint8_t *data) {
  uint8_t raw[1024];
  uint32_t crc;
  int i, idx = 0;

  for (i = 2, idx = 0; i < VR_PXE_TOTAL_RW_SIZE; i += 4, idx += 2) {
    memcpy(&raw[idx], &data[i], 2);
  }

  if (crc_exp != (crc = cal_crc32(raw, idx))) {
    syslog(LOG_WARNING, "%s: CRC %08X mismatch, expect %08X", __func__, crc, crc_exp);
    return -1;
  }

  printf("File CRC : %08X\n", crc);
  return 0;
}

static int
check_tps_image(uint32_t crc_exp, uint8_t *data) {
  uint8_t raw[256];
  uint16_t crc;
  int i, idx = 0;

  // the data to calculate CRC
  memcpy(&raw[idx], &data[VR_TPS_BLK_WR_LEN-VR_TPS_FIRST_BLK_LEN], VR_TPS_FIRST_BLK_LEN);
  idx += VR_TPS_FIRST_BLK_LEN;
  for (i = 1; i < 8; i++) {
    memcpy(&raw[idx], &data[VR_TPS_BLK_WR_LEN*i+1], VR_TPS_BLK_LEN);
    idx += VR_TPS_BLK_LEN;
  }
  memcpy(&raw[idx], &data[VR_TPS_BLK_WR_LEN*i+1], VR_TPS_LAST_BLK_LEN);
  idx += VR_TPS_LAST_BLK_LEN;

  if (crc_exp != (crc = cal_crc16(raw, idx))) {
    syslog(LOG_WARNING, "%s: CRC %04X mismatch, expect %04X", __func__, crc, crc_exp);
    return -1;
  }

  printf("File CRC : %04X\n", crc);
  return 0;
}

static int
program_pxe(uint8_t addr, uint64_t devid_exp, uint32_t crc_exp, uint8_t *data) {
  int fd, i, ret = -1;
  uint8_t tbuf[32], rbuf[32], remain = 0, page = 0;
  uint32_t crc;
  float dsize, next_prog;

  if ((fd = i2c_open()) < 0) {
    return -1;
  }

  do {
    // check remaining writes
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_50;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_PXE_REG_REMAIN_WR;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read remaining writes failed", __func__);
      break;
    }
    remain = (((rbuf[1] << 8) | rbuf[0]) >> 6) & 0x3F;

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_20;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    printf("Remaining writes: %u\n", remain);
    if (!remain) {
      syslog(LOG_WARNING, "%s: no remaining writes", __func__);
      ret = -1;
      break;
    }

    // unlock registers
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_3F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x27;
    tbuf[1] = 0x7C;
    tbuf[2] = 0xB3;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x2A;
    tbuf[1] = 0xB2;
    tbuf[2] = 0x8A;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    // write configuration data
    dsize = (float)VR_PXE_TOTAL_RW_SIZE/100;
    next_prog = dsize;
    for (i = 0; i < VR_PXE_TOTAL_RW_SIZE; i+= 4) {
      if (page != ((data[i+1] >> 1) & 0x3F)) {
        page = (data[i+1] >> 1) & 0x3F;
        tbuf[0] = VR_REG_PAGE;
        tbuf[1] = page;
        if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
          syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
          break;
        }
      }

      tbuf[0] = data[i];  // offset
      tbuf[1] = data[i+2];
      tbuf[2] = data[i+3];
      if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
        syslog(LOG_WARNING, "%s: write data failed, page=%02X offset=%02X data=%02X%02X",
                            __func__, page, tbuf[0], tbuf[2], tbuf[1]);
        break;
      }

      if ((i + 4) >= (int)next_prog) {
        printf("\rupdated: %d %%  ", (int)(next_prog/dsize));
        fflush(stdout);
        next_prog += dsize;
      }
    }
    printf("\n");
    if (ret) {
      break;
    }

    // lock registers
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_3F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x2A;
    tbuf[1] = 0x00;
    tbuf[2] = 0x00;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: lock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    // save configuration to NVM
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_3F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x29;
    tbuf[1] = 0xD7;
    tbuf[2] = 0xEF;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x2C;  // clear fault
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x34;  // upload from the registers to NVM
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    msleep(500);

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_60;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x01;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    if ((rbuf[0] & 0x01)) {
      syslog(LOG_WARNING, "%s: unexpected status, reg%02X=%02X", __func__, tbuf[0], rbuf[0]);
      ret = -1;
      break;
    }

    tbuf[0] = 0x02;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    if ((rbuf[0] & 0x0A)) {
      syslog(LOG_WARNING, "%s: unexpected status, reg%02X=%02X", __func__, tbuf[0], rbuf[0]);
      ret = -1;
      break;
    }

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_3F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x29;
    tbuf[1] = 0x00;
    tbuf[2] = 0x00;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: lock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    // check CRC
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_3F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x27;
    tbuf[1] = 0x7C;
    tbuf[2] = 0xB3;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x32;  // download from the NVM to registers
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    msleep(10);

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_6F;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_PXE_REG_CRC_L;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = VR_PXE_REG_CRC_H;
    if ((ret = i2c_io(fd, addr, tbuf, 1, &rbuf[2], 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    memcpy(&crc, rbuf, 4);
    if (crc != crc_exp) {
      printf("checksum %04X mismatch, expect %04X\n", crc, crc_exp);
      ret = -1;
    }
  } while (0);

  close(fd);
  return ret;
}

static int
program_tps(uint8_t addr, uint64_t devid_exp, uint32_t crc_exp, uint8_t *data) {
  int fd, i, ret = -1;
  uint8_t tbuf[64], rbuf[64];
  uint16_t crc;
  uint32_t offset = 0, dsize;
  uint64_t devid = 0x00;

  if ((fd = i2c_open()) < 0) {
    return -1;
  }

  do {
    // check device ID
    tbuf[0] = VR_TPS_REG_DEVID;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, VR_TPS_DEVID_LEN+1))) {
      syslog(LOG_WARNING, "%s: read IC_DEVICE_ID failed", __func__);
      break;
    }
    for (i = 0; i < VR_TPS_DEVID_LEN; i++) {
      ((uint8_t *)&devid)[i] = rbuf[VR_TPS_DEVID_LEN-i];
    }

    if (memcmp(&devid_exp, &devid, VR_TPS_DEVID_LEN)) {
      syslog(LOG_WARNING, "%s: IC_DEVICE_ID %llx mismatch, expect %llx", __func__, devid, devid_exp);
      ret = -1;
      break;
    }

    // set USER_NVM_INDEX to 0
    tbuf[0] = VR_TPS_REG_NVM_IDX;
    tbuf[1] = 0x00;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write NVM index failed", __func__);
      break;
    }

    // write configuration data
    dsize = VR_TPS_TOTAL_WR_SIZE/10;
    for (i = 0; i < VR_TPS_NVM_IDX_NUM; i++) {
      tbuf[0] = VR_TPS_REG_NVM_EXE;
      memcpy(&tbuf[1], &data[offset], VR_TPS_BLK_WR_LEN);
      if ((ret = i2c_io(fd, addr, tbuf, VR_TPS_BLK_WR_LEN+1, rbuf, 0))) {
        break;
      }

      offset += VR_TPS_BLK_WR_LEN;
      printf("\rupdated: %d %%  ", (offset/dsize)*10);
      fflush(stdout);
    }
    printf("\n");
    if (ret) {
      break;
    }

    msleep(200);

    // check CRC
    tbuf[0] = VR_TPS_REG_CRC;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read CRC failed", __func__);
      break;
    }

    memcpy(&crc, rbuf, 2);
    if (crc != crc_exp) {
      printf("CRC %04X mismatch, expect %04X\n", crc, crc_exp);
      ret = -1;
    }
  } while (0);

  close(fd);
  return ret;
}

int
vr_fw_update(char *path) {
  int ret, i;
  uint8_t buf[MAX_IMAGE_BUF_SIZE] = {0};
  uint8_t addr = 0x00;
  uint32_t crc_exp = 0x00;
  uint64_t devid_exp = 0x00;

  // addr: slave address of device
  // devid_exp: expected device ID
  // crc_exp: expected CRC
  ret = parse_file(path, &addr, &devid_exp, &crc_exp, buf);
  if (ret) {
    syslog(LOG_WARNING, "%s: parse %s failed", __func__, path);
    return -1;
  }

  addr <<= 1;  // 8-bit slave address
  for (i = 0; i < m_vr_cnt; i++) {
    if (m_vr_list[i].addr == addr) {
      printf("Update VR: %s\n", m_vr_list[i].dev_name);
      ret = m_vr_list[i].validate_image(crc_exp, buf);
      if (ret) {
        break;
      }

      ret = m_vr_list[i].program_image(addr, devid_exp, crc_exp, buf);
      if (ret) {
        break;
      }

      snprintf((char *)buf, sizeof(buf), "/tmp/cache_store/vr_%02xh_crc", m_vr_list[i].addr);
      unlink((char *)buf);
      break;
    }
  }
  if (i >= m_vr_cnt) {
    syslog(LOG_WARNING, "%s: device not found", __func__);
    return -1;
  }

  return ret;
}
