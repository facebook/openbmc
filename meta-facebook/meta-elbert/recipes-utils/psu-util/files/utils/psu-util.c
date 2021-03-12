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

#include <facebook/elbert-psu.h>

#define NUM_PSU_SLOTS 4
#define REQ_BACKUP_PSUS 1

static void
print_usage(const char *name) {
  printf("Usage: %s <psu1|psu2|psu3|psu4> --update <file_path> [vendor]\n", name);
}

int
main(int argc, const char *argv[]) {
  uint8_t psu_num = 0, prsnt = 0, pwr_ok = 0, backup_psus = 0;
  int pid_file = 0, ret = 0;
  int i;

  if (argc < 3) {
    print_usage(argv[0]);
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
    print_usage(argv[0]);
    return -1;
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
    printf("At least %d other PSU must be functional to perform update.\n",
           REQ_BACKUP_PSUS);
    return -1;
  }

  if (!strcmp(argv[2], "--update") && argv[3] != NULL) {
    ret = do_update_psu(psu_num, argv[3], argv[4]);
    if (ret) {
      syslog(LOG_WARNING, "PSU%d update fail!", psu_num + 1);
      printf("PSU%d update fail!\n", psu_num + 1);
    } else {
      syslog(LOG_WARNING, "PSU%d update success!", psu_num + 1);
    }
  }
  else {
    print_usage(argv[0]);
    return -1;
  }

  return ret;
}
