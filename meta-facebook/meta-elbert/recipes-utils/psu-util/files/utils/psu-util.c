/*
 * psu-util
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#include <unistd.h>
#include <getopt.h>
#include <facebook/elbert-psu.h>

#define NUM_PSU_SLOTS 4
#define REQ_BACKUP_PSUS 1
#define BUFSIZE 255

static void
print_usage(void) {
  printf("Usage: psu-util <psu1|psu2|psu3|psu4> --update <file_path> "
         "[--vendor <vendor>]\n");
}

int
main(int argc, char *argv[]) {
  uint8_t psu_num = 0, prsnt = 0, pwr_ok = 0, backup_psus = 0;
  int pid_file = 0, ret = 0, opt_index = 0, i, opt;
  char fw_file[BUFSIZE + 1] = {0}, vendor[BUFSIZE + 1] = {0};

  static struct option long_options[] = {
    {"update", required_argument, NULL, 'u'},
    {"vendor", required_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
  };

  if (argc < 3) {
    print_usage();
    return -1;
  }

  if (!strcmp(argv[1], "psu1")) {
    psu_num = 0;
  }
  else if (!strcmp(argv[1] , "psu2")) {
    psu_num = 1;
  }
  else if (!strcmp(argv[1] , "psu3")) {
    psu_num = 2;
  }
  else if (!strcmp(argv[1] , "psu4")) {
    psu_num = 3;
  }
  else {
    print_usage();
    return -1;
  }

  while ((opt = getopt_long(argc, argv, "u:v", long_options, &opt_index)) != -1) {
    switch(opt) {
      case 'u':
        snprintf(fw_file, BUFSIZE, "%s", optarg);
        break;
      case 'v':
        snprintf(vendor, BUFSIZE, "%s", optarg);
        break;
      default:
        print_usage();
        return -1;
    }
  }

  pid_file = open("/var/run/psu-util.pid", O_CREAT | O_RDWR, 0666);
  if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    printf("Another psu-util instance is running...\n");
    exit(EXIT_FAILURE);
  }

  ret = is_psu_prsnt(psu_num, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
    printf("PSU%d is not present!\n", psu_num + 1);
    return -1;
  }

  for (i = 0; i < NUM_PSU_SLOTS; i++) {
    if (i != psu_num) {
      is_psu_power_ok(i, &pwr_ok);
      if (pwr_ok) {
        backup_psus++;
      }
    }
  }
  if (backup_psus < REQ_BACKUP_PSUS) {
    printf("At least %d other PSUs must be functional to perform update.\n",
           REQ_BACKUP_PSUS);
    return -1;
  }

  ret = do_update_psu(psu_num, fw_file, vendor);
  if (ret) {
    syslog(LOG_WARNING, "PSU%d update fail!", psu_num + 1);
    printf("PSU%d update fail!\n", psu_num + 1);
  } else {
    syslog(LOG_WARNING, "PSU%d update success!", psu_num + 1);
    printf("PSU%d update success!\n", psu_num + 1);
  }

  return ret;
}
