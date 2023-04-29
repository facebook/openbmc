#include <stdio.h>
#include <syslog.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/libgpio.h>
#include "pal.h"
#include "pal_common.h"

//#define DEBUG
#define SENSOR_SKIP_MAX (1)

extern PAL_SENSOR_MAP mb_sensor_map[];
extern PAL_SENSOR_MAP hgx_sensor_map[];
extern PAL_SENSOR_MAP swb_sensor_map[];
extern PAL_SENSOR_MAP bb_sensor_map[];
extern PAL_SENSOR_MAP acb_sensor_map[];
extern PAL_SENSOR_MAP accl_sensor_map[];
extern PAL_SENSOR_MAP meb_sensor_map[];
extern PAL_SENSOR_MAP meb_clx_sensor_map[];
extern PAL_SENSOR_MAP meb_e1s_sensor_map[];

extern const uint8_t mb_sensor_list[];
extern const uint8_t mb_discrete_sensor_list[];

extern const uint8_t swb_sensor_list[];
extern const uint8_t swb_discrete_sensor_list[];

extern const uint8_t hgx_sensor_list[];

extern const uint8_t nic0_sensor_list[];
extern const uint8_t nic1_sensor_list[];

extern const uint8_t vpdb_sensor_list[];
extern const uint8_t vpdb_1brick_sensor_list[];
extern const uint8_t vpdb_3brick_sensor_list[];
extern const uint8_t vpdb_adc_sensor_list[];

extern const uint8_t hpdb_sensor_list[];
extern const uint8_t hpdb_adc_sensor_list[];

extern const uint8_t fan_bp1_sensor_list[];
extern const uint8_t fan_bp2_sensor_list[];

extern const uint8_t scm_sensor_list[];

extern const uint8_t hsc_sensor_list[];

extern const uint8_t shsc_sensor_list[];

extern const uint8_t acb_sensor_list[];
extern const uint8_t accl_sensor_list[];
extern const uint8_t meb_sensor_list[];
extern const uint8_t meb_cxl_sensor_list[];
extern const uint8_t meb_e1s_sensor_list[];

extern size_t mb_sensor_cnt;
extern size_t mb_discrete_sensor_cnt;

extern size_t swb_sensor_cnt;
extern size_t swb_discrete_sensor_cnt;

extern size_t hgx_sensor_cnt;

extern size_t nic0_sensor_cnt;
extern size_t nic1_sensor_cnt;

extern size_t vpdb_sensor_cnt;
extern size_t vpdb_1brick_sensor_cnt;
extern size_t vpdb_3brick_sensor_cnt;
extern size_t vpdb_adc_sensor_cnt;

extern size_t hpdb_sensor_cnt;
extern size_t hpdb_adc_sensor_cnt;

extern size_t fan_bp1_sensor_cnt;
extern size_t fan_bp2_sensor_cnt;

extern size_t scm_sensor_cnt;

extern size_t hsc_sensor_cnt;

extern size_t shsc_sensor_cnt;

extern size_t acb_sensor_cnt;
extern size_t accl_sensor_cnt;
extern size_t meb_sensor_cnt;
extern size_t meb_cxl_sensor_cnt;
extern size_t meb_e1s_sensor_cnt;

struct snr_map sensor_map[] = {
  { FRU_ALL,  NULL,           false},
  { FRU_MB,   mb_sensor_map,  true },
  { FRU_SWB,  swb_sensor_map, true },
  { FRU_HGX,  hgx_sensor_map, true },
  { FRU_NIC0, bb_sensor_map,  true },
  { FRU_NIC1, bb_sensor_map,  true },
  { FRU_OCPDBG,  NULL,           false },
  { FRU_BMC,  NULL,           false },
  { FRU_SCM,  bb_sensor_map,  true },
  { FRU_VPDB, bb_sensor_map,  true },
  { FRU_HPDB, bb_sensor_map,  true },
  { FRU_FAN_BP1,  bb_sensor_map,  true },
  { FRU_FAN_BP2,  bb_sensor_map,  true },
  { FRU_FIO,  NULL,           false },
  { FRU_HSC,  mb_sensor_map,  true },
  { FRU_SHSC, swb_sensor_map, true },
  { FRU_ACB,  acb_sensor_map, true },
  { FRU_MEB,  meb_sensor_map, true },
  { FRU_ACB_ACCL1,  accl_sensor_map, true },
  { FRU_ACB_ACCL2,  accl_sensor_map, true },
  { FRU_ACB_ACCL3,  accl_sensor_map, true },
  { FRU_ACB_ACCL4,  accl_sensor_map, true },
  { FRU_ACB_ACCL5,  accl_sensor_map, true },
  { FRU_ACB_ACCL6,  accl_sensor_map, true },
  { FRU_ACB_ACCL7,  accl_sensor_map, true },
  { FRU_ACB_ACCL8,  accl_sensor_map, true },
  { FRU_ACB_ACCL9,  accl_sensor_map, true },
  { FRU_ACB_ACCL10, accl_sensor_map, true },
  { FRU_ACB_ACCL11, accl_sensor_map, true },
  { FRU_ACB_ACCL12, accl_sensor_map, true },
  { FRU_MEB_JCN1,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN2,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN3,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN4,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN5,  meb_e1s_sensor_map, true },
  { FRU_MEB_JCN6,  meb_e1s_sensor_map, true },
  { FRU_MEB_JCN7,  meb_e1s_sensor_map, true },
  { FRU_MEB_JCN8,  meb_e1s_sensor_map, true },
  { FRU_MEB_JCN9,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN10,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN11,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN12,  meb_clx_sensor_map, true },
  { FRU_MEB_JCN13,  meb_e1s_sensor_map, true },
  { FRU_MEB_JCN14,  meb_e1s_sensor_map, true }
};

static int get_jcn_config_string(uint8_t status, char *type_str) {
  int ret = 0;

  switch (status) {
  case E1S_CARD:
  case E1S_0_CARD:
  case E1S_1_CARD:
  case E1S_0_1_CARD:
    snprintf(type_str, MAX_VALUE_LEN, "%s", JCN_CONFIG_STR_E1S);
    break;
  case CXL_CARD:
    snprintf(type_str, MAX_VALUE_LEN, "%s", JCN_CONFIG_STR_CXL);
    break;
  case NIC_CARD:
    snprintf(type_str, MAX_VALUE_LEN, "%s", JCN_CONFIG_STR_NIC);
    break;
  default:
    ret = -1;
    break;
  }

  return ret;
}

static int pal_get_pldm_jcn_config_from_bic(uint8_t fru, char *type_str) {
  fru_status status = {0};
  int ret = 0;

  ret = pal_get_pldm_fru_status(fru, JCN_0_1, &status);
  if (ret == 0 && status.fru_prsnt == FRU_PRSNT && status.fru_type != UNKNOWN_CARD ) {
    if(get_jcn_config_string(status.fru_type, type_str) != 0) {
      return -1;
    }
    return 0;
  }

  return -1;
}

uint8_t pal_get_meb_jcn_config(uint8_t fru) {
  char key[MAX_KEY_LEN] = {0};
  char type_str[MAX_VALUE_LEN] = {0};

  if (fru < FRU_MEB_JCN1 || fru > FRU_MEB_JCN14) {
    return UNKNOWN_CARD;
  } else if (fru >= FRU_MEB_JCN5 && fru <= FRU_MEB_JCN8) {
    return E1S_CARD;
  }

  memset(key, 0, MAX_KEY_LEN);
  memset(type_str, 0, MAX_VALUE_LEN);
  snprintf(key, sizeof(key), "mc_fru%d_config", fru);

  if (kv_get(key, type_str, NULL, 0) != 0) {
    // not cached before, get from BIC
    if (pal_get_pldm_jcn_config_from_bic(fru, type_str) != 0) {
      return UNKNOWN_CARD ;
    }
    kv_set(key, type_str, 0, 0);
  }

  // get from cache
  if (strcmp(type_str, JCN_CONFIG_STR_CXL) == 0) {
    return CXL_CARD;
  } else if (strcmp(type_str, JCN_CONFIG_STR_E1S) == 0) {
    return E1S_CARD;
  } else if (strcmp(type_str, JCN_CONFIG_STR_NIC) == 0) {
    return NIC_CARD;
  } else {
    return UNKNOWN_CARD ;
  }
}

static void reload_sensor_table(uint8_t fru) {
  uint8_t config = 0;

  if ((fru >= FRU_MEB_JCN1 && fru <= FRU_MEB_JCN4) ||
      (fru >= FRU_MEB_JCN9 && fru <= FRU_MEB_JCN12) ) {
    config = pal_get_meb_jcn_config(fru);

    switch (config) {
      case CXL_CARD:
        // using default sensor table
        break;
      case E1S_CARD:
        sensor_map[fru].map = meb_e1s_sensor_map;
        break;
      default:
#ifdef DEBUG
        syslog(LOG_ERR, "[%s] Fail to get meb fru%u, config = %u\n", __func__, fru, config);
#endif
        break;
    }
  } else if (fru == FRU_MEB_JCN13 || fru == FRU_MEB_JCN14) {
    config = pal_get_meb_jcn_config(fru);

    switch (config) {
      case E1S_CARD:
        // using default sensor table
        break;
      case NIC_CARD:
        // Todo: add nic sensor table
        // sensor_map[fru].map = meb_nic_sensor_map;
        break;
      default:
#ifdef DEBUG
        syslog(LOG_ERR, "[%s] Fail to get meb fru%u, config = %u\n", __func__, fru, config);
#endif
        break;
    }
  }

  return;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  int ret=0;
  uint8_t id;
  uint8_t rev;
  static uint8_t snr_mb_tmp[255]={0};
  static uint8_t snr_swb_tmp[255]={0};
  static uint8_t snr_vpdb_tmp[64]={0};
  static uint8_t snr_hpdb_tmp[64]={0};
  bool module = is_mb_hsc_module();
  bool smodule = is_swb_hsc_module();

  if (fru == FRU_MB) {
    if (!module) {
      memcpy(snr_mb_tmp, mb_sensor_list, mb_sensor_cnt);
      memcpy(&snr_mb_tmp[mb_sensor_cnt], hsc_sensor_list, hsc_sensor_cnt);
      *sensor_list = snr_mb_tmp;
      *cnt = mb_sensor_cnt + hsc_sensor_cnt;
    } else {
      *sensor_list = (uint8_t *) mb_sensor_list;
      *cnt = mb_sensor_cnt;
    }
  } else if (fru == FRU_NIC0) {
    *sensor_list = (uint8_t *) nic0_sensor_list;
    *cnt = nic0_sensor_cnt;
  } else if (fru == FRU_NIC1) {
    *sensor_list = (uint8_t *) nic1_sensor_list;
    *cnt = nic1_sensor_cnt;
  } else if (fru == FRU_HGX) {
    *sensor_list = (uint8_t *) hgx_sensor_list;
    *cnt = hgx_sensor_cnt;
  } else if (fru == FRU_VPDB) {
    get_comp_source(fru, VPDB_BRICK_SOURCE, &id);
    if (id == THIRD_SOURCE) {
      memcpy(snr_vpdb_tmp, vpdb_sensor_list, vpdb_sensor_cnt);
      memcpy(&snr_vpdb_tmp[vpdb_sensor_cnt], vpdb_1brick_sensor_list, vpdb_1brick_sensor_cnt);
      *cnt = vpdb_sensor_cnt + vpdb_1brick_sensor_cnt;
      // ADC sensor only support greater than PVT in discrete Sku
      if(!pal_get_board_rev_id(FRU_VPDB, &rev) && rev > VPDB_DISCRETE_REV_PVT) {
        memcpy(&snr_vpdb_tmp[*cnt], vpdb_adc_sensor_list, vpdb_adc_sensor_cnt);
        *cnt += vpdb_adc_sensor_cnt;
      }
    } else {
      memcpy(snr_vpdb_tmp, vpdb_sensor_list, vpdb_sensor_cnt);
      memcpy(&snr_vpdb_tmp[vpdb_sensor_cnt], vpdb_3brick_sensor_list, vpdb_3brick_sensor_cnt);
      *cnt = vpdb_sensor_cnt + vpdb_3brick_sensor_cnt;
      // ADC sensor only support greater than PVT in normal Sku
      if(!pal_get_board_rev_id(FRU_VPDB, &rev) && rev >= PDB_REV_PVT2 ) {
        memcpy(&snr_vpdb_tmp[*cnt], vpdb_adc_sensor_list, vpdb_adc_sensor_cnt);
        *cnt += vpdb_adc_sensor_cnt;
      }
    }
    *sensor_list = snr_vpdb_tmp;

  } else if (fru == FRU_HPDB) {
    memcpy(snr_hpdb_tmp, hpdb_sensor_list, hpdb_sensor_cnt);
    *cnt = hpdb_sensor_cnt;
    // ADC sensor only support greater than PVT
    if(!pal_get_board_rev_id(FRU_HPDB, &rev) && rev >= PDB_REV_PVT2) {
      memcpy(&snr_hpdb_tmp[*cnt], hpdb_adc_sensor_list, hpdb_adc_sensor_cnt);
      *cnt += hpdb_adc_sensor_cnt;
    }
    *sensor_list = snr_hpdb_tmp;

  } else if (fru == FRU_FAN_BP1) {
    *sensor_list = (uint8_t *) fan_bp1_sensor_list;
    *cnt = fan_bp1_sensor_cnt;
  } else if (fru == FRU_FAN_BP2) {
    *sensor_list = (uint8_t *) fan_bp2_sensor_list;
    *cnt = fan_bp2_sensor_cnt;
  } else if (fru == FRU_SCM) {
    *sensor_list = (uint8_t *) scm_sensor_list;
    *cnt = scm_sensor_cnt;
  } else if (fru == FRU_SWB) {
    if (!smodule) {
      memcpy(snr_swb_tmp, swb_sensor_list, swb_sensor_cnt);
      memcpy(&snr_swb_tmp[swb_sensor_cnt], shsc_sensor_list, shsc_sensor_cnt);
      *sensor_list = snr_swb_tmp;
      *cnt = swb_sensor_cnt + shsc_sensor_cnt;
    } else {
      *sensor_list = (uint8_t *) swb_sensor_list;
      *cnt = swb_sensor_cnt;
    }
  } else if (fru == FRU_HSC) {
    if (module) {
      *sensor_list = (uint8_t *) hsc_sensor_list;
      *cnt = hsc_sensor_cnt;
    }
  } else if (fru == FRU_SHSC) {
    if (smodule) {
      *sensor_list = (uint8_t *) shsc_sensor_list;
      *cnt = shsc_sensor_cnt;
    }
  } else if (fru == FRU_ACB) {
    if (pal_is_artemis()) {
      *sensor_list = (uint8_t *) acb_sensor_list;
      *cnt = acb_sensor_cnt;
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
  } else if (fru >= FRU_ACB_ACCL1 && fru <= FRU_ACB_ACCL12) {
    if (pal_is_artemis()) {
      *sensor_list = (uint8_t *) accl_sensor_list;
      *cnt = accl_sensor_cnt;
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
  } else if (fru == FRU_MEB) {
    // include JCN5~JCN8 with E1.S*1
    if (pal_is_artemis()) {
      *sensor_list = (uint8_t *) meb_sensor_list;
      *cnt = meb_sensor_cnt;
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
  } else if ((fru >= FRU_MEB_JCN1 && fru <= FRU_MEB_JCN4) ||
             (fru >= FRU_MEB_JCN9 && fru <= FRU_MEB_JCN12)) {
    // install with CXL*1 or E1.S*2
    if (pal_is_artemis()) {
      // default: cxl sensor table
      *sensor_list = (uint8_t *) meb_cxl_sensor_list;
      *cnt = meb_cxl_sensor_cnt;
      if (pal_get_meb_jcn_config(fru) == E1S_CARD) {
        *sensor_list = (uint8_t *) meb_e1s_sensor_list;
        *cnt = meb_e1s_sensor_cnt;
      }
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
  } else if (fru == FRU_MEB_JCN13 || fru == FRU_MEB_JCN14) {
    // install with NIC*1 or E1.S*2
    if (pal_is_artemis()) {
      // default: e1s sensor table
      *sensor_list = (uint8_t *) meb_e1s_sensor_list;
      *cnt = meb_e1s_sensor_cnt;
      if (pal_get_meb_jcn_config(fru) == NIC_CARD) {
        // Todo: add NIC sensor table
        //*sensor_list = (uint8_t *) meb_nic_sensor_list;
        //*cnt = meb_nic_sensor_cnt;
      }
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
  } else if (fru > MAX_NUM_FRUS) {
    return -1;
  } else {
    *sensor_list = NULL;
    *cnt = 0;
  }
  return ret;
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  if (fru == FRU_MB) {
    *sensor_list = (uint8_t *) mb_discrete_sensor_list;
    *cnt = mb_discrete_sensor_cnt;
  } else if (fru == FRU_SWB) {
    *sensor_list = (uint8_t *) swb_discrete_sensor_list;
    *cnt = swb_discrete_sensor_cnt;
  } else if (fru > MAX_NUM_FRUS) {
      return -1;
  } else {
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
  return 0;
}


int retry_err_handle(uint8_t retry_curr, uint8_t retry_max) {

  if( retry_curr <= retry_max) {
    return READING_SKIP;
  }
  return READING_NA;
}

int retry_skip_handle(uint8_t retry_curr, uint8_t retry_max) {

  if( retry_curr <= retry_max) {
    return READING_SKIP;
  }
  return 0;
}

int check_polling_status(uint8_t fru){
  char key[MAX_KEY_LEN] = {0};
  char val[MAX_VALUE_LEN] = {0};
  char name[32] = {0};

  pal_get_fru_name(fru, name);
  snprintf(key, sizeof(key), "%s_polling_status", name);

  if(kv_get(key, val, 0, 0)) {
    //syslog(LOG_CRIT, " %s not key  - FRU: %d", __func__, fru);
    return sensor_map[fru].polling;
  }

  if(atoi(val) == 1) {
    return true;
  } else {
    return false;
  }
}

static uint8_t*
get_map_retry(uint8_t fru)
{
  static uint8_t mb_retry[256] = {0};
  static uint8_t swb_retry[256] = {0};
  static uint8_t bb_retry[256] = {0};
  static uint8_t mb_hsc_retry[16] = {0};
  static uint8_t swb_hsc_retry[16] = {0};
  static uint8_t acb_retry[MAX_SENSOR_NUMBER + 1] = {0};
  static uint8_t accl_retry[FRU_ACB_ACCL_CNT][MAX_SENSOR_NUMBER + 1] = {0};
  static uint8_t meb_retry[MAX_SENSOR_NUMBER + 1] = {0};
  static uint8_t meb_jcn_retry[FRU_MEB_JCN_CNT][MAX_SENSOR_NUMBER + 1] = {0};
  static bool is_retry_table_init = false;

  if (is_retry_table_init == false) {
    memset(meb_jcn_retry, 0, sizeof(meb_jcn_retry));
    memset(accl_retry, 0, sizeof(accl_retry));
    is_retry_table_init = true;
  }

  if (fru == FRU_SWB)
    return swb_retry;
  else if (fru == FRU_MB)
    return mb_retry;
  else if (fru == FRU_HSC)
    return mb_hsc_retry;
  else if (fru == FRU_SHSC)
    return swb_hsc_retry;
  else if (fru == FRU_ACB)
    return acb_retry;
  else if (fru >= FRU_ACB_ACCL1 && fru <= FRU_ACB_ACCL12)
    return accl_retry[fru - FRU_ACB_ACCL1];
  else if (fru == FRU_MEB)
    return meb_retry;
  else if (fru >= FRU_MEB_JCN1 && fru <= FRU_MEB_JCN14)
    return meb_jcn_retry[fru - FRU_MEB_JCN1];
  else
    return bb_retry;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  bool server_off;
  uint8_t *retry = get_map_retry(fru);

  reload_sensor_table(fru);

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);

  server_off = is_server_off();
  if (check_polling_status(fru)) {
    if (server_off) {
      if (sensor_map[fru].map[sensor_num].stby_read == true) {
        ret = sensor_map[fru].map[sensor_num].read_sensor(fru, sensor_num, (float*) value);
      } else {
        ret = READING_NA;
      }
    } else {
      ret = sensor_map[fru].map[sensor_num].read_sensor(fru, sensor_num, (float*) value);
    }

    if ( ret == 0 ) {
      if( (sensor_map[fru].map[sensor_num].snr_thresh.ucr_thresh <= *(float*)value) &&
          (sensor_map[fru].map[sensor_num].snr_thresh.ucr_thresh != 0) ) {
        ret = retry_skip_handle(retry[sensor_num], SENSOR_SKIP_MAX);
        if ( ret == READING_SKIP ) {
          retry[sensor_num]++;
#ifdef DEBUG
          syslog(LOG_CRIT,"sensor retry=%d touch ucr thres=%f snrnum=0x%x value=%f\n",
                 retry[sensor_num],
                 sensor_map[fru].map[sensor_num].snr_thresh.ucr_thresh,
                 sensor_num,
                  *(float*)value );
#endif
        }

      } else if( (sensor_map[fru].map[sensor_num].snr_thresh.lcr_thresh >= *(float*)value) &&
                 (sensor_map[fru].map[sensor_num].snr_thresh.lcr_thresh != 0) ) {
        ret = retry_skip_handle(retry[sensor_num], SENSOR_SKIP_MAX);
        if ( ret == READING_SKIP ) {
          retry[sensor_num]++;
#ifdef DEBUG
          syslog(LOG_CRIT,"sensor retry=%d touch lcr thres=%f snrnum=0x%x value=%f\n",
                 retry[sensor_num],
                 sensor_map[fru].map[sensor_num].snr_thresh.lcr_thresh,
                 sensor_num,
                 *(float*)value );
#endif
        }
      } else {
        retry[sensor_num] = 0;
      }
    }
  } else {
    return -1;
  }

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    sprintf(str, "%.2f",*((float*)value));
  }
  if (kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  char fru_name[32] = {0};
  char units_name[8] = {0};
  uint8_t scale = 0;

  reload_sensor_table(fru);
  scale = sensor_map[fru].map[sensor_num].units;

  if (check_polling_status(fru)) {
    if (pal_get_fru_name(fru, fru_name) == 0) {
      for (int i = 0; i < strlen(fru_name); i++)
        fru_name[i] = toupper(fru_name[i]);
    }

    switch(scale) {
      case TEMP:
        sprintf(units_name, "_C");
        break;
      case FAN:
        sprintf(units_name, "_RPM");
        break;
      case VOLT:
        sprintf(units_name, "_V");
        break;
      case mVOLT:
        sprintf(units_name, "_mV");
        break;
      case CURR:
        sprintf(units_name, "_A");
        break;
      case POWER:
        sprintf(units_name, "_W");
        break;
      case ENRGY:
        sprintf(units_name, "_J");
        break;
      case PRESS:
        sprintf(units_name, "_P");
        break;
    }

    sprintf(name, "%s_%s%s", fru_name,
                             sensor_map[fru].map[sensor_num].snr_name,
                             units_name);
  } else {
    return -1;
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;

  reload_sensor_table(fru);

  if (check_polling_status(fru)) {
    switch(thresh) {
      case UCR_THRESH:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.ucr_thresh;
        break;
      case UNC_THRESH:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.unc_thresh;
        break;
      case UNR_THRESH:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.unr_thresh;
        break;
      case LCR_THRESH:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.lcr_thresh;
        break;
      case LNC_THRESH:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.lnc_thresh;
        break;
      case LNR_THRESH:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.lnr_thresh;
        break;
      case POS_HYST:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.pos_hyst;
        break;
      case NEG_HYST:
        *val = sensor_map[fru].map[sensor_num].snr_thresh.neg_hyst;
        break;
      default:
        return -1;
    }
  } else {
    syslog(LOG_WARNING, "Threshold type error value=%d\n", thresh);
    return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {

  uint8_t scale = 0;

  reload_sensor_table(fru);
  scale = sensor_map[fru].map[sensor_num].units;

  if (sensor_map[fru].polling) {
    switch(scale) {
      case TEMP:
        sprintf(units, "C");
        break;
      case FAN:
        sprintf(units, "RPM");
        break;
      case VOLT:
        sprintf(units, "Volts");
        break;
      case mVOLT:
        sprintf(units, "mVolts");
        break;
      case CURR:
        sprintf(units, "Amps");
        break;
      case POWER:
        sprintf(units, "Watts");
        break;
      case ENRGY:
        sprintf(units, "Joules");
        break;
      case PRESS:
        sprintf(units, "Pa");
        break;
      default:
        return -1;
    }
  } else {
    return -1;
  }
  return 0;
}

static int
gt_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  return -1;
}

int
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (gt_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }

  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

static void fan_state_led_ctrl(uint8_t fru, uint8_t snr_num, bool assert) {
  char blue_led[32] = {0};
  char amber_led[32] = {0};
  uint8_t fan_id = sensor_map[fru].map[snr_num].id/2;

  snprintf(blue_led, sizeof(blue_led), "FAN%d_LED_GOOD", (int)fan_id);
  snprintf(amber_led, sizeof(amber_led), "FAN%d_LED_FAIL", (int)fan_id);

  gpio_set_value_by_shadow(amber_led, assert? GPIO_VALUE_HIGH: GPIO_VALUE_LOW);
  gpio_set_value_by_shadow(blue_led, assert? GPIO_VALUE_LOW: GPIO_VALUE_HIGH);
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char sensor_name[32];
  char thresh_name[10];
  uint8_t fan_id;

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong thresh enum value", __func__);
      exit(-1);
  }

  if (sensor_map[fru].map[snr_num].units == VOLT) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fVolts - Assert", sensor_name, thresh_name, val);
  } else if (sensor_map[fru].map[snr_num].units == mVOLT) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fmVolts - Assert", sensor_name, thresh_name, val);
  } else if (sensor_map[fru].map[snr_num].units == FAN){
      fan_id = sensor_map[fru].map[snr_num].id;
      sprintf(cmd, "FAN%d %s %dRPM - Assert",fan_id ,thresh_name, (int)val);
      fan_state_led_ctrl(fru, snr_num, true);
  } else if (sensor_map[fru].map[snr_num].units == POWER) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %dW - Assert", sensor_name, thresh_name, (int)val);
  } else if (sensor_map[fru].map[snr_num].units == TEMP) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fC - Assert", sensor_name, thresh_name, val);
  }
  pal_add_cri_sel(cmd);

  switch (thresh) {
    case UNR_THRESH:
    case LNR_THRESH:
      if (fru == FRU_MB && snr_num >= MB_SNR_RETIMER0_TEMP &&
          snr_num <= MB_SNR_RETIMER7_TEMP) {
        pal_set_server_power(FRU_MB, SERVER_POWER_OFF);
      }
    break;
  }
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char sensor_name[32];
  char thresh_name[10];
  uint8_t fan_id;

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "%s: wrong thresh enum value", __func__);
      exit(-1);
  }


  if (sensor_map[fru].map[snr_num].units == VOLT) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fVolts - Deassert", sensor_name, thresh_name, val);
  } else if (sensor_map[fru].map[snr_num].units == mVOLT) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fmVolts - Deassert", sensor_name, thresh_name, val);
  } else if (sensor_map[fru].map[snr_num].units == FAN){
      fan_id = sensor_map[fru].map[snr_num].id;
      sprintf(cmd, "FAN%d %s %dRPM - Deassert",fan_id ,thresh_name, (int)val);
      fan_state_led_ctrl(fru, snr_num, true);
  } else if (sensor_map[fru].map[snr_num].units == POWER) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %dW - Deassert", sensor_name, thresh_name, (int)val);
  } else if (sensor_map[fru].map[snr_num].units == TEMP) {
      pal_get_sensor_name(fru, snr_num, sensor_name);
      sprintf(cmd, "%s %s %.2fC - Deassert", sensor_name, thresh_name, val);
  }
  pal_add_cri_sel(cmd);

}

int
pal_sensor_discrete_check(uint8_t fru,
                          uint8_t snr_num,
                          char *snr_name,
                          uint8_t o_val,
                          uint8_t n_val) {
  char name[64];

  pal_get_sensor_name(fru, snr_num, name);
  if (!GETBIT(n_val, 0)) {
    syslog(LOG_CRIT, "ASSERT: %s - FRU: %d", name, fru);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s - FRU: %d", name, fru);
  }
  return 0;
}
