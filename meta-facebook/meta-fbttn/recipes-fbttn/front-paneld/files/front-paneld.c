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
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <sys/mman.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40
#define MAX_NUM_SLOTS 1
#define HB_SLEEP_TIME (5 * 60)
#define HB_TIMESTAMP_COUNT (60 * 60 / HB_SLEEP_TIME)

#define LED_ON 1
#define LED_OFF 0

#define ID_LED_ON  1
#define ID_LED_OFF 0

#define LED_ON_TIME_IDENTIFY 200
#define LED_OFF_TIME_IDENTIFY 200

#define LED_ON_TIME_HEALTH 900
#define LED_OFF_TIME_HEALTH 100

#define LED_ON_TIME_BMC_SELECT 500
#define LED_OFF_TIME_BMC_SELECT 500

#define PATH_HEARTBEAT_HEALTH "/tmp/heartbeat_health"
#define BMC_RMT_HB_TIMEOUT_COUNT  1800
#define SCC_LOC_HB_TIMEOUT_COUNT  1800
#define SCC_RMT_HB_TIMEOUT_COUNT  1800
#define BMC_RMT_HB_RPM_LIMIT  0
#define SCC_LOC_HB_RPM_LIMIT  0
#define SCC_RMT_HB_RPM_LIMIT  0

#define PAGE_SIZE                     0x1000
#define AST_SRAM_BMC_REBOOT_BASE      0x1E721000
#define BMC_REBOOT_BY_KERN_PANIC(base) *((uint32_t *)(base + 0x200))
#define BMC_REBOOT_BY_CMD(base) *((uint32_t *)(base + 0x204))

#define BIT_RECORD_LOG                (1 << 8)
// kernel panic
#define FLAG_KERN_PANIC               (1 << 0)
// reboot command
#define FLAG_REBOOT_CMD               (1 << 0)
#define FLAG_CFG_UTIL                 (1 << 1)


uint8_t g_sync_led[MAX_NUM_SLOTS+1] = {0x0};
unsigned char g_err_code[ERROR_CODE_NUM];

int
write_cache(const char *device, uint8_t value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
    return err;
  }

  rc = fprintf(fp, "%d", value);
  fclose(fp);

  if (rc < 0) {
    return ENOENT;
  } else {
    return 0;
  }
}
void cache_post_code(unsigned char* buffer,int buf_len){
  FILE *fp;
  fp = fopen(POST_CODE_FILE, "w");
  if (fp) {
      lockf(fileno(fp),F_LOCK,0L);
      while (buf_len) {
      fprintf(fp, "%X ", buffer[buf_len-1]);
      buf_len--;
    }
    fprintf(fp, "\n");
  }
  lockf(fileno(fp),F_ULOCK,0L);
  fclose(fp);
}
void check_cache_post_code(){
  FILE *fp;

  if (access(POST_CODE_FILE, F_OK) == 0) {
      //exist
      rename(POST_CODE_FILE, LAST_POST_CODE_FILE);
  }
}
// Thread for monitoring debug card hotswap
static void *
debug_card_handler() {
  uint8_t prsnt;
  uint8_t pos ;
  int index = 0, ret, timer = 0;
  uint8_t uart_selection = HAND_SW_BMC;
  int CURT = 0;
  static uint8_t error[MAX_ERROR_CODES], count_ret = 0, count_cur = 0;
  static uint8_t code = 0; // BIOS post code or BMC/Expander error code
  uint8_t buffer[MAX_IPMB_RES_LEN], buf_len;
  uint8_t status;

  while (1) {

    ret = pal_is_fru_prsnt(FRU_SLOT1, &prsnt);
    if (!ret && prsnt == 1) {
      ret = pal_get_server_power(FRU_SLOT1, &status);
      if(!ret && (status == SERVER_POWER_ON) ) {
        ret = pal_post_enable(FRU_SLOT1);
        if (!ret) {
          ret = pal_post_get_buffer(buffer, &buf_len);
          if (!ret) {
            cache_post_code(buffer,buf_len);
          }
        }
      } else {
        check_cache_post_code();
      }
    }
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret) {
      goto debug_card_out;
    }

    // If Debug Card is present
    if (prsnt) {
      ret = pal_get_uart_sel_pos(&pos);
      if (ret) {
        goto debug_card_out;
      }

      /*
       * Handle whether it is server postcode or BMC error code in
       * pal_post_handle() definition and display on USB debug card
      */
      // Determine what to display on Debug Card 7-Segment LED
      if (pos == UART_SEL_BMC) {
        uart_selection = HAND_SW_BMC;
        code = 0;

        //BMC Error Code Polling
        if (timer == 0) {
          pal_get_error_code(error, &count_ret);
        }

        //if Count is 0 means no error, shows 0
        if(count_ret == 0) {
          code = 0;
        }
        else {
          if(count_ret < count_cur) {
            index = 0;
          }
          count_cur = count_ret;
          //Each number shows twice while loop is around 1 second
          code = error[index/2];
          //Expander error code shows in 0~99 decimal, bmc shows A0~FF hexadecimal
          if(code < 100)
            code = code/10*16+code%10;

          index++;

          //when count_cur reach count_cur*2, means all sensors have been shown, so set index=0 to get error code again
          if(index >= count_cur*2)
            index = 0;
        }
        timer++;
        //while loop 6 times is around 3 seconds
        if(timer == 7)
          timer = 0;
      }
      // Only if UART Select is on Server, then get post code via bic and show it
      else if (pos == UART_SEL_SERVER) {
        uart_selection = HAND_SW_SERVER;
        code = 0;

        // Make sure the server is present
        ret = pal_is_fru_prsnt(FRU_SLOT1, &prsnt);
        if (ret || !prsnt) {
          goto debug_card_out;
        }

        // Get last post code and display it
        ret = pal_post_get_last(FRU_SLOT1, &code);
        if (ret) {
          goto debug_card_out;
        }
      } else {  // default UART output is BMC
        uart_selection = HAND_SW_BMC;
        code = 0;
      }

      ret = pal_post_handle(uart_selection, code);
      if (ret != 0) {
        goto debug_card_out;
      }
    }

debug_card_out:
    if (prsnt)
      msleep(500);
    else
      sleep(1);
  }
}

// Thread to monitor the hand switch
static void *
usb_handler() {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t pos;
  uint8_t prsnt;
  uint8_t lpc;

  while (1) {
    // Get the current hand switch position
    ret = pal_get_uart_sel_pos(&pos);
    if (ret) {
      goto hand_sw_out;
    }
    curr = pos;
    if (curr == prev) {
      // No state change, continue;
      goto hand_sw_out;
    }

    // Switch USB Mux to selected server
    ret = pal_switch_usb_mux(pos);
    if (ret) {
      goto hand_sw_out;
    }

    prev = curr;
hand_sw_out:
    sleep(1);
    continue;
  }
}

// Thread to monitor debug card reset button
static void *
dbg_rst_btn_handler() {
  uint8_t btn;
  uint8_t pos;
  uint8_t prsnt;
  int ret;
  int i;

  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if ((ret < 0) || (prsnt == 0)) {
      msleep(500);
      continue;
    }

    // Check if reset button is pressed
    ret = pal_get_dbg_rst_btn(&btn);
    if ((ret < 0) || (btn == 0)) {
      goto dbg_rst_btn_out;
    }

    // Check UART selection
    ret = pal_get_uart_sel_pos(&pos);
    if (ret < 0) {
      goto dbg_rst_btn_out;
    }

    syslog(LOG_WARNING, "Debug card reset button pressed\n");
    if (pos == UART_SEL_BMC) {
      syslog(LOG_CRIT, "Debug card reset button pressed for BMC\n");
      run_command("reboot");
      break;
    } else if (pos == UART_SEL_SERVER) {
      ret = pal_set_rst_btn(FRU_SLOT1, 0);
      if (ret < 0) {
        goto dbg_rst_btn_out;
      }

      // Wait for the button to be released
      for (i = 0; i < BTN_MAX_SAMPLES; i++) {
        ret = pal_get_dbg_rst_btn(&btn);
        if ((ret < 0) || (btn == 1)) {
          msleep(100);
          continue;
        }
        pal_update_ts_sled();
        syslog(LOG_WARNING, "Debug card reset button released\n");
        syslog(LOG_CRIT, "Debug card reset button pressed for FRU: %d\n", FRU_SLOT1);
        ret = pal_set_rst_btn(FRU_SLOT1, 1);
        goto dbg_rst_btn_out;
      }

      // handle error case
      if (i == BTN_MAX_SAMPLES) {
        pal_update_ts_sled();
        syslog(LOG_WARNING, "Debug card reset button seems to stuck for long time\n");
        goto dbg_rst_btn_out;
      }
    } else {
      goto dbg_rst_btn_out;
    }
dbg_rst_btn_out:
    msleep(100);
  }
}

// Thread to handle debug card power button and power on/off the selected server
static void *
dbg_pwr_btn_handler() {
  uint8_t btn, cmd;
  uint8_t power;
  uint8_t pos;
  uint8_t prsnt;
  int ret;
  int i;

  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if ((ret < 0) || (prsnt == 0)) {
      msleep(500);
      continue;
    }

    // Check if power button is pressed
    ret = pal_get_dbg_pwr_btn(&btn);
    if ((ret < 0) || (btn == 0)) {
      goto dbg_pwr_btn_out;
    }

    // Check UART selection
    ret = pal_get_uart_sel_pos(&pos);
    if (ret < 0) {
      goto dbg_pwr_btn_out;
    }

    if (pos == UART_SEL_BMC) {
      // do nothing
      goto dbg_pwr_btn_out;
    } else if (pos == UART_SEL_SERVER) {
      syslog(LOG_WARNING, "Debug card power button pressed\n");
      // Wait for the button to be released
      for (i = 0; i < BTN_POWER_OFF; i++) {
        ret = pal_get_dbg_pwr_btn(&btn);
        if ((ret < 0) || (btn == 1)) {
          msleep(100);
          continue;
        }
        syslog(LOG_WARNING, "Debug card power button released\n");
        break;
      }

      // Get the current power state (power on vs. power off)
      ret = pal_get_server_power(FRU_SLOT1, &power);
      if (ret < 0) {
        goto dbg_pwr_btn_out;
      }

      // Set power command should reverse of current power state
      cmd = !power;

      // To determine long button press
      if (i >= BTN_POWER_OFF) {
        pal_update_ts_sled();
        syslog(LOG_CRIT, "Debug card power button long pressed for FRU: %d\n", FRU_SLOT1);
      } else {

        // If current power state is ON and it is not a long press,
        // the power off should be Graceful Shutdown
        if (power == SERVER_POWER_ON) {
          cmd = SERVER_GRACEFUL_SHUTDOWN;
        }

        pal_update_ts_sled();
        syslog(LOG_CRIT, "Debug card power button pressed for FRU: %d\n", FRU_SLOT1);
      }

      // Reverse the power state of the given server
      ret = pal_set_server_power(FRU_SLOT1, cmd);
    } else {
      goto dbg_pwr_btn_out;
    }
dbg_pwr_btn_out:
    msleep(100);
  }
}

// Thread to monitor Reset Button and propagate to selected server
static void *
rst_btn_handler() {
  int ret;
  int i;
  uint8_t btn;

  while (1) {
    // Check if reset button is pressed
    ret = pal_get_rst_btn(&btn);
    if (ret || !btn) {
      goto rst_btn_out;
    }

    // Pass the reset button to the selected slot
    syslog(LOG_WARNING, "Reset button pressed\n");
    ret = pal_set_rst_btn(FRU_SLOT1, 0);
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
      syslog(LOG_CRIT, "Reset button pressed for FRU: %d\n", FRU_SLOT1);
      ret = pal_set_rst_btn(FRU_SLOT1, 1);
      goto rst_btn_out;
    }

    // handle error case
    if (i == BTN_MAX_SAMPLES) {
      pal_update_ts_sled();
      syslog(LOG_WARNING, "Reset button seems to stuck for long time\n");
      goto rst_btn_out;
    }
rst_btn_out:
    msleep(100);
  }
}

// Thread to handle Power Button and power on/off the selected server
static void *
pwr_btn_handler() {
  int ret;
  uint8_t btn, cmd;
  int i;
  uint8_t power;

  while (1) {
    // Check if power button is pressed
    ret = pal_get_pwr_btn(&btn);
    if (ret || !btn) {
      goto pwr_btn_out;
    }

    syslog(LOG_WARNING, "Power button pressed\n");
    // Wait for the button to be released
    for (i = 0; i < BTN_POWER_OFF; i++) {
      ret = pal_get_pwr_btn(&btn);
      if (ret || btn ) {
        msleep(100);
        continue;
      }
      syslog(LOG_WARNING, "Power button released\n");
      break;
    }


    // Get the current power state (power on vs. power off)
    ret = pal_get_server_power(FRU_SLOT1, &power);
    if (ret) {
      goto pwr_btn_out;
    }

    // Set power command should reverse of current power state
    cmd = !power;

    // To determine long button press
    if (i >= BTN_POWER_OFF) {
      pal_update_ts_sled();
      syslog(LOG_CRIT, "Power button long pressed for FRU: %d\n", FRU_SLOT1);
    } else {

      // If current power state is ON and it is not a long press,
      // the power off should be Graceful Shutdown
      if (power == SERVER_POWER_ON)
        cmd = SERVER_GRACEFUL_SHUTDOWN;

      pal_update_ts_sled();
      syslog(LOG_CRIT, "Power button pressed for FRU: %d\n", FRU_SLOT1);
    }

    // Reverse the power state of the given server
    ret = pal_set_server_power(FRU_SLOT1, cmd);
pwr_btn_out:
    msleep(100);
  }
}

// Thread to handle LED state of the server at given slot
static void *
led_handler(void *num) {
  int ret;
  uint8_t prsnt;
  uint8_t power;
  uint8_t pos;
  uint8_t led_blink;
  uint8_t ident = 0;
  int led_on_time, led_off_time;
  char identify[16] = {0};
  char tstr[64] = {0};
  int power_led_on_time = 500;
  int power_led_off_time = 500;
  uint8_t hlth = 0;

  uint8_t slot = (*(int*) num) + 1;

#ifdef DEBUG
  syslog(LOG_INFO, "led_handler for slot %d\n", slot);
#endif

  while (1) {
    // Check if this LED is managed by sync_led thread
    if (g_sync_led[slot]) {
      sleep(1);
      continue;
    }

    // Get power status for this slot
    ret = pal_get_server_power(slot, &power);
    if (ret) {
      sleep(1);
      continue;
    }

    // Get health status for this slot
    ret = pal_get_fru_health(slot, &hlth);
    if (ret) {
      sleep(1);
      continue;
    }

    led_blink = 0;  // solid on/off

    //If no identify: Set LEDs based on power and hlth status
    if (!led_blink) {
		pal_set_led(slot, LED_ON);
      goto led_handler_out;
    }

    // Set blink rate
    if (power == SERVER_POWER_ON) {
      led_on_time = 900;
      led_off_time = 100;
    } else {
      led_on_time = 100;
      led_off_time = 900;
    }

    // Start blinking the LED
    if (hlth == FRU_STATUS_GOOD) {
      pal_set_led(slot, LED_ON);
    }

    msleep(led_on_time);

    if (hlth == FRU_STATUS_GOOD) {
      pal_set_led(slot, LED_OFF);
    }

    msleep(led_off_time);
led_handler_out:
    msleep(100);
  }
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
  pal_set_led(slot, LED_ON);

  while (1) {
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

    // Handle BMC select condition when no slot is being identified
    if (ident == 1) {
      // Start blinking Blue LED
      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        g_sync_led[slot] = 1;
        pal_set_led(slot, LED_ON);
      }

      msleep(LED_ON_TIME_BMC_SELECT);

      for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
        pal_set_led(slot, LED_OFF);
      }

      msleep(LED_OFF_TIME_BMC_SELECT);
      continue;
    }
    for (slot = 1; slot <= MAX_NUM_SLOTS; slot++) {
      g_sync_led[slot] = 0;
    }
    msleep(200);
  }
}

// Thread for handling the IOM LED
static void *
iom_led_handler() {
  uint8_t slot1_hlth;
  uint8_t iom_hlth;
  uint8_t dpb_hlth;
  uint8_t scc_hlth;
  uint8_t nic_hlth;
  int ret;
  static int count = 0;
  uint8_t power_status;

  // Initial error code
  memset(g_err_code, 0, sizeof(unsigned char) * ERROR_CODE_NUM);

  while (1) {
    // Get health status for all the fru and then update the IOM LED status
    ret = pal_get_fru_health(FRU_SLOT1, &slot1_hlth);
    if (ret) {
      pal_err_code_enable(0xF7);
    }
    else {
      if (!slot1_hlth) {
        pal_err_code_enable(0xF7);
      }
      else {
        pal_err_code_disable(0xF7);
      }
    }

    ret = pal_get_fru_health(FRU_IOM, &iom_hlth);
    if (ret) {
      pal_err_code_enable(0xF8);
    }
    else {
      if (!iom_hlth) {
        pal_err_code_enable(0xF8);
      }
      else {
        pal_err_code_disable(0xF8);
      }
    }

    ret = pal_get_fru_health(FRU_DPB, &dpb_hlth);
    if (ret) {
      pal_err_code_enable(0xF9);
    }
    else {
      if (!dpb_hlth) {
        pal_err_code_enable(0xF9);
      }
      else {
        pal_err_code_disable(0xF9);
      }
    }

    ret = pal_get_fru_health(FRU_SCC, &scc_hlth);
    if (ret) {
      pal_err_code_enable(0xFA);
    }
    else {
      if (!scc_hlth) {
        pal_err_code_enable(0xFA);
      }
      else {
        pal_err_code_disable(0xFA);
      }
    }

    ret = pal_get_fru_health(FRU_NIC, &nic_hlth);
    if (ret) {
      pal_err_code_enable(0xFB);
    }
    else {
      if (!nic_hlth) {
        pal_err_code_enable(0xFB);
      }
      else {
        pal_err_code_disable(0xFB);
      }
    }

    //IOM LED Control Monitor
    if(pal_sum_error_code()) {
      //If there is any error code, make LED to solid Yellow
      pal_iom_led_control(IOM_LED_YELLOW);
      count = 0;
    }
    else {
      ret = pal_get_server_power(FRU_SLOT1, &power_status);
      if (ret) {
        // we don't change the LED indicator if fail to get power status
        syslog(LOG_WARNING, "%s, failed to get server power status", __func__);
      }
      else{
        if (power_status == SERVER_POWER_ON){
          pal_iom_led_control(IOM_LED_BLUE);
          count = 0;
        }
        else{
          // Server Power is off. Blinking Yellow  1s on -> 4s off
          if ((count%5) == 0)
            pal_iom_led_control(IOM_LED_YELLOW);
          else
            pal_iom_led_control(IOM_LED_OFF);

          count++;
        }
      }
    }

    sleep(1);
  }
}

// Thread for handling the BMC and SCC heartbeat stauts
static void *
hb_mon_handler() {
  int bmc_rmt_hb_value = -1;
  int scc_loc_hb_value = -1;
  int scc_rmt_hb_value = -1;
  int count_bmc_rmt = 0;
  int count_scc_loc = 0;
  int count_scc_rmt = 0;
  int curr_bmc_rmt_status = -1;
  int curr_scc_loc_status = -1;
  int curr_scc_rmt_status = -1;
  int prev_bmc_rmt_status = -1;
  int prev_scc_loc_status = -1;
  int prev_scc_rmt_status = -1;
  uint8_t iom_type = 0;
  uint8_t scc_rmt_type = 0;
  uint8_t hb_health = 0;
  int fru_health_last_state = 1;
  int fru_health_kv_state = 1;
  int ret = 0;
  char tmp_health[MAX_VALUE_LEN];

  // Get remote SCC and IOM type to identify IOM is M.2 or IOC solution
  iom_type = pal_get_iom_type();
  scc_rmt_type = ((pal_get_sku() >> 6) & 0x1);
  // Type 5
  if (scc_rmt_type == 0) {
    if (iom_type == IOM_IOC) {
      syslog(LOG_CRIT, "The chassis type is type V, the IOM type is IOC solution. The IOM does not match in this chassis. Default monitor type V HB.");
    }
    else if (iom_type != IOM_M2) {
      syslog(LOG_CRIT, "The chassis type is type V, the IOM type is unable to identify. Default monitor type V HB.");
    }
    iom_type = IOM_M2;
    hb_health = (1 << 5) | (1 << 4);
  }
  // Type 7
  else {
    if (iom_type == IOM_M2) {
      syslog(LOG_CRIT, "The chassis type is type VII, the IOM type is M.2 solution. The IOM does not match in this chassis. Default monitor type VII HB.");
    }
    else if (iom_type != IOM_IOC) {
      syslog(LOG_CRIT, "The chassis type is type VII, the IOM type is unable to identify. Default monitor type VII HB");
    }
    iom_type = IOM_IOC;
    hb_health = (1 << 4) | (1 << 3);
  }

  // Update heartbeat present bits [5:3] = {BMC_RMT, SCC_LOC, SCC_RMT}
  write_cache(PATH_HEARTBEAT_HEALTH, hb_health);

  while(1) {
    // get current health status from kv_store
    memset(tmp_health, 0, MAX_VALUE_LEN);
    ret = pal_get_key_value("heartbeat_health", tmp_health);
    if (ret){
      syslog(LOG_ERR, " %s - kv get heartbeat_health status failed", __func__);
    }
    fru_health_kv_state = atoi(tmp_health);

    // Heartbeat health bits [2:0] = {BMC_RMT, SCC_LOC, SCC_RMT}
    // Diagnosis flow:
    //   1. Get heartbeat
    //   2. Timeout detect: heartbeat has no response continuous 3 minutes. 1800 = 3 * 60 * 1000 / 100
    //   3. Cache heartbeat health in /tmp
    //   4. Update heartbeat status

    // BMC remote heartbeat
    if (iom_type == IOM_M2) {
      // Get heartbeat
      bmc_rmt_hb_value = pal_get_bmc_rmt_hb();
      if (bmc_rmt_hb_value <= BMC_RMT_HB_RPM_LIMIT) {
        count_bmc_rmt++;
      }
      else {
        count_bmc_rmt = 0;
      }
      // Timeout Detect
      if (count_bmc_rmt > BMC_RMT_HB_TIMEOUT_COUNT) {
        curr_bmc_rmt_status = 0;
      }
      else {
        curr_bmc_rmt_status = 1;
      }
      // Cache heartbeat health in /tmp
      if (curr_bmc_rmt_status == 0) {
        hb_health = hb_health | (1 << 2);
        if (curr_bmc_rmt_status != prev_bmc_rmt_status) {
          syslog(LOG_CRIT, "BMC remote heartbeat is abnormal");
        }
        pal_err_code_enable(0xFC);
        pal_set_key_value("heartbeat_health", "0");
      }
      else {
        hb_health = hb_health & (~(1 << 2));
        pal_err_code_disable(0xFC);
      }
      // Update heartbeat status
      prev_bmc_rmt_status = curr_bmc_rmt_status;
    }

    // SCC local heartbeat
    if ((iom_type == IOM_M2) || (iom_type == IOM_IOC)) {
      scc_loc_hb_value = pal_get_scc_loc_hb();
      if (scc_loc_hb_value <= SCC_LOC_HB_RPM_LIMIT) {
        count_scc_loc++;
      }
      else {
        count_scc_loc = 0;
      }
      if (count_scc_loc > SCC_LOC_HB_TIMEOUT_COUNT) {
        curr_scc_loc_status = 0;
      }
      else {
        curr_scc_loc_status = 1;
      }
      if (curr_scc_loc_status == 0) {
        hb_health = hb_health | (1 << 1);
        if (curr_scc_loc_status != prev_scc_loc_status) {
          syslog(LOG_CRIT, "SCC local heartbeat is abnormal");
        }
        pal_err_code_enable(0xFD);
        pal_set_key_value("heartbeat_health", "0");
      }
      else {
        hb_health = hb_health & (~(1 << 1));
        pal_err_code_disable(0xFD);
      }
      prev_scc_loc_status = curr_scc_loc_status;
    }

    // SCC remote heartbeat
    if (iom_type == IOM_IOC) {
      scc_rmt_hb_value = pal_get_scc_rmt_hb();
      if (scc_rmt_hb_value <= SCC_RMT_HB_RPM_LIMIT) {
        count_scc_rmt++;
      }
      else {
        count_scc_rmt = 0;
      }
      if (count_scc_rmt > SCC_RMT_HB_TIMEOUT_COUNT) {
        curr_scc_rmt_status = 0;
      }
      else {
        curr_scc_rmt_status = 1;
      }
      if (curr_scc_rmt_status == 0) {
        hb_health = hb_health | (1 << 0);
        if (curr_scc_rmt_status != prev_scc_rmt_status) {
          syslog(LOG_CRIT, "SCC remote heartbeat is abnormal");
        }
        pal_err_code_enable(0xFE);
        pal_set_key_value("heartbeat_health", "0");
      }
      else {
        hb_health = hb_health & (~(1 << 0));
        pal_err_code_disable(0xFE);
      }
      prev_scc_rmt_status = curr_scc_rmt_status;
    }
    write_cache(PATH_HEARTBEAT_HEALTH, hb_health);

    msleep(100);

    // If log-util clear all fru, cleaning heartbeat detection count
    // After doing it, front-paneld will regenerate assert
    if ((fru_health_kv_state != fru_health_last_state) && (fru_health_kv_state == 1)) {
      count_bmc_rmt = 0;
      count_scc_loc = 0;
      count_scc_rmt = 0;
    }
    fru_health_last_state = fru_health_kv_state;
  }
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_debug_card;
  pthread_t tid_dbg_rst_btn;
  pthread_t tid_dbg_pwr_btn;
  pthread_t tid_rst_btn;
  pthread_t tid_pwr_btn;
  pthread_t tid_sync_led;
  pthread_t tid_leds[MAX_NUM_SLOTS];
  pthread_t tid_iom_led;
  pthread_t tid_hb_mon;
  int i;
  int *ip;

  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
    exit(1);
  }

  if (pthread_create(&tid_dbg_rst_btn, NULL, dbg_rst_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card reset button error\n");
    exit(1);
  }

  if (pthread_create(&tid_dbg_pwr_btn, NULL, dbg_pwr_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card power button error\n");
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

  for (i = 0; i < MAX_NUM_SLOTS; i++) {
    ip = malloc(sizeof(int));
    *ip = i;
    if (pthread_create(&tid_leds[i], NULL, led_handler, (void*)ip) < 0) {
      syslog(LOG_WARNING, "pthread_create for led error\n");
      exit(1);
    }
  }

  if (pthread_create(&tid_iom_led, NULL, iom_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for iom led error\n");
    exit(1);
  }

  if (pthread_create(&tid_hb_mon, NULL, hb_mon_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for heartbeat error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_dbg_rst_btn, NULL);
  pthread_join(tid_dbg_pwr_btn, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  pthread_join(tid_sync_led, NULL);
  for (i = 0;  i < MAX_NUM_SLOTS; i++) {
    pthread_join(tid_leds[i], NULL);
  }
  pthread_join(tid_iom_led, NULL);
  pthread_join(tid_hb_mon, NULL);

  return 0;
}
