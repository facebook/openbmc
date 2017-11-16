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

#define MAX_NUM_OPTIONS 5
enum {
  OPT_SLOT1 = 1,
  OPT_SLOT2 = 2,
  OPT_SLOT3 = 3,
  OPT_SLOT4 = 4,
  OPT_ALL   = 5,
};

static void
print_usage_help(void) {
  printf("Usage: hsvc-util <all|slot1|slot2|slot3|slot4> <--start | --stop>\n");
}


int
activate_hsvc(uint8_t slot_id) {
  char cmd[128] = {0};
  int pair_set_type;  
  int pair_slot_id;
  int runoff_id = slot_id;
  int ret=-1;
  uint8_t status;
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
           // Need to 12V-off pair slot first to avoid accessing device card error 
           if (status) {
             runoff_id = pair_slot_id;
           }
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
         sleep(30);
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
    printf("Fail to turn slot%d to 12V-off\n",slot_id);
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

int
main(int argc, char **argv) {

  uint8_t slot_id;
  int ret = 0;
  char cmd[80];
  char tstr[64] = {0};
  uint8_t flag = 0;

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
  
  // check operation to perform
  if (!strcmp(argv[2], "--start")) {
     
     if (slot_id < OPT_ALL) {
        ret = activate_hsvc(slot_id);
        if (ret != 0) {
          goto err_exit;
        } else {
          return 0;
        }
     }

     for (slot_id = OPT_SLOT1; slot_id < MAX_NUM_OPTIONS; slot_id++) {
        ret |= activate_hsvc(slot_id);
     }
     if (ret != 0) {
       goto err_exit;
     } else {
       return 0;
     }
  } else if (!strcmp(argv[2], "--stop")) {
     if (slot_id < OPT_ALL) {
       sprintf(tstr, "identify_slot%d", slot_id);
       ret = pal_set_key_value(tstr, "off");
       if (ret < 0) {
         syslog(LOG_ERR, "pal_set_key_value: set %s off failed",tstr);
         goto err_exit;
       } else {
         return 0;
       }
     }

     for (slot_id = OPT_SLOT1; slot_id < MAX_NUM_OPTIONS; slot_id++) {
        sprintf(tstr, "identify_slot%d", slot_id);
        ret = pal_set_key_value(tstr, "off");
        if (ret < 0) {
          syslog(LOG_ERR, "pal_set_key_value: set %s off failed",tstr);
          flag = 1;
        }
     }
     if (flag == 1) {
       goto err_exit;
     } else {
       return 0;
     }
  }

err_exit:
  print_usage_help();
  return -1;
}
