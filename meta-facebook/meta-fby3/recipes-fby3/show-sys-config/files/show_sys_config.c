/*
 * show_bmc_config
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
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <facebook/fby3_common.h>
#include <jansson.h>

#define MAX_RETRY 3

#define GET_BIT(data, index) ((data >> index) & 0x1)

bool verbosed = false;
bool output_json = false;

enum {
  UTIL_EXECUTION_OK = 0,
  UTIL_EXECUTION_FAIL = -1,
};

enum {
  CLASS1 = 1,
  CLASS2,
  UNKNOWN_CONFIG = 0xff,
};

enum {
  SERVER_PRSNT = 0,
  SERVER_NOT_PRSNT,
  SERVER_ABNORMAL_STATE,
  M2_BOARD_PRSNT = 0,
  M2_BOARD_NOT_PRSNT,
};

typedef struct server {
  uint8_t is_server_present;
  uint8_t config;
  uint8_t is_1ou_present;
  uint8_t is_2ou_present;
} server_conf;

typedef struct conf {
  uint8_t type;
  uint8_t sys_config;
  uint8_t fru_cnt;
  server_conf server_info[4];
} sys_conf;

char *prsnt_sts[] = {"Present", "Not Present", "Abnormal state"};

static int
get_server_config(uint8_t slot_id, uint8_t *data) {
  int ret = UTIL_EXECUTION_FAIL; 
  int retry = MAX_RETRY;
  uint8_t tbuf[4] = {0x05, 0x42, 0x01, 0x0d};
  uint8_t rbuf[1] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  while ( ret < 0 && retry-- > 0 ) {
    ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  }

  if ( ret < 0 ) {
    printf("BIC no response!\n");
    return UTIL_EXECUTION_FAIL;
  }

  *data = rbuf[0];
  return UTIL_EXECUTION_OK;
}

static int
show_sys_configuration(sys_conf system) {
  uint8_t fru_cnt = system.fru_cnt;
  uint8_t i = 0;
  uint8_t sts = 0;
  char config_str[8] = "\0";
  json_t *root = NULL;
  json_t *server_info = NULL;
  json_t *data[4] = {NULL, NULL, NULL, NULL};

  if ( system.sys_config == UNKNOWN_CONFIG ) {
    snprintf(config_str, sizeof(config_str), "%s", "Unknown");
  } else {
    snprintf(config_str, sizeof(config_str), "%X", system.sys_config);
  } 

  if ( output_json == true ) {
    //create the object
    root = json_object();
    server_info  = json_object();
    
    //set the basic configuration
    json_object_set_new(root, "Type", json_string((system.type == CLASS1)?"Class 1":"Class 2"));
    json_object_set_new(root, "Config", json_string(config_str));    

    //prepare the data
    for ( i = FRU_SLOT1; i <= fru_cnt; i++) {
      //create the object
      data[i-1] = json_object();
      sts = system.server_info[i-1].is_server_present;
      if ( sts != SERVER_PRSNT ) {
        json_object_set_new(data[i-1], "Type", json_string("Not Present"));
      } else {        
        snprintf(config_str, sizeof(config_str), "%X", system.server_info[i-1].config);
        json_object_set_new(data[i-1], "Type", json_string(config_str));

        sts = system.server_info[i-1].is_1ou_present;
        json_object_set_new(data[i-1], "FrontExpansionCard", json_string(prsnt_sts[sts]));

        sts = system.server_info[i-1].is_2ou_present;
        json_object_set_new(data[i-1], "RiserExpansionCard", json_string(prsnt_sts[sts]));
      }
      snprintf(config_str, sizeof(config_str), "slot%X", i);
      //set up the object info
      json_object_set_new(server_info, config_str, data[i-1]);
    }

    //set up the root info
    json_object_set_new(root, "server_config", server_info);
    json_dumpf(root, stdout, 4);

    //release the resource
    for ( i = FRU_SLOT1; i <= fru_cnt; i++ ) {
      json_decref(data[i-1]);
    }
    json_decref(server_info);
    json_decref(root);
  } else {
    printf("Type : Class %d, Config: %s\n", system.type, config_str);
    printf("---------------------------\n");
    for ( i = FRU_SLOT1; i <= fru_cnt; i++) {
      sts = system.server_info[i-1].is_server_present;
      if ( sts != SERVER_PRSNT ) {
        printf("Slot%d : %s\n\n", i, prsnt_sts[sts]);
        continue;
      }
    
      printf("Slot%d : %s, Config %X\n", i, prsnt_sts[sts], system.server_info[i-1].config);
      if ( verbosed == true ) {
        sts = system.server_info[i-1].is_1ou_present;
        printf("  FrontExpansionCard: %s\n", prsnt_sts[sts]);
        sts = system.server_info[i-1].is_2ou_present;
        printf("  RiserExpansionCard: %s\n", prsnt_sts[sts]);
      }
      printf("\n");
    }
  }

  return UTIL_EXECUTION_OK;
}

int
main(int argc, char **argv) {
  uint8_t bmc_location = 0;
  uint8_t is_fru_present = 0;
  int ret = UTIL_EXECUTION_OK;
  int i = 0;
  uint8_t total_fru = 0;
  uint8_t data = 0;
  uint8_t check_sys_config = UNKNOWN_CONFIG;
  sys_conf sys_info = {0};

  i = 1;
  while ( argc >= 2 && i < argc) {
    if ( strcmp("-v", argv[i]) == 0 ) {
      verbosed = true;
    } else if ( strcmp("-json", argv[i]) == 0) {
      output_json = true;
    }
    i++;
  }

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Cannot get the location of BMC", __func__);
    return UTIL_EXECUTION_FAIL;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    total_fru = FRU_SLOT4;
    sys_info.type = CLASS1;
  } else {
    total_fru = FRU_SLOT1;
    sys_info.type = CLASS2;
  }

  sys_info.fru_cnt = total_fru;

  for ( i = FRU_SLOT1; i <= total_fru; i++ ) {
    ret = fby3_common_is_fru_prsnt(i, &is_fru_present);
    if ( ret < 0 || is_fru_present == 0 ) {
      //the server is not present
      sys_info.server_info[i-1].is_server_present = SERVER_NOT_PRSNT; 
    } else {
      ret = get_server_config(i, &data);
      if ( ret < 0 ) {
        sys_info.server_info[i-1].is_server_present = SERVER_ABNORMAL_STATE;
      } else {
        uint8_t front_exp_bit = GET_BIT(data, 2);
        uint8_t riser_exp_bit = GET_BIT(data, 3);
        uint8_t server_config = UNKNOWN_CONFIG;
        sys_info.server_info[i-1].is_server_present = SERVER_PRSNT;
        if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
          sys_info.server_info[i-1].is_1ou_present = (front_exp_bit == 0)?M2_BOARD_PRSNT:M2_BOARD_NOT_PRSNT;
        } else {
          sys_info.server_info[i-1].is_1ou_present = M2_BOARD_NOT_PRSNT;
          front_exp_bit = M2_BOARD_NOT_PRSNT;
        }

        sys_info.server_info[i-1].is_2ou_present = (riser_exp_bit == 0)?M2_BOARD_PRSNT:M2_BOARD_NOT_PRSNT;

        if ( (front_exp_bit == riser_exp_bit) && (front_exp_bit == 0) ) {
          server_config = 0xD; 
        } else if ( front_exp_bit > riser_exp_bit ) {
          server_config = 0xC;
        } else if ( front_exp_bit < riser_exp_bit ) {
          server_config = 0xB;
        } else {
          server_config = 0xA;
        }
        sys_info.server_info[i-1].config = server_config; 
      }
    }
  }

  //check the sys config  
  for ( i = FRU_SLOT1; i <= total_fru; i++ ) {
    if ( sys_info.server_info[i-1].is_server_present != SERVER_PRSNT ) {
      continue;
    }

    if ( check_sys_config == UNKNOWN_CONFIG ) {
      check_sys_config = sys_info.server_info[i-1].config;
    } else {
      if ( check_sys_config != sys_info.server_info[i-1].config ) {
        check_sys_config = UNKNOWN_CONFIG;
        break;
      }
    } 
  }

  sys_info.sys_config = check_sys_config;

  return show_sys_configuration(sys_info);
}
