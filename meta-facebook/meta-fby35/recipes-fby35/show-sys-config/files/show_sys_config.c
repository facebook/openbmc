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
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <jansson.h>

#define MAX_RETRY 3

enum {
  CLASS1 = 1,
  CLASS2,
  UNKNOWN_CONFIG = 0xff,
};

typedef struct server {
  uint8_t is_server_present;
  uint8_t config;
  uint8_t is_mgmt_cbl_present;
  uint8_t cbl_sts_val;
  uint8_t is_1ou_present;
  uint8_t is_2ou_present;
} server_conf;

typedef struct conf {
  uint8_t type;
  uint8_t sys_config;
  uint8_t fru_cnt;
  server_conf server_info[4];
} sys_conf;

static const char *prsnt_sts[] = {"Present", "Not Present", "Abnormal state"};

static bool verbosed = false;
static bool output_json = false;

static int
get_server_config(uint8_t slot_id, uint8_t *data, uint8_t bmc_location) {
  int ret = UTIL_EXECUTION_FAIL;
  int retry = MAX_RETRY;
  uint8_t tbuf[4] = {0x01, 0x42, 0x01, 0x05};
  uint8_t rbuf[1] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  uint8_t bic_ready;

  ret = fby35_common_is_bic_ready(slot_id, &bic_ready);
  if ( ret < 0 || bic_ready != 1 ) {
    return UTIL_EXECUTION_FAIL;
  }

  do {
    ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  } while ( ret < 0 && retry-- > 0 );

  if ( ret < 0 ) {
    return UTIL_EXECUTION_FAIL;
  }

  data[0] = rbuf[0];
  
  ret = pal_check_sled_mgmt_cbl_id(slot_id, tbuf, bmc_location);
  if ( ret < 0 ) {
    return UTIL_EXECUTION_FAIL;
  }

  data[1] = tbuf[0];
  data[2] = tbuf[1];
  return UTIL_EXECUTION_OK;
}

static int
show_sys_configuration(sys_conf *system) {
  uint8_t fru_cnt = system->fru_cnt;
  uint8_t i = 0;
  uint8_t sts = 0;
  char config_str[8] = "\0";
  char cable_str[64] = "\0";
  json_t *root = NULL;
  json_t *server_info = NULL;
  json_t *data[4] = {NULL, NULL, NULL, NULL};

  if ( system->sys_config == UNKNOWN_CONFIG ) {
    snprintf(config_str, sizeof(config_str), "%s", "Unknown");
  } else {
    snprintf(config_str, sizeof(config_str), "%X", system->sys_config);
  }

  if ( output_json == true ) {
    //create the object
    root = json_object();
    server_info  = json_object();

    //set the basic configuration
    json_object_set_new(root, "Type", json_string((system->type == CLASS1)?"Class 1":"Class 2"));
    json_object_set_new(root, "Config", json_string(config_str));

    //prepare the data
    for ( i = FRU_SLOT1; i <= fru_cnt; i++) {
      if ( (system->sys_config == 0xB) || (system->sys_config == 0xD) ) {
        if ( i == FRU_SLOT2 || i == FRU_SLOT4) {
          continue;
        }
      }
      //create the object
      data[i-1] = json_object();
      sts = system->server_info[i-1].is_server_present;
      if ( sts != STATUS_PRSNT ) {
        json_object_set_new(data[i-1], "Type", json_string("Abnormal - slot not detected"));
      } else {
        snprintf(config_str, sizeof(config_str), "%X", system->server_info[i-1].config);
        json_object_set_new(data[i-1], "Type", json_string(config_str));

        sts = system->server_info[i-1].is_mgmt_cbl_present;
        if (sts == STATUS_ABNORMAL) {
          snprintf(cable_str, sizeof(cable_str), "Abnormal - Slot%d instead of Slot%d", (system->server_info[i-1].cbl_sts_val >> 4), (system->server_info[i-1].cbl_sts_val & 0x0f));
        } else {
          snprintf(cable_str, sizeof(cable_str), "%s", prsnt_sts[sts]);
        }
        json_object_set_new(data[i-1], "SledManagementCable", json_string(cable_str));

        sts = system->server_info[i-1].is_1ou_present;
        json_object_set_new(data[i-1], "FrontExpansionCard", json_string(prsnt_sts[sts]));

        sts = system->server_info[i-1].is_2ou_present;
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
    printf("Type : Class %d, Config: %s\n", system->type, config_str);
    printf("---------------------------\n");
    for ( i = FRU_SLOT1; i <= fru_cnt; i++) {
      if ( (system->sys_config == 0xB) || (system->sys_config == 0xD) ) {
        if ( i == FRU_SLOT2 || i == FRU_SLOT4) {
          continue;
        }
      }
      sts = system->server_info[i-1].is_server_present;
      if ( sts != STATUS_PRSNT ) {
        printf("Slot%d : Abnormal - slot not detected\n\n", i);
        continue;
      }

      printf("Slot%d : %s, Config %X\n", i, prsnt_sts[sts], system->server_info[i-1].config);
      if ( verbosed == true ) {
        sts = system->server_info[i-1].is_mgmt_cbl_present;
        if (sts == STATUS_ABNORMAL) {
          snprintf(cable_str, sizeof(cable_str), "Abnormal - Slot%d instead of Slot%d", (system->server_info[i-1].cbl_sts_val >> 4), (system->server_info[i-1].cbl_sts_val & 0x0f));
        } else {
          snprintf(cable_str, sizeof(cable_str), "%s", prsnt_sts[sts]);
        }
        printf("    SledManagementCable: %s\n", cable_str);
        sts = system->server_info[i-1].is_1ou_present;
        printf("     FrontExpansionCard: %s\n", prsnt_sts[sts]);
        sts = system->server_info[i-1].is_2ou_present;
        printf("     RiserExpansionCard: %s\n", prsnt_sts[sts]);
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
  uint8_t data[3] = {0};
  uint8_t check_sys_config = UNKNOWN_CONFIG;
  sys_conf sys_info = {0};

  for ( i = 1; i < argc; i++ ) {
    if ( strcmp("-v", argv[i]) == 0 ) {
      verbosed = true;
    } else if ( strcmp("-json", argv[i]) == 0 ) {
      output_json = true;
    }
  }

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Cannot get the location of BMC", __func__);
    return UTIL_EXECUTION_FAIL;
  }

  if ( bmc_location == BB_BMC ) {
    total_fru = FRU_SLOT4;
    sys_info.type = CLASS1;
  } else {
    total_fru = FRU_SLOT1;
    sys_info.type = CLASS2;
  }

  sys_info.fru_cnt = total_fru;

  for ( i = FRU_SLOT1; i <= total_fru; i++ ) {
    ret = pal_is_fru_prsnt(i, &is_fru_present);
    if ( ret < 0 || is_fru_present == 0 ) {
      //the server is not present
      sys_info.server_info[i-1].is_server_present = STATUS_NOT_PRSNT;
    } else {
      ret = get_server_config(i, data, bmc_location);
      if ( ret < 0 ) {
        sys_info.server_info[i-1].is_server_present = STATUS_ABNORMAL;
      } else {
        uint8_t front_exp_bit = GETBIT(data[0], 2);
        uint8_t riser_exp_bit = GETBIT(data[0], 3);
        uint8_t server_config = UNKNOWN_CONFIG;

        sys_info.server_info[i-1].is_server_present = STATUS_PRSNT;
        sys_info.server_info[i-1].is_mgmt_cbl_present = data[1];
        sys_info.server_info[i-1].cbl_sts_val = data[2];

        if ( sys_info.type == CLASS1 ) {
          sys_info.server_info[i-1].is_1ou_present = (front_exp_bit == 0)?STATUS_PRSNT:STATUS_NOT_PRSNT;
        } else {
          sys_info.server_info[i-1].is_1ou_present = STATUS_NOT_PRSNT;
          front_exp_bit = STATUS_NOT_PRSNT;
        }

        sys_info.server_info[i-1].is_2ou_present = (riser_exp_bit == 0)?STATUS_PRSNT:STATUS_NOT_PRSNT;

        if ( sys_info.type == CLASS2 ) {
          server_config = 0xD;
        } else if ( riser_exp_bit == STATUS_PRSNT ) {
          server_config = 0xB;
        } else if ( front_exp_bit == STATUS_PRSNT && riser_exp_bit == STATUS_NOT_PRSNT ) {
          server_config = 0xC;
        } else {
          server_config = 0xA;
        }
        sys_info.server_info[i-1].config = server_config;
      }
    }
  }

  //check the sys config
  for ( i = FRU_SLOT1; i <= total_fru; i++ ) {
    if ( sys_info.server_info[i-1].is_server_present != STATUS_PRSNT ) {
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

  return show_sys_configuration(&sys_info);
}
