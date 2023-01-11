#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "tps53688.h"

extern int vr_rdwr(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t) = &vr_rdwr;

static int
get_tps_devid(uint8_t bus, uint8_t addr, uint64_t *devid) {
  int i;
  uint8_t tbuf[16], rbuf[16];

  if (devid == NULL) {
    return -1;
  }

  tbuf[0] = VR_TPS_REG_DEVID;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, VR_TPS_DEVID_LEN+1) < 0) {
    syslog(LOG_WARNING, "%s: read IC_DEVICE_ID failed", __func__);
    return -1;
  }

  for (i = 0; i < VR_TPS_DEVID_LEN; i++) {
    ((uint8_t *)devid)[i] = rbuf[VR_TPS_DEVID_LEN-i];
  }

  return 0;
}

static int
get_tps_crc(uint8_t bus, uint8_t addr, uint16_t *crc) {
  uint64_t devid = 0x00;
  uint8_t tbuf[16], rbuf[16];

  if (crc == NULL) {
    return -1;
  }

  if (get_tps_devid(bus, addr, &devid) < 0) {
    return -1;
  }

  tbuf[0] = (devid == VR_TPS53689_DEVID) || (devid == VR_TPS53685_DEVID)
                  ? VR_TPS_REG_CRC2 : VR_TPS_REG_CRC;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
    syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
    return -1;
  }

  memcpy(crc, rbuf, 2);
  return 0;
}

static int
cache_tps_crc(uint8_t fru_id, uint8_t bus, uint8_t addr, char *key, char *checksum, uint16_t *crc) {
  uint16_t tmp_crc = 0;
  uint16_t remain = 0;
  char tmp_str[MAX_VALUE_LEN] = {0};

  if (key == NULL) {
    return -1;
  }

  if (!crc) {
    crc = &tmp_crc;
  }
  if (get_tps_crc(bus, addr, crc) < 0) {
    return -1;
  }

  if (pal_load_tps_remaining_wr(fru_id, addr, &remain, checksum, crc, GET_VR_CRC) != 0) {
    snprintf(checksum, MAX_VALUE_LEN, "Texas Instruments %04X", *crc);
  }

  if (!checksum) {
    checksum = tmp_str;
  }
  kv_set(key, checksum, 0, 0);

  return 0;
}

int
get_tps_ver(struct vr_info *info, char *ver_str) {
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
    if (cache_tps_crc(info->slot_id, info->bus, info->addr, key, tmp_str, NULL))
      return VR_STATUS_FAILURE;
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1))
    return VR_STATUS_FAILURE;

  return VR_STATUS_SUCCESS;
}

void *
tps_parse_file(struct vr_info *info, const char *path) {
  FILE *fp;
  char line[200], xdigit[8] = {0};
  char *token, *str;
  int i, idx = 0;
  struct tps_config *config = NULL;

  if (info == NULL || path == NULL) {
    return NULL;
  }

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
cal_crc16(uint8_t const *data, int len) {
  uint16_t crc = 0x0000;
  int i, b;

  if (data == NULL) {
    return 0;
  }

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

  if (data == NULL) {
    return -1;
  }

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
program_tps(uint8_t fru_id, uint8_t bus, uint8_t addr, struct tps_config *config, bool force) {
  int i, ret = -1;
  uint64_t devid = 0x00;
  uint32_t offset = 0, dsize;
  uint16_t crc = 0;
  uint16_t remain = 0;
  uint8_t tbuf[64], rbuf[64];
  char checksum[MAX_KEY_LEN] = {0};

  if (config == NULL) {
    return -1;
  }

  // check device ID
  if (get_tps_devid(bus, addr, &devid) < 0) {
    return -1;
  }

  if (memcmp(&config->devid_exp, &devid, VR_TPS_DEVID_LEN)) {
    syslog(LOG_WARNING, "%s: IC_DEVICE_ID %llx mismatch, expect %llx", __func__, devid, config->devid_exp);
    return -1;
  }

  if (get_tps_crc(bus, addr, &crc) < 0) {
    return -1;
  }

  // check remaining writes
  if (pal_load_tps_remaining_wr(fru_id, addr, &remain, checksum, &crc, GET_VR_CRC) == 0) {
    if (!force && (remain <= VR_WARN_REMAIN_WR)) {
      printf("WARNING: the remaining writes is below the threshold value %d!\n",
           VR_WARN_REMAIN_WR);
      printf("Please use \"--force\" option to try again.\n");
      syslog(LOG_WARNING, "%s: insufficient remaining writes %u", __func__, remain);
      return -1;
    }
  }

  if (!force && (crc == config->crc_exp)) {
    printf("WARNING: the CRC is the same as used now %04X!\n", crc);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: redundant programming", __func__);
    return -1;
  }

  // set USER_NVM_INDEX to 0
  tbuf[0] = VR_TPS_REG_NVM_IDX;
  tbuf[1] = 0x00;
  if (vr_xfer(bus, addr, tbuf, 2, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: write NVM index failed", __func__);
    return -1;
  }

  // write configuration data
  dsize = VR_TPS_TOTAL_WR_SIZE/10;
  for (i = 0; i < VR_TPS_NVM_IDX_NUM; i++) {
    tbuf[0] = VR_TPS_REG_NVM_EXE;
    memcpy(&tbuf[1], &config->data[offset], VR_TPS_BLK_WR_LEN);
    if ((ret = vr_xfer(bus, addr, tbuf, VR_TPS_BLK_WR_LEN+1, rbuf, 0)) < 0) {
      break;
    }

    offset += VR_TPS_BLK_WR_LEN;
    printf("\rupdated: %d %%  ", (int)(offset/dsize)*10);
    fflush(stdout);
  }
  printf("\n");
  if (ret) {
    return -1;
  }
  msleep(200);

  return 0;
}

int
tps_fw_update(struct vr_info *info, void *args) {
  struct tps_config *config = (struct tps_config *)args;
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  uint16_t remain = 0;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  printf("Update VR: %s\n", info->dev_name);
  if (check_tps_image(config->crc_exp, config->data)) {
    return VR_STATUS_FAILURE;
  }

  if (info->xfer) {
    vr_xfer = info->xfer;
  }
  if (program_tps(info->slot_id, info->bus, info->addr, config, info->force)) {
    return VR_STATUS_FAILURE;
  }

  if (pal_is_support_vr_delay_activate() && info->private_data) {
    vr_get_fw_avtive_key(info, ver_key);

    if (pal_load_tps_remaining_wr(info->slot_id, info->addr, &remain, value, &config->crc_exp, UPDATE_VR_CRC) != 0) {
      snprintf(value, MAX_VALUE_LEN, "Texas Instruments %04X", config->crc_exp);
    }
    kv_set(ver_key, value, 0, KV_FPERSIST);
  }

  return VR_STATUS_SUCCESS;
}

int
tps_fw_verify(struct vr_info *info, void *args) {
  struct tps_config *config = (struct tps_config *)args;
  char key[MAX_KEY_LEN];
  uint16_t crc = 0;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  // update checksum
  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }
  if (cache_tps_crc(info->slot_id, info->bus, info->addr, key, NULL, &crc)) {
    return VR_STATUS_FAILURE;
  }

  if (crc != config->crc_exp) {
    printf("CRC %04X mismatch, expect %04X\n", crc, config->crc_exp);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}
