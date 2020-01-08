#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "pxe1110c.h"

extern int i2c_io(int, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);

static int
get_pxe_crc(uint8_t bus, uint8_t addr, char *key, char *checksum) {
  int fd, ret = -1;
  uint8_t tbuf[16], rbuf[16];

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
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

int
get_pxe_ver(struct vr_info *info, char *ver_str) {
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (get_pxe_crc(info->bus, info->addr, key, tmp_str))
      return -1;
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1))
    return -1;

  return 0;
}

void *
pxe_parse_file(struct vr_info *info, const char *path) {
  FILE *fp;
  char line[200], *str;
  int i, idx = 0;
  struct pxe_config *config = NULL;

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct pxe_config *)calloc(1, sizeof(struct pxe_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (!strncmp(line, "//E1110C", 8)) {
      if ((str = strtok(line, " -")) && (str = strtok(NULL, " -"))) {
        config->addr = strtol(str, NULL, 16);
      }

      if (str && (str = strtok(NULL, " -"))) {
        config->crc_exp = strtoll(str, NULL, 16);
      }
    } else if (strstr(line, "//ReadWriteWord")) {
      if ((str = strtok(line, " \t"))) {
        i = strtol(str, NULL, 16);
        memcpy(&config->data[idx], &i, 2);
      }

      if (str && (str = strtok(NULL, " \t"))) {
        i = strtol(str, NULL, 2);
        memcpy(&config->data[idx+2], &i, 2);
      }

      idx += 4;
      if (idx > VR_PXE_TOTAL_RW_SIZE) {
        syslog(LOG_WARNING, "%s: unexpected image size", __func__);
        fclose(fp);
        free(config);
        return NULL;
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

static int
check_pxe_image(uint32_t crc_exp, uint8_t *data) {
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
program_pxe(uint8_t bus, uint8_t addr, uint8_t *data) {
  int fd, i, ret = -1;
  uint8_t tbuf[32], rbuf[32], remain = 0, page = 0;
  float dsize, next_prog;

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
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
  } while (0);

  close(fd);
  return ret;
}

int
pxe_fw_update(struct vr_info *info, void *args) {
  struct pxe_config *config = (struct pxe_config *)args;
  int ret;

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  do {
    printf("Update VR: %s\n", info->dev_name);
    ret = check_pxe_image(config->crc_exp, config->data);
    if (ret) {
      break;
    }

    ret = program_pxe(info->bus, info->addr, config->data);
    if (ret) {
      break;
    }
  } while (0);

  return ret;
}

int
pxe_fw_verify(struct vr_info *info, void *args) {
  struct pxe_config *config = (struct pxe_config *)args;
  int fd, ret;
  uint32_t crc;
  uint8_t tbuf[64], rbuf[64];

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  if ((fd = i2c_cdev_slave_open(info->bus, (info->addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
    return -1;
  }

  do {
    // check CRC
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_PXE_PAGE_6F;
    if ((ret = i2c_io(fd, info->addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_PXE_REG_CRC_L;
    if ((ret = i2c_io(fd, info->addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = VR_PXE_REG_CRC_H;
    if ((ret = i2c_io(fd, info->addr, tbuf, 1, &rbuf[2], 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }
  } while (0);
  close(fd);

  memcpy(&crc, rbuf, 4);
  if (crc != config->crc_exp) {
    printf("CRC %04X mismatch, expect %04X\n", crc, config->crc_exp);
    ret = -1;
  } else {
    snprintf((char *)tbuf, sizeof(tbuf), "/tmp/cache_store/vr_%02xh_crc", info->addr);
    unlink((char *)tbuf);
  }

  return ret;
}
