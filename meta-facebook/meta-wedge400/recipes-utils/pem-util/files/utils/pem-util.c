/*
 * pem-util
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

#include <facebook/wedge400-pem.h>

static const char *option_list[] = {
  "  --get_pem_info",
  "  --get_eeprom_info",
  "    options:",
  "      --print",
  "      --clear",
  "  --get_blackbox_info",
  "    options:",
  "      --print",
  "      --clear",
  "  --get_archive_log",
  "    options:",
  "      --print",
  "      --clear",
  "  --archive_pem_chips",
};

static void
print_usage(const char *name) {
  int i;

  printf("Usage: %s <pem1|pem2> --update [--force] <file_path>\n", name);
  printf("Usage: %s <pem1|pem2> <command> <options>\n", name);
  printf("       command:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

int
main(int argc, const char *argv[]) {
  uint8_t pem_num = 0, prsnt = 0;
  int pid_file = 0;
  int ret = 0;

  if (argc < 3) {
    print_usage(argv[0]);
    return -1;
  }

  if (!strcmp(argv[1], "pem1")) {
    pem_num = 0;
  }
  else if (!strcmp(argv[1] , "pem2")) {
    pem_num = 1;
  }
  else {
    print_usage(argv[0]);
    return -1;
  }

  pid_file = open("/var/run/pem-util.pid", O_CREAT | O_RDWR, 0666);
  if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    printf("Another pem-util instance is running...\n");
    exit(EXIT_FAILURE);
  }

  ret = is_pem_prsnt(pem_num, &prsnt);
  if (ret) {
    printf("Get PEM%d present error!\n", pem_num + 1);
    return ret;
  }
  if (!prsnt) {
    printf("PEM%d is not present!\n", pem_num + 1);
    return -1;
  }

  if (!strcmp(argv[2], "--get_pem_info")) {
    ret = get_pem_info(pem_num);
  }
  else if (!strcmp(argv[2], "--get_blackbox_info") && argv[3] != NULL) {
    ret = get_blackbox_info(pem_num, argv[3]);
  }
  else if (!strcmp(argv[2], "--get_eeprom_info") && argv[3] != NULL) {
    ret = get_eeprom_info(pem_num, argv[3]);
  }
  else if (!strcmp(argv[2], "--get_archive_log") && argv[3] != NULL) {
    ret = get_archive_log(pem_num, argv[3]);
  }
  else if (!strcmp(argv[2], "--archive_pem_chips")) {
    ret = archive_pem_chips(pem_num);
  }
  else if (!strcmp(argv[2], "--update") && argv[3] != NULL) {
    if (!strcmp(argv[3], "--force") && argv[4] != NULL) {
      ret = do_update_pem(pem_num, argv[4], argv[5], 1);
    } else {
      ret = do_update_pem(pem_num, argv[3], argv[4], 0);
    }
    if (ret < 0) {
      syslog(LOG_WARNING, "PEM%d update fail!", pem_num + 1);
      printf("PEM%d update fail!\n", pem_num + 1);
    } else {
      if (ret == FW_IDENTICAL) {
        printf("PEM%d firmware is identical to the target, skip to update.\n" \
               "Please add \"--force\" option to update forcibly!\n", pem_num + 1);
      } else {
        syslog(LOG_WARNING, "PEM%d update success!", pem_num + 1);
      }
    }
  }
  else {
    print_usage(argv[0]);
    return -1;
  }

  return ret;
}
