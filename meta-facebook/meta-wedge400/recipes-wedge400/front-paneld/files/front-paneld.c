/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/limits.h>
#include <openbmc/log.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>

#define FRONTPANELD_NAME       "front-paneld"
#define FRONTPANELD_PID_FILE   "/var/run/front-paneld.pid"

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40

#define INTERVAL_MAX  5

//Debug card presence check interval. Preferred range from ## milliseconds to ## milleseconds.
#define DBG_CARD_CHECK_INTERVAL 500
#define LED_CHECK_INTERVAL 5000
#define LED_SETTING_INTERVAL 500

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */

#define FRONT_PANELD_SCM_MONITOR_THREAD     "scm_monitor"
#define FRONT_PANELD_LED_SETTING_THREAD     "LED_setting"
#define FRONT_PANELD_LED_MONITOR_THREAD     "LED_monitor"
#define FRONT_PANELD_DEBUG_CARD_THREAD      "debug_card"
#define FRONT_PANELD_UART_SEL_BTN_THREAD    "uart_sel_btn"
#define FRONT_PANELD_PWR_BTN_THREAD         "pwr_btn"
#define FRONT_PANELD_RST_BTN_THREAD         "rst_btn"

// Debug card monitoring is disabled by default.
static bool enable_debug_card = false;

// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused){
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  OBMC_INFO("%s thread started", __FUNCTION__);
  while (1) {
    ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
    if (ret || !prsnt) {
      goto scm_mon_out;
    }
    curr = prsnt;
    if (curr != prev) {
      if (curr) {
        // SCM was inserted
        OBMC_WARN("SCM Insertion\n");

        ret = pal_get_server_power(FRU_SCM, &power);
        if (ret) {
          goto scm_mon_out;
        }
		
        if (power == SERVER_POWER_OFF) {
          sleep(3);
          OBMC_WARN("SCM power on\n");
          pal_set_server_power(FRU_SCM, SERVER_POWER_ON);
          /* Setup management port LED */
          run_command("/usr/local/bin/setup_mgmt.sh led");
          goto scm_mon_out;
        }
      } else {
        // SCM was removed
        OBMC_WARN("SCM Extraction\n");
      }
    }
scm_mon_out:
    prev = curr;
      sleep(1);
  }
  return 0;
}

void
exithandler(int signum) {
  int brd_rev;
  pal_get_board_rev(&brd_rev);
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SMB);
  exit(0);
}

/*
 * @brief  checking fan presence and fan speed
 * @note   will be use for fan_led controling
 * @param  brd_rev:
 * @retval 0 is normal , -1 is fail
 */
static int fan_check(int brd_rev) {
  int i;
  float value, unc, lcr;
  uint8_t ret = 0;
  uint8_t fru, prsnt;
  uint8_t fan_num = MAX_NUM_FAN * 2; // rear & front: MAX_NUM_FAN * 2
  char sensor_name[LARGEST_DEVICE_NAME];
  static uint8_t pre_prsnt[MAX_NUM_FAN] = {0};
  static uint8_t pre_sensor[MAX_NUM_FAN * 2] = {0};
  enum {
    FAN_SENSOR_NORMAL = 0,
    FAN_SENSOR_CANT_ACCESS,
    FAN_SENSOR_INVALID,
    FAN_SENSOR_UCR,
    FAN_SENSOR_LCR
  };
  int sensor_num[] = {
      FAN_SENSOR_FAN1_FRONT_TACH,
      FAN_SENSOR_FAN1_REAR_TACH,
      FAN_SENSOR_FAN2_FRONT_TACH,
      FAN_SENSOR_FAN2_REAR_TACH,
      FAN_SENSOR_FAN3_FRONT_TACH,
      FAN_SENSOR_FAN3_REAR_TACH,
      FAN_SENSOR_FAN4_FRONT_TACH,
      FAN_SENSOR_FAN4_REAR_TACH};
  for (uint8_t fan = 0; fan < MAX_NUM_FAN; fan++) {
    if (pal_is_fru_prsnt(fan + FRU_FAN1, &prsnt)) {
      OBMC_ERROR(ENODEV,"%s: fan %d can't get presence status", __func__, fan + 1);
      ret = -1;
    }
    if ( !prsnt && pre_prsnt[fan] )
      OBMC_WARN("%s: fan %d isn't presence\n", __func__, fan + 1);
    else if ( prsnt && !pre_prsnt[fan])
      OBMC_WARN("%s: fan %d is presence\n", __func__, fan + 1);
    pre_prsnt[fan] = prsnt;

    if (!prsnt) {
      ret = -1;
    }
  }
  if (ret) {
    return ret;
  }

  for (i = 0; i < fan_num; i++) {
    fru = FRU_FAN1 + i / 2;
    pal_get_sensor_name(fru, sensor_num[i], sensor_name);
    if (fan_sensor_read(sensor_num[i], &value)) {
      if (pre_sensor[i] != FAN_SENSOR_CANT_ACCESS) {
        OBMC_WARN(
            "%s: can't read sensor sensor id %d (%s)\n",
            __func__,
            sensor_num[i],
            sensor_name);
      }
      pre_sensor[i] = FAN_SENSOR_CANT_ACCESS;
      ret = -1;
      continue;
    }
    if (value == 0) {
      if (pre_sensor[i] != FAN_SENSOR_INVALID) {
        OBMC_WARN(
            "%s: sensor value not available a sensor sensor id %d (%s)\n",
            __func__,
            sensor_num[i],
            sensor_name);
      }
      pre_sensor[i] = FAN_SENSOR_INVALID;
      ret = -1;
      continue;
    }

    pal_get_sensor_threshold(fru, sensor_num[i], UCR_THRESH, &unc);
    pal_get_sensor_threshold(fru, sensor_num[i], LCR_THRESH, &lcr);
    if (value > unc) {
      if (pre_sensor[i] != FAN_SENSOR_UCR) {
        OBMC_WARN(
            "%s: %s value is over than UNC ( %.2f > %.2f )\n",
            __func__,
            sensor_name,
            value,
            unc);
      }
      pre_sensor[i] = FAN_SENSOR_UCR;
      ret = -1;
      continue;
    } else if (value < lcr) {
      if (pre_sensor[i] != FAN_SENSOR_LCR) {
        OBMC_WARN(
            "%s: %s value is under than LCR ( %.2f < %.2f )\n",
            __func__,
            sensor_name,
            value,
            lcr);
      }
      pre_sensor[i] = FAN_SENSOR_LCR;
      ret = -1;
      continue;
    } else {
      if (pre_sensor[i] != FAN_SENSOR_NORMAL) {
        OBMC_WARN(
            "%s: %s value is normal\n",
            __func__,
            sensor_name);
      }
      pre_sensor[i] = FAN_SENSOR_NORMAL;
    }
  }
  return ret;
}

/*
 * @brief  checking PSU presence and PSU Voltage
 * @note   will be use for psu_led controling
 * @param  brd_rev:
 * @retval 0 is normal , -1 is fail
 */
static int psu_check(int brd_rev) {
  #define MAX_PSU_FRU 4
  #define MAX_PSU_SENSOR 3
  int i, ret = 0;
  float value, ucr, lcr;
  uint8_t fru, ready[MAX_PSU_FRU] = {0}, prsnts[MAX_PSU_FRU] = {0};
  int psu1_sensor_num[] = {
      PSU1_SENSOR_IN_VOLT, PSU1_SENSOR_12V_VOLT, PSU1_SENSOR_STBY_VOLT};
  int psu2_sensor_num[] = {
      PSU2_SENSOR_IN_VOLT, PSU2_SENSOR_12V_VOLT, PSU2_SENSOR_STBY_VOLT};
  int pem1_sensor_num[] = {PEM1_SENSOR_IN_VOLT, PEM1_SENSOR_OUT_VOLT};
  int pem2_sensor_num[] = {PEM2_SENSOR_IN_VOLT, PEM2_SENSOR_OUT_VOLT};
  char sensor_name[LARGEST_DEVICE_NAME];
  int *sensor_num, sensor_cnt;
  static uint8_t pre_state = 0;
  static uint8_t pre_sensor[MAX_PSU_FRU][MAX_PSU_SENSOR] = {0};
  enum {
    PSU_STATE_NORMAL = 0,
    PSU_STATE_1_ABSENT,
    PSU_STATE_2_ABSENT,
  };
  enum {
    PSU_SENSOR_NORMAL = 0,
    PSU_SENSOR_CANT_ACCESS,
    PSU_SENSOR_LCR,
    PSU_SENSOR_UCR,
  };
  for (fru = FRU_PEM1; fru <= FRU_PSU2; fru++) {
    if (pal_is_fru_prsnt(fru, &prsnts[fru - FRU_PEM1])) {
      OBMC_ERROR(ENODEV, "%s: fru %d can't get presence status", __func__, i);
      return -1;
    }
  }

  // PSU220V need plug both PSU1 and PSU2
  // PSU48V  can plug only 1 PSU
  // PEM     can plug only 1 PEM
  if (!prsnts[0] && !prsnts[1] && !prsnts[2] && !prsnts[3]) {
    OBMC_ERROR(ENODEV, "%s: Cannot detect any PSU/PEM module \n", __func__);
    return -1;
  }

  if (!is_psu48()) {
    if (prsnts[0] || prsnts[1]) {
      // just only 1 PEM  skip to test another 1
    } else if (!prsnts[2] && prsnts[3]) {
      if ( pre_state != PSU_STATE_1_ABSENT ) {
        OBMC_WARN("%s: PSU1 isn't presence\n", __func__);
      }
      pre_state = PSU_STATE_1_ABSENT;
      return -1;
    } else if (prsnts[2] && !prsnts[3]) {
      if ( pre_state != PSU_STATE_2_ABSENT ) {
        OBMC_WARN("%s: PSU2 isn't presence\n", __func__);
      }
      pre_state = PSU_STATE_2_ABSENT;
      return -1;
    } else {
      if ( pre_state != PSU_STATE_NORMAL ) {
        OBMC_WARN("%s: PSUs is presence\n", __func__);
      }
      pre_state = PSU_STATE_NORMAL;
    }
  }

  for (fru = FRU_PEM1; fru <= FRU_PSU2; fru++) {
    // skip absent psu
    if (!prsnts[fru - FRU_PEM1]) {
      continue;
    }

    switch (fru) {
      case FRU_PEM1:
        sensor_cnt = ARRAY_SIZE(pem1_sensor_num);
        sensor_num = pem1_sensor_num;
        break;
      case FRU_PEM2:
        sensor_cnt = ARRAY_SIZE(pem2_sensor_num);
        sensor_num = pem2_sensor_num;
        break;
      case FRU_PSU1:
        sensor_cnt = ARRAY_SIZE(psu1_sensor_num);
        sensor_num = psu1_sensor_num;
        break;
      case FRU_PSU2:
        sensor_cnt = ARRAY_SIZE(psu2_sensor_num);
        sensor_num = psu2_sensor_num;
        break;
      default:
        OBMC_WARN("%s: skip with invalid fru %d\n", __func__, fru);
        continue;
    }

    pal_is_fru_ready(fru, &ready[fru - FRU_PEM1]);
    if (!ready[fru - FRU_PEM1]) {
      if (fru >= FRU_PSU1 &&
          !ready[fru - 2]) { // if PSU and PEM aren't ready both
        OBMC_WARN(
            "%s: PSU%d and PEM%d can't access\n",
            __func__,
            fru - FRU_PSU1 + 1,
            fru - FRU_PSU1 + 1);
        return -1;
      }
      continue;
    }

    for (i = 0; i < sensor_cnt; i++) {
      pal_get_sensor_threshold(fru, sensor_num[i], UCR_THRESH, &ucr);
      pal_get_sensor_threshold(fru, sensor_num[i], LCR_THRESH, &lcr);
      pal_get_sensor_name(fru, sensor_num[i], sensor_name);

      if (fru >= FRU_PEM1 && fru <= FRU_PEM2) {
        if (pem_sensor_read(sensor_num[i], &value)) {
          if ( pre_sensor[fru-FRU_PEM1][i] != PSU_SENSOR_CANT_ACCESS) {
            OBMC_WARN(
                "%s: can't access read fru %d sensor id %d (%s)\n",
                __func__,
                fru,
                sensor_num[i],
                sensor_name);
          }
          pre_sensor[fru-FRU_PEM1][i] = PSU_SENSOR_CANT_ACCESS;
          ret = -1;
        }
      } else if (fru >= FRU_PSU1 && fru <= FRU_PSU2) {
        if (psu_sensor_read(sensor_num[i], &value)) {
          if ( pre_sensor[fru-FRU_PEM1][i] != PSU_SENSOR_CANT_ACCESS) {
            OBMC_WARN(
                "%s: can't access read  fru %d sensor id %d (%s)\n",
                __func__,
                fru,
                sensor_num[i],
                sensor_name);
          }
          pre_sensor[fru-FRU_PEM1][i] = PSU_SENSOR_CANT_ACCESS;
          ret = -1;
        }
      }

      if (value > ucr) {
        if ( pre_sensor[fru-FRU_PEM1][i] != PSU_SENSOR_UCR) {
          OBMC_WARN(
              "%s: %s value is over than UCR ( %.2f > %.2f )\n",
              __func__,
              sensor_name,
              value,
              ucr);
        }
        pre_sensor[fru-FRU_PEM1][i] = PSU_SENSOR_UCR;
        ret = -1;
      } else if (value < lcr) {
        if ( pre_sensor[fru-FRU_PEM1][i] != PSU_SENSOR_LCR) {
          OBMC_WARN(
              "%s: %s value is under than LCR ( %.2f < %.2f )\n",
              __func__,
              sensor_name,
              value,
              lcr);
        }
        pre_sensor[fru-FRU_PEM1][i] = PSU_SENSOR_LCR;
        ret = -1;
      } else {
        if ( pre_sensor[fru-FRU_PEM1][i] != PSU_SENSOR_NORMAL) {
          OBMC_WARN(
              "%s: %s value is normal\n",
              __func__,
              sensor_name);
        }
        pre_sensor[fru-FRU_PEM1][i] = PSU_SENSOR_NORMAL;
      }
    }
  }
  return ret;
}

// userver state use for set_scm_led()
enum
{
  USERVER_STATE_NONE = 0,
  USERVER_STATE_NORMAL,
  USERVER_STATE_POWER_OFF,
  USERVER_STATE_PING_DOWN,
};

/*
 * @brief  checking SCM power and ping status between BMC-COMe
 * @note   will be use for sys_led controling
 * @param  brd_rev:
 * @retval 0 is normal , -1 is COMe cannot ping , -2 is COMe power off 
 */
static int scm_check(int brd_rev) {
  static int prev_userver_state = USERVER_STATE_NONE;
  char path[PATH_MAX];
  char cmd[64];
  int power;
  int ret;
  const char* scm_ip_usb = "fe80::ff:fe00:2%usb0";

  snprintf(path, sizeof(path), GPIO_COME_PWRGD, "value");
  if (device_read(path, &power)) {
    OBMC_WARN("%s: can't get GPIO value '%s'", __func__, path);
    return -2;
  }

  if (!power) {
    if (prev_userver_state != USERVER_STATE_POWER_OFF) {
      OBMC_WARN("%s: micro server is power off\n", __func__);
      prev_userver_state = USERVER_STATE_POWER_OFF;
    }
    return -2;
  }

  // -c count = 1 times
  // -W timeout = 1 secound
  snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s", scm_ip_usb);
  ret = system(cmd);
  if (ret == 0) { // PING OK
    if (prev_userver_state != USERVER_STATE_NORMAL) {
      OBMC_WARN("%s: micro server in normal state", __func__);
      prev_userver_state = USERVER_STATE_NORMAL;
    }
    return 0;
  }

  if (prev_userver_state != USERVER_STATE_PING_DOWN) {
    OBMC_WARN("%s: can't ping to %s", __func__, scm_ip_usb);
    prev_userver_state = USERVER_STATE_PING_DOWN;
  }
  return -1;
}

enum {
  LED_STATE_OFF,
  LED_STATE_BLUE,
  LED_STATE_GREEN,
  LED_STATE_YELLOW,
  LED_STATE_RED,
  LED_STATE_ALTENATE_BLINK,
  LED_STATE_YELLOW_BLINK,
};

static uint8_t leds_state[4] = {
  LED_STATE_OFF, // SLED_SYS
  LED_STATE_OFF, // SLED_FAN
  LED_STATE_OFF, // SLED_PSU
  LED_STATE_OFF, // SLED_SMB
};

// Thread for setting frontpanel LED
static void *
LED_setting_handler(void *unused) {
  int brd_rev;
  int ret;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;
  static uint8_t blink_state = 0;

  OBMC_INFO("%s thread started", __FUNCTION__);
  ret = pal_get_board_rev(&brd_rev);
  if (ret) {
    OBMC_INFO("%s: %s get borad rev error",  FRONTPANELD_NAME,__FUNCTION__);
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    return 0;
  }

  while (1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if (last_time == 0 || curr_time < last_time ||
        curr_time - last_time > LED_SETTING_INTERVAL) { // LED blink with 0.5 second

      blink_state = 1 - blink_state;
      for (int led = 0; led < 4; led++) {
        if (leds_state[led] == LED_STATE_OFF) {
          set_sled(brd_rev, SLED_CLR_OFF, led);
        } else if (leds_state[led] == LED_STATE_BLUE) {
          set_sled(brd_rev, SLED_CLR_BLUE, led);
        } else if (leds_state[led] == LED_STATE_GREEN) {
          set_sled(brd_rev, SLED_CLR_GREEN, led);
        } else if (leds_state[led] == LED_STATE_YELLOW) {
          set_sled(brd_rev, SLED_CLR_YELLOW, led);
        } else if (leds_state[led] == LED_STATE_RED) {
          set_sled(brd_rev, SLED_CLR_RED, led);
        } else if (leds_state[led] == LED_STATE_ALTENATE_BLINK) {
          if (blink_state) {
            set_sled(brd_rev, SLED_CLR_YELLOW, led);
          } else {
            set_sled(brd_rev, SLED_CLR_BLUE, led);
          }
        } else if (leds_state[led] == LED_STATE_YELLOW_BLINK) {
          if (blink_state) {
            set_sled(brd_rev, SLED_CLR_YELLOW, led);
          } else {
            set_sled(brd_rev, SLED_CLR_OFF, led);
          }
        }
      }
      last_time = curr_time;
    }
    msleep(100);
  }
  return 0;
}

// Thread for monitoring status for setting frontpanel LED
static void *
LED_monitor_handler(void *unused) {
  int brd_rev;
  int ret;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;
  uint8_t prsnt = 0;
  uint8_t sys_ug = 0, fan_ug = 0, psu_ug = 0, smb_ug = 0;

  OBMC_INFO("%s thread started", __FUNCTION__);
  ret = pal_get_board_rev(&brd_rev);
  if (ret)
  {
    OBMC_INFO("%s: %s get borad rev error",  FRONTPANELD_NAME,__FUNCTION__);
    set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SYS);
    return 0;
  }

  while(1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if (last_time == 0 || curr_time < last_time ||
        curr_time - last_time > LED_CHECK_INTERVAL) {  // check status every 5 second
      /* SYS LED
       *  Blue  : All FRUs are present and no alarms
       *  Amber : FRU not present
       *  Blue/amber alternate blink : Firmware upgrading
       */
      ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
      if (ret) {
        OBMC_WARN("%s: Cannot get SCM FRU presence\n",__func__);
        leds_state[SLED_SYS] = LED_STATE_YELLOW;
        goto fan_led;
      }
      if (!prsnt) {
        OBMC_WARN("%s: SCM isn't presence\n",__func__);
        leds_state[SLED_SYS] = LED_STATE_YELLOW;
        goto fan_led;
      }
      
      sys_ug = 0;
      fan_ug = 0;
      psu_ug = 0;
      smb_ug = 0;
      ret = pal_mon_fw_upgrade(brd_rev, &sys_ug, &fan_ug, &psu_ug, &smb_ug);
      if (ret) {
        OBMC_WARN("%s: Cannot check FW upgrade status\n",__func__);
        leds_state[SLED_SYS] = LED_STATE_YELLOW;
        goto fan_led;
      }
      if (sys_ug || fan_ug || psu_ug || smb_ug) {
        OBMC_WARN("%s: FW is upgrading\n",__func__);
        leds_state[SLED_SYS] = LED_STATE_ALTENATE_BLINK;
      } else {
        leds_state[SLED_SYS] = LED_STATE_BLUE;
      }
       

    fan_led:
      /* FAN LED
       *  Blue  : All fans are present and in normal RPM range
       *  Amber : One or more fans are not present or out-of-range RPM
       */
      if (fan_check(brd_rev) == 0) {
        leds_state[SLED_FAN] = LED_STATE_BLUE;
      } else {
        leds_state[SLED_FAN] = LED_STATE_YELLOW;
      }

      /* PSU LED
       *  Blue : PSU/PEM presence follow correct configuration
       *  Amber: PSU/PEM not presence follow correct configuration
       *         or have one or more sensor out-of-range
       */
      if (psu_check(brd_rev) == 0) {
        leds_state[SLED_PSU] = LED_STATE_BLUE;
      } else {
        leds_state[SLED_PSU] = LED_STATE_YELLOW;
      }

      /* SCM LED
       *  Red   : uServer power off
       *  Amber : uServer cannot ping
       *  Green : uServer normal and can ping from BMC
       */
      ret = scm_check(brd_rev);
      if (ret == 0) {
        leds_state[SLED_SMB] = LED_STATE_GREEN;
      } else if (ret == -1) {
        leds_state[SLED_SMB] = LED_STATE_YELLOW;
      } else if (ret == -2) {
        leds_state[SLED_SMB] = LED_STATE_RED;
      }
      last_time = curr_time;
    }
    sleep(1);
  }
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;

  OBMC_INFO("%s thread started", __FUNCTION__);
  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);

    if (ret) {
      goto debug_card_out;
    }
    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr != prev) {
      if (!curr) {
        // Debug Card was removed
        OBMC_WARN("Debug Card Extraction\n");

      } else {
        // Debug Card was inserted
        OBMC_WARN("Debug Card Insertion\n");
      }
      prev = curr;
    }
debug_card_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return 0;
}

/* Thread to handle Power Button to power on/off userver */
static void *
pwr_btn_handler(void *unused) {
  int i;
  int ret;
  uint8_t btn = 0;
  uint8_t cmd = 0;
  uint8_t power = 0;
  uint8_t prsnt = 0;

  OBMC_INFO("%s thread started", __FUNCTION__);

  while (1) {
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto pwr_btn_out;
    }

    ret = pal_get_dbg_pwr_btn(&btn);
    if (ret || !btn) {
        goto pwr_btn_out;
    }

    OBMC_INFO("Power button pressed\n");
    // Wati for the button to be released
    for (i = 0; i < BTN_POWER_OFF; i++) {
        ret = pal_get_dbg_pwr_btn(&btn);
        if (ret || btn) {
            msleep(100);
            continue;
        }
        OBMC_INFO("Power button released\n");
        break;
    }

    // Get the current power state (on or off)
    ret = pal_get_server_power(FRU_SCM, &power);
    if (ret) {
      goto pwr_btn_out;
    }

    // Set power command should reverse of current power state
    cmd = !power;
    
    // To determine long button press
    if (i >= BTN_POWER_OFF) {
      pal_update_ts_sled();
    } else {
      // If current power state is ON and it is not a long press,
      // the power off shoul dbe Graceful Shutdown
      if (power == SERVER_POWER_ON)
        cmd = SERVER_GRACEFUL_SHUTDOWN;

      pal_update_ts_sled();
      OBMC_INFO("Power button pressed for FRU: %d\n", FRU_SCM);
    }

    // Reverse the power state of the given server.
    ret = pal_set_server_power(FRU_SCM, cmd);
pwr_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }
  return 0;
}

/* Thread to handle Reset Button to reset userver */
static void *
rst_btn_handler(void *unused) {
  uint8_t btn;
  int ret;
  uint8_t prsnt = 0;

  OBMC_INFO("%s thread started", __FUNCTION__);

  while (1) {
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto rst_btn_out;
    }
    /* Check the position of hand switch
     * and if reset button is pressed */
    ret = pal_get_dbg_rst_btn(&btn);
    if (ret || !btn) {
      goto rst_btn_out;
    }

    /* Reset userver */
    pal_set_server_power(FRU_SCM, SERVER_POWER_RESET);
rst_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }
  return 0;
}


/* Thread to handle Reset Button to reset userver */
static void *
uart_sel_btn_handler(void *unused) {
  uint8_t btn;
  uint8_t prsnt = 0;
  int ret;

  OBMC_INFO("%s thread started", __FUNCTION__);

  /* Clear Debug Card reset button at the first time */
  ret = pal_clr_dbg_uart_btn();
  if (ret) {
    OBMC_INFO("%s: %s clear uart button status error",  FRONTPANELD_NAME,__FUNCTION__);
    return 0;
  }
  while (1) {
    /* Check the position of hand switch
     * and if reset button is pressed */
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto uart_sel_btn_out;
    }
    ret = pal_get_dbg_uart_btn(&btn);
    if (ret || !btn) {
      goto uart_sel_btn_out;
    }
    pal_switch_uart_mux();
    pal_clr_dbg_uart_btn();
uart_sel_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }
  return 0;
}

static void
dump_usage(const char *prog_name)
{
  int i;
  struct {
    const char *opt;
    const char *desc;
  } options[] = {
    {"-h|--help", "print this help message"},
    {"-d|--enable-debug-card", "enable debug card monitoring"},
    {NULL, NULL},
  };

  printf("Usage: %s [options] ", prog_name);
  for (i = 0; options[i].opt != NULL; i++) {
    printf("    %-24s - %s\n", options[i].opt, options[i].desc);
  }
}

static int
parse_cmdline_args(int argc, char* const argv[])
{
  struct option long_opts[] = {
    {"help",              no_argument, NULL, 'h'},
    {"enable-debug-card", no_argument, NULL, 'd'},
    {NULL,               0,           NULL, 0},
  };

  while (1) {
    int opt_index = 0;
    int ret = getopt_long(argc, argv, "hd", long_opts, &opt_index);
    if (ret == -1)
      break; /* end of arguments */

    switch (ret) {
    case 'h':
      dump_usage(argv[0]);
      exit(0);

    case 'd':
      enable_debug_card = true;
      break;

    default:
      return -1;
    }
  } /* while */

  return 0;
}

int
main (int argc, char * const argv[]) {
  int pid_file;
  int i, rc = 0;
  uint8_t brd_type_rev;
    struct {
    const char *name;
    void* (*handler)(void *args);
    bool initialized;
    pthread_t tid;
    unsigned int flags;
#define FP_FLAG_DEBUG_CARD  0x1
  } front_paneld_threads[7] = {
    {
      .name = FRONT_PANELD_SCM_MONITOR_THREAD,
      .handler = scm_monitor_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_LED_SETTING_THREAD,
      .handler = LED_setting_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_LED_MONITOR_THREAD,
      .handler = LED_monitor_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_DEBUG_CARD_THREAD,
      .handler = debug_card_handler,
      .initialized = false,
      .flags = FP_FLAG_DEBUG_CARD,
    },
    {
      .name = FRONT_PANELD_UART_SEL_BTN_THREAD,
      .handler = uart_sel_btn_handler,
      .initialized = false,
      .flags = FP_FLAG_DEBUG_CARD,
    },
    {
      .name = FRONT_PANELD_PWR_BTN_THREAD,
      .handler = pwr_btn_handler,
      .initialized = false,
      .flags = FP_FLAG_DEBUG_CARD,
    },
    {
      .name = FRONT_PANELD_RST_BTN_THREAD,
      .handler = rst_btn_handler,
      .initialized = false,
      .flags = FP_FLAG_DEBUG_CARD,
    },
  };

  if (parse_cmdline_args(argc, argv) != 0) {
    return -1;
  }

  signal(SIGTERM, exithandler);
  pid_file = open(FRONTPANELD_PID_FILE, O_CREAT | O_RDWR, 0666);
  if (pid_file < 0) {
    fprintf(stderr, "%s: failed to open %s: %s\n",
            FRONTPANELD_NAME, FRONTPANELD_PID_FILE, strerror(errno));
    return -1;
  }

  if (flock(pid_file, LOCK_EX | LOCK_NB) != 0) {
    if(EWOULDBLOCK == errno) {
      fprintf(stderr, "%s: another instance is running. Exiting..\n",
              FRONTPANELD_NAME);
    } else {
      fprintf(stderr, "%s: failed to lock %s: %s\n",
              FRONTPANELD_NAME, FRONTPANELD_PID_FILE, strerror(errno));
    }
    close(pid_file);
    return -1;
  }

  // initialize logging
  if( obmc_log_init(FRONTPANELD_NAME, LOG_INFO, 0) != 0) {
    fprintf(stderr, "%s: failed to initialize logger: %s\n",
            FRONTPANELD_NAME, strerror(errno));
    return -1;
  }

  if ( obmc_log_set_syslog(LOG_CONS, LOG_DAEMON) != 0) {
    fprintf(stderr, "%s: failed to setup syslog: %s\n",
            FRONTPANELD_NAME, strerror(errno));
    return -1;
  }
  obmc_log_unset_std_stream();

  OBMC_INFO("LED Initialize\n");
  init_led();

  if (pal_get_board_type_rev(&brd_type_rev)) {
    OBMC_WARN("Get board revision failed\n");
    exit(-1);
  }

  for (i = 0; i < ARRAY_SIZE(front_paneld_threads); i++) {
    // Do not start debug_card related threads unless "enable_debug_card"
    // is explicitly enabled.
    if ((front_paneld_threads[i].flags & FP_FLAG_DEBUG_CARD) &&
        (!enable_debug_card)) {
      continue;
    }

    rc = pthread_create(&front_paneld_threads[i].tid, NULL,
                        front_paneld_threads[i].handler, NULL);
    if (rc != 0) {
      OBMC_ERROR(rc, "front_paneld: failed to create %s thread: %s\n",
                 front_paneld_threads[i].name, strerror(rc));
      goto cleanup;
    }
    front_paneld_threads[i].initialized = true;
  }

cleanup:
  for (i = ARRAY_SIZE(front_paneld_threads) - 1; i >= 0; i--) {
    if (front_paneld_threads[i].initialized) {
      pthread_join(front_paneld_threads[i].tid, NULL);
      front_paneld_threads[i].initialized = false;
    }
  }
  return rc;
}
