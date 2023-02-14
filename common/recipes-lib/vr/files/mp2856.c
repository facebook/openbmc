#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "mp2856.h"

extern int vr_rdwr(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
static int (*vr_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t) = &vr_rdwr;
static int (*mps_remaining_wr)(uint8_t addr, uint16_t *remain, bool is_update) = NULL;

static int
mp2856_set_page(uint8_t bus, uint8_t addr, uint8_t page) {
  uint8_t tbuf[16], rbuf[16];
  tbuf[0] = VR_REG_PAGE;
  tbuf[1] = page;
  if (vr_xfer(bus, addr, tbuf, 2, rbuf, 0) < 0) {
    syslog(LOG_WARNING, "%s: set page to 0x%02X failed", __func__, tbuf[1]);
    return VR_STATUS_FAILURE;
  }
  return VR_STATUS_SUCCESS;
}

static int
mp2856_is_pwd_unlock(uint8_t bus, uint8_t addr) {
  uint8_t tbuf[16], rbuf[16];
  tbuf[0] = VR_MPS_REG_STATUS_CML;
  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_0) < 0) {
    return VR_STATUS_FAILURE;
  }
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, 1) < 0) {
    syslog(LOG_WARNING, "%s: read 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }

  if( (rbuf[0] & MASK_PWD_MATCH) == 0x00) {
    printf("ERROR: PWD_MATCH not set!\n");
    return VR_STATUS_FAILURE;
  }
  return VR_STATUS_SUCCESS;
}

static int
mp2856_get_product_id(uint8_t bus, uint8_t addr, uint16_t *product_id ) {
  uint8_t tbuf[16], rbuf[16];

  tbuf[0] = VR_MPS_REG_PRODUCT_ID;
  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_0) < 0) {
    return VR_STATUS_FAILURE;
  }
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, 3) < 0) {
    syslog(LOG_WARNING, "%s: read 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }
  memcpy(product_id, &rbuf[1], 2);

  return VR_STATUS_SUCCESS;
}

static int
mp2856_enable_mtp_page_rw(uint8_t bus, uint8_t addr) {
  uint8_t tbuf[16], rbuf[16];

  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_1) < 0) {
    return VR_STATUS_FAILURE;
  }

  tbuf[0] = VR_MPS_REG_MFR_MTP_PMBUS_CTRL;
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
    syslog(LOG_WARNING, "%s: read 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }

  if ((rbuf[0] & MASK_MTP_BYTE_RW_EN) == 0) {
    tbuf[1]= rbuf[0] | MASK_MTP_BYTE_RW_EN ;
    tbuf[2]= rbuf[1];
    if (vr_xfer(bus, addr, tbuf, 3, rbuf, 0) < 0) {
      syslog(LOG_WARNING, "%s: write 0x%02X failed", __func__, tbuf[0]);
      return VR_STATUS_FAILURE;
    }
  }

  return VR_STATUS_SUCCESS;
}

static int
mp2856_unlock_write_protect_mode(uint8_t bus, uint8_t addr) {
  uint8_t tbuf[16], rbuf[16];
  tbuf[0] = VR_MPS_REG_MFR_VR_CONFIG2;
  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_1) < 0) {
    return VR_STATUS_FAILURE;
  }
  if (vr_xfer(bus, addr, tbuf, 1, rbuf, 2) < 0) {
    syslog(LOG_WARNING, "%s: read 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }

  if((rbuf[1] & MASK_WRITE_PROTECT_MODE) == 0) {
    //MTP protection mode
    //check write protect status
    if(mp2856_set_page(bus, addr, VR_MPS_PAGE_0) < 0) {
      return VR_STATUS_FAILURE;
    }
    tbuf[0] = VR_MPS_REG_WRITE_PROTECT;
    if (vr_xfer(bus, addr, tbuf, 1, rbuf, 1) < 0) {
      syslog(LOG_WARNING, "%s: read 0x%02X failed", __func__, tbuf[0]);
      return VR_STATUS_FAILURE;
    }

    if (rbuf[0] == MP2856_DISABLE_WRITE_PROTECT) {
      return VR_STATUS_SUCCESS;
    } else {
       //Unlock MTP Write protection
      tbuf[1] = MP2856_DISABLE_WRITE_PROTECT;
      if (vr_xfer(bus, addr, tbuf, 2, rbuf, 0) < 0) {
        syslog(LOG_WARNING, "%s: read 0x%02X failed", __func__, tbuf[0]);
        return VR_STATUS_FAILURE;
      }
    }
  }

  return VR_STATUS_SUCCESS;
}

static int
mp2856_write_data(uint8_t bus, uint8_t addr, struct mp2856_data *data  ) {
  uint8_t tbuf[16], rbuf[16];
  uint8_t txlen = 0;

  tbuf[0] = data->reg_addr,
  memcpy(&tbuf[1], &data->reg_data[0],  data->reg_len);
  txlen = data->reg_len+1;

#ifdef DEBUG
  for(int i=0; i<txlen; i++)
   printf("tx[%d]=%x ", i, tbuf[i]);
  printf(" txlen=%d\n", txlen);
#endif

  if (vr_xfer(bus, addr, tbuf, txlen, rbuf, 0)) {
    syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }
  return VR_STATUS_SUCCESS;
}

static int
mp2856_get_remaining_wr_msg(uint8_t addr, uint16_t *remain, char *msg, bool is_update) {
  if ((mps_remaining_wr == NULL) || (remain == NULL) || (msg == NULL)) {
    syslog(LOG_WARNING, "%s: fail to load remaining writes due to NULL pointer check \n", __func__);
    return VR_STATUS_FAILURE;
  }

  if (mps_remaining_wr(addr, remain, GET_VR_CRC) < 0) {
    snprintf(msg, MAX_VALUE_LEN, ", Remaining Writes: Unknown");
  } else {
    if (*remain == UNINITIALIZED_REMAIN_WR) {
      *remain = MAX_MP2856_REMAIN_WR;
      mps_remaining_wr(addr, remain, UPDATE_VR_CRC);
    }
    if (is_update && *remain) {
      (*remain)--;
      mps_remaining_wr(addr, remain, UPDATE_VR_CRC);
    }
    snprintf(msg, MAX_VALUE_LEN, ", Remaining Writes: %u", *remain);
  }

  return VR_STATUS_SUCCESS;
}

int
program_mp2856(uint8_t bus, uint8_t addr, struct mp2856_config *config  ) {
 uint8_t tbuf[16], rbuf[16];
 uint8_t page = 0;
 int i, ret =0;
 int page2_start = 0;
 struct mp2856_data *data;

  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_0) < 0) {
    return VR_STATUS_FAILURE;
  }

  //Program Page0 and Page1 registers
  for (i = 0; i < config->wr_cnt; i++) {
    data = &config->pdata[i];
    if (data->page == 2 ) {
      page2_start = i;
      break;
    }
    if( page != data->page) {
      if(mp2856_set_page(bus, addr, data->page) < 0) {
        return VR_STATUS_FAILURE;
      }
      page = data->page;
    }
    mp2856_write_data(bus, addr, data);
    printf("\rupdated: %d %%  ", ((i+1)*100)/config->wr_cnt);
    fflush(stdout);
  }

  //Store Page0/1 reggisters to MTP
  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_0) < 0) {
    return VR_STATUS_FAILURE;
   }
  tbuf[0] = VR_MPS_CMD_STORE_NORMAL_CODE;
  if ((ret = vr_xfer(bus, addr, tbuf, 1, rbuf, 0))) {
    syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }
  //Wait CMD STORE_NORMAL_CODE finish
  msleep(500);


  if (mp2856_enable_mtp_page_rw(bus, addr) < 0){
    printf("ERROR: Enable MTP PAGE RW FAILED!\n");
    return VR_STATUS_FAILURE;
  }
  //Enable STORE_MULTI_CODE
  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_2) < 0) {
    return VR_STATUS_FAILURE;
  }
  tbuf[0] = VR_MPS_CMD_STORE_MULTI_CODE;
  if ((ret = vr_xfer(bus, addr, tbuf, 1, rbuf, 0))) {
    syslog(LOG_WARNING, "%s: write register 0x%02X failed", __func__, tbuf[0]);
    return VR_STATUS_FAILURE;
  }

  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_2A) < 0) {
    return VR_STATUS_FAILURE;
  }
  msleep(2);
  //Program Page2 registers
  for (i = page2_start; i < config->wr_cnt; i++) {
    data = &config->pdata[i];
    if (data->page != 2 ){
      break;
    }
    mp2856_write_data(bus, addr, data);
    msleep(2);
    printf("\rupdated: %d %%  ", ((i+1)*100)/config->wr_cnt);
    fflush(stdout);
  }
  printf("\n");
  if(mp2856_set_page(bus, addr, VR_MPS_PAGE_1) < 0) {
    return VR_STATUS_FAILURE;
  }

  return VR_STATUS_SUCCESS;
}

static void
cache_mp2856_new_crc(struct vr_info *info, void *args) {
  struct mp2856_config *config = (struct mp2856_config *)args;
  int ret = VR_STATUS_FAILURE;
  uint8_t tbuf[16];
  uint8_t crc_user[2];
  uint8_t multi_config[2];
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  uint8_t bus = info->bus;
  uint8_t addr = info->addr;
  uint16_t remain = 0;
  char remaining_wr_msg[64];

  vr_get_fw_avtive_key(info, ver_key);
  do {
    if ((ret = mp2856_set_page(bus, addr, VR_MPS_PAGE_1))) {
      break;
    }

    tbuf[0] = (config->mode == MP297X) ?
          VR_MPS_REG_CRC_NORMAL_CODE_297X:VR_MPS_REG_CRC_NORMAL_CODE_2856;
    if ((ret = vr_xfer(bus, addr, tbuf, 1, crc_user, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    tbuf[0] = (config->mode == MP297X) ?
          VR_MPS_REG_CRC_MULTI_CONFIG_297X:VR_MPS_REG_CRC_MULTI_CONFIG_2856;
    if ((ret = vr_xfer(bus, addr, tbuf, 1, multi_config, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    snprintf(value, MAX_VALUE_LEN, "MPS %02x%02x%02x%02x",
             crc_user[1], crc_user[0], multi_config[1], multi_config[0]);

    if(mps_remaining_wr) {
      if(mp2856_get_remaining_wr_msg(addr, &remain, remaining_wr_msg, UPDATE_VR_CRC) == VR_STATUS_SUCCESS) {
        strncat(value, remaining_wr_msg, MAX_VALUE_LEN-1);
      }
    }

    kv_set(ver_key, value, 0, KV_FPERSIST);
  } while (0);

}

/* WARNING:
 * From programing guide:Do not issue any PMBus read command during program
 * Page2 Registers, thus platform should stop VR sensor polling before updating! */
int
mp2856_fw_update(struct vr_info *info, void *args) {
  struct mp2856_config *config = (struct mp2856_config *)args;
  uint16_t product_id = 0;
  uint16_t remain = 0;

  if (info == NULL || config == NULL) {
    return VR_STATUS_FAILURE;
  }

  printf("Update VR: %s\n", info->dev_name);

  if (info->xfer) {
    vr_xfer = info->xfer;
  }
  
  if (info->remaining_wr_op) {
    mps_remaining_wr = info->remaining_wr_op;
  }

  //Check i2c address
  if (info->addr != (config->addr<<1)) {
    syslog(LOG_WARNING, "%s: I2C ADDR 0x%X mismatch, expect 0x%X",
           __func__, info->addr, (config->addr<<1));
    return VR_STATUS_FAILURE;
  }
  //Check Product ID
  mp2856_get_product_id(info->bus, info->addr, &product_id);
  if (product_id != config->product_id_exp) {
    syslog(LOG_WARNING, "%s: IC_DEVICE_ID 0x%X mismatch, expect 0x%X",
           __func__, product_id, config->product_id_exp);
    return VR_STATUS_FAILURE;
  }
  
  //Check remaining writes
  if(mps_remaining_wr) {
    if (mps_remaining_wr(info->addr, &remain, GET_VR_CRC) == VR_STATUS_SUCCESS) {
      printf("Remaining writes: %u\n", remain);
      if (!info->force && (remain <= VR_WARN_REMAIN_WR)) {
        printf("WARNING: the remaining writes is below the threshold value %d!\n",
           VR_WARN_REMAIN_WR);
        printf("Please use \"--force\" option to try again.\n");
        syslog(LOG_WARNING, "%s: insufficient remaining writes %u", __func__, remain);
        return VR_STATUS_FAILURE;
      }
    }
  }

  if(mp2856_is_pwd_unlock(info->bus, info->addr)) {
    syslog(LOG_WARNING, "%s: PWD UNLOCK failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if(mp2856_unlock_write_protect_mode(info->bus, info->addr)) {
    syslog(LOG_WARNING, "%s: Unlock MTP Write protection failed", __func__);
    return VR_STATUS_FAILURE;
  }

  if(program_mp2856(info->bus, info->addr, config)) {
    return VR_STATUS_FAILURE;
  }

  if (pal_is_support_vr_delay_activate() && info->private_data) {
    cache_mp2856_new_crc(info, args);
  }

  mp2856_set_page(info->bus, info->addr, VR_MPS_PAGE_0);

  return VR_STATUS_SUCCESS;
}

void* mp2856_parse_file(struct vr_info *info, const char *path) {
  char line[120], *str;
  char buf[10]={0};
  FILE *fp = NULL;
  int dcnt = 0;
  int col = 0;
  uint8_t len;
  uint8_t ate_cnt;
  struct mp2856_config *config = NULL;

  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path!\n");
    return NULL;
  }

  config = (struct mp2856_config *)calloc(1, sizeof(struct mp2856_config));
  if (config == NULL) {
    printf("ERROR: no space for creating config!\n");
    fclose(fp);
    return NULL;
  }

  if(fgets(line, sizeof(line), fp)) {
    str = strtok(line, "\t");
    while( str != NULL ) {
      col++;
      str = strtok(NULL, "\t");
    }
  }

  if (col >= ATE_COL_MAX) {
    config->mode = MP285X;
    ate_cnt = ATE_COL_MAX;
  } else {
    config->mode = MP297X;
    ate_cnt = ATE_COL_MAX - 1;
  }
  fseek(fp, 0, SEEK_SET);

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (!strncmp(line, "END", 3)) {
      break;
    }

    //configure ID , page number, register address(hex) ,register address(dec),register name ,register data(hex),register data(dec),Write Type
    for(col = 0 ; col < ate_cnt ; col++) {
      if (col == ATE_CONF_ID)
        str = strtok(line, "\t");//Col 1
      else
        str = strtok(NULL, "\t");//Col 2~N
      switch (col) {
        case ATE_CONF_ID:
          config->cfg_id = strtol(str, NULL, 16);
          break;
        case ATE_PAGE_NUM:
          config->pdata[dcnt].page = strtol(str, NULL, 16);
          break;
        case ATE_REG_ADDR_HEX:
          config->pdata[dcnt].reg_addr = strtol(str, NULL, 16);
          break;
        case ATE_REG_DATA_HEX:
          len = strlen(str)/2;
          for(int i=0; i<len; i++) {
            memcpy(buf, &(str[i*2]), 2);
            config->pdata[dcnt].reg_data[len-1-i] = (uint8_t)strtol(buf, NULL, 16);
          }
          config->pdata[dcnt].reg_len = len;
          break;
        case ATE_WRITE_TYPE:
          if (!strncmp(str, "B2", 2)) {
            memmove(&config->pdata[dcnt].reg_data[1], &config->pdata[dcnt].reg_data[0], config->pdata[dcnt].reg_len);
            config->pdata[dcnt].reg_data[0] = config->pdata[dcnt].reg_len;
            config->pdata[dcnt].reg_len += 1;
          }
          break;
      }
    }
#ifdef DEBUG
    printf("dcnt=%d ,", dcnt);
    for(int i=0; i< config->pdata[dcnt].reg_len; i++)
      printf("data[%d]=%x ", i, config->pdata[dcnt].reg_data[i]);

    printf("Page=%d, Reg=0x%x\n", config->pdata[dcnt].page, config->pdata[dcnt].reg_addr);
#endif
    dcnt++;
  }
  config->wr_cnt = dcnt;

  if(config->mode == MP285X) {
    while (fgets(line, sizeof(line), fp) != NULL) {
      if (!strncmp(line, "Product ID", 10)) {
        sscanf(line, "%*[^:]:%9s", buf);
        str=strtok(buf, "MP");
        config->product_id_exp = strtol(str, NULL, 16);
      } else if  (!strncmp(line, "I2C", 3)) {
        sscanf(line, "%*[^:]:%9s", buf);
        config->addr = strtol(buf, NULL, 16);
      } else if  (!strncmp(line, "4-digi", 6)) {
        sscanf(line, "%*[^:]:%9s", buf);
        config->cfg_id =  strtol(buf, NULL, 16);
      }
    }
  } else {
    while (fgets(line, sizeof(line), fp) != NULL) {
      str=strtok(line, ": ");
      while( str != NULL ) {
        if(!strncmp(str, "MP29", 2)) {
          memcpy(buf, &str[4], 2);
          config->product_id_exp = strtol(buf, NULL, 16);
          break;
        }
        str=strtok(NULL, ": ");
      }
    }
    config->addr = info->addr >> 1;
  }
#ifdef DEBUG
  printf("id=%x\n", config->product_id_exp);
#endif
  fclose(fp);

  return config;
}

static int
cache_mp2856_crc(uint8_t bus, uint8_t addr, char *key, char *checksum) {
  int ret = VR_STATUS_FAILURE;
  uint8_t tbuf[16];
  uint8_t crc_user[2];
  uint8_t multi_config[2];
  uint16_t remain = 0;
  char remaining_wr_msg[64];

  do {
    if ((ret = mp2856_set_page(bus, addr, VR_MPS_PAGE_29))) {
      break;
    }
    tbuf[0] = VR_MPS_REG_CRC_USER;
    if ((ret = vr_xfer(bus, addr, tbuf, 1, crc_user, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    if ((ret = mp2856_set_page(bus, addr, VR_MPS_PAGE_2A))) {
      break;
    }

    tbuf[0] = VR_MPS_REG_MULTI_CONFIG;
    if ((ret = vr_xfer(bus, addr, tbuf, 1, multi_config, 2))) {
      syslog(LOG_WARNING, "%s: read register 0x%02X failed", __func__, tbuf[0]);
      break;
    }

    snprintf(checksum, MAX_VALUE_LEN, "MPS %02X%02X%02X%02X",
           crc_user[1], crc_user[0], multi_config[1], multi_config[0]);
    
    if (mps_remaining_wr) {
      if(mp2856_get_remaining_wr_msg(addr, &remain, remaining_wr_msg, GET_VR_CRC) == VR_STATUS_SUCCESS) {
        strncat(checksum, remaining_wr_msg, MAX_VALUE_LEN-1);
      }
    }

    kv_set(key, checksum, 0, 0);
  } while (0);

  mp2856_set_page(bus, addr, VR_MPS_PAGE_0);
  return ret;
}

int
get_mp2856_ver(struct vr_info *info, char *ver_str) {
  int ret;
  char key[MAX_KEY_LEN], tmp_str[MAX_VALUE_LEN] = {0};

  if (info->private_data) {
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", (char *)info->private_data, info->addr);
  } else {
    snprintf(key, sizeof(key), "vr_%02xh_crc", info->addr);
  }

  if (kv_get(key, tmp_str, NULL, 0)) {
    if (info->xfer) {
      vr_xfer = info->xfer;
    }

    if (info->remaining_wr_op) {
      mps_remaining_wr = info->remaining_wr_op;
    }
    
    //Stop sensor polling before read crc from register
    if (info->sensor_polling_ctrl) {
      info->sensor_polling_ctrl(false);
    }

    ret = cache_mp2856_crc(info->bus, info->addr, key, tmp_str);

    //Resume sensor polling
    if (info->sensor_polling_ctrl) {
      info->sensor_polling_ctrl(true);
    }

    if (ret) {
      return VR_STATUS_FAILURE;
    }
  }

  if (snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str) > (MAX_VER_STR_LEN-1))
    return VR_STATUS_FAILURE;

  return VR_STATUS_SUCCESS;
}
