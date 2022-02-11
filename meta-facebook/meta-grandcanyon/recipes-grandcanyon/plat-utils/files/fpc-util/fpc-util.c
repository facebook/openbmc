/*
 * fpc-util
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>
#include <openbmc/kv.h>
#include <facebook/fbgc_common.h>

static void
print_usage_help(void) {
  uint8_t chassis_type = 0;
  
  printf("Usage: fpc-util --identify <on/off>\n");
  printf("       fpc-util --identify on <optional>..\n");
  printf("         <optional>:\n");
  printf("           --interval <1/5/10>     This is an optional setting. If this option is not set, will use 0.5 second by default.\n");
  printf("                                   User can set 1, 5, or 10 to adjust the blinking rate to 1, 5, 10 seconds.\n");
  printf("       fpc-util --status <yellow/blue/off>\n");
  
  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s(): Failed to get chassis type.", __func__);
    return;
  }

  if (chassis_type == CHASSIS_TYPE5) {
    printf("       fpc-util --e1s0 <on/off/blinking>\n");
    printf("       fpc-util --e1s1 <on/off/blinking>\n");
  }
}

int
main(int argc, char **argv) {
  int ret = 0;
  char key[MAX_KEY_LEN] = {0};
  uint8_t chassis_type = 0;
  
  memset(key, 0x0, sizeof(key));
  
  if (argc == 5) {
    if ((strcmp(argv[1], "--identify") == 0)
      && (strcmp(argv[2], "on") == 0)
      && (strcmp(argv[3], "--interval") == 0)) {
      if ((strcmp(argv[4], "1") == 0)
        || (strcmp(argv[4], "5") == 0)
        || (strcmp(argv[4], "10") == 0)) {
        snprintf(key, sizeof(key), "system_identify_led_interval");
        ret = pal_set_key_value(key, argv[4]);
        if (ret < 0) {
          printf("fpc-util: failed to set interval of identification\n");
          return ret;
        }
        
        snprintf(key, sizeof(key), "system_identify_server");
        ret = pal_set_key_value(key, argv[2]);
        if (ret < 0) {
          printf("fpc-util: failed to set identification on\n");
          return ret;
        }
        printf("fpc-util: identification is %s, interval is set to %s second\n", argv[2], argv[4]);
        return ret;
      } else {
        printf("fpc-util: %s is invalid interval time\n", argv[4]);
        goto err_exit;
      }
    }
  }
  
  if (argc != 3) {
    goto err_exit;
  }

  if (strcmp(argv[1], "--identify") == 0) {
    if (strcmp(argv[2], "on") == 0) {
      snprintf(key, sizeof(key), "system_identify_led_interval");
      ret = pal_set_key_value(key, "default");
      if (ret < 0) {
        printf("fpc-util: failed to set interval of identification\n");
        return ret;
      }
      printf("fpc-util: identification is %s, default interval is 0.5 second\n", argv[2]);
      snprintf(key, sizeof(key), "system_identify_server");
      return pal_set_key_value(key, argv[2]);
    } else if (strcmp(argv[2], "off") == 0) {
      printf("fpc-util: identification is %s\n", argv[2]);
      snprintf(key, sizeof(key), "system_identify_server");
      return pal_set_key_value(key, argv[2]);
    } else {
      goto err_exit;
    }
  } else if (strcmp(argv[1], "--status") == 0) {
    snprintf(key, sizeof(key), "flag_fpc_status");
    if (strcmp(argv[2], "yellow") == 0) {
      kv_set(key, STR_VALUE_1, 0, 0);
      return pal_set_status_led(FRU_UIC, STATUS_LED_YELLOW);
    } else if (strcmp(argv[2], "blue") == 0) {
      kv_set(key, STR_VALUE_1, 0, 0);
      return pal_set_status_led(FRU_UIC, STATUS_LED_BLUE);
    } else if (strcmp(argv[2], "off") == 0) {
      kv_set(key, STR_VALUE_0, 0, 0);
      return pal_set_status_led(FRU_UIC, STATUS_LED_OFF);
    } else {
      goto err_exit;
    }
  } 
  
  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get chassis type.", __func__);
    return -1;
  }
  
  if (chassis_type == CHASSIS_TYPE5) {
    if (strcmp(argv[1], "--e1s0") == 0) {
      snprintf(key, sizeof(key), "e1s0_led_status");
      if (strcmp(argv[2], "on") == 0) {
        return kv_set(key, "on", 0, 0);
      } else if (strcmp(argv[2], "off") == 0) {
        return kv_set(key, "off", 0, 0);
      } else if (strcmp(argv[2], "blinking") == 0) {
        return kv_set(key, "blinking", 0, 0);
      } else {
        goto err_exit;
      }
    } else if (strcmp(argv[1], "--e1s1") == 0) {
      snprintf(key, sizeof(key), "e1s1_led_status");
      if (strcmp(argv[2], "on") == 0) {
        return kv_set(key, "on", 0, 0);
      } else if (strcmp(argv[2], "off") == 0) {
        return kv_set(key, "off", 0, 0);
      } else if (strcmp(argv[2], "blinking") == 0) {
        return kv_set(key, "blinking", 0, 0);
      } else {
        goto err_exit;
      }
    } else {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;
}

