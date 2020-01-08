#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "tps53688.h"

extern int i2c_io(int, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);

static int
get_tps_crc(uint8_t bus, uint8_t addr, char *key, char *checksum) {
  int fd, ret = -1;
  uint8_t tbuf[16], rbuf[16];

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
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
get_tps_ver(struct vr_info *info, char *ver_str) {
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (get_tps_crc(info->bus, info->addr, key, tmp_str))
      return -1;
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1))
    return -1;

  return 0;
}

void *
tps_parse_file(struct vr_info *info, const char *path) {
  FILE *fp;
  char line[200], xdigit[8] = {0};
  char *token, *str;
  int i, idx = 0;
  struct tps_config *config = NULL;

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct tps_config *)calloc(1, sizeof(struct tps_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    token = strtok(line, ",");
    if (!token) {
      continue;
    }

    if (!strcmp(token, "Comment")) {
      if ((token = strtok(NULL, ",")) && (token = strstr(token, "IC_DEVICE_ID"))) {
        if ((token = strstr(token, "0x")) && (str = strtok(token, " ")))
          config->devid_exp = strtoll(str, NULL, 16);
      }
    } else if (!strcmp(token, "BlockWrite")) {
      for (i = 0; i < 3; i++) {
        if (!(token = strtok(NULL, ",")))
          break;

        if (!config->addr) {
          config->addr = strtol(token, NULL, 16) << 1;
        }
      }
      if (!token) {
        continue;
      }

      token += 2;
      for (i = 0; i <= (VR_TPS_BLK_LEN*2); i += 2) {
        strncpy(xdigit, &token[i], 2);
        config->data[idx++] = strtol(xdigit, NULL, 16);
        if (idx > VR_TPS_TOTAL_WR_SIZE) {
          syslog(LOG_WARNING, "%s: unexpected image size", __func__);
          fclose(fp);
          free(config);
          return NULL;
        }
      }

      if (idx == VR_TPS_BLK_WR_LEN) {  // USER_NVM_INDEX 0
        memcpy(&config->crc_exp, &config->data[VR_TPS_OFFS_CRC], 2);
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
check_tps_image(uint16_t crc_exp, uint8_t *data) {
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
program_tps(uint8_t bus, uint8_t addr, uint64_t devid_exp, uint8_t *data) {
  int fd, i, ret = -1;
  uint64_t devid = 0x00;
  uint32_t offset = 0, dsize;
  uint8_t tbuf[64], rbuf[64];

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
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
  } while (0);

  close(fd);
  return ret;
}

int
tps_fw_update(struct vr_info *info, void *args) {
  struct tps_config *config = (struct tps_config *)args;
  int ret;

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  do {
    printf("Update VR: %s\n", info->dev_name);
    ret = check_tps_image(config->crc_exp, config->data);
    if (ret) {
      break;
    }

    ret = program_tps(info->bus, info->addr, config->devid_exp, config->data);
    if (ret) {
      break;
    }
  } while (0);

  return ret;
}

int
tps_fw_verify(struct vr_info *info, void *args) {
  struct tps_config *config = (struct tps_config *)args;
  int fd, ret;
  uint16_t crc;
  uint8_t tbuf[64], rbuf[64];

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  if ((fd = i2c_cdev_slave_open(info->bus, (info->addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
    return -1;
  }

  // check CRC
  tbuf[0] = VR_TPS_REG_CRC;
  ret = i2c_io(fd, info->addr, tbuf, 1, rbuf, 2);
  close(fd);
  if (ret) {
    syslog(LOG_WARNING, "%s: read CRC failed", __func__);
    return -1;
  }

  memcpy(&crc, rbuf, 2);
  if (crc != config->crc_exp) {
    printf("CRC %04X mismatch, expect %04X\n", crc, config->crc_exp);
    ret = -1;
  } else {
    snprintf((char *)tbuf, sizeof(tbuf), "/tmp/cache_store/vr_%02xh_crc", info->addr);
    unlink((char *)tbuf);
  }

  return ret;
}
