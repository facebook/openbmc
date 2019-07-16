/*
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
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40
#define BTN_HSVC          100
#define MAX_NUM_SLOTS 4
#define HB_SLEEP_TIME (5 * 60)
#define HB_TIMESTAMP_COUNT (60 * 60 / HB_SLEEP_TIME)

#define LED_ON 1
#define LED_OFF 0

#define ID_LED_ON 0
#define ID_LED_OFF 1

#define LED_ON_TIME_IDENTIFY 200
#define LED_OFF_TIME_IDENTIFY 200

#define LED_ON_TIME_HEALTH 900
#define LED_OFF_TIME_HEALTH 100

#define LED_ON_TIME_BMC_SELECT 500
#define LED_OFF_TIME_BMC_SELECT 500

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

#define FW_UPDATE_ONGOING 1
#define CRASHDUMP_ONGOING 2
#define CPLDDUMP_ONGOING  3

static uint8_t g_sync_led[MAX_NUM_SLOTS+1] = {0x0};
static uint8_t m_pos = 0xff;
static uint8_t m_fan_latch = 0;

slot_kv_st slot_kv_list[] = {
  {"identify_slot%d", "off"},
};

static int
get_handsw_pos(uint8_t *pos) {
  if ((m_pos > HAND_SW_BMC) || (m_pos < HAND_SW_SERVER1))
    return -1;

  *pos = m_pos;
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t pos, usb_pos;
  uint8_t prev_pos = 0xff, prev_phy_pos = 0xff;
  uint8_t lpc;
  uint8_t status;
  char str[8];

  while (1) {
    ret = pal_get_hand_sw_physically(&pos);
    if (ret) {
      goto debug_card_out;
    }

    if (pos == prev_phy_pos) {
      goto get_hand_sw_cache;
    }

    msleep(10);
    ret = pal_get_hand_sw_physically(&pos);
    if (ret) {
      goto debug_card_out;
    }

    prev_phy_pos = pos;
    sprintf(str, "%u", pos);
    ret = kv_set("spb_hand_sw", str, 0, 0);
    if (ret) {
      goto debug_card_out;
    }

get_hand_sw_cache:
    ret = pal_get_hand_sw(&pos);
    if (ret) {
      goto debug_card_out;
    }

    if (pos == prev_pos) {
      goto debug_card_prs;
    }
    m_pos = pos;

    ret = pal_switch_usb_mux(pos);
    if (ret) {
      goto debug_card_out;
    }

    ret = pal_switch_uart_mux(pos);
    if (ret) {
      goto debug_card_out;
    }

debug_card_prs:
    ret = pal_get_usb_sw(&usb_pos);
    if (ret) {
      goto debug_card_out;
    }

    if (usb_pos <= MAX_NUM_SLOTS) {
      if (!pal_is_slot_server(usb_pos) || (!pal_get_server_power(usb_pos, &status) && (status != SERVER_POWER_ON))) {
        pal_enable_usb_mux(USB_MUX_OFF);
      } else {
        pal_enable_usb_mux(USB_MUX_ON);
      }
    }

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
        syslog(LOG_WARNING, "Debug Card Extraction\n");
      } else {
        // Debug Card was inserted
        syslog(LOG_WARNING, "Debug Card Insertion\n");
      }
    }

    if ((pos == prev_pos) && (curr == prev)) {
      goto debug_card_out;
    }

    ret = pal_switch_uart_mux(pos);
    if (ret) {
      goto debug_card_out;
    }

    // If Debug Card is present
    if (curr) {
      // Enable POST code based on hand switch
      if (pos == HAND_SW_BMC) {
        // For BMC, there is no need to have POST specific code
        goto debug_card_done;
      }

      // Make sure the server at selected position is ready
      ret = pal_is_fru_ready(pos, &prsnt);
      if (ret || !prsnt) {
        goto debug_card_done;
      }

      // Enable POST codes for all slots
      ret = pal_post_enable(pos);
      if (ret) {
        goto debug_card_done;
      }

      // Get last post code and display it
      ret = pal_post_get_last(pos, &lpc);
      if (ret) {
        goto debug_card_done;
      }

      ret = pal_post_handle(pos, lpc);
      if (ret) {
        goto debug_card_out;
      }
    }

debug_card_done:
    prev = curr;
    prev_pos = pos;
debug_card_out:
    if (curr == 1)
      msleep(500);
    else
      sleep(1);
  }

  return 0;
}

static int
is_btn_blocked(uint8_t fru, uint8_t * ongoing_fru) {
  uint8_t fru_start, fru_end, fru_sys_end;
  uint8_t i,pair_fru;

  *ongoing_fru = fru;
  if (fru == HAND_SW_BMC) {
    fru_start = FRU_SLOT1;
    fru_end = FRU_SLOT4;
    fru_sys_end = FRU_BMC;
  } else {
    fru_start = fru_end = fru_sys_end = fru;
  }

  for (i = fru_start; i <= fru_sys_end; i++) {
    if (pal_is_fw_update_ongoing(i)) {
      *ongoing_fru = i;
      return FW_UPDATE_ONGOING;
    }
  }

  for (i = fru_start; i <= fru_end; i++) {
    if (pal_is_crashdump_ongoing(i)) {
      *ongoing_fru = i;
      return CRASHDUMP_ONGOING;
    }
  }

  for (i = fru_start; i <= fru_end; i++) {
    if (pal_is_cplddump_ongoing(i)) {
      *ongoing_fru = i;
      return CPLDDUMP_ONGOING;
    }
  }

  // Device Card + Server
  switch (pal_get_pair_slot_type(fru)) {
    case TYPE_CF_A_SV:
    case TYPE_GP_A_SV:
    case TYPE_GPV2_A_SV:
      if (0 == fru%2)
        pair_fru = fru - 1;
      else
        pair_fru = fru + 1;
      if (pal_is_fw_update_ongoing(pair_fru)) {
        *ongoing_fru = pair_fru;
        return FW_UPDATE_ONGOING;
      }
      if (pal_is_crashdump_ongoing(pair_fru)) {
        *ongoing_fru = pair_fru;
        return CRASHDUMP_ONGOING;
      }
      if (pal_is_cplddump_ongoing(pair_fru)) {
        *ongoing_fru = pair_fru;
        return CPLDDUMP_ONGOING;
      }
      return 0;
    default:
      return 0;
  }
}

// Thread to monitor Reset Button and propagate to selected server
static void *
rst_btn_handler() {
  int ret;
  uint8_t pos;
  int i;
  uint8_t btn;
  uint8_t last_btn = 0;
  uint8_t ongoing_fru;

  ret = pal_get_rst_btn(&btn);
  if (0 == ret) {
    last_btn = btn;
  }

  while (1) {
    // Check the position of hand switch
    ret = get_handsw_pos(&pos);
    if (ret || pos == HAND_SW_BMC) {
      // For BMC, no need to handle Reset Button
      sleep(1);
      continue;
    }

    // Check if reset button is pressed
    ret = pal_get_rst_btn(&btn);
    if (ret || !btn) {
      if (last_btn != btn) {
        pal_set_rst_btn(pos, 1);
      }
      goto rst_btn_out;
    }

    if ((ret = is_btn_blocked(pos,&ongoing_fru))) {
      if (!last_btn) {
        switch (ret) {
          case FW_UPDATE_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to FW update is ongoing, FRU: %d", ongoing_fru);
            break;
          case CRASHDUMP_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to crashdump is ongoing, FRU: %d", ongoing_fru);
            break;
          case CPLDDUMP_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to CPLD dump is ongoing, FRU: %d", ongoing_fru);
            break;
        }
      }
      goto rst_btn_out;
    }

    // Pass the reset button to the selected slot
    syslog(LOG_WARNING, "Reset button pressed\n");
    ret = pal_set_rst_btn(pos, 0);
    if (ret) {
      goto rst_btn_out;
    }

    // Wait for the button to be released
    for (i = 0; i < BTN_MAX_SAMPLES; i++) {
      ret = pal_get_rst_btn(&btn);
      if (ret || btn) {
        msleep(100);
        continue;
      }
      pal_update_ts_sled();
      syslog(LOG_WARNING, "Reset button released\n");
      syslog(LOG_CRIT, "Reset Button pressed for FRU: %d\n", pos);
      ret = pal_set_rst_btn(pos, 1);
      if (!ret) {
        pal_set_restart_cause(pos, RESTART_CAUSE_RESET_PUSH_BUTTON);
      }
      goto rst_btn_out;
    }

    // handle error case
    if (i == BTN_MAX_SAMPLES) {
      pal_update_ts_sled();
      syslog(LOG_WARNING, "Reset button seems to stuck for long time\n");
      goto rst_btn_out;
    }
rst_btn_out:
    last_btn = btn;
    msleep(100);
  }

  return 0;
}

// Thread to handle Power Button and power on/off the selected server
static void *
pwr_btn_handler() {
  int ret, i;
  uint8_t pos, btn;
  uint8_t cmd = 0;
  uint8_t power, st_12v = 0;
  char tstr[64];
  bool release_flag = true;
  uint8_t ongoing_fru;

  while (1) {
    // Check the position of hand switch
    ret = get_handsw_pos(&pos);
    if (ret) {
      sleep(1);
      continue;
    }

    // Check if power button is pressed
    ret = pal_get_pwr_btn(&btn);
    if (ret || !btn) {
      if (false == release_flag)
        release_flag = true;

      goto pwr_btn_out;
    }

    if (false == release_flag)
      goto pwr_btn_out;

    release_flag = false;
    syslog(LOG_WARNING, "Power button pressed\n");

    // Wait for the button to be released
    for (i = 0; i < BTN_HSVC; i++) {
      ret = pal_get_pwr_btn(&btn);
      if (ret || btn ) {
        msleep(100);
        continue;
      }

      release_flag = true;
      syslog(LOG_WARNING, "Power button released\n");
      break;
    }

    if ((ret = is_btn_blocked(pos,&ongoing_fru))) {
      switch (ret) {
        case FW_UPDATE_ONGOING:
          syslog(LOG_CRIT, "Power Button blocked due to FW update is ongoing, FRU: %d", ongoing_fru);
          break;
        case CRASHDUMP_ONGOING:
          syslog(LOG_CRIT, "Power Button blocked due to crashdump is ongoing, FRU: %d", ongoing_fru);
          break;
        case CPLDDUMP_ONGOING:
          syslog(LOG_CRIT, "Power Button blocked due to CPLD dump is ongoing, FRU: %d", ongoing_fru);
          break;
      }
      goto pwr_btn_out;
    }

    // Get the current power state (power on vs. power off)
    if (pos != HAND_SW_BMC) {
      ret = pal_is_server_12v_on(pos, &st_12v);
      if (ret) {
        goto pwr_btn_out;
      }

      if (i >= BTN_HSVC) {
        pal_update_ts_sled();
        syslog(LOG_CRIT, "Power Button Long Press for FRU: %d\n", pos);

        if (!pal_is_hsvc_ongoing(pos) || st_12v) {
          sprintf(tstr, "/usr/bin/hsvc-util slot%u --start", pos);
          run_command(tstr);
          goto pwr_btn_out;
        }
      }

      if (st_12v) {
        ret = pal_get_server_power(pos, &power);
        if (ret) {
          goto pwr_btn_out;
        }
        // Set power command should reverse of current power state
        cmd = !power;
      }
    }

    // To determine long button press
    if (i >= BTN_POWER_OFF) {
      // if long press (>4s) and hand-switch position == bmc, then initiate
      // sled-cycle
      if (pos == HAND_SW_BMC) {
        pal_update_ts_sled();
        syslog(LOG_CRIT, "SLED_CYCLE using power button successful");
        sleep(1);
        pal_sled_cycle();
      } else {
        if (i < BTN_HSVC) {
          pal_update_ts_sled();
          syslog(LOG_CRIT, "Power Button Long Press for FRU: %d", pos);
        }

        if (!st_12v) {
          sprintf(tstr, "/usr/bin/hsvc-util slot%u --stop", pos);
          run_command(tstr);
          sprintf(tstr, "/usr/local/bin/power-util slot%u 12V-on", pos);
          run_command(tstr);
          goto pwr_btn_out;
        }
      }
    } else {
      // If current power state is ON and it is not a long press,
      // the power off should be Graceful Shutdown
      if (power == SERVER_POWER_ON)
        cmd = SERVER_GRACEFUL_SHUTDOWN;

      pal_update_ts_sled();
      syslog(LOG_CRIT, "Power Button Press for FRU: %d\n", pos);
    }

    if ((pos != HAND_SW_BMC) && st_12v) {
      if (cmd == SERVER_POWER_ON)
        pal_set_restart_cause(pos, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);

      // Reverse the power state of the given server
      ret = pal_set_server_power(pos, cmd);
    }

pwr_btn_out:
    msleep(100);
  }
  return 0;
}

// Thread to handle LED state of the server at given slot
static void *
led_handler() {
  int ret;
  uint8_t pos;
  uint8_t slot;
  uint8_t ready;
  uint8_t power[MAX_NUM_SLOTS+1] = {0};
  uint8_t hlth[MAX_NUM_SLOTS+1] = {0};
  int led_on_time, led_off_time;
  uint8_t spb_hlth, nic_hlth, chassis_hlth;

  while (1) {
    // Get hand switch position to see if this is selected server
    ret = get_handsw_pos(&pos);
    if (ret != 0) {
      sleep(1);
      continue;
    }

    ret = pal_get_fru_health(FRU_SPB, &spb_hlth);
    if (ret) {
      sleep(1);
      continue;
    }

    ret = pal_get_fru_health(FRU_NIC, &nic_hlth);
    if (ret) {
      sleep(1);
      continue;
    }

    if ((spb_hlth == FRU_STATUS_GOOD) && (nic_hlth == FRU_STATUS_GOOD)) {
      chassis_hlth = FRU_STATUS_GOOD;
    } else {
      chassis_hlth = FRU_STATUS_BAD;
    }

    for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
      // Check if this LED is managed by sync_led thread
      if (g_sync_led[slot]) {
        continue;
      }

      ret = pal_is_fru_ready(slot, &ready);
      if (!ret && ready) {
        // Get power status for this slot
        ret = pal_get_server_power(slot, &power[slot]);
        if (ret) {
          continue;
        }

        // Get health status for this slot
        ret = pal_get_fru_health(slot, &hlth[slot]);
        if (ret) {
          continue;
        }
      } else {
        power[slot] = SERVER_POWER_OFF;
        hlth[slot] = FRU_STATUS_GOOD;
      }

      if ((pos == slot) || (power[slot] == SERVER_POWER_ON)) {
        if ((hlth[slot] == FRU_STATUS_GOOD) && (chassis_hlth == FRU_STATUS_GOOD)) {
          pal_set_led(slot, LED_ON);
          pal_set_id_led(slot, ID_LED_OFF);
        } else {
          pal_set_led(slot, LED_OFF);
          pal_set_id_led(slot, ID_LED_ON);
        }
      } else {
        pal_set_led(slot, LED_OFF);
        pal_set_id_led(slot, ID_LED_OFF);
      }
    }

    if (pos > MAX_NUM_SLOTS) {
      sleep(1);
      continue;
    }

    if (g_sync_led[pos]) {
      sleep(1);
      continue;
    }

    // Set blink rate
    if (power[pos] == SERVER_POWER_ON) {
      led_on_time = 900;
      led_off_time = 100;
    } else {
      led_on_time = 100;
      led_off_time = 900;
    }

    msleep(led_on_time);

    if ((hlth[pos] == FRU_STATUS_GOOD) && (chassis_hlth == FRU_STATUS_GOOD)) {
      pal_set_led(pos, LED_OFF);
    } else {
      pal_set_id_led(pos, ID_LED_OFF);
    }

    msleep(led_off_time);

    if (power[pos] == SERVER_POWER_ON) {
      if ((hlth[pos] == FRU_STATUS_GOOD) && (chassis_hlth == FRU_STATUS_GOOD)) {
        pal_set_led(pos, LED_ON);
      } else {
        pal_set_id_led(pos, ID_LED_ON);
      }
    }
  }

  return 0;
}

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  int ret;
  uint8_t pos;
  uint8_t ident = 0;
  char identify[16] = {0};
  char tstr[64] = {0};
  char id_arr[5] = {0};
  uint8_t slot;

#ifdef DEBUG
  syslog(LOG_INFO, "led_handler for slot %d\n", slot);
#endif

  while (1) {
    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, 16);
    ret = pal_get_key_value("identify_sled", identify);
    if (ret == 0 && !strcmp(identify, "on")) {
      // Turn OFF Blue LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        g_sync_led[slot] = 1;
        pal_set_led(slot, LED_OFF);
      }

      // Start blinking the ID LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_id_led(slot, ID_LED_ON);
      }

      msleep(LED_ON_TIME_IDENTIFY);

      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_id_led(slot, ID_LED_OFF);
      }
      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    }

    // Get hand switch position to see if this is selected server
    ret = get_handsw_pos(&pos);
    if (ret) {
      sleep(1);
      continue;
    }

    // Handle BMC select condition when no slot is being identified
    if (pos == HAND_SW_BMC) {
      // Turn OFF Yellow LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        g_sync_led[slot] = 1;
        pal_set_id_led(slot, ID_LED_OFF);
      }

      // Start blinking Blue LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_led(slot, LED_ON);
      }

      msleep(LED_ON_TIME_BMC_SELECT);

      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_led(slot, LED_OFF);
      }

      msleep(LED_OFF_TIME_BMC_SELECT);
      continue;
    }

    // Check if slot needs to be identified
    ident = 0;
    for (slot = 1; slot <= MAX_NUM_SLOTS; slot++)  {
      id_arr[slot] = 0x0;
      sprintf(tstr, "identify_slot%d", slot);
      memset(identify, 0x0, 16);
      ret = pal_get_key_value(tstr, identify);
      if (ret == 0 && !strcmp(identify, "on")) {
        id_arr[slot] = 0x1;
        ident = 1;
      }
    }

    // Handle individual identify slot condition
    if (ident) {
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        if (id_arr[slot]) {
          g_sync_led[slot] = 1;
          pal_set_led(slot, LED_OFF);
          pal_set_id_led(slot, ID_LED_ON);
          if (m_fan_latch && pal_is_hsvc_ongoing(slot)) {
            pal_set_slot_id_led(slot, LED_ON); // Slot ID LED on top of each TL
          }
        } else {
          g_sync_led[slot] = 0;
        }
      }

      msleep(LED_ON_TIME_IDENTIFY);

      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        if (id_arr[slot]) {
          pal_set_id_led(slot, ID_LED_OFF);
          pal_set_slot_id_led(slot, LED_OFF); // Slot ID LED on top of each TL
        }
      }

      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    }

    for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
      g_sync_led[slot] = 0;
    }
    msleep(500);
  }

  return 0;
}

// Thread to handle Seat LED state
static void *
seat_led_handler() {
  int ret;
  uint8_t slot, val, ident;
  uint8_t slot_12v_on, slot_prsnt;

  while (1) {
    ret = pal_get_fan_latch(&val);
    if (ret < 0) {
      msleep(500);
      continue;
    }
    m_fan_latch = val;

    // Handle Sled fully seated
    if (val) {  // SLED is pull out
      pal_set_sled_led(LED_ON);

      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        if (pal_is_hsvc_ongoing(slot))
          continue;
        if (pal_is_server_12v_on(slot, &slot_12v_on))
          continue;
        if (pal_is_fru_prsnt(slot, &slot_prsnt))
          continue;

        if (slot_prsnt != 1 || slot_12v_on != 1) {
          pal_set_slot_id_led(slot, LED_OFF);  // Turn slot ID LED off
        } else {
          pal_set_slot_id_led(slot, LED_ON);   // Turn slot ID LED on
        }
      }
    } else {    // SLED is fully pull in
      ident = 0;
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        if (pal_is_hsvc_ongoing(slot)) {
          ident = 1;
        }
        pal_set_slot_id_led(slot, LED_OFF);    // Turn slot ID LED off
      }

      // Start blinking the SEAT LED
      if (1 == ident) {
        pal_set_sled_led(LED_ON);
        msleep(LED_ON_TIME_IDENTIFY);

        pal_set_sled_led(LED_OFF);
        msleep(LED_OFF_TIME_IDENTIFY);
        continue;
      } else {
        pal_set_sled_led(LED_OFF);
      }
    }

    sleep(1);
  }

  return 0;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_debug_card;
  pthread_t tid_rst_btn;
  pthread_t tid_pwr_btn;
  pthread_t tid_sync_led;
  pthread_t tid_led;
  pthread_t tid_seat_led;
  int rc;
  int pid_file;
  int slot_id;
  int i;
  char slot_kv[80] = {0};
  int ret;

  for (slot_id = 1; slot_id <= MAX_NUM_SLOTS; slot_id++) {
    for (i = 0; i < sizeof(slot_kv_list)/sizeof(slot_kv_st); i++) {
      memset(slot_kv, 0, sizeof(slot_kv));
      sprintf(slot_kv, slot_kv_list[i].slot_key, slot_id);
      if ((ret = pal_set_key_value(slot_kv, slot_kv_list[i].slot_def_val)) < 0) {  //Restore Slot indication LED status to normal when BMC reset
        syslog(LOG_WARNING, "%s %s: kv_set failed. %d", __func__, slot_kv_list[i].slot_key, ret);
      }
    }
  }

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }


  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for reset button error\n");
    exit(1);
  }

  if (pthread_create(&tid_pwr_btn, NULL, pwr_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for power button error\n");
    exit(1);
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    exit(1);
  }

  if (pthread_create(&tid_led, NULL, led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
    exit(1);
  }

  if (pthread_create(&tid_seat_led, NULL, seat_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for seat led error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_led, NULL);
  pthread_join(tid_seat_led, NULL);

  return 0;
}
