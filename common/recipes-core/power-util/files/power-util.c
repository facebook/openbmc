/*
 * power-util
 *
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
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <fcntl.h>
#include <openbmc/pal.h>

#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"

#define MAX_RETRIES          10

#ifndef PWR_OPTION_LIST
#define PWR_OPTION_LIST "status, graceful-shutdown, off, on, reset, cycle, " \
                        "12V-off, 12V-on, 12V-cycle"
#endif
#ifndef PWR_DEV_OPTION_LIST
#define PWR_DEV_OPTION_LIST "status, off, on, reset, cycle"
#endif
#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"

#ifdef FRU_DEVICE_LIST
  static const char * pal_dev_list_power = pal_dev_list;
  static const char * dev_pwr_option_list = pal_dev_pwr_option_list;
#else
  static const char * pal_dev_list_power = NULL;
  static const char * dev_pwr_option_list = NULL;
#endif

const char *pwr_option_list = PWR_OPTION_LIST;

enum {
  PWR_STATUS = 1,
  PWR_GRACEFUL_SHUTDOWN,
  PWR_OFF,
  PWR_ON,
  PWR_RESET,
  PWR_CYCLE,
  PWR_12V_OFF,
  PWR_12V_ON,
  PWR_12V_CYCLE,
  PWR_SLED_CYCLE
};

static const char *option_list[] = {
  "NA",  // place holder so the rest of the list matches enum above
  "STATUS",
  "GRACEFUL_SHUTDOWN",
  "PWR_OFF",
  "PWR_ON",
  "POWER_RESET",
  "POWER_CYCLE",
  "12V_OFF",
  "12V_ON",
  "12V_CYCLE",
  "SLED_CYCLE",
};


static void
print_usage() {
  printf("Usage: power-util [ %s ] [ %s ]\nUsage: power-util sled-cycle\n",
      pal_server_list, pwr_option_list);
  if (pal_dev_list_power != NULL && dev_pwr_option_list != NULL) {
    if (!strncmp(pal_dev_list_power, "all, ", strlen("all, "))) {
      pal_dev_list_power = pal_dev_list_power + strlen("all, ");
    }
    printf("Usage: power-util [ %s ] [ %s ] [ %s ]\n",
      pal_server_list, pal_dev_list_power, dev_pwr_option_list);
  }
}

static bool
is_power_cmd_valid(char *option)
{
  char option_list[256];
  char *pch;

  /* All platforms support sled-cycle */
  if (!strcmp(option, "sled-cycle")) {
    return true;
  }

  /* strtok modifies the passed in string. We cannot
   * pass a const string to it. So make a copy */
  strcpy(option_list, pwr_option_list);
  pch = strtok(option_list, ", ");
  while (pch != NULL) {
    if (!strcmp(pch, option)) {
      return true;
    }
    pch = strtok(NULL, ", ");
  }
  return false;
}

static int
get_power_opt(char *option, uint8_t *opt) {

  if (!is_power_cmd_valid(option)) {
    return -1;
  }

  if (!strcmp(option, "status")) {
    *opt = PWR_STATUS;
  } else if (!strcmp(option, "graceful-shutdown")) {
    *opt = PWR_GRACEFUL_SHUTDOWN;
  } else if (!strcmp(option, "off")) {
    *opt = PWR_OFF;
  } else if (!strcmp(option, "on")) {
    *opt = PWR_ON;
  } else if (!strcmp(option, "reset")) {
    *opt = PWR_RESET;
  } else if (!strcmp(option, "cycle")) {
    *opt = PWR_CYCLE;
  } else if (!strcmp(option, "12V-off")) {
    *opt = PWR_12V_OFF;
  } else if (!strcmp(option, "12V-on")) {
    *opt = PWR_12V_ON;
  } else if (!strcmp(option, "12V-cycle")) {
    *opt = PWR_12V_CYCLE;
  } else if (!strcmp(option, "sled-cycle")) {
    *opt = PWR_SLED_CYCLE;
  } else {
    return -1;
  }

  return 0;
}

//check power policy and power state to power on/off server after AC power restore
void
power_policy_control(uint8_t fru, char *last_ps) {
  uint8_t chassis_status[5] = {0};
  uint8_t chassis_status_length;
  uint8_t power_policy = POWER_CFG_UKNOWN;
  char pwr_state[MAX_VALUE_LEN] = {0};

  if (pal_is_slot_server(fru) == 0) {
    return;
  }

  //get power restore policy
  //defined by IPMI Spec/Section 28.2.
  pal_get_chassis_status(fru, NULL, chassis_status, &chassis_status_length);

  //byte[1], bit[6:5]: power restore policy
  power_policy = (*chassis_status >> 5);

  //Check power policy and last power state
  if(power_policy == POWER_CFG_LPS) {
    if (!last_ps) {
      pal_get_last_pwr_state(fru, pwr_state);
      last_ps = pwr_state;
    }
    if (!(strcmp(last_ps, "on"))) {
      sleep(3);
      pal_set_server_power(fru, SERVER_POWER_ON);
    }
  }
  else if(power_policy == POWER_CFG_ON) {
    sleep(3);
    pal_set_server_power(fru, SERVER_POWER_ON);
  }
}

static bool can_change_power(uint8_t fru)
{
  char fruname[32];
  if (pal_get_fru_name(fru, fruname)) {
    sprintf(fruname, "fru%d", fru);
  }
  if (pal_is_fw_update_ongoing(fru)) {
    printf("FW update for %s is ongoing, block the power controlling.\n", fruname);
    exit(-1);
  }
  if (pal_is_crashdump_ongoing(fru)) {
    printf("Crashdump for %s is ongoing, block the power controlling.\n", fruname);
    exit(-1);
  }
  if (pal_is_cplddump_ongoing(fru)) {
    printf("CPLD dump for %s is ongoing, block the power controlling.\n", fruname);
    exit(-1);
  }
  return true;
}

static int
dev_power_util(uint8_t fru, uint8_t dev_id ,uint8_t opt) {
  int ret = 0;
  uint8_t status, type;
  int retries;

  if (opt != PWR_STATUS) {
    if (!can_change_power(fru)) {
      return -1;
    }
  }

  switch(opt) {
    case PWR_STATUS:
      ret = pal_get_device_power(fru, dev_id, &status, &type);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_get_device_power failed for fru %u dev %u\n", fru, dev_id-1);
        return ret;
      }
      printf("Power status for fru %u dev %u : %s\n", fru, dev_id-1, status?"ON":"OFF");

      break;
    case PWR_OFF:
      printf("Powering fru %u dev %u to OFF state...\n", fru, dev_id-1);

      ret = pal_set_device_power(fru, dev_id, SERVER_POWER_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_device_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u dev %u is already powered OFF...\n", fru, dev_id-1);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_POWER_OFF successful for FRU: %d DEV: %u", fru, dev_id-1);
      }
      break;

    case PWR_ON:

      printf("Powering fru %u dev %u to ON state...\n", fru, dev_id-1);

      ret = pal_set_device_power(fru, dev_id, SERVER_POWER_ON);
      if (ret == 1) {
        printf("fru %u dev %u is already powered ON...\n", fru, dev_id-1);
        return 0;
      } else if (ret == -2) {  //check if fru is not ready
        syslog(LOG_WARNING, "power_util: pal_set_device_power failed for"
          " fru %u", fru);
        return ret;
      }

      for (retries = 0; retries < MAX_RETRIES; retries++) {
         sleep(3);
         ret = pal_get_device_power(fru, dev_id, &status, &type);
         if ((ret >= 0) && (status == SERVER_POWER_ON)) {
           syslog(LOG_CRIT, "SERVER_POWER_ON successful for FRU: %u DEV: %u", fru, dev_id-1);
           break;
         }
         ret = pal_set_device_power(fru, dev_id, SERVER_POWER_ON);
      }
      if (ret < 0 || status != SERVER_POWER_ON) {
        syslog(LOG_WARNING, "power_util: pal_set_device_power failed for"
          " for fru %u dev %u with status=%u", fru, dev_id-1, status);
        return ret;
      }

      break;

    case PWR_CYCLE:
      printf("Power cycling fru %u dev %u...\n", fru, dev_id-1);

      ret = pal_set_device_power(fru, dev_id, SERVER_POWER_CYCLE);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else {
        syslog(LOG_CRIT, "SERVER_POWER_CYCLE successful for FRU: %d DEV: %u", fru, dev_id-1);
      }
      break;

    default:
      syslog(LOG_WARNING, "power_util: wrong option");

  }

  return ret;
}

static int
power_util(uint8_t fru, uint8_t opt) {
  int ret = 0;
  uint8_t status;
  int retries;
  char pwr_state[MAX_VALUE_LEN] = {0};

  if (opt == PWR_SLED_CYCLE) {
    for(fru = 1; fru <= MAX_NUM_FRUS; fru++) {
      if (!can_change_power(fru)) {
        return -1;
      }
    }
  } else if (opt != PWR_STATUS) {
    if (!can_change_power(fru)) {
      return -1;
    }
  }

  switch(opt) {
    case PWR_STATUS:
      ret = pal_get_server_power(fru, &status);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_get_server_power failed for fru %u\n", fru);
        return ret;
      }
      //printf("Power status for fru %u : %s\n", fru, status?"ON":"OFF");
      printf("Power status for fru %u : ", fru);
      switch(status) {
        case SERVER_POWER_ON:
          printf("ON\n");
          break;
        case SERVER_POWER_OFF:
          printf("OFF\n");
          break;
        case SERVER_12V_OFF:
          printf("OFF (12V-OFF)\n");
          break;
      }

      break;

    case PWR_GRACEFUL_SHUTDOWN:
      printf("Shutting down fru %u gracefully...\n", fru);

      ret = pal_set_server_power(fru, SERVER_GRACEFUL_SHUTDOWN);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered OFF...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_GRACEFUL_SHUTDOWN successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_OFF_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_OFF:
      printf("Powering fru %u to OFF state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_POWER_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered OFF...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_POWER_OFF successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_OFF_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_ON:

      printf("Powering fru %u to ON state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_POWER_ON);
      if (ret == 1) {
        printf("fru %u is already powered ON...\n", fru);
        return 0;
      }
      else if (ret == -2) {  //check if fru is not ready
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      }

      for (retries = 0; retries < MAX_RETRIES; retries++) {
         sleep(3);
         ret = pal_get_server_power(fru, &status);
         if ((ret >= 0) && (status == SERVER_POWER_ON)) {
           syslog(LOG_CRIT, "SERVER_POWER_ON successful for FRU: %d", fru);
           break;
         }
         ret = pal_set_server_power(fru, SERVER_POWER_ON);
      }
      if (ret < 0 || status != SERVER_POWER_ON) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " for fru %u with status=%u", fru, status);
        return ret;
      }

      ret = pal_set_last_pwr_state(fru, POWER_ON_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_ON);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      pal_set_restart_cause(fru, RESTART_CAUSE_IPMI_CHASSIS_CMD);
      break;
    case PWR_RESET:
      printf("Power reset fru %u...\n", fru);
      ret = pal_set_server_power(fru, SERVER_POWER_RESET);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_rst_btn failed for"
          " fru %u", fru);
        printf("Power reset fail for fru %u\n", fru);
        return 0;
      }
      syslog(LOG_CRIT, "SERVER_POWER_RESET successful for FRU: %d", fru);
      pal_set_restart_cause(fru, RESTART_CAUSE_IPMI_CHASSIS_CMD);
      break;

    case PWR_CYCLE:
      printf("Power cycling fru %u...\n", fru);

      ret = pal_set_server_power(fru, SERVER_POWER_CYCLE);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else {
        syslog(LOG_CRIT, "SERVER_POWER_CYCLE successful for FRU: %d", fru);
      }

      ret = pal_set_last_pwr_state(fru, POWER_ON_STR);
      if (ret < 0) {
        return ret;
      }

      ret = pal_set_led(fru, LED_ON);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      pal_set_restart_cause(fru, RESTART_CAUSE_IPMI_CHASSIS_CMD);
      break;

    case PWR_12V_OFF:

      printf("12V Powering fru %u to OFF state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_12V_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered 12V-OFF...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_12V_OFF successful for FRU: %d", fru);
      }

      ret = pal_set_led(fru, LED_OFF);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_led failed for fru %u", fru);
        return ret;
      }
      break;

    case PWR_12V_ON:

      printf("12V Powering fru %u to ON state...\n", fru);

      ret = pal_set_server_power(fru, SERVER_12V_ON);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else if (ret == 1) {
        printf("fru %u is already powered 12V-ON...\n", fru);
        return 0;
      } else {
        syslog(LOG_CRIT, "SERVER_12V_ON successful for FRU: %d", fru);

        power_policy_control(fru, NULL);
      }
      break;

    case PWR_12V_CYCLE:

      printf("12V Power cycling fru %u...\n", fru);

      pal_get_last_pwr_state(fru, pwr_state);

      ret = pal_set_server_power(fru, SERVER_12V_CYCLE);
      if (ret < 0) {
        syslog(LOG_WARNING, "power_util: pal_set_server_power failed for"
          " fru %u", fru);
        return ret;
      } else {
        syslog(LOG_CRIT, "SERVER_12V_CYCLE successful for FRU: %d", fru);

        power_policy_control(fru, pwr_state);
      }
      break;

    case PWR_SLED_CYCLE:
      syslog(LOG_CRIT, "SLED_CYCLE starting...");
      pal_update_ts_sled();
      sync();
      sleep(1);
      pal_sled_cycle();
      break;

    default:
      syslog(LOG_WARNING, "power_util: wrong option");

  }

  return ret;
}

static int
add_process_running_flag(uint8_t slot_id, uint8_t opt) {
  int pid_file;
  char path[128];

  if (opt == PWR_STATUS) {
    return opt;
  } else {
    sprintf(path, PWR_UTL_LOCK, slot_id);
    pid_file = open(path, O_CREAT | O_RDWR, 0666);
    if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
      return (-opt);
    }
  }

  return opt;
}

static void
rm_process_running_flag(uint8_t slot_id, uint8_t opt) {
  char path[128];

  if (opt != PWR_STATUS) {
    sprintf(path, PWR_UTL_LOCK, slot_id);
    remove(path);
  }
}

int
main(int argc, char **argv) {

  int ret;

  uint8_t fru, status, opt;
  char *option;
  uint8_t num_devs = 0;
  uint8_t dev_id = DEV_NONE;

  if (argc > 2) {
    ret = pal_get_fru_id(argv[1], &fru);
    if (ret < 0) {
      printf("Wrong fru: %s\n", argv[1]);
      print_usage();
      exit(-1);
    }
  } else {
    fru = -1;
  }

  pal_get_num_devs(fru,&num_devs);

  /* Check for sled-cycle */
  if (argc < 2 || argc > 3) {
    if ( argc != 4 || num_devs == 0) {
      print_usage();
      exit (-1);
    }
  }

  if (argc > 2) {
    ret = pal_is_fru_prsnt(fru, &status);
    if (ret < 0) {
      printf("pal_is_fru_prsnt failed for fru: %d\n", fru);
      print_usage();
      exit(-1);
    }
    if (status == 0) {
      printf("%s is empty!\n", argv[1]);
      print_usage();
      exit(-1);
    }
  }

  if (argc == 4) {
    ret = pal_get_dev_id(argv[2], &dev_id);
    if (ret < 0 || dev_id == DEV_ALL) {
      printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[2]);
      print_usage();
      exit(-1);
    }
  }

  option =  argc == 2 ? argv[1] : argc == 3 ? argv [2] : argv[3];

  ret = get_power_opt(option, &opt);
  /* If argc is 2, the option is sled-cycle;  we should ignore power-util fru sled-cycle*/
  if ((ret < 0) || (argc == 2 && opt != PWR_SLED_CYCLE) || (argc == 3 && opt == PWR_SLED_CYCLE)) {
    printf("Wrong option: %s\n", option);
    print_usage();
    exit(-1);
  }

  if (argc == 4 && opt != PWR_ON && opt != PWR_OFF && opt != PWR_STATUS && opt != PWR_CYCLE) {
    printf("Wrong option for %s: %s\n",argv [2] ,option);
    print_usage();
    exit(-1);
  }

  // Check if another instance is running
  if (add_process_running_flag(fru, opt) < 0) {
    printf("power_util: another instance is running for FRU:%d...\n",fru);
    //Make power-util exit code to "-2" when another instance is running
    exit(-2);
  }

  if (dev_id == DEV_NONE) {
    ret = power_util(fru, opt);
    if (ret < 0) {
      printf("ERROR: power-util fru[%d] [%s] failed\n", fru, option_list[opt]);
    }
  } else if (dev_id != DEV_ALL) {
    ret = dev_power_util(fru, dev_id, opt);
    if (ret < 0) {
      printf("ERROR: power-util fru[%d] dev[%d] [%s] failed\n", fru, dev_id-1, option_list[opt]);
    }
  }

  rm_process_running_flag(fru, opt);
  return ret;
}
