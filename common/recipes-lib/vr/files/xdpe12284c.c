#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "xdpe12284c.h"

extern int i2c_io(int, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);

static int
get_xdpe_crc(uint8_t bus, uint8_t addr, char *key, char *checksum) {
  int fd, ret = -1;
  uint8_t tbuf[16], rbuf[16];

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
    return -1;
  }

  do {
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_62;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_XDPE_REG_CRC_L;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      break;
    }

    tbuf[0] = VR_XDPE_REG_CRC_H;
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
get_xdpe_ver(struct vr_info *info, char *ver_str) {
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (get_xdpe_crc(info->bus, info->addr, key, tmp_str))
      return -1;
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1))
    return -1;

  return 0;
}

void *
xdpe_parse_file(struct vr_info *info, const char *path) {
  FILE *fp;
  char line[200], *str;
  int i, idx = 0;
  int offs, value;
  struct xdpe_config *config = NULL;

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
check_xdpe_image(uint32_t crc_exp, uint8_t *data) {
  uint8_t raw[1024];
  uint32_t crc;
  int i, idx = 0;

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
program_xdpe(uint8_t bus, uint8_t addr, uint8_t *data) {
  int fd, i, ret = -1;
  uint8_t tbuf[32], rbuf[32], remain = 0, page = 0;
  uint16_t memptr;
  float dsize, next_prog;

  if ((fd = i2c_cdev_slave_open(bus, (addr>>1), I2C_SLAVE_FORCE_CLAIM)) < 0) {
    return -1;
  }

  do {
    // check remaining writes
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_50;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_XDPE_REG_REMAIN_WR;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read remaining writes failed", __func__);
      break;
    }
    remain = (((rbuf[1] << 8) | rbuf[0]) >> 6) & 0x3F;

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_20;
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

    // MCFG space status
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_32;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x1A;
    tbuf[1] = 0xA1;
    tbuf[2] = 0x08;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x25;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_60;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x01;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    // read next memory location
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_62;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_XDPE_REG_NEXT_MEM;
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read next memory location failed", __func__);
      break;
    }
    memptr = ((rbuf[1] << 8) | rbuf[0]) & 0x3FF;
    printf("Memory pointer: 0x%X\n", memptr);

    // write configuration data
    dsize = (float)VR_XDPE_TOTAL_RW_SIZE/100;
    next_prog = dsize;
    for (i = 0; i < VR_XDPE_TOTAL_RW_SIZE; i+= 4) {
      if (page != data[i+1]) {
        page = data[i+1];
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

      // read back to compare
      if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 2))) {
        syslog(LOG_WARNING, "%s: read data failed, page=%02X offset=%02X", __func__, page, tbuf[0]);
        break;
      }

      if (memcmp(&tbuf[1], rbuf, 2)) {
        printf("data %02X%02X mismatch, expect %02X%02X\n", tbuf[2], tbuf[1], rbuf[1], rbuf[0]);
        break;
      }

      if ((i + 4) >= (int)next_prog) {
        printf("\rupdated: %d %%  ", (int)((next_prog+dsize/2)/dsize));
        fflush(stdout);
        next_prog += dsize;
      }
    }
    printf("\n");
    if (ret) {
      break;
    }

    // save configuration to EMTP
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_32;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x1A;
    tbuf[1] = 0xA1;
    tbuf[2] = 0x08;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: unlock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x1D;  // clear fault
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x26;  // upload from the registers to EMTP
    if ((ret = i2c_io(fd, addr, tbuf, 1, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    msleep(500);

    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_60;
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
    tbuf[1] = VR_XDPE_PAGE_32;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x1A;
    tbuf[1] = 0x00;
    tbuf[2] = 0x00;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: lock register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    // check CRC
    tbuf[0] = VR_REG_PAGE;
    tbuf[1] = VR_XDPE_PAGE_32;
    if ((ret = i2c_io(fd, addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = 0x2C;
    tbuf[1] = memptr & 0xFF;
    tbuf[2] = (memptr >> 8) & 0x03;
    if ((ret = i2c_io(fd, addr, tbuf, 3, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = 0x22;  // download from the EMTP to registers
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
xdpe_fw_update(struct vr_info *info, void *args) {
  struct xdpe_config *config = (struct xdpe_config *)args;
  int ret;

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  do {
    printf("Update VR: %s\n", info->dev_name);
    ret = check_xdpe_image(config->crc_exp, config->data);
    if (ret) {
      break;
    }

    ret = program_xdpe(info->bus, info->addr, config->data);
    if (ret) {
      break;
    }
  } while (0);

  return ret;
}

int
xdpe_fw_verify(struct vr_info *info, void *args) {
  struct xdpe_config *config = (struct xdpe_config *)args;
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
    tbuf[1] = VR_XDPE_PAGE_62;
    if ((ret = i2c_io(fd, info->addr, tbuf, 2, rbuf, 0))) {
      syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
      break;
    }

    tbuf[0] = VR_XDPE_REG_CRC_L;
    if ((ret = i2c_io(fd, info->addr, tbuf, 1, rbuf, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = VR_XDPE_REG_CRC_H;
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
