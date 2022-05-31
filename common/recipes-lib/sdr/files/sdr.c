/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include "sdr.h"

#define FIELD_RATE_UNIT(x)  ((x & (0x07 << 3)) >> 3)
#define FIELD_OP(x)         ((x & (0x03 << 1)) >> 1)
#define FIELD_PERCENTAGE(x) (x & 0x01)

#define FIELD_TYPE(x)       ((x & (0x03 << 6)) >> 6)
#define FIELD_LEN(x)        (x & 0x1F)

#ifndef ERR_NOT_READY
#define ERR_NOT_READY -2
#endif
#ifndef MAX_RETRIES_SDR_INIT
#define MAX_RETRIES_SDR_INIT 1
#endif

#define MAX_NAME_LEN        16

/* Array for BCD Plus definition. */
const char bcd_plus_array[] = "0123456789 -.XXX";

/* Array for 6-Bit ASCII definition. */
const char * ascii_6bit[4] = {
  " !\"#$%&'()*+,-./",
  "0123456789:;<=>?",
  "@ABCDEFGHIJKLMNO",
  "PQRSTUVWXYZ[\\]^_"
};

/* List of all the Sensor Rate Units types. */
const char * sensor_rate_units[] = {
  "",             /* 0x00 */
  "per \xC2\xB5s",  /* 0x01 */
  "per ms",         /* 0x02 */
  "per s",          /* 0x03 */
  "per min",        /* 0x04 */
  "per hour",       /* 0x05 */
  "per day",        /* 0x06 */
  "reserved",       /* 0x07 */
};


/* List of all the Sensor Base Units types. */
const char * sensor_base_units[] = {
  "",             /* 000 */
  "C",              /* 001 */
  "F",              /* 002 */
  "K",              /* 003 */
  "Volts",          /* 004 */
  "Amps",           /* 005 */
  "Watts",          /* 006 */
  "Joules",         /* 007 */
  "Coulombs",       /* 008 */
  "VA",             /* 009 */
  "Nits",           /* 010 */
  "lumen",          /* 011 */
  "lux",			      /* 012 */
  "Candela",			  /* 013 */
  "kPa",			      /* 014 */
  "PSI",			      /* 015 */
  "Newton",			    /* 016 */
  "CFM",			      /* 017 */
  "RPM",			      /* 018 */
  "Hz",			        /* 019 */
  "\xC2\xB5s",			/* 020 */
  "ms",			        /* 021 */
  "sec",			      /* 022 */
  "min",			      /* 023 */
  "hour",			      /* 024 */
  "day",			      /* 025 */
  "week",			      /* 026 */
  "mil",			      /* 027 */
  "inches",			    /* 028 */
  "feet",			      /* 029 */
  "cu in",			    /* 030 */
  "cu feet",			  /* 031 */
  "mm",			        /* 032 */
  "cm",			        /* 033 */
  "m",			        /* 034 */
  "cu cm",			    /* 035 */
  "cu m",			      /* 036 */
  "liters",			    /* 037 */
  "fluid ounce",		/* 038 */
  "radians",			  /* 039 */
  "steradians",			/* 040 */
  "revolutions",		/* 041 */
  "cycles",			    /* 042 */
  "gravities",			/* 043 */
  "ounce",			    /* 044 */
  "pound",			    /* 045 */
  "ft-lb",			    /* 046 */
  "oz-in",			    /* 047 */
  "gauss",			    /* 048 */
  "gilberts",			  /* 049 */
  "henry",			    /* 050 */
  "millihenry",			/* 051 */
  "farad",			    /* 052 */
  "microfarad",			/* 053 */
  "ohms",			      /* 054 */
  "siemens",			  /* 055 */
  "mole",			      /* 056 */
  "becquerel",			/* 057 */
  "PPM",						/* 058 */
  "reserved",				/* 059 */
  "Db",						  /* 060 */
  "DbA",						/* 061 */
  "DbC",						/* 062 */
  "gray",						/* 063 */
  "sievert",				/* 064 */
  "color temp deg K", /* 065 */
  "bit",					  /* 066 */
  "kilobit",			 	/* 067 */
  "megabit",			 	/* 068 */
  "gigabit",			 	/* 069 */
  "B",						  /* 070 */
  "KB",						  /* 071 */
  "MB",						  /* 072 */
  "GB",						  /* 073 */
  "word",						/* 074 */
  "dword",					/* 075 */
  "qword",					/* 076 */
  "line",						/* 077 */
  "hit",						/* 078 */
  "miss",						/* 079 */
  "retry",					/* 080 */
  "reset",					/* 081 */
  "overflow",				/* 082 */
  "underrun",				/* 083 */
  "collision",			/* 084 */
  "packets",				/* 085 */
  "messages",				/* 086 */
  "characters",			/* 087 */
  "error",					/* 088 */
  "correctable error",	  /* 089 */
  "uncorrectable error",	/* 090 */
  "fatal error",					/* 091 */
  "grams",					/* 092 */
  "",						    /* 093 */
};

/* Get the units of the sensor from the SDR */
static int
_sdr_get_sensor_units(sdr_full_t *sdr, uint8_t *op, uint8_t *modifier,
    char *units) {

  int ret;
  uint8_t percent;
  uint8_t rate_idx;
  uint8_t base_idx;

  /* Bits 5:3 */
  rate_idx = FIELD_RATE_UNIT(sdr->sensor_units1);

  /* Bits 2:1 */
  *op = FIELD_OP(sdr->sensor_units1);

  /* Bit 0 */
  percent = FIELD_PERCENTAGE(sdr->sensor_units1);

  base_idx = sdr->sensor_units2;

  if (*op == 0x0 || *op == 0x3)
    *modifier = 0;
  else
    *modifier = sdr->sensor_units3;

  if (percent) {
    sprintf(units, "%%");
  } else {
    if (base_idx >= 0 && base_idx <= MAX_SENSOR_BASE_UNIT) {
      if (rate_idx > 0 && rate_idx < MAX_SENSOR_RATE_UNIT) {
        sprintf(units, "%s %s", sensor_base_units[base_idx],
            sensor_rate_units[rate_idx]);
      } else {
        sprintf(units, "%s", sensor_base_units[base_idx]);
      }
    }
  }

  return 0;
}

int
sdr_get_sensor_units(uint8_t fru, uint8_t snr_num, char *units) {

  int ret = 0;
  uint8_t op;
  uint8_t modifier;
  sdr_full_t *sdr;

  sensor_info_t sinfo[MAX_SENSOR_NUM + 1] = {0};

  if (pal_sensor_sdr_init(fru, sinfo) < 0) {
    sdr = NULL;
  } else {
    sdr = &sinfo[snr_num].sdr;
  }

  if (sdr != NULL) {
    ret = _sdr_get_sensor_units(sdr, &op, &modifier, units);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "_sdr_get_sensor_units failed for FRU: %d snr_num: %d",
          fru, snr_num);
#endif
    }
  } else {
    ret = pal_get_sensor_units(fru, snr_num, units);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "pal_get_sensor_units failed for FRU: %d snr_num: %d",
          fru, snr_num);
#endif
    }
  }

  return ret;
}

/* Get the name of the sensor from the SDR */
static int
_sdr_get_sensor_name(sdr_full_t *sdr, char *name) {
  int field_type, field_len;
  int idx, idx_eff, val;
  char *str;

  /* Bits 7:6 */
  field_type = FIELD_TYPE(sdr->str_type_len);
  /* Bits 4:0 */
  field_len = FIELD_LEN(sdr->str_type_len) + 1;

  str = sdr->str;

  /* Case: length is zero */
  if (field_len == 1) {
#ifdef DEBUG
    syslog(LOG_WARNING, "get_sensor_name: str length is 0\n");
#endif
    return -1;
  }

  /* Retrieve field data depending on the type it was stored. */
  switch (field_type) {
    case TYPE_BINARY:
      /* TODO: Need to add support to read data stored in binary type. */
      break;

    case TYPE_BCD_PLUS:

      for (idx = 0; idx < field_len - 1; idx++) {
        name[idx * 2] = bcd_plus_array[(str[idx] >> 4) & 0x0F];
        name[idx * 2 + 1] = bcd_plus_array[str[idx] & 0x0F];
      }
      name[idx * 2] = '\0';
      break;

  case TYPE_ASCII_6BIT:

    idx_eff = idx = 0;

    while (field_len > 0) {

      /* 6-Bits => Bits 5:0 of the first byte */
      val = str[idx] & 0x3F;
      name[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];
      field_len--;

      if (field_len > 0) {
        /* 6-Bits => Bits 3:0 of second byte + Bits 7:6 of first byte. */
        val = ((str[idx] & 0xC0) >> 6) |
              ((str[idx + 1] & 0x0F) << 2);
        name[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];
        field_len--;
      }

      if (field_len > 0) {
        /* 6-Bits => Bits 1:0 of third byte + Bits 7:4 of second byte. */
        val = ((str[idx + 1] & 0xF0) >> 4) |
              ((str[idx + 2] & 0x03) << 4);
        name[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];

        /* 6-Bits => Bits 7:2 of third byte. */
        val = ((str[idx + 2] & 0xFC) >> 2);
        name[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];

        field_len--;
        idx += 3;
      }
    }
    /* Add Null terminator */
    name[idx_eff] = '\0';
    break;

  case TYPE_ASCII_8BIT:
    snprintf(name, field_len, "%s", str);
    /* Add Null terminator */
    name[field_len] = '\0';
    break;
  }

  return 0;
}

int
sdr_get_sensor_name(uint8_t fru, uint8_t snr_num, char *name) {

  int ret = 0;
  sdr_full_t *sdr;

  sensor_info_t sinfo[MAX_SENSOR_NUM + 1] = {0};

  if (pal_sensor_sdr_init(fru, sinfo) < 0) {
    sdr = NULL;
  } else {
    sdr = &sinfo[snr_num].sdr;
  }

  if (sdr != NULL) {
    ret = _sdr_get_sensor_name(sdr, name);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "_sdr_get_sensor_name failed for FRU: %d snr_num: %d",
          fru, snr_num);
#endif
    }
  } else {
    ret = pal_get_sensor_name(fru, snr_num, name);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "pal_get_sensor_name failed for FRU: %d snr_num: %d",
          fru, snr_num);
#endif
    }
  }

  return ret;
}

/* Get the threshold values from the SDRs */
static int
get_sdr_thresh_val(uint8_t fru, sdr_full_t *sdr, uint8_t snr_num,
    uint8_t thresh, float *value) {

  int ret;
  uint8_t m_lsb, m_msb;
  uint16_t m = 0;
  uint8_t b_lsb, b_msb;
  uint16_t b = 0;
  int8_t b_exp, r_exp;
  uint8_t thresh_val;

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
#ifdef DEBUG
      syslog(LOG_ERR, "get_sdr_thresh_val: reading unknown threshold val");
#endif
      return -1;
  }

  if (pal_convert_sensor_reading(sdr, (int)thresh_val, value) < 0) {
    return -1;
  }

  return 0;
}



/*
 * Populate all fields of thresh_sensor_t struct for a particular sensor.
 * Incase the threshold value is 0 mask the check for that threshvold
 * value in flag field.
 */
static int
_sdr_get_snr_thresh(uint8_t fru, sdr_full_t *sdr, uint8_t snr_num,
    thresh_sensor_t *snr) {

  int ret;
  int value;
  float fvalue;
  uint8_t op, modifier;

  snr->curr_state = NORMAL_STATE;

  if (_sdr_get_sensor_name(sdr, snr->name)) {
#ifdef DEBUG
    syslog(LOG_WARNING, "sdr_get_sensor_name: FRU %d: num: 0x%X: reading name"
        " from SDR failed.", fru, snr_num);
#endif
    return -1;
  }

  // TODO: Add support for modifier (Mostly modifier is zero)
  if (_sdr_get_sensor_units(sdr, &op, &modifier, snr->units)) {
#ifdef DEBUG
    syslog(LOG_WARNING, "sdr_get_sensor_units: FRU %d: num 0x%X: reading units"
        " from SDR failed.", fru, snr_num);
#endif
    return -1;
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, UCR_THRESH, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, UCR_THRESH",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->ucr_thresh = fvalue;
    if (!(fvalue)) {
      snr->flag = CLEARBIT(snr->flag, UCR_THRESH);
    }
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, UNC_THRESH, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, UNC_THRESH",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->unc_thresh = fvalue;
    if (!(fvalue)) {
      snr->flag = CLEARBIT(snr->flag, UNC_THRESH);
    }
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, UNR_THRESH, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, UNR_THRESH",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->unr_thresh = fvalue;
    if (!(fvalue)) {
      snr->flag = CLEARBIT(snr->flag, UNR_THRESH);
    }
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, LCR_THRESH, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, LCR_THRESH",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->lcr_thresh = fvalue;
    if (!(fvalue)) {
      snr->flag = CLEARBIT(snr->flag, LCR_THRESH);
    }
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, LNC_THRESH, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, LNC_THRESH",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->lnc_thresh = fvalue;
    if (!(fvalue)) {
      snr->flag = CLEARBIT(snr->flag, LNC_THRESH);
    }
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, LNR_THRESH, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, LNR_THRESH",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->lnr_thresh = fvalue;
    if (!(fvalue)) {
      snr->flag = CLEARBIT(snr->flag, LNR_THRESH);
    }
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, POS_HYST, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, POS_HYST",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->pos_hyst = fvalue;
  }

  if (get_sdr_thresh_val(fru, sdr, snr_num, NEG_HYST, &fvalue)) {
#ifdef DEBUG
    syslog(LOG_ERR,
        "get_sdr_thresh_val: failed for FRU: %d, num: 0x%X, %-16s, NEG_HYST",
        fru, snr_num, snr->name);
#endif
  } else {
    snr->neg_hyst = fvalue;
  }

  return 0;
}

int
sdr_get_snr_thresh(uint8_t fru, uint8_t snr_num, thresh_sensor_t *snr) {

  int ret = 0;
  sdr_full_t *sdr;
#ifdef DEBUG
  int cnt = 0;
#endif /* DEBUG */
  int retry = 0;
  char fpath[64] = {0};
  char initpath[64] = {0};
  char initflag[64] = {0};
  char fru_name[16];

  sensor_info_t sinfo[MAX_SENSOR_NUM + 1] = {0};

  ret = pal_sensor_sdr_init(fru, sinfo);

  while (ret == ERR_NOT_READY) {

    if (retry++ > MAX_RETRIES_SDR_INIT) {
      syslog(LOG_INFO, "sdr_get_snr_thresh: failed for fru: %d", fru);

      return ERR_NOT_READY;
    }
#ifdef DEBUG
    syslog(LOG_INFO, "sdr_get_snr_thresh: fru: %d, ret: %d cnt: %d", fru, ret, cnt++);
#endif /* DEBUG */
    msleep(50);
    ret = pal_sensor_sdr_init(fru, sinfo);
  }

  if ((ret < 0) || (pal_is_sdr_from_file(fru, snr_num) == false)) {
    sdr = NULL;
  } else {
    sdr = &sinfo[snr_num].sdr;
  }

  /* Set all the threshold options set in the flag */
  snr->flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH) |
    GETMASK(UNC_THRESH) | GETMASK(UNR_THRESH) | GETMASK(LCR_THRESH) |
    GETMASK(LNC_THRESH) | GETMASK(LNR_THRESH);

  ret = pal_get_fru_name(fru, fru_name);
  if (ret < 0) {
    printf("%s: Fail to get fru%d name\n", __func__, fru);
    return -1;
  }

  sprintf(initpath, INIT_THRESHOLD_BIN, fru_name);
  if (0 == access(initpath, F_OK)) { // init done
    sprintf(fpath, THRESHOLD_BIN, fru_name);
    if (0 == access(fpath, F_OK)) {
      ret = pal_get_thresh_from_file(fru, snr_num, snr);
      if (0 != ret) {
        syslog(LOG_WARNING, "%s: Fail to get threshold from file for slot%d", __func__, fru);
        return -1;
      }

      return ret;
    } 
  }

  if (sdr != NULL) {
    ret = _sdr_get_snr_thresh(fru, sdr, snr_num, snr);
    if (ret < 0) {
#ifdef DEBUG
      syslog(LOG_ERR, "_sdr_get_snr_thresh failed for FRU: %d snr_num: %d",
          fru, snr_num);
#endif
    }
  } else {

    ret = pal_get_sensor_name(fru, snr_num, snr->name);
    ret = pal_get_sensor_units(fru, snr_num, snr->units);
    ret = pal_get_sensor_poll_interval(fru, snr_num, &(snr->poll_interval));
    ret = pal_get_sensor_threshold(fru, snr_num, UCR_THRESH, &(snr->ucr_thresh));
    if (!(snr->ucr_thresh)) {
      snr->flag = CLEARBIT(snr->flag, UCR_THRESH);
    }
    ret = pal_get_sensor_threshold(fru, snr_num, UNC_THRESH, &(snr->unc_thresh));
    if (!(snr->unc_thresh)) {
      snr->flag = CLEARBIT(snr->flag, UNC_THRESH);
    }
    ret = pal_get_sensor_threshold(fru, snr_num, UNR_THRESH, &(snr->unr_thresh));
    if (!(snr->unr_thresh)) {
      snr->flag = CLEARBIT(snr->flag, UNR_THRESH);
    }
    ret = pal_get_sensor_threshold(fru, snr_num, LCR_THRESH, &(snr->lcr_thresh));
    if (!(snr->lcr_thresh)) {
      snr->flag = CLEARBIT(snr->flag, LCR_THRESH);
    }
    ret = pal_get_sensor_threshold(fru, snr_num, LNC_THRESH, &(snr->lnc_thresh));
    if (!(snr->lnc_thresh)) {
      snr->flag = CLEARBIT(snr->flag, LNC_THRESH);
    }
    ret = pal_get_sensor_threshold(fru, snr_num, LNR_THRESH, &(snr->lnr_thresh));
    if (!(snr->lnr_thresh)) {
      snr->flag = CLEARBIT(snr->flag, LNR_THRESH);
    }
    ret = pal_get_sensor_threshold(fru, snr_num, POS_HYST, &(snr->pos_hyst));
    ret = pal_get_sensor_threshold(fru, snr_num, NEG_HYST, &(snr->neg_hyst));

  }

  pal_sensor_threshold_flag(fru, snr_num, &(snr->flag));

  return ret;
}
