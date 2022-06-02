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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>

#define BTN_MAX_SAMPLES   200
#define FW_UPDATE_ONGOING 1
#define CRASHDUMP_ONGOING 2
#define LED_ON_TIME_IDENTIFY 500
#define LED_OFF_TIME_IDENTIFY 500


// Update the Identification LED for the given fru with the status
static int
is_btn_blocked(uint8_t fru) {

  if (pal_is_fw_update_ongoing(fru)) {
    return FW_UPDATE_ONGOING;
  }

  if (pal_is_crashdump_ongoing(fru)) {
    return CRASHDUMP_ONGOING;
  }
  return 0;
}

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  gpio_desc_t *PCH_BEEP = gpio_open_by_shadow("FM_PCH_BEEP_LED");
  gpio_desc_t *HDD_LED = gpio_open_by_shadow("FM_HDD_LED_N");
  gpio_desc_t *PCH_BEEP_OUT = gpio_open_by_shadow("FM_PCH_BEEP_LED_OUT");
  gpio_desc_t *HDD_LED_OUT = gpio_open_by_shadow("FM_HDD_LED_N_OUT");
  gpio_desc_t *ID_LED_OUT = gpio_open_by_shadow("FM_ID_LED_N");
  gpio_desc_t *PWR_GD = gpio_open_by_shadow(GPIO_CPU_POWER_GOOD);	
  gpio_desc_t *PWR_LED_OUT = gpio_open_by_shadow("BMC_PWR_LED");
  gpio_value_t value;
  uint8_t id_on = 0xFF;
  char identify[32];
  gpio_value_t pwr_led_val = GPIO_VALUE_LOW;
  gpio_value_t hd_led_val = GPIO_VALUE_LOW;
  gpio_value_t beep_led_val = GPIO_VALUE_LOW;

  while (1) {

    // Power_LED depands on PWR GD
    if (gpio_get_value(PWR_GD, &value) == 0 && pwr_led_val != value){
      gpio_set_value(PWR_LED_OUT, value);
      pwr_led_val = value;
    } else {
      fprintf(stderr, "gpio_get_value fail (GPIO_CPU_POWER_GOOD)\n");
    }

    // HDD_LED depands on sgpio
    if (gpio_get_value(HDD_LED, &value) == 0 && hd_led_val != value){
      gpio_set_value(HDD_LED_OUT, value ? GPIO_VALUE_LOW:GPIO_VALUE_HIGH);
      hd_led_val = value;
    } else {
      fprintf(stderr, "gpio_get_value fail (FM_HDD_LED_N)\n");
    }

    // ID LED and BEEP LED
    memset(identify, 0x0, sizeof(identify)); 
    // identify = on
    if (pal_get_key_value("identify_sled", identify) == 0 && !strcmp(identify, "on")) {
        id_on = 1;

        //Disable BEEP
        gpio_set_value(PCH_BEEP_OUT, GPIO_VALUE_LOW);

        // Start blinking the ID LED
        gpio_set_value(ID_LED_OUT, GPIO_VALUE_HIGH);
        msleep(LED_ON_TIME_IDENTIFY);
        gpio_set_value(ID_LED_OUT, GPIO_VALUE_LOW);
        msleep(LED_OFF_TIME_IDENTIFY);

    // identify = off & ID_LED still on
    } else if (id_on) {
      // PCH_BEEP depands on sgpio
      // ID_LED set low
      gpio_set_value(ID_LED_OUT, GPIO_VALUE_LOW);

      if (gpio_get_value(PCH_BEEP, &value) < 0) {
        fprintf(stderr, "gpio_get_value fail (FM_PCH_BEEP_LED)\n");
      } else {
        gpio_set_value(PCH_BEEP_OUT, value ? GPIO_VALUE_LOW:GPIO_VALUE_HIGH);
        id_on = false;
      }
    // identify = off
    } else {
      // PCH_BEEP depands on sgpio
      if (gpio_get_value(PCH_BEEP, &value) == 0 && beep_led_val != value) {
        gpio_set_value(PCH_BEEP_OUT, value);
        beep_led_val = value;
      } else {
        fprintf(stderr, "gpio_get_value fail (FM_PCH_BEEP_LED)\n");
      }
    }
    msleep(100);
  }

  gpio_close(PCH_BEEP);
  gpio_close(HDD_LED);
  gpio_close(PCH_BEEP_OUT);
  gpio_close(HDD_LED_OUT);
  gpio_close(ID_LED_OUT);
  gpio_close(PWR_GD);
  gpio_close(PWR_LED_OUT);

  return NULL;
}

static void *
rst_btn_handler() {
  int ret;
  int i;
  uint8_t btn;
  uint8_t last_btn = 0;

  ret = pal_get_rst_btn(&btn);
  if (0 == ret) {
    last_btn = btn;
  }

  while (1) {
    // Check if reset button is pressed
    ret = pal_get_rst_btn(&btn);
    if (ret || !btn) {
      if (last_btn != btn) {
        pal_set_rst_btn(FRU_MB, 1);
      }
      goto rst_btn_out;
    }
    ret = is_btn_blocked(FRU_MB);

    if (ret) {
      if (!last_btn) {
        switch (ret) {
          case FW_UPDATE_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to FW update is ongoing\n");
            break;
          case CRASHDUMP_ONGOING:
            syslog(LOG_CRIT, "Reset Button blocked due to crashdump is ongoing");
            break;
        }
      }
      goto rst_btn_out;
    }

    // Pass the reset button to the selected slot
    syslog(LOG_WARNING, "Reset button pressed\n");
    ret = pal_set_rst_btn(FRU_MB, 0);
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
      syslog(LOG_CRIT, "Reset Button pressed for FRU: %d\n", FRU_MB);
      ret = pal_set_rst_btn(FRU_MB, 1);
      if (!ret) {
        pal_set_restart_cause(FRU_MB, RESTART_CAUSE_RESET_PUSH_BUTTON);
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

int
main (int argc, char * const argv[]) {
  int pid_file, rc;
  pthread_t tid_sync_led;
  pthread_t tid_rst_btn;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pthread_create(&tid_sync_led, NULL, led_sync_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led sync error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for reset button error\n");
    exit(1);
  }

  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_rst_btn, NULL);

  return 0;
}
