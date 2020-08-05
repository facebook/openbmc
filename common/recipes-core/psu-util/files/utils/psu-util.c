/*
 * psu-util
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <facebook/psu.h>

#ifndef PSU_NUM
#define PSU_NUM 0
#error "please define the correct PSU_NUM value"
#endif

static const char *option_list[] = {
  "  --get_psu_info",
  "  --get_eeprom_info",
  "  --get_blackbox_info",
  "    options:",
  "      --print",
  "      --clear",
};

static void
print_usage(const char *name, const char psu_num) {
  int i;
  printf("Usage: %s <", name);
  for (i = 0; i < psu_num; i++)
  {
    printf("psu%d", i + 1);

    if(i + 1< psu_num)
    {
      printf("|");
    }
  }
  printf("> --update <file_path>\n");

  printf("Usage: %s <", name);
  for (i = 0; i < psu_num; i++)
  {
    printf("psu%d", i + 1);

    if(i + 1< psu_num)
    {
      printf("|");
    }
  }
  printf("> <command> <options>\n");

  printf("       command:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

char get_psu_id(const char *name, const char psu_num)
{
  char ret = -1;
  char i = 0;
  char buffer[32];
  for (i = 0; i < psu_num; i++)
  {
    snprintf(buffer, sizeof(buffer), "psu%d", i+1);
    if(!strcmp(name, buffer))
    {
      ret = i;
      break;
    }
  }
  return ret;
}

int
main(int argc, const char *argv[]) {
  uint8_t psu_slot = 0, prsnt = 0;
  int pid_file = 0;
  int ret = 0;

  if (argc < 3) {
    print_usage(argv[0], PSU_NUM);
    return -1;
  }

  psu_slot = get_psu_id(argv[1], PSU_NUM);
  if (psu_slot < 0) {
    print_usage(argv[0], PSU_NUM);
    return -1;
  }

  pid_file = open("/var/run/psu-util.pid", O_CREAT | O_RDWR, 0666);

  if (pid_file < 0) {
     syslog(LOG_WARNING, "%s: fails to open\n", __func__);
  }

  if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    printf("Another psu-util instance is running...\n");
    exit(EXIT_FAILURE);
  }

  ret = is_psu_prsnt(psu_slot, &prsnt);
  if (ret) {
    printf("Get PSU%d present error!\n", psu_slot + 1);
    return ret;
  }
  if (!prsnt) {
    printf("PSU%d is not present!\n", psu_slot + 1);
    return -1;
  }

  if (!strcmp(argv[2], "--get_psu_info")) {
    ret = get_psu_info(psu_slot);
  }
  else if (!strcmp(argv[2], "--get_blackbox_info") && argv[3] != NULL) {
    ret = get_blackbox_info(psu_slot, argv[3]);
  }
  else if (!strcmp(argv[2], "--get_eeprom_info")) {
    ret = get_eeprom_info(psu_slot);
  }
  else if (!strcmp(argv[2], "--update") && argv[3] != NULL) {
    ret = do_update_psu(psu_slot, argv[3], argv[4]);
    if (ret) {
      syslog(LOG_WARNING, "PSU%d update fail!", psu_slot + 1);
      printf("PSU%d update fail!\n", psu_slot + 1);
    } else {
      syslog(LOG_WARNING, "PSU%d update success!", psu_slot + 1);
    }
  }
  else {
    print_usage(argv[0], PSU_NUM);
    return -1;
  }

  return ret;
}
