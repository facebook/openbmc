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

void sdr_ts_recent_add(time_stamp_t *ts);
void sdr_ts_recent_erase(time_stamp_t *ts);
int sdr_num_entries(void);
int sdr_free_space(void);
int sdr_rsv_id(int node);
int sdr_get_entry(int node, int rsv_id, int read_rec_id, sdr_rec_t *rec,
                       int *next_rec_id);
int sdr_init(void);

#endif /* __SDR_H__ */
