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
#include <jansson.h>
#include <getopt.h>

#define FRUID_SIZE      512
#define FLAG_PRINT  (0)
#define FLAG_DUMP   (0x01 << 0)
#define FLAG_WRITE  (0x01 << 1)
#define FLAG_MODIFY (0x01 << 2)

enum format{
  DEFAULT_FORMAT = 0,
  JSON_FORMAT = 1,
};

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
    if (fruid->chassis.custom5 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 5",fruid->chassis.custom5);
    if (fruid->chassis.custom6 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 6",fruid->chassis.custom6);
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
    if (fruid->board.custom5 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 5",fruid->board.custom5);
    if (fruid->board.custom6 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 6",fruid->board.custom6);
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
    if (fruid->product.custom5 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 5",fruid->product.custom5);
    if (fruid->product.custom6 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 6",fruid->product.custom6);
  }

  printf("\n");
}

/* Print the FRUID in json format */
static void
print_json_fruid_info(fruid_info_t *fruid, const char *name)
{
  json_t *fru_object = json_object();

  json_object_set_new(fru_object, "FRU Information", json_string(name));
  if (fruid->chassis.flag) {
    json_object_set_new(fru_object, "Chassis Type", json_string(fruid->chassis.type_str));
    json_object_set_new(fru_object, "Chassis Part Number", json_string(fruid->chassis.part));
    json_object_set_new(fru_object, "Chassis Serial Number", json_string(fruid->chassis.serial));
    if (fruid->chassis.custom1 != NULL)
      json_object_set_new(fru_object, "Chassis Custom Data 1", json_string(fruid->chassis.custom1));
    if (fruid->chassis.custom2 != NULL)
      json_object_set_new(fru_object, "Chassis Custom Data 2", json_string(fruid->chassis.custom2));
    if (fruid->chassis.custom3 != NULL)
      json_object_set_new(fru_object, "Chassis Custom Data 3", json_string(fruid->chassis.custom3));
    if (fruid->chassis.custom4 != NULL)
      json_object_set_new(fru_object, "Chassis Custom Data 4", json_string(fruid->chassis.custom4));
    if (fruid->chassis.custom5 != NULL)
      json_object_set_new(fru_object, "Chassis Custom Data 5", json_string(fruid->chassis.custom5));
    if (fruid->chassis.custom6 != NULL)
      json_object_set_new(fru_object, "Chassis Custom Data 6", json_string(fruid->chassis.custom6));
  }

  if (fruid->board.flag) {
    json_object_set_new(fru_object, "Board Mfg Date", json_string(fruid->board.mfg_time_str));
    json_object_set_new(fru_object, "Board Mfg", json_string(fruid->board.mfg));
    json_object_set_new(fru_object, "Board Product", json_string(fruid->board.name));
    json_object_set_new(fru_object, "Board Serial", json_string(fruid->board.serial));
    json_object_set_new(fru_object, "Board Part Number", json_string(fruid->board.part));
    json_object_set_new(fru_object, "Board FRU ID", json_string(fruid->board.fruid));
    if (fruid->board.custom1 != NULL)
      json_object_set_new(fru_object, "Board Custom Data 1", json_string(fruid->board.custom1));
    if (fruid->board.custom2 != NULL)
      json_object_set_new(fru_object, "Board Custom Data 2", json_string(fruid->board.custom2));
    if (fruid->board.custom3 != NULL)
      json_object_set_new(fru_object, "Board Custom Data 3", json_string(fruid->board.custom3));
    if (fruid->board.custom4 != NULL)
      json_object_set_new(fru_object, "Board Custom Data 4", json_string(fruid->board.custom4));
    if (fruid->board.custom5 != NULL)
      json_object_set_new(fru_object, "Board Custom Data 5", json_string(fruid->board.custom5));
    if (fruid->board.custom6 != NULL)
      json_object_set_new(fru_object, "Board Custom Data 6", json_string(fruid->board.custom6));
  }

  if (fruid->product.flag) {
    json_object_set_new(fru_object, "Product Manufacturer", json_string(fruid->product.mfg));
    json_object_set_new(fru_object, "Product Name", json_string(fruid->product.name));
    json_object_set_new(fru_object, "Product Part Number", json_string(fruid->product.part));
    json_object_set_new(fru_object, "Product Version", json_string(fruid->product.version));
    json_object_set_new(fru_object, "Product Serial", json_string(fruid->product.serial));
    json_object_set_new(fru_object, "Product Asset Tag", json_string(fruid->product.asset_tag));
    json_object_set_new(fru_object, "Product FRU ID", json_string(fruid->product.fruid));
    if (fruid->product.custom1 != NULL)
      json_object_set_new(fru_object, "Product Custom Data 1", json_string(fruid->product.custom1));
    if (fruid->product.custom2 != NULL)
      json_object_set_new(fru_object, "Product Custom Data 2", json_string(fruid->product.custom2));
    if (fruid->product.custom3 != NULL)
      json_object_set_new(fru_object, "Product Custom Data 3", json_string(fruid->product.custom3));
    if (fruid->product.custom4 != NULL)
      json_object_set_new(fru_object, "Product Custom Data 4", json_string(fruid->product.custom4));
    if (fruid->product.custom5 != NULL)
      json_object_set_new(fru_object, "Product Custom Data 5", json_string(fruid->product.custom5));
    if (fruid->product.custom6 != NULL)
      json_object_set_new(fru_object, "Product Custom Data 6", json_string(fruid->product.custom6));
  }

  json_dumpf(fru_object, stdout, 4);
  json_decref(fru_object);

  printf("\n");
}

/* Populate and print fruid_info by parsing the fru's binary dump */
void get_fruid_info(uint8_t fru, char *path, char* name, unsigned char print_format) {
  int ret;
  fruid_info_t fruid;

  ret = fruid_parse(path, &fruid);
  if (ret) {
    fprintf(stderr, "Failed print FRUID for %s\nCheck syslog for errors!\n",
        name);
  } else if (print_format == JSON_FORMAT) {
    print_json_fruid_info(&fruid, name);
    free_fruid_info(&fruid);
  } else {
    print_fruid_info(&fruid, name);
    free_fruid_info(&fruid);
  }
}

static void
print_usage() {
  if (pal_dev_list_print_t != NULL || pal_dev_list_rw_t != NULL) {
    printf("Usage: fruid-util [ %s ] [ %s ] [--json]\n"
      "Usage: fruid-util [ %s ] [ %s ] [--dump | --write ] <file>\n",
      pal_fru_list_print_t, pal_dev_list_print_t, pal_fru_list_rw_t, pal_dev_list_rw_t);
    printf("Usage: fruid-util [ %s ] [ %s ] --modify --<field> <data> <file>\n",
      pal_fru_list_rw_t, pal_dev_list_rw_t);
    printf("       <field> : CPN  (Chassis Part Number)\n"
           "                       e.g., fruid-util fru1 --modify --CPN \"xxxxx\" xxx.bin\n"
           "                 CSN  (Chassis Serial Number)\n"
           "                 CCD1 (Chassis Custom Data 1)\n"
           "                 CCD2 (Chassis Custom Data 2)\n"
           "                 CCD3 (Chassis Custom Data 3)\n"
           "                 CCD4 (Chassis Custom Data 4)\n"
           "                 CCD5 (Chassis Custom Data 5)\n"
           "                 CCD6 (Chassis Custom Data 6)\n"
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
           "                 BCD5 (Board Custom Data 5)\n"
           "                 BCD6 (Board Custom Data 6)\n"
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
           "                 PCD4 (Product Custom Data 4)\n"
           "                 PCD5 (Product Custom Data 5)\n"
           "                 PCD6 (Product Custom Data 6)\n");
  } else {
    printf("Usage: fruid-util [ %s ] [--json]\n"
      "Usage: fruid-util [ %s ] [--dump | --write ] <file>\n",
      pal_fru_list_print_t, pal_fru_list_rw_t);
    printf("Usage: fruid-util [ %s ] --modify <field> <data> <file>\n",
      pal_fru_list_rw_t);
    printf("       [field] : CPN  (Chassis Part Number)\n"
           "                       e.g., fruid-util fru1 --modify --CPN \"xxxxx\" xxx.bin\n"
           "                 CSN  (Chassis Serial Number)\n"
           "                 CCD1 (Chassis Custom Data 1)\n"
           "                 CCD2 (Chassis Custom Data 2)\n"
           "                 CCD3 (Chassis Custom Data 3)\n"
           "                 CCD4 (Chassis Custom Data 4)\n"
           "                 CCD5 (Chassis Custom Data 5)\n"
           "                 CCD6 (Chassis Custom Data 6)\n"
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
           "                 BCD5 (Board Custom Data 5)\n"
           "                 BCD6 (Board Custom Data 6)\n"
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
           "                 PCD4 (Product Custom Data 4)\n"
           "                 PCD5 (Product Custom Data 5)\n"
           "                 PCD6 (Product Custom Data 6)\n");
  }
  exit(-1);
}

int check_dump_arg(int argc, char * argv[]) {
  /*  example:
      fruid-util  slot  --dump  file
      fruid-util  slot  device  --dump  file
  */
  if (argc != 4 && argc != 5) {
    return -1;
  }
  if (strstr(pal_fru_list_print_t, argv[1]) == NULL) {
    printf("Cannot dump FRUID for %s\n", argv[1]);
    return -1;
  }
  if (argc == 5) {
    if (pal_dev_list_rw_t == NULL) {
      return -1;
    }
    if (strstr(pal_dev_list_rw_t, argv[2]) == NULL) {
      printf("Cannot dump FRUID for %s %s\n", argv[1], argv[2]);
      return -1;
    }
  }

  return 0;
}

int check_write_arg(int argc, char * argv[])
{
  /*  example:
      fruid-util  slot  --write  file
      fruid-util  slot  device  --write  file
  */
  if (argc != 4 && argc != 5) {
    return -1;
  }
  if (strstr(pal_fru_list_print_t, argv[1]) == NULL) {
    printf("Cannot write FRUID for %s\n", argv[1]);
    return -1;
  }
  if (argc == 5) {
    if (pal_dev_list_rw_t == NULL) {
      return -1;
    }
    if (strstr(pal_dev_list_rw_t, argv[2]) == NULL) {
      printf("Cannot write FRUID for %s %s\n", argv[1], argv[2]);
      return -1;
    }
  }

  return 0;
}

int check_modify_arg(int argc, char * argv[])
{
  /*  exampl
      fruid-util  slot  --modify  --CPN  556699  /tmp/modify.bin
      fruid-util  slot  device  ---modify  --CPN  556699  /tmp/modify.bin
  */
  if (argc != 6 && argc != 7) {
    return -1;
  }
  if (strstr(pal_fru_list_print_t, argv[1]) == NULL) {
    printf("Cannot modify FRUID for %s\n", argv[1]);
    return -1;
  }
  if (argc == 6) {
    if (strcmp(argv[2], "--modify") != 0)
      return -1;
  }
  if (argc == 7) {
    if (pal_dev_list_rw_t == NULL) {
      return -1;
    }
    if (strstr(pal_dev_list_rw_t, argv[2]) == NULL) {
      printf("Cannot modify FRUID for %s %s\n", argv[1], argv[2]);
      return -1;
    }
  }

  return 0;
}

int check_print_arg(int argc, char * argv[])
{
  /*  example:
      fruid-util  all
      fruid-util  all  --json
      fruid-util  slot  device
      fruid-util  slot  device  --json
  */
  if (argc != 2 && argc != 3 && argc != 4) {
    return -1;
  }
  if (strstr(pal_fru_list_print_t, argv[optind]) == NULL) {
    printf("Cannot read FRUID for %s\n", argv[optind]);
    return -1;
  }
  if (argv[optind+1] != NULL) {
    if (pal_dev_list_print_t == NULL) {
      return -1;
    }
    if (strstr(pal_dev_list_print_t, argv[optind+1]) == NULL) {
      printf("Cannot print FRUID for %s %s\n", argv[optind], argv[optind+1]);
      return -1;
    }
  }
  if (argc == 4) {
    if (strcmp(argv[1], "--json") != 0)
      return -1;
  }

  return 0;
}

int print_fru(int fru, char * device, unsigned char print_format) {
  int ret;
  char path[64] = {0};
  char name[64] = {0};
  uint8_t status;
  uint8_t num_devs = 0;
  uint8_t dev_id = DEV_NONE;

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

  if (device != NULL) {
    pal_get_num_devs(fru,&num_devs);
    ret = pal_get_dev_id(device, &dev_id);
    if (ret < 0) {
      print_usage();
    }
    if (dev_id != DEV_NONE && dev_id != DEV_ALL) {
      if (num_devs == 0)
        return -1;
      pal_get_dev_name(fru,dev_id,name);
      ret = pal_get_dev_fruid_path(fru, dev_id, path);
      if (ret < 0) {
        printf("%s is unavailable!\n\n", name);
      }
    }
    if (dev_id == DEV_ALL) {
      // when use command "fruid-util slot1 all", will show slot1 fru first then show all device fru.
      ret = pal_get_fruid_path(fru, path);
    }
  } else {
    ret = pal_get_fruid_path(fru, path);
  }

  if (ret < 0) {
    return ret;
  }

  get_fruid_info(fru, path, name, print_format);
  if (num_devs && dev_id == DEV_ALL) {
    for (uint8_t i=1;i<=num_devs;i++) {
      pal_get_dev_name(fru,i,name);
      ret = pal_get_dev_fruid_path(fru, i, path);
      if (ret < 0) {
        printf("%s is unavailable!\n\n", name);
      } else {
        get_fruid_info(fru, path, name, print_format);
      }
    }
  }
  return ret;
}

int do_print_fru(int argc, char * argv[], unsigned char print_format)
{
  int ret;
  uint8_t fru;
  char * device = argv[optind+1];

  ret = check_print_arg(argc, argv);
  if (ret) {
    print_usage();
  }

  ret = pal_get_fru_id(argv[optind], &fru);
  if (ret < 0) {
    print_usage();
  }

  if (fru != FRU_ALL) {
    ret = print_fru(fru, device, print_format);
    if (ret < 0) {
      print_usage();
    }
  } else {
    fru = 1;
    while (fru <= MAX_NUM_FRUS) {
      ret = print_fru(fru, device, print_format);
      fru++;
    }
  }

  return 0;
}

int do_action(int argc, char * argv[], unsigned char action_flag) {
  int ret;
  int fd_tmpbin;
  int fd_newbin;
  int fru_size;
  char path[64] = {0};
  char name[64] = {0};
  char eeprom_path[64] = {0};
  char command[128] = {0};
  char *file_path;
  uint8_t status;
  uint8_t fru;
  uint8_t num_devs = 0;
  uint8_t dev_id = DEV_NONE;
  fruid_info_t fruid;
  FILE *fp;

  ret = pal_get_fru_id(argv[optind], &fru);
  if (ret < 0) {
    print_usage();
  }

  if (fru == FRU_ALL) {
    print_usage();
    return -1;
  }

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

  if (argc == 5 || argc == 7) {
    pal_get_num_devs(fru, &num_devs);
    if (num_devs == 0) {
      print_usage();
    }
    ret = pal_get_dev_id(argv[optind+1], &dev_id);
    if (ret < 0) {
      print_usage();
    }
    if ( dev_id == DEV_NONE || dev_id == DEV_ALL ) {
      print_usage();
    }
    pal_get_dev_name(fru, dev_id,name);
    ret = pal_get_dev_fruid_path(fru, dev_id, path);
  } else {
    ret = pal_get_fruid_path(fru, path);
  }
  if (ret < 0) {
    return ret;
  }

  switch(action_flag) {
    case FLAG_DUMP:
      file_path = argv[optind-1];
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
      }

      close(fd_newbin);
      close(fd_tmpbin);
      return ret;

    case FLAG_WRITE:
      file_path = argv[optind-1];
      // Check if the new eeprom binary file exits.
      // TODO: Add file size check before adding to the eeprom
      if (access(file_path, F_OK) == -1) {
        print_usage();
      }
      // Verify the checksum of the new binary
      ret = fruid_parse(file_path, &fruid);
      if(ret != 0) {
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
          syslog(LOG_WARNING, "[%s] Please check the fruid: %d dev_id: %d file_path: %s", __func__, fru, dev_id, file_path);
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
        if (system(command) != 0) {
          syslog(LOG_ERR, "Copy of %s to %s failed!\n", file_path, eeprom_path);
        }

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
      }

      close(fd_newbin);
      close(fd_tmpbin);
      return ret;

    case FLAG_MODIFY:
      file_path = argv[argc-1];
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

      ret = fruid_modify(path, file_path, argv[optind-2], argv[optind-1]);
      if(ret < 0){
        printf("Fail to modify %s FRU\n", name);
        return ret;
      }
      get_fruid_info(fru, file_path, name, DEFAULT_FORMAT);
      return 0;

    default:
        return -1;
  }

  return 0;
}

/* Utility to just print the FRUID */
int main(int argc, char * argv[]) {
  int c;
  unsigned char print_format = DEFAULT_FORMAT;
  unsigned char action_flag = 0;

  if (!strncmp(pal_fru_list_rw_t, "all, ", strlen("all, "))) {
    pal_fru_list_rw_t = pal_fru_list_rw_t + strlen("all, ");
  }
  if (pal_dev_list_rw_t != NULL) {
    if (!strncmp(pal_dev_list_rw_t, "all, ", strlen("all, "))) {
      pal_dev_list_rw_t = pal_dev_list_rw_t + strlen("all, ");
    }
  }

  struct option opts[] = {
    {"dump", 1, NULL, 'd'},
    {"write", 1, NULL, 'w'},
    {"modify", 0, NULL, 'm'},
    {"json", 0, NULL, 'j'},
    {"CPN", 1, NULL, 'f'},
    {"CSN", 1, NULL, 'f'},
    {"CCD1", 1, NULL, 'f'},
    {"CCD2", 1, NULL, 'f'},
    {"CCD3", 1, NULL, 'f'},
    {"CCD4", 1, NULL, 'f'},
    {"CCD5", 1, NULL, 'f'},
    {"CCD6", 1, NULL, 'f'},
    {"BMD", 1, NULL, 'f'},
    {"BM", 1, NULL, 'f'},
    {"BP", 1, NULL, 'f'},
    {"BSN", 1, NULL, 'f'},
    {"BPN", 1, NULL, 'f'},
    {"BFI", 1, NULL, 'f'},
    {"BCD1", 1, NULL, 'f'},
    {"BCD2", 1, NULL, 'f'},
    {"BCD3", 1, NULL, 'f'},
    {"BCD4", 1, NULL, 'f'},
    {"BCD5", 1, NULL, 'f'},
    {"BCD6", 1, NULL, 'f'},
    {"PM", 1, NULL, 'f'},
    {"PN", 1, NULL, 'f'},
    {"PPN", 1, NULL, 'f'},
    {"PV", 1, NULL, 'f'},
    {"PSN", 1, NULL, 'f'},
    {"PAT", 1, NULL, 'f'},
    {"PFI", 1, NULL, 'f'},
    {"PCD1", 1, NULL, 'f'},
    {"PCD2", 1, NULL, 'f'},
    {"PCD3", 1, NULL, 'f'},
    {"PCD4", 1, NULL, 'f'},
    {"PCD5", 1, NULL, 'f'},
    {"PCD6", 1, NULL, 'f'},
  };

  const char *optstring = "";   //not support short option

  while((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1) {
    switch(c) {
      case 'd':
        if (check_dump_arg(argc, argv)) {
          print_usage();
        }
        action_flag = action_flag | FLAG_DUMP;
        break;
      case 'w':
        if (check_write_arg(argc, argv)) {
          print_usage();
        }
        action_flag = action_flag | FLAG_WRITE;
        break;
      case 'm':
        if (check_modify_arg(argc, argv)) {
          print_usage();
        }
        action_flag = action_flag | FLAG_MODIFY;
        break;
      case 'j':
        print_format = JSON_FORMAT;
        break;
      case 'f':
        /* field option */
        break;
      case '?':
        printf("unknown option !!!\n\n");
        print_usage();
        break;
      default:
        print_usage();
    }
  }

  switch(action_flag) {
    case FLAG_DUMP:
    case FLAG_WRITE:
    case FLAG_MODIFY:
      do_action(argc, argv, action_flag);
      break;
    case FLAG_PRINT:
      do_print_fru(argc, argv, print_format);
      break;
    default:
      print_usage();
  }

  return 0;
}
