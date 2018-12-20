/*
 * hsvc-util
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
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/file.h>
#include <facebook/bic.h>
#include <facebook/fby2_common.h>
#include <facebook/fby2_sensor.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

#define HSVC_UTL_LOCK "/var/run/hsvc-util_%d.lock"
#define HSVC_START_LOCK "/tmp/hsvc-start-slot%d.lock"
#define MAX_NUM_OPTIONS 5
enum {
  OPT_SLOT1 = 1,
  OPT_SLOT2 = 2,
  OPT_SLOT3 = 3,
  OPT_SLOT4 = 4,
  OPT_ALL   = 5,
};

slot_kv_st slot_kv_list[] = {
  // {slot_key, slot_def_val}
  {"pwr_server%d_last_state", "on"},
  {"sysfw_ver_slot%d",         "0"},
  {"slot%d_por_cfg",         "lps"},
  {"slot%d_sensor_health",     "1"},
  {"slot%d_sel_error",         "1"},
  {"slot%d_boot_order",  "0000000"},
  {"slot%d_cpu_ppin",          "0"},
  {"fru%d_restart_cause",      "3"},
};

static void
print_usage_help(void) {
  printf("Usage: hsvc-util <all|slot1|slot2|slot3|slot4> <--start | --stop>\n");
}

int
activate_hsvc(uint8_t slot_id) {
  char cmd[128] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int pair_set_type;
  int pair_slot_id;
  int runoff_id = slot_id;
  int ret=-1;
  uint8_t status, i;
  char tstr[64] = {0};

  if (0 == slot_id%2)
    pair_slot_id = slot_id - 1;
  else
    pair_slot_id = slot_id + 1;

  pair_set_type = pal_get_pair_slot_type(slot_id);

  if (0 != slot_id%2) {
     switch(pair_set_type) {
        case TYPE_SV_A_SV:
        case TYPE_SV_A_CF:
        case TYPE_SV_A_GP:
        case TYPE_CF_A_CF:
        case TYPE_CF_A_GP:
        case TYPE_GP_A_CF:
        case TYPE_GP_A_GP:
           // do nothing
           break;
        case TYPE_CF_A_SV:
        case TYPE_GP_A_SV:
        case TYPE_GPV2_A_SV:
           // Need to 12V-off pair slot first to avoid accessing device card error
           runoff_id = pair_slot_id;
           break;
     }
  }

  if ( runoff_id != slot_id ) {
     printf("SLOT%u is device and SLOT%u is server\n",slot_id,pair_slot_id);
     printf("Need to power off SLOT%u first\n",runoff_id);
  }

  if ( SLOT_TYPE_SERVER == fby2_get_slot_type(runoff_id) ) {
     ret = pal_is_server_12v_on(runoff_id, &status);
     if (ret < 0) {
       syslog(LOG_ERR, "%s: pal_is_server_12v_on failed", __func__);
       return -1;
     }

     if (status) {
       printf("Delay 30s for graceful-shutdown\n");
       sprintf(cmd, "/usr/local/bin/power-util slot%u graceful-shutdown", runoff_id);
       ret = run_command(cmd);
       if (0 == ret) {
         for (i = 0; i < 10; i++) {
           sleep(3);
           ret = pal_get_server_power(runoff_id, &status);
           if (!ret && !status) {
              sleep(3);
              break;
           }
         }
       } else {
         return -1;
       }
     }
  }

  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-off", runoff_id);
  ret = run_command(cmd);
  if (0 == ret) {
    printf("SLED is ready to be pulled out for hot-service now\n");
  } else {
    printf("Fail to turn slot%d to 12V-off\n", runoff_id);
    return -1;
  }

  sprintf(key, "slot%d_por_cfg", runoff_id);
  if (pal_get_key_value(key, cvalue) == 0) {
    if (!memcmp(cvalue, "lps",  strlen("lps"))) {
      ret = pal_set_last_pwr_state(runoff_id, "on");
      if (ret < 0) {
        printf("Failed to set slot power status to on during Hot-service\n");
        return ret;
      }
    }
  } else {
    printf("Failed to get slot %d power policy\n", runoff_id);
    return -1;
  }

  sprintf(tstr, "identify_slot%d", slot_id);
  ret = pal_set_key_value(tstr, "on");
  if (ret < 0) {
    syslog(LOG_ERR, "pal_set_key_value: set %s on failed",tstr);
    return -1;
  }

  return 0;
}

static int
kv_list_load_default(uint8_t slot_id) {
  int i;
  int flag = 0;
  char slot_kv[80] = {0};
  int ret = 0;

  // Re-init kv list
  for(i=0; i < sizeof(slot_kv_list)/sizeof(slot_kv_st); i++) {
    memset(slot_kv, 0, sizeof(slot_kv));
    sprintf(slot_kv, slot_kv_list[i].slot_key, slot_id);
    if ((ret = pal_set_key_value(slot_kv, slot_kv_list[i].slot_def_val)) < 0) {
      syslog(LOG_WARNING, "%s %s: kv_set failed. %d", __func__, slot_kv_list[i].slot_key, ret);
      flag = 1;
    }
  }

  if (1 == flag) {
    return -1;
  } else {
    return 0;
  }
}

static int
add_process_running_flag(uint8_t slot_id) {
  int pid_file;
  char path[128];

  sprintf(path, HSVC_UTL_LOCK, slot_id);
  pid_file = open(path, O_CREAT | O_RDWR, 0666);
  if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    return -1;
  }

  return 0;
}

static void
rm_process_running_flag(uint8_t slot_id) {
  char path[128];

  sprintf(path, HSVC_UTL_LOCK, slot_id);
  remove(path);
}

int
main(int argc, char **argv) {
  int i;
  uint8_t slot_id;
  int ret = 0;
  uint8_t flag = 0;
  char tstr[64] = {0};
  char cmd[128] = {0};

  // Check for border conditions
  if ((argc != 3)) {
    goto err_exit;
  }

  // Derive slot_id from first parameter
  if (!strcmp(argv[1], "slot1")) {
    slot_id = OPT_SLOT1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = OPT_SLOT2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = OPT_SLOT3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = OPT_SLOT4;
  } else if (!strcmp(argv[1] , "all"))   {
    slot_id = OPT_ALL;
  } else {
      goto err_exit;
  }

  if (slot_id < OPT_ALL) {
    // Check if another instance is running (all)
    if (add_process_running_flag(OPT_ALL) < 0) {
      printf("hsvc_util: another instance is running for FRU: all ...\n");
      //Make hsvc-util exit code to "-2" when another instance is running
      exit(-2);
    } else {
      rm_process_running_flag(OPT_ALL);
    }

    // Check if another instance is running
    if (add_process_running_flag(slot_id) < 0) {
      printf("hsvc_util: another instance is running for FRU:%d ...\n", slot_id);
      //Make hsvc-util exit code to "-2" when another instance is running
      exit(-2);
    }
  }

  if (OPT_ALL == slot_id) {
    for(i = OPT_SLOT1; i < MAX_NUM_OPTIONS; i++) {
      // Check if another instance is running (all)
      if (add_process_running_flag(i) < 0) {
        printf("hsvc_util: another instance is running for FRU:%d ...\n", i);
        //Make hsvc-util exit code to "-2" when another instance is running
        exit(-2);
      } else {
        rm_process_running_flag(i);
      }
    }

    // Check if another instance is running (all)
    if (add_process_running_flag(slot_id) < 0) {
      printf("hsvc_util: another instance is running for FRU: all ...\n");
      //Make hsvc-util exit code to "-2" when another instance is running
      exit(-2);
    }
  }

  // check operation to perform
  if (!strcmp(argv[2], "--start")) {

     if (slot_id < OPT_ALL) {
        pal_set_hsvc_ongoing(slot_id, 1, 0);
        ret = activate_hsvc(slot_id);
        rm_process_running_flag(slot_id);
        if (ret != 0) {
          printf("hsvc-util: Fail to activate hot service control for FRU:%d ...\n", slot_id);
          goto err_exit;
        } else {
          sprintf(tstr, HSVC_START_LOCK, slot_id);
          memset(cmd, 0, sizeof(cmd));
          sprintf(cmd, "touch %s", tstr);
          run_command(cmd);
          return 0;
        }
     }

     // OPT_ALL
     flag = 0;
     for (slot_id = OPT_SLOT1; slot_id < MAX_NUM_OPTIONS; slot_id++) {
        pal_set_hsvc_ongoing(slot_id, 1, 0);
        ret = activate_hsvc(slot_id);
        if (ret != 0) {
          printf("hsvc-util: Fail to activate hot service control for FRU:%d ...\n", slot_id);
          flag = 1;
        } else {
          sprintf(tstr, HSVC_START_LOCK, slot_id);
          memset(cmd, 0, sizeof(cmd));
          sprintf(cmd, "touch %s", tstr);
          run_command(cmd);
        }
     }
     rm_process_running_flag(OPT_ALL);
     if (1 == flag) {
       goto err_exit;
     } else {
       return 0;
     }
  } else if (!strcmp(argv[2], "--stop")) {
     if (slot_id < OPT_ALL) {
       // Check if --start is activated befor
       sprintf(tstr, HSVC_START_LOCK, slot_id);
       if (access(tstr, F_OK) != 0) {
         printf("hsvc-util: hsvc-util --start should be activated before hsvc-util --stop ...\n");
         rm_process_running_flag(slot_id);
         goto err_exit;
       }

       if (kv_list_load_default(slot_id) == -1) {
         printf("hsvc-util: Fail to set key value to default for FRU:%d ...\n", slot_id);
       }

       memset(cmd, 0, sizeof(cmd));
       sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-on", slot_id);
       ret = run_command(cmd);
       if (0 == ret) {
         printf("12V on is successful for FRU:%d ...\n", slot_id);
       } else {
         printf("Fail to turn slot%d to 12V-on\n", slot_id);
         rm_process_running_flag(slot_id);
         goto err_exit;
       }

       ret = pal_set_hsvc_ongoing(slot_id, 0, 1);
       rm_process_running_flag(slot_id);
       if (ret != 0) {
         printf("hsvc-util: Fail to stop hot service control for FRU:%d ...\n", slot_id);
         goto err_exit;
       } else {
         sprintf(tstr, HSVC_START_LOCK, slot_id);
         memset(cmd, 0, sizeof(cmd));
         sprintf(cmd, "rm -rf %s", tstr);
         run_command(cmd);
         printf("Hot service control is finished ...\n");
         return 0;
       }
     }

     // OPT_ALL
     flag = 0;
     for (slot_id = OPT_SLOT1; slot_id < MAX_NUM_OPTIONS; slot_id++) {
       // Check if --start is activated before
       sprintf(tstr, HSVC_START_LOCK, slot_id);
       if (access(tstr, F_OK) != 0) {
         printf("hsvc-util: hsvc-util --start should be activated before hsvc-util --stop ...\n");
         flag = 1;
         continue;
       }

       if (kv_list_load_default(slot_id) == -1) {
         printf("hsvc-util: Fail to set key value to default for FRU:%d ...\n", slot_id);
       }

       memset(cmd, 0, sizeof(cmd));
       sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-on", slot_id);
       ret = run_command(cmd);
       if (0 == ret) {
         printf("12V on is successful for FRU:%d ...\n", slot_id);
       } else {
         printf("Fail to turn slot%d to 12V-on\n",slot_id);
         flag = 1;
         continue;
       }

       ret = pal_set_hsvc_ongoing(slot_id, 0, 1);
       if (ret != 0) {
         printf("hsvc-util: Fail to stop hot service control for FRU:%d ...\n", slot_id);
         flag = 1;
         continue;
       }

       sprintf(tstr, HSVC_START_LOCK, slot_id);
       memset(cmd, 0, sizeof(cmd));
       sprintf(cmd, "rm -rf %s", tstr);
       run_command(cmd);
     }

     rm_process_running_flag(OPT_ALL);
     if (1 == flag) {
       goto err_exit;
     } else {
       printf("Hot service control is finished ...\n");
       return 0;
     }
  }

err_exit:
  print_usage_help();
  return -1;
}
