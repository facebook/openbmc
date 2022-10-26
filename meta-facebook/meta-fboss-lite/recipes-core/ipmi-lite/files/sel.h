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

#ifndef __SEL_H__
#define __SEL_H__

#include "timestamp.h"

enum {
  IPMI_SEL_INIT_ERASE = 0xAA,
  IPMI_SEL_ERASE_STAT = 0x00,
};

typedef enum {
  SEL_ERASE_IN_PROG = 0x00,
  SEL_ERASE_DONE = 0x01,
} sel_erase_stat_t;

typedef struct {
  unsigned char msg[16];
} sel_msg_t;

typedef struct {
  unsigned char msg[35];
} ras_sel_msg_t;

void sel_ts_recent_add(int node, time_stamp_t* ts);
void sel_ts_recent_erase(int node, time_stamp_t* ts);
int sel_num_entries(int node);
int sel_free_space(int node);
int sel_rsv_id(int node);
int sel_get_entry(int node, int read_rec_id, sel_msg_t* msg, int* next_rec_id);
int sel_add_entry(int node, sel_msg_t* msg, int* rec_id);
int sel_erase(int node, int rsv_id);
int sel_erase_status(int node, int rsv_id, sel_erase_stat_t* status);
int sel_init(void);
int ras_sel_add_entry(int node, ras_sel_msg_t* msg);

#endif /* __SEL_H__ */
