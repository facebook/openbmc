/*
 * gpiod
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/minipack_gpio.h>

#define MAX_NUM_SLOTS       1
#define DELAY_GPIOD_READ    500000 // Polls each slot gpio values every 4*x usec
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"
#define IPMB_BUS 0

/* To hold the gpio info and status */
typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_pin_t;

static gpio_pin_t gpio_slot1[BIC_GPIO_MAX] = {0};

/* Returns the pointer to the struct holding all gpio info for the fru#. */
static gpio_pin_t *
get_struct_gpio_pin(uint8_t fru) {

  gpio_pin_t *gpios;

  switch (fru) {
    case IPMB_BUS:
      gpios = gpio_slot1;
      break;
    default:
      syslog(LOG_WARNING, "get_struct_gpio_pin: Wrong SLOT ID %d\n", fru);
      return NULL;
  }

  return gpios;
}

static void
populate_gpio_pins(uint8_t fru) {

  int i, ret;
  const char *name;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    return;
  }

  // Only monitor the PWRGD_COREPWR pin
  gpios[PWRGOOD_CPU].flag = 1;

  for (i = 0; i < BIC_GPIO_MAX; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GETBIT(gpio_ass_val, i);
      name = minipack_gpio_type_to_name(i);
      if (name[0] != '\0')
        strncpy(gpios[i].name, name, sizeof(gpios[i].name));
    }
  }
}

/* Wrapper function to configure and get all gpio info */
static void
init_gpio_pins() {
  populate_gpio_pins(IPMB_BUS);
}


/* Monitor the gpio pins */
static int
gpio_monitor_poll() {
  int i, ret;
  uint32_t revised_pins, n_pin_val, o_pin_val[MAX_NUM_SLOTS] = {0};
  gpio_pin_t *gpios;
  char pwr_state[MAX_VALUE_LEN];
  bic_gpio_t gpio = {0};

  // Inform BIOS that BMC is ready

  ret = bic_set_gpio(IPMB_BUS, BMC_READY_N, 0);
  ret = bic_get_gpio(IPMB_BUS, &gpio);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: bic_get_gpio failed for fru %u",
           __func__, IPMB_BUS);
#endif
    return -1;
  }

  gpios = get_struct_gpio_pin(IPMB_BUS);
  if  (gpios == NULL) {
    return -1;
  }

  pal_light_scm_led(SCM_LED_AMBER);
  o_pin_val[0] = 0;

  /* Keep monitoring each fru's gpio pins every 4 * GPIOD_READ_DELAY seconds */
  while(1) {
    memset(pwr_state, 0, MAX_VALUE_LEN);
    pal_get_last_pwr_state(FRU_SCM, pwr_state);

    /* Get the GPIO pins */
    if ((ret = bic_get_gpio(IPMB_BUS, (bic_gpio_t *) &n_pin_val)) < 0) {
      n_pin_val = CLEARBIT(o_pin_val[0], PWRGOOD_CPU);
    }

    if (o_pin_val[0] == n_pin_val) {
      usleep(DELAY_GPIOD_READ);
      continue;
    }

    revised_pins = (n_pin_val ^ o_pin_val[0]);

    for (i = 0; i < BIC_GPIO_MAX; i++) {
      if (GETBIT(revised_pins, i) && (gpios[i].flag == 1)) {
        gpios[i].status = GETBIT(n_pin_val, i);

        // Check if the new GPIO val is ASSERT
        if (gpios[i].status == gpios[i].ass_val) {
          if (strcmp(pwr_state, "off")) {
            pal_set_last_pwr_state(FRU_SCM, "off");
          }
          syslog(LOG_CRIT, "FRU: %d, System powered OFF", IPMB_BUS);
          pal_light_scm_led(SCM_LED_AMBER);
        } else {
          // Inform BIOS that BMC is ready
          bic_set_gpio(IPMB_BUS, BMC_READY_N, 0);
          if (strcmp(pwr_state, "on")) {
            pal_set_last_pwr_state(FRU_SCM, "on");
          }
          syslog(LOG_CRIT, "FRU: %d, System powered ON", IPMB_BUS);
          pal_light_scm_led(SCM_LED_BLUE);
        }
      }
    }

    o_pin_val[0] = n_pin_val;
    usleep(DELAY_GPIOD_READ);
  } /* while loop */

  return 0; /* never reached */
} /* function definition*/

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod() {
  gpio_monitor_poll();
}

int
main(int argc, void **argv) {
  int rc, pid_file;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {
    init_gpio_pins();
    daemon(0,1);
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");
    run_gpiod();
  }

  return 0;
}
