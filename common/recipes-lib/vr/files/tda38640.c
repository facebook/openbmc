#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "tda38640.h"

#define CHECKSUM_FIELD "[Image 00] :"
#define IMAGE_COUNT "Image Count :"
#define CNFG_TAG "[CNFG]"
#define DATA_END_TAG "[End]"
#define DATA_COMMENT "//"

#define DATA_LEN_IN_LINE 17
#define CNFG_REMAINING_WRITES_MAX  5
#define USER_REMAINING_WRITES_MAX  48
#define VR_PROGRAM_DELAY 200  //200 ms
#define VR_PROGRAM_RECHECK 3

extern int vr_rdwr(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t) = &vr_rdwr;

static int
tda38640_set_page(uint8_t bus, uint8_t addr, uint8_t page) {
  uint8_t set_page_tbuf[] = {PAGE_REG, page};

  int ret = vr_xfer(bus, addr, set_page_tbuf, 2, NULL, 0);
  if (ret < 0) {
    return -1;
  }
  return 0;
}

static int
tda38640_unlock_reg(uint8_t bus, uint8_t addr, bool unlock) {
  uint8_t unlock_regs_tbuf[] = {UNLOCK_REGS_REG, 0x3};

  if (unlock == false) {
    unlock_regs_tbuf[1] = 0x7;
  }

  int ret = vr_xfer(bus, addr, unlock_regs_tbuf, 2, NULL, 0);
  if (ret < 0) {
    return -1;
  }
  return 0;
}

static int
tda38640_prog_cmd(uint8_t bus, uint8_t addr, uint8_t action, uint8_t offset, uint8_t *rbuf) {
  if ((rbuf == NULL) && (action == USER_RD)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return -1;
  }

  int ret = 0;
  uint8_t prog_cmd_tbuf_low[2] = {PROG_CMD_REG, action};
  uint8_t prog_cmd_tbuf_high[2] = {PROG_CMD_REG + 1, offset};

  if (action != USER_RD) {
    ret = vr_xfer(bus, addr, prog_cmd_tbuf_low, 2, rbuf, 0);
    ret |= vr_xfer(bus, addr, prog_cmd_tbuf_high, 2, rbuf, 0);
  } else {
    ret = vr_xfer(bus, addr, prog_cmd_tbuf_high, 1, rbuf, 1);
  }

  if (ret < 0) {
    return -1;
  }
  return 0;
}

static int
read_tda38640_remaining_wr_reg(uint8_t bus, uint8_t addr, uint8_t *reg_offset, size_t read_byte,
                               uint8_t *writes_bit_cnt) {
  if ((reg_offset == NULL) || (writes_bit_cnt == NULL)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return -1;
  }

  int ret = 0;
  uint8_t rbuf[2] = {0, 0};
  uint8_t cnfg_writes_bit_temp;
  uint16_t user_writes_bit_temp;

  ret = vr_xfer(bus, addr, reg_offset, 1, rbuf, read_byte);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Failed to get remaining writes to VR addr:%x",
           __func__, addr);
    return -1;
  }

  if (read_byte == sizeof(uint8_t))
  {
    // cnfg section only check 5 images mapping to bit[7:3]
    rbuf[0] &= 0xf8;
    memcpy(&cnfg_writes_bit_temp, rbuf, sizeof(uint8_t));
    *writes_bit_cnt += pal_bitcount((unsigned int)cnfg_writes_bit_temp);
  } else {
    // user section only check 48 images mapping to bit[7:0] in 3 offsets
    memcpy(&user_writes_bit_temp, rbuf, sizeof(uint16_t));
    *writes_bit_cnt += pal_bitcount((unsigned int)user_writes_bit_temp);
  }

  return 0;
}

static int
get_tda38640_remaining_wr(uint8_t bus, uint8_t addr, uint8_t *user_remain, uint8_t *cnfg_remain) {
  if ((user_remain == NULL) || (cnfg_remain == NULL)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return -1;
  }

  int ret = 0;
  uint8_t user_writes_bit = 0;
  uint8_t cnfg_writes_bit = 0;
  uint8_t cnfg_reg = CNFG_REG;
  uint8_t user_1_reg = USER_1_REG;
  uint8_t user_2_reg = USER_2_REG;
  uint8_t user_3_reg = USER_3_REG;

  ret = tda38640_set_page(bus, addr, 0);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Failed to set VR page to VR addr%x",
          __func__, addr);
    return -1;
  }

  ret = read_tda38640_remaining_wr_reg(bus, addr, &cnfg_reg, 1, &cnfg_writes_bit);
  ret |= read_tda38640_remaining_wr_reg(bus, addr, &user_1_reg, 2, &user_writes_bit);
  ret |= read_tda38640_remaining_wr_reg(bus, addr, &user_2_reg, 2, &user_writes_bit);
  ret |= read_tda38640_remaining_wr_reg(bus, addr, &user_3_reg, 2, &user_writes_bit);

  if (ret < 0) {
    return -1;
  }

  *cnfg_remain = CNFG_REMAINING_WRITES_MAX - cnfg_writes_bit;
  *user_remain = USER_REMAINING_WRITES_MAX - user_writes_bit;

  return 0;
}

static int
get_tda38640_crc(uint8_t bus, uint8_t addr, uint32_t *sum) {
  int ret = 0;
  uint8_t rbuf[4];
  uint8_t crc_low_reg = CRC_LOW_REG;
  uint8_t crc_high_reg = CRC_HIGH_REG;

  if (sum == NULL) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return -1;
  }

  ret = tda38640_set_page(bus, addr, 0);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Failed to set VR page to VR addr%x",
          __func__, addr);
    return -1;
  }

  ret = vr_xfer(bus, addr, &crc_high_reg, 1, rbuf, 2);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Failed to get VR ver to VR addr:%x",
           __func__, addr);
    return -1;
  }

  ret = vr_xfer(bus, addr, &crc_low_reg, 1, &rbuf[2], 2);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Failed to get VR ver to VR addr:%x",
           __func__, addr);
    return -1;
  }
  memcpy(sum, rbuf, sizeof(uint32_t));

  return 0;
}

static int
cache_tda38640_crc(uint8_t bus, uint8_t addr, char *key, char *sum_str) {
  if ((key == NULL) || (sum_str == NULL)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return -1;
  }

  uint32_t checksum;
  uint8_t user_remain, cnfg_remain;

  if (get_tda38640_remaining_wr(bus, addr, &user_remain, &cnfg_remain) < 0) {
    return -1;
  }

  if (get_tda38640_crc(bus, addr, &checksum) < 0) {
    return -1;
  }

  snprintf(sum_str, MAX_VALUE_LEN,
           "Infineon %08X, Remaining Writes: %u",
           checksum, user_remain);
  kv_set(key, sum_str, 0, 0);

  return 0;
}

static int
program_tda38640(uint8_t bus, uint8_t addr, struct tda38640_config *config, bool force) {
  if (config == NULL) {
    syslog(LOG_WARNING, "%s: VR config is NULL", __func__);
    return -1;
  }

  int ret = 0;
  uint8_t user_remain, cnfg_remain;
  uint8_t rbuf_prog_progress;
  uint32_t sum = 0;

  ret = get_tda38640_crc(bus, addr, &sum);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: get tda38640 checksum failed", __func__);
    return -1;
  }

  if (!force && (sum == config->checksum)) {
    printf("the checksum %08X is the same as the current firmware!\n", sum);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: redundant programming", __func__);
    return -1;
  }

  /* program user section into VR register through i2c
  step 1: check remaining writes
  step 2: write 0x3 to register 0xd4 to unlock device registers
  step 3: write bytes by following .mic file's order
  step 4: write program command to register 0xd6
  step 5: check register 0xd7[7] for programming progress
  */
  for (int i = 0; i < config->sect_count; i++) {
    ret = get_tda38640_remaining_wr(bus, addr, &user_remain, &cnfg_remain);
    if (ret < 0) {
      return -1;
    }

    printf("Remaining Writes: %u\n", user_remain);
    if (!user_remain) {
      syslog(LOG_WARNING, "%s: no remaining writes", __func__);
      return -1;
    }

    ret = tda38640_unlock_reg(bus, addr, true);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s: Failed to unlock VR to VR addr:%x",
            __func__, addr);
      return -1;
    }

    uint8_t page_tmp = 0;
    for (int j = 0; j < config->section[i].offset_count; j++) {
      uint8_t offset_tmp[2];
      memcpy(&offset_tmp, &config->section[i].offset[j], sizeof(uint16_t));

      if (page_tmp != offset_tmp[1]) {
        ret = tda38640_unlock_reg(bus, addr, false);
        if (ret < 0) {
          syslog(LOG_WARNING, "%s: Failed to lock VR to VR addr:%x",
                __func__, addr);
          return -1;
        }

        ret = tda38640_set_page(bus, addr, offset_tmp[1]);
        if (ret < 0) {
          syslog(LOG_WARNING, "%s: Failed to set VR page to VR addr%x",
                __func__, addr);
          return -1;
        }
        page_tmp = offset_tmp[1];

        ret = tda38640_unlock_reg(bus, addr, true);
        if (ret < 0) {
          syslog(LOG_WARNING, "%s: Failed to unlock VR to VR addr:%x",
                __func__, addr);
          return -1;
        }
      }

      //write from byte 0 to byte 15 per column
      for (int k = 0; k < 16; k++) {
        int data_iter = (j * 16) + k;
        uint8_t prog_data_tbuf[] = {(offset_tmp[0] + k), config->section[i].data[data_iter]};
        ret = vr_xfer(bus, addr, prog_data_tbuf, 2, NULL, 0);
        if (ret < 0) {
          syslog(LOG_WARNING, "%s: Failed to write program data to VR addr:%x",
                 __func__, addr);
          return -1;
        }
      }
    }

    ret = tda38640_prog_cmd(bus, addr, USER_WR,
                            (USER_REMAINING_WRITES_MAX - user_remain), NULL);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s: Failed to do write program cmd to VR addr:%x",
            __func__, addr);
      return -1;
    }

    msleep(VR_PROGRAM_DELAY);

    for (int retry = VR_PROGRAM_DELAY; retry > 0; retry--) {
      ret = tda38640_prog_cmd(bus, addr, USER_RD, 0, &rbuf_prog_progress);
      if (ret >= 0) {
        break;
      }
    }

    if (ret < 0 || ((rbuf_prog_progress & 0x80) != 0x80)) {
      syslog(LOG_WARNING, "%s: Failed to check program progress done for VR addr:%x",
            __func__, addr);
      return -1;
    }
  }

  return 0;
}

int
get_tda38640_ver(struct vr_info *info, char *ver_str) {
  if ((info == NULL) || (ver_str == NULL)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return VR_STATUS_FAILURE;
  }

  char key[MAX_KEY_LEN] = {0};
  size_t max_ver_str_len = MAX_VER_STR_LEN;

  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_checksum", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_checksum", info->addr);
  }

  if (kv_get(key, ver_str, &max_ver_str_len, 0)) {
    if (info->xfer) {
      vr_xfer = info->xfer;
    }
    if (cache_tda38640_crc(info->bus, info->addr, key, ver_str)) {
      return VR_STATUS_FAILURE;
    }
  }

  return VR_STATUS_SUCCESS;
}

void *
tda38640_parse_file(struct vr_info *info, const char *path) {
  if ((info == NULL) || (path == NULL)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return NULL;
  }

  const size_t len_image_count = strlen(IMAGE_COUNT);
  const size_t len_cnfg_tag = strlen(CNFG_TAG);
  const size_t len_end = strlen(DATA_END_TAG);
  const size_t len_comment = strlen(DATA_COMMENT);
  FILE *fp = NULL;
  char line[128], *token = NULL;
  int i =0;
  int sect_cnt = 0;
  int cnfg_cnt = 0;
  int offset_cnt = 0;
  int data_cnt = 0;
  bool is_cnfg = false;
  bool is_data = false;
  struct tda38640_config *config = NULL;

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct tda38640_config *)calloc(1, sizeof(struct tda38640_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (!strncmp(line, DATA_COMMENT, len_comment)) {
      continue;
    } else if (!strncmp(line, DATA_END_TAG, len_end)) {
      // end of data
      break;
    } else if (is_data == true) {
      uint16_t offset = 0;
      uint8_t byte = 0;
      char *token_list[32] = {0};
      int token_size = pal_file_line_split(token_list, line, " ", DATA_LEN_IN_LINE);
      if (token_size < 1) {
        // unexpected data line
        continue;
      }
      if (is_cnfg == false) {
        offset = (uint16_t)strtol(token_list[0], NULL, 16);
        config->section[sect_cnt].offset[offset_cnt++] = offset;
      } else {
        is_data = false;
      }

      for (i = 1; i < token_size; i++) {
        byte = (uint32_t)strtoul(token_list[i], NULL, 16);
        if (is_cnfg == false) {
          // BYTE0 ~ BYTE15
          if (data_cnt >= TDA38640_SECT_COLUMN_NUM * 16) {
            syslog(LOG_WARNING, "%s: Exceed max data count", __func__);
            fclose(fp);
            free(config);
            return NULL;
          }
          config->section[sect_cnt].data[data_cnt++] = byte;
        } else {
          is_data = false;
          config->cnfg_data[cnfg_cnt++] = byte;
        }
      }
      // 0x0070: last column of user section
      if (offset == 0x0070) {
        is_data = false;
        config->section[sect_cnt].offset_count = offset_cnt;
        sect_cnt++;
      }
    } else {
      // now only support getting single image checksum
      if ((token = strstr(line, CHECKSUM_FIELD)) != NULL) {
        if ((token = strstr(token, "0x")) != NULL) {
          config->checksum = (uint32_t)strtoul(token, NULL, 16);
        }

        is_data = true;
        is_cnfg = false;
        offset_cnt = 0;
        data_cnt = 0;
      } else if ((token = strstr(line, IMAGE_COUNT)) != NULL) {
        config->sect_count = (uint8_t)strtoul(&token[len_image_count], NULL, 16);
      } else if (!strncmp(line, CNFG_TAG, len_cnfg_tag)) {
        is_data = true;
        is_cnfg = true;
      } else {
        continue;
      }
    }
  }
  fclose(fp);

  if (!config->checksum) {
    free(config);
    config = NULL;
  }

  return config;
}

int
tda38640_fw_update(struct vr_info *info, void *args) {
  struct tda38640_config *config = (struct tda38640_config *)args;

  if ((info == NULL) || (config == NULL)) {
    syslog(LOG_WARNING, "%s: invalid parameter pointer is NULL", __func__);
    return VR_STATUS_FAILURE;
  }

  printf("Update VR: %s\n", info->dev_name);
  if (info->xfer) {
    vr_xfer = info->xfer;
  }

  if (program_tda38640(info->bus, info->addr, config, info->force)) {
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}
