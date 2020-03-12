/*
 * sensord
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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/libgpio.h>

#define POLL_TIMEOUT -1 /* Forever */

struct delayed_log {
  useconds_t usec;
  char msg[1024];
};

static void 
log_gpio_change(gpiopoll_pin_t *gp, gpio_value_t value, useconds_t log_delay)
{
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
}

void 
slot_present(gpiopoll_pin_t *gpdesc, gpio_value_t value) {
  log_gpio_change(gpdesc, value, 0);
}

static void 
slot_hotplug_setup(uint8_t slot_id) {
#define SCRIPT_PATH "sh /etc/init.d/server-init.sh %d &"
  char cmd[128] = {0};
  int cmd_len = sizeof(cmd);

  snprintf(cmd, cmd_len, SCRIPT_PATH, slot_id);
  if ( system(cmd) != 0 ) {
    syslog(LOG_CRIT, "Failed to run: %s", cmd);
  }
}

static void 
slot1_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const uint8_t slot_id = 0x1;
  slot_hotplug_setup(slot_id);
  log_gpio_change(gp, curr, 0);
}

static void
slot2_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const uint8_t slot_id = 0x2;
  slot_hotplug_setup(slot_id);
  log_gpio_change(gp, curr, 0);
}

static void
slot3_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const uint8_t slot_id = 0x3;
  slot_hotplug_setup(slot_id);
  log_gpio_change(gp, curr, 0);
}

static void
slot4_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const uint8_t slot_id = 0x4;
  slot_hotplug_setup(slot_id);
  log_gpio_change(gp, curr, 0);
}

static void 
ac_on_off_button_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(gp, curr, 0);
}

// GPIO table
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"PRSNT_MB_BMC_SLOT1_BB_N", "GPIOB4", GPIO_EDGE_BOTH, slot1_hotplug_hndlr, slot_present},
  {"PRSNT_MB_BMC_SLOT2_BB_N", "GPIOB5", GPIO_EDGE_BOTH, slot2_hotplug_hndlr, slot_present},
  {"PRSNT_MB_BMC_SLOT3_BB_N", "GPIOB6", GPIO_EDGE_BOTH, slot3_hotplug_hndlr, slot_present},
  {"PRSNT_MB_BMC_SLOT4_BB_N", "GPIOB7", GPIO_EDGE_BOTH, slot4_hotplug_hndlr, slot_present},
  {"AC_ON_OFF_BTN_SLOT1_N",       "GPIOL0", GPIO_EDGE_BOTH, ac_on_off_button_hndlr, NULL},
  {"AC_ON_OFF_BTN_BMC_SLOT2_N_R", "GPIOL1", GPIO_EDGE_BOTH, ac_on_off_button_hndlr, NULL},
  {"AC_ON_OFF_BTN_SLOT3_N",       "GPIOL2", GPIO_EDGE_BOTH, ac_on_off_button_hndlr, NULL},
  {"AC_ON_OFF_BTN_BMC_SLOT4_N_R", "GPIOL3", GPIO_EDGE_BOTH, ac_on_off_button_hndlr, NULL},
};

static void
*ac_button_event()
{
  gpio_desc_t *gpio = gpio_open_by_shadow("ADAPTER_BUTTON_BMC_CO_N_R");
  gpio_value_t value = GPIO_VALUE_INVALID;
  int time_elapsed = 0;

  if ( gpio == NULL ) {
    syslog(LOG_CRIT, "ADAPTER_BUTTON_BMC_CO_N_R pin monitoring not functional");
  }

  while (1) {
    if ( gpio_get_value(gpio, &value) ) {
      syslog(LOG_WARNING, "Could not get the current value of ADAPTER_BUTTON_BMC_CO_N_R");
      sleep(1);
      continue;
    }

    //The button is pressed
    if ( GPIO_VALUE_LOW == value) {
      time_elapsed++;
    } else if ( GPIO_VALUE_HIGH == value ) {
      //Check if a user is trying to do the sled cycle
      if ( time_elapsed > 4 ) {
        //only log the assertion of the event since the system is rebooted
        syslog(LOG_CRIT, "ASSERT: GPIOE3-ADAPTER_BUTTON_BMC_CO_N_R\n");
        if ( system("/usr/local/bin/power-util sled-cycle") != 0 ) {
          syslog(LOG_WARNING, "%s() Failed to do the sled cycle when pressed the adapter button", __func__);
        }
      }

      time_elapsed = 0;
    }
    
    //sleep periodically.
    sleep(1);
  }

  return NULL;
}

int
main(int argc, char **argv) {
  int rc = 0;
  int pid_file = 0;
  pthread_t tid_ac_button_event;
  gpiopoll_desc_t *polldesc = NULL;

  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiointrd instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiointrd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiointrd: daemon started");

    //Create a thread to monitor the button
    if (pthread_create(&tid_ac_button_event, NULL, ac_button_event, NULL) < 0) {
      syslog(LOG_WARNING, "Failed to create pthread_create for ac_button_event");
      exit(1);
    }

    polldesc = gpio_poll_open(g_gpios, sizeof(g_gpios)/sizeof(g_gpios[0]));
    if (!polldesc) {
      syslog(LOG_CRIT, "Cannot start poll operation on GPIOs");
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "Poll returned error");
      }
      gpio_poll_close(polldesc);
    }
  }

  return 0;
}
