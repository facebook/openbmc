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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/fruid.h>
#include <openbmc/pal.h>

#define EEPROM_READ     0x1
#define EEPROM_WRITE    0x2
#define FRUID_SIZE      512

/* To copy the bin files */
static int
copy_file(int out, int in, int bs) {

  ssize_t bytes_rd, bytes_wr;
  uint64_t tmp[FRUID_SIZE];

  while ((bytes_rd = read(in, tmp, FRUID_SIZE)) > 0) {
    bytes_wr = write(out, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      return errno;
    }
  }
  return 0;
}

/* Print the FRUID in detail */
static void
print_fruid_info(fruid_info_t *fruid, const char *name)
{
  /* Print format */
  printf("%-27s: %s", "\nFRU Information",
      name /* Name of the FRU device */ );
  printf("%-27s: %s", "\n---------------", "------------------");

  if (fruid->chassis.flag) {
    printf("%-27s: %s", "\nChassis Type",fruid->chassis.type_str);
    printf("%-27s: %s", "\nChassis Part Number",fruid->chassis.part);
    printf("%-27s: %s", "\nChassis Serial Number",fruid->chassis.serial);
    if (fruid->chassis.custom1 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 1",fruid->chassis.custom1);
    if (fruid->chassis.custom2 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 2",fruid->chassis.custom2);
    if (fruid->chassis.custom3 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 3",fruid->chassis.custom3);
  }

  if (fruid->board.flag) {
    printf("%-27s: %s", "\nBoard Mfg Date",fruid->board.mfg_time_str);
    printf("%-27s: %s", "\nBoard Mfg",fruid->board.mfg);
    printf("%-27s: %s", "\nBoard Product",fruid->board.name);
    printf("%-27s: %s", "\nBoard Serial",fruid->board.serial);
    printf("%-27s: %s", "\nBoard Part Number",fruid->board.part);
    printf("%-27s: %s", "\nBoard FRU ID",fruid->board.fruid);
    if (fruid->board.custom1 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 1",fruid->board.custom1);
    if (fruid->board.custom2 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 2",fruid->board.custom2);
    if (fruid->board.custom3 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 3",fruid->board.custom3);
  }

  if (fruid->product.flag) {
    printf("%-27s: %s", "\nProduct Manufacturer",fruid->product.mfg);
    printf("%-27s: %s", "\nProduct Name",fruid->product.name);
    printf("%-27s: %s", "\nProduct Part Number",fruid->product.part);
    printf("%-27s: %s", "\nProduct Version",fruid->product.version);
    printf("%-27s: %s", "\nProduct Serial",fruid->product.serial);
    printf("%-27s: %s", "\nProduct Asset Tag",fruid->product.asset_tag);
    printf("%-27s: %s", "\nProduct FRU ID",fruid->product.fruid);
    if (fruid->product.custom1 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 1",fruid->product.custom1);
    if (fruid->product.custom2 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 2",fruid->product.custom2);
    if (fruid->product.custom3 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 3",fruid->product.custom3);
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

#ifdef CONFIG_FBTTN
  // No NIC for FBTTN
  // TODO: Remove NIC fruid option from main()
  char pal_fru_list[] = "all, slot1, iom, dpb, scc";
#endif /* CONFIG_FBTTN */

  if (!strncmp(pal_fru_list, "all, ", strlen("all, "))) {
    printf("Usage: fruid-util [ %s ]\n"
        "Usage: fruid-util [ %s ] [--dump | --write ] <file>\n",
        pal_fru_list, pal_fru_list + strlen("all, "));

    return;
  }

  printf("Usage: fruid-util [ %s ]\n"
      "Usage: fruid-util [%s] [--dump | --write ] <file>\n",
      pal_fru_list, pal_fru_list);
}

/* Utility to just print the FRUID */
int main(int argc, char * argv[]) {

  int ret;
  int rw = 0;
  int fd_tmpbin;
  int fd_newbin;
  int fd_eeprom;
  uint8_t fru;
  char *file_path = NULL;
  char path[64] = {0};
  char eeprom_path[64] = {0};
  char name[64] = {0};
  char command[128] = {0};
  uint8_t status;
  fruid_info_t fruid;

  if (argc != 2 && argc != 4) {
    print_usage();
    exit(-1);
  }

  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0) {
    print_usage();
    return ret;
  }

  if (fru == FRU_ALL && argc > 2) {
    printf("Cannot dump/write FRUID for \"all\". Please use select individual FRU.\n");
    print_usage();
    exit(-1);
  }

  if (argc > 2) {
    if (!strcmp(argv[2], "--dump")) {
      rw = EEPROM_READ;
      file_path = argv[3];
    } else if (!strcmp(argv[2], "--write")) {
      rw = EEPROM_WRITE;
      file_path = argv[3];
    }
  }

  // Check if the new eeprom binary file exits.
  // TODO: Add file size check before adding to the eeprom
  if (rw == EEPROM_WRITE && (access(file_path, F_OK) == -1)) {
      print_usage();
      exit(-1);
  }

  if (fru != FRU_ALL) {
    ret = pal_get_fruid_name(fru, name);
    if (ret < 0) {
       return ret;
    }

    ret = pal_is_fru_prsnt(fru, &status);
    if (ret < 0) {
       printf("pal_is_server_fru failed for fru: %d\n", fru);
       return ret;
    }

    if (status == 0) {
      printf("%s is empty!\n\n", name);
      return ret;
    }

    ret = pal_get_fruid_path(fru, path);
    if (ret < 0) {
      return ret;
    }

    errno = 0;

    /* FRUID BINARY DUMP */
    if (rw == EEPROM_READ) {
      fd_tmpbin = open(path, O_RDONLY);
      if (fd_tmpbin == -1) {
        syslog(LOG_ERR, "Unable to open the %s file: %s", path, strerror(errno));
        return errno;
      }

      fd_newbin = open(file_path, O_WRONLY | O_CREAT, 0644);
      if (fd_newbin == -1) {
        syslog(LOG_ERR, "Unable to create %s file: %s", file_path, strerror(errno));
        return errno;
      }

      ret = copy_file(fd_newbin, fd_tmpbin, FRUID_SIZE);
      if (ret < 0) {
        syslog(LOG_ERR, "copy: write to %s file failed: %s",
            file_path, strerror(errno));
        return ret;
      }

      close(fd_newbin);
      close(fd_tmpbin);

    } else if (rw == EEPROM_WRITE) {

    /* FRUID BINARY WRITE */

      // Verify the checksum of the new binary
      ret = fruid_parse(file_path, &fruid);
      if(ret != 0){
        syslog(LOG_CRIT, "New FRU data checksum is invalid");
        return -1;
      }

      fd_tmpbin = open(path, O_WRONLY);
      if (fd_tmpbin == -1) {
        syslog(LOG_ERR, "Unable to open the %s file: %s", path, strerror(errno));
        return errno;
      }

      fd_newbin = open(file_path, O_RDONLY);
      if (fd_newbin == -1) {
        syslog(LOG_ERR, "Unable to open the %s file: %s", file_path, strerror(errno));
        return errno;
      }

      ret = pal_get_fruid_eeprom_path(fru, eeprom_path);
      if (ret < 0) {
        //Can not handle in common, so call pal libray for update
        pal_fruid_write(fru, file_path);
      } else {
        if (access(eeprom_path, F_OK) == -1) {
          syslog(LOG_ERR, "cannot access the eeprom file : %s for fru %d",
              eeprom_path, fru);
          close(fd_newbin);
          close(fd_tmpbin);
          return -1;
        }
        sprintf(command, "dd if=%s of=%s bs=%d count=1", file_path, eeprom_path, FRUID_SIZE);
        system(command);
      }

      ret = copy_file(fd_tmpbin, fd_newbin, FRUID_SIZE);
      if (ret < 0) {
        syslog(LOG_ERR, "copy: write to %s file failed: %s",
            path, strerror(errno));
        return ret;
      }

      close(fd_newbin);
      close(fd_tmpbin);

    } else {
      /* FRUID PRINT ONE FRU */

      get_fruid_info(fru, path, name);
    }

  } else if (fru == 0) {

    /* FRUID PRINT ALL FRUs */

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

      ret = pal_is_fru_prsnt(fru, &status);
      if (ret < 0) {
         printf("pal_is_server_fru failed for fru: %d\n", fru);
         fru++;
         continue;
      }

      if (status == 0) {
         printf("\n%s is empty!\n\n", name);
         fru++;
         continue;
      }

      get_fruid_info(fru, path, name);

      fru++;
    }
  }

  return 0;
}
