/*
 * enclosure-util
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
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <facebook/fbgc_common.h>
#include <openbmc/pal.h>
#include <facebook/exp.h>
#include "enclosure.h"

#define MAX_NUM_HDD_SLOT         36
#define MAX_HDD_ID_STR_LEN       8    // include the string terminal
#define MAX_HDD_STATUS_LIST_LEN  108  // include the string terminal

#define HDD_STATUS_NORMAL        0x01
#define HDD_STATUS_MISSING       0x05

#define I2C_DEV_E1S_0_PATH       "/dev/i2c-12"
#define I2C_DEV_E1S_1_PATH       "/dev/i2c-13"

static void
print_usage_help(void) {
  uint8_t chassis_type = 0;

  printf("Usage: enclosure-util --error\n");
  printf("         Expander error code range: 1~100\n");
  printf("         BMC error code range:      0x64~0xFF\n");
  printf("       enclosure-util --hdd-status\n");
  printf("       enclosure-util --hdd-status <hdd id 0~%d>\n", (MAX_NUM_HDD_SLOT - 1));
  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get chassis type. \n", __func__);
    return;
  }
  
  if (chassis_type == CHASSIS_TYPE5) {
    printf("       enclosure-util --e1s-health\n");
    printf("       enclosure-util --e1s-status\n");
  }
  
}

void
show_error_code() {
  uint8_t error[MAX_NUM_ERR_CODES] = {0}, error_count = 0;
  int i = 0, ret = 0;
  
  memset(error, 0, sizeof(error));
  
  ret = pal_get_error_code(error, &error_count);
  if (ret < 0) {
    printf("enclosure-util: fail to get error code\n");
    return;
  }
  
  if (error_count == 0){
    printf("Error Counter: 0 (No Error)\n");
  } else {
    printf("Error Counter: %d\n", error_count);

    for (i = 0; i < error_count; i++) {
      // Expander error: 0~99
      if (error[i] < MAX_NUM_EXP_ERR_CODES) {
        printf("Error Code %0d: ", error[i]);
        printf("%s", exp_error_code_description[error[i]]);
      
      // BMC error: 0x64(100)~0xFF(255)
      } else {
        printf("Error Code 0x%0X: ", error[i]);
        printf("%s", bmc_error_code_description[error[i] - BMC_ERR_CODE_START_NUM]);
      }
      printf("\n");
    }
  }
}

void
show_hdd_status(int hdd_id) {
  int ret = 0, i = 0, strncat_len = 0;
  char hdd_id_str[MAX_HDD_ID_STR_LEN] = {0};
  char normal_hdd_list[MAX_HDD_STATUS_LIST_LEN] = {0};
  char abnormal_hdd_list[MAX_HDD_STATUS_LIST_LEN] = {0};
  char missing_hdd_list[MAX_HDD_STATUS_LIST_LEN] = {0};
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00}, tlen = 0;
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00}, rlen = 0;
  hdd_status_res hdd_status_resp[MAX_NUM_HDD_SLOT] = {0};
  
  memset(hdd_id_str, 0, sizeof(hdd_id_str));
  memset(tbuf, 0, sizeof(tbuf));
  memset(rbuf, 0, sizeof(rbuf));
  memset(normal_hdd_list, 0, sizeof(normal_hdd_list));
  memset(abnormal_hdd_list, 0, sizeof(abnormal_hdd_list));
  memset(missing_hdd_list, 0, sizeof(missing_hdd_list));
  memset(&hdd_status_resp, 0, sizeof(hdd_status_resp));
  
  // get HDD status
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_GET_HDD_STATUS, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    printf("enclosure-util: failed to get HDD status, Expander IPMB command NetFn: 0x%2X Code: 0x%02X was error\n", NETFN_OEM_REQ, CMD_OEM_EXP_GET_HDD_STATUS);
    return;
  }

  if (sizeof(hdd_status_resp) != rlen) {
    syslog(LOG_ERR, "%s(): failed to get HDD status. Get length: %d is wrong, expected length: %d\n", __func__, rlen, sizeof(hdd_status_resp));
    return;
  }
  
  memcpy(&hdd_status_resp, rbuf, sizeof(hdd_status_resp));
  
  if (hdd_id == -1) {
    for (i = 0; i < MAX_NUM_HDD_SLOT; i++) {
      snprintf(hdd_id_str, sizeof(hdd_id_str), "%d ", i);
      
      if (hdd_status_resp[i].element_status[0] == HDD_STATUS_NORMAL) {
        strncat_len = sizeof(normal_hdd_list) - strlen(normal_hdd_list) - 1;
        strncat(normal_hdd_list, hdd_id_str, strncat_len);
      } else if (hdd_status_resp[i].element_status[0] == HDD_STATUS_MISSING) {
        strncat_len = sizeof(missing_hdd_list) - strlen(missing_hdd_list) - 1;
        strncat(missing_hdd_list, hdd_id_str, strncat_len);
      } else {
        strncat_len = sizeof(abnormal_hdd_list) - strlen(abnormal_hdd_list) - 1;
        strncat(abnormal_hdd_list, hdd_id_str, strncat_len);
      } 
    }
    
    printf("Normal HDD ID: %s\n", normal_hdd_list);
    printf("Abnormal HDD ID: %s\n", abnormal_hdd_list);
    printf("Missing HDD ID: %s\n", missing_hdd_list);
    
  } else {
    printf("HDD_%d Status: %02X %02X %02X %02X\n", hdd_id, 
      hdd_status_resp[hdd_id].element_status[0], hdd_status_resp[hdd_id].element_status[1], 
      hdd_status_resp[hdd_id].element_status[2], hdd_status_resp[hdd_id].element_status[3]);
  }
}

void 
show_e1s_status() {
  int ret = 0;
  
  /* read NVMe-MI data */
  printf("e1.s 0: \n");
  ret = pal_get_drive_status(I2C_DEV_E1S_0_PATH);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): fail to get e1.s 0 status.", __func__);
  }

  /* read NVMe-MI data */
  printf("e1.s 1: \n");
  ret = pal_get_drive_status(I2C_DEV_E1S_1_PATH);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): fail to get e1.s 1 status.", __func__);
  }
}

void
show_e1s_health() {
  int ret = 0;

  /* read NVMe-MI data */  
  ret = pal_get_drive_health(I2C_DEV_E1S_0_PATH);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): fail to get e1.s 0 health.", __func__);
    printf("e1.s 0: Abnormal\n");
  } else {
    printf("e1.s 0: Normal\n");
  }

  /* read NVMe-MI data */  
  ret = pal_get_drive_health(I2C_DEV_E1S_1_PATH);
  if(ret < 0) {
    syslog(LOG_DEBUG, "%s(): fail to get e1.s 1 health.", __func__);
    printf("e1.s 1: Abnormal\n");
  } else {
    printf("e1.s 1: Normal\n");
  }
}

int
main(int argc, char **argv) {
  int hdd_id = -1;
  uint8_t chassis_type = 0;

  if (argv == NULL) {
    syslog(LOG_WARNING, "fail to execute enclosure-util because NULL parameter: **argv");
    return -1;
  }
  
  if (argc == 2) {
    if (strcmp(argv[1], "--error") == 0) {
      show_error_code();
      return 0;
    } else if (strcmp(argv[1], "--hdd-status") == 0) {
      show_hdd_status(hdd_id);
      return 0;
    }
    
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get chassis type. \n", __func__);
      return -1;
    }
  
    if (chassis_type == CHASSIS_TYPE5) {
      if (strcmp(argv[1], "--e1s-status") == 0) {
        show_e1s_status();
        return 0;
      } else if (strcmp(argv[1], "--e1s-health") == 0) {
        show_e1s_health();
        return 0;
      }
    }
  }

  if ((argc == 3) && (strcmp(argv[1], "--hdd-status") == 0)) {
    hdd_id = atoi(argv[2]);
    if ((hdd_id >= MAX_NUM_HDD_SLOT) || (hdd_id < 0)){
      printf("enclosure-util: invalid hdd id: %d\n\n", hdd_id);
      print_usage_help();
      return -1;
    }
    show_hdd_status(hdd_id);
    return 0;
  }
  
  print_usage_help();
  return -1;
}
