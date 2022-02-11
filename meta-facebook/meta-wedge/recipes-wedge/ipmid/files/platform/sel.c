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
#include "sel.h"
#include "timestamp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

// SEL File.
#define SEL_LOG_FILE  "/mnt/data/sel.bin"

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
static int g_rsv_id = 0x01;

// Cached version of SEL Header and data
static sel_hdr_t g_sel_hdr;
static sel_msg_t g_sel_data[SEL_ELEMS_MAX];

// Local helper functions to interact with file system
static int
file_get_sel_hdr(void) {
  FILE *fp;

  fp = fopen(SEL_LOG_FILE, "r");
  if (fp == NULL) {
    return -1;
  }

  if (fread(&g_sel_hdr, sizeof(sel_hdr_t), 1, fp) <= 0) {
    syslog(LOG_ALERT, "file_get_sel_hdr: fread\n");
    fclose (fp);
    return -1;
  }

  fclose(fp);
  return 0;
}

static int
file_get_sel_data(void) {
  FILE *fp;

  fp = fopen(SEL_LOG_FILE, "r");
  if (fp == NULL) {
    syslog(LOG_ALERT, "file_get_sel_data: fopen\n");
    return -1;
  }

  if (fseek(fp, SEL_DATA_OFFSET, SEEK_SET)) {
    syslog(LOG_ALERT, "file_get_sel_data: fseek\n");
    fclose(fp);
    return -1;
  }

  unsigned char buf[SEL_ELEMS_MAX * 16];
  if (fread(buf, 1, SEL_ELEMS_MAX * sizeof(sel_msg_t), fp) <= 0) {
    syslog(LOG_ALERT, "file_get_sel_data: fread\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  for (int i = 0; i < SEL_ELEMS_MAX; i++) {
    for (int j = 0; j < sizeof(sel_msg_t);j++) {
      g_sel_data[i].msg[j] = buf[i*16 + j];
    }
  }

  return 0;
}

static int
file_store_sel_hdr(void) {
  FILE *fp;

  fp = fopen(SEL_LOG_FILE, "r+");
  if (fp == NULL) {
    syslog(LOG_ALERT, "file_store_sel_hdr: fopen\n");
    return -1;
  }

  if (fwrite(&g_sel_hdr, sizeof(sel_hdr_t), 1, fp) <= 0) {
    syslog(LOG_ALERT, "file_store_sel_hdr: fwrite\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

static int
file_store_sel_data(int recId, sel_msg_t *data) {
  FILE *fp;
  int index;

  fp = fopen(SEL_LOG_FILE, "r+");
  if (fp == NULL) {
    syslog(LOG_ALERT, "file_store_sel_data: fopen\n");
    return -1;
  }

  // Records are stored using zero-based index
  index = (recId-1) * sizeof(sel_msg_t);

  if (fseek(fp, SEL_DATA_OFFSET+index, SEEK_SET)) {
    syslog(LOG_ALERT, "file_store_sel_data: fseek\n");
    fclose(fp);
    return -1;
  }

  if (fwrite(data->msg, sizeof(sel_msg_t), 1, fp) <= 0) {
    syslog(LOG_ALERT, "file_store_sel_data: fwrite\n");
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

// Platform specific SEL API entry points
// Retrieve time stamp for recent add operation
void
plat_sel_ts_recent_add(time_stamp_t *ts) {
  memcpy(ts->ts, g_sel_hdr.ts_add.ts, 0x04);
}

// Retrieve time stamp for recent erase operation
void
plat_sel_ts_recent_erase(time_stamp_t *ts) {
  memcpy(ts->ts, g_sel_hdr.ts_erase.ts, 0x04);
}

// Retrieve total number of entries in SEL log
int
plat_sel_num_entries(void) {
  if (g_sel_hdr.begin <= g_sel_hdr.end) {
      return (g_sel_hdr.end - g_sel_hdr.begin);
  } else {
    return (g_sel_hdr.end + (SEL_INDEX_MAX - g_sel_hdr.begin + 1));
  }
}

// Retrieve total free space available in SEL log
int
plat_sel_free_space(void) {
  int total_space;
  int used_space;

  total_space = SEL_RECORDS_MAX * sizeof(sel_msg_t);
  used_space = plat_sel_num_entries() * sizeof(sel_msg_t);

  return (total_space - used_space);
}

// Reserve an ID that will be used in later operations
// IPMI/Section 31.4
int
plat_sel_rsv_id() {
  // Increment the current reservation ID and return
  if (g_rsv_id++ == SEL_RSVID_MAX) {
    g_rsv_id = SEL_RSVID_MIN;
  }

  return g_rsv_id;
}

// Get the SEL entry for a given record ID
// IPMI/Section 31.5
int
plat_sel_get_entry(int read_rec_id, sel_msg_t *msg, int *next_rec_id) {

  int index;

  // Find the index in to array based on given index
  if (read_rec_id == SEL_RECID_FIRST) {
    index = g_sel_hdr.begin;
  } else if (read_rec_id == SEL_RECID_LAST) {
    if (g_sel_hdr.end) {
      index = g_sel_hdr.end - 1;
    } else {
      index = SEL_INDEX_MAX;
    }
  } else {
    index = read_rec_id - 1;
  }

  // If the log is empty return error
  if (plat_sel_num_entries() == 0) {
    syslog(LOG_ALERT, "plat_sel_get_entry: No entries\n");
    return -1;
  }

  // Check for boundary conditions
  if ((index < SEL_INDEX_MIN) || (index > SEL_INDEX_MAX)) {
    syslog(LOG_ALERT, "plat_sel_get_entry: Invalid Record ID %d\n", read_rec_id);
    return -1;
  }

  // If begin < end, check to make sure the given id falls between
  if (g_sel_hdr.begin < g_sel_hdr.end) {
    if (index < g_sel_hdr.begin || index >= g_sel_hdr.end) {
      syslog(LOG_ALERT, "plat_sel_get_entry: Wrong Record ID %d\n", read_rec_id);
      return -1;
    }
  }

  // If end < begin, check to make sure the given id is valid
  if (g_sel_hdr.begin > g_sel_hdr.end) {
    if (index >= g_sel_hdr.end && index < g_sel_hdr.begin) {
      syslog(LOG_ALERT, "plat_sel_get_entry: Wrong Record ID2 %d\n", read_rec_id);
      return -1;
    }
  }

  memcpy(msg->msg, g_sel_data[index].msg, sizeof(sel_msg_t));

  // Return the next record ID in the log
  *next_rec_id = read_rec_id++;
  if (*next_rec_id > SEL_INDEX_MAX) {
    *next_rec_id = SEL_INDEX_MIN;
  }

  // If this is the last entry in the log, return 0xFFFF
  if (*next_rec_id == g_sel_hdr.end) {
    *next_rec_id = SEL_RECID_LAST;
  }

  return 0;
}

// Add a new entry in to SEL log
// IPMI/Section 31.6
int
plat_sel_add_entry(sel_msg_t *msg, int *rec_id) {
  // If the SEL if full, roll over. To keep track of empty condition, use
  // one empty location less than the max records.
  if (plat_sel_num_entries() == SEL_RECORDS_MAX) {
      syslog(LOG_ALERT, "plat_sel_add_entry: SEL rollover\n");
    if (++g_sel_hdr.begin > SEL_INDEX_MAX) {
      g_sel_hdr.begin = SEL_INDEX_MIN;
    }
  }

  // Update message's time stamp starting at byte 4
  time_stamp_fill(&msg->msg[3]);

  // Add the enry at end
  memcpy(g_sel_data[g_sel_hdr.end].msg, msg->msg, sizeof(sel_msg_t));

  // Return the newly added record ID
  *rec_id = g_sel_hdr.end+1;

  if (file_store_sel_data(*rec_id, msg)) {
    syslog(LOG_ALERT, "plat_sel_add_entry: file_store_sel_data\n");
    return -1;
  }

  // Increment the end pointer
  if (++g_sel_hdr.end > SEL_INDEX_MAX) {
    g_sel_hdr.end = SEL_INDEX_MIN;
  }

  // Update timestamp for add in header
  time_stamp_fill(g_sel_hdr.ts_add.ts);

  // Store the structure persistently
  if (file_store_sel_hdr()) {
    syslog(LOG_ALERT, "plat_sel_add_entry: file_store_sel_hdr\n");
    return -1;
  }

  return 0;
}

// Erase the SEL completely
// IPMI/Section 31.9
// Note: To reduce wear/tear, instead of erasing, manipulating the metadata
int
plat_sel_erase(int rsv_id) {
  if (rsv_id != g_rsv_id) {
    return -1;
  }

  // Erase SEL Logs
  g_sel_hdr.begin = SEL_INDEX_MIN;
  g_sel_hdr.end = SEL_INDEX_MIN;

  // Update timestamp for erase in header
  time_stamp_fill(g_sel_hdr.ts_erase.ts);

  // Store the structure persistently
  if (file_store_sel_hdr()) {
    syslog(LOG_ALERT, "plat_sel_erase: file_store_sel_hdr\n");
    return -1;
  }

  return 0;
}

// To get the erase status while erase happens
// IPMI/Section 31.2
// Note: Since we are not doing offline erasing, need not return in-progress state
int
plat_sel_erase_status(int rsv_id, sel_erase_stat_t *status) {
  if (rsv_id != g_rsv_id) {
    return -1;
  }

  // Since we do not do any offline erasing, always return erase done
  *status = SEL_ERASE_DONE;

  return 0;
}

// Initialize SEL log file
int
plat_sel_init(void) {
  FILE *fp;

  // Check if the file exists or not
  if (access(SEL_LOG_FILE, F_OK) == 0) {
    // Since file is present, fetch all the contents to cache
    if (file_get_sel_hdr()) {
      syslog(LOG_ALERT, "plat_init_sel: file_get_sel_hdr\n");
      return -1;
    }

    if (file_get_sel_data()) {
      syslog(LOG_ALERT, "plat_init_sel: file_get_sel_data\n");
      return -1;
    }

    return 0;
  }

  // File not present, so create the file
  fp = fopen(SEL_LOG_FILE, "w+");
  if (fp == NULL) {
    syslog(LOG_ALERT, "plat_init_sel: fopen\n");
    return -1;
  }

  fclose (fp);

  // Populate SEL Header in to the file
  g_sel_hdr.magic = SEL_HDR_MAGIC;
  g_sel_hdr.version = SEL_HDR_VERSION;
  g_sel_hdr.begin = SEL_INDEX_MIN;
  g_sel_hdr.end = SEL_INDEX_MIN;
  memset(g_sel_hdr.ts_add.ts, 0x0, 4);
  memset(g_sel_hdr.ts_erase.ts, 0x0, 4);

  if (file_store_sel_hdr()) {
    syslog(LOG_ALERT, "plat_init_sel: file_store_sel_hdr\n");
    return -1;
  }

  // Populate SEL Data in to the file
  for (int i = 1;  i <= SEL_RECORDS_MAX; i++) {
    sel_msg_t msg = {0};
    if (file_store_sel_data(i, &msg)) {
      syslog(LOG_ALERT, "plat_init_sel: file_store_sel_data\n");
      return -1;
    }
  }

  return 0;
}
