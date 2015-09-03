/*
 * sensord
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <openbmc/sdr.h>
#ifdef CONFIG_YOSEMITE
#include <facebook/yosemite_sensor.h>
#endif /* CONFIG_YOSEMITE */

#define MAX_SENSOR_NUM      0xFF
#define NORMAL_STATE        0x00

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)



/* Enum for type of Upper and Lower threshold values */
enum {
  UCR_THRESH = 0x01,
  UNC_THRESH,
  UNR_THRESH,
  LCR_THRESH,
  LNC_THRESH,
  LNR_THRESH,
  POS_HYST,
  NEG_HYST,
};

/* To hold the sensor info and calculated threshold values from the SDR */
typedef struct {
  uint8_t flag;
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
  int curr_state;
  char name[23];
  char units[64];

} thresh_sensor_t;

/* Function pointer to read sensor current value */
static int (*read_snr_val)(uint8_t, uint8_t, void *);

#ifdef CONFIG_YOSEMITE

// TODO: Change to 6 after adding SPB and NIC
#define MAX_NUM_FRUS      4
#define YOSEMITE_SDR_PATH   "/tmp/sdr_%s.bin"

static thresh_sensor_t snr_slot1[MAX_SENSOR_NUM] = {0};
static thresh_sensor_t snr_slot2[MAX_SENSOR_NUM] = {0};
static thresh_sensor_t snr_slot3[MAX_SENSOR_NUM] = {0};
static thresh_sensor_t snr_slot4[MAX_SENSOR_NUM] = {0};
static thresh_sensor_t snr_spb[MAX_SENSOR_NUM] = {0};
static thresh_sensor_t snr_nic[MAX_SENSOR_NUM] = {0};

static sensor_info_t sinfo_slot1[MAX_SENSOR_NUM] = {0};
static sensor_info_t sinfo_slot2[MAX_SENSOR_NUM] = {0};
static sensor_info_t sinfo_slot3[MAX_SENSOR_NUM] = {0};
static sensor_info_t sinfo_slot4[MAX_SENSOR_NUM] = {0};
static sensor_info_t sinfo_spb[MAX_SENSOR_NUM] = {0};
static sensor_info_t sinfo_nic[MAX_SENSOR_NUM] = {0};
#endif /* CONFIG_YOSEMITE */


/*
 * Returns the pointer to the struct holding all sensor info and
 * calculated threshold values for the fru#
 */
static thresh_sensor_t *
get_struct_thresh_sensor(uint8_t fru) {

  thresh_sensor_t *snr;

#ifdef CONFIG_YOSEMITE
  switch (fru) {
    case FRU_SLOT1:
      snr = snr_slot1;
      break;
    case FRU_SLOT2:
      snr = snr_slot2;
      break;
    case FRU_SLOT3:
      snr = snr_slot3;
      break;
    case FRU_SLOT4:
      snr = snr_slot4;
      break;
    case FRU_SPB:
      snr = snr_spb;
      break;
    case FRU_NIC:
      snr = snr_nic;
      break;
    default:
      syslog(LOG_ALERT, "get_struct_thresh_sensor: Wrong FRU ID %d\n", fru);
      return NULL;
  }
#endif /* CONFIG_YOSEMITE */

  return snr;
}


/* Returns the all the SDRs for the particular fru# */
static sensor_info_t *
get_struct_sensor_info(uint8_t fru) {

  sensor_info_t *sinfo;

#ifdef CONFIG_YOSEMITE
  switch (fru) {
    case FRU_SLOT1:
      sinfo = sinfo_slot1;
      break;
    case FRU_SLOT2:
      sinfo = sinfo_slot2;
      break;
    case FRU_SLOT3:
      sinfo = sinfo_slot3;
      break;
    case FRU_SLOT4:
      sinfo = sinfo_slot4;
      break;
    case FRU_SPB:
      sinfo = sinfo_spb;
      break;
    case FRU_NIC:
      sinfo = sinfo_nic;
      break;
    default:
      syslog(LOG_ALERT, "get_struct_sensor_info: Wrong FRU ID %d\n", fru);
      return NULL;
  }
#endif /* CONFIG_YOSEMITE */

  return sinfo;
}


/* Returns the SDR for a particular sensor of particular fru# */
static sdr_full_t *
get_struct_sdr(uint8_t fru, uint8_t snr_num) {

  sdr_full_t *sdr;
  sensor_info_t *sinfo;
  sinfo = get_struct_sensor_info(fru);
  if (sinfo == NULL) {
    syslog(LOG_ALERT, "get_struct_sdr: get_struct_sensor_info failed\n");
    return NULL;
  }
  sdr = &sinfo[snr_num].sdr;
  return sdr;
}

/* Get the threshold values from the SDRs */
static int
get_sdr_thresh_val(uint8_t fru, uint8_t snr_num, uint8_t thresh, void *value) {

  int ret;
  uint8_t x;
  uint8_t m_lsb, m_msb, m;
  uint8_t b_lsb, b_msb, b;
  int8_t b_exp, r_exp;
  uint8_t thresh_val;
  sdr_full_t *sdr;

  sdr = get_struct_sdr(fru, snr_num);
  if (sdr == NULL) {
    syslog(LOG_ALERT, "get_sdr_thresh_val: get_struct_sdr failed\n");
    return -1;
  }

  switch (thresh) {
    case UCR_THRESH:
      thresh_val = sdr->uc_thresh;
      break;
    case UNC_THRESH:
      thresh_val = sdr->unc_thresh;
      break;
    case UNR_THRESH:
      thresh_val = sdr->unr_thresh;
      break;
    case LCR_THRESH:
      thresh_val = sdr->lc_thresh;
      break;
    case LNC_THRESH:
      thresh_val = sdr->lnc_thresh;
      break;
    case LNR_THRESH:
      thresh_val = sdr->lnr_thresh;
      break;
    case POS_HYST:
      thresh_val = sdr->pos_hyst;
      break;
    case NEG_HYST:
      thresh_val = sdr->neg_hyst;
      break;
    default:
      syslog(LOG_ERR, "get_sdr_thresh_val: reading unknown threshold val");
      return -1;
  }

  // y = (mx + b * 10^b_exp) * 10^r_exp
  x = thresh_val;

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }
  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }

  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  return 0;
}

/*
 * Populate all fields of thresh_sensor_t struct for a particular sensor.
 * Incase the threshold value is 0 mask the check for that threshvold
 * value in flag field.
 */
int
init_snr_thresh(uint8_t fru, uint8_t snr_num, uint8_t flag) {

  int value;
  float fvalue;
  uint8_t op, modifier;
  thresh_sensor_t *snr;

  sdr_full_t *sdr;

  sdr = get_struct_sdr(fru, snr_num);
  if (sdr == NULL) {
    syslog(LOG_ALERT, "init_snr_name: get_struct_sdr failed\n");
    return -1;
  }

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
    syslog(LOG_ALERT, "init_snr_thresh: get_struct_thresh_sensor failed");
    return -1;
  }

  snr[snr_num].flag = flag;
  snr[snr_num].curr_state = NORMAL_STATE;

  if (sdr_get_sensor_name(sdr, snr[snr_num].name)) {
    syslog(LOG_ALERT, "sdr_get_sensor_name: FRU %d: num: 0x%X: reading name"
        " from SDR failed.", fru, snr_num);
    return -1;
  }

  // TODO: Add support for modifier (Mostly modifier is zero)
  if (sdr_get_sensor_units(sdr, &op, &modifier, snr[snr_num].units)) {
    syslog(LOG_ALERT, "sdr_get_sensor_units: FRU %d: num 0x%X: reading units"
        " from SDR failed.", fru, snr_num);
    return -1;
  }

  if (get_sdr_thresh_val(fru, snr_num, UCR_THRESH, &fvalue)) {
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, UCR_THRESH",
        fru, snr_num, snr[snr_num].name);
  } else {
    snr[snr_num].ucr_thresh = fvalue;
    if (!(fvalue)) {
      snr[snr_num].flag = CLEARBIT(snr[snr_num].flag, UCR_THRESH);
      syslog(LOG_ALERT,
          "FRU: %d, num: 0x%X, %-16s, UCR_THRESH check disabled val->%.2f",
          fru, snr_num, snr[snr_num].name, fvalue);
    }
  }

  if (get_sdr_thresh_val(fru, snr_num, UNC_THRESH, &fvalue)) {
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, UNC_THRESH",
        fru, snr_num, snr[snr_num].name);
  } else {
    snr[snr_num].unc_thresh = fvalue;
    if (!(fvalue)) {
      snr[snr_num].flag = CLEARBIT(snr[snr_num].flag, UNC_THRESH);
      syslog(LOG_ALERT,
          "FRU: %d, num: 0x%X, %-16s, UNC_THRESH check disabled val->%.2f",
          fru, snr_num, snr[snr_num].name, fvalue);
    }
  }

  if (get_sdr_thresh_val(fru, snr_num, UNR_THRESH, &fvalue)) {
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, UNR_THRESH",
        fru, snr_num, snr[snr_num].name);
  } else {
    snr[snr_num].unr_thresh = fvalue;
    if (!(fvalue)) {
      snr[snr_num].flag = CLEARBIT(snr[snr_num].flag, UNR_THRESH);
      syslog(LOG_ALERT,
          "FRU: %d, num: 0x%X, %-16s, UNR_THRESH check disabled val->%.2f",
          fru, snr_num, snr[snr_num].name, fvalue);
    }
  }

  if (get_sdr_thresh_val(fru, snr_num, LCR_THRESH, &fvalue)) {
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, LCR_THRESH",
        fru, snr_num, snr[snr_num].name);
  } else {
    snr[snr_num].lcr_thresh = fvalue;
    if (!(fvalue)) {
      snr[snr_num].flag = CLEARBIT(snr[snr_num].flag, LCR_THRESH);
      syslog(LOG_ALERT,
          "FRU: %d, num: 0x%X, %-16s, LCR_THRESH check disabled val->%.2f",
          fru, snr_num, snr[snr_num].name, fvalue);
    }
  }

  if (get_sdr_thresh_val(fru, snr_num, LNC_THRESH, &fvalue)) {
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, LNC_THRESH",
        fru, snr_num, snr[snr_num].name);
  } else {
    snr[snr_num].lnc_thresh = fvalue;
    if (!(fvalue)) {
      snr[snr_num].flag = CLEARBIT(snr[snr_num].flag, LNC_THRESH);
      syslog(LOG_ALERT,
          "FRU: %d, num: 0x%X, %-16s, LNC_THRESH check disabled val->%.2f",
          fru, snr_num, snr[snr_num].name, fvalue);
    }
  }

  if (get_sdr_thresh_val(fru, snr_num, LNR_THRESH, &fvalue)) {
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, LNR_THRESH",
        fru, snr_num, snr[snr_num].name);
  } else {
    snr[snr_num].lnr_thresh = fvalue;
    if (!(fvalue)) {
      snr[snr_num].flag = CLEARBIT(snr[snr_num].flag, LNR_THRESH);
      syslog(LOG_ALERT,
          "FRU: %d, num: 0x%X, %-16s, LNR_THRESH check disabled val->%.2f",
          fru, snr_num, snr[snr_num].name, fvalue);
    }
  }

  if (get_sdr_thresh_val(fru, snr_num, POS_HYST, &fvalue)) {
    syslog(LOG_ERR, "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, POS_HYST",
        fru, snr_num, snr[snr_num].name);
  } else
    snr[snr_num].pos_hyst = fvalue;

  if (get_sdr_thresh_val(fru, snr_num, NEG_HYST, &fvalue)) {
    syslog(LOG_ERR, "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, NEG_HYST",
        fru, snr_num, snr[snr_num].name);
  } else
    snr[snr_num].neg_hyst = fvalue;

  return 0;
}

#ifdef CONFIG_YOSEMITE
/* Initialize all thresh_sensor_t structs for all the Yosemite sensors */
static void
init_yosemite_snr_thresh(uint8_t fru) {

  int i;

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:

      for (i < 0; i < bic_sensor_cnt; i++) {
        init_snr_thresh(fru, bic_sensor_list[i], GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));
      }
      /*
  		init_snr_thresh(fru, BIC_SENSOR_MB_OUTLET_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCCIN_VR_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_GBE_VR_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_1V05PCH_VR_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_MB_INLET_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_PCH_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_THERM_MARGIN, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VDDR_VR_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_GBE_VR_CURR, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_1V05_PCH_VR_CURR, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCCIN_VR_POUT, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCCIN_VR_CURR, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCCIN_VR_VOL,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_INA230_POWER, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_PACKAGE_PWR, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_TJMAX, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VDDR_VR_POUT, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VDDR_VR_CURR, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VDDR_VR_VOL,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_SCSUS_VR_CURR, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_SCSUS_VR_VOL,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_SCSUS_VR_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_SCSUS_VR_POUT, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_GBE_VR_POUT, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_VCC_GBE_VR_VOL,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_1V05_PCH_VR_VOL,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_DIMMA0_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_DIMMA1_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_DIMMB0_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_SOC_DIMMB1_TEMP, GETMASK(UCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_P3V3_MB,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_P12V_MB,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_P1V05_PCH,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_P3V3_STBY_MB,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_P5V_STBY_MB,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_PV_BAT,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_PVDDR,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));

  		init_snr_thresh(fru, BIC_SENSOR_PVCC_GBE,
      GETMASK(UCR_THRESH) | GETMASK(LCR_THRESH));
  // TODO: Add Support for Discrete sensors
  //		init_snr_thresh(fru, BIC_SENSOR_POST_ERR, //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_SYSTEM_STATUS, //Discrete
  //		init_snr_thresh(fru, BIC_SENSOR_SPS_FW_HLTH); //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_POWER_THRESH_EVENT, //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_MACHINE_CHK_ERR, //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_PCIE_ERR); //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_OTHER_IIO_ERR); //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_PROC_HOT_EXT); //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_POWER_ERR); //Event-only
  //		init_snr_thresh(fru, , ); //Event-only
  //		init_snr_thresh(fru, BIC_SENSOR_PROC_FAIL); //Discrete
  //		init_snr_thresh(fru, BIC_SENSOR_SYS_BOOT_STAT ); //Discrete
  //		init_snr_thresh(fru, BIC_SENSOR_VR_HOT); //Discrete
  //		init_snr_thresh(fru, BIC_SENSOR_CPU_DIMM_HOT ); //Discrete
  //		init_snr_thresh(fru, BIC_SENSOR_CAT_ERR,  //Event-only

      */

      break;

    case FRU_SPB:
  // TODO: Add support for threshold calculation for SP sensors
  /*
  		init_snr_thresh(fru, SP_SENSOR_INLET_TEMP, "SP_SENSOR_INLET_TEMP");
  		init_snr_thresh(fru, SP_SENSOR_OUTLET_TEMP, "SP_SENSOR_OUTLET_TEMP");
  		init_snr_thresh(fru, SP_SENSOR_MEZZ_TEMP, "SP_SENSOR_MEZZ_TEMP");
  		init_snr_thresh(fru, SP_SENSOR_FAN0_TACH, "SP_SENSOR_FAN0_TACH");
  		init_snr_thresh(fru, SP_SENSOR_FAN1_TACH, "SP_SENSOR_FAN1_TACH");
  		init_snr_thresh(fru, SP_SENSOR_AIR_FLOW, "SP_SENSOR_AIR_FLOW");
  		init_snr_thresh(fru, SP_SENSOR_P5V, "SP_SENSOR_P5V");
  		init_snr_thresh(fru, SP_SENSOR_P12V, "SP_SENSOR_P12V");
  		init_snr_thresh(fru, SP_SENSOR_P3V3_STBY, "SP_SENSOR_P3V3_STBY");
  		init_snr_thresh(fru, SP_SENSOR_P12V_SLOT0, "SP_SENSOR_P12V_SLOT0");
  		init_snr_thresh(fru, SP_SENSOR_P12V_SLOT1, "SP_SENSOR_P12V_SLOT1");
  		init_snr_thresh(fru, SP_SENSOR_P12V_SLOT2, "SP_SENSOR_P12V_SLOT2");
  		init_snr_thresh(fru, SP_SENSOR_P12V_SLOT3, "SP_SENSOR_P12V_SLOT3");
  		init_snr_thresh(fru, SP_SENSOR_P3V3, "SP_SENSOR_P3V3");
  		init_snr_thresh(fru, SP_SENSOR_HSC_IN_VOLT, "SP_SENSOR_HSC_IN_VOLT");
  		init_snr_thresh(fru, SP_SENSOR_HSC_OUT_CURR, "SP_SENSOR_HSC_OUT_CURR");
  		init_snr_thresh(fru, SP_SENSOR_HSC_TEMP, "SP_SENSOR_HSC_TEMP");
  		init_snr_thresh(fru, SP_SENSOR_HSC_IN_POWER, "SP_SENSOR_HSC_IN_POWER");
  */
      break;

    case FRU_NIC:
      // TODO: Add support for NIC sensor threshold, if any.
      break;

    default:
      syslog(LOG_ALERT, "init_yosemite_snr_thresh: wrong FRU ID");
      exit(-1);
  }
}
#endif /* CONFIG_YOSEMITE */

/* Wrapper function to initialize all the platform sensors */
static void
init_all_snr_thresh() {
  int fru;

  char path[64] = {0};
  sensor_info_t *sinfo;


#ifdef CONFIG_YOSEMITE
  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_FRUS); fru++) {

    if (get_fru_sdr_path(fru, path) < 0) {
      syslog(LOG_ALERT, "yosemite_sdr_init: get_fru_sdr_path failed\n");
      continue;
    }
#endif /* CONFIG_YOSEMITE */

    sinfo = get_struct_sensor_info(fru);

    if (sdr_init(path, sinfo) < 0)
      syslog(LOG_ERR, "init_all_snr_thresh: sdr_init failed for FRU %d", fru);
  }

#ifdef CONFIG_YOSEMITE
  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_FRUS); fru++) {
        init_yosemite_snr_thresh(fru);
  }
#endif /* CONFIG_YOSEMITE */
}




static float
get_snr_thresh_val(uint8_t fru, uint8_t snr_num, uint8_t thresh) {

  float val;
  thresh_sensor_t *snr;

  snr = get_struct_thresh_sensor(fru);

  switch (thresh) {
    case UCR_THRESH:
      val = snr[snr_num].ucr_thresh;
      break;
    case UNC_THRESH:
      val = snr[snr_num].unc_thresh;
      break;
    case UNR_THRESH:
      val = snr[snr_num].unr_thresh;
      break;
    case LCR_THRESH:
      val = snr[snr_num].lcr_thresh;
      break;
    case LNC_THRESH:
      val = snr[snr_num].lnc_thresh;
      break;
    case LNR_THRESH:
      val = snr[snr_num].lnr_thresh;
      break;
    default:
      syslog(LOG_ALERT, "get_snr_thresh_val: wrong thresh enum value");
      exit(-1);
  }

  return val;
}

/*
 * Check the curr sensor values against the threshold and
 * if the curr val has deasserted, log it.
 */
static void
check_thresh_deassert(uint8_t fru, uint8_t snr_num, uint8_t thresh,
  float curr_val) {
  uint8_t curr_state = 0;
  float thresh_val;
  char thresh_name[100];
  thresh_sensor_t *snr;

  snr = get_struct_thresh_sensor(fru);

  if (!GETBIT(snr[snr_num].flag, thresh) ||
      !GETBIT(snr[snr_num].curr_state, thresh))
    return;

  thresh_val = get_snr_thresh_val(fru, snr_num, thresh);

  switch (thresh) {
    case UNC_THRESH:
      if (curr_val <= thresh_val) {
        curr_state = ~(SETBIT(curr_state, UNR_THRESH) |
            SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Non Critical");
      }
      break;

    case UCR_THRESH:
      if (curr_val <= thresh_val) {
        curr_state = ~(SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNR_THRESH));
        sprintf(thresh_name, "Upper Critical");
      }
      break;

    case UNR_THRESH:
      if (curr_val <= thresh_val) {
        curr_state = ~(SETBIT(curr_state, UNR_THRESH));
        sprintf(thresh_name, "Upper Non Recoverable");
      }
      break;

    case LNC_THRESH:
      if (curr_val >= thresh_val) {
        curr_state = ~(SETBIT(curr_state, LNR_THRESH) |
            SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Non Critical");
      }
      break;

    case LCR_THRESH:
      if (curr_val >= thresh_val) {
        curr_state = ~(SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNR_THRESH));
        sprintf(thresh_name, "Lower Critical");
      }
      break;

    case LNR_THRESH:
      if (curr_val >= thresh_val) {
        curr_state = ~(SETBIT(curr_state, LNR_THRESH));
        sprintf(thresh_name, "Lower Non Recoverable");
      }
      break;

    default:
      syslog(LOG_ALERT, "get_snr_thresh_val: wrong thresh enum value");
      exit(-1);
  }

  if (curr_state) {
    snr[snr_num].curr_state &= curr_state;
    syslog(LOG_CRIT, "DEASSERT: %s threshold raised - FRU: %d, num: 0x%X,"
        " snr: %-16s,",thresh_name, fru, snr_num, snr[snr_num].name);
    syslog(LOG_CRIT, "curr_val: %.2f %s, thresh_val: %.2f %s cf: %u",
      curr_val, snr[snr_num].units, thresh_val, snr[snr_num].units, snr[snr_num].curr_state);
  }
}


/*
 * Check the curr sensor values against the threshold and
 * if the curr val has asserted, log it.
 */
static void
check_thresh_assert(uint8_t fru, uint8_t snr_num, uint8_t thresh,
  float curr_val) {
  uint8_t curr_state = 0;
  float thresh_val;
  char thresh_name[100];
  thresh_sensor_t *snr;

  snr = get_struct_thresh_sensor(fru);

  if (!GETBIT(snr[snr_num].flag, thresh) ||
      GETBIT(snr[snr_num].curr_state, thresh))
    return;

  thresh_val = get_snr_thresh_val(fru, snr_num, thresh);

  switch (thresh) {
    case UNR_THRESH:
      if (curr_val >= thresh_val) {
        curr_state = (SETBIT(curr_state, UNR_THRESH) |
            SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Non Recoverable");
      }
      break;

    case UCR_THRESH:
      if (curr_val >= thresh_val) {
        curr_state = (SETBIT(curr_state, UCR_THRESH) |
            SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Critical");
      }
      break;

    case UNC_THRESH:
      if (curr_val >= thresh_val) {
        curr_state = (SETBIT(curr_state, UNC_THRESH));
        sprintf(thresh_name, "Upper Non Critical");
      }
      break;

    case LNR_THRESH:
      if (curr_val <= thresh_val) {
        curr_state = (SETBIT(curr_state, LNR_THRESH) |
            SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Non Recoverable");
      }
      break;

    case LCR_THRESH:
      if (curr_val <= thresh_val) {
        curr_state = (SETBIT(curr_state, LCR_THRESH) |
            SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Critical");
      }
      break;

    case LNC_THRESH:
      if (curr_val <= thresh_val) {
        curr_state = (SETBIT(curr_state, LNC_THRESH));
        sprintf(thresh_name, "Lower Non Critical");
      }
      break;

    default:
      syslog(LOG_ALERT, "get_snr_thresh_val: wrong thresh enum value");
      exit(-1);
  }

  if (curr_state) {
    curr_state &= snr[snr_num].flag;
    snr[snr_num].curr_state |= curr_state;
    syslog(LOG_CRIT, "ASSERT: %s threshold raised - FRU: %d, num: 0x%X,"
        " snr: %-16s,",thresh_name, fru, snr_num, snr[snr_num].name);
    syslog(LOG_CRIT, "curr_val: %.2f %s, thresh_val: %.2f %s cf: %u",
      curr_val, snr[snr_num].units, thresh_val, snr[snr_num].units, snr[snr_num].curr_state);
  }
}

/*
 * Starts monitoring all the sensors on a fru for all the threshold values.
 * Each pthread runs this monitoring for a different fru.
 */
static void *
snr_monitor(void *arg) {

  uint8_t fru = *(uint8_t *) arg;
  int f, ret, snr_num;
  float normal_val, curr_val;
  thresh_sensor_t *snr;

#ifdef TESTING
  float temp_thresh;
  int cnt = 0;
#endif /* TESTING */

  snr = get_struct_thresh_sensor(fru);
  if (snr == NULL) {
    syslog(LOG_ALERT, "snr_monitor: get_struct_thresh_sensor failed");
    exit(-1);
  }

  while(1) {

#ifdef TESTING
    cnt++;
#endif /* TESTING */

    for (snr_num = 0; snr_num < MAX_SENSOR_NUM; snr_num++) {
      curr_val = 0;
      if (snr[snr_num].flag) {
        if (!(ret = read_snr_val(fru, snr_num, &curr_val))) {


#ifdef TESTING
          /*
           * The curr_val crosses UCR and then return to a state
           * where UNC < curr_val < UCR and then eventually back to normal.
           */
          if (cnt == 5 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].flag |= SETBIT(snr[snr_num].flag, UNC_THRESH);
            temp_thresh = snr[snr_num].ucr_thresh;
            snr[snr_num].ucr_thresh = 20.0;
            snr[snr_num].unc_thresh = 10.0;
          } else if (cnt == 8 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].ucr_thresh = temp_thresh;
          } else if (cnt == 10 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].unc_thresh = 50.0;
          } else if (cnt == 11 && snr_num == BIC_SENSOR_MB_INLET_TEMP) {
            snr[snr_num].unc_thresh = 0.0;
            snr[snr_num].flag &= CLEARBIT(snr[snr_num].flag, UNC_THRESH);

          }
#endif /* TESTING */

#ifdef DEBUG
          if (cnt == 2) {
            syslog(LOG_INFO, "pthread %d, cnt: %d, num: 0x%X name: %-16s"
                " units:%s", fru, cnt, snr_num, snr[snr_num].name,
                snr[snr_num].units);
          }
#endif /* DEBUG */

          check_thresh_assert(fru, snr_num, UNR_THRESH, curr_val);
          check_thresh_assert(fru, snr_num, UCR_THRESH, curr_val);
          check_thresh_assert(fru, snr_num, UNC_THRESH, curr_val);
          check_thresh_assert(fru, snr_num, LNR_THRESH, curr_val);
          check_thresh_assert(fru, snr_num, LCR_THRESH, curr_val);
          check_thresh_assert(fru, snr_num, LNC_THRESH, curr_val);

          check_thresh_deassert(fru, snr_num, UNC_THRESH, curr_val);
          check_thresh_deassert(fru, snr_num, UCR_THRESH, curr_val);
          check_thresh_deassert(fru, snr_num, UNR_THRESH, curr_val);
          check_thresh_deassert(fru, snr_num, LNC_THRESH, curr_val);
          check_thresh_deassert(fru, snr_num, LCR_THRESH, curr_val);
          check_thresh_deassert(fru, snr_num, LNR_THRESH, curr_val);

        } else {
          /*
           * Incase the read_snr_val failed for a sensor,
           * disable all the threshold checks for that sensor
           * after logging an approciate syslog message.
           */
          if (ret) {
            syslog(LOG_ERR, "FRU: %d, num: 0x%X, snr:%-16s, read failed",
            fru, snr_num, snr[snr_num].name);
            syslog(LOG_ERR, "FRU: %d, num: 0x%X, snr:%-16s, check disabled",
                fru, snr_num, snr[snr_num].name);
            snr[snr_num].flag = 0;
            //}
          }
        } /* read_snr_val return check */
      } /* flag check */
    } /* loop for all sensors */
    sleep(5);
  } /* while loop*/
} /* function definition */


/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_sensord(int argc, char **argv) {
  int i, arg;

  pthread_t thread_snr[MAX_NUM_FRUS];
  int fru[MAX_NUM_FRUS] = {0};

  for (arg = 1; arg < argc; arg ++) {
#ifdef CONFIG_YOSEMITE
    if (!(strcmp(argv[arg], "slot1")))
      fru[FRU_SLOT1 - 1] =  FRU_SLOT1;
    else if (!(strcmp(argv[arg], "slot2")))
      fru[FRU_SLOT2 - 1] =  FRU_SLOT2;
    else if (!(strcmp(argv[arg], "slot3")))
      fru[FRU_SLOT3 - 1] =  FRU_SLOT3;
    else if (!(strcmp(argv[arg], "slot4")))
      fru[FRU_SLOT4 - 1] =  FRU_SLOT4;
    else if (!(strcmp(argv[arg], "spb")))
      fru[FRU_SPB - 1] =  FRU_SPB;
    else if (!(strcmp(argv[arg], "nic")))
      fru[FRU_NIC - 1] =  FRU_NIC;
    else {
      syslog(LOG_ALERT, "Wrong argument: %s", argv[arg]);
      exit(1);
    }
#endif /* CONFIG_YOSEMITE */

  }

  for (i = 0; i < MAX_NUM_FRUS; i++) {
    if (fru[i]) {
      if (pthread_create(&thread_snr[i], NULL, snr_monitor,
            (void*) &fru[i]) < 0) {
        syslog(LOG_ALERT, "pthread_create for FRU %d failed\n", fru[i]);
      }
#ifdef DEBUG
      else {
        syslog(LOG_ALERT, "pthread_create for FRU %d succeed\n", fru[i]);
      }
#endif /* DEBUG */
    }
    sleep(1);
  }

  for (i = 0; i < MAX_NUM_FRUS; i++) {
    if (fru[i])
      pthread_join(thread_snr[i], NULL);
  }
}


#ifdef DEBUG
void print_snr_thread(uint8_t fru, thresh_sensor_t *snr)
{
  int i;
  float curr_val;

  for (i = 1; i <= MAX_SENSOR_NUM; i++) {
    if (snr[i].flag) {
      curr_val = 0;
      if(!(read_snr_val(fru, i, &curr_val))) {
        printf("%-30s:\t%.2f %s\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
        snr[i].name, curr_val,snr[i].units,
        snr[i].ucr_thresh,
        snr[i].unc_thresh,
        snr[i].lcr_thresh,
        snr[i].lnr_thresh,
        snr[i].pos_hyst,
        snr[i].neg_hyst);
      } else
        printf("Reading failed: %-16s\n", snr[i].name);
    }
  }
}
#endif /* DEBUG */

int
main(int argc, void **argv) {
  int dev, rc, pid_file;

  if (argc < 2) {
    syslog(LOG_ALERT, "Usage: sensord <options>");
    printf("Usage: sensord <options>\n");
#ifdef CONFIG_YOSEMITE
    syslog(LOG_ALERT, "Options: [slot1 | slot2 | slot3 | slot4 | spb | nic]");
    printf("Options: [slot1 | slot2 | slot3 | slot4 | spb | nic]\n");
#endif /* CONFIG_YOSEMITE */
    exit(1);
  }

  pid_file = open("/var/run/sensord.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another sensord instance is running...\n");
      exit(-1);
    }
  } else {

#ifdef CONFIG_YOSEMITE
  read_snr_val = &yosemite_sensor_read;
#endif /* CONFIG_YOSEMITE */

    init_all_snr_thresh();

#ifdef DEBUG
    print_snr_thread(1, snr_slot1);
    print_snr_thread(2, snr_slot2);
    print_snr_thread(3, snr_slot3);
    print_snr_thread(4, snr_slot4);
#endif /* DEBUG */

    daemon(0,1);
    openlog("sensord", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "sensord: daemon started");
    run_sensord(argc, (char **) argv);
  }

  return 0;
}
