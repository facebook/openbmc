/*
 * name-util
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <jansson.h>
#include <openbmc/pal.h>

#define MAX_FRU_PER_TYPE   16
#define VERSION_MAJOR      '1'
#define VERSION_MINOR      '0'
#define VERSION_PATCH      '0'

#define VERSION_STR     "version"
#define SERVER_FRU_STR  "server_fru"
#define NIC_FRU_STR     "nic_fru"
#define BMC_FRU_STR     "bmc_fru"
#define MAX_FRU_TYPE    3
const char version[] = {VERSION_MAJOR, '.', VERSION_MINOR, '.', VERSION_PATCH};

typedef struct {
  fru_type_t type;
  const char* name_str;
  json_t *obj;
} fru_struct_t;


static json_t *
get_fru_json_arr(const char **fru_list, uint8_t num_fru) {
  json_t *fru_arr = NULL;
  int i, ret = 0;

  fru_arr = json_array();
  if (fru_arr == NULL) {
    printf("%s failed to allocate fru_arr\n", __FUNCTION__); //debug
    return NULL;
  }

  for (i = 0; i < num_fru; ++i) {
    ret = json_array_append(fru_arr, json_string(fru_list[i]));
    if (ret != 0) {
      printf("%s array_append, ret=%d\n", __FUNCTION__, ret); //debug
      return NULL;
    }
  }
  return fru_arr;
}

static void print_usage_help(void) {
  printf("Usage: name-util all \n");
  printf("       - shows all possible FRU names on the system\n");
  printf("       - not all FRUs may be present\n");
}

static int get_list_fru(int fru_type) {
  json_t *fru_obj = NULL;
  json_t *server_arr = NULL;
  json_t *bmc_arr = NULL;
  json_t *nic_arr = NULL;
  fru_struct_t *cur_fru_type = NULL;
  const char **fru_list = {0};
  uint8_t num_fru = 0;
  int i, ret = 0;

  fru_struct_t fru_type_list[MAX_FRU_TYPE] = {
      { .type = FRU_TYPE_SERVER, .name_str = SERVER_FRU_STR, .obj = server_arr  },
      { .type = FRU_TYPE_NIC, .name_str = NIC_FRU_STR, .obj = nic_arr },
      { .type = FRU_TYPE_BMC, .name_str = BMC_FRU_STR, .obj = bmc_arr  },
  };

  fru_obj = json_object();
  if (!fru_obj)
    goto ERROR_EXIT;

  ret = json_object_set_new(fru_obj, VERSION_STR, json_string(version));
  if (ret != 0) {
    printf("error, failed to set version, ret=%d\n", ret);
    goto ERROR_EXIT;
  }

  for (i = 0; i < MAX_FRU_TYPE; ++i) {
    cur_fru_type = &(fru_type_list[i]);
    ret = pal_get_fru_type_list(cur_fru_type->type, &fru_list, &num_fru);
    if (ret == 0 && num_fru > 0 && (cur_fru_type->obj = get_fru_json_arr(fru_list, num_fru))) {
      ret = json_object_set_new(fru_obj, cur_fru_type->name_str, cur_fru_type->obj);
      if (ret != 0) {
        printf("error, failed to add %s, ret %d\n", cur_fru_type->name_str, ret);
        goto ERROR_EXIT;
      }
    } else {
      printf("error, failed to get %s, ret %d num_fru %d\n", cur_fru_type->name_str, ret, num_fru);
      goto ERROR_EXIT;
    }
  }

  ret = json_dumpf(fru_obj, stdout, JSON_INDENT(4));
  json_decref(fru_obj);
  printf("\n");

  return ret;

ERROR_EXIT:
  if (server_arr)
    json_decref(server_arr);
  if (bmc_arr)
    json_decref(bmc_arr);
  if (nic_arr)
    json_decref(nic_arr);
  if (fru_obj)
    json_decref(fru_obj);
  ret = -1;
  return ret;
}

int main(int argc, char **argv)
{
  int fru_type, ret = 0;

  if (argc < 2) {
    goto err_exit;
  }
  if (!strcmp(argv[1], "all")) {
    fru_type = 0xff;
  } else {
    goto err_exit;
  }
  ret = get_list_fru(fru_type);
  return ret;

err_exit:
  print_usage_help();
  return -1;
}
