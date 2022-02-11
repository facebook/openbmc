/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This file represents platform specific implementation for storing
 * SDR record entries and acts as back-end for IPMI stack
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
#include "sdr.h"
#include "sensor.h"
#include "timestamp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

// SDR Header magic number
#define SDR_HDR_MAGIC 0xFBFBFBFB

// SDR Header version number
#define SDR_HDR_VERSION 0x01

// SDR reservation IDs can not be 0x00 or 0xFFFF
#define SDR_RSVID_MIN  0x01
#define SDR_RSVID_MAX  0xFFFE

#define SDR_RECORDS_MAX 64 // to support around 64 sensors

// SDR index to keep track
#define SDR_INDEX_MIN 0
#define SDR_INDEX_MAX (SDR_RECORDS_MAX - 1)

// Record ID can not be 0x0 (IPMI/Section 31)
#define SDR_RECID_MIN 1
#define SDR_RECID_MAX SDR_RECORDS_MAX

// Special RecID value for first and last (IPMI/Section 31)
#define SDR_RECID_FIRST 0x0000
#define SDR_RECID_LAST 0xFFFF

#define SDR_VERSION 0x51
#define SDR_LEN_MAX 64

#define SDR_FULL_TYPE 0x01
#define SDR_MGMT_TYPE 0x12
#define SDR_OEM_TYPE 0xC0

#define SDR_FULL_LEN 64
#define SDR_MGMT_LEN 32
#define SDR_OEM_LEN 64

// SDR header struct to keep track of SEL Log entries
typedef struct {
  int magic; // Magic number to check validity
  int version; // version number of this header
  int begin; // index to the first SDR entry
  int end; // index to the last SDR entry
  time_stamp_t ts_add; // last addition time stamp
  time_stamp_t ts_erase; // last erase time stamp
} sdr_hdr_t;

// Keep track of last Reservation ID
static int g_rsv_id = 0x01;

// SDR Header and data global structures
static sdr_hdr_t g_sdr_hdr;
static sdr_rec_t g_sdr_data[SDR_RECORDS_MAX];

// Add a new SDR entry
static int
plat_sdr_add_entry(sdr_rec_t *rec, int *rec_id) {
  // If SDR is full, return error
  if (plat_sdr_num_entries() == SDR_RECORDS_MAX) {
      syslog(LOG_ALERT, "plat_sdr_add_entry: SDR full\n");
      return -1;
  }

  // Add Record ID which is array index + 1
  rec->rec[0] = g_sdr_hdr.end+1;

  // Add the enry at end
  memcpy(g_sdr_data[g_sdr_hdr.end].rec, rec->rec, sizeof(sdr_rec_t));

  // Return the newly added record ID
  *rec_id = g_sdr_hdr.end+1;

  // Increment the end pointer
  ++g_sdr_hdr.end;

  // Update timestamp for add in header
  time_stamp_fill(g_sdr_hdr.ts_add.ts);

  return 0;
}

static int
sdr_add_mgmt_rec(sensor_mgmt_t *p_rec) {
  int rec_id = 0;
  sdr_rec_t sdr = { 0 };
  sdr_mgmt_t rec = { 0 };

  // Populate SDR MGMT record
  rec.ver = SDR_VERSION;
  rec.type = SDR_MGMT_TYPE;
  rec.len = SDR_MGMT_LEN;

  rec.slave_addr = p_rec->slave_addr;
  rec.chan_no = p_rec->chan_no;

  rec.pwr_state_init = p_rec->pwr_state_init;
  rec.dev_caps = p_rec->dev_caps;
  rec.ent_id = p_rec->ent_id;
  rec.ent_inst = p_rec->ent_inst;
  rec.oem = p_rec->oem;
  rec.str_type_len = p_rec->str_type_len;
  memcpy(rec.str, p_rec->str, SENSOR_STR_SIZE);

  // Copy this record to generic SDR record
  memcpy(sdr.rec, &rec, SDR_LEN_MAX);

  // Add this record to SDR repo
  if (plat_sdr_add_entry(&sdr, &rec_id)) {
    syslog(LOG_ALERT, "sdr_add_mgmt_rec: plat_sdr_add_entry failed\n");
    return -1;
  }

  return 0;
}

static int
sdr_add_disc_rec(sensor_disc_t *p_rec) {
  int rec_id = 0;
  sdr_rec_t sdr = { 0 };
  sdr_full_t rec = { 0 };

  // Populate SDR FULL record
  rec.ver = SDR_VERSION;
  rec.type = SDR_FULL_TYPE;
  rec.len = SDR_FULL_LEN;

  rec.owner = p_rec->owner;
  rec.lun = p_rec->lun;

  rec.ent_id = p_rec->ent_id;
  rec.ent_inst = p_rec->ent_inst;
  rec.sensor_init = p_rec->sensor_init;
  rec.sensor_caps = p_rec->sensor_caps;
  rec.sensor_type = p_rec->sensor_type;
  rec.evt_read_type = p_rec->evt_read_type;
  memcpy(rec.assert_evt_mask, p_rec->assert_evt_mask, 2);
  memcpy(rec.deassert_evt_mask, p_rec->deassert_evt_mask, 2);
  memcpy(rec.read_evt_mask, p_rec->read_evt_mask, 2);
  rec.oem = p_rec->oem;
  rec.str_type_len = p_rec->str_type_len;
  memcpy(rec.str, p_rec->str, SENSOR_STR_SIZE);

  // Copy this record to generic SDR record
  memcpy(sdr.rec, &rec, SDR_LEN_MAX);

  // Add this record to SDR repo
  if (plat_sdr_add_entry(&sdr, &rec_id)) {
    syslog(LOG_ALERT, "sdr_add_disc_rec: plat_sdr_add_entry failed\n");
    return -1;
  }

  return 0;
}

static int
sdr_add_thresh_rec(sensor_thresh_t *p_rec) {
  int rec_id = 0;
  sdr_rec_t sdr = { 0 };
  sdr_full_t rec = { 0 };

  // Populate SDR FULL record
  rec.ver = SDR_VERSION;
  rec.type = SDR_FULL_TYPE;
  rec.len = SDR_FULL_LEN;

  rec.owner = p_rec->owner;
  rec.lun = p_rec->lun;

  rec.ent_id = p_rec->ent_id;
  rec.ent_inst = p_rec->ent_inst;
  rec.sensor_init = p_rec->sensor_init;
  rec.sensor_caps = p_rec->sensor_caps;
  rec.sensor_type = p_rec->sensor_type;
  rec.evt_read_type = p_rec->evt_read_type;
  memcpy(rec.lt_read_mask, p_rec->lt_read_mask, 2);
  memcpy(rec.ut_read_mask, p_rec->ut_read_mask, 2);
  memcpy(rec.set_thresh_mask, p_rec->set_thresh_mask, 2);
  rec.sensor_units1 = p_rec->sensor_units1;
  rec.sensor_units2 = p_rec->sensor_units2;
  rec.sensor_units3 = p_rec->sensor_units3;
  rec.linear = p_rec->linear;
  rec.m_val = p_rec->m_val;
  rec.m_tolerance = p_rec->m_tolerance;
  rec.b_val = p_rec->b_val;
  rec.b_accuracy = p_rec->b_accuracy;
  rec.analog_flags = p_rec->analog_flags;
  rec.nominal = p_rec->nominal;
  rec.normal_max = p_rec->normal_max;
  rec.normal_min = p_rec->normal_min;
  rec.max_reading = p_rec->max_reading;
  rec.min_reading = p_rec->min_reading;
  rec.unr_thresh = p_rec->unr_thresh;
  rec.uc_thresh = p_rec->uc_thresh;
  rec.unc_thresh = p_rec->unc_thresh;
  rec.lnr_thresh = p_rec->lnr_thresh;
  rec.lc_thresh = p_rec->lc_thresh;
  rec.lnc_thresh = p_rec->lnc_thresh;
  rec.pos_hyst = p_rec->pos_hyst;
  rec.neg_hyst = p_rec->neg_hyst;
  rec.oem = p_rec->oem;
  rec.str_type_len = p_rec->str_type_len;
  memcpy(rec.str, p_rec->str, SENSOR_STR_SIZE);

  // Copy this record to generic SDR record
  memcpy(sdr.rec, &rec, SDR_LEN_MAX);

  // Add this record to SDR repo
  if (plat_sdr_add_entry(&sdr, &rec_id)) {
    syslog(LOG_ALERT, "sdr_add_thresh_rec: plat_sdr_add_entry failed\n");
    return -1;
  }

  return 0;
}

static int
sdr_add_oem_rec(sensor_oem_t *p_rec) {
  int rec_id = 0;
  sdr_rec_t sdr = { 0 };
  sdr_oem_t rec = { 0 };

  // Populate SDR OEM record
  rec.ver = SDR_VERSION;
  rec.type = SDR_OEM_TYPE;
  rec.len = SDR_OEM_LEN;

  memcpy(rec.mfr_id, p_rec->mfr_id, 3);
  memcpy(rec.oem_data, p_rec->oem_data, SENSOR_OEM_DATA_SIZE);

  // Copy this record to generic SDR record
  memcpy(sdr.rec, &rec, SDR_LEN_MAX);

  // Add this record to SDR repo
  if (plat_sdr_add_entry(&sdr, &rec_id)) {
    syslog(LOG_ALERT, "sdr_add_oem_rec: plat_sdr_add_entry failed\n");
    return -1;
  }

  return 0;
}

// Platform specific SEL API entry points
// Retrieve time stamp for recent add operation
void
plat_sdr_ts_recent_add(time_stamp_t *ts) {
  memcpy(ts->ts, g_sdr_hdr.ts_add.ts, 0x04);
}

// Retrieve time stamp for recent erase operation
void
plat_sdr_ts_recent_erase(time_stamp_t *ts) {
  memcpy(ts->ts, g_sdr_hdr.ts_erase.ts, 0x04);
}

// Retrieve total number of entries in SDR repo
int
plat_sdr_num_entries(void) {
    return (g_sdr_hdr.end - g_sdr_hdr.begin);
}

// Retrieve total free space available in SDR repo
int
plat_sdr_free_space(void) {
  int total_space;
  int used_space;

  total_space = SDR_RECORDS_MAX * sizeof(sdr_rec_t);
  used_space = plat_sdr_num_entries() * sizeof(sdr_rec_t);

  return (total_space - used_space);
}

// Reserve an ID that will be used in later operations
// IPMI/Section 33.11
int
plat_sdr_rsv_id() {
  // Increment the current reservation ID and return
  if (g_rsv_id++ == SDR_RSVID_MAX) {
    g_rsv_id = SDR_RSVID_MIN;
  }

  return g_rsv_id;
}

// Get the SDR entry for a given record ID
// IPMI/Section 33.12
int
plat_sdr_get_entry(int rsv_id, int read_rec_id, sdr_rec_t *rec,
                   int *next_rec_id) {

  int index;

  // Make sure the rsv_id matches
  if (rsv_id != g_rsv_id) {
    syslog(LOG_ALERT, "plat_sdr_get_entry: Reservation ID mismatch\n");
    return -1;
  }

  // Find the index in to array based on given index
  if (read_rec_id == SDR_RECID_FIRST) {
    index = g_sdr_hdr.begin;
  } else if (read_rec_id == SDR_RECID_LAST) {
    index = g_sdr_hdr.end - 1;
  } else {
    index = read_rec_id - 1;
  }

  // If the SDR repo is empty return error
  if (plat_sdr_num_entries() == 0) {
    syslog(LOG_ALERT, "plat_sdr_get_entry: No entries\n");
    return -1;
  }

  // Check for boundary conditions
  if ((index < SDR_INDEX_MIN) || (index > SDR_INDEX_MAX)) {
    syslog(LOG_ALERT, "plat_sdr_get_entry: Invalid Record ID %d\n", read_rec_id);
    return -1;
  }

  // Check to make sure the given id is valid
  if (index < g_sdr_hdr.begin || index >= g_sdr_hdr.end) {
    syslog(LOG_ALERT, "plat_sdr_get_entry: Wrong Record ID %d\n", read_rec_id);
    return -1;
  }

  memcpy(rec->rec, g_sdr_data[index].rec, sizeof(sdr_rec_t));

  // Return the next record ID in the log
  *next_rec_id = ++read_rec_id;

  // If this is the last entry in the log, return 0xFFFF
  if (*next_rec_id == g_sdr_hdr.end) {
    *next_rec_id = SDR_RECID_LAST;
  }

  return 0;
}


// Initialize SDR Repo structure
int
plat_sdr_init(void) {
  int num;
  sensor_mgmt_t *p_mgmt;
  sensor_thresh_t *p_thresh;
  sensor_disc_t *p_disc;
  sensor_oem_t *p_oem;

  // Populate SDR Header
  g_sdr_hdr.magic = SDR_HDR_MAGIC;
  g_sdr_hdr.version = SDR_HDR_VERSION;
  g_sdr_hdr.begin = SDR_INDEX_MIN;
  g_sdr_hdr.end = SDR_INDEX_MIN;
  memset(g_sdr_hdr.ts_add.ts, 0x0, 4);
  memset(g_sdr_hdr.ts_erase.ts, 0x0, 4);

  // Populate all mgmt control sensors
  plat_sensor_mgmt_info(&num, &p_mgmt);
  for (int i = 0; i < num; i++) {
    sdr_add_mgmt_rec(&p_mgmt[i]);
  }

  // Populate all discrete sensors
  plat_sensor_disc_info(&num, &p_disc);
  for (int i = 0; i < num; i++) {
    sdr_add_disc_rec(&p_disc[i]);
  }

  // Populate all threshold sensors
  plat_sensor_thresh_info(&num, &p_thresh);
  for (int i = 0; i < num; i++) {
    sdr_add_thresh_rec(&p_thresh[i]);
  }

  // Populate all OEM sensors
  plat_sensor_oem_info(&num, &p_oem);
  for (int i = 0; i < num; i++) {
    sdr_add_oem_rec(&p_oem[i]);
  }

  return 0;
}
