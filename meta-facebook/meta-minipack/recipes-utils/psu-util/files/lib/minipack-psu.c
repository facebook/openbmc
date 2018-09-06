/*
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#include "minipack-psu.h"

static delta_hdr_t delta_hdr;

i2c_info_t psu[] = {
  {49, 0x51, 0x59, PSU1_EEPROM},
  {48, 0x50, 0x58, PSU2_EEPROM},
  {57, 0x51, 0x59, PSU3_EEPROM},
  {56, 0x50, 0x58, PSU4_EEPROM},
};

pmbus_info_t pmbus[] = {
  {"MFR_ID", 0x99},
  {"MFR_MODEL", 0x9a},
  {"MFR_REVISION", 0x9b},
  {"MFR_DATE", 0x9d},
  {"MFR_SERIAL", 0x9e},
  {"PRI_FW_VER", 0xdd},
  {"SEC_FW_VER", 0xd7},
};

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
            "Failed to open slave @ address 0x%x, errno=%d", addr,errno);
    close(fd);
    return -1;
  }

  rc = ioctl(fd, I2C_PEC, 1);
  if (rc < 0) {
    syslog(LOG_WARNING,
            "Failed to set I2C PEC @ address 0x%x, errno=%d", addr,errno);
    close(fd);
    return -1;
  }

  return fd;
}

static int
get_mfr_model(int fd, u_int8_t *block) {
  int rc = -1;

  rc = i2c_smbus_read_block_data(fd, pmbus[MFR_MODEL].reg, block);
  if (rc < 0) {
    ERR_PRINT("get_mfr_model()");
    return -1;
  }
  return 0;
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
  int size;
  struct stat st;

  stat(file_path, &st);
  size = st.st_size;

  return size;
}

static int
delta_img_hdr_parse(const char *file_path) {
  int i;
  int ret;
  int fd_file = -1;
  int index = 0;
  uint8_t hdr_buf[DELTA_HDR_LENGTH];
  uint8_t compatibilty = 0;

  fd_file = open(file_path, O_RDONLY, 0666);
  if (fd_file < 0) {
    ERR_PRINT("delta_img_hdr_parse()");
    return -1;
  }
  if (read(fd_file, hdr_buf, DELTA_HDR_LENGTH) < 0) {
    ERR_PRINT("delta_img_hdr_parse()");
    return -1;
  }

  delta_hdr.crc[0] = hdr_buf[index++];
  delta_hdr.crc[1] = hdr_buf[index++];
  delta_hdr.page_start = (hdr_buf[index++] << 8) | hdr_buf[index++];
  delta_hdr.page_end = (hdr_buf[index++] << 8) | hdr_buf[index++];
  delta_hdr.byte_per_blk = (hdr_buf[index++] << 8) | hdr_buf[index++];
  delta_hdr.blk_per_page = (hdr_buf[index++] << 8) | hdr_buf[index++];
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

  if (!strncmp(delta_hdr.fw_id, DELTA_MODEL, strlen(DELTA_MODEL))) {
    printf("Vendor: Delta\n");
    printf("Model: %s\n", delta_hdr.fw_id);
    printf("HW Compatibility: %d\n", delta_hdr.compatibility);
    printf("MCU: 0x%x\n", delta_hdr.uc);
    printf("Ver: %d.%d\n", delta_hdr.app_fw_major, delta_hdr.app_fw_minor);
    return DELTA_1500;
  } else {
    printf("Get Image Header Fail!\n");
    return -1;
  }
}

static int
delta_unlock_upgrade(int fd_i2c) {
  uint8_t block[I2C_SMBUS_BLOCK_MAX] =
          {delta_hdr.uc, delta_hdr.fw_id[10], delta_hdr.fw_id[9],
                         delta_hdr.fw_id[8], delta_hdr.fw_id[7],
                         delta_hdr.fw_id[6], delta_hdr.fw_id[5],
                         delta_hdr.fw_id[4], delta_hdr.fw_id[3],
                         delta_hdr.fw_id[2], delta_hdr.fw_id[1],
                         delta_hdr.fw_id[0], delta_hdr.compatibility};

  printf("-- Unlock Upgrade --\n");
  i2c_smbus_write_block_data(fd_i2c, UNLOCK_UPGRADE,
                                 13, block);
  return 0;
}

static int
delta_boot_flag(int fd_i2c, uint16_t mode) {
  uint16_t word = (mode << 8) | delta_hdr.uc;

  if (mode == BOOT_MODE) {
    printf("-- Bootloader Mode --\n");
  } else {
    printf("-- Reset PSU --\n");
  }
  i2c_smbus_write_word_data(fd_i2c, BOOT_FLAG, word);

  return 0;
}

static int
delta_fw_transmit(int fd_i2c, const char *path) {
  FILE* fp;
  int fw_len = 0;
  int block_total = 0;
  int byte_index = 0;
  int fw_block = (delta_hdr.uc == 0x10)
                 ? DELTA_PRI_NUM_OF_BLOCK * DELTA_PRI_NUM_OF_PAGE
                 : DELTA_SEC_NUM_OF_BLOCK * DELTA_SEC_NUM_OF_PAGE;
  uint8_t page_num_lo = (delta_hdr.uc == 0x10) ? DELTA_PRI_PAGE_START
                                               : DELTA_SEC_PAGE_START;
  uint8_t block_size = (delta_hdr.uc == 0x10) ? DELTA_PRI_NUM_OF_BLOCK
                                              : DELTA_SEC_NUM_OF_BLOCK;
  uint8_t page_num_max = (delta_hdr.uc == 0x10) ? DELTA_PRI_PAGE_END
                                                : DELTA_SEC_PAGE_END;
  uint8_t block[I2C_SMBUS_BLOCK_MAX] = {0};
  uint8_t fw_buf[16];

  fw_len = check_file_len(path) - 32;

  uint8_t fw_data[fw_len];

  fp = fopen(path, "rb");
  if (fp == NULL) {
    ERR_PRINT("delta_fw_transmit()");
    return -1;
  }
  fseek(fp, 32, SEEK_SET);
  fread(fw_data, sizeof(uint8_t), fw_len, fp);
  fclose(fp);

  printf("-- Transmit Firmware --\n");

  while (block_total <= fw_block) {
    block[0] = delta_hdr.uc;

    /* block[1] - Block Num LO
       block[2] - Block Num HI */
    if (block[1] < block_size) {
      memcpy(&fw_buf[0], &fw_data[byte_index], 16);
      memcpy(&block[3], &fw_buf, 16);
      i2c_smbus_write_block_data(fd_i2c, DATA_TO_RAM,
                                      19, block);
      if (delta_hdr.uc == 0x10) {
        msleep(60);
      } else if (delta_hdr.uc == 0x20) {
        msleep(10);
      }

      block[1]++;
      block[2] = 0;
      block_total++;
      byte_index = byte_index + 16;
      printf("-- (%d/%d) (%d%%/100%%) --\r",
                  block_total, fw_block, (100 * block_total) / fw_block);
    } else {
      block[1] = page_num_lo;
      block[2] = 0;
      i2c_smbus_write_block_data(fd_i2c, DATA_TO_FLASH,
                                      3, block);
      msleep(90);
      if (page_num_lo == page_num_max) {
        printf("\n");
        return 0;
      } else {
        page_num_lo++;
        block[1] = 0;
      }
    }
  }
  return 0;
}

static int
delta_crc_transmit(int fd_i2c) {
  uint8_t block[I2C_SMBUS_BLOCK_MAX] =
                     {delta_hdr.uc, delta_hdr.crc[0], delta_hdr.crc[1]};

  printf("-- Transmit CRC --\n");
  i2c_smbus_write_block_data(fd_i2c, CRC_CHECK, 3, block);

  return 0;
}

int
update_delta_psu(int fd, const char *file_path) {
  if (delta_img_hdr_parse(file_path) == DELTA_1500) {
    delta_unlock_upgrade(fd);
    msleep(20);
    delta_boot_flag(fd, BOOT_MODE);
    msleep(2500);
    delta_fw_transmit(fd, file_path);
    delta_crc_transmit(fd);
    msleep(1500);
    delta_boot_flag(fd, NORMAL_MODE);

    if (delta_hdr.uc == 0x10) {
      msleep(4000);
    } else if (delta_hdr.uc == 0x20) {
      msleep(2000);
    }
    printf("-- Upgrade Done --\n");
    return 0;
  } else {
    return -1;
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
    ERR_PRINT("belpower_img_hdr_parse()");
    return -1;
  }

  if (fgets(hdr_buf, sizeof(hdr_buf), fp) == NULL) {
    ERR_PRINT("belpower_img_hdr_parse()");
  }
  fclose(fp);

  if (hdr_buf[0] == 'H') {
    while (i < strlen(hdr_buf) - 4) {
      hdr_str[j++] = hex_to_byte(hdr_buf[i++], hdr_buf[i++]);
    }
    hdr_str[j] = '\0';
  }
  if (!strncmp(&hdr_str[8], BELPOWER_MODEL, 16)) {
    printf("Vendor: Belpower\n");
    printf("Model: %s\n", BELPOWER_MODEL);
    return BELPOWER_1500_NAC;
  } else {
    printf("Get Image Header Fail!\n");
    return -1;
  }
}

static int
belpower_fw_transmit(int fd_i2c, uint8_t num, const char *file_path) {
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
    ERR_PRINT("belpower_fw_transmit()");
    return -1;
  }

  printf("-- Transmit Firmware --\n");
  while (!feof(fp)) {
    fgets(file_buf, sizeof(file_buf), fp);
    while (i < strlen(file_buf) - 4) {
      if (i == 0) {
        command = file_buf[i++];
      }
      byte_buf[j++] = hex_to_byte(file_buf[i++], file_buf[i++]);
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
        if (strstr(byte_buf, "bootloader") ||
            strstr(byte_buf, "application")) {
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
        if (strcmp(byte_buf, error_text)) {
          memcpy (error_text, byte_buf, sizeof(error_text));
          printf("\n");
          printf("%s\n", error_text);
        };
        if (strstr(error_text, "Could not enter primary bootloader")) {
          while (retry) {
            printf("Retry enter primary bootloader\n");
            ret = i2c_rdwr_msg_transfer(fd_i2c, psu[num].pmbus_addr << 1,
                                primary_cmd, sizeof(primary_cmd), NULL, 0);
            sleep(5);
            ret |= i2c_rdwr_msg_transfer(fd_i2c, psu[num].pmbus_addr << 1,
                          read_cmd, 1, &word_receive, 2);
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
            ret = i2c_rdwr_msg_transfer(fd_i2c, psu[num].pmbus_addr << 1,
                                  &byte_buf[2], byte_buf[0], &byte, byte_buf[1]);
            printf("\n");
            printf("read byte:0x%.2x\n", byte);
            if (ret == 0 && byte == byte_buf[3]) {
              success = true;
            } else {
              success = false;
            }
          } else if (byte_buf[1] == 2) { /* Read word */
            ret = i2c_rdwr_msg_transfer(fd_i2c, psu[num].pmbus_addr << 1,
                          &byte_buf[2], byte_buf[0], &word_receive, byte_buf[1]);
            if (ret == 0 && word_receive[0] == byte_buf[3]
                         && word_receive[1] == byte_buf[4]) {
              success = true;
            } else {
              success = false;
            }
          }
        } else { /* Write */
          ret = i2c_rdwr_msg_transfer(fd_i2c, psu[num].pmbus_addr << 1,
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

int
update_belpower_psu(int fd, uint8_t num, const char *file_path) {
  if (belpower_img_hdr_parse(file_path) == BELPOWER_1500_NAC ) {
    belpower_fw_transmit(fd, num, file_path);
  } else {
    return -1;
  };

  return 0;
}

int
update_murata_psu(int fd, const char *file_path) {
  printf("Not implemented yet\n");
  return 0;
}

int
do_update_psu(uint8_t num, const char *file_path, const char *vendor) {
  int fd = -1, ret = -1;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];

  fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  if (fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if (vendor == NULL) {
    ret = get_mfr_model(fd, block);
    if (ret < 0) {
      printf("Cannot Get PSU Model\n");
      return -1;
    }

    if (!strncmp(block, DELTA_MODEL, strlen(DELTA_MODEL))) {
      ret = update_delta_psu(fd, file_path);
    } else if (!strncmp(block, BELPOWER_MODEL, strlen(BELPOWER_MODEL))) {
      ret = update_belpower_psu(fd, num, file_path);
    } else if (!strncmp(block, MURATA_MODEL, strlen(MURATA_MODEL))) {
      ret = update_murata_psu(fd, file_path);
    } else {
      printf("Unsupported device: %s\n", block);
      return -1;
    }
  } else {
    if (!strncasecmp(vendor, "delta", strlen("delta"))) {
      ret = update_delta_psu(fd, file_path);
    } else if (!strncasecmp(vendor, "belpower", strlen("belpower"))) {
      ret = update_belpower_psu(fd, num, file_path);
    } else if (!strncasecmp(vendor, "murata", strlen("murata"))) {
      ret = update_murata_psu(fd, file_path);
    } else {
      printf("Unsupported vendor: %s\n", vendor);
      return -1;
    }
  }
  close(fd);

  return ret;
}

/* Print the FRUID in detail */
static void
print_fruid_info(fruid_info_t *fruid, const char *name) {
  /* Print format */
  printf("%-27s: %s", "\nFRU Information",
      name /* Name of the FRU device */ );
  printf("%-27s: %s", "\n---------------", "------------------");

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

/* Populate and print fruid_info by parsing the fru's binary dump */
int
get_eeprom_info(u_int8_t num, const char *tpye) {
  int ret = -1, fd = -1;
  char name[5];
  char eeprom[16];
  fruid_info_t fruid;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];

  fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  if (fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  if (tpye == NULL) {
    if (get_mfr_model(fd, block)) {
      close(fd);
      return -1;
    }
    close(fd);

    if (!strncmp(block, DELTA_MODEL, strlen(DELTA_MODEL))) {
      snprintf(eeprom, sizeof(eeprom), "%s", "24c64");
    } else if (!strncmp(block, BELPOWER_MODEL, strlen(BELPOWER_MODEL)) ||
               !strncmp(block, MURATA_MODEL, strlen(MURATA_MODEL))) {
      snprintf(eeprom, sizeof(eeprom), "%s", "24c02");
    } else {
      printf("Unsupported device or device is not ready");
      return -1;
    }
  } else {
    if (!strncasecmp(tpye, "24c64", strlen("24c64"))) {
      snprintf(eeprom, sizeof(eeprom), "%s", "24c64");
    } else if (!strncasecmp(tpye, "24c02", strlen("24c02"))) {
      snprintf(eeprom, sizeof(eeprom), "%s", "24c02");
    } else {
      printf("Unsupported eeprom type: %s\n", tpye);
      return -1;
    }
  }
  pal_add_i2c_device(psu[num].bus, psu[num].eeprom_addr, eeprom);

  ret = fruid_parse(psu[num].eeprom_file, &fruid);

  pal_del_i2c_device(psu[num].bus, psu[num].eeprom_addr);

  snprintf(name, sizeof(name), "PSU%d", num + 1);
  if (ret) {
    printf("Failed print EEPROM info!\n");
    return -1;
  } else {
    print_fruid_info(&fruid, name);
    free_fruid_info(&fruid);
  }

  return 0;
}

int
get_psu_info(u_int8_t num) {
  int fd = -1, rc = -1;
  int i = 0, size = 0;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];
  uint16_t word = 0;

  fd = i2c_open(psu[num].bus, psu[num].pmbus_addr);
  if (fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  size = sizeof(pmbus) / sizeof(pmbus[0]);
  for (i = 0; i < size; i++) {
    if (i < size -2) {
      memset(block, 0, sizeof(block));
      rc = i2c_smbus_read_block_data(fd, pmbus[i].reg, block);
      if (rc < 0) {
        ERR_PRINT("get_psu_info()");
        return -1;
      }
      printf("%s: %s\n", pmbus[i].item, block);
    } else {
      word = i2c_smbus_read_word_data(fd, pmbus[i].reg);
      printf("%s: %d.%d\n", pmbus[i].item, (word & 0xf), ((word >> 8) & 0xf));
    }
  }
  close(fd);

  return 0;
}
