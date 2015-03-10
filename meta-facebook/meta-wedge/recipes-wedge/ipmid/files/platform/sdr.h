/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
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

#ifndef __SDR_H__
#define __SDR_H__

#include "timestamp.h"

typedef struct {
  unsigned char rec[64];
} sdr_rec_t;

// Full Sensor SDR record; IPMI/Section 43.1
typedef struct {
  // Sensor Record Header
  unsigned char rec_id[2];
  unsigned char ver;
  unsigned char type;
  unsigned char len;
  // Record Key Bytes
  unsigned char owner;
  unsigned char lun;
  unsigned char sensor_num;
  // Record Body Bytes
  unsigned char ent_id;
  unsigned char ent_inst;
  unsigned char sensor_init;
  unsigned char sensor_caps;
  unsigned char sensor_type;
  unsigned char evt_read_type;
  union {
    unsigned char assert_evt_mask[2];
    unsigned char lt_read_mask[2];
  };
  union {
    unsigned char deassert_evt_mask[2];
    unsigned char ut_read_mask[2];
  };
  union {
    unsigned char read_evt_mask[2];
    unsigned char set_thresh_mask[2];
  };
  unsigned char sensor_units1;
  unsigned char sensor_units2;
  unsigned char sensor_units3;
  unsigned char linear;
  unsigned char m_val;
  unsigned char m_tolerance;
  unsigned char b_val;
  unsigned char b_accuracy;
  unsigned char accuracy_dir;
  unsigned char rb_exp;
  unsigned char analog_flags;
  unsigned char nominal;
  unsigned char normal_max;
  unsigned char normal_min;
  unsigned char max_reading;
  unsigned char min_reading;
  unsigned char unr_thresh;
  unsigned char uc_thresh;
  unsigned char unc_thresh;
  unsigned char lnr_thresh;
  unsigned char lc_thresh;
  unsigned char lnc_thresh;
  unsigned char pos_hyst;
  unsigned char neg_hyst;
  unsigned char rsvd[2];
  unsigned char oem;
  unsigned char str_type_len;
  char str[16];
} sdr_full_t;

// Mgmt. Controller SDR record; IPMI/ Section 43.9
typedef struct {
  // Sensor Record Header
  unsigned char rec_id[2];
  unsigned char ver;
  unsigned char type;
  unsigned char len;
  // Record Key Bytes
  unsigned char slave_addr;
  unsigned char chan_no;
  // Record Body Bytes
  unsigned char pwr_state_init;
  unsigned char dev_caps;
  unsigned char rsvd[3];
  unsigned char ent_id;
  unsigned char ent_inst;
  unsigned char oem;
  unsigned char str_type_len;
  char str[16];
} sdr_mgmt_t;

// OEM type SDR record; IPMI/Section 43.12
typedef struct {
  // Sensor Record Header
  unsigned char rec_id[2];
  unsigned char ver;
  unsigned char type;
  unsigned char len;
  // Record Body Bytes
  unsigned char mfr_id[3];
  unsigned char oem_data[56];
} sdr_oem_t;

void plat_sdr_ts_recent_add(time_stamp_t *ts);
void plat_sdr_ts_recent_erase(time_stamp_t *ts);
int plat_sdr_num_entries(void);
int plat_sdr_free_space(void);
int plat_sdr_rsv_id();
int plat_sdr_get_entry(int rsv_id, int read_rec_id, sdr_rec_t *rec,
                       int *next_rec_id);
int plat_sdr_init(void);

#endif /* __SDR_H__ */
