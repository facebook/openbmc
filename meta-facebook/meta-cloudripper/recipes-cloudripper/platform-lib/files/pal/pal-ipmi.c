/*
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

 /*
  * This file contains functions and logics that depends on Cloudripper specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <openbmc/log.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <facebook/bic.h>
#include <facebook/wedge_eeprom.h>
#include "pal.h"

int pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                     uint8_t *res_data, uint8_t *res_len)
{
    int board_sku_id = 0, board_rev;
    unsigned char *data = res_data;
    int completion_code = CC_UNSPECIFIED_ERROR;

    board_sku_id = pal_get_plat_sku_id();

    if (pal_get_board_rev(&board_rev) == -1) {
        *res_len = 0;
        return completion_code;
    }

    *data++ = board_sku_id;
    *data++ = board_rev;
    *data++ = slot;
    *data++ = 0x00; // 1S Server.
    *res_len = data - res_data;
    completion_code = CC_SUCCESS;

    return completion_code;
}

void pal_get_chassis_status(uint8_t slot, uint8_t *req_data,
                            uint8_t *res_data, uint8_t *res_len) {
    char key[MAX_KEY_LEN];
    char buff[MAX_VALUE_LEN];
    int policy = 3;
    uint8_t status, ret;
    unsigned char *data = res_data;

    /* Platform Power Policy */
    sprintf(key, "%s", "server_por_cfg");
    if (pal_get_key_value(key, buff) == 0) {
        if (!memcmp(buff, "off", strlen("off")))
            policy = 0;
        else if (!memcmp(buff, "lps", strlen("lps")))
            policy = 1;
        else if (!memcmp(buff, "on", strlen("on")))
            policy = 2;
        else
            policy = 3;
    }

    /* Current Power State */
    ret = pal_get_server_power(FRU_SCM, &status);
    if (ret >= 0) {
          *data++ = status | (policy << 5);
    } else {
          /* load default */
          OBMC_WARN("ipmid: pal_get_server_power failed for server\n");
          *data++ = 0x00 | (policy << 5);
    }
    *data++ = 0x00;   /* Last Power Event */
    *data++ = 0x40;   /* Misc. Chassis Status */
    *data++ = 0x00;   /* Front Panel Button Disable */
    *res_len = data - res_data;
}

uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy,
                                     uint8_t *res_data) {
    uint8_t completion_code;
    char key[MAX_KEY_LEN];
    unsigned char policy = *pwr_policy & 0x07;  /* Power restore policy */

  completion_code = CC_SUCCESS;   /* Fill response with default values */
    sprintf(key, "%s", "server_por_cfg");
    switch (policy) {
    case 0:
      if (pal_set_key_value(key, "off") != 0)
        completion_code = CC_UNSPECIFIED_ERROR;
      break;
    case 1:
      if (pal_set_key_value(key, "lps") != 0)
        completion_code = CC_UNSPECIFIED_ERROR;
      break;
    case 2:
      if (pal_set_key_value(key, "on") != 0)
        completion_code = CC_UNSPECIFIED_ERROR;
      break;
    case 3:
      /* no change (just get present policy support) */
      break;
    default:
        completion_code = CC_PARAM_OUT_OF_RANGE;
      break;
    }
    return completion_code;
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int pal_get_plat_sku_id(void) {
    return 0x06; // Cloudripper
}

//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                             uint8_t *res_data, uint8_t *res_len) {
    uint8_t completion_code = CC_SUCCESS;
    uint8_t pcie_conf = 0x02; // Cloudripper
    uint8_t *data = res_data;
    *data++ = pcie_conf;
    *res_len = data - res_data;
    return completion_code;
}

int pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
    int i;
    char key[MAX_KEY_LEN];
    char str[MAX_VALUE_LEN] = {0};
    char tstr[10];
    sprintf(key, "%s", "sysfw_ver_server");

    for (i = 0; i < SIZE_SYSFW_VER; i++) {
        sprintf(tstr, "%02x", ver[i]);
        strcat(str, tstr);
    }

    return pal_set_key_value(key, str);
}

int pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
    int i;
    int j = 0;
    int ret;
    int msb, lsb;
    char key[MAX_KEY_LEN];
    char str[MAX_VALUE_LEN] = {0};
    char tstr[4];

    sprintf(key, "%s", "sysfw_ver_server");

    ret = pal_get_key_value(key, str);
    if (ret) {
        return ret;
    }

    for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
        sprintf(tstr, "%c\n", str[i]);
        msb = strtol(tstr, NULL, 16);

        sprintf(tstr, "%c\n", str[i+1]);
        lsb = strtol(tstr, NULL, 16);
        ver[j++] = (msb << 4) | lsb;
    }

    return 0;
}

// Enable POST buffer for the server in given slot
int pal_post_enable(uint8_t slot) {
    int ret;

    bic_config_t config = {0};
    bic_config_u *t = (bic_config_u *) &config;

    ret = bic_get_config(IPMB_BUS, &config);
    if (ret) {
        PAL_DEBUG("post_enable: bic_get_config failed for fru: %d\n", slot);
        return ret;
    }

    if (0 == t->bits.post) {
        t->bits.post = 1;
        ret = bic_set_config(IPMB_BUS, &config);
        if (ret) {
            PAL_DEBUG("post_enable: bic_set_config failed\n");
            return ret;
        }
    }

    return 0;
}

// Get the last post code of the given slot
int pal_post_get_last(uint8_t slot, uint8_t *status) {
    int ret;
    uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
    uint8_t len = 0 ;

    ret = bic_get_post_buf(IPMB_BUS, buf, &len);
    if (ret) {
        return ret;
    }

    *status = buf[0];

    return 0;
}

int pal_post_get_last_and_len(uint8_t slot, uint8_t *data, uint8_t *len) {
    int ret;

    ret = bic_get_post_buf(IPMB_BUS, data, len);
    if (ret) {
        return ret;
    }

    return 0;
}

int pal_get_80port_record(uint8_t slot, uint8_t *res_data,
                          size_t max_len, size_t *res_len)
{
    int ret;
    uint8_t len;

    ret = bic_get_post_buf(IPMB_BUS, res_data, &len);
    if (ret) {
        return ret;
    } else {
        *res_len = len;
    }

    return 0;
}
