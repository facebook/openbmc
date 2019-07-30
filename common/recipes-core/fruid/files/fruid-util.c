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
#define FRU_MODIFY      0x3
#define FRUID_SIZE      512

#ifdef CUSTOM_FRU_LIST
  static const char * pal_fru_list_print_t =  pal_fru_list_print;
  static const char * pal_fru_list_rw_t =  pal_fru_list_rw;
#else
  static const char * pal_fru_list_print_t =  pal_fru_list;
  static const char * pal_fru_list_rw_t =  pal_fru_list;
#endif /* CUSTOM_FRU_LIST */

#ifdef FRU_DEVICE_LIST
  static const char * pal_dev_list_print_t = pal_dev_list;
  static const char * pal_dev_list_rw_t =  pal_dev_list;
#else
  static const char * pal_dev_list_print_t = NULL;
  static const char * pal_dev_list_rw_t = NULL;
#endif

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
    if (fruid->chassis.custom4 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 4",fruid->chassis.custom4);
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
    if (fruid->board.custom4 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 4",fruid->board.custom4);
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
    if (fruid->product.custom4 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 4",fruid->product.custom4);
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

static void
print_usage() {
  if (pal_dev_list_print_t != NULL || pal_dev_list_rw_t != NULL) {
    printf("Usage: fruid-util [ %s ] [ %s ]\n"
      "Usage: fruid-util [ %s ] [ %s ] [--dump | --write ] <file>\n",
      pal_fru_list_print_t, pal_dev_list_print_t, pal_fru_list_rw_t, pal_dev_list_rw_t);
    printf("Usage: fruid-util [ %s ] [ %s ] [ --modify ] [ --field ] [ \"data\" ] <file>\n",
      pal_fru_list_rw_t, pal_dev_list_rw_t);
    printf("       [field] : CPN  (Chassis Part Number)\n"
           "                       e.g., fruid-util fru1 --modify --CPN \"xxxxx\" xxx.bin\n" 
           "                 CSN  (Chassis Serial Number)\n"
           "                 CCD1 (Chassis Custom Data 1)\n"
           "                 CCD2 (Chassis Custom Data 2)\n"
           "                 CCD3 (Chassis Custom Data 3)\n"
           "                 CCD4 (Chassis Custom Data 4)\n"
           "                 BMD  (Board Mfg Date)\n"   
           "                       e.g., fruid-util fru1 --modify --BMD \"$( date +%%s -d \"2018-01-01 17:00:00\" )\" xxx.bin\n"
           "                 BM   (Board Mfg)\n"
           "                 BP   (Board Product)\n"
           "                 BSN  (Board Serial)\n"
           "                 BPN  (Board Part Number)\n"
           "                 BFI  (Board FRU ID)\n"
           "                 BCD1 (Board Custom Data 1)\n"
           "                 BCD2 (Board Custom Data 2)\n"
           "                 BCD3 (Board Custom Data 3)\n"
           "                 BCD4 (Board Custom Data 4)\n"
           "                 PM   (Product Manufacturer)\n"
           "                 PN   (Product Name)\n"
           "                 PPN  (Product Part Number)\n"
           "                 PV   (Product Version)\n"
           "                 PSN  (Product Serial)\n"
           "                 PAT  (Product Asset Tag)\n"
           "                 PFI  (Product FRU ID)\n"
           "                 PCD1 (Product Custom Data 1)\n"
           "                 PCD2 (Product Custom Data 2)\n"
           "                 PCD3 (Product Custom Data 3)\n"
           "                 PCD4 (Product Custom Data 4)\n");
  } else {
    printf("Usage: fruid-util [ %s ]\n"
      "Usage: fruid-util [ %s ] [--dump | --write ] <file>\n",
      pal_fru_list_print_t, pal_fru_list_rw_t);
    printf("Usage: fruid-util [ %s ] [ --modify ] [ --field ] [ \"data\" ] <file>\n",
      pal_fru_list_rw_t);
    printf("       [field] : CPN  (Chassis Part Number)\n"
           "                       e.g., fruid-util fru1 --modify --CPN \"xxxxx\" xxx.bin\n"
           "                 CSN  (Chassis Serial Number)\n"
           "                 CCD1 (Chassis Custom Data 1)\n"
           "                 CCD2 (Chassis Custom Data 2)\n"
           "                 CCD3 (Chassis Custom Data 3)\n"
           "                 CCD4 (Chassis Custom Data 4)\n"
           "                 BMD  (Board Mfg Date)\n"
           "                       e.g., fruid-util fru1 --modify --BMD \"$( date +%%s -d \"2018-01-01 17:00:00\" )\" xxx.bin\n"
           "                 BM   (Board Manufacturer)\n"
           "                 BP   (Board Product Number)\n"
           "                 BSN  (Board Serial Number)\n"
           "                 BPN  (Board Part Number)\n"
           "                 BFI  (Board FRU ID)\n"
           "                 BCD1 (Board Custom Data 1)\n"
           "                 BCD2 (Board Custom Data 2)\n"
           "                 BCD3 (Board Custom Data 3)\n"
           "                 BCD4 (Board Custom Data 4)\n"
           "                 PM   (Product Manufacturer)\n"
           "                 PN   (Product Name)\n"
           "                 PPN  (Product Part Number)\n"
           "                 PV   (Product Version)\n"
           "                 PSN  (Product Serial Number)\n"
           "                 PAT  (Product Asset Tag)\n"
           "                 PFI  (Product FRU ID)\n"
           "                 PCD1 (Product Custom Data 1)\n"
           "                 PCD2 (Product Custom Data 2)\n"
           "                 PCD3 (Product Custom Data 3)\n"
           "                 PCD4 (Product Custom Data 4)\n");
  }
  exit(-1);
}

/* Utility to just print the FRUID */
int main(int argc, char * argv[]) {

  int ret;
  int rw = 0;
  int fd_tmpbin;
  int fd_newbin;
  FILE *fp;
  int fru_size;
  uint8_t fru;
  uint8_t dev_id = DEV_NONE;
  char *file_path = NULL;
  char path[64] = {0};
  char eeprom_path[64] = {0};
  char name[64] = {0};
  char command[128] = {0};
  uint8_t status;
  fruid_info_t fruid;
  char* exist;
  uint8_t num_devs = 0;

  if (!strncmp(pal_fru_list_rw_t, "all, ", strlen("all, "))) {
    pal_fru_list_rw_t = pal_fru_list_rw_t + strlen("all, ");
  }

  if (pal_dev_list_rw_t != NULL) {
    if (!strncmp(pal_dev_list_rw_t, "all, ", strlen("all, "))) {
      pal_dev_list_rw_t = pal_dev_list_rw_t + strlen("all, ");
    }
  }

  //Check if the input FRU is valid
  if (argc == 2 || argc == 3) {
    exist = strstr(pal_fru_list_print_t, argv[1]);
    if (exist == NULL) {
      printf("Cannot read FRUID for %s\n", argv[1]);
      print_usage();
    }
  } else if (argc == 4 || argc == 5) {
    exist = strstr(pal_fru_list_rw_t, argv[1]);
    if (exist == NULL) {
      printf("Cannot dump/write FRUID for %s\n", argv[1]);
      print_usage();
    }
  } else if (argc == 6 || argc == 7) {
    exist = strstr(pal_fru_list_rw_t, argv[1]);
    if (exist == NULL) {
      printf("Cannot modify FRUID for %s\n", argv[1]);
      print_usage();
    }
  } else {
    print_usage();
  }

  ret = pal_get_fru_id(argv[1], &fru);
  if (ret < 0) {
    print_usage();
  }

  pal_get_num_devs(fru,&num_devs);

  //Check if the input device is valid
  if (argc == 3 || argc == 5 || argc == 7) {
    ret = pal_get_dev_id(argv[2], &dev_id);
    if (ret < 0) {
      print_usage();
    }

    if (num_devs) {
      if (argc == 3) {
        if (pal_dev_list_print_t == NULL)
          print_usage();
        exist = strstr(pal_dev_list_print_t, argv[2]);
        if (exist == NULL) {
          printf("Cannot read FRUID for %s %s\n", argv[1], argv[2]);
          print_usage();
        }
      } else if (argc == 5){
        if (pal_dev_list_rw_t == NULL)
          print_usage();
        exist = strstr(pal_dev_list_rw_t, argv[2]);
        if (exist == NULL) {
          printf("Cannot dump/write FRUID for %s %s\n", argv[1], argv[2]);
          print_usage();
        }
      } else {
        if (pal_dev_list_rw_t == NULL)
          print_usage();
        exist = strstr(pal_dev_list_rw_t, argv[2]);
        if (exist == NULL) {
          printf("Cannot modify FRUID for %s %s\n", argv[1], argv[2]);
          print_usage();
        }
      }
    } else {
      if (fru != FRU_ALL) {
        print_usage();
      }
    }
  }

  //Check dump/write options
  if (argc == 4 || argc == 5) {
    if (!strcmp(argv[argc-2], "--dump")) {
      rw = EEPROM_READ;
      file_path = argv[argc-1];
    } else if (!strcmp(argv[argc-2], "--write")) {
      rw = EEPROM_WRITE;
      file_path = argv[argc-1];
    } else {
      print_usage();
    }
  }

  //Check modify options
  if (argc == 6 || argc == 7) {
    if (!strcmp(argv[argc-4], "--modify")) {
      rw = FRU_MODIFY;
      file_path = argv[argc-1];
    } else {
      print_usage();
    }
  }

  // Check if the new eeprom binary file exits.
  // TODO: Add file size check before adding to the eeprom
  if (rw == EEPROM_WRITE && (access(file_path, F_OK) == -1)) {
      print_usage();
  }

  if (fru != FRU_ALL) {
    ret = pal_get_fruid_name(fru, name);
    if (ret < 0) {
       return ret;
    }

    ret = pal_is_fru_prsnt(fru, &status);
    if (ret < 0) {
       printf("pal_is_fru_prsnt failed for fru: %d\n", fru);
       return ret;
    }

    if (status == 0) {
      printf("%s is not present!\n\n", name);
      return ret;
    }

    ret = pal_is_fru_ready(fru, &status);
    if ((ret < 0) || (status == 0)) {
      printf("%s is unavailable!\n\n", name);
      return ret;
    }

    if (num_devs && dev_id != DEV_NONE && dev_id != DEV_ALL) {
      pal_get_dev_name(fru,dev_id,name);
      ret = pal_get_dev_fruid_path(fru, dev_id, path);
    } else {
      if (dev_id == DEV_ALL && rw != 0)
        print_usage();
      ret = pal_get_fruid_path(fru, path);
    }

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


      fp = fopen(file_path, "rb");
      if ( NULL == fp )
      {
        syslog(LOG_ERR, "Unable to get the %s fp %s", file_path, strerror(errno));
        return errno;
      }

      //get the size of the new binary
      fseek(fp, 0L, SEEK_END);
      fru_size = ftell(fp);
      rewind(fp);
      fclose(fp);

      //check the size overflow or not
      fru_size = (fru_size > FRUID_SIZE)?FRUID_SIZE:fru_size;

      ret = pal_get_fruid_eeprom_path(fru, eeprom_path);
      if (ret < 0) {
        //Can not handle in common, so call pal libray for update
        if (num_devs) {
          if (dev_id == DEV_ALL) 
            ret = -1;
          else if (dev_id == DEV_NONE)
            ret = pal_fruid_write(fru, file_path);
          else
            ret = pal_dev_fruid_write(fru, dev_id, file_path);
        } else {
          ret = pal_fruid_write(fru, file_path);
        }

        if (ret < 0) {
          syslog(LOG_WARNING, "[%s] Please check the fruid: %d device id: %d file_path: %s", __func__, fru, dev_id,file_path);
          close(fd_newbin);
          close(fd_tmpbin);
          return -1;
        }
      } else {
        if (access(eeprom_path, F_OK) == -1) {
          syslog(LOG_ERR, "cannot access the eeprom file : %s for fru %d",
              eeprom_path, fru);
          close(fd_newbin);
          close(fd_tmpbin);
          return -1;
        }
        sprintf(command, "dd if=%s of=%s bs=%d count=1", file_path, eeprom_path, fru_size);
        system(command);

        ret = pal_compare_fru_data(eeprom_path, file_path, fru_size);
        if (ret < 0)
        {
          syslog(LOG_ERR, "[%s] FRU:%d Write Fail", __func__, fru);
          close(fd_newbin);
          close(fd_tmpbin);
          return -1;
        }
      }

      ret = copy_file(fd_tmpbin, fd_newbin, fru_size);
      if (ret < 0) {
        syslog(LOG_ERR, "copy: write to %s file failed: %s",
            path, strerror(errno));
        close(fd_newbin);
        close(fd_tmpbin);
        return ret;
      }

      close(fd_newbin);
      close(fd_tmpbin);

    } else if (rw == FRU_MODIFY) {
      /* FRUID BINARY MODIFY */
      
      if(access(file_path, F_OK) == -1) {  //copy current FRU bin file to specified bin file
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
      } else {
        memset(path, 0, sizeof(path));
        sprintf(path, "%s", file_path);
      }

      ret = fruid_modify(path, file_path, argv[argc-3], argv[argc-2]);
      if(ret < 0){
        printf("Fail to modify %s FRU\n", name);  
        return ret;
      }

      get_fruid_info(fru, file_path, name);
    } else {
      /* FRUID PRINT ONE FRU */

      get_fruid_info(fru, path, name);
      if (num_devs && dev_id == DEV_ALL) {
        for (uint8_t i=1;i<=num_devs;i++) {
          pal_get_dev_name(fru,i,name);
          ret = pal_get_dev_fruid_path(fru, i, path);
          if (ret < 0) {
            printf("%s is unavailable!\n\n", name);
          } else {
            get_fruid_info(fru, path, name);
          }
        }
      }
    }
  } else if (fru == 0) {

    /* FRUID PRINT ALL FRUs */

    fru = 1;

    while (fru <= MAX_NUM_FRUS) {
      ret = pal_get_fruid_path(fru, path);
      if (ret < 0) {
        fru++;
        continue;
      }

      ret = pal_get_fruid_name(fru, name);
      if (ret < 0) {
        fru++;
        continue;
      }

      ret = pal_is_fru_prsnt(fru, &status);
      if (ret < 0) {
        printf("pal_is_fru_prsnt failed for fru: %d\n", fru);
        fru++;
        continue;
      }

      if (status == 0) {
        printf("\n%s is not present!\n\n", name);
        fru++;
        continue;
      }

      ret = pal_is_fru_ready(fru, &status);
      if ((ret < 0) || (status == 0)) {
        printf("%s is unavailable!\n\n", name);
        fru++;
        continue;
      }

      if (dev_id == DEV_NONE || dev_id == DEV_ALL)
        get_fruid_info(fru, path, name);

      num_devs = 0;
      pal_get_num_devs(fru,&num_devs);
      if (num_devs) {
        if (dev_id == DEV_ALL){
          for (uint8_t i=1;i<=num_devs;i++) {
            pal_get_dev_name(fru,i,name);
            ret = pal_get_dev_fruid_path(fru, i, path);
            if (ret < 0) {
              printf("%s is unavailable!\n\n", name);
            } else {
              get_fruid_info(fru, path, name);
            }
          }
        } else if (dev_id != DEV_NONE) {
          pal_get_dev_name(fru,dev_id,name);
          ret = pal_get_dev_fruid_path(fru, dev_id, path);
          if (ret < 0) {
            printf("%s is unavailable!\n\n", name);
          }
          get_fruid_info(fru, path, name);
        }
      }
      fru++;
    }
  }

  return 0;
}
