/*
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#include <limits.h>
#include <openbmc/fruid.h>
#include <openbmc/log.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <signal.h>

#include "elbert-psu.h"

static delta_hdr_t delta_hdr;

i2c_info_t psu[] = {
    {-1, 24, 0x58},
    {-1, 25, 0x58},
    {-1, 26, 0x58},
    {-1, 27, 0x58},
};

static int g_fd = -1;

static void exithandler(int signum) {
  printf("\nPSU update abort!\n");
  syslog(LOG_WARNING, "PSU update abort!");
  close(g_fd);
  run_command("rm /var/run/psu-util.pid");
  exit(0);
}

static int i2c_open(uint8_t bus, uint8_t addr) {
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
    syslog(
        LOG_WARNING,
        "Failed to open slave @ address 0x%x, errno=%d",
        addr,
        errno);
    close(fd);
    return -1;
  }

  return fd;
}

static int check_file_len(const char* file_path) {
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

static int check_psu_status(uint8_t num, uint8_t reg) {
  uint8_t byte;

  byte = i2c_smbus_read_byte_data(psu[num].fd, reg);

  if (byte > 32) {
    return byte;
  }

  return 0;
}

static int delta_img_hdr_parse(const char* file_path) {
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

  for (i = 0; i < DELTA_ID_LEN; i++) {
    delta_hdr.fw_id[i] = hdr_buf[index++];
  }

  delta_hdr.compatibility = hdr_buf[index++];
  delta_hdr.sec_data_start = hdr_buf[index++];
  delta_hdr.sec_data_start |= hdr_buf[index++] << 8;
  index += RESERVED_LINE_0;
  delta_hdr.pri_fw_major = hdr_buf[index++];
  delta_hdr.pri_fw_minor = hdr_buf[index++];
  delta_hdr.pri_crc[0] = hdr_buf[index++];
  delta_hdr.pri_crc[1] = hdr_buf[index++];
  index += RESERVED_LINE_1;
  delta_hdr.sec_fw_major = hdr_buf[index++];
  delta_hdr.sec_fw_minor = hdr_buf[index++];
  delta_hdr.sec_crc[0] = hdr_buf[index++];
  delta_hdr.sec_crc[1] = hdr_buf[index];

  if (!strncmp((char*)delta_hdr.fw_id, DELTA_MODEL, strlen(DELTA_MODEL))) {
    ret = DELTA_2400;
    printf("Vendor: Delta\n");
  } else {
    printf("Get Image Header Fail!\n");
    close(fd_file);
    return -1;
  }

  printf("Model: %s\n", delta_hdr.fw_id);
  printf("HW Compatibility: %d\n", delta_hdr.compatibility);
  printf(
      "Primary Ver: %d.%d\n", delta_hdr.pri_fw_major, delta_hdr.pri_fw_minor);
  printf(
      "Secondary Ver: %d.%d\n", delta_hdr.sec_fw_major, delta_hdr.sec_fw_minor);
  close(fd_file);

  return ret;
}

static int delta_unlock_upgrade(uint8_t num) {
  uint8_t i;
  uint8_t block[DELTA_ID_LEN + 1];

  block[DELTA_ID_LEN] = delta_hdr.compatibility;
  for (i = 0; i < DELTA_ID_LEN; i++) {
    block[i] = delta_hdr.fw_id[i];
  }

  /* Make sure to enable all commands in case previous upgrading failed. */
  i2c_smbus_write_byte_data(psu[num].fd, WRITE_PROTECT, ENABLE_ALL_CMDS);
  i2c_smbus_write_block_data(psu[num].fd, UNLOCK_UPGRADE, sizeof(block), block);
  return 0;
}

static int delta_boot_flag(uint8_t num, uint8_t mode, uint8_t op) {
  int data;

  if (op == WRITE) {
    if (mode == BOOT_MODE) {
      printf("-- Bootloader Mode --\n");
    } else {
      printf("-- Reset PSU --\n");
    }
    return i2c_smbus_write_byte_data(psu[num].fd, BOOT_FLAG, mode);
  } else {
    data = i2c_smbus_read_byte_data(psu[num].fd, BOOT_FLAG);
    return data;
  }
}

static int delta_crc_check(int num, int section) {
  uint8_t* file_crc;
  uint8_t crc[2];
  int raw_crc;
  int ret = 0;

  if (section == PRIMARY)
    file_crc = delta_hdr.pri_crc;
  else
    file_crc = delta_hdr.sec_crc;

  raw_crc = i2c_smbus_read_word_data(psu[num].fd, CRC_CHECK);
  crc[0] = raw_crc & 0xff;
  crc[1] = (raw_crc >> 8) & 0xff;
  if ((crc[0] != file_crc[0]) || (crc[1] != file_crc[1])) {
    OBMC_ERROR(
        errno,
        "CRC check failed %x %x != %x %x",
        crc[0],
        crc[1],
        file_crc[0],
        file_crc[1]);
    ret = -1;
  }

  return ret;
}

static int
delta_fw_transmit_line(uint8_t num, int line_num, uint8_t** data, int delay) {
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 2] = {0};
  int ret = 0;

  /* block[0] - Line Num LO
   * block[1] - Line Num HI */
  block[0] = line_num & 0xff;
  block[1] = (line_num >> 8) & 0xff;
  memcpy(&block[2], *data, BYTES_PER_LINE);
  i2c_smbus_write_block_data(
      psu[num].fd, DATA_TO_RAM, BYTES_PER_LINE + 2, block);
  msleep(delay);
  *data += BYTES_PER_LINE;
#ifdef DEBUG
  ret = delta_boot_flag(num, BOOT_MODE, READ);
  if (ret & 0x20) {
    printf("-- FW transmission error --\n");
    ret = -1;
  } else {
    ret = 0;
  }
#endif
  return ret;
}

static int delta_fw_transmit_section(
    uint8_t num,
    int start_line,
    int end_line,
    uint8_t** data,
    int delay) {
  int line_num;
  int lines_uploaded = 0;
  int total_lines;
  int ret = 0;

  total_lines = end_line - start_line;
  for (line_num = start_line; line_num < end_line; line_num++) {
    ret = delta_fw_transmit_line(num, line_num, data, delay);
    if (ret)
      return -1;
    lines_uploaded++;
    printf(
        "-- (%d/%d) (%d%%/100%%) --\r",
        lines_uploaded,
        total_lines,
        (100 * lines_uploaded / total_lines));
  }

  printf("\n");
  if (start_line < NUM_HEADER_LINES)
    msleep(POST_HEADER_DELAY);
  else {
    i2c_smbus_write_byte(psu[num].fd, DATA_TO_FLASH);
    msleep(FLASH_DELAY);

    if (start_line < delta_hdr.sec_data_start)
      ret = delta_crc_check(num, PRIMARY);
    else
      ret = delta_crc_check(num, SECONDARY);
  }

  return ret;
}

static int delta_fw_transmit(uint8_t num, const char* path) {
  FILE* fp;
  int fw_len = 0;
  uint8_t* fw_data = NULL;
  uint8_t* byte_ptr;
  int ret = 0;

  fw_len = check_file_len(path);
  if (fw_len < 0) {
    OBMC_ERROR(errno, "failed to get %s size", path);
    return -1;
  } else if (fw_len <= DELTA_HDR_LENGTH) {
    OBMC_WARN(
        "%s size is too small (%d < %d)\n", path, fw_len, DELTA_HDR_LENGTH);
    return -1;
  }

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

  ret = fread(fw_data, sizeof(*fw_data), fw_len, fp);
  if (ret != fw_len) {
    OBMC_WARN("failed to read %d items from %s\n", fw_len, path);
    ret = -1;
    goto exit;
  }
  byte_ptr = fw_data;

  printf("-- Transmit Header --\n");
  ret = delta_fw_transmit_section(
      num, 0, NUM_HEADER_LINES, &byte_ptr, HEADER_DELAY);
  if (ret)
    goto exit;

  printf("-- Transmit Primary Firmware --\n");
  ret = delta_fw_transmit_section(
      num,
      NUM_HEADER_LINES,
      delta_hdr.sec_data_start,
      &byte_ptr,
      PRIMARY_DELAY);
  if (ret)
    goto exit;

  printf("-- Transmit Secondary Firmware --\n");
  ret = delta_fw_transmit_section(
      num,
      delta_hdr.sec_data_start,
      fw_len / BYTES_PER_LINE,
      &byte_ptr,
      SECONDARY_DELAY);
  if (ret)
    goto exit;

exit:
  if (fp != NULL)
    fclose(fp);
  free(fw_data);
  return ret;
}

static int update_delta_psu(
    uint8_t num,
    const char* file_path,
    uint8_t model,
    const char* vendor) {
  int ret = -1;

  ret = delta_img_hdr_parse(file_path);
  if (vendor == NULL) {
    if (ret != model) {
      printf("PSU and image don't match!\n");
      return UPDATE_SKIP;
    }
  }
  if (ret == DELTA_2400) {
    if (ioctl(psu[num].fd, I2C_PEC, 1) < 0) {
      ERR_PRINT("update_delta_psu()");
      return UPDATE_SKIP;
    }

    delta_unlock_upgrade(num);
    msleep(UNLOCK_DELAY);
    delta_boot_flag(num, BOOT_MODE, WRITE);
    msleep(BOOT_FLAG_DELAY);

    ret = delta_boot_flag(num, BOOT_MODE, READ);
    if ((ret & 0xf) == 0xd) {
      /* If update failed while writing secondary firmware, then PSU
       * MCU select may be set to secondary immediately after entering
       * bootloader mode. In this case, reset state to return MCU
       * select to primary. */
      printf("-- Secondary MCU Selected, Reverting to Primary --\n");
      delta_boot_flag(num, NORMAL_MODE, WRITE);
      msleep(BOOT_FLAG_DELAY);
      delta_unlock_upgrade(num);
      msleep(UNLOCK_DELAY);
      delta_boot_flag(num, BOOT_MODE, WRITE);
      msleep(BOOT_FLAG_DELAY);
      ret = delta_boot_flag(num, BOOT_MODE, READ);
    }

    if ((ret & 0xf) != 0xc) {
      printf("-- Set Bootloader Mode Error --\n");
      return -1;
    }

    if (delta_fw_transmit(num, file_path)) {
      return -1;
    }

    delta_boot_flag(num, NORMAL_MODE, WRITE);
    msleep(BOOT_FLAG_DELAY);

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

    printf("-- Upgrade Done --\n");
    return 0;
  } else {
    return UPDATE_SKIP;
  };
}

int is_psu_prsnt(uint8_t num, uint8_t* status) {
  return pal_is_fru_prsnt(num + FRU_PSU1, status);
}

int is_psu_power_ok(uint8_t num, uint8_t* status) {
  return pal_is_psu_power_ok(num + FRU_PSU1, status);
}

int get_mfr_model(uint8_t num, uint8_t* block) {
  int rc = -1;
  uint8_t psu_num = num + 1;

  if (check_psu_status(num, MFR_MODEL)) {
    printf("PSU%d get status fail!\n", psu_num);
    return -1;
  }

  rc = i2c_smbus_read_block_data(psu[num].fd, MFR_MODEL, block);
  if (rc < 0) {
    ERR_PRINT("get_mfr_model()");
    return -1;
  }
  return 0;
}

int do_update_psu(uint8_t num, const char* file_path, const char* vendor) {
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

  if (vendor == NULL) {
    ret = get_mfr_model(num, block);
    if (ret < 0) {
      printf("Cannot Get PSU Model\n");
      ret = UPDATE_SKIP;
      goto update_exit;
    }

    if (!strncmp((char*)block, DELTA_MODEL, strlen(DELTA_MODEL))) {
      ret = update_delta_psu(num, file_path, DELTA_2400, vendor);
    } else {
      printf("Unsupported device: %s\n", block);
      ret = UPDATE_SKIP;
    }
  } else {
    if (!strncasecmp(vendor, "delta", strlen("delta"))) {
      ret = update_delta_psu(num, file_path, DELTA_2400, vendor);
    } else {
      printf("Unsupported vendor: %s\n", vendor);
      ret = UPDATE_SKIP;
    }
  }

update_exit:
  close(psu[num].fd);
  run_command("rm /var/run/psu-util.pid");

  return ret;
}
