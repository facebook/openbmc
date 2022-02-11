/*
 * guid-util
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
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <openbmc/pal.h>

#define GUID_SIZE 16

#ifdef GUID_FRU_LIST
static const char *guid_fru_list = pal_guid_fru_list;
#else
static const char *guid_fru_list = NULL;
#endif

static char *guid_comp[MAX_NUM_FRUS] = {0};

static void
print_usage_help(void) {
  char flist[128], str[16];
  int i;

  if (guid_fru_list == NULL) {
    printf("Usage: guid-util <dev|sys> <--get>\n");
    printf("       guid-util <dev|sys> <--set> <uid>\n");
  } else {
    snprintf(flist, sizeof(flist), "<%s", guid_comp[0]);
    for (i = 1; i < MAX_NUM_FRUS && guid_comp[i]; i++) {
      snprintf(str, sizeof(str), "|%s", guid_comp[i]);
      strncat(flist, str, sizeof(flist)-strlen(flist)-1);
    }
    snprintf(str, sizeof(str), ">");
    strncat(flist, str, sizeof(flist)-strlen(flist)-1);

    printf("Usage: guid-util %s <dev|sys> <--get>\n", flist);
    printf("       guid-util %s <dev|sys> <--set> <uid>\n", flist);
  }
}

int
main(int argc, char **argv) {
  int ret, i;
  int min_argc = 3, type_idx = 1;
  uint8_t fru = 1;
  uint8_t do_get = 0, do_set = 0, do_sys = 0;
  char gf_list[128], node_src[64], *tok;
  char guid[GUID_SIZE] = {0};
  static struct option long_opts[] = {
    {"get", no_argument, 0, 'g'},
    {"set", required_argument, 0, 's'},
    {0,0,0,0},
  };

  if (guid_fru_list) {
    min_argc += 1;
    type_idx += 1;  // argv index for type, <dev|sys>

    // initialize guid_comp[] for help message
    i = 0;
    strncpy(gf_list, guid_fru_list, sizeof(gf_list) - 1);
    tok = strtok(gf_list, ", ");
    while (tok) {
      guid_comp[i++] = tok;
      tok = strtok(NULL, ", ");
    }
  }

  do {
    if (argc < min_argc) {  // check the minimum argc
      break;
    }

    while ((ret = getopt_long(argc-type_idx, &argv[type_idx], "gs:", long_opts, NULL)) != -1) {
      switch (ret) {
        case 'g':
          do_get = 1;
          break;
        case 's':
          do_set = 1;
          strncpy(node_src, optarg, sizeof(node_src) - 1);
          break;
      }
    }
    if (!(do_get ^ do_set)) {
      break;
    }

    if (guid_fru_list) {
      ret = pal_get_fru_id(argv[1], &fru);
      if (ret) {
        break;
      }
    }

    if (!strcmp(argv[type_idx], "dev")) {
      do_sys = 0;
    } else if (!strcmp(argv[type_idx], "sys")) {
      do_sys = 1;
    } else {
      break;
    }

    if (do_get) {
      ret = (do_sys) ? pal_get_sys_guid(fru, guid) : pal_get_dev_guid(fru, guid);
      if (ret) {
        break;
      }

      for (i = 0; i < GUID_SIZE; i++) {
        if (i > 0) {
          printf(":");
        }
        printf("0x%02X", guid[i]);
      }
      printf("\n");
      return 0;
    }

    if (do_set) {
      ret = (do_sys) ? pal_set_sys_guid(fru, node_src) : pal_set_dev_guid(fru, node_src);
      if (ret) {
        break;
      }
      return 0;
    }
  } while (0);

  print_usage_help();
  return -1;
}
