#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "xdpe152xx.h"

#define ADDRESS_FIELD  "PMBus Address :"
#define CHECKSUM_FIELD "Checksum :"
#define DATA_START_TAG "[Configuration Data]"
#define DATA_END_TAG   "[End Configuration Data]"
#define DATA_COMMENT   "//"
#define DATA_XV        "XV"

#define DATA_LEN_IN_LINE 5
#define SECT_TRIM 0x02
#define IFX_CONF_SIZE 1344  // Config(604) + PMBus(504) + SVID(156) + PMBusPartial(80)
#define REMAINING_TIMES(x) (((x[1] << 8) | x[0]) / IFX_CONF_SIZE)

#define CRC32_POLY 0xEDB88320  // polynomial

#define VR_WRITE_DELAY  10   // 10 ms
#define VR_RELOAD_DELAY 500  // 500 ms

enum {
  PMBUS_STS_CML       = 0x7E,
  IFX_MFR_AHB_ADDR    = 0xCE,
  IFX_MFR_REG_WRITE   = 0xDE,
  IFX_MFR_REG_READ    = 0xDF,
  IFX_MFR_FW_CMD_DATA = 0xFD,
  IFX_MFR_FW_CMD      = 0xFE,

  FW_RESET      = 0x0e,
  OTP_PTN_RMNG  = 0x10,
  OTP_CONF_STO  = 0x11,
  OTP_FILE_INVD = 0x12,
  GET_CRC       = 0x2D,
};

extern int vr_rdwr(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t) = &vr_rdwr;

static int
xdpe152xx_mfr_fw(uint8_t bus, uint8_t addr, uint8_t code, uint8_t *data, uint8_t *resp) {
  uint8_t tbuf[16], rbuf[16];

  if (data) {
    tbuf[0] = IFX_MFR_FW_CMD_DATA;
    tbuf[1] = 4;  // block write 4 bytes
    memcpy(&tbuf[2], data, 4);
    if (vr_xfer(bus, addr, tbuf, 6, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: Block write 0x%02X failed", __func__, tbuf[0]);
      return -1;
    }
  }

  tbuf[0] = IFX_MFR_FW_CMD;
  tbuf[1] = code;
  if (vr_xfer(bus, addr, tbuf, 2, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: Write code 0x%02X failed", __func__, tbuf[1]);
    return -1;
  }

  if (resp) {
    tbuf[0] = IFX_MFR_FW_CMD_DATA;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 6) < 0) {
      syslog(LOG_WARNING, "%s: Block read 0x%02X failed", __func__, tbuf[0]);
      return -1;
    }

    if (rbuf[0] != 4) {  // block read 4 bytes
      syslog(LOG_WARNING, "%s: Unexpected data, len = %u", __func__, rbuf[0]);
      return -1;
    }
    memcpy(resp, rbuf+1, 4);
  }

  return 0;
}

static int
get_xdpe152xx_remaining_wr(uint8_t bus, uint8_t addr, uint8_t *remain) {
  uint8_t tbuf[16] = {0}, rbuf[16];

  if (remain == NULL) {
    return -1;
  }

  if (xdpe152xx_mfr_fw(bus, addr, OTP_PTN_RMNG, tbuf, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: Failed to get vr remaining writes", __func__);
    return -1;
  }
  *remain = REMAINING_TIMES(rbuf);

  return 0;
}

static int
get_xdpe152xx_crc(uint8_t bus, uint8_t addr, uint32_t *sum) {
  uint8_t tbuf[16] = {0}, rbuf[16];

  if (sum == NULL) {
    return -1;
  }

  if (xdpe152xx_mfr_fw(bus, addr, GET_CRC, tbuf, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: Failed to get vr ver", __func__);
    return -1;
  }
  memcpy(sum, rbuf, sizeof(uint32_t));

  return 0;
}

static int
cache_xdpe152xx_crc(uint8_t bus, uint8_t addr, char *key, char *sum_str, uint32_t *sum) {
  uint8_t remain;
  uint32_t tmp_sum;
  char tmp_str[MAX_VALUE_LEN] = {0};

  if (key == NULL) {
    return -1;
  }

  if (get_xdpe152xx_remaining_wr(bus, addr, &remain) < 0) {
    return -1;
  }

  if (!sum) {
    sum = &tmp_sum;
  }
  if (get_xdpe152xx_crc(bus, addr, sum) < 0) {
    return -1;
  }

  if (!sum_str) {
    sum_str = tmp_str;
  }
  snprintf(sum_str, MAX_VALUE_LEN, "Infineon %08X, Remaining Writes: %u", *sum, remain);
  kv_set(key, sum_str, 0, 0);

  return 0;
}

static int
split(char **dst, char *src, char *delim, int maxsz) {
  if (dst == NULL || src == NULL || delim == NULL) {
    return -1;
  }

  char *s = strtok(src, delim);
  int size = 0;

  while (s) {
    *dst++ = s;
    if ((++size) >= maxsz) {
      break;
    }
    s = strtok(NULL, delim);
  }

  return size;
}

static uint32_t
cal_crc32(uint32_t *data, int len) {
  uint32_t crc = 0xFFFFFFFF;
  int i, b;

  if (data == NULL) {
    return 0;
  }

  for (i = 0; i < len; i++) {
    crc ^= data[i];
    for (b = 0; b < 32; b++) {
      if (crc & 0x1) {
        crc = (crc >> 1) ^ CRC32_POLY;  // lsb-first
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}

static int
check_xdpe152xx_image(struct xdpe152xx_config *config) {
  uint8_t i;
  uint32_t sum = 0, crc;

  if (config == NULL) {
    return -1;
  }

  for (i = 0; i < config->sect_cnt; i++) {
    struct config_sect *sect = &config->section[i];
    if (sect == NULL) {
      return -1;
    }

    // check CRC of section header
    crc = cal_crc32(&sect->data[0], 2);
    if (crc != sect->data[2]) {
      syslog(LOG_WARNING, "%s: CRC %08X mismatch, expect %08X", __func__, crc,
             sect->data[2]);
      return -1;
    }
    sum += crc;

    // check CRC of section data
    crc = cal_crc32(&sect->data[3], sect->data_cnt-4);
    if (crc != sect->data[sect->data_cnt-1]) {
      syslog(LOG_WARNING, "%s: CRC %08X mismatch, expect %08X", __func__, crc,
             sect->data[sect->data_cnt-1]);
      return -1;
    }
    sum += crc;
  }

  printf("File CRC : %08X\n", sum);
  if (sum != config->sum_exp) {
    syslog(LOG_WARNING, "%s: Checksum mismatched! Expected: %08X, Actual: %08X",
           __func__, config->sum_exp, sum);
    return -1;
  }

  return 0;
}

static int
program_xdpe152xx(uint8_t bus, uint8_t addr, struct xdpe152xx_config *config, bool force) {
  uint8_t tbuf[16], rbuf[16];
  uint8_t remain = 0;
  uint32_t sum = 0;
  int i, j, size = 0;
  int prog = 0, ret = 0;

  if (config == NULL) {
    return -1;
  }

  if (get_xdpe152xx_crc(bus, addr, &sum) < 0) {
    return -1;
  }

  if (!force && (sum == config->sum_exp)) {
    printf("WARNING: the Checksum is the same as used now %08X!\n", sum);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: redundant programming", __func__);
    return -1;
  }

  // check remaining writes
  if (get_xdpe152xx_remaining_wr(bus, addr, &remain) < 0) {
    return -1;
  }

  printf("Remaining writes: %u\n", remain);
  if (!remain) {
    syslog(LOG_WARNING, "%s: no remaining writes", __func__);
    return -1;
  }

  if (!force && (remain <= VR_WARN_REMAIN_WR)) {
    printf("WARNING: the remaining writes is below the threshold value %d!\n",
           VR_WARN_REMAIN_WR);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: insufficient remaining writes %u", __func__, remain);
    return -1;
  }

  for (i = 0; i < config->sect_cnt; i++) {
    struct config_sect *sect = &config->section[i];
    if (sect == NULL) {
      ret = -1;
      break;
    }

    if ((i <= 0) || (sect->type != config->section[i-1].type)) {
      // clear bit0 of PMBUS_STS_CML
      tbuf[0] = PMBUS_STS_CML;
      tbuf[1] = 0x01;
      if ((ret = vr_xfer(bus, addr, tbuf, 2, rbuf, 0)) < 0) {
        syslog(LOG_WARNING, "%s: Failed to write PMBUS_STS_CML", __func__);
        break;
      }

      // invalidate existing data
      tbuf[0] = sect->type;  // section type
      tbuf[1] = 0x00;        // xv0
      tbuf[2] = 0x00;
      tbuf[3] = 0x00;
      if ((ret = xdpe152xx_mfr_fw(bus, addr, OTP_FILE_INVD, tbuf, NULL)) < 0) {
        syslog(LOG_WARNING, "%s: Failed to invalidate %02X", __func__, sect->type);
        break;
      }
      msleep(VR_WRITE_DELAY);

      // set scratchpad addr
      tbuf[0] = IFX_MFR_AHB_ADDR;
      tbuf[1] = 4;
      tbuf[2] = 0x00;
      tbuf[3] = 0xe0;
      tbuf[4] = 0x05;
      tbuf[5] = 0x20;
      if ((ret = vr_xfer(bus, addr, tbuf, 6, rbuf, 0)) < 0) {
        syslog(LOG_WARNING, "%s: Failed to set scratchpad addr", __func__);
        break;
      }
      msleep(VR_WRITE_DELAY);
      size = 0;
    }

    // program data into scratch
    for (j = 0; j < sect->data_cnt; j++) {
      tbuf[0] = IFX_MFR_REG_WRITE;
      tbuf[1] = 4;
      memcpy(&tbuf[2], &sect->data[j], 4);
      if ((ret = vr_xfer(bus, addr, tbuf, 6, rbuf, 0)) < 0) {
        syslog(LOG_WARNING, "%s: Failed to write data %08X", __func__, sect->data[j]);
        break;
      }
      msleep(VR_WRITE_DELAY);
    }
    if (ret) {
      break;
    }

    size += sect->data_cnt * 4;
    if ((i+1 >= config->sect_cnt) || (sect->type != config->section[i+1].type)) {
      // upload scratchpad to OTP
      memcpy(tbuf, &size, 2);
      tbuf[2] = 0x00;
      tbuf[3] = 0x00;
      if ((ret = xdpe152xx_mfr_fw(bus, addr, OTP_CONF_STO, tbuf, NULL)) < 0) {
        syslog(LOG_WARNING, "%s: Failed to upload data to OTP", __func__);
        break;
      }

      // wait for programming soak (2ms/byte, at least 200ms)
      // ex: Config (604 bytes): (604 / 50) + 2 = 14 (1400 ms)
      size = (size / 50) + 2;
      for (j = 0; j < size; j++) {
        msleep(100);
      }

      tbuf[0] = PMBUS_STS_CML;
      if ((ret = vr_xfer(bus, addr, tbuf, 1, rbuf, 1)) < 0) {
        syslog(LOG_WARNING, "%s: Failed to read PMBUS_STS_CML", __func__);
        break;
      }
      if (rbuf[0] & 0x01) {
        syslog(LOG_WARNING, "%s: CML Other Memory Fault: %02X (%02X)",
               __func__, rbuf[0], sect->type);
        ret = -1;
        break;
      }
    }

    prog += sect->data_cnt;
    printf("\rupdated: %d %%  ", (prog*100)/config->total_cnt);
    fflush(stdout);
  }
  printf("\n");
  if (ret) {
    return -1;
  }

  return 0;
}

int
get_xdpe152xx_ver(struct vr_info *info, char *ver_str) {
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  if (info == NULL || ver_str == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (info->xfer) {
      vr_xfer = info->xfer;
    }
    if (cache_xdpe152xx_crc(info->bus, info->addr, key, tmp_str, NULL)) {
      return VR_STATUS_FAILURE;
    }
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1)) {
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

void *
xdpe152xx_parse_file(struct vr_info *info, const char *path) {
  const size_t len_start = strlen(DATA_START_TAG);
  const size_t len_end = strlen(DATA_END_TAG);
  const size_t len_comment = strlen(DATA_COMMENT);
  const size_t len_xv = strlen(DATA_XV);
  FILE *fp = NULL;
  char line[128], *token = NULL;
  int i, data_cnt = 0, sect_idx = -1;
  uint8_t sect_type = 0x0;
  uint16_t offset;
  uint32_t dword;
  bool is_data = false;
  struct xdpe152xx_config *config = NULL;

  if (info == NULL || path == NULL) {
    return NULL;
  }

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct xdpe152xx_config *)calloc(1, sizeof(struct xdpe152xx_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (!strncmp(line, DATA_COMMENT, len_comment)) {
      token = line + len_comment;
      if (!strncmp(token, DATA_XV, len_xv)) {  // find "//XV..."
        printf("Parsing %s", token);
      }
      continue;
    } else if (!strncmp(line, DATA_END_TAG, len_end)) {  // end of data
      break;
    } else if (is_data == true) {
      char *token_list[8] = {0};
      int token_size = split(token_list, line, " ", DATA_LEN_IN_LINE);
      if (token_size < 1) {  // unexpected data line
        continue;
      }

      offset = (uint16_t)strtol(token_list[0], NULL, 16);
      if (sect_type == SECT_TRIM && offset != 0x0) {  // skip Trim
        continue;
      }

      for (i = 1; i < token_size; i++) {  // DWORD0 ~ DWORD3
        dword = (uint32_t)strtoul(token_list[i], NULL, 16);
        if ((offset == 0x0) && (i == 1)) {  // start of section header
          sect_type = (uint8_t)dword;  // section type
          if (sect_type == SECT_TRIM) {
            break;
          }

          if ((++sect_idx) >= MAX_SECT_NUM) {
            syslog(LOG_WARNING, "%s: Exceed max section number", __func__);
            fclose(fp);
            free(config);
            return NULL;
          }

          config->section[sect_idx].type = sect_type;
          config->sect_cnt = sect_idx + 1;
          data_cnt = 0;
        }
        if (data_cnt >= MAX_SECT_DATA_NUM) {
          syslog(LOG_WARNING, "%s: Exceed max data count", __func__);
          fclose(fp);
          free(config);
          return NULL;
        }
        config->section[sect_idx].data[data_cnt++] = dword;
        config->section[sect_idx].data_cnt = data_cnt;
        config->total_cnt++;
      }
    } else {
      if ((token = strstr(line, ADDRESS_FIELD)) != NULL) {
        if ((token = strstr(token, "0x")) != NULL) {
          config->addr = (uint8_t)(strtoul(token, NULL, 16) << 1);
        }
      } else if ((token = strstr(line, CHECKSUM_FIELD)) != NULL) {
        if ((token = strstr(token, "0x")) != NULL) {
          config->sum_exp = (uint32_t)strtoul(token, NULL, 16);
          printf("Configuration Checksum: %08X\n", config->sum_exp);
        }
      } else if (!strncmp(line, DATA_START_TAG, len_start)) {
        is_data = true;
        continue;
      } else {
        continue;
      }
    }
  }
  fclose(fp);

  if (!config->addr) {  // for *.mic file that PMBus Address is not included in
    config->addr = info->addr;
  }

  if (!config->sum_exp) {
    free(config);
    config = NULL;
  }

  return config;
}

int
xdpe152xx_fw_update(struct vr_info *info, void *args) {
  struct xdpe152xx_config *config = (struct xdpe152xx_config *)args;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  printf("Update VR: %s\n", info->dev_name);
  if (check_xdpe152xx_image(config)) {
    return VR_STATUS_FAILURE;
  }

  if (info->xfer) {
    vr_xfer = info->xfer;
  }
  if (program_xdpe152xx(info->bus, info->addr, config, info->force)) {
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

int
xdpe152xx_fw_verify(struct vr_info *info, void *args) {
  struct xdpe152xx_config *config = (struct xdpe152xx_config *)args;
  char key[MAX_KEY_LEN];
  uint32_t sum = 0;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  if (xdpe152xx_mfr_fw(info->bus, info->addr, FW_RESET, NULL, NULL) < 0) {
    syslog(LOG_WARNING, "%s: Failed to reset vr", __func__);
    return -1;
  }
  msleep(VR_RELOAD_DELAY);

  // update checksum
  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }
  if (cache_xdpe152xx_crc(info->bus, info->addr, key, NULL, &sum)) {
    return VR_STATUS_FAILURE;
  }

  if (sum != config->sum_exp) {
    printf("Checksum %08X mismatch, expect %08X\n", sum, config->sum_exp);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}
