#include <stdio.h>
#include <syslog.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/libgpio.h>
#include <libpldm-oem/pldm.h>
#include "pal.h"
#include "pal_common.h"

//#define DEBUG
#define SENSOR_SKIP_MAX (1)

extern PAL_SENSOR_MAP mb_sensor_map[];
extern PAL_SENSOR_MAP hgx_sensor_map[];
extern PAL_SENSOR_MAP swb_sensor_map[];
extern PAL_SENSOR_MAP bb_sensor_map[];
extern PAL_SENSOR_MAP acb_artemis_sensor_map[];
extern PAL_SENSOR_MAP acb_freya_sensor_map[];
extern PAL_SENSOR_MAP meb_sensor_map[];
extern PAL_SENSOR_MAP meb_clx_sensor_map[];
extern PAL_SENSOR_MAP meb_e1s_sensor_map[];

extern const uint8_t mb_sensor_list[];
extern const uint8_t mb_discrete_sensor_list[];

extern const uint8_t swb_sensor_list[];
extern const uint8_t swb_discrete_sensor_list[];
extern const uint8_t swb_optic_sensor_list[];

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

extern const uint8_t acb_artemis_sensor_list[];
extern const uint8_t acb_freya_sensor_list[];
extern const uint8_t meb_sensor_list[];
extern const uint8_t meb_cxl_sensor_list[];
extern const uint8_t meb_e1s_sensor_list[];

extern size_t mb_sensor_cnt;
extern size_t mb_discrete_sensor_cnt;

extern size_t swb_sensor_cnt;
extern size_t swb_discrete_sensor_cnt;
extern size_t swb_optic_sensor_cnt;

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

extern size_t acb_freya_sensor_cnt;
extern size_t acb_artemis_sensor_cnt;
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
  { FRU_ACB,  acb_artemis_sensor_map, true },
  { FRU_MEB,  meb_sensor_map, true },
  { FRU_ACB_ACCL1,  NULL, false },
  { FRU_ACB_ACCL2,  NULL, false },
  { FRU_ACB_ACCL3,  NULL, false },
  { FRU_ACB_ACCL4,  NULL, false },
  { FRU_ACB_ACCL5,  NULL, false },
  { FRU_ACB_ACCL6,  NULL, false },
  { FRU_ACB_ACCL7,  NULL, false },
  { FRU_ACB_ACCL8,  NULL, false },
  { FRU_ACB_ACCL9,  NULL, false },
  { FRU_ACB_ACCL10, NULL, false },
  { FRU_ACB_ACCL11, NULL, false },
  { FRU_ACB_ACCL12, NULL, false },
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

int
get_pldm_sensor(uint8_t bus, uint8_t eid, uint8_t sensor_num, float *value)
{
  uint8_t tbuf[255] = {0};
  uint8_t* rbuf = (uint8_t *) NULL;//, tmp;
  struct pldm_snr_reading_t* resp;
  uint8_t tlen = 0;
  size_t  rlen = 0;
  int16_t integer=0;
  float decimal=0;
  int rc;

  struct pldm_msg* pldmbuf = (struct pldm_msg *)tbuf;
  pldmbuf->hdr.request = 1;
  pldmbuf->hdr.type    = PLDM_PLATFORM;
  pldmbuf->hdr.command = PLDM_GET_SENSOR_READING;
  tlen = PLDM_HEADER_SIZE;
  tbuf[tlen++] = sensor_num;
  tbuf[tlen++] = 0;
  tbuf[tlen++] = 0;

  rc = oem_pldm_send_recv(bus, eid, tbuf, tlen, &rbuf, &rlen);

  if (rc == PLDM_SUCCESS) {
    resp= (struct pldm_snr_reading_t*)rbuf;
    if (resp->data.completion_code || resp->data.sensor_operational_state) {
      rc = -1;
      goto exit;
    }
    integer = resp->data.present_reading[0] | resp->data.present_reading[1] << 8;
    decimal = (float)(resp->data.present_reading[2] | resp->data.present_reading[3] << 8)/1000;

    if (integer >= 0)
      *value = (float)integer + decimal;
    else
      *value = (float)integer - decimal;
  }

exit:
  if (rbuf)
    free(rbuf);

  return rc;

}

static int pal_get_acb_card_config_from_bic() {
  fru_status status = {0};
  int ret = 0;
  int fru = 0;

  for (fru = FRU_ACB_ACCL1; fru <= FRU_ACB_ACCL12; fru++) {
    ret = pal_get_pldm_fru_status(fru, JCN_0_1, &status);
    if (ret == 0 && status.fru_prsnt == FRU_PRSNT && status.fru_type != UNKNOWN_CARD ) {
      return (int)(status.fru_type);
    }
  }

  return -1;
}

uint8_t pal_get_acb_card_config() {
  char type_str[MAX_VALUE_LEN] = {0};
  uint8_t card_type = 0;

  memset(type_str, 0, MAX_VALUE_LEN);

  if (kv_get(KEY_ACB_CARD_CONFIG, type_str, NULL, 0) != 0) {
    // not cached before, get from BIC
    if ((card_type = pal_get_acb_card_config_from_bic()) < 0) {
      return UNKNOWN_CARD ;
    }

    snprintf(type_str, sizeof(type_str), "%d", card_type);
    kv_set(KEY_ACB_CARD_CONFIG, type_str, 0, 0);
  }

  // get from cache
  return atoi(type_str);
}

static void reload_sensor_table(uint8_t fru) {
  uint8_t config = 0;

  if (fru == FRU_ACB) {
    config = pal_get_acb_card_config();

    switch (config) {
      case ARTEMIS_CARD:
        // using default sensor table
        break;
      case FREYA_CARD:
        sensor_map[fru].map = acb_freya_sensor_map;
        break;
      default:
#ifdef DEBUG
        syslog(LOG_ERR, "[%s] Fail to get acb fru%u, config = %u\n", __func__, fru, config);
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
  static uint8_t snr_swb_tmp[255]={0};
  static uint8_t snr_vpdb_tmp[64]={0};
  static uint8_t snr_hpdb_tmp[64]={0};

  if (fru == FRU_MB) {
      *sensor_list = (uint8_t *) mb_sensor_list;
      *cnt = mb_sensor_cnt;
  } else if (fru == FRU_NIC0) {
    uint8_t status = 0;
    if (pal_is_fru_prsnt(FRU_NIC0, &status) == 0 && (status == FRU_PRSNT)) {
      *sensor_list = (uint8_t *) nic0_sensor_list;
      *cnt = nic0_sensor_cnt;
    } else {
        *sensor_list = NULL;
        *cnt = 0;
    }
  } else if (fru == FRU_NIC1) {
    uint8_t status = 0;
    if (pal_is_fru_prsnt(FRU_NIC1, &status) == 0 && (status == FRU_PRSNT)) {
      *sensor_list = (uint8_t *) nic1_sensor_list;
      *cnt = nic1_sensor_cnt;
    } else {
      *sensor_list = NULL;
      *cnt = 0;
    }
  } else if (fru == FRU_HGX) {
    *sensor_list = (uint8_t *) hgx_sensor_list;
    *cnt = hgx_sensor_cnt;
  } else if (fru == FRU_VPDB) {
    memcpy(snr_vpdb_tmp, vpdb_sensor_list, vpdb_sensor_cnt);
    // brick
    if (!pal_get_board_rev_id(FRU_VPDB, &rev) && rev == VPDB_DISCRETE_REV_PVT &&
        !get_comp_source(FRU_VPDB, VPDB_BRICK_SOURCE, &id) && id == DISCRETE_SOURCE) {
      memcpy(&snr_vpdb_tmp[vpdb_sensor_cnt], vpdb_1brick_sensor_list, vpdb_1brick_sensor_cnt);
      *cnt = vpdb_sensor_cnt + vpdb_1brick_sensor_cnt;
    } else {
      memcpy(&snr_vpdb_tmp[vpdb_sensor_cnt], vpdb_3brick_sensor_list, vpdb_3brick_sensor_cnt);
      *cnt = vpdb_sensor_cnt + vpdb_3brick_sensor_cnt;
    }
    // adc
    if (!pal_get_board_rev_id(FRU_VPDB, &rev) && rev >= PDB_REV_PVT2 ) {
      memcpy(&snr_vpdb_tmp[*cnt], vpdb_adc_sensor_list, vpdb_adc_sensor_cnt);
      *cnt += vpdb_adc_sensor_cnt;
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
    memcpy(snr_swb_tmp, swb_sensor_list, swb_sensor_cnt);
    *cnt = swb_sensor_cnt;

    //Add swb optic sensor
    get_comp_source(fru, SWB_NIC_SOURCE, &id);
    if (id == SECOND_SOURCE) {
      memcpy(&snr_swb_tmp[*cnt], swb_optic_sensor_list, swb_optic_sensor_cnt);
      *cnt += swb_optic_sensor_cnt;
    }

    *sensor_list = (uint8_t *) snr_swb_tmp;
  } else if (fru == FRU_ACB) {
    if (pal_is_artemis()) {
      if (pal_get_acb_card_config() == FREYA_CARD) {
        *sensor_list = (uint8_t *) acb_freya_sensor_list;
        *cnt = acb_freya_sensor_cnt;
      } else {
        *sensor_list = (uint8_t *) acb_artemis_sensor_list;
        *cnt = acb_artemis_sensor_cnt;
      }
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
    if (pal_is_artemis()) {
      *sensor_list = (uint8_t *) meb_cxl_sensor_list;
      *cnt = meb_cxl_sensor_cnt;
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
  static uint8_t acb_retry[MAX_SENSOR_NUMBER + 1] = {0};
  static uint8_t meb_retry[MAX_SENSOR_NUMBER + 1] = {0};
  static uint8_t meb_jcn_retry[FRU_MEB_JCN_CNT][MAX_SENSOR_NUMBER + 1] = {0};
  static bool is_retry_table_init = false;

  if (is_retry_table_init == false) {
    memset(meb_jcn_retry, 0, sizeof(meb_jcn_retry));
    is_retry_table_init = true;
  }

  if (fru == FRU_SWB)
    return swb_retry;
  else if (fru == FRU_MB)
    return mb_retry;
  else if (fru == FRU_ACB)
    return acb_retry;
  else if (fru == FRU_MEB)
    return meb_retry;
  else if (fru >= FRU_MEB_JCN1 && fru <= FRU_MEB_JCN14)
    return meb_jcn_retry[fru - FRU_MEB_JCN1];
  else
    return bb_retry;
}

static bool
skip_power_on_reading(uint8_t fru, uint8_t skip_sec) {
  char last_power_good[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  // block sensor reading within skip_sec after host power on
  if (fru == FRU_ACB || fru == FRU_MEB || (fru >= FRU_MEB_JCN1 && fru <= FRU_MEB_JCN14)) {
    if(kv_get(KV_KEY_LAST_POWER_GOOD, last_power_good, NULL, 0) == 0) {
      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (ts.tv_sec < (strtoul(last_power_good, NULL, 10) + skip_sec)) {
        return true;
      }
    }
  }

  return false;
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
      if(skip_power_on_reading(fru, SKIP_POWER_ON_SENSOR_READING_TIME)
        && sensor_map[fru].map[sensor_num].stby_read == false) {
        return READING_NA;
      } else {
        ret = sensor_map[fru].map[sensor_num].read_sensor(fru, sensor_num, (float*) value);
      }
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
  if (pal_is_artemis()) {
    switch (sensor_num) {
      case SENSOR_NUM_BIC_ALERT:
        sprintf(name, "BIC SENSOR STATUS");
        return 0;
      default:
        break;
    }
  }

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
      get_comp_source(fru, fru == FRU_FAN_BP1 ? FAN_BP1_LED_SOURCE : FAN_BP2_LED_SOURCE, &fan_id);
      if(fan_id == MAIN_SOURCE)
        fan_state_led_ctrl(fru, snr_num, true);
      else
        fan_state_led_ctrl(fru, snr_num, false);
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

      get_comp_source(fru, fru == FRU_FAN_BP1 ? FAN_BP1_LED_SOURCE : FAN_BP2_LED_SOURCE, &fan_id);
      if(fan_id == MAIN_SOURCE)
        fan_state_led_ctrl(fru, snr_num, false);
      else
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
  if (GETBIT(n_val, 0)) {
    syslog(LOG_CRIT, "ASSERT: %s - FRU: %d", name, fru);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s - FRU: %d", name, fru);
  }
  return 0;
}

int
pal_sensor_monitor_initial(void) {
  int ret = 0;
  uint8_t mb_sku = 0x00;
  char path[MAX_PATH_LEN] = {0};

  if (pal_is_artemis()) {
    if (access(MEB_MP5990_BIND_DIR, F_OK) != 0) {
      if (i2c_add_device(MEB_MP5990_BUS, MEB_MP5990_ADDR, MEB_MP5990_DEVICE_NAME) < 0) {
        syslog(LOG_ERR, "[%s] Fail to load MEB HSC driver\n", __func__);
        return -1;
      }
    }

    ret = pal_get_board_sku_id(FRU_MB, &mb_sku);
    if (ret != 0) {
      return -1;
    }

    if ((mb_sku & 0x0F) == GTA_CONFIG_1) {
      for (int addr = MB_INA230_ADDR_START; addr <= MB_INA230_ADDR_END; addr ++) {
        snprintf(path, sizeof(path), MB_INA230_BIND_DIR, addr);
        if (access(path, F_OK) == 0) {
          if (i2c_delete_device(MB_INA230_BUS, addr) < 0) {
            syslog(LOG_ERR, "[%s] Fail to unload MB INA230 driver\n", __func__);
            return -1;
          }
        }
      }
    }
    sensors_reinit();
  }

  return 0;
}

int
mb_vr_polling_ctrl(bool enable) {
  static int lock_fd = -1;

  if (enable) {
    if (lock_fd >= 0) {
      int fd = lock_fd;
      lock_fd = -1;
      single_instance_unlock(fd);
    }
  } else {
    int fd = single_instance_lock_blocked("mb_vr_mon");
    if (fd >= 0) {
      lock_fd = fd;
    }
  }
  return 0;
}

static int
get_vr_reading(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret, lock = -1;
  uint8_t cpu_id = sensor_map[fru].map[sensor_num].id/VR_SNR_PER_CPU;

  if (cpu_id < MAX_CPU_CNT) {  // for CPU VRs
    // check CPU present
    if (!is_cpu_socket_occupy(cpu_id)) {
      return READING_NA;
    }

    if ((lock = single_instance_lock("mb_vr_mon")) < 0) {
      return READING_SKIP;
    }
  }

  ret = sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
  if (lock >= 0) {
    single_instance_unlock(lock);
  }

  return ret;
}

int
read_vr_temp(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};

  if ((ret = get_vr_reading(fru, sensor_num, value)) == 0) {
    if (*value == 0) {
      retry[vr_id]++;
      return retry_err_handle(retry[vr_id], 5);
    }
    retry[vr_id] = 0;
  }

  return ret;
}

int
read_vr_vout(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t vr_id = sensor_map[fru].map[sensor_num].id;
  static uint8_t retry[VR_NUM_CNT] = {0};

  if ((ret = get_vr_reading(fru, sensor_num, value)) == 0) {
    if (*value == 0) {
      retry[vr_id]++;
      return retry_err_handle(retry[vr_id], 5);
    }
    retry[vr_id] = 0;
  }

  return ret;
}

int
read_vr_iout(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;

  // According to VR vendors, if the current is read a negative value ( >= -5), it should be ragarded as 0
  if ((ret = get_vr_reading(fru, sensor_num, value)) == 0) {
    if (*value >= -5 && *value <= 0) {
      *value = 0;
    }
  }

  return ret;
}

int
read_vr_pout(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = 0;

  // According to VR vendors, if the power is read a negative value ( >= -5.5), it should be ragarded as 0
  if ((ret = get_vr_reading(fru, sensor_num, value)) == 0) {
    if (*value >= -5.5 && *value <= 0) {
      *value = 0;
    }
  }

  return ret;
}
