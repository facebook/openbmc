/*
 * psu-util
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#include <facebook/agc032a-psu.h>

static const char *option_list[] = {
  "  --get_psu_info",
  "  --get_eeprom_info",
  "  --get_blackbox_info",
  "    options:",
  "      --print",
  "      --clear",
};

static void
print_usage(const char *name) {
  int i;

  printf("Usage: %s <psu1|psu2> --update <file_path>\n", name);
  printf("Usage: %s <psu1|psu2> <command> <options>\n", name);
  printf("       command:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

int
main(int argc, const char *argv[]) {
  uint8_t psu_num = 0, prsnt = 0;
  int pid_file = 0;
  int ret = 0;

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
    printf("Get PSU%d present error!\n", psu_num + 1);
    return ret;
  }
  if (!prsnt) {
    printf("PSU%d is not present!\n", psu_num + 1);
    return -1;
  }

  if (!strcmp(argv[2], "--get_psu_info")) {
    ret = get_psu_info(psu_num);
  }
  else if (!strcmp(argv[2], "--get_blackbox_info") && argv[3] != NULL) {
    ret = get_blackbox_info(psu_num, argv[3]);
  }
  else if (!strcmp(argv[2], "--get_eeprom_info")) {
    ret = get_eeprom_info(psu_num, argv[3]);
  }
  else if (!strcmp(argv[2], "--update") && argv[3] != NULL) {
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
