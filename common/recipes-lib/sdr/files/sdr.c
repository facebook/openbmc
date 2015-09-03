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
#include "sdr.h"

#define FIELD_RATE_UNIT(x)  ((x & (0x07 << 3)) >> 3)
#define FIELD_OP(x)         ((x & (0x03 << 1)) >> 1)
#define FIELD_PERCENTAGE(x) (x & 0x01)

#define FIELD_TYPE(x)       ((x & (0x03 << 6)) >> 6)
#define FIELD_LEN(x)        (x & 0xF)

/* Array for BCD Plus definition. */
const char bcd_plus_array[] = "0123456789 -.XXX";

/* Array for 6-Bit ASCII definition. */
const char * ascii_6bit[4] = {
  " !\"#$%&'()*+,-./",
  "0123456789:;<=>?",
  "@ABCDEFGHIJKLMNO",
  "PQRSTUVWXYZ[\\]^_"
};

/* Get the units of the sensor from the SDR */
int
sdr_get_sensor_units(sdr_full_t *sdr, uint8_t *op, uint8_t *modifier,
    char *units) {

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
    sprintf(units, "%");
  } else {
    if (base_idx > 0 && base_idx <= MAX_SENSOR_BASE_UNIT) {
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


/* Get the name of the sensor from the SDR */
int
sdr_get_sensor_name(sdr_full_t *sdr, char *name) {
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
    syslog(LOG_ALERT, "get_sensor_name: str length is 0\n");
    // TODO: Fix this hack later
    sprintf(name, "%s", str);
    return -1;
  }

  /* Retrieve field data depending on the type it was stored. */
  switch (field_type) {
    case TYPE_BINARY:
      /* TODO: Need to add support to read data stored in binary type. */
      break;

    case TYPE_BCD_PLUS:

      idx = 0;
      while (idx != field_len) {
        name[idx] = bcd_plus_array[str[idx] & 0x0F];
        idx++;
      }
      name[idx] = '\0';
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
    snprintf(name, field_len, str);
    /* Add Null terminator */
    name[field_len] = '\0';
    break;
  }

  return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
int
sdr_init(char *path, sensor_info_t *sinfo) {
  int fd;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;

  while (access(path, F_OK) == -1) {
    sleep(5);
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "sdr_init: open failed for %s\n", path);
    return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "sdr_init: read returns %d bytes\n", bytes_rd);
      return -1;
    }

    sdr = (sdr_full_t *) buf;
    snr_num = sdr->sensor_num;
    sinfo[snr_num].valid = true;
    memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
  }

  return 0;
}

