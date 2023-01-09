#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "xdpe12284c.h"

#define REMAINING_TIMES(x) ((((x[1] << 8) | x[0]) >> 6) & 0x3F)

extern int vr_rdwr(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t) = &vr_rdwr;

static int
set_page(uint8_t bus, uint8_t addr, uint8_t page) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = VR_REG_PAGE;
  tbuf[1] = page;
  if (vr_xfer(bus, addr, tbuf, 2, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
    return -1;
  }

  return 0;
}

static int
get_xdpe122xx_remaining_wr(uint8_t bus, uint8_t addr, uint8_t *remain) {
  uint8_t tbuf[16], rbuf[16];
  int ret = -1;

  if (remain == NULL) {
    return -1;
  }

  do {
    if (set_page(bus, addr, VR_XDPE_PAGE_50) < 0) {
      break;
    }

    tbuf[0] = VR_XDPE_REG_REMAIN_WR;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
      syslog(LOG_WARNING, "%s: read remaining writes failed", __func__);
      break;
    }
    *remain = REMAINING_TIMES(rbuf);
    ret = 0;
  } while (0);

  if (set_page(bus, addr, 0x00) < 0) {
    return -1;
  }
  if (ret) {
    return -1;
  }

  return 0;
}

static int
get_xdpe122xx_crc(uint8_t bus, uint8_t addr, uint32_t *crc) {
  uint8_t tbuf[16], rbuf[16];
  int ret = -1;

  if (crc == NULL) {
    return -1;
  }

  do {
    if (set_page(bus, addr, VR_XDPE_PAGE_62) < 0) {
      break;
    }

    tbuf[0] = VR_XDPE_REG_CRC_L;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = VR_XDPE_REG_CRC_H;
    if (vr_xfer(bus, addr, tbuf, 1, &rbuf[2], 2) < 0) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    memcpy(crc, rbuf, sizeof(uint32_t));
    ret = 0;
  } while (0);

  if (set_page(bus, addr, 0x00) < 0) {
    return -1;
  }
  if (ret) {
    return -1;
  }

  return 0;
}

static int
cache_xdpe122xx_crc(uint8_t bus, uint8_t addr, char *key, char *sum_str, uint32_t *crc) {
  uint8_t remain;
  uint32_t tmp_crc;
  char tmp_str[MAX_VALUE_LEN] = {0};

  if (key == NULL) {
    return -1;
  }

  if (get_xdpe122xx_remaining_wr(bus, addr, &remain) < 0) {
    return -1;
  }

  if (!crc) {
    crc = &tmp_crc;
  }
  if (get_xdpe122xx_crc(bus, addr, crc) < 0) {
    return -1;
  }

  if (!sum_str) {
    sum_str = tmp_str;
  }
  snprintf(sum_str, MAX_VALUE_LEN, "Infineon %08X, Remaining Writes: %u", *crc, remain);
  kv_set(key, sum_str, 0, 0);

  return 0;
}

int
get_xdpe_ver(struct vr_info *info, char *ver_str) {
  int ret;
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

    //Stop sensor polling before read crc from register
    if (info->sensor_polling_ctrl) {
      info->sensor_polling_ctrl(false);
    }

    ret = cache_xdpe122xx_crc(info->bus, info->addr, key, tmp_str, NULL);

    //Resume sensor polling
    if (info->sensor_polling_ctrl) {
      info->sensor_polling_ctrl(true);
    }

    if (ret) {
      return VR_STATUS_FAILURE;
    }
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1)) {
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

void *
xdpe_parse_file(struct vr_info *info, const char *path) {
  FILE *fp;
  char line[200], *str;
  int i, idx = 0;
  int offs, value;
  struct xdpe_config *config = NULL;

  if (info == NULL || path == NULL) {
    return NULL;
  }

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct xdpe_config *)calloc(1, sizeof(struct xdpe_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (!strncmp(line, "XDPE12284C", 10)) {
      if ((str = strtok(line, " -")) && (str = strtok(NULL, " -"))) {
        config->addr = strtol(str, NULL, 16);
      }

      if (str && (str = strtok(NULL, " -"))) {
        config->crc_exp = strtoll(str, NULL, 16);
      }
    } else if (!strncmp(line, "[Config Data]", 13)) {
      while (fgets(line, sizeof(line), fp) != NULL) {
        if (!strncmp(line, "[End", 4)) {
          break;
        }

        if (line[0] == '2') {
          str = strtok(line, " ");
          offs = strtol(str, NULL, 16);
          for (i = 0; (i < 16) && (str = strtok(NULL, " ")); i++, offs++) {
            if (str[0] == '-') {
              continue;
            }
            value = strtol(str, NULL, 16);
            memcpy(&config->data[idx], &offs, 2);
            memcpy(&config->data[idx+2], &value, 2);
            idx += 4;
            if (idx > VR_XDPE_TOTAL_RW_SIZE) {
              syslog(LOG_WARNING, "%s: unexpected image size", __func__);
              fclose(fp);
              free(config);
              return NULL;
            }
          }
        }
      }
    }
  }

  fclose(fp);
  if (!config->addr || !config->crc_exp) {
    free(config);
    config = NULL;
  }

  return config;
}

static uint32_t
cal_crc32(uint8_t const *data, int len) {
  uint32_t crc = 0xFFFFFFFF;
  int i, b;

  if (data == NULL) {
    return 0;
  }

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

static int
check_xdpe122xx_image(uint32_t crc_exp, uint8_t *data) {
  uint8_t raw[1024];
  uint32_t crc;
  int i, idx = 0;

  if (data == NULL) {
    return -1;
  }

  for (i = 2, idx = 0; i < VR_XDPE_TOTAL_RW_SIZE; i += 4, idx += 2) {
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
program_xdpe122xx(uint8_t bus, uint8_t addr, struct xdpe_config *config, bool force) {
  int i, ret = -1, rc = 0;
  uint8_t tbuf[32], rbuf[32], remain = 0;
  uint8_t *data, page = 0;
  uint32_t crc = 0;
  float dsize, next_prog;

  if (config == NULL) {
    return -1;
  }

  if (get_xdpe122xx_crc(bus, addr, &crc) < 0) {
    return -1;
  }

  if (!force && (crc == config->crc_exp)) {
    printf("WARNING: the CRC is the same as used now %08X!\n", crc);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: redundant programming", __func__);
    return -1;
  }

  // check remaining writes
  if (get_xdpe122xx_remaining_wr(bus, addr, &remain) < 0) {
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

  do {
    // read next memory location
    if (set_page(bus, addr, VR_XDPE_PAGE_62) < 0) {
      break;
    }

    tbuf[0] = VR_XDPE_REG_NEXT_MEM;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
      syslog(LOG_WARNING, "%s: read next memory location failed", __func__);
      break;
    }
    config->memptr = ((rbuf[1] << 8) | rbuf[0]) & 0x3FF;
    printf("Memory pointer: 0x%X\n", config->memptr);

    // write configuration data
    data = config->data;
    dsize = (float)VR_XDPE_TOTAL_RW_SIZE/100;
    next_prog = dsize;
    for (i = 0; i < VR_XDPE_TOTAL_RW_SIZE; i+= 4) {
      if (page != data[i+1]) {
        page = data[i+1];
        if ((rc = set_page(bus, addr, page)) < 0) {
          break;
        }
      }

      tbuf[0] = data[i];  // offset
      tbuf[1] = data[i+2];
      tbuf[2] = data[i+3];
      if ((rc = vr_xfer(bus, addr, tbuf, 3, rbuf, 0)) < 0) {
        syslog(LOG_WARNING, "%s: write data failed, page=%02X offset=%02X data=%02X%02X",
               __func__, page, tbuf[0], tbuf[2], tbuf[1]);
        break;
      }

      // read back to compare
      if ((rc = vr_xfer(bus, addr, tbuf, 1, rbuf, 2)) < 0) {
        syslog(LOG_WARNING, "%s: read data failed, page=%02X offset=%02X", __func__, page, tbuf[0]);
        break;
      }
      if (memcmp(&tbuf[1], rbuf, 2)) {
        printf("data %02X%02X mismatch, expect %02X%02X\n", tbuf[2], tbuf[1], rbuf[1], rbuf[0]);
        rc = -1;
        break;
      }

      if ((i + 4) >= (int)next_prog) {
        printf("\rupdated: %d %%  ", (int)((next_prog+dsize/2)/dsize));
        fflush(stdout);
        next_prog += dsize;
      }
    }
    printf("\n");
    if (rc) {
      break;
    }

    // save configuration to EMTP
    if (set_page(bus, addr, VR_XDPE_PAGE_32) < 0) {
      break;
    }

    tbuf[0] = 0x1A;
    tbuf[1] = 0xA1;
    tbuf[2] = 0x08;
    if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x1D;  // clear fault
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x26;  // upload from the registers to EMTP
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    msleep(500);

    if (set_page(bus, addr, VR_XDPE_PAGE_60) < 0) {
      break;
    }

    tbuf[0] = 0x01;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    if ((rbuf[0] & 0x01)) {
      syslog(LOG_WARNING, "%s: unexpected status, reg%02X=%02X", __func__, tbuf[0], rbuf[0]);
      break;
    }

    tbuf[0] = 0x02;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    if ((rbuf[0] & 0x0A)) {
      syslog(LOG_WARNING, "%s: unexpected status, reg%02X=%02X", __func__, tbuf[0], rbuf[0]);
      break;
    }

    if (set_page(bus, addr, VR_XDPE_PAGE_32) < 0) {
      break;
    }

    tbuf[0] = 0x1A;
    tbuf[1] = 0x00;
    tbuf[2] = 0x00;
    if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: lock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    ret = 0;
  } while (0);

  if (set_page(bus, addr, 0x00) < 0) {
    return -1;
  }
  if (ret) {
    return -1;
  }

  return 0;
}

int
xdpe_fw_update(struct vr_info *info, void *args) {
  struct xdpe_config *config = (struct xdpe_config *)args;
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  uint8_t remain;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  printf("Update VR: %s\n", info->dev_name);
  if (check_xdpe122xx_image(config->crc_exp, config->data)) {
    return VR_STATUS_FAILURE;
  }

  if (info->xfer) {
    vr_xfer = info->xfer;
  }
  if (program_xdpe122xx(info->bus, info->addr, config, info->force)) {
    return VR_STATUS_FAILURE;
  }

  if (pal_is_support_vr_delay_activate()) {
    vr_get_fw_avtive_key(info, key);
    if (get_xdpe122xx_remaining_wr(info->bus, info->addr, &remain) < 0) {
      snprintf(value, sizeof(value), "Infineon %08X, Remaining Writes: Unknown", config->crc_exp);
    } else {
      snprintf(value, sizeof(value), "Infineon %08X, Remaining Writes: %u", config->crc_exp, remain);
    }
    kv_set(key, value, 0, 0);
  }

  return VR_STATUS_SUCCESS;
}

int
xdpe_fw_verify(struct vr_info *info, void *args) {
  struct xdpe_config *config = (struct xdpe_config *)args;
  char key[MAX_KEY_LEN];
  uint8_t tbuf[16], rbuf[16];
  uint32_t crc = 0;
  int ret = -1;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  // reload config
  do {
    if (set_page(info->bus, info->addr, VR_XDPE_PAGE_32) < 0) {
      break;
    }

    tbuf[0] = 0x2C;
    tbuf[1] = config->memptr & 0xFF;
    tbuf[2] = (config->memptr >> 8) & 0x03;
    if (vr_xfer(info->bus, info->addr, tbuf, 3, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x22;  // download from the EMTP to registers
    if (vr_xfer(info->bus, info->addr, tbuf, 1, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
    msleep(10);
    ret = 0;
  } while (0);

  if (set_page(info->bus, info->addr, 0x00) < 0) {
    return VR_STATUS_FAILURE;
  }
  if (ret) {
    return VR_STATUS_FAILURE;
  }

  // update checksum
  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }
  if (cache_xdpe122xx_crc(info->bus, info->addr, key, NULL, &crc)) {
    return VR_STATUS_FAILURE;
  }

  if (crc != config->crc_exp) {
    printf("CRC %08X mismatch, expect %08X\n", crc, config->crc_exp);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}
