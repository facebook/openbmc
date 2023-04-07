/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <signal.h>
#include <limits.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/fruid.h>
#include <openbmc/log.h>
#include <facebook/wedge_eeprom.h>
#include "psu.h"
#include "psu-platform.h"

static delta_hdr_t delta_hdr;
static murata_hdr_t murata_hdr;
static murata2k_hdr_t murata2k_hdr;

pmbus_info_t pmbus[] = {
  {"MFR_ID", 0x99},
  {"MFR_MODEL", 0x9a},
  {"MFR_REVISION", 0x9b},
  {"MFR_DATE", 0x9d},
  {"MFR_SERIAL", 0x9e},
  {"PRI_FW_VER", 0xdd},
  {"SEC_FW_VER", 0xd7},
  {"STATUS_WORD", 0x79},
  {"STATUS_VOUT", 0x7a},
  {"STATUS_IOUT", 0x7b},
  {"STATUS_INPUT", 0x7c},
  {"STATUS_TEMP", 0x7d},
  {"STATUS_CML", 0x7e},
  {"STATUS_FAN", 0x81},
  {"STATUS_STBY_WORD", 0xd3},
  {"STATUS_VSTBY", 0xd4},
  {"STATUS_ISTBY", 0xd5},
  {"OPTN_TIME_TOTAL", 0xd8},
  {"OPTN_TIME_PRESENT", 0xd9},
};

static int g_fd = -1;

static void
exithandler(int signum) {
  printf("\nPSU update abort!\n");
  syslog(LOG_WARNING, "PSU update abort!");
  close(g_fd);
  run_command("rm /var/run/psu-util.pid");
  exit(0);
}

static int
i2c_open(uint8_t bus, uint8_t addr) {
  int fd = -1;
  int rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);

  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s, errno=%d", fn, errno);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE_FORCE, addr);
  if (rc < 0) {
    syslog(LOG_WARNING,
            "Failed to open slave @ address 0x%x, errno=%d", addr, errno);
    close(fd);
    return -1;
  }

  return fd;
}

/*
 * PMBus Linear-11 Data Format
 * X = Y*2^N
 * X is the "real world" value;
 * Y is an 11 bit, two's complement integer;
 * N is a 5 bit, two's complement integer.
 */
static float
linear_convert(uint8_t value[]) {
  uint16_t data = (value[1] << 8) | value[0];
  int y = 0, n = 0;
  float x = 0;
  uint8_t sign_y = (data >> 10) & 0x1;
  uint8_t sign_n = (data >> 15) & 0x1;

  y = sign_y ? -1 * ((~data & 0x3ff) + 1) : data & 0x3ff;
  n = sign_n ? -1 * ((~(data >> 11) & 0xf) + 1) : (data >> 11) & 0xf;
  x = (float) y * pow(2, n);

  return x;
}

static time_info_t
time_convert(uint32_t value) {
  time_info_t record;

  record.day = value / 86400;
  record.hour = (value / 3600) % 24;
  record.min = (value / 60) % 60;
  record.sec = value % 60;

  return record;
}

static int
ascii_to_hex(int ascii) {
  ascii = ascii & 0xFF;

  if (ascii >= 0x30 && ascii <= 0x39) {
    return (ascii - 0x30);      /* 0-9 */
  } else if (ascii >= 0x41 && ascii <= 0x46) {
    return (ascii - 0x41 + 10); /* A-F */
  } else if (ascii >= 0x61 && ascii <= 0x66) {
    return (ascii - 0x61 + 10); /* a-f */
  } else {
    return -1;
  }
}

static uint8_t
hex_to_byte(uint8_t hbyte, uint8_t lbyte) {
  return (ascii_to_hex(hbyte) << 4) | ascii_to_hex(lbyte);
}

static int
check_file_len(const char *file_path) {
  struct stat st;

  if (stat(file_path, &st) != 0) {
    return -1;
  }

  if (st.st_size > INT_MAX) {
    errno = EFBIG;
    return -1; /* integer overflow */
  }

  return (int)st.st_size;
}

static int
check_file_line_cnt(const char *file_path) {
  FILE *fp = fopen(file_path, "rb");
  int c, cnt = 0;

  while ((c = fgetc(fp)) != EOF) {
    if (c == '\n') {
        cnt++;
    }
  }
  fclose(fp);

  return cnt;
}

static int
check_psu_status(uint8_t num, uint8_t reg) {
  uint8_t byte;

  byte = i2c_smbus_read_byte_data(psu[num].fd, reg);

  if (byte > 32) {
    return byte;
  }

  return 0;
}

uint8_t
pec_calc(uint8_t incrc, uint8_t indata) {
  uint8_t i, crc8;

  crc8 = incrc ^ indata;
  for (i = 0; i < 8; i++) {
    if ((crc8 & 0x80) != 0) {
      crc8 <<= 1;
      crc8 ^= 0x07;
    } else {
      crc8 <<= 1;
    }
  }

  return crc8;
}

static int
delta_img_hdr_parse(const char *file_path) {
  int i, ret;
  int fd_file = -1;
  int index = 0;
  uint8_t hdr_buf[DELTA_HDR_LENGTH];

  fd_file = open(file_path, O_RDONLY, 0666);
  if (fd_file < 0) {
    ERR_PRINT("delta_img_hdr_parse()");
    return -1;
  }
  if (read(fd_file, hdr_buf, DELTA_HDR_LENGTH) < 0) {
    ERR_PRINT("delta_img_hdr_parse()");
    close(fd_file);
    return -1;
  }

  delta_hdr.crc[0] = hdr_buf[index++];
  delta_hdr.crc[1] = hdr_buf[index++];
  delta_hdr.page_start = hdr_buf[index++];
  delta_hdr.page_start |= hdr_buf[index++] << 8;
  delta_hdr.page_end = hdr_buf[index++];
  delta_hdr.page_end |= hdr_buf[index++] << 8;
  delta_hdr.byte_per_blk = hdr_buf[index++];
  delta_hdr.byte_per_blk |= hdr_buf[index++] << 8;
  delta_hdr.blk_per_page = hdr_buf[index++];
  delta_hdr.blk_per_page |= hdr_buf[index++] << 8;
  delta_hdr.uc = hdr_buf[index++];
  delta_hdr.app_fw_major = hdr_buf[index++];
  delta_hdr.app_fw_minor = hdr_buf[index++];
  delta_hdr.bl_fw_major = hdr_buf[index++];
  delta_hdr.bl_fw_minor = hdr_buf[index++];
  delta_hdr.fw_id_len = hdr_buf[index++];

  for (i = 0; i < delta_hdr.fw_id_len; i++) {
    delta_hdr.fw_id[i] = hdr_buf[index++];
  }
  delta_hdr.compatibility = hdr_buf[index];

  if (!strncmp((char *)delta_hdr.fw_id, DELTA_MODEL, strlen(DELTA_MODEL))) {
    ret = DELTA_1500;
    printf("Vendor: Delta\n");
  } else if (!strncmp((char *)delta_hdr.fw_id, DELTA_MODEL_2K,
                                        strlen(DELTA_MODEL_2K))) {
    ret = DELTA_2000;
    printf("Vendor: Delta\n");
  } else if (!strncmp((char *)delta_hdr.fw_id, LITEON_MODEL,
                                        strlen(LITEON_MODEL))) {
    ret = LITEON_1500;
    printf("Vendor: Liteon\n");
  } else if (!strncmp((char *)delta_hdr.fw_id, LITEON_MODEL_DC,
                                        strlen(LITEON_MODEL_DC))) {
    ret = LITEON_DC_48;
    printf("Vendor: Liteon\n");
  } else if (!strncmp((char *)delta_hdr.fw_id, DELTA_MODEL_DC,
                                        strlen(DELTA_MODEL_DC))) {
    ret = DELTA_DC_48;
  } else {
    printf("Get Image Header Fail!\n");
    close(fd_file);
    return -1;
  }

  printf("Model: %s\n", delta_hdr.fw_id);
  printf("HW Compatibility: %d\n", delta_hdr.compatibility);
  if (delta_hdr.uc == 0x10) {
    printf("MCU: primary\n");
  } else if (delta_hdr.uc == 0x20) {
    printf("MCU: secondary\n");
  } else if (delta_hdr.uc == 0xff) {
    printf("MCU: primary and secondary\n");
  } else {
    printf("MCU: unknown number 0x%x\n", delta_hdr.uc);
    ret = -1;
  }
  printf("Ver: %d.%d\n", delta_hdr.app_fw_major, delta_hdr.app_fw_minor);
  close(fd_file);

  return ret;
}

static int
delta_unlock_upgrade(uint8_t num) {
  uint8_t i, j;
  uint8_t block[delta_hdr.fw_id_len + 2];

  block[0] = delta_hdr.uc;
  block[delta_hdr.fw_id_len + 1] = delta_hdr.compatibility;

  for (i = 1, j = delta_hdr.fw_id_len-1; i <= delta_hdr.fw_id_len; i++, j--) {
    block[i] = delta_hdr.fw_id[j];
  }

  i2c_smbus_write_block_data(psu[num].fd, UNLOCK_UPGRADE,
                                 sizeof(block), block);
  return 0;
}

static int
delta_boot_flag(uint8_t num, uint16_t mode, uint8_t op) {
  uint16_t word = (mode << 8) | delta_hdr.uc;

  if (op == WRITE) {
    if (mode == BOOT_MODE) {
      printf("-- Bootloader Mode --\n");
    } else {
      printf("-- Reset PSU --\n");
    }
    return i2c_smbus_write_word_data(psu[num].fd, BOOT_FLAG, word);
  } else {
    return i2c_smbus_read_byte_data(psu[num].fd, BOOT_FLAG);
  }
}

static int
delta_fw_transmit(uint8_t num, const char *path) {
  FILE* fp;
  int fw_len = 0;
  int block_total = 0;
  int byte_index = 0;
  uint16_t page_num_lo = delta_hdr.page_start;
  uint16_t block_size = delta_hdr.blk_per_page;
  uint16_t page_num_max = delta_hdr.page_end;
  uint32_t fw_block = block_size * (page_num_max - page_num_lo + 1);
  uint8_t block[I2C_SMBUS_BLOCK_MAX] = {0};
  uint8_t fw_buf[16];
  uint8_t *fw_data = NULL;
  int ret = 0;

  fw_len = check_file_len(path);
  if (fw_len < 0) {
    OBMC_ERROR(errno, "failed to get %s size", path);
    return -1;
  } else if (fw_len <= 32) {
    OBMC_WARN("%s size is too small (%d < 32)\n", path, fw_len);
    return -1;
  }

  fw_len -= 32;
  fw_data = malloc(fw_len);
  if (fw_data == NULL) {
    OBMC_ERROR(errno, "failed to allocate %d bytes", fw_len);
    return -1;
  }

  fp = fopen(path, "rb");
  if (fp == NULL) {
    OBMC_ERROR(errno, "failed to open %s", path);
    ret = -1;
    goto exit;
  }
  ret = fseek(fp, 32, SEEK_SET);
  if (ret != 0) {
    OBMC_ERROR(errno, "fseek %s failed", path);
    goto exit;
  }
  ret = fread(fw_data, sizeof(*fw_data), fw_len, fp);
  if (ret != fw_len) {
    OBMC_WARN("failed to read %d items from %s\n", fw_len, path);
    ret = -1;
    goto exit;
  }

  if (delta_hdr.uc == 0x10) {
    printf("-- Transmit Primary Firmware --\n");
  } else if (delta_hdr.uc == 0x20){
    printf("-- Transmit Secondary Firmware --\n");
  } else if (delta_hdr.uc == 0xff){
    printf("-- Transmit Primary and Secondary Firmwares --\n");
  }

  while (block_total <= fw_block) {
    block[0] = delta_hdr.uc;

    /* block[1] - Block Num LO
       block[2] - Block Num HI */
    if (block[1] < block_size) {
      memcpy(&fw_buf[0], &fw_data[byte_index], 16);
      memcpy(&block[3], &fw_buf, 16);
      i2c_smbus_write_block_data(psu[num].fd, DATA_TO_RAM,
                                      19, block);
      if (delta_hdr.uc == 0x10) {
        msleep(25);
      } else if (delta_hdr.uc == 0x20) {
        msleep(5);
      } else if (delta_hdr.uc == 0xff) {
        msleep(5);
      }

      block[1]++;
      block[2] = 0;
      block_total++;
      byte_index = byte_index + 16;
      printf("-- (%d/%d) (%d%%/100%%) --\r",
                  block_total, fw_block, (100 * block_total) / fw_block);
#ifdef DEBUG
      if (delta_boot_flag(num, BOOT_MODE, READ) & 0x20) {
        printf("-- FW transmission error --\n");
        ret = -1;
        goto exit;
      }
#endif
    } else {
      block[1] = (page_num_lo & 0xff);
      block[2] = ((page_num_lo >> 8) & 0xff);
      i2c_smbus_write_block_data(psu[num].fd, DATA_TO_FLASH,
                                      3, block);
      msleep(90);
      if (page_num_lo == page_num_max) {
        printf("\n");
        goto exit;
      } else {
        page_num_lo++;
        block[1] = 0;
        block[2] = 0;
      }
    }
  }
exit:
  if (fp != NULL)
    fclose(fp);
  free(fw_data);
  return ret;
}

static int
delta_crc_transmit(uint8_t num) {
  uint8_t block[] = {delta_hdr.uc, delta_hdr.crc[0], delta_hdr.crc[1]};

  printf("-- Transmit CRC --\n");
  i2c_smbus_write_block_data(psu[num].fd, CRC_CHECK, sizeof(block), block);

  return 0;
}

static int
update_delta_psu(uint8_t num, const char *file_path,
                 uint8_t model, const char *vendor) {
  int ret = -1;

  ret = delta_img_hdr_parse(file_path);
  if (vendor == NULL) {
    if (ret != model) {
      printf("PSU and image doesn't match!\n");
      return UPDATE_SKIP;
    }
  }
  if (ret == DELTA_1500 || ret == LITEON_1500 || ret == DELTA_2000 ||
                           ret == LITEON_DC_48 || ret == DELTA_DC_48) {
    if (delta_hdr.byte_per_blk != 16) {
      printf("Image block size invalid!\n");
      return UPDATE_SKIP;
    }

    if (ioctl(psu[num].fd, I2C_PEC, 1) < 0) {
      ERR_PRINT("delta_img_hdr_parse()");
      return UPDATE_SKIP;
    }

    delta_unlock_upgrade(num);
    msleep(20);
    delta_boot_flag(num, BOOT_MODE, WRITE);
    msleep(2500);
#ifdef DEBUG
    ret = delta_boot_flag(num, BOOT_MODE, READ) & 0xf;
    if ((delta_hdr.uc == 0x10 && ret != 0x0c) ||
        (delta_hdr.uc == 0x20 && ret != 0x0d)) {
      printf("-- Set Bootloader Mode Error --\n");
      return -1;
    }
#endif
    if (delta_fw_transmit(num, file_path) < 0) {
      return -1;
    }

    delta_crc_transmit(num);
    msleep(1500);
    delta_boot_flag(num, NORMAL_MODE, WRITE);

    if (delta_hdr.uc == 0x10) {
      msleep(4000);
    } else if (delta_hdr.uc == 0x20) {
      msleep(2000);
    } else if (delta_hdr.uc == 0xff) {
      msleep(22000);
    }
#ifdef DEBUG
    ret = delta_boot_flag(num, BOOT_MODE, READ);
    if ((ret & 0x7) == 0x4) {
      if (ret & 0x80) {
        printf("-- Primary FW Identifier Error --\n");
        return -1;
      } else if (ret & 0x40) {
        printf("-- Primary CRC16 Application Checksum Wrong --\n");
        return -1;
      }
    } else if ((ret & 0x7) == 0x5) {
      if (ret & 0x80) {
        printf("-- Secondary FW Identifier Error --\n");
        return -1;
      } else if (ret & 0x40) {
        printf("-- Secondary CRC16 Application Checksum Wrong --\n");
        return -1;
      }
    }
#endif
    printf("-- Upgrade Done --\n");
    return 0;
  } else {
    return UPDATE_SKIP;
  };
}

static int
belpower_img_hdr_parse(const char *file_path) {
  FILE* fp;
  int i = 1, j = 0;
  uint8_t hdr_buf[128];
  uint8_t hdr_str[128];

  fp = fopen(file_path, "rb");
  if (fp == NULL) {
    OBMC_ERROR(errno, "%s open failed", file_path);
    return -1;
  }

  if (fgets((char *)hdr_buf, sizeof(hdr_buf), fp) == NULL) {
    OBMC_ERROR(errno, "fgets %s failed", file_path);
    fclose(fp);
    return -1;
  }

  fclose(fp);

  if (hdr_buf[0] == 'H') {
    while (i < strlen((char *)hdr_buf) - 4) {
      hdr_str[j++] = hex_to_byte(hdr_buf[i], hdr_buf[i+1]);
      i = i + 2;
    }
    hdr_str[j] = '\0';
  }
  if (strncmp(((char *)hdr_str)+8, BEL_MODEL, 16) != 0) {
    printf("Get Image Header Fail!\n");
    return -1;
  }

  printf("Vendor: Belpower\n");
  printf("Model: %s\n", BEL_MODEL);
  return BELPOWER_1500_NAC;
}

static int
belpower_fw_transmit(uint8_t num, const char *file_path) {
  FILE* fp;
  uint8_t file_buf[128];
  uint8_t byte_buf[128];
  uint8_t primary_cmd[3] = {0xC7, 0x00, 0x39};
  uint8_t read_cmd[1] = {0xC7};
  uint8_t word_receive[2];
  uint8_t byte;
  uint8_t command = 0;
  uint8_t progress = 0;
  uint8_t retry = 3;
  uint16_t delay = 0;
  char error_text[64] = {0};
  int ret = 0, i = 0, j = 0;
  bool success = true;

  fp = fopen(file_path, "rb");
  if (fp == NULL) {
    OBMC_ERROR(errno, "%s open failed", file_path);
    return -1;
  }

  printf("-- Transmit Firmware --\n");
  while (!feof(fp)) {
    if (fgets((char *)file_buf, sizeof(file_buf), fp) == NULL) {
      OBMC_ERROR(errno, "fgets %s failed", file_path);
      fclose(fp);
      return -1;
    }

    while (i < strlen((char *)file_buf) - 4) {
      if (i == 0) {
        command = file_buf[i++];
      }
      byte_buf[j++] = hex_to_byte(file_buf[i], file_buf[i+1]);
      i = i + 2;
    }
    byte_buf[j] = '\0';

    switch (command) {
      case 'H':
        break;
      case 'L':
        /* Skip if previous command was unsuccessful */
        if (!success) {
          break;
        }
        /* Log text */
        if (strstr((char *)byte_buf, "bootloader") ||
            strstr((char *)byte_buf, "application")) {
          printf("\n");
          printf("%s", byte_buf);
        }
        break;
      case 'T':
        /* Skip if previous command was unsuccessful */
        if (!success) {
          break;
        }
        delay = ((uint16_t) byte_buf[1]) << 8 | (uint16_t) byte_buf[0];
        break;
      case 'X':
        /* Skip if previous command was successful */
        if (success) {
          break;
        }
        /* Exit with error message */
        if (strcmp((char *)byte_buf, error_text)) {
          memcpy (error_text, byte_buf, sizeof(error_text));
          printf("\n");
          printf("%s\n", error_text);
        };
        if (strstr(error_text, "Could not enter primary bootloader")) {
          while (retry) {
            printf("Retry enter primary bootloader\n");
            ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                                primary_cmd, sizeof(primary_cmd), NULL, 0);
            sleep(5);
            ret |= i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                          read_cmd, 1, word_receive, 2);
            if (ret == 0 && word_receive[0] == 0x0
                         && word_receive[1] == 0x1) {
              success = true;
              retry = 0;
            } else {
              retry--;
            }
          }
        }
        break;
      case 'M':
        /* Check MD5, Skip...*/
        break;
      case 'W':
        /* Skip if previous command was unsuccessful */
        if (!success) {
          break;
        }
        /* Send command and check result if necessary */
        if (byte_buf[0] == 1) { /* Read */
          if (byte_buf[1] == 1) { /* Read byte */
            ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                                &byte_buf[2], byte_buf[0], &byte, byte_buf[1]);
            printf("\n");
            printf("read byte:0x%.2x\n", byte);
            if (ret == 0 && byte == byte_buf[3]) {
              success = true;
            } else {
              success = false;
            }
          } else if (byte_buf[1] == 2) { /* Read word */
            ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                        &byte_buf[2], byte_buf[0], word_receive, byte_buf[1]);
            if (ret == 0 && word_receive[0] == byte_buf[3]
                         && word_receive[1] == byte_buf[4]) {
              success = true;
            } else {
              success = false;
            }
          }
        } else { /* Write */
          ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                                        &byte_buf[2], byte_buf[0], NULL, 0);
          if (!ret) {
            success = true;
          } else {
            success = false;
          }
        }
        if (success && delay != 0) {
          msleep(delay);
        }
        break;
      case 'P':
        /* Skip if previous command was unsuccessful */
        if (!success) {
          break;
        }
        progress = byte_buf[0];
        break;
      default:
        /* Unrecognized command: exit */
        break;
    }
    printf("-- (%d%%/100%%) --\r", progress);
    i = 0;
    j = 0;
    retry = 3;
  }
  fclose(fp);
  printf("\n");
  printf("-- Upgrade Done --\n");

  return 0;
}

static int
update_belpower_psu(uint8_t num, const char *file_path) {
  int ret = -1;

  if (belpower_img_hdr_parse(file_path) == BELPOWER_1500_NAC) {
    ret = belpower_fw_transmit(num, file_path);
  } else {
    return UPDATE_SKIP;
  };

  return ret;
}

static int
murata_img_hdr_parse(const char *file_path) {
  FILE* fp;
  int line = 0;
  uint8_t hdr_buf[128];
  uint8_t model_shift = 8;
  uint8_t revision_shift = 11;
  uint8_t target_shift = 9;
  uint8_t unlock_shift = 9;
  uint32_t unlock = 0;

  fp = fopen(file_path, "rb");
  if (fp == NULL) {
    OBMC_ERROR(errno, "%s open failed", file_path);
    return -1;
  }

  for (line = 0; line < 6; line++) {
    if (fgets((char *)hdr_buf, sizeof(hdr_buf), fp) == NULL) {
      OBMC_ERROR(errno, "fgets %s failed", file_path);
      fclose(fp);
      return -1;
    }

    switch (line) {
      case 1:
        if (!strncmp(((char *)hdr_buf)+model_shift, MURATA_MODEL,
                                             strlen(MURATA_MODEL))) {
          printf("Vendor: Murata\n");
          printf("Model: %s\n", MURATA_MODEL);
        } else {
          printf("Get Image Header Fail!\n");
          fclose(fp);
          return -1;
        }
        break;
      case 2:
        if (!strncmp((char *)hdr_buf, "revision = ", revision_shift)) {
          printf("Ver: %c%c.%c%c\n",
                  hdr_buf[strlen((char *)hdr_buf) - 8],
                  hdr_buf[strlen((char *)hdr_buf) - 7],
                  hdr_buf[strlen((char *)hdr_buf) - 5],
                  hdr_buf[strlen((char *)hdr_buf) - 4]);
        } else {
          printf("Get Image Header Fail!\n");
          fclose(fp);
          return -1;
        }
        break;
      case 3:
        if (!strncmp(((char *)hdr_buf)+target_shift, "primary",
                                              strlen("primary"))) {
          murata_hdr.uc = 0x50;
        } else if (!strncmp(((char *)hdr_buf)+target_shift,
                                        "secondary", strlen("secondary"))) {
          murata_hdr.uc = 0x53;
        } else {
          printf("Get Image Header Fail!\n");
          fclose(fp);
          return -1;
        }
        printf("MCU: %s", &hdr_buf[target_shift]);
        break;
      case 5:
        if (!strncmp((char *)hdr_buf, "unlock = ", unlock_shift)) {
          unlock = strtoul(((char *)hdr_buf)+unlock_shift, NULL, 0);
          memcpy(&murata_hdr.unlock, &unlock, sizeof(murata_hdr.unlock));
        } else {
          printf("Get Image Header Fail!\n");
          fclose(fp);
          return -1;
        }
        break;
      default:
        break;
    }
  }
  fclose(fp);
  murata_hdr.boot_addr = 0x60;

  return MURATA_1500;
}

static int
murata_bootload_mode(uint8_t num) {
  int ret = -1, i;
  uint8_t cmd[] = {psu[num].pmbus_addr << 1, 0xfa,
                   murata_hdr.unlock[3], murata_hdr.unlock[2],
                   murata_hdr.unlock[1], murata_hdr.unlock[0]};
  uint8_t pec = 0;

  for (i = 0; i < 6; i++) {
    pec = pec_calc(pec, cmd[i]);
  }

  uint8_t block[] = {0xfa,
                   murata_hdr.unlock[3], murata_hdr.unlock[2],
                   murata_hdr.unlock[1], murata_hdr.unlock[0], pec};

  ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                                    block, sizeof(block), NULL, 0);

  return ret;
}

static int
murata_upgrade_status(int fd_i2c, uint8_t num) {
  uint8_t byte = 0xfa;
  uint8_t byte_receive = 0;

  i2c_rdwr_msg_transfer(fd_i2c, murata_hdr.boot_addr << 1,
                                  &byte, 1, &byte_receive, 1);
  return byte_receive;
}

static int
murata_end_of_file(int fd_i2c, uint8_t num) {
  int ret = -1;
  uint8_t block[] = {0xfa, 0x44, 0x00, 0x00, 0x00, 0x01, 0xff};

  ret = i2c_rdwr_msg_transfer(fd_i2c, murata_hdr.boot_addr << 1,
                                  block, sizeof(block), NULL, 0);
  printf("-- Transmit EOF --\n");

  return ret;
}

static int
murata_reset_psu(int fd_i2c, uint8_t num) {
  int ret = -1;
  uint8_t block[] = {0xf8, 0xaf};

  ret = i2c_rdwr_msg_transfer(fd_i2c, murata_hdr.boot_addr << 1,
                                  block, sizeof(block), NULL, 0);
  printf("-- Reset PSU --\n");

  return ret;
}

static int
murata_fw_transmit(uint8_t num, const char *file_path) {
  int fd = -1, i = 0, j = 0;
  int file_line_curr = 0, file_line_total = 0;
  FILE* fp;
  uint8_t file_buf[128];
  uint8_t byte_buf[128];
  uint8_t block[I2C_SMBUS_BLOCK_MAX] = {0xfa, 0x44};
  int ret = 0;

  fp = fopen(file_path, "rb");
  if (fp == NULL) {
    OBMC_ERROR(errno, "fopen %s failed", file_path);
    return -1;
  }
  file_line_total = check_file_line_cnt(file_path);

  /* When entering bootloader mode, Murata PSU PMBUS address change to 0x60 */
  fd = i2c_open(psu[num].bus, murata_hdr.boot_addr);
  if (fd < 0) {
    OBMC_ERROR(errno, "failed to open i2c %u-00%02x",
               psu[num].bus, murata_hdr.boot_addr);
    ret = -1;
    goto exit;
  }

  while (!feof(fp)) {
    if (fgets((char *)file_buf, sizeof(file_buf), fp) == NULL) {
      OBMC_ERROR(errno, "fgets %s failed", file_path);
      ret = -1;
      goto exit;
    }

    if (file_line_curr < 6) {
      /* FW header information */
    } else if (file_line_curr == 6) {
      if (!strncmp((char *)file_buf, "[data]", strlen("[data]"))) {
        if (murata_hdr.uc == 0x50) {
          printf("-- Transmit Primary Firmware --\n");
        } else if (murata_hdr.uc == 0x53){
          printf("-- Transmit Secondary Firmware --\n");
        }
      }
    } else {
      printf("-- (%d/%d) (%d%%/100%%) --\r",
             file_line_curr, file_line_total,
             (100 * file_line_curr) / file_line_total);
      while (i < strlen((char *)file_buf) - 2) {
        /* Skip first character ":" */
        if (i == 0) {
          i++;
        }
        byte_buf[j++] = hex_to_byte(file_buf[i], file_buf[i+1]);
        i = i + 2;
      }
      byte_buf[j] = '\0';
      memcpy(&block[2], &byte_buf, byte_buf[0] + 5);
      i2c_rdwr_msg_transfer(fd, murata_hdr.boot_addr << 1,
                                      block, byte_buf[0] + 7, NULL, 0);
      while (murata_upgrade_status(fd, num) == 0x55) {
        msleep(300);
      }
    }
    file_line_curr++;
    i = 0;
    j = 0;
  }
  printf("\n");

  murata_end_of_file(fd, num);
  sleep(1);
  if (murata_upgrade_status(fd, num) == 0xaa) {
    murata_reset_psu(fd, num);
  }

  printf("-- Upgrade Done --\n");

exit:
  if (fd >= 0)
    close(fd);
  if (fp != NULL)
    fclose(fp);

  return ret;
}

int
update_murata_psu(uint8_t num, const char *file_path) {
  int ret = -1;

  if(murata_img_hdr_parse(file_path) == MURATA_1500) {
    murata_bootload_mode(num);
    sleep(1);
    ret = murata_fw_transmit(num, file_path);
  } else {
    return UPDATE_SKIP;
  };

  return ret;
}

static int
murata2k_img_hdr_parse(const char *file_path) {
  int i = 0, ret = 0;
  int fd_file = -1;
  int readnum = 0;
  int index = 0;
  uint8_t hdr_buf[MURATA2K_HDR_LENGTH];

  fd_file = open(file_path, O_RDONLY, 0666);
  if (fd_file < 0) {
    OBMC_ERROR(errno, "Open file %s failed\n", file_path);
    return -1;
  }

  readnum = read(fd_file, hdr_buf, sizeof(hdr_buf));
  if (readnum < sizeof(hdr_buf)) {
    OBMC_ERROR(errno, "Read file %d failed\n", fd_file);
    close(fd_file);
    return -1;
  }

  memset(&murata2k_hdr, 0, sizeof(murata2k_hdr));
  murata2k_hdr.crc[0] = hdr_buf[index++];
  murata2k_hdr.crc[1] = hdr_buf[index++];
  murata2k_hdr.page_start = hdr_buf[index++];
  murata2k_hdr.page_start |= hdr_buf[index++] << 8;
  murata2k_hdr.page_end = hdr_buf[index++];
  murata2k_hdr.page_end |= hdr_buf[index++] << 8;
  murata2k_hdr.byte_per_blk = hdr_buf[index++];
  murata2k_hdr.byte_per_blk |= hdr_buf[index++] << 8;
  murata2k_hdr.blk_per_page = hdr_buf[index++];
  murata2k_hdr.blk_per_page |= hdr_buf[index++] << 8;
  murata2k_hdr.uc = hdr_buf[index++];
  murata2k_hdr.app_fw_major = hdr_buf[index++];
  murata2k_hdr.app_fw_minor = hdr_buf[index++];
  murata2k_hdr.bl_fw_major = hdr_buf[index++];
  murata2k_hdr.bl_fw_minor = hdr_buf[index++];
  murata2k_hdr.fw_id_len = hdr_buf[index++];

  if (MURATA2K_FWID_LENGTH != murata2k_hdr.fw_id_len) {
    OBMC_WARN("Error FWID length: %d!\n", murata2k_hdr.fw_id_len);
    close(fd_file);
    return -1;
  }

  for (i = 0; i < murata2k_hdr.fw_id_len; i++) {
    murata2k_hdr.fw_id[i] = hdr_buf[index++];
  }
  murata2k_hdr.compatibility = hdr_buf[index];

  if (index >= sizeof(hdr_buf)) {
    OBMC_WARN("Buffer hdr_buf overflow, index: %d, hdr_buf size: %d!\n",
              index, sizeof(hdr_buf));
    close(fd_file);
    return -1;
  }

  if (!strncmp((char *)murata2k_hdr.fw_id,
      MURATA_FWID_2K, strlen(MURATA_FWID_2K))) {
    ret = MURATA_2000;
    OBMC_INFO("Vendor: Murata\n");
  } else {
    OBMC_WARN("Get image header fail, error FW ID: %s!\n", murata2k_hdr.fw_id);
    close(fd_file);
    return -1;
  }

  OBMC_INFO("FW ID: %s\n", murata2k_hdr.fw_id);
  OBMC_INFO("HW Compatibility: %d\n", murata2k_hdr.compatibility);
  if (murata2k_hdr.uc == 0x10) {
    OBMC_INFO("MCU: primary\n");
  } else if (murata2k_hdr.uc == 0x20) {
    OBMC_INFO("MCU: secondary\n");
  } else {
    OBMC_WARN("MCU: unknown number 0x%x\n", murata2k_hdr.uc);
    ret = -1;
  }
  OBMC_INFO("Ver: %d.%d\n", murata2k_hdr.app_fw_major,
                                    murata2k_hdr.app_fw_minor);
  close(fd_file);

  return ret;
}

static int
murata2k_unlock_upgrade(uint8_t num) {
  uint8_t i = 0, j = 0;
  uint8_t block[murata2k_hdr.fw_id_len + 2];

  block[0] = murata2k_hdr.uc;
  block[murata2k_hdr.fw_id_len + 1] = murata2k_hdr.compatibility;

  for (i = 1, j = murata2k_hdr.fw_id_len - 1;
       i <= murata2k_hdr.fw_id_len; i++, j--) {
    block[i] = murata2k_hdr.fw_id[j];
  }

  i2c_smbus_write_block_data(psu[num].fd, UNLOCK_UPGRADE, sizeof(block), block);
  return 0;
}

static int
murata2k_boot_flag(uint8_t num, uint16_t mode, uint8_t op) {
  uint16_t word = (mode << 8) | murata2k_hdr.uc;

  if (op == WRITE) {
    if (mode == BOOT_MODE) {
      OBMC_INFO("-- Bootloader Mode --\n");
    } else {
      OBMC_INFO("-- Reset PSU --\n");
    }
    return i2c_smbus_write_word_data(psu[num].fd, BOOT_FLAG, word);
  } else {
    return i2c_smbus_read_byte_data(psu[num].fd, BOOT_FLAG);
  }
}

static int
murata2k_fw_transmit(uint8_t num, const char *path) {
  FILE* fp = NULL;
  int fw_len = 0;
  int block_total = 0;
  int byte_index = 0;
  uint16_t page_num_lo = murata2k_hdr.page_start;
  uint16_t block_size = murata2k_hdr.blk_per_page;
  uint16_t page_num_max = murata2k_hdr.page_end;
  uint32_t fw_block = block_size * (page_num_max - page_num_lo + 1);
  uint8_t block[MURATA2K_BYTE_PER_BLK + 3] = {0};
  uint8_t fw_buf[MURATA2K_BYTE_PER_BLK];
  uint8_t *fw_data = NULL;
  int ret = 0;

  fw_len = check_file_len(path);
  if (fw_len < 0) {
    OBMC_ERROR(errno, "failed to get %s size\n", path);
    return -1;
  } else if (fw_len <= MURATA2K_HDR_LENGTH) {
    OBMC_WARN("%s size is too small (%d < 32)\n", path, fw_len);
    return -1;
  }

  fw_len -= MURATA2K_HDR_LENGTH;
  fw_data = malloc(fw_len);
  if (fw_data == NULL) {
    OBMC_ERROR(errno, "failed to allocate %d bytes\n", fw_len);
    return -1;
  }

  fp = fopen(path, "rb");
  if (fp == NULL) {
    OBMC_ERROR(errno, "failed to open %s\n", path);
    ret = -1;
    goto exit;
  }
  ret = fseek(fp, MURATA2K_HDR_LENGTH, SEEK_SET);
  if (ret != 0) {
    OBMC_ERROR(errno, "fseek %s failed\n", path);
    goto exit;
  }
  ret = fread(fw_data, sizeof(*fw_data), fw_len, fp);
  if (ret != fw_len) {
    OBMC_WARN("failed to read %d items from %s\n", fw_len, path);
    ret = -1;
    goto exit;
  }

  if (murata2k_hdr.uc == 0x10) {
    OBMC_INFO("-- Transmit Primary Firmware --\n");
  } else if (murata2k_hdr.uc == 0x20) {
    OBMC_INFO("-- Transmit Secondary Firmware --\n");
  }

  while (block_total <= fw_block) {
    block[0] = murata2k_hdr.uc;

    /* block[1] - Block Num LO
       block[2] - Block Num HI */
    if (block[1] < block_size) {
      memcpy(&fw_buf[0], &fw_data[byte_index], MURATA2K_BYTE_PER_BLK);
      memcpy(&block[3], &fw_buf, MURATA2K_BYTE_PER_BLK);
      i2c_smbus_write_block_data(psu[num].fd, DATA_TO_RAM,
                                 sizeof(block), block);
      if (murata2k_hdr.uc == 0x10) {
        msleep(60);
      } else if (murata2k_hdr.uc == 0x20) {
        msleep(10);
      }

      block[1]++;
      block[2] = 0;
      block_total++;
      byte_index = byte_index + MURATA2K_BYTE_PER_BLK;
      /* This printf can not be replaced by OBMC_INFO
      because OBMC_INFO treats the \r as \n */
      printf("-- (%d/%d) (%d%%/100%%) --\r",
             block_total, fw_block, (100 * block_total) / fw_block);
    } else {
      block[1] = (page_num_lo & 0xff);
      block[2] = ((page_num_lo >> 8) & 0xff);
      i2c_smbus_write_block_data(psu[num].fd, DATA_TO_FLASH, 3, block);
      msleep(90);
      if (page_num_lo == page_num_max) {
        OBMC_INFO("\n");
        goto exit;
      } else {
        page_num_lo++;
        block[1] = 0;
        block[2] = 0;
      }
    }
  }
exit:
  if (fp != NULL)
    fclose(fp);
  free(fw_data);
  return ret;
}

static int
murata2k_crc_transmit(uint8_t num) {
  uint8_t block[] = {murata2k_hdr.uc,
                     murata2k_hdr.crc[0],
                     murata2k_hdr.crc[1]};

  OBMC_INFO("-- Transmit CRC --\n");
  i2c_smbus_write_block_data(psu[num].fd, CRC_CHECK, sizeof(block), block);

  return 0;
}

static int
update_murata2k_psu(uint8_t num, const char *file_path) {
  int ret = -1;

  ret = murata2k_img_hdr_parse(file_path);
  if (ret == MURATA_2000) {
    if (murata2k_hdr.byte_per_blk != MURATA2K_BYTE_PER_BLK) {
      OBMC_WARN("Image block size %d invalid!\n", murata2k_hdr.byte_per_blk);
      return UPDATE_SKIP;
    }

    if (ioctl(psu[num].fd, I2C_PEC, 1) < 0) {
      OBMC_ERROR(errno, "psu%d ioctl error!\n", num);
      return UPDATE_SKIP;
    }

    murata2k_unlock_upgrade(num);
    msleep(20);
    murata2k_boot_flag(num, BOOT_MODE, WRITE);
    msleep(2500);
    if (murata2k_fw_transmit(num, file_path) < 0) {
      return -1;
    }

    murata2k_crc_transmit(num);
    msleep(1500);
    murata2k_boot_flag(num, NORMAL_MODE, WRITE);

    if (murata2k_hdr.uc == 0x10) {
      msleep(4000);
    } else if (murata2k_hdr.uc == 0x20) {
      msleep(2000);
    }
    OBMC_INFO("-- Upgrade Done --\n");
    return 0;
  } else {
    return UPDATE_SKIP;
  };
}

int
is_psu_prsnt(uint8_t num, uint8_t *status) {

  return pal_is_fru_prsnt(num + FRU_PSU1, status);
}

int
get_mfr_model(uint8_t num, uint8_t *block) {
  int rc = -1;
  uint8_t psu_num = num + 1;

  if (check_psu_status(num, pmbus[MFR_MODEL].reg)) {
    printf("PSU%d get status fail!\n", psu_num);
    return -1;
  }

  rc = i2c_smbus_read_block_data(psu[num].fd, pmbus[MFR_MODEL].reg, block);
  if (rc < 0) {
    ERR_PRINT("get_mfr_model()");
    return -1;
  }
  return 0;
}

int
do_update_psu(uint8_t num, const char *file_path, const char *vendor) {
  int ret = -1;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1] = {0};

  signal(SIGHUP, exithandler);
  signal(SIGINT, exithandler);
  signal(SIGTERM, exithandler);
  signal(SIGQUIT, exithandler);

  psu[num].fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  g_fd = psu[num].fd;
  if (psu[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    ret = UPDATE_SKIP;
    goto update_exit;
  }

  sensord_operation(num, STOP);
  if (vendor == NULL) {
    ret = get_mfr_model(num, block);
    if (ret < 0) {
      printf("Cannot Get PSU Model\n");
      ret = UPDATE_SKIP;
      goto update_exit;
    }

    if (!strncmp((char *)block, DELTA_MODEL, strlen(DELTA_MODEL))) {
      ret = update_delta_psu(num, file_path, DELTA_1500, vendor);
    } else if (!strncmp((char *)block, DELTA_MODEL_2K,
                                          strlen(DELTA_MODEL_2K))) {
      ret = update_delta_psu(num, file_path, DELTA_2000, vendor);
    } else if (!strncmp((char *)block, LITEON_MODEL, strlen(LITEON_MODEL))) {
      ret = update_delta_psu(num, file_path, LITEON_1500, vendor);
    } else if (!strncmp((char *)block, LITEON_MODEL_DC,
                                          strlen(LITEON_MODEL_DC))) {
      ret = update_delta_psu(num, file_path, LITEON_DC_48, vendor);
    } else if (!strncmp((char *)block, DELTA_MODEL_DC,
                                          strlen(DELTA_MODEL_DC))) {
      ret = update_delta_psu(num, file_path, DELTA_DC_48, vendor);
    } else if (!strncmp((char *)block, BEL_MODEL, strlen(BEL_MODEL))) {
      ret = update_belpower_psu(num, file_path);
    } else if (!strncmp((char *)block, MURATA_MODEL, strlen(MURATA_MODEL))) {
      ret = update_murata_psu(num, file_path);
    } else if (!strncmp((char *)block, MURATA_MODEL_2K,
               strlen(MURATA_MODEL_2K))) {
      ret = update_murata2k_psu(num, file_path);
    } else {
      printf("Unsupported device: %s\n", block);
      ret = UPDATE_SKIP;
    }
  } else {
    if (!strncasecmp(vendor, "delta", strlen("delta"))) {
      ret = update_delta_psu(num, file_path, DELTA_1500, vendor);
    } else if (!strncasecmp(vendor, "2k-delta", strlen("2k-delta"))){
      ret = update_delta_psu(num, file_path, DELTA_2000, vendor);
    } else if (!strncasecmp(vendor, "liteon", strlen("liteon"))){
      ret = update_delta_psu(num, file_path, LITEON_1500, vendor);
    } else if (!strncasecmp(vendor, "belpower", strlen("belpower"))) {
      ret = update_belpower_psu(num, file_path);
    } else if (!strncasecmp(vendor, "murata", strlen("murata"))) {
      ret = update_murata_psu(num ,file_path);
    } else if (!strncasecmp(vendor, "2k-murata", strlen("2k-murata"))) {
      ret = update_murata2k_psu(num ,file_path);
    } else if (!strncasecmp(vendor, "48-liteon", strlen("48-liteon"))) {
      ret = update_delta_psu(num, file_path, LITEON_DC_48, vendor);
    } else if (!strncasecmp(vendor, "48-delta", strlen("48-delta"))) {
      ret = update_delta_psu(num, file_path, DELTA_DC_48, vendor);
    } else {
      printf("Unsupported vendor: %s\n", vendor);
      ret = UPDATE_SKIP;
    }
  }

update_exit:
  if (ret == 0 || ret == UPDATE_SKIP) {
    sensord_operation(num, START);
  }
  close(psu[num].fd);
  run_command("rm /var/run/psu-util.pid");

  return ret;
}

/* Print the FRUID in detail */
static void
print_fruid_info(fruid_info_t *fruid, uint8_t num) {
  /* Print format */
  printf("%-27s: PSU%d (Bus:%d Addr:0x%x)", "\nFRU Information",
                          num + 1, psu[num].bus, psu[num].eeprom_addr);
  printf("%-27s: %s", "\n---------------", "-----------------------");

  if (fruid->chassis.flag) {
    printf("%-27s: %s", "\nChassis Type",fruid->chassis.type_str);
    printf("%-27s: %s", "\nChassis Part Number",fruid->chassis.part);
    printf("%-27s: %s", "\nChassis Serial Number",fruid->chassis.serial);
    if (fruid->chassis.custom1 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 1",fruid->chassis.custom1);
    if (fruid->chassis.custom2 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 2",fruid->chassis.custom2);
    if (fruid->chassis.custom3 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 3",fruid->chassis.custom3);
    if (fruid->chassis.custom4 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 4",fruid->chassis.custom4);
  }

  if (fruid->board.flag) {
    printf("%-27s: %s", "\nBoard Mfg Date",fruid->board.mfg_time_str);
    printf("%-27s: %s", "\nBoard Mfg",fruid->board.mfg);
    printf("%-27s: %s", "\nBoard Product",fruid->board.name);
    printf("%-27s: %s", "\nBoard Serial",fruid->board.serial);
    printf("%-27s: %s", "\nBoard Part Number",fruid->board.part);
    printf("%-27s: %s", "\nBoard FRU ID",fruid->board.fruid);
    if (fruid->board.custom1 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 1",fruid->board.custom1);
    if (fruid->board.custom2 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 2",fruid->board.custom2);
    if (fruid->board.custom3 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 3",fruid->board.custom3);
    if (fruid->board.custom4 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 4",fruid->board.custom4);
  }

  if (fruid->product.flag) {
    printf("%-27s: %s", "\nProduct Manufacturer",fruid->product.mfg);
    printf("%-27s: %s", "\nProduct Name",fruid->product.name);
    printf("%-27s: %s", "\nProduct Part Number",fruid->product.part);
    printf("%-27s: %s", "\nProduct Version",fruid->product.version);
    printf("%-27s: %s", "\nProduct Serial",fruid->product.serial);
    printf("%-27s: %s", "\nProduct Asset Tag",fruid->product.asset_tag);
    printf("%-27s: %s", "\nProduct FRU ID",fruid->product.fruid);
    if (fruid->product.custom1 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 1",fruid->product.custom1);
    if (fruid->product.custom2 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 2",fruid->product.custom2);
    if (fruid->product.custom3 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 3",fruid->product.custom3);
    if (fruid->product.custom4 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 4",fruid->product.custom4);
  }

  printf("\n");
}


static int
get_ac_psu_eeprom_info(uint8_t num) {
  int ret = -1;
  char eeprom[16];
  fruid_info_t fruid;

  snprintf(eeprom, sizeof(eeprom), "%s", "24c02");
  i2c_add_device(psu[num].bus, psu[num].eeprom_addr, eeprom);
  ret = fruid_parse(psu[num].eeprom_file, &fruid);
  i2c_delete_device(psu[num].bus, psu[num].eeprom_addr);

  if (ret) {
    snprintf(eeprom, sizeof(eeprom), "%s", "24c64");
    i2c_add_device(psu[num].bus, psu[num].eeprom_addr, eeprom);
    ret = fruid_parse(psu[num].eeprom_file, &fruid);
    i2c_delete_device(psu[num].bus, psu[num].eeprom_addr);
    if (ret) {
      printf("Failed print EEPROM info!\n");
      return -1;
    }
  }
  print_fruid_info(&fruid, num);
  free_fruid_info(&fruid);
  return 0;
}

static void
print_dc_psu_fru_eeprom(i2c_info_t *psu_info,
              struct wedge_eeprom_st fruid, uint8_t num) {
    /* Print format */
  printf("%-27s: PSU%d (Bus:%d Addr:0x%x)", "\nFRU Information",
                          num + 1, (psu_info+num)->bus,
                          (psu_info+num)->eeprom_addr);
  printf("%-27s: %s", "\n---------------", "-----------------------");
  printf("\n%-32s: %d", "Version", fruid.fbw_version);
  printf("\n%-32s: %s", "Product Name", fruid.fbw_product_name);
  printf("\n%-32s: %s", "Product Part Number", fruid.fbw_product_number);
  printf("\n%-32s: %s", "System Assembly Part Number",
                                      fruid.fbw_assembly_number);
  printf("\n%-32s: %s", "Facebook PCBA Part Number",
                                      fruid.fbw_facebook_pcba_number);
  printf("\n%-32s: %s", "Facebook PCB Part Number",
                                      fruid.fbw_facebook_pcb_number);
  printf("\n%-32s: %s", "ODM PCBA Part Number",
                                      fruid.fbw_odm_pcba_number);
  printf("\n%-32s: %s", "ODM PCBA Serial Number", fruid.fbw_odm_pcba_serial);
  printf("\n%-32s: %d", "Product Production State", fruid.fbw_production_state);
  printf("\n%-32s: %d", "Product Version", fruid.fbw_product_version);
  printf("\n%-32s: %d", "Product Sub-Version", fruid.fbw_product_subversion);
  printf("\n%-32s: %s", "Product Serial Number", fruid.fbw_product_serial);
  printf("\n%-32s: %s", "Product Asset Tag", fruid.fbw_product_asset);
  printf("\n%-32s: %s", "System Manufacturer", fruid.fbw_system_manufacturer);
  printf("\n%-32s: %s", "System Manufacturing Date",
         fruid.fbw_system_manufacturing_date);
  printf("\n%-32s: %s", "PCB Manufacturer", fruid.fbw_pcb_manufacturer);
  printf("\n%-32s: %s", "Assembled At", fruid.fbw_assembled);
  printf("\n%-32s: %02X:%02X:%02X:%02X:%02X:%02X", "Local MAC",
         fruid.fbw_local_mac[0], fruid.fbw_local_mac[1],
         fruid.fbw_local_mac[2], fruid.fbw_local_mac[3],
         fruid.fbw_local_mac[4], fruid.fbw_local_mac[5]);
  printf("\n%-32s: %02X:%02X:%02X:%02X:%02X:%02X", "Extended MAC Base",
         fruid.fbw_mac_base[0], fruid.fbw_mac_base[1],
         fruid.fbw_mac_base[2], fruid.fbw_mac_base[3],
         fruid.fbw_mac_base[4], fruid.fbw_mac_base[5]);
  printf("\n%-32s: %d", "Extended MAC Address Size", fruid.fbw_mac_size);
  printf("\n%-32s: %s", "Location on Fabric", fruid.fbw_location);
  printf("\n%-32s: 0x%x", "CRC8", fruid.fbw_crc8);
  printf("\n");
}

static int
get_dc_psu_eeprom_info(i2c_info_t *psu_info,uint8_t num) {
  int ret = -1;
  char eeprom[16];

  struct wedge_eeprom_st fruid_pem;
  snprintf(eeprom, sizeof(eeprom), "%s", "24c02");
  i2c_add_device((psu_info+num)->bus, (psu_info+num)->eeprom_addr, eeprom);
  ret = wedge_eeprom_parse((psu_info+num)->eeprom_file, &fruid_pem);
  i2c_delete_device((psu_info+num)->bus, (psu_info+num)->eeprom_addr);
  printf("print EEPROM info ret = 0x%x!\n", ret);
  if (ret) {
    snprintf(eeprom, sizeof(eeprom), "%s", "24c64");
    i2c_add_device((psu_info+num)->bus, (psu_info+num)->eeprom_addr, eeprom);
    ret = wedge_eeprom_parse((psu_info+num)->eeprom_file, &fruid_pem);
    i2c_delete_device((psu_info+num)->bus, (psu_info+num)->eeprom_addr);
    if (ret) {
      printf("Failed print EEPROM info ret = 0x%x!\n", ret);
      return -1;
    }
  }

  print_dc_psu_fru_eeprom(psu_info, fruid_pem, num);

  return 0;
}

/* Populate and print fruid_info by parsing the fru's binary dump */
int
get_eeprom_info(uint8_t num) {
  int ret = -1;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];

  psu[num].fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  g_fd = psu[num].fd;
  if (psu[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if (get_mfr_model(num, block)) {
    close(psu[num].fd);
    return -1;
  }

  if (!strncmp((char *)block, LITEON_MODEL_DC, strlen(LITEON_MODEL_DC)) ||
    !strncmp((char *)block, DELTA_MODEL_DC, strlen(DELTA_MODEL_DC))) {
    ret = get_dc_psu_eeprom_info(psu, num);
  }
  else {
    ret = get_ac_psu_eeprom_info(num);
  }
  close(psu[num].fd);
  return ret;
}

int
get_psu_info(uint8_t num) {
  int rc = -1;
  int i = 0, size = 0;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];
  uint8_t byte;
  uint16_t word = 0;
  uint32_t optn_time = 0;
  time_info_t optn;

  psu[num].fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  g_fd = psu[num].fd;
  if (psu[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if (get_mfr_model(num, block)) {
    close(psu[num].fd);
    return -1;
  }

  if (!strncmp((char *)block, DELTA_MODEL, strlen(DELTA_MODEL)) ||
      !strncmp((char *)block, DELTA_MODEL_2K, strlen(DELTA_MODEL_2K)) ||
      !strncmp((char *)block, LITEON_MODEL, strlen(LITEON_MODEL)) ||
      !strncmp((char *)block, MURATA_MODEL, strlen(MURATA_MODEL)) ||
      !strncmp((char *)block, MURATA_MODEL_2K, strlen(MURATA_MODEL_2K)) ||
      !strncmp((char *)block, LITEON_MODEL_DC, strlen(LITEON_MODEL_DC)) ||
      !strncmp((char *)block, DELTA_MODEL_DC, strlen(DELTA_MODEL_DC))) {
    size = OPTN_TIME_PRESENT;
  } else {
    size = STATUS_FAN;
  }

  printf("\n%-26s: PSU%d (Bus:%d Addr:0x%x)", "PSU Information",
                              num + 1, psu[num].bus, psu[num].pmbus_addr);
  printf("\n%-26s: %s", "---------------", "-----------------------");
  for (i = MFR_ID; i <= size; i++) {
    switch (i) {
      case MFR_ID:
      case MFR_MODEL:
      case MFR_REVISION:
      case MFR_DATE:
      case MFR_SERIAL:
        printf("\n%-18s (0x%X) : ", pmbus[i].item, pmbus[i].reg);
        if (check_psu_status(num, pmbus[i].reg)) {
          printf("NA");
        } else {
          memset(block, 0, sizeof(block));
          rc = i2c_smbus_read_block_data(psu[num].fd, pmbus[i].reg, block);
          if (rc < 0) {
            printf("NA");
          } else {
            printf("%s", block);
          }
        }
        break;
      case PRI_FW_VER:
      case SEC_FW_VER:
        word = i2c_smbus_read_word_data(psu[num].fd, pmbus[i].reg);
        printf("\n%-18s (0x%X) : %d.%d", pmbus[i].item, pmbus[i].reg,
                                (word & 0xff), ((word >> 8) & 0xff));
        break;
      case STATUS_WORD:
      case STATUS_STBY_WORD:
        word = i2c_smbus_read_word_data(psu[num].fd, pmbus[i].reg);
        printf("\n%-18s (0x%X) : 0x%04x", pmbus[i].item, pmbus[i].reg, word);
        break;
      case STATUS_VOUT:
      case STATUS_IOUT:
      case STATUS_INPUT:
      case STATUS_TEMP:
      case STATUS_CML:
      case STATUS_FAN:
      case STATUS_VSTBY:
      case STATUS_ISTBY:
        byte = i2c_smbus_read_byte_data(psu[num].fd, pmbus[i].reg);
        printf("\n%-18s (0x%X) : 0x%02x", pmbus[i].item, pmbus[i].reg, byte);
        break;
      case OPTN_TIME_TOTAL:
      case OPTN_TIME_PRESENT:
        printf("\n%-18s (0x%X) : ", pmbus[i].item, pmbus[i].reg);
        if (check_psu_status(num, pmbus[i].reg)) {
          printf("NA");
        } else {
          memset(block, 0, sizeof(block));
          rc = i2c_smbus_read_block_data(psu[num].fd, pmbus[i].reg, block);
          if (rc != 4) {
            printf("NA");
          } else {
            optn_time = block[0] | block[1] << 8 |
                        block[2] << 16 | block[3] << 24;
            optn = time_convert(optn_time);
            printf("%dD:%dH:%dM:%dS", optn.day, optn.hour, optn.min, optn.sec);
          }
        }
        break;
    }
  }
  printf("\n");
  close(psu[num].fd);

  return 0;
}

int
get_blackbox_info(uint8_t num, const char *option) {
  int ret = 0;
  uint8_t psu_num = num + 1;
  uint8_t read_cmd[3] = {0xfb, 1};
  uint8_t clear_cmd[3] = {0xfb, 0xaa, 0x55};
  uint32_t time_total = 0, time_present = 0;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];
  uint8_t byte_buf[PSU_MAX_BLACKBOX_LEN+1];
  blackbox_info_t blackbox;
  time_info_t total;
  time_info_t present;

  psu[num].fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  g_fd = psu[num].fd;
  if (psu[num].fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if (get_mfr_model(num, block)) {
    close(psu[num].fd);
    return -1;
  }

  if (strncmp((char *)block, DELTA_MODEL, strlen(DELTA_MODEL)) &&
      strncmp((char *)block, DELTA_MODEL_2K, strlen(DELTA_MODEL_2K)) &&
      strncmp((char *)block, LITEON_MODEL, strlen(LITEON_MODEL)) &&
      strncmp((char *)block, MURATA_MODEL, strlen(MURATA_MODEL)) &&
      strncmp((char *)block, MURATA_MODEL_2K, strlen(MURATA_MODEL_2K)) &&
      strncmp((char *)block, LITEON_MODEL_DC, strlen(LITEON_MODEL_DC)) &&
      strncmp((char *)block, DELTA_MODEL_DC, strlen(DELTA_MODEL_DC))) {
    printf("PSU%d not support blackbox!\n", psu_num);
    close(psu[num].fd);
    return -1;
  }
 /*
  * FIXME: LITEON & DELTA 48V DC EVT1 hardware doesn't report accurate data for
  * these fields, and we will fix them step by step.
  */
  if ((!strncmp(option, "--print", strlen("--print")))) {
    for (read_cmd[2] = 0; read_cmd[2] < 5; read_cmd[2]++) {
      ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                                    read_cmd, 3, byte_buf, PSU_MAX_BLACKBOX_LEN);
      if (ret) {
        printf("PSU%d blackbox page %d read fail!\n", psu_num, read_cmd[2]);
        close(psu[num].fd);
        return -1;
      }
      memcpy(&blackbox, byte_buf, PSU_MAX_BLACKBOX_LEN);

      time_total = blackbox.optn_time_total[0] |
                   blackbox.optn_time_total[1] << 8 |
                   blackbox.optn_time_total[2] << 16 |
                   blackbox.optn_time_total[3] << 24;
      time_present = blackbox.optn_time_present[0] |
                     blackbox.optn_time_present[1] << 8 |
                     blackbox.optn_time_present[2] << 16 |
                     blackbox.optn_time_present[3] << 24;
      total = time_convert(time_total);
      present = time_convert(time_present);

      printf("%-27s: PSU%d (Bus:%d Addr:0x%x)", "\nBlackbox Information",
                                   psu_num, psu[num].bus, psu[num].pmbus_addr);
      printf("%-27s: %s", "\n--------------------", "-----------------------");
      printf("%-27s: %d", "\nPAGE", blackbox.page);
      printf("%-19s (0xDD) : %d.%d", "\nPRI_FW_VER", blackbox.pri_code_ver[0],
                                                     blackbox.pri_code_ver[1]);
      printf("%-19s (0xD7) : %d.%d", "\nSEC_FW_VER", blackbox.sec_code_ver[0],
                                                     blackbox.sec_code_ver[1]);
      printf("%-19s (0x79) : 0x%04x", "\nSTATUS_WORD",
                         (blackbox.v1_status[1] << 8) | blackbox.v1_status[0]);
      printf("%-19s (0x7A) : 0x%02x", "\nSTATUS_VOUT", blackbox.v1_status_vout);
      printf("%-19s (0x7B) : 0x%02x", "\nSTATUS_IOUT", blackbox.v1_status_iout);
      printf("%-19s (0x7C) : 0x%02x", "\nSTATUS_INPUT", blackbox.input_status);
      printf("%-19s (0x7D) : 0x%02x", "\nSTATUS_TEMP", blackbox.temp_status);
      printf("%-19s (0x7E) : 0x%02x", "\nSTATUS_CML", blackbox.cml_status);
      printf("%-19s (0x81) : 0x%02x", "\nSTATUS_FAN", blackbox.fan_status);
      printf("%-19s (0xD3) : 0x%04x", "\nSTATUS_STBY_WORD",
                       (blackbox.vsb_status[1] << 8) | blackbox.vsb_status[0]);
      printf("%-19s (0xD4) : 0x%02x", "\nSTATUS_VSTBY",
                                             blackbox.vsb_status_vout);
      printf("%-19s (0xD5) : 0x%02x", "\nSTATUS_ISTBY",
                                             blackbox.vsb_status_iout);
      printf("%-19s (0xD8) : %dD:%dH:%dM:%dS", "\nOPTN_TIME_TOTAL",
                                total.day, total.hour, total.min, total.sec);
      printf("%-19s (0xD9) : %dD:%dH:%dM:%dS", "\nOPTN_TIME_PRESENT",
                        present.day, present.hour, present.min, present.sec);
      printf("%-19s (0x88) : %.2f V", "\nIN_VOLT",
                                              linear_convert(blackbox.vin));
      printf("%-19s (0x89) : %.2f A", "\nIN_CURR",
                                              linear_convert(blackbox.iin));
      printf("%-19s (0x8B) : %.2f V", "\n12V_VOLT",
                                              linear_convert(blackbox.vout));
      printf("%-19s (0x8C) : %.2f A", "\n12V_CURR",
                                              linear_convert(blackbox.iout));
      printf("%-19s (0x8D) : %.2f C", "\nTEMP1",
                                              linear_convert(blackbox.temp1));
      printf("%-19s (0x8E) : %.2f C", "\nTEMP2",
                                              linear_convert(blackbox.temp2));
      printf("%-19s (0x8F) : %.2f C", "\nTEMP3",
                                              linear_convert(blackbox.temp3));
      printf("%-19s (0x90) : %.2f RPM", "\nFAN_SPEED",
                                              linear_convert(blackbox.fan_speed));
      // The new blackbox format supports 42 bytes count for Delta and Liteon DC48 volt.
      if((!strncmp((char *)block, DELTA_MODEL_DC, strlen(DELTA_MODEL_DC)) || 
          !strncmp((char *)block, LITEON_MODEL_DC, strlen(LITEON_MODEL_DC))) &&
          blackbox.len == 42)
      {
        printf("%-19s (0x7F) : 0x%02x", "\nSTATUS_OTHER", blackbox.status_other);
      }
    }
  } else if ((!strncmp(option, "--clear", strlen("--clear")))) {
    ret = i2c_rdwr_msg_transfer(psu[num].fd, psu[num].pmbus_addr << 1,
                                         clear_cmd, 3, NULL, 0);
    if (ret) {
      printf("PSU%d blackbox clear fail!\n", psu_num);
      close(psu[num].fd);
      return -1;
    }
  } else {
    printf("Invalid option!\n");
    close(psu[num].fd);
    return -1;
  }
  close(psu[num].fd);

  return 0;
}
