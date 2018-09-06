/*
 * psu-util
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#include <signal.h>
#include <facebook/minipack-psu.h>

static const char *option_list[] = {
  "--get_psu_info",
  "--get_eeprom_info"
};

static void
print_usage(const char *name) {
  int i;

  printf("Usage: %s <psu1|psu2|psu3|psu4> --update <file_path>\n", name);
  printf("Usage: %s <psu1|psu2|psu3|psu4> <option>\n", name);
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

void
exithandler(int signum) {
  printf("PSU update abort!\n");
  syslog(LOG_WARNING, "PSU update abort!");
  exit(1);
}

int
main(int argc, const char *argv[]) {
  u_int8_t psu_num = 0;
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
  if(flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    printf("Another psu-util instance is running...\n");
    exit(EXIT_FAILURE);
  }

  if (!strcmp(argv[2], "--get_psu_info")) {
    ret = get_psu_info(psu_num);
  }
  else if (!strcmp(argv[2], "--get_eeprom_info")) {
    ret = get_eeprom_info(psu_num, argv[3]);
  }
  else if (!strcmp(argv[2], "--update")) {
    signal(SIGHUP, exithandler);
    signal(SIGINT, exithandler);
    signal(SIGTSTP, exithandler);
    signal(SIGTERM, exithandler);
    signal(SIGQUIT, exithandler);

    run_command("sv stop sensord > /dev/nul");

    switch (psu_num) {
      case 0:
        run_command("/usr/local/bin/sensord scm smb pim1 pim2 pim3 pim4 "
                    "pim5 pim6 pim7 pim8 psu2 psu3 psu4 > /dev/null 2>&1 &");
        break;
      case 1:
        run_command("/usr/local/bin/sensord scm smb pim1 pim2 pim3 pim4 "
                    "pim5 pim6 pim7 pim8 psu1 psu3 psu4 > /dev/null 2>&1 &");
        break;
      case 2:
        run_command("/usr/local/bin/sensord scm smb pim1 pim2 pim3 pim4 "
                    "pim5 pim6 pim7 pim8 psu1 psu2 psu4 > /dev/null 2>&1 &");
        break;
      case 3:
        run_command("/usr/local/bin/sensord scm smb pim1 pim2 pim3 pim4 "
                    "pim5 pim6 pim7 pim8 psu1 psu2 psu3 > /dev/null 2>&1 &");
        break;
    }
    syslog(LOG_WARNING, "Stop monitor PSU%d sensor to update", psu_num + 1);

    ret = do_update_psu(psu_num, argv[3], argv[4]);
    if (ret) {
      syslog(LOG_WARNING, "PSU%d update fail!", psu_num + 1);
      printf("PSU%d update fail!\n", psu_num + 1);
    } else {
      syslog(LOG_WARNING, "PSU%d update success!", psu_num + 1);
    }

    run_command("killall sensord");
    run_command("sv start sensord > /dev/nul");
    syslog(LOG_WARNING, "Start monitor PSU%d sensor", psu_num + 1);
  }
  else {
    print_usage(argv[0]);
    return -1;
  }

  return ret;
}
