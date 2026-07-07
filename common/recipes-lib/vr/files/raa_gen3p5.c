#include "raa_gen3p5.h"
#include <openbmc/kv.h>
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-pal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

extern int vr_rdwr(uint8_t, uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t) =
    &vr_rdwr;

static bool debug = false;

static int raa_dma_rd(uint8_t bus, uint8_t addr, uint8_t* reg, uint8_t* resp) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = RAA_GEN3P5_CMD_DMAADDR;
  memcpy(&tbuf[1], reg, 2);
  if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
    syslog(
        LOG_WARNING,
        "%s: write register failed. Data[0]: 0x%02X, Data[1]: 0x%02X",
        __func__,
        reg[0],
        reg[1]);
    return VR_STATUS_FAILURE;
  }

  tbuf[0] = RAA_GEN3P5_CMD_DMAFIX;
  if (vr_xfer(bus, addr, tbuf, 1, resp, 4) < 0) {
    syslog(
        LOG_WARNING,
        "%s: dma read failed. Data[0]: 0x%02X, Data[1]: 0x%02X",
        __func__,
        reg[0],
        reg[1]);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

static int raa_dma_wr(uint8_t bus, uint8_t addr, uint8_t* reg, uint8_t* req) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = RAA_GEN3P5_CMD_DMAADDR;
  memcpy(&tbuf[1], reg, 2);
  if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
    syslog(
        LOG_WARNING,
        "%s: write register failed. Data[0]: 0x%02X, Data[1]: 0x%02X",
        __func__,
        reg[0],
        reg[1]);
    return VR_STATUS_FAILURE;
  }

  tbuf[0] = RAA_GEN3P5_CMD_DMAFIX;
  memcpy(&tbuf[1], req, 4);
  if (vr_xfer(bus, addr, tbuf, 5, rbuf, 0) < 0) {
    syslog(
        LOG_WARNING,
        "%s: dma write failed. Data[0]: 0x%02X, Data[1]: 0x%02X",
        __func__,
        reg[0],
        reg[1]);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

static int disable_packet_capture(uint8_t bus, uint8_t addr) {
  uint8_t reg[2], tbuf[8], rbuf[8];

  // step 1a, Retrieve Device Data
  reg[0] = RAA_GEN3P5_DMAADDR_DISABLE_PKT_CAP & 0xFF;
  reg[1] = (RAA_GEN3P5_DMAADDR_DISABLE_PKT_CAP >> 8) & 0xFF;
  if (raa_dma_rd(bus, addr, reg, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read dma failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (debug) {
    printf(
        "%s original pkt cap data: [0x%02X 0x%02X 0x%02X 0x%02X]\n",
        __func__,
        rbuf[0],
        rbuf[1],
        rbuf[2],
        rbuf[3]);
  }

  // step 1b, Disable Packet Capture
  memcpy(&tbuf[0], rbuf, 4);
  tbuf[0] &= ~(1 << 5); // clear bit 5
  if (raa_dma_wr(bus, addr, reg, tbuf) < 0) {
    syslog(LOG_WARNING, "%s: write dma failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (debug) {
    printf(
        "%s modified pkt cap data: [0x%02X 0x%02X 0x%02X 0x%02X]\n",
        __func__,
        tbuf[0],
        tbuf[1],
        tbuf[2],
        tbuf[3]);
  }

  // step 1c, Apply Packet Capture Changes
  tbuf[0] = 0xE7;
  tbuf[1] = 0x02;
  tbuf[2] = 0x00;
  if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: apply change failed", __func__);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

static int get_remaining_wr(uint8_t bus, uint8_t addr, uint8_t* remain) {
  uint8_t reg[2], rbuf[8];

  // step 2, Retrieve NVM Counter Data
  reg[0] = RAA_GEN3P5_DMAADDR_REMAIN_WR & 0xFF;
  reg[1] = (RAA_GEN3P5_DMAADDR_REMAIN_WR >> 8) & 0xFF;
  if (raa_dma_rd(bus, addr, reg, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read NVM counter failed", __func__);
    return VR_STATUS_FAILURE;
  }
  *remain = rbuf[0];

  return VR_STATUS_SUCCESS;
}

static int get_device_id(uint8_t bus, uint8_t addr, uint32_t* devid) {
  uint8_t tbuf[8], rbuf[8];

  // step 3b, Read IC_DEVICE_ID from Device
  tbuf[0] = RAA_GEN3P5_CMD_IC_DEVICE_ID;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, RAA_GEN3P5_IC_DEVICE_ID_LEN + 1) < 0) {
    syslog(LOG_WARNING, "%s: read IC_DEVICE_ID failed", __func__);
    return VR_STATUS_FAILURE;
  }
  memcpy(devid, &rbuf[1], RAA_GEN3P5_IC_DEVICE_ID_LEN);

  return VR_STATUS_SUCCESS;
}

static int get_device_rev(uint8_t bus, uint8_t addr, uint32_t* rev) {
  uint8_t tbuf[8], rbuf[8];

  // step 3c, Read IC_DEVICE_REV from Device
  tbuf[0] = RAA_GEN3P5_CMD_IC_DEVICE_REV;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, RAA_GEN3P5_IC_DEVICE_REV_LEN + 1) < 0) {
    syslog(LOG_WARNING, "%s: read IC_DEVICE_REV failed", __func__);
    return VR_STATUS_FAILURE;
  }
  memcpy(rev, &rbuf[1], RAA_GEN3P5_IC_DEVICE_REV_LEN);

  return VR_STATUS_SUCCESS;
}

static int check_dev_rev(uint32_t rev) {
  uint8_t rev_highest = (rev >> 24) & 0xFF;

  if (rev_highest < 0x03 || rev_highest > 0x06) {
    syslog(LOG_WARNING, "%s: unexpected IC_DEVICE_REV %08X", __func__, rev);
    return VR_STATUS_FAILURE;
  }
  return VR_STATUS_SUCCESS;
}

static int poll_programmer_status(uint8_t bus, uint8_t addr) {
  uint8_t reg[2], rbuf[8];
  int retry = 3;

  // step 5a, Poll PROGRAMMER_STATUS Register
  // If after a 2 second timeout bit 0 = 0, the part has failed programming.
  do {
    reg[0] = RAA_GEN3P5_DMAADDR_PROGRAMMER_STATUS & 0xFF;
    reg[1] = (RAA_GEN3P5_DMAADDR_PROGRAMMER_STATUS >> 8) & 0xFF;
    if (raa_dma_rd(bus, addr, reg, rbuf) < 0) {
      syslog(LOG_WARNING, "%s: read polling status failed", __func__);
      return VR_STATUS_FAILURE;
    }

    if (debug) {
      printf("%s retry %d, rbuf[0] = 0x%02X\n", __func__, retry, rbuf[0]);
    }

    if (rbuf[0] & 0x01) {
      break;
    }

    if ((--retry) <= 0) {
      syslog(LOG_WARNING, "%s: Failed to program the device", __func__);
      return VR_STATUS_FAILURE;
    }
    sleep(1);
  } while (retry > 0);

  return VR_STATUS_SUCCESS;
}

static int get_bank_status(uint8_t bus, uint8_t addr) {
  uint8_t reg[2], rbuf[8];

  // step 2, Retrieve NVM Counter Data
  reg[0] = RAA_GEN3P5_DMAADDR_BANK_STATUS & 0xFF;
  reg[1] = (RAA_GEN3P5_DMAADDR_BANK_STATUS >> 8) & 0xFF;
  if (raa_dma_rd(bus, addr, reg, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read NVM counter failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (debug) {
    printf(
        "%s bank status: [0x%02X 0x%02X 0x%02X 0x%02X]\n",
        __func__,
        rbuf[0],
        rbuf[1],
        rbuf[2],
        rbuf[3]);
  }

  return VR_STATUS_SUCCESS;
}

static int get_crc(uint8_t bus, uint8_t addr, uint32_t* crc) {
  uint8_t reg[2], rbuf[8];

  reg[0] = RAA_GEN3P5_DMAADDR_CRC & 0xFF;
  reg[1] = (RAA_GEN3P5_DMAADDR_CRC >> 8) & 0xFF;
  if (raa_dma_rd(bus, addr, reg, rbuf) < 0) {
    syslog(LOG_WARNING, "%s: read CRC failed", __func__);
    return VR_STATUS_FAILURE;
  }
  memcpy(crc, rbuf, sizeof(uint32_t));

  return VR_STATUS_SUCCESS;
}

static int cache_crc(
    uint8_t bus,
    uint8_t addr,
    char* key,
    char* show_info,
    uint32_t* checksum) {
  uint8_t remain;
  uint32_t tmp_sum;
  char tmp_str[MAX_VALUE_LEN] = {0};

  if (get_remaining_wr(bus, addr, &remain) < 0) {
    return VR_STATUS_FAILURE;
  }

  if (!checksum) {
    checksum = &tmp_sum;
  }

  if (get_crc(bus, addr, checksum) < 0) {
    return VR_STATUS_FAILURE;
  }

  if (!show_info) {
    show_info = tmp_str;
  }
  snprintf(
      show_info,
      MAX_VALUE_LEN,
      "Renesas %08X, Remaining Writes: %u",
      *checksum,
      remain);
  kv_set(key, show_info, 0, 0);

  return VR_STATUS_SUCCESS;
}

int get_raa_gen3p5_ver(struct vr_info* info, char* ver_str) {
  int ret, lock = -1;
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  if (info->private_data) {
    snprintf(
        key,
        sizeof(key),
        "%s_vr_%02xh_crc",
        (char*)info->private_data,
        info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }

  char key_debug[MAX_KEY_LEN] = {0}, str_debug[MAX_VALUE_LEN] = {0};
  snprintf(key_debug, sizeof(key_debug), "vr_%02xh_debug", info->addr);
  if (kv_get(key_debug, str_debug, NULL, 0) == 0) {
    debug = (atoi(str_debug) == 1);
  } else {
    debug = false;
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
    ret = cache_crc(info->bus, info->addr, key, tmp_str, NULL);
    if (lock >= 0) {
      single_instance_unlock(lock);
    }

    if (ret) {
      return VR_STATUS_FAILURE;
    }
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) >
      (MAX_VER_STR_LEN - 1)) {
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

void* raa_gen3p5_parse_file(struct vr_info* info, const char* path) {
  FILE* fp = NULL;
  char line[64], xdigit[8] = {0};
  int i, valid_cnt, cmd_data_len, dcnt = 0, dcnt_nowr = 0;
  size_t raa_gen3p5_supported_dev_ids_len =
      sizeof(raa_gen3p5_supported_dev_ids) /
      sizeof(raa_gen3p5_supported_dev_ids[0]);
  bool supported = false;
  uint8_t buf[32] = {0};
  struct raa_gen3p5_config* config = NULL;

  char key_debug[MAX_KEY_LEN] = {0}, str_debug[MAX_VALUE_LEN] = {0};
  snprintf(key_debug, sizeof(key_debug), "vr_%02xh_debug", info->addr);
  if (kv_get(key_debug, str_debug, NULL, 0) == 0) {
    debug = (atoi(str_debug) == 1);
  } else {
    debug = false;
  }

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config =
      (struct raa_gen3p5_config*)calloc(1, sizeof(struct raa_gen3p5_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    memset(buf, 0, sizeof(buf));

    for (i = 0, valid_cnt = 0; i < strlen(line) && valid_cnt < sizeof(buf);
         i += 2, valid_cnt++) {
      memcpy(xdigit, &line[i], 2);
      buf[valid_cnt] = strtol(xdigit, NULL, 16);
    }

    if (debug) {
      for (i = 0; i < valid_cnt; i++) {
        printf("%02X ", buf[i]);
      }
      printf("\n");
    }

    cmd_data_len = 0;
    if (valid_cnt > 1 && buf[1] >= 3) {
      // Number of bytes - PMBus Address(1 byte) - CMD code(1 byte) - Packet
      // Error Code(1 byte)
      cmd_data_len = buf[1] - 3;
    }

    if (buf[0] == 0x49) {
      if (buf[3] == RAA_GEN3P5_CMD_IC_DEVICE_ID) {
        if ((4 + RAA_GEN3P5_IC_DEVICE_ID_LEN) > valid_cnt ||
            cmd_data_len != RAA_GEN3P5_IC_DEVICE_ID_LEN) {
          printf("ERROR: Device ID data malformed or buffer overflow risk!\n");
          goto error_cleanup;
        }
        for (i = 0; i < RAA_GEN3P5_IC_DEVICE_ID_LEN; i++) {
          ((uint8_t*)&config->devid_exp)[i] =
              buf[3 + RAA_GEN3P5_IC_DEVICE_ID_LEN - i];
        }

        supported = false;
        for (i = 0; i < raa_gen3p5_supported_dev_ids_len; i++) {
          if (config->devid_exp == raa_gen3p5_supported_dev_ids[i]) {
            supported = true;
            break;
          }
        }
        if (!supported) {
          printf(
              "%s: unsupported device id 0x%08X\n",
              __func__,
              config->devid_exp);
          goto error_cleanup;
        }

        config->addr = buf[2];
      } else if (buf[3] == RAA_GEN3P5_CMD_IC_DEVICE_REV) {
        if ((4 + RAA_GEN3P5_IC_DEVICE_REV_LEN) > valid_cnt ||
            cmd_data_len != RAA_GEN3P5_IC_DEVICE_REV_LEN) {
          printf(
              "ERROR: Device Revision data malformed or buffer overflow risk!\n");
          goto error_cleanup;
        }
        for (i = 0; i < RAA_GEN3P5_IC_DEVICE_REV_LEN; i++) {
          ((uint8_t*)&config->rev_exp)[i] = buf[4 + i];
        }
      }

      dcnt_nowr++;
    } else if (buf[0] == 0x00) {
      if (buf[1] < 2 || (buf[1] + 2) > valid_cnt ||
          dcnt >= RAA_GEN3P5_CONFIG_PDATA_LEN) {
        printf(
            "%s: invalid length or buffer overflow risk detected at dcnt %d\n",
            __func__,
            dcnt);
        goto error_cleanup;
      }

      config->pdata[dcnt].len = buf[1] - 2;
      config->pdata[dcnt].pec = buf[3 + config->pdata[dcnt].len];

      if ((config->pdata[dcnt].len + 1) > sizeof(config->pdata[dcnt].raw)) {
        printf("ERROR: Payload size exceeds target buffer raw capacity!\n");
        goto error_cleanup;
      }
      memcpy(config->pdata[dcnt].raw, &buf[2], config->pdata[dcnt].len + 1);

      if (dcnt == (RAA_GEN3P5_FW_CRC_LINE - dcnt_nowr - 1)) {
        if (RAA_GEN3P5_CRC_LEN + 4 > valid_cnt ||
            cmd_data_len != RAA_GEN3P5_CRC_LEN) {
          printf("ERROR: CRC length invalid or buffer overflow risk!\n");
          goto error_cleanup;
        }

        memcpy(&config->crc_exp, &buf[4], RAA_GEN3P5_CRC_LEN);
        printf(
            "Configuration GEN3.5 CRC (Production): %08X\n", config->crc_exp);
      }

      dcnt++;
    } else {
      continue;
    }
  }
  fclose(fp);
  fp = NULL;

  printf("dev id  = 0x%08X\n", config->devid_exp);
  printf("dev rev = 0x%08X\n", config->rev_exp);
  printf("crc exp = 0x%08X\n", config->crc_exp);

  config->wr_cnt = dcnt;
  if (!config->wr_cnt || !config->addr || !config->crc_exp) {
    goto error_cleanup;
  }

  return config;

error_cleanup:
  if (config)
    free(config);
  config = NULL;
  if (fp)
    fclose(fp);
  return NULL;
}

static uint8_t cal_crc8(uint8_t const* data, int len) {
  uint8_t crc = 0x00;
  int i, b;

  for (i = 0; i < len; i++) {
    crc ^= data[i];
    for (b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07; // polynomial 0x07
      } else {
        crc = (crc << 1);
      }
    }
  }

  return crc;
}

static int check_raa_gen3p5_image(struct raa_gen3p5_config* config) {
  int i;
  uint8_t crc8;

  for (i = 0; i < config->wr_cnt; i++) {
    crc8 = cal_crc8(config->pdata[i].raw, config->pdata[i].len + 1);
    if (crc8 != config->pdata[i].pec) {
      syslog(
          LOG_WARNING,
          "CRC8[%d] %02X mismatch, expect %02X",
          i,
          crc8,
          config->pdata[i].pec);
      return VR_STATUS_FAILURE;
    }
  }

  return VR_STATUS_SUCCESS;
}

static int program_raa_gen3p5(
    uint8_t bus,
    uint8_t addr,
    struct raa_gen3p5_config* config,
    bool force) {
  int i, ret = -1;
  uint8_t tbuf[32], rbuf[16];
  uint8_t remain = 0;
  uint32_t devid = 0, rev = 0, crc = 0;

  if (get_crc(bus, addr, &crc) < 0) {
    syslog(LOG_WARNING, "%s: get CRC failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (!force && (crc == config->crc_exp)) {
    printf("WARNING: the CRC is the same as used now %08X!\n", crc);
    printf("Please use \"--force\" option to try again.\n");
    syslog(LOG_WARNING, "%s: redundant programming", __func__);
    return VR_STATUS_FAILURE;
  }

  // check remaining writes
  if (get_remaining_wr(bus, addr, &remain) < 0) {
    syslog(LOG_WARNING, "%s: get remaining writes failed", __func__);
    return VR_STATUS_FAILURE;
  }

  printf("Remaining writes: %u\n", remain);
  if (!remain) {
    syslog(LOG_WARNING, "%s: no remaining writes", __func__);
    return VR_STATUS_FAILURE;
  }

  if (!force && (remain <= VR_WARN_REMAIN_WR)) {
    printf(
        "WARNING: the remaining writes is below the threshold value %d!\n",
        VR_WARN_REMAIN_WR);
    printf("Please use \"--force\" option to try again.\n");
    syslog(
        LOG_WARNING, "%s: insufficient remaining writes %u", __func__, remain);
    return VR_STATUS_FAILURE;
  }

  // check device id
  if (get_device_id(bus, addr, &devid) < 0) {
    syslog(LOG_WARNING, "%s: get device ID failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (devid != config->devid_exp) {
    syslog(
        LOG_WARNING,
        "%s: IC_DEVICE_ID 0x%08X mismatch, expect 0x%08X",
        __func__,
        devid,
        config->devid_exp);
    return VR_STATUS_FAILURE;
  }

  // check device revision
  if (get_device_rev(bus, addr, &rev) < 0) {
    syslog(LOG_WARNING, "%s: get device revision failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (check_dev_rev(rev) < 0) {
    syslog(LOG_WARNING, "%s: device revision check failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if (disable_packet_capture(bus, addr) < 0) {
    syslog(LOG_WARNING, "%s: disable packet capture failed", __func__);
    return VR_STATUS_FAILURE;
  }

  // write configuration data
  for (i = 0; i < config->wr_cnt; i++) {
    memcpy(tbuf, &config->pdata[i].cmd, config->pdata[i].len);
    if (debug) {
      printf("Write RAA[%d]:", i);
      for (int j = 0; j < config->pdata[i].len; j++) {
        printf(" %02X", tbuf[j]);
      }
      printf("\n");
    }
    if ((ret = vr_xfer(bus, addr, tbuf, config->pdata[i].len, rbuf, 0))) {
      break;
    }
    printf("\rupdated: %d %%  ", ((i + 1) * 100) / config->wr_cnt);
    fflush(stdout);
  }
  printf("\n");
  if (ret) {
    return VR_STATUS_FAILURE;
  }

  // check the status
  if (poll_programmer_status(bus, addr) < 0) {
    return VR_STATUS_FAILURE;
  }

  if (debug) {
    if (get_bank_status(bus, addr) < 0) {
      syslog(LOG_WARNING, "%s: get bank status failed", __func__);
      return VR_STATUS_FAILURE;
    }
  }

  return VR_STATUS_SUCCESS;
}

int raa_gen3p5_fw_update(struct vr_info* info, void* args) {
  struct raa_gen3p5_config* config = (struct raa_gen3p5_config*)args;

  char key_debug[MAX_KEY_LEN] = {0}, str_debug[MAX_VALUE_LEN] = {0};
  snprintf(key_debug, sizeof(key_debug), "vr_%02xh_debug", info->addr);
  if (kv_get(key_debug, str_debug, NULL, 0) == 0) {
    debug = (atoi(str_debug) == 1);
  } else {
    debug = false;
  }

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  if (info->addr != config->addr) {
    printf(
        "ERROR: The 7-bit address in the FW file is 0x%02x, but the device address is 0x%02x\n",
        config->addr,
        info->addr);
    syslog(
        LOG_WARNING,
        "%s: address mismatch; please use the correct FW file",
        __func__);
    return VR_STATUS_FAILURE;
  }

  printf("Update VR: %s\n", info->dev_name);
  if (check_raa_gen3p5_image(config)) {
    return VR_STATUS_FAILURE;
  }

  if (info->xfer) {
    vr_xfer = info->xfer;
  } else {
    vr_xfer = &vr_rdwr;
  }

  if (program_raa_gen3p5(info->bus, info->addr, config, info->force)) {
    syslog(LOG_WARNING, "%s: program failed", __func__);
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}
