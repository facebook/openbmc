/*
 * fby2-sensors
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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <facebook/fby2_sensor.h>
#include <facebook/fby2_common.h>

int
sdr_init(char *path, sensor_info_t *sinfo) {
  int fd;
  int ret = 0;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;

  while (access(path, F_OK) == -1) {
    return -1;
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open failed for %s\n", __func__, path);
    return -1;
  }

  ret = flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: read returns %d bytes\n", __func__, bytes_rd);
      unflock_retry(fd);
      close(fd);
      return -1;
    }

    sdr = (sdr_full_t *) buf;
    snr_num = sdr->sensor_num;
    sinfo[snr_num].valid = true;
    memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
  }

  ret = unflock_retry(fd);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

struct remote_dev {
  char *path;
  uint8_t slot_id;
  uint8_t intf;
  uint8_t sensor_list[MAX_SENSOR_NUM];
  uint8_t sensor_cnt;
  char *desc;
} remote_snr_list[] = {
  {"/tmp/sdr_slot1.16.bin", 0x1, 0x10/*baseboard*/, {0x01}, 0x1, "baseboard"},
  {"/tmp/sdr_slot1.5.bin", 0x1, 0x05/*m2 exp*/, {0x7}, 0x1, "m2 expansion"},
};

static int remote_snr_size = sizeof(remote_snr_list) / sizeof(struct remote_dev);

int get_sdr_thresh_val(sdr_full_t *sdr, uint8_t thresh, void *value)
{
  int ret;
  uint8_t x;
  uint8_t m_lsb, m_msb;
  uint16_t m = 0;
  uint8_t b_lsb, b_msb;
  uint16_t b = 0;
  int8_t b_exp, r_exp;
  uint8_t thresh_val = thresh;

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

int
main(int argc, char **argv) {
#define FIELD_LEN(x)        (x & 0x1F)
  sensor_info_t sinfo[MAX_SENSOR_NUM];
  sdr_full_t *sdr;
  uint8_t slot_id;
  uint8_t intf;
  uint8_t sensor_num;
  ipmi_sensor_reading_t sensor;
  int field_len;
  int i, j;
  int ret;
  float ucr = 0;

  for ( i = 0; i < remote_snr_size; i++ ) {
    memset(sinfo, 0, sizeof(sinfo));
    //init sdr
    if ( sdr_init(remote_snr_list[i].path, &sinfo) < 0 ) {
      continue;
    }
    slot_id = remote_snr_list[i].slot_id;
    intf = remote_snr_list[i].intf;
    for ( j = 0; j < remote_snr_list[i].sensor_cnt; j++ ) {
      sensor_num = remote_snr_list[i].sensor_list[j];
      //init threshold
      get_sdr_thresh_val(&sinfo[sensor_num].sdr, sinfo[sensor_num].sdr.uc_thresh, (void *)&ucr);
      field_len = FIELD_LEN(sinfo[sensor_num].sdr.str_type_len) + 1;
      sinfo[sensor_num].sdr.str[field_len] = '\0';
      ret = bic_read_sensor_param(slot_id, sensor_num, &sensor, intf);
      if ( sensor.value != 0 ) {
        printf("%-28s (0x%X) : %7.2f C | UCR: %.2f. (%s)\n", sinfo[sensor_num].sdr.str, sensor_num, (float)sensor.value, ucr, remote_snr_list[i].desc);
      } else {
        printf("%-28s (0x%X) : NA | UCR: %.2f (%s)\n", sinfo[sensor_num].sdr.str, sensor_num, ucr, remote_snr_list[i].desc);
      }
    }
  }

  return 0;
}
