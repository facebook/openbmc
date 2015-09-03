/*
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
#include <openbmc/fruid.h>
#include <openbmc/pal.h>

/* Print the FRUID in detail */
void print_fruid_info(fruid_info_t *fruid, const char *name)
{
  /* Print format */
  printf("%-27s: %s", "\nFRU Information",
      name /* Name of the FRU device */ );
  printf("%-27s: %s", "\n---------------", "------------------");

  if (fruid->chassis.flag) {
    printf("%-27s: %s", "\nChassis Type",fruid->chassis.type_str);
    printf("%-27s: %s", "\nChassis Part",fruid->chassis.part);
    printf("%-27s: %s", "\nChassis Serial Number",fruid->chassis.serial);
    printf("%-27s: %s", "\nChassis Custom Data",fruid->chassis.custom);
  }

  if (fruid->board.flag) {
    printf("%-27s: %s", "\nBoard Mfg Time",fruid->board.mfg_time_str);
    printf("%-27s: %s", "\nBoard Manufacturer",fruid->board.mfg);
    printf("%-27s: %s", "\nBoard Name",fruid->board.name);
    printf("%-27s: %s", "\nBoard Serial Number",fruid->board.serial);
    printf("%-27s: %s", "\nBoard Part",fruid->board.part);
    printf("%-27s: %s", "\nBoard FRU ID",fruid->board.fruid);
    printf("%-27s: %s", "\nBoard Custom Data",fruid->board.custom);
  }

  if (fruid->product.flag) {
    printf("%-27s: %s", "\nProduct Manufacturer",fruid->product.mfg);
    printf("%-27s: %s", "\nProduct Name",fruid->product.name);
    printf("%-27s: %s", "\nProduct Part",fruid->product.part);
    printf("%-27s: %s", "\nProduct Version",fruid->product.version);
    printf("%-27s: %s", "\nProduct Serial Number",fruid->product.serial);
    printf("%-27s: %s", "\nProduct Asset Tag",fruid->product.asset_tag);
    printf("%-27s: %s", "\nProduct FRU ID",fruid->product.fruid);
    printf("%-27s: %s", "\nProduct Custom Data",fruid->product.custom);
  }

  printf("\n");
}

/* Populate and print fruid_info by parsing the fru's binary dump */
void get_fruid_info(uint8_t fru, char *path, char* name) {
  int ret;
  fruid_info_t fruid;

  ret = fruid_parse(path, &fruid);
  if (ret) {
    fprintf(stderr, "Failed print FRUID for %s\nCheck syslog for errors!\n",
        name);
  } else {
    print_fruid_info(&fruid, name);
    free_fruid_info(&fruid);
  }

}

static int
print_usage() {
  printf("Usage: fruid-util [ %s ]\n", pal_fru_list);
}

/* Utility to just print the FRUID */
int main(int argc, char * argv[]) {

  int ret;
  uint8_t fru;
  char path[64] = {0};
  char name[64] = {0};

  if (argc != 2) {
    print_usage();
    exit(-1);
  }

  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0) {
    print_usage();
    return ret;
  }

  if (fru == 0) {
    fru = 1;
    while (fru <= MAX_NUM_FRUS) {
      ret = pal_get_fruid_path(fru, path);
      if (ret < 0) {
        return ret;
      }

      ret = pal_get_fruid_name(fru, name);
      if (ret < 0) {
        return ret;
      }

      if (fru == FRU_NIC) {
        printf("fruid-util does not support nic\n");
        exit(-1);
      }

      get_fruid_info(fru, path, name);

      fru++;
    }
  } else {
    ret = pal_get_fruid_path(fru, path);
    if (ret < 0) {
      return ret;
    }

    ret = pal_get_fruid_name(fru, name);
    if (ret < 0) {
      return ret;
    }

    if (fru == FRU_NIC) {
      printf("fruid-util does not support nic\n");
      exit(-1);
    }

    get_fruid_info(fru, path, name);
  }

  return 0;
}
