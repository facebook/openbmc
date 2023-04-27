/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This file represents platform specific implementation for storing
 * SEL logs and acts as back-end for IPMI stack
 *
 * TODO: Optimize the file handling to keep file open always instead of
 * current open/seek/close
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
#define _XOPEN_SOURCE
#include "sel.h"
#include "timestamp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <openbmc/pal.h>

// SEL File.
#define SEL_LOG_FILE  "/mnt/data/sel%d.bin"
#define SIZE_PATH_MAX 32

// SEL Header magic number
#define SEL_HDR_MAGIC 0xFBFBFBFB

// SEL Header version number
#define SEL_HDR_VERSION 0x01

// SEL Data offset from file beginning
#define SEL_DATA_OFFSET 0x100

// SEL reservation IDs can not be 0x00 or 0xFFFF
#define SEL_RSVID_MIN  0x01
#define SEL_RSVID_MAX  0xFFFE

// Number of SEL records before wrap
#define SEL_RECORDS_MAX 128 // TODO: Based on need we can make it bigger
#define SEL_ELEMS_MAX (SEL_RECORDS_MAX+1)

// Index for circular array
#define SEL_INDEX_MIN 0x00
#define SEL_INDEX_MAX SEL_RECORDS_MAX

// Record ID can not be 0x0 (IPMI/Section 31)
#define SEL_RECID_MIN (SEL_INDEX_MIN+1)
#define SEL_RECID_MAX (SEL_INDEX_MAX+1)

// Special RecID value for first and last (IPMI/Section 31)
#define SEL_RECID_FIRST 0x0000
#define SEL_RECID_LAST 0xFFFF

#define RAS_SEL_LENGTH 1024

// SEL header struct to keep track of SEL Log entries
typedef struct {
  int magic; // Magic number to check validity
  int version; // version number of this header
  int begin; // index to the beginning of the log
  int end; // index to end of the log
  time_stamp_t ts_add; // last addition time stamp
  time_stamp_t ts_erase; // last erase time stamp
} sel_hdr_t;

// Keep track of last Reservation ID
static int g_rsv_id[MAX_NODES+1];

// Cached version of SEL Header and data
static sel_hdr_t g_sel_hdr[MAX_NODES+1];
static sel_msg_t g_sel_data[MAX_NODES+1][SEL_ELEMS_MAX];

// Local helper functions to interact with file system
static int
file_get_sel_hdr(int node) {
  FILE *fp;
  char fpath[SIZE_PATH_MAX] = {0};

  sprintf(fpath, SEL_LOG_FILE, node);

  fp = fopen(fpath, "r");
  if (fp == NULL) {
    return -1;
  }

  if (fread(&g_sel_hdr[node], sizeof(sel_hdr_t), 1, fp) == 0) {
    syslog(LOG_WARNING, "file_get_sel_hdr: fread\n");
    fclose (fp);
    return -1;
  }

  fclose(fp);
  return 0;
}

static int
file_get_sel_data(int node) {
  FILE *fp;
  int i, j;
  char fpath[SIZE_PATH_MAX] = {0};

  sprintf(fpath, SEL_LOG_FILE, node);

  fp = fopen(fpath, "r");
  if (fp == NULL) {
    syslog(LOG_WARNING, "file_get_sel_data: fopen\n");
    return -1;
  }

  if (fseek(fp, SEL_DATA_OFFSET, SEEK_SET)) {
    syslog(LOG_WARNING, "file_get_sel_data: fseek\n");
    fclose(fp);
    return -1;
  }

  unsigned char buf[SEL_ELEMS_MAX * 16];
  if (fread(buf, 1, SEL_ELEMS_MAX * sizeof(sel_msg_t), fp) == 0) {
    syslog(LOG_WARNING, "file_get_sel_data: fread\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  for (i = 0; i < SEL_ELEMS_MAX; i++) {
    for (j = 0; j < sizeof(sel_msg_t);j++) {
      g_sel_data[node][i].msg[j] = buf[i*16 + j];
    }
  }

  return 0;
}

static int
file_store_sel_hdr(int node) {
  FILE *fp;
  char fpath[SIZE_PATH_MAX] = {0};

  sprintf(fpath, SEL_LOG_FILE, node);

  fp = fopen(fpath, "r+");
  if (fp == NULL) {
    syslog(LOG_WARNING, "file_store_sel_hdr: fopen\n");
    return -1;
  }

  if (fwrite(&g_sel_hdr[node], sizeof(sel_hdr_t), 1, fp) == 0) {
    syslog(LOG_WARNING, "file_store_sel_hdr: fwrite\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

static int
file_store_sel_data(int node, int recId, sel_msg_t *data) {
  FILE *fp;
  int index;
  char fpath[SIZE_PATH_MAX] = {0};

  sprintf(fpath, SEL_LOG_FILE, node);

  fp = fopen(fpath, "r+");
  if (fp == NULL) {
    syslog(LOG_WARNING, "file_store_sel_data: fopen\n");
    return -1;
  }

  // Records are stored using zero-based index
  index = (recId-1) * sizeof(sel_msg_t);

  if (fseek(fp, SEL_DATA_OFFSET+index, SEEK_SET)) {
    syslog(LOG_WARNING, "file_store_sel_data: fseek\n");
    fclose(fp);
    return -1;
  }

  if (fwrite(data->msg, sizeof(sel_msg_t), 1, fp) == 0) {
    syslog(LOG_WARNING, "file_store_sel_data: fwrite\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

static void
dump_sel_syslog(int fru, sel_msg_t *data) {
  int i = 0;
  char temp_str[8]  = {0};
  char str[128] = {0};

  for (i = 0; i < 15; i++) {
    sprintf(temp_str, "%02X:", data->msg[i]);
    strcat(str, temp_str);
  }
  sprintf(temp_str, "%02X", data->msg[15]);
  strcat(str, temp_str);

  syslog(LOG_WARNING, "SEL Entry, FRU: %d, Content: %s\n", fru, str);
}

static void
dump_ras_sel_syslog(uint8_t fru, ras_sel_msg_t *data) {
  int i = 0;
  char temp_str[8]  = {0};
  char str[256] = {0};

  for (i = 0; i < SIZE_RAS_SEL - 1; i++) {
    sprintf(temp_str, "%02X:", data->msg[i]);
    strcat(str, temp_str);
  }
  sprintf(temp_str, "%02X", data->msg[SIZE_RAS_SEL - 1]);
  strcat(str, temp_str);

  syslog(LOG_WARNING, "SEL Entry, FRU: %d, Content: %s\n", fru, str);
}

static void
parse_ras_sel(uint8_t fru, ras_sel_msg_t *data) {
  uint8_t *sel = data->msg;
  char error_log[RAS_SEL_LENGTH];
  char mfg_id[16];

  /* Manufacturer ID (byte 2:0) */
  sprintf(mfg_id, "%02x%02x%02x", sel[2], sel[1], sel[0]);

  /* Command-specific (byte 3:34) */
  pal_parse_ras_sel(fru, &sel[3], error_log);

  syslog(LOG_CRIT, "SEL Entry: FRU: %d, MFG ID: %s, "
         "%s",
         fru, mfg_id, error_log);

  pal_update_ts_sled();
}

static void
time_stamp_offset(unsigned char *ts, int sec) {
  unsigned int time;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  time = tv.tv_sec - sec;
  ts[0] = time & 0xFF;
  ts[1] = (time >> 8) & 0xFF;
  ts[2] = (time >> 16) & 0xFF;
  ts[3] = (time >> 24) & 0xFF;

  return;
}

/* Parse SEL Log based on IPMI v2.0 Section 32.1 & 32.2*/
static void
parse_sel(uint8_t fru, sel_msg_t *data) {
  uint32_t timestamp;
  uint8_t *sel = data->msg;
  uint8_t sensor_num;
  uint8_t general_info;
  uint8_t record_type;
  char sensor_name[32];
  char error_log[256];
  char error_type[64];
  char oem_data[32];
  int ret;
  struct tm ts;
  char time[64];
  char mfg_id[16];
  char event_data[8];

  if(pal_is_modify_sel_time(sel, RAS_SEL_LENGTH)) {
    time_stamp_offset(&data->msg[3], 5);
  }

  /* Record Type (Byte 2) */
  record_type = (uint8_t) sel[2];
  if (record_type == 0x02) {
    sprintf(error_type, "Standard");
  } else if (record_type >= 0xC0 && record_type <= 0xDF) {
    sprintf(error_type, "OEM timestamped");
  } else if (record_type == 0xFB) {
    sprintf(error_type, "Facebook Unified SEL");
  } else if (record_type >= 0xE0 && record_type <= 0xFF) {
    sprintf(error_type, "OEM non-timestamped");
  } else {
    sprintf(error_type, "Unknown");
  }

  if (record_type < 0xE0) {
    /* Timestamped SEL. including Standard and OEM. */

    /* Convert Timestamp from Unix time (Byte 6:3) to Human readable format */
    timestamp = 0;
    timestamp |= sel[3];
    timestamp |= sel[4] << 8;
    timestamp |= sel[5] << 16;
    timestamp |= sel[6] << 24;
    sprintf(time, "%u", timestamp);
    memset(&ts, 0, sizeof(struct tm));
    strptime(time, "%s", &ts);
    memset(&time, 0, sizeof(time));
    strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", &ts);
  }

  if (record_type < 0xC0) {
    /* standard SEL records*/

    /* Sensor num (Byte 11) */
    sensor_num = (uint8_t) sel[11];
    pal_get_event_sensor_name(fru, sel, sensor_name);

    /* Event Data (Byte 13:15) */
    ret = pal_parse_sel(fru, sel, error_log);
    sprintf(event_data, "%02X%02X%02X", sel[13], sel[14], sel[15]);

    /* Check if action needs to be taken based on the SEL message */
    if (!ret)
      ret = pal_sel_handler(fru, sensor_num, &sel[10]);

    syslog(LOG_CRIT, "SEL Entry: FRU: %d, Record: %s (0x%02X), Time: %s, "
        "Sensor: %s (0x%02X), Event Data: (%s) %s",
        fru,
        error_type, record_type,
        time,
        sensor_name, sensor_num,
        event_data, error_log);
  } else if (record_type < 0xE0) {
    /* timestamped OEM SEL records */

    pal_parse_oem_sel(fru, sel, error_log);

    /* Manufacturer ID (byte 9:7) */
    sprintf(mfg_id, "%02x%02x%02x", sel[9], sel[8], sel[7]);

    /* OEM Data (Byte 10:15) */
    sprintf(oem_data, "%02X%02X%02X%02X%02X%02X", sel[10], sel[11], sel[12],
          sel[13], sel[14], sel[15]);

    syslog(LOG_CRIT, "SEL Entry: FRU: %d, Record: %s (0x%02X), Time: %s, "
        "MFG ID: %s, OEM Data: (%s) %s",
        fru,
        error_type, record_type,
        time,
        mfg_id,
        oem_data, error_log);
  } else {
    /* non-timestamped OEM SEL records */
    //special case.
    if ( record_type == 0xFB )
    {
      time_stamp_fill(&sel[4]);
      pal_parse_oem_unified_sel(fru, sel, error_log);
      syslog(LOG_CRIT, "SEL Entry: FRU: %d, Record: %s (0x%02X), %s", fru, error_type, record_type, error_log);
      general_info = (uint8_t) sel[3];
      pal_oem_unified_sel_handler(fru, general_info, sel);
    }
    else
    {

      pal_parse_oem_sel(fru, sel, error_log);
      /* OEM Data (Byte 3:15) */
      sprintf(oem_data, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", sel[3], sel[4], sel[5],
          sel[6], sel[7], sel[8], sel[9], sel[10], sel[11], sel[12], sel[13], sel[14], sel[15]);

      syslog(LOG_CRIT, "SEL Entry: FRU: %d, Record: %s (0x%02X), "
          "OEM Data: (%s) %s",
          fru,
          error_type, record_type,
          oem_data, error_log);
    }
  }

  pal_update_ts_sled();
}

// Platform specific SEL API entry points
// Retrieve time stamp for recent add operation
void
sel_ts_recent_add(int node, time_stamp_t *ts) {
  memcpy(ts->ts, g_sel_hdr[node].ts_add.ts, 0x04);
}

// Retrieve time stamp for recent erase operation
void
sel_ts_recent_erase(int node, time_stamp_t *ts) {
  memcpy(ts->ts, g_sel_hdr[node].ts_erase.ts, 0x04);
}

// Retrieve total number of entries in SEL log
int
sel_num_entries(int node) {
  if (g_sel_hdr[node].begin <= g_sel_hdr[node].end) {
      return (g_sel_hdr[node].end - g_sel_hdr[node].begin);
  } else {
    return (g_sel_hdr[node].end + (SEL_INDEX_MAX - g_sel_hdr[node].begin + 1));
  }
}

// Retrieve total free space available in SEL log
int
sel_free_space(int node) {
  int total_space;
  int used_space;

  total_space = SEL_RECORDS_MAX * sizeof(sel_msg_t);
  used_space = sel_num_entries(node) * sizeof(sel_msg_t);

  return (total_space - used_space);
}

// Reserve an ID that will be used in later operations
// IPMI/Section 31.4
int
sel_rsv_id(int node) {
  // Increment the current reservation ID and return
  if (g_rsv_id[node]++ == SEL_RSVID_MAX) {
    g_rsv_id[node] = SEL_RSVID_MIN;
  }

  return g_rsv_id[node];
}

// Get the SEL entry for a given record ID
// IPMI/Section 31.5
int
sel_get_entry(int node, int read_rec_id, sel_msg_t *msg, int *next_rec_id) {

  int index;

  // Find the index in to array based on given index
  if (read_rec_id == SEL_RECID_FIRST) {
    index = g_sel_hdr[node].begin;
  } else if (read_rec_id == SEL_RECID_LAST) {
    if (g_sel_hdr[node].end) {
      index = g_sel_hdr[node].end - 1;
    } else {
      index = SEL_INDEX_MAX;
    }
  } else {
    index = read_rec_id;
  }

  // If the log is empty return error
  if (sel_num_entries(node) == 0) {
    syslog(LOG_WARNING, "sel_get_entry: No entries\n");
    return -1;
  }

  // Check for boundary conditions
  if ((index < SEL_INDEX_MIN) || (index > SEL_INDEX_MAX)) {
    syslog(LOG_WARNING, "sel_get_entry: Invalid Record ID %d\n", read_rec_id);
    return -1;
  }

  // If begin < end, check to make sure the given id falls between
  if (g_sel_hdr[node].begin < g_sel_hdr[node].end) {
    if (index < g_sel_hdr[node].begin || index >= g_sel_hdr[node].end) {
      syslog(LOG_WARNING, "sel_get_entry: Wrong Record ID %d\n", read_rec_id);
      return -1;
    }
  }

  // If end < begin, check to make sure the given id is valid
  if (g_sel_hdr[node].begin > g_sel_hdr[node].end) {
    if (index >= g_sel_hdr[node].end && index < g_sel_hdr[node].begin) {
      syslog(LOG_WARNING, "sel_get_entry: Wrong Record ID2 %d\n", read_rec_id);
      return -1;
    }
  }

  memcpy(msg->msg, g_sel_data[node][index].msg, sizeof(sel_msg_t));

  // Return the next record ID in the log
  *next_rec_id = ++read_rec_id;
  if (*next_rec_id > SEL_INDEX_MAX) {
    *next_rec_id = SEL_INDEX_MIN;
  }

  // If this is the last entry in the log, return 0xFFFF
  if (*next_rec_id == g_sel_hdr[node].end) {
    *next_rec_id = SEL_RECID_LAST;
  }

  return 0;
}

// Add a new entry in to SEL log for RAS SEL
int
ras_sel_add_entry(int node, ras_sel_msg_t *msg) {

  // Print the data in syslog
  dump_ras_sel_syslog(node, msg);

  // Parse the RAS SEL message
  parse_ras_sel(node, msg);

  return 0;
}

// Add a new entry in to SEL log
// IPMI/Section 31.6
int
sel_add_entry(int node, sel_msg_t *msg, int *rec_id) {
  // If the SEL if full, roll over. To keep track of empty condition, use
  // one empty location less than the max records.
  if (sel_num_entries(node) == SEL_RECORDS_MAX) {
      syslog(LOG_WARNING, "sel_add_entry: SEL rollover\n");
    if (++g_sel_hdr[node].begin > SEL_INDEX_MAX) {
      g_sel_hdr[node].begin = SEL_INDEX_MIN;
    }
  }

  msg->msg[0] = g_sel_hdr[node].end & 0xFF;
  msg->msg[1] = (g_sel_hdr[node].end >> 8) & 0xFF;

  // Update message's time stamp starting at byte 4
  if (msg->msg[2] < 0xE0)
    time_stamp_fill(&msg->msg[3]);

  // Add the enry at end
  memcpy(g_sel_data[node][g_sel_hdr[node].end].msg, msg->msg, sizeof(sel_msg_t));

  // Return the newly added record ID
  *rec_id = g_sel_hdr[node].end+1;

  // Print the data in syslog
  dump_sel_syslog(node, msg);

  // Parse the SEL message
  parse_sel((uint8_t) node, msg);

  if (file_store_sel_data(node, *rec_id, msg)) {
    syslog(LOG_WARNING, "sel_add_entry: file_store_sel_data\n");
    return -1;
  }

  // Increment the end pointer
  if (++g_sel_hdr[node].end > SEL_INDEX_MAX) {
    g_sel_hdr[node].end = SEL_INDEX_MIN;
  }

  // Update timestamp for add in header
  time_stamp_fill(g_sel_hdr[node].ts_add.ts);

  // Store the structure persistently
  if (file_store_sel_hdr(node)) {
    syslog(LOG_WARNING, "sel_add_entry: file_store_sel_hdr\n");
    return -1;
  }

  return 0;
}

// Erase the SEL completely
// IPMI/Section 31.9
// Note: To reduce wear/tear, instead of erasing, manipulating the metadata
int
sel_erase(int node, int rsv_id) {
  if (rsv_id != g_rsv_id[node]) {
    return -1;
  }

  // Erase SEL Logs
  g_sel_hdr[node].begin = SEL_INDEX_MIN;
  g_sel_hdr[node].end = SEL_INDEX_MIN;

  // Update timestamp for erase in header
  time_stamp_fill(g_sel_hdr[node].ts_erase.ts);

  // Store the structure persistently
  if (file_store_sel_hdr(node)) {
    syslog(LOG_WARNING, "sel_erase: file_store_sel_hdr\n");
    return -1;
  }

  return 0;
}

// To get the erase status while erase happens
// IPMI/Section 31.2
// Note: Since we are not doing offline erasing, need not return in-progress state
int
sel_erase_status(int node, int rsv_id, sel_erase_stat_t *status) {
  if (rsv_id != g_rsv_id[node]) {
    return -1;
  }

  // Since we do not do any offline erasing, always return erase done
  *status = SEL_ERASE_DONE;

  return 0;
}

// Initialize SEL log file
static int
sel_node_init(int node) {
  FILE *fp;
  int i;
  char fpath[SIZE_PATH_MAX] = {0};

  sprintf(fpath, SEL_LOG_FILE, node);

  // Check if the file exists or not
  if (access(fpath, F_OK) == 0) {
    // Since file is present, fetch all the contents to cache
    if (file_get_sel_hdr(node)) {
      syslog(LOG_WARNING, "init_sel: file_get_sel_hdr\n");
      return -1;
    }

    if (file_get_sel_data(node)) {
      syslog(LOG_WARNING, "init_sel: file_get_sel_data\n");
      return -1;
    }

    return 0;
  }

  // File not present, so create the file
  fp = fopen(fpath, "w+");
  if (fp == NULL) {
    syslog(LOG_WARNING, "init_sel: fopen\n");
    return -1;
  }

  fclose (fp);

  // Populate SEL Header in to the file
  g_sel_hdr[node].magic = SEL_HDR_MAGIC;
  g_sel_hdr[node].version = SEL_HDR_VERSION;
  g_sel_hdr[node].begin = SEL_INDEX_MIN;
  g_sel_hdr[node].end = SEL_INDEX_MIN;
  memset(g_sel_hdr[node].ts_add.ts, 0x0, 4);
  memset(g_sel_hdr[node].ts_erase.ts, 0x0, 4);

  if (file_store_sel_hdr(node)) {
    syslog(LOG_WARNING, "init_sel: file_store_sel_hdr\n");
    return -1;
  }

  // Populate SEL Data in to the file
  for (i = 1;  i <= SEL_RECORDS_MAX; i++) {
    sel_msg_t msg = {0};
    if (file_store_sel_data(node, i, &msg)) {
      syslog(LOG_WARNING, "init_sel: file_store_sel_data\n");
      return -1;
    }
  }

  g_rsv_id[node] = 0x01;

  return 0;
}

int
sel_init(void) {
  int ret;
  int i;

  for (i = 1; i < MAX_NODES+1; i++) {
    ret = sel_node_init(i);
    if (ret) {
      return ret;
    }
  }

  return ret;
}

