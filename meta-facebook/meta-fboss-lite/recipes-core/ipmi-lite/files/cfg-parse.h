/* Copyright (c) Meta Platforms, Inc. and affiliates.
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
 */

#pragma once
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include "timestamp.h"

#define MAX_NAME_SIZE 32
#define MAX_VAL_STR_SIZE 196

typedef enum {
  NUMBER = 1,
  BOOLEAN = 2,
  STRING = 3,
  FRU = 4,
  KV_ACCESS = 5,
} en_type;

typedef struct _fru_t {
  int i2c_addr;
} fru_t;

typedef struct _cfg_item_t {
  struct _cfg_item_t* next;
  char name[MAX_NAME_SIZE];
  en_type type;
  union {
    bool bval;
    char str[MAX_VAL_STR_SIZE];
    int ival;
    fru_t fru;
  } u;
} cfg_item_t;

typedef struct _plat_cfg_t {
  char plat_name[MAX_NAME_SIZE];
  cfg_item_t* cfg_ent_list;
  int cnt;
} plat_cfg_t;

int get_cfg_item_num(const char* name, int* val);
int get_cfg_item_str(const char* name, char* val, uint8_t len);
int get_cfg_item_bool(const char* name, bool* val);
int get_cfg_item_kv(const char* name);
int get_cfg_item_fru(const char* name, fru_t* fru);
int plat_config_parse();
void free_cfg_items();

extern plat_cfg_t* g_cfg;
