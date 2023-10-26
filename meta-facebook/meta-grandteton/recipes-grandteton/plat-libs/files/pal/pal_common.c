#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <syslog.h>
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>
#include "pal_common.h"
#include "pal_def.h"
#include "pal_swb_sensors.h"
#include <openbmc/hal_fruid.h>
#include "pal.h"
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>


//#define DEBUG

char *mb_source_data[] = {
  "mb_hsc_source",
  "mb_vr_source",
  "mb_adc_source",
};

char *swb_source_data[] = {
  "swb_hsc_source",
  "swb_vr_source",
  "swb_nic_source",
};

char *vpdb_source_data[] = {
  "vpdb_hsc_source",
  "vpdb_brick_source",
  "vpdb_adc_source",
  "vpdb_rsense_source",
};

char *hpdb_source_data[] = {
  "hpdb_hsc_source",
  "hpdb_adc_source",
  "hpdb_rsense_source",
};

char *fan_bp1_source_data[] = {
  "fan_bp1_fan_chip_source",
  "fan_bp1_fan_led_source",
};

char *fan_bp2_source_data[] = {
  "fan_bp2_fan_chip_source",
  "fan_bp2_fan_led_source",
};

struct source_info {
  uint8_t fru;
  char** source;
};


struct source_info comp_source_data[] = {
  {FRU_ALL,   NULL},
  {FRU_MB,    mb_source_data},
  {FRU_SWB,   swb_source_data},
  {FRU_HGX,   NULL},
  {FRU_NIC0,  NULL},
  {FRU_NIC1,  NULL},
  {FRU_OCPDBG,   NULL},
  {FRU_BMC,   NULL},
  {FRU_SCM,   NULL},
  {FRU_VPDB,  vpdb_source_data},
  {FRU_HPDB,  hpdb_source_data},
  {FRU_FAN_BP1,   fan_bp1_source_data},
  {FRU_FAN_BP2,   fan_bp2_source_data},
  {FRU_FIO,   NULL},
  {FRU_HSC,   NULL},
  {FRU_SHSC,  NULL},

};


bool
is_cpu_socket_occupy(uint8_t cpu_idx) {
  static bool cached = false;
  static unsigned int cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      FM_CPU0_SKTOCC,
      FM_CPU1_SKTOCC,
    };
    if (gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return false;
    }
    cached = true;
  }

  // bit == 1 implies CPU is absent.
  if (cached_id & (1 << cpu_idx)) {
    return false;
  }
  return true;
}

bool
pal_skip_access_me(void) {
  if (!access("/tmp/fin_bios_upd", F_OK)) {
    return true;
  }

  return false;
}

int
read_device(const char *device, int *value) {
  FILE *fp = NULL;

  if (device == NULL || value == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  fp = fopen(device, "r");
  if (fp == NULL) {
    syslog(LOG_INFO, "%s failed to open device %s error: %s", __func__, device, strerror(errno));
    return -1;
  }

  if (fscanf(fp, "%d", value) != 1) {
    syslog(LOG_INFO, "%s failed to read device %s error: %s", __func__, device, strerror(errno));
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return 0;
}

bool
gta_check_exmax_prsnt(uint8_t cable_id) {
  uint8_t temp_reg_val[6] = {0};
  int i2cfd = 0, ret = -1;
  uint8_t tlen, rlen;
  uint8_t tbuf[MAX_I2C_TXBUF_SIZE] = {0};
  uint8_t rbuf[MAX_I2C_RXBUF_SIZE] = {0};
  gpio_value_t gpio_prsnt_A;
  enum GTA_EXMAX_CABLE_PRESENT_OFFSET {
    EXMAX_CABLE_PRESENT_OFFSET_1 = 0x00,
    EXMAX_CABLE_PRESENT_OFFSET_2 = 0x04,
    EXMAX_CABLE_PRESENT_OFFSET_3 = 0x05,
    EXMAX_CABLE_PRESENT_OFFSET_4 = 0x06,
    EXMAX_CABLE_PRESENT_OFFSET_5 = 0x07,
    EXMAX_CABLE_PRESENT_OFFSET_6 = 0x50,
  };
  enum GTA_EXMAX_CABLE_PRESENT_MASK {
    EXMAX_CABLE_PRESENT_MASK_A_1 = 0x01,
    EXMAX_CABLE_PRESENT_MASK_A_2 = 0x01,
    EXMAX_CABLE_PRESENT_MASK_B   = 0x02,
    EXMAX_CABLE_PRESENT_MASK_C_1 = 0x04,
    EXMAX_CABLE_PRESENT_MASK_C_2 = 0x40,
    EXMAX_CABLE_PRESENT_MASK_D_1 = 0x01,
    EXMAX_CABLE_PRESENT_MASK_D_2 = 0x80,
    EXMAX_CABLE_PRESENT_MASK_E   = 0x02,
    EXMAX_CABLE_PRESENT_MASK_F   = 0x10,
  };

  uint8_t cbl_prsnt_offset[] = {EXMAX_CABLE_PRESENT_OFFSET_1, EXMAX_CABLE_PRESENT_OFFSET_2, EXMAX_CABLE_PRESENT_OFFSET_3, \
                                EXMAX_CABLE_PRESENT_OFFSET_4, EXMAX_CABLE_PRESENT_OFFSET_5, EXMAX_CABLE_PRESENT_OFFSET_6};

  i2cfd = i2c_cdev_slave_open(I2C_BUS_7, GTA_MB_CPLD_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_ERR, "%s(): fail to open device: I2C BUS: %d", __func__, I2C_BUS_7);
    return false;
  }

  for (int i = 0; i < ARRAY_SIZE(cbl_prsnt_offset); i ++) {
    tbuf[0] = cbl_prsnt_offset[i];
    tlen = 1;
    rlen = 1;
    ret = i2c_rdwr_msg_transfer(i2cfd, GTA_MB_CPLD_ADDR, tbuf, tlen, rbuf, rlen);
    if (ret < 0) {
      syslog(LOG_INFO, "%s() I2C transfer to MB CPLD: 0x%02x failed, RET: %d", __func__,cbl_prsnt_offset[i] ,ret);
      i2c_cdev_slave_close(i2cfd);
      return false;
    }
    temp_reg_val[i] = rbuf[0];
  }

  switch(cable_id) {
    case GTA_EXMAX_CABLE_A:
      // Check MB CPLD and IO-EXP
      gpio_prsnt_A = gpio_get_value_by_shadow(CABLE_PRSNT_A);
      if ((temp_reg_val[1] & EXMAX_CABLE_PRESENT_MASK_A_1) == EXMAX_CABLE_PRESENT_MASK_A_1 &&
          (temp_reg_val[5] & EXMAX_CABLE_PRESENT_MASK_A_2) == EXMAX_CABLE_PRESENT_MASK_A_2 &&
          (gpio_prsnt_A)) {
            syslog(LOG_CRIT, "Cable A is not Present");
      } else {
        syslog(LOG_CRIT, "Cable A is Present");
      }
      break;
    case GTA_EXMAX_CABLE_B:
      if ((temp_reg_val[1] & EXMAX_CABLE_PRESENT_MASK_B) == EXMAX_CABLE_PRESENT_MASK_B) {
        syslog(LOG_CRIT, "Cable B is not Present");
      } else {
        syslog(LOG_CRIT, "Cable B is Present");
      }
      break;
    case GTA_EXMAX_CABLE_C:
      // Check MB CPLD
      if ((temp_reg_val[1] & EXMAX_CABLE_PRESENT_MASK_C_1) == EXMAX_CABLE_PRESENT_MASK_C_1 &&
          (temp_reg_val[3] & EXMAX_CABLE_PRESENT_MASK_C_2) == EXMAX_CABLE_PRESENT_MASK_C_2) {
        syslog(LOG_CRIT, "Cable C is not Present");
      } else {
        syslog(LOG_CRIT, "Cable C is Present");
      }
      break;
    case GTA_EXMAX_CABLE_D:
      // Check MB CPLD
      if ((temp_reg_val[0] & EXMAX_CABLE_PRESENT_MASK_D_2) == EXMAX_CABLE_PRESENT_MASK_D_2 &&
          (temp_reg_val[4] & EXMAX_CABLE_PRESENT_MASK_D_1) == EXMAX_CABLE_PRESENT_MASK_D_1) {
        syslog(LOG_CRIT, "Cable D is not Present");
      } else {
        syslog(LOG_CRIT, "Cable D is Present");
      }
      break;
    case GTA_EXMAX_CABLE_E:
      // Check MB CPLD
      if ((temp_reg_val[5] & EXMAX_CABLE_PRESENT_MASK_E) == EXMAX_CABLE_PRESENT_MASK_E) {
        syslog(LOG_CRIT, "Cable E is not Present");
      } else {
        syslog(LOG_CRIT, "Cable E is Present");
      }
      break;
    case GTA_EXMAX_CABLE_F:
      // Check MB CPLD
      if ((temp_reg_val[2] & EXMAX_CABLE_PRESENT_MASK_F) == EXMAX_CABLE_PRESENT_MASK_F) {
        syslog(LOG_CRIT, "Cable F is not Present");
      } else {
        syslog(LOG_CRIT, "Cable F is Present");
      }
      break;
    default:
      i2c_cdev_slave_close(i2cfd);
      return false;
  }
  i2c_cdev_slave_close(i2cfd);
  return true;
}

bool
gta_expansion_board_present(uint8_t fru_id, uint8_t *status) {
  gpio_value_t gpio_prsnt_1;
  gpio_value_t gpio_prsnt_2;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int i2cfd = 0, ret = -1;
  uint8_t tlen, rlen;
  uint8_t tbuf[MAX_I2C_TXBUF_SIZE] = {0};
  uint8_t rbuf[MAX_I2C_RXBUF_SIZE] = {0};
  enum GTA_MEB_PRSNT_OFFSET {
    MEB_PRSNT_OFFSET = 0x05,
  };
  enum GTA_MEB_PRSNT_MASK {
    MEB_PRSNT_MASK_1 = 0x10,
    MEB_PRSNT_MASK_2 = 0x40,
  };

  snprintf(key, sizeof(key), "fru%d_prsnt", fru_id);

  switch (fru_id) {
    case FRU_ACB:
      gpio_prsnt_1 = gpio_get_value_by_shadow(CABLE_PRSNT_A);
      gpio_prsnt_2 = gpio_get_value_by_shadow(CABLE_PRSNT_B);
      if (gpio_prsnt_1 && gpio_prsnt_2) {
        *status = FRU_NOT_PRSNT;
      } else {
        *status = FRU_PRSNT;
      }
      break;
    case FRU_MEB:
      i2cfd = i2c_cdev_slave_open(I2C_BUS_7, GTA_MB_CPLD_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
      if (i2cfd < 0) {
        syslog(LOG_ERR, "%s(): fail to open device: I2C BUS: %d", __func__, I2C_BUS_7);
        return false;
      }
      tbuf[0] = MEB_PRSNT_OFFSET;
      tlen = 1;
      rlen = 2;
      ret = i2c_rdwr_msg_transfer(i2cfd, GTA_MB_CPLD_ADDR, tbuf, tlen, rbuf, rlen);
      if (ret < 0) {
        syslog(LOG_INFO, "%s() I2C transfer to MB CPLD failed, RET: %d", __func__, ret);
        i2c_cdev_slave_close(i2cfd);
        return false;
      } else {
        if ((rbuf[0] & MEB_PRSNT_MASK_1) == MEB_PRSNT_MASK_1 &&
            (rbuf[1] & MEB_PRSNT_MASK_2) == MEB_PRSNT_MASK_2) {
          *status = FRU_NOT_PRSNT;
        } else {
          *status = FRU_PRSNT;
        }
      }
      i2c_cdev_slave_close(i2cfd);
      break;
    default:
      syslog(LOG_WARNING, "%s() FRU: %u Not Support", __func__, fru_id);
      return false;
  }
  snprintf(value, sizeof(value), "%d", *status);
  kv_set(key, value, 0, 0);
  return true;
}

bool
fru_presence(uint8_t fru_id, uint8_t *status) {
  gpio_value_t gpio_value;
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};

  switch (fru_id) {
    case FRU_FIO:
    case FRU_SHSC:
    case FRU_SWB:
      gpio_value = gpio_get_value_by_shadow(BIC_READY);
      if(gpio_value != GPIO_VALUE_INVALID) {
        *status = gpio_value ? FRU_NOT_PRSNT: FRU_PRSNT;
        return true;
      }
      return false;
    case FRU_HGX:
      gpio_value = gpio_get_value_by_shadow(HMC_PRESENCE);
      if(gpio_value != GPIO_VALUE_INVALID) {
        *status = gpio_value ? FRU_NOT_PRSNT: FRU_PRSNT;
        return true;
      }
      return false;
    case FRU_ACB:
    case FRU_MEB:
      if (!pal_is_artemis()) {
        *status = FRU_NOT_PRSNT;
        return true;
      }
      snprintf(key, sizeof(key), "fru%d_prsnt", fru_id);
      if (kv_get(key, value, NULL, 0) == 0) {
        *status = atoi(value);
        return true;
      }
      return gta_expansion_board_present(fru_id, status);
    default:
      return fru_presence_ext(fru_id, status);
  }
}

bool
check_pwron_time(int time) {
  char str[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  long pwron_time;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (!kv_get("snr_pwron_flag", str, NULL, 0)) {
    pwron_time = strtoul(str, NULL, 10);
   // syslog(LOG_WARNING, "power on time =%ld\n", pwron_time);
    if ( (ts.tv_sec - pwron_time > time ) && ( pwron_time != 0 ) ) {
      return true;
    }
  } else {
     sprintf(str, "%lld", ts.tv_sec);
     kv_set("snr_pwron_flag", str, 0, 0);
  }

  return false;
}

bool
pal_bios_completed(uint8_t fru)
{
  gpio_value_t value;

  if ( FRU_MB != fru )
  {
    syslog(LOG_WARNING, "[%s]incorrect fru id: %d", __func__, fru);
    return false;
  }

//BIOS COMPLT need time to inital when platform reset.
  if( !check_pwron_time(10) ) {
    return false;
  }

  value = gpio_get_value_by_shadow(FM_BIOS_POST_CMPLT);
  if(value != GPIO_VALUE_INVALID)
    return value ? false : true;

  return false;
}

void
get_dimm_present_info(uint8_t fru, bool *dimm_sts_list) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int i;
  size_t ret;

  //check dimm info from /mnt/data/sys_config/
  for (i=0; i<MAX_DIMM_NUM; i++) {
    sprintf(key, "sys_config/fru%d_dimm%d_location", fru, i);
    if(kv_get(key, value, &ret, KV_FPERSIST) != 0 || ret < 4) {
      syslog(LOG_WARNING,"[%s]Cannot get dimm_slot%d present info", __func__, i);
      value[0] = 0xff;
    }

    if ( 0xff == value[0] ) {
      dimm_sts_list[i] = false;
    } else {
      dimm_sts_list[i] = true;
    }
  }
}

bool
is_dimm_present(uint8_t dimm_id) {
  static bool is_check = false;
  static bool dimm_sts_list[MAX_DIMM_NUM] = {0};
  uint8_t fru = FRU_MB;

  if(!pal_bios_completed(fru)) {
    if(is_check == true) {
      for(int id=0; id<MAX_DIMM_NUM; id++) {
        dimm_sts_list[id] = 0;
      }
      is_check = false;
    }
    return false;
  }

  if ( is_check == false ) {
    is_check = true;
    get_dimm_present_info(fru, dimm_sts_list);
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "dimm id=%d, presnet=%d\n", dimm_id, dimm_sts_list[dimm_id]);
#endif
  if( dimm_sts_list[dimm_id] == true) {
    return true;
  }
  return false;
}

int
get_comp_source(uint8_t fru, uint8_t comp_id, uint8_t* source) {
  char value[MAX_VALUE_LEN] = {0};

  if(kv_get(comp_source_data[fru].source[comp_id], value, 0, 0)) {
    syslog(LOG_WARNING,"[%s] get source fail fru=%d comp=%d", __func__, fru, comp_id);
    return -1;
  }

  *source = (uint8_t)atoi(value);
  return 0;
}

int
get_acb_vr_source(uint8_t* source) {
  char *find = NULL;
  char value[MAX_VALUE_LEN] = {0};
  if (kv_get("cb_vr0_active_ver", value, 0, 0)) {
    syslog(LOG_WARNING,"[%s] get acb vr active version fail", __func__);
    return -1;
  }

  find = strchr(value, ' ');
  if (find == NULL) {
    syslog(LOG_WARNING,"[%s] invalid acb vr version", __func__);
    return -1;
  }

  *find = '\0';
  if (strcmp(value, "Infineon") == 0) {
    *source = MAIN_SOURCE;
    return 0;
  } else if (strcmp(value, "MPS") == 0) {
    *source = SECOND_SOURCE;
    return 0;
  } else {
    syslog(LOG_WARNING,"[%s] unknown acb vr version", __func__);
    return -1;
  }
}

bool
is_mb_hsc_module(void) {
  static bool cached = false;
  static bool val = false;
  uint8_t id;

  if (!cached) {
    get_comp_source(FRU_MB, MB_HSC_SOURCE, &id); //Fail:Main Source
    if (id == SECOND_SOURCE || id == THIRD_SOURCE)
      val = true;
    cached = true;
  }
  return val;
}


bool
is_swb_hsc_module(void) {
  static bool cached = false;
  static bool val = false;
  uint8_t id;
  uint8_t status = 0;

  if(!cached) {
    if(fru_presence(FRU_SWB, &status) && status == FRU_PRSNT) {
      get_comp_source(FRU_SWB, SWB_HSC_SOURCE, &id);
      if (id == SECOND_SOURCE || id == THIRD_SOURCE)
        val = true;
    }
    cached = true;
  }
  return val;
}

bool
sgpio_valid_check(){
  int bit1 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID0");
  int bit2 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID1");
  int bit3 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID2");
  int bit4 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID3");
  if ( bit1 == 0 && bit2 == 1 && bit3 == 0 && bit4 == 1) {
    return true;
  } else {
    return false;
  }
}

int read_cpld_health(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;
  static unsigned int retry;

  ret = sgpio_valid_check();
  if (!ret) {
    if (retry_err_handle(retry, 5) == READING_NA) {
      *value = 1;
    } else {
      *value = 0;
    }
  } else {
    *value = 0;
    retry = 0;
  }
  return 0;
}

int mb_retimer_lock(void) {
  return single_instance_lock_blocked("mb_retimer");
}

void mb_retimer_unlock(int lock) {
  if (lock >= 0) {
    single_instance_unlock(lock);
  }
}

int
pal_get_board_rev_id(uint8_t fru, uint8_t *id) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  char name[32];

  if(pal_get_fru_name(fru, name))
    return -1;

  snprintf(key, sizeof(key), "%s_rev", name);

  if (kv_get(key, value, NULL, 0)) {
    return -1;
  }

  *id = (uint8_t)atoi(value);
  return 0;
}

int
pal_get_board_sku_id(uint8_t fru, uint8_t *id) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  char name[32];

  if(pal_get_fru_name(fru, name))
    return -1;

  snprintf(key, sizeof(key), "%s_sku", name);

  if (kv_get(key, value, NULL, 0)) {
    return -1;
  }

  *id = (uint8_t)atoi(value);
  return 0;
}
