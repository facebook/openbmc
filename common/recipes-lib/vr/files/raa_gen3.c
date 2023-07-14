#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "raa_gen3.h"

//#define VR_RAA_DEBUG
extern int vr_rdwr(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t) = &vr_rdwr;

static int
raa_dma_rd(uint8_t bus, uint8_t addr, uint8_t *reg, uint8_t *resp) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = VR_RAA_REG_DMA_ADDR;
  memcpy(&tbuf[1], reg, 2);
  if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, reg[0]);
    return -1;
  }

  tbuf[0] = VR_RAA_REG_DMA_DATA;
  if (vr_xfer(bus, addr, tbuf, 1, resp, 4) < 0) {
    syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, reg[0]);
    return -1;
  }

  return 0;
}


static int
get_raa_remaining_wr(uint8_t bus, uint8_t addr, uint8_t mode, uint8_t *remain) {
  uint8_t tbuf[8], rbuf[8];

  if(mode == RAA_GEN2)
    tbuf[0] = VR_RAA_REG_GEN2_REMAIN_WR;
  else
    tbuf[0] = VR_RAA_REG_REMAIN_WR;
  tbuf[1] = 0x00;
  if (raa_dma_rd(bus, addr, tbuf, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read NVM counter failed", __func__);
    return -1;
  }
  *remain = rbuf[0];

  return 0;
}

static int
get_raa_hex_mode(uint8_t bus, uint8_t addr, uint8_t *mode) {
  uint8_t tbuf[8], rbuf[8];

  tbuf[0] = VR_RAA_REG_DEVID;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, VR_RAA_DEV_ID_LEN+1) < 0) {
    syslog(LOG_WARNING, "%s: read IC_DEVICE_ID failed", __func__);
    return -1;
  }

#ifdef VR_RAA_DEBUG
  for(int i=0; i<5; i++) {
    printf("%s rbuf[%d]=%x\n", __func__, i, rbuf[i]);
  }
#endif

  if( rbuf[2] < 0x70 ) {
    *mode = RAA_GEN2;
  } else {
    tbuf[0] = VR_RAA_REG_HEX_MODE_CFG0;
    tbuf[1] = VR_RAA_REG_HEX_MODE_CFG1;
    if (raa_dma_rd(bus, addr, tbuf, rbuf) < 0) {
      syslog(LOG_WARNING, "%s: read HEX mode failed", __func__);
      return -1;
    }
    *mode = (rbuf[0] == 0) ? RAA_GEN3_LEGACY : RAA_GEN3_PRODUCTION;
  }

#ifdef VR_RAA_DEBUG
  if(*mode == RAA_GEN2)
    printf("Mode GEN2\n");
  else
    printf("Mode GEN3\n");
#endif

  return 0;
}

static int
get_raa_devid(uint8_t bus, uint8_t addr, uint32_t *devid) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = VR_RAA_REG_DEVID;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, VR_RAA_DEV_ID_LEN+1) < 0) {
    syslog(LOG_WARNING, "%s: read IC_DEVICE_ID failed", __func__);
    return -1;
  }
  memcpy(devid, &rbuf[1], VR_RAA_DEV_ID_LEN);

  return 0;
}

static int
get_raa_dev_rev(uint8_t bus, uint8_t addr, uint32_t *rev, uint8_t mode) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = VR_RAA_REG_DEVREV;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, VR_RAA_DEV_REV_LEN+1) < 0) {
    syslog(LOG_WARNING, "%s: read IC_DEVICE_REV failed", __func__);
    return -1;
  }

  if (mode == RAA_GEN3_LEGACY) {
    memcpy(rev, &rbuf[1], VR_RAA_DEV_REV_LEN);
  } else {
    for(int i=0; i < VR_RAA_DEV_REV_LEN; i++) {
      ((uint8_t *)rev)[i] = rbuf[VR_RAA_DEV_REV_LEN-i];
    }
  }
  return 0;
}

uint32_t
swap_uint32( uint32_t val ){
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

static int
check_dev_rev(uint32_t rev, uint8_t mode) {
  uint8_t sw_rev = rev & 0xFF;
  uint8_t hw_rev = (rev >> 24) & 0xFF;

  switch (mode) {
    case RAA_GEN2:
      if ( sw_rev < VR_RAA_GEN2_SW_REV_MIN ||
           hw_rev < VR_RAA_GEN2_HW_REV_MIN ) {
        syslog(LOG_WARNING, "%s: GEN2 unexpected IC_DEVICE_REV %08X", __func__, rev);
        return -1;
      }
      break;
    case RAA_GEN3_LEGACY:
    case RAA_GEN3_PRODUCTION:
      // Confirmed with Renesas FAE,
      // We can ignore the IC_DEVICE_REV check.
      // But we need to make sure that legacy IC use legacy HEX and production IC use production HEX.
      // if ( sw_rev < VR_RAA_GEN3_SW_REV_MIN ) {
      //   syslog(LOG_WARNING, "%s: GEN3 unexpected IC_DEVICE_REV %08X", __func__, rev);
      //   return -1;
      // }
      break;
    default:
      syslog(LOG_WARNING, "RAA Mode not support");
      return -1;
  }
  return 0;
}

static int
get_raa_polling_status(uint8_t bus, uint8_t addr, uint8_t mode) {
  uint8_t tbuf[8], rbuf[8];
  int retry = 3;

  do {
    if (mode == RAA_GEN2) {
      tbuf[0] = VR_RAA_REG_GEN2_PROG_STATUS;
      tbuf[1] = VR_RAA_REG_GEN2_PROG_STATUS;
    } else {
      tbuf[0] = VR_RAA_REG_PROG_STATUS;
      tbuf[1] = 0x00;
    }
    if (raa_dma_rd(bus, addr, tbuf, rbuf) < 0) {
      syslog(LOG_WARNING, "%s: read polling status failed", __func__);
      return -1;
    }
#ifdef VR_RAA_DEBUG
    printf("%s rbuf[0]=%x\n", __func__, rbuf[0]);
#endif

    // bit1 is held to 1, it means the action is successful
    if (rbuf[0] & 0x01) {
      break;
    }

    if ((--retry) <= 0) {
      syslog(LOG_WARNING, "%s: Failed to program the device", __func__);
      return -1;
    }
    sleep(1);
  } while (retry > 0);

  return 0;
}

static int
set_raa_restore_cfg(uint8_t bus, uint8_t addr, uint8_t cfg_id) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = VR_RAA_REG_RESTORE_CFG;
  tbuf[1] = cfg_id;
  if (vr_xfer(bus, addr, tbuf, 2, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: set restore cfg failed", __func__);
    return -1;
  }

  return 0;
}

static int
get_raa_crc(uint8_t bus, uint8_t addr, uint8_t mode, uint32_t *crc) {
  uint8_t tbuf[8], rbuf[8];

  if(mode == RAA_GEN2)
    tbuf[0] = VR_RAA_REG_GEN2_CRC;
  else
    tbuf[0] = VR_RAA_REG_CRC;
  tbuf[1] = 0x00;
  if (raa_dma_rd(bus, addr, tbuf, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read CRC failed", __func__);
    return -1;
  }
  memcpy(crc, rbuf, sizeof(uint32_t));

  return 0;
}

static int
cache_raa_crc(uint8_t bus, uint8_t addr, char *key, char *show_info, uint32_t *checksum) {
  uint8_t remain;
  uint8_t mode;
  uint32_t tmp_sum;
  char tmp_str[MAX_VALUE_LEN] = {0};

  if (get_raa_hex_mode(bus, addr, &mode)) {
    return -1;
  }

  if (get_raa_remaining_wr(bus, addr, mode, &remain) < 0) {
    return -1;
  }

  if (!checksum) {
    checksum = &tmp_sum;
  }

  if (get_raa_crc(bus, addr, mode, checksum) < 0) {
    return -1;
  }

  if (!show_info) {
    show_info = tmp_str;
  }
  snprintf(show_info, MAX_VALUE_LEN, "Renesas %08X, Remaining Writes: %u",
           *checksum, remain);
  kv_set(key, show_info, 0, 0);

  return 0;
}

static int
get_raa_mcu_ver(uint8_t bus, uint8_t addr, uint32_t *ver) {
  uint8_t tbuf[8], rbuf[8];

  tbuf[0] = VR_RAA_REG_MCU_VER;
  tbuf[1] = 0x00;
  if (raa_dma_rd(bus, addr, tbuf, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read MCU version failed", __func__);
    return -1;
  }
  memcpy(ver, rbuf, sizeof(uint32_t));

  return 0;
}

int
cache_raa_mcu_ver(uint8_t bus, uint8_t addr, char *key, char *version) {
  uint32_t ver;

  if (get_raa_mcu_ver(bus, addr, &ver) < 0) {
    return -1;
  }

  snprintf(version, MAX_VALUE_LEN, "Renesas %08X", ver);
  kv_set(key, version, 0, 0);

  return 0;
}

int
get_raa_ver(struct vr_info *info, char *ver_str) {
  int ret, lock = -1;
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }

  if (kv_get(key, tmp_str, NULL, 0)) {
    if (info->xfer) {
      vr_xfer = info->xfer;
    } else {
      vr_xfer = &vr_rdwr;
    }

    if ((lock = single_instance_lock_blocked(key)) < 0) {
      syslog(LOG_WARNING, "%s: Failed to get %s lock", __func__, key);
    }
    ret = cache_raa_crc(info->bus, info->addr, key, tmp_str, NULL);
    if (lock >= 0) {
      single_instance_unlock(lock);
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
raa_parse_file(struct vr_info *info, const char *path) {
  FILE *fp = NULL;
  char line[64], xdigit[8] = {0};
  int i, j, dcnt = 0;
  uint8_t buf[32];
  struct raa_config *config = NULL;

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct raa_config *)calloc(1, sizeof(struct raa_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    for (i = 0, j = 0; i < strlen(line); i += 2, j++) {
      memcpy(xdigit, &line[i], 2);
      buf[j] = strtol(xdigit, NULL, 16);
    }
#ifdef VR_RAA_DEBUG
    for (i = 0; i < j; i++) {
      printf("%02X ", buf[i]);
    }
    printf("\n");
#endif

    if (buf[0] == 0x49 && buf[3] == 0xAD) {
      for (i = 0; i < VR_RAA_DEV_ID_LEN; i++) {
        ((uint8_t *)&config->devid_exp)[i] = buf[3+VR_RAA_DEV_ID_LEN-i];
      }
      config->addr = buf[2];
    } else if (buf[0] == 0x49 && buf[3] == 0xAE) {
      for (i = 0; i < VR_RAA_DEV_REV_LEN; i++) {
        ((uint8_t *)&config->rev_exp)[i] = buf[4+i];
      }
      if ((config->rev_exp & 0xFF) < VR_RAA_GEN3_SW_REV_MIN) {
        config->mode = RAA_GEN3_LEGACY;
      } else {
        config->mode = RAA_GEN3_PRODUCTION;
      }
    } else if (buf[0] == 0x49 && buf[3] == 0x00){ //GEN2 HEX FILE REVERSION
      config->mode = RAA_GEN2;
    } else if (buf[0] == 0x00) {
      if ((buf[1] + 2) >= sizeof(buf)) {
        dcnt = 0;
        break;
      }

      config->pdata[dcnt].len = buf[1] - 2;
      config->pdata[dcnt].pec = buf[3+config->pdata[dcnt].len];
      memcpy(config->pdata[dcnt].raw, &buf[2], config->pdata[dcnt].len+1);

      switch (dcnt) {
        case VR_RAA_CFG_ID:
          // set Configuration ID
          config->cfg_id = buf[4] & 0x0F;
          break;
        case VR_RAA_GEN3_LEGACY_CRC:
          if (config->mode == RAA_GEN3_LEGACY) {
            memcpy(&config->crc_exp, &buf[4], VR_RAA_CHECKSUM_LEN);
            printf("Configuration GEN3 CRC (Legacy): %08X\n", config->crc_exp);
          }
          break;
        case VR_RAA_GEN3_PRODUCTION_CRC:
          if (config->mode == RAA_GEN3_PRODUCTION) {
            memcpy(&config->crc_exp, &buf[4], VR_RAA_CHECKSUM_LEN);
            printf("Configuration GEN3 CRC (Production): %08X\n", config->crc_exp);
          }
          break;
        case VR_RAA_GEN2_CRC:
          if (config->mode == RAA_GEN2) {
            memcpy(&config->crc_exp, &buf[4], VR_RAA_CHECKSUM_LEN);
            printf("Configuration GEN2 CRC: %08X\n", config->crc_exp);
          }
          break;
      }

      dcnt++;
    } else {
      continue;
    }
  }
  fclose(fp);

#ifdef VR_RAA_DEBUG
  printf("dev id  = 0x%08X\n", config->devid_exp);
  printf("dev rev = 0x%08X\n", config->rev_exp);
  printf("mode    = %u\n", config->mode);
  printf("cfg id  = %u\n", config->cfg_id);
  printf("crc exp = 0x%08X\n", config->crc_exp);
#endif
  config->wr_cnt = dcnt;
  if (!config->wr_cnt || !config->addr || !config->crc_exp) {
    free(config);
    config = NULL;
  }

  return config;
}

static uint8_t
cal_crc8(uint8_t const *data, int len) {
  uint8_t crc = 0x00;
  int i, b;

  for (i = 0; i < len; i++) {
    crc ^= data[i];
    for (b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;  // polynomial 0x07
      } else {
        crc = (crc << 1);
      }
    }
  }

  return crc;
}

static int
check_raa_image(struct raa_config *config) {
  int i;
  uint8_t crc8;

  for (i = 0; i < config->wr_cnt; i++) {
    crc8 = cal_crc8(config->pdata[i].raw, config->pdata[i].len+1);
    if (crc8 != config->pdata[i].pec) {
      syslog(LOG_WARNING, "CRC8[%d] %02X mismatch, expect %02X", i, crc8,
             config->pdata[i].pec);
      return -1;
    }
  }

  return 0;
}

static int
program_raa(uint8_t bus, uint8_t addr, struct raa_config *config, bool force) {
  int i, ret = -1;
  uint8_t tbuf[32], rbuf[16];
  uint8_t remain = 0, mode = 0xff;
  uint32_t devid = 0, rev = 0, crc = 0;

  // check mode
  if (get_raa_hex_mode(bus, addr, &mode)) {
    return -1;
  }

  if (get_raa_crc(bus, addr, mode, &crc) < 0) {
    return -1;
  }

  if (!force && (crc == config->crc_exp)) {
    printf("WARNING: the CRC is the same as used now %08X!\n", crc);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: redundant programming", __func__);
    return -1;
  }

  // check remaining writes
  if (get_raa_remaining_wr(bus, addr, mode, &remain) < 0) {
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

  // check device id
  if (get_raa_devid(bus, addr, &devid) < 0) {
    return -1;
  }

  if (devid != config->devid_exp) {
    syslog(LOG_WARNING, "%s: IC_DEVICE_ID 0x%08X mismatch, expect 0x%08X",
           __func__, devid, config->devid_exp);
    return -1;
  }

  // check device revision
  if (get_raa_dev_rev(bus, addr, &rev, mode) < 0) {
    return -1;
  }

  if (check_dev_rev(rev, mode) < 0) {
    return -1;
  }

  // check mode
  if (mode != config->mode) {
    syslog(LOG_WARNING, "%s: HEX mode %u mismatch, expect %u", __func__,
           mode, config->mode);
    return -1;
  }

  // write configuration data
  for (i = 0; i < config->wr_cnt; i++) {
    memcpy(tbuf, &config->pdata[i].cmd, config->pdata[i].len);
#ifdef VR_RAA_DEBUG
    printf("Write RAA[%d]:", i);
    for (int j = 0; j < config->pdata[i].len; j++) {
      printf(" %02X", tbuf[j]);
    }
    printf("\n");
#endif
    if ((ret = vr_xfer(bus, addr, tbuf, config->pdata[i].len, rbuf, 0))) {
      break;
    }
    printf("\rupdated: %d %%  ", ((i+1)*100)/config->wr_cnt);
    fflush(stdout);
  }
  printf("\n");
  if (ret) {
    return -1;
  }

  // check the status
  if (get_raa_polling_status(bus, addr, mode) < 0) {
    return -1;
  }

  return 0;
}

int
raa_fw_update(struct vr_info *info, void *args) {
  struct raa_config *config = (struct raa_config *)args;
  uint8_t mode = 0;
  uint8_t remain = 0;
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};


  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  printf("Update VR: %s\n", info->dev_name);
  if (check_raa_image(config)) {
    return VR_STATUS_FAILURE;
  }

  if (info->xfer) {
    vr_xfer = info->xfer;
  } else {
    vr_xfer = &vr_rdwr;
  }

  if (program_raa(info->bus, info->addr, config, info->force)) {
    return VR_STATUS_FAILURE;
  }

  if (pal_is_support_vr_delay_activate() && info->private_data) {
    vr_get_fw_avtive_key(info, ver_key);

    if (get_raa_hex_mode(info->bus, info->addr, &mode) < 0) {
    snprintf(value, sizeof(value), "%08x", config->crc_exp);
      snprintf(value, sizeof(value), "Renesas %08X, Remaining Writes: Unknown",
             config->crc_exp);
    } else if (get_raa_remaining_wr(info->bus, info->addr, mode, &remain) < 0) {
      snprintf(value, sizeof(value), "Renesas %08X, Remaining Writes: Unknown",
             config->crc_exp);
    } else {
      snprintf(value, sizeof(value), "Renesas %08X, Remaining Writes: %u",
             config->crc_exp, remain);
    }

    kv_set(ver_key, value, 0, KV_FPERSIST);
  }

  return VR_STATUS_SUCCESS;
}

int
raa_fw_verify(struct vr_info *info, void *args) {
  struct raa_config *config = (struct raa_config *)args;
  char key[MAX_KEY_LEN];
  uint32_t crc;

  if (info->addr != config->addr) {
    return VR_STATUS_SKIP;
  }

  // restore cfg
  if (set_raa_restore_cfg(info->bus, info->addr, config->cfg_id)) {
    return VR_STATUS_FAILURE;
  }
  msleep(100);

  // update Checksum
  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }
   
  if (cache_raa_crc(info->bus, info->addr, key, NULL, &crc)) {
    return VR_STATUS_FAILURE;
  }

  if (crc != config->crc_exp) {
    printf("CRC %08X mismatch, expect %08X\n", crc, config->crc_exp);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}
