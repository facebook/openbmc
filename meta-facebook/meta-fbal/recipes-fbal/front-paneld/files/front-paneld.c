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
#include <openbmc/misc-utils.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>

#define LED_ON_TIME_IDENTIFY 500
#define LED_OFF_TIME_IDENTIFY 500

#define BTN_MAX_SAMPLES   200
#define FW_UPDATE_ONGOING 1
#define CRASHDUMP_ONGOING 2


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

static int
recover_cm() {
  ipmi_dev_id_t cm_dev_id;
  syslog(LOG_CRIT, "ASSERT: Chassis Manager unresponsive. Initiating remediation");
  gpio_desc_t *rst_gpio = gpio_open_by_shadow("RST_CM_N");
  if (!rst_gpio) {
    syslog(LOG_CRIT, "ASSERT: Chassis Manager remediation failed: Cannot open GPIO");
    return 1;
  }
  if (gpio_set_value(rst_gpio, GPIO_VALUE_LOW)) {
    syslog(LOG_WARNING, "RST_CM_N set to low failed");
  }
  sleep(0.5);
  if (gpio_set_value(rst_gpio, GPIO_VALUE_HIGH)) {
    syslog(LOG_WARNING, "RST_CM_N set to high failed");
  }
  gpio_close(rst_gpio);
  // Give CM 5 seconds to start up.
  sleep(5);
  int ret = retry_cond(cmd_cmc_get_dev_id(&cm_dev_id) == 0, 15, 1000);
  if (ret == 0) {
    syslog(LOG_CRIT, "DEASSERT: Chassis Manager unresponsive. Recovered");
  }
  return ret;
}

static void *
cm_monitor() {
  ipmi_dev_id_t cm_dev_id;
  int recovery_tries = 0;
  int last_state = 0;
  while (1) {
    // Try for 15s to get to CM.
    if (retry_cond(cmd_cmc_get_dev_id(&cm_dev_id) == 0, 15, 1000) == 0) {
      // All is good, try again later.
      recovery_tries = 0;
      if (last_state != 0) {
        // If previous recovery attempt had failed, we need to add
        // a DEASSERT here.
        syslog(LOG_CRIT, "DEASSERT: Chassis Manager unresponsive. Recovered");
      }
      last_state = 0;
    } else if (recovery_tries < 10) {
      // Try to recover CM 10 times, after that give up.
      last_state = recover_cm();
      recovery_tries++;
    }
    sleep(5);
  }
  return NULL;
}

// Thread to handle LED state of the SLED
static void *
led_sync_handler() {
  int ret;
  uint8_t id_on = 0xFF;
  char identify[MAX_VALUE_LEN] = {0};

  while (1) {
    // Handle Slot IDENTIFY condition
    memset(identify, 0x0, sizeof(identify));
    ret = pal_get_key_value("identify_sled", identify);
    if (ret == 0 && !strcmp(identify, "on")) {
      id_on = 1;

      // Start blinking the ID LED
      pal_set_id_led(FRU_MB, LED_ON);
      msleep(LED_ON_TIME_IDENTIFY);

      pal_set_id_led(FRU_MB, LED_OFF);
      msleep(LED_OFF_TIME_IDENTIFY);
      continue;
    } else if (id_on) {
      id_on = 0;
      pal_set_id_led(FRU_MB, 0xFF);
    }

    sleep(1);
  }
  return NULL;
}

static void *
fault_led_handler() {
  int ret;
  uint8_t mb_health = FRU_STATUS_GOOD, pdb_health = FRU_STATUS_GOOD;
  uint8_t nic0_health = FRU_STATUS_GOOD, nic1_health = FRU_STATUS_GOOD;

  while (1) {
    if ((ret = pal_get_fru_health(FRU_MB, &mb_health))) {
      syslog(LOG_WARNING, "Fail to get MB health");
    }

    if ((ret = pal_get_fru_health(FRU_PDB, &pdb_health))) {
      syslog(LOG_WARNING, "Fail to get PDB health");
    }

    if ((ret = pal_get_fru_health(FRU_NIC0, &nic0_health))) {
      syslog(LOG_WARNING, "Fail to get NIC0 health");
    }

    if ((ret = pal_get_fru_health(FRU_NIC1, &nic1_health))) {
      syslog(LOG_WARNING, "Fail to get NIC1 health");
    }

    if ((mb_health != FRU_STATUS_GOOD) || (pdb_health != FRU_STATUS_GOOD) ||
        (nic0_health != FRU_STATUS_GOOD) || (nic1_health != FRU_STATUS_GOOD)) {
      pal_set_fault_led(FRU_MB, LED_N_ON);
    } else {
      pal_set_fault_led(FRU_MB, LED_N_OFF);
    }

    sleep(2);
  }

  return NULL;
}

static void *
rst_btn_handler() {
  int ret;
  uint8_t pos = FRU_MB;
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
        pal_set_rst_btn(pos, 1);
      }
      goto rst_btn_out;
    }
    ret = is_btn_blocked(pos);

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

int
main (int argc, char * const argv[]) {
  int pid_file, rc;
  pthread_t tid_sync_led;
  pthread_t tid_fault_led;
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

  if (pthread_create(&tid_fault_led, NULL, fault_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for led error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for reset button error\n");
    exit(1);
  }
  uint8_t pos = 0;
  pthread_t tid_cm_monitor;
  pal_get_mb_position(&pos);
  if (pos == 0) {
    if (pthread_create(&tid_cm_monitor, NULL, cm_monitor, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for cm monitor errror\n");
      exit(1);
    }
  }

  pthread_join(tid_sync_led, NULL);
  pthread_join(tid_fault_led, NULL);
  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_cm_monitor, NULL);

  return 0;
}
