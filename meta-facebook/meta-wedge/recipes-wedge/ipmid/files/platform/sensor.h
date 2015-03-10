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

#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "timestamp.h"

#define IANA_ID_SIZE 3
#define SENSOR_STR_SIZE 16
#define SENSOR_OEM_DATA_SIZE 56

// Threshold Sensor Descriptor
typedef struct {
  unsigned char owner;
  unsigned char lun;
  unsigned char sensor_num;
  unsigned char ent_id;
  unsigned char ent_inst;
  unsigned char sensor_init;
  unsigned char sensor_caps;
  unsigned char sensor_type;
  unsigned char evt_read_type;
  unsigned char lt_read_mask[2];
  unsigned char ut_read_mask[2];
  unsigned char set_thresh_mask[2];
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
  unsigned char oem;
  unsigned char str_type_len;
  char str[SENSOR_STR_SIZE];
} sensor_thresh_t;

// Discrete Sensor Descriptor
typedef struct {
  unsigned char owner;
  unsigned char lun;
  unsigned char sensor_num;
  unsigned char ent_id;
  unsigned char ent_inst;
  unsigned char sensor_init;
  unsigned char sensor_caps;
  unsigned char sensor_type;
  unsigned char evt_read_type;
  unsigned char assert_evt_mask[2];
  unsigned char deassert_evt_mask[2];
  unsigned char read_evt_mask[2];
  unsigned char oem;
  unsigned char str_type_len;
  char str[SENSOR_STR_SIZE];
} sensor_disc_t;

// Mgmt. Controller Sensor Descriptor
typedef struct {
  unsigned char slave_addr;
  unsigned char chan_no;
  unsigned char pwr_state_init;
  unsigned char dev_caps;
  unsigned char ent_id;
  unsigned char ent_inst;
  unsigned char oem;
  unsigned char str_type_len;
  char str[SENSOR_STR_SIZE];
} sensor_mgmt_t;

// OEM type Sensor Descriptor
typedef struct {
  unsigned char mfr_id[IANA_ID_SIZE];
  unsigned char oem_data[SENSOR_OEM_DATA_SIZE];
} sensor_oem_t;

void plat_sensor_mgmt_info(int *num, sensor_mgmt_t **p_sensor);
void plat_sensor_disc_info(int *num, sensor_disc_t **p_sensor);
void plat_sensor_thresh_info(int *num, sensor_thresh_t **p_sensor);
void plat_sensor_oem_info(int *num, sensor_oem_t **p_sensor);
int plat_sensor_init(void);

#endif /* __SENSOR_H__ */
