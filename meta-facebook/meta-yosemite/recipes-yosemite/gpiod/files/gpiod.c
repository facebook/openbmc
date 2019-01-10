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
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/yosemite_gpio.h>

#define MAX_NUM_SLOTS       4
#define DELAY_GPIOD_READ    500000 // Polls each slot gpio values every 4*x usec
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"

#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"

/* To hold the gpio info and status */
typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_pin_t;

static gpio_pin_t gpio_slot1[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot2[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot3[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot4[MAX_GPIO_PINS] = {0};

/* Returns the pointer to the struct holding all gpio info for the fru#. */
static gpio_pin_t *
get_struct_gpio_pin(uint8_t fru) {

  gpio_pin_t *gpios;

  switch (fru) {
    case FRU_SLOT1:
      gpios = gpio_slot1;
      break;
    case FRU_SLOT2:
      gpios = gpio_slot2;
      break;
    case FRU_SLOT3:
      gpios = gpio_slot3;
      break;
    case FRU_SLOT4:
      gpios = gpio_slot4;
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
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "populate_gpio_pins: get_struct_gpio_pin failed.");
    return;
  }

  // Only monitor the PWRGD_COREPWR pin
  gpios[PWRGOOD_CPU].flag = 1;

  for (i = 0; i < MAX_GPIO_PINS; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GETBIT(gpio_ass_val, i);
      ret = yosemite_get_gpio_name(fru, i, gpios[i].name);
      if (ret < 0)
        continue;
    }
  }
}

/* Wrapper function to configure and get all gpio info */
static void
init_gpio_pins() {
  int fru;

  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_SLOTS); fru++) {
    populate_gpio_pins(fru);
  }

}

/* Monitor the gpio pins */
static int
gpio_monitor_poll(uint8_t fru_flag) {
  int i, ret;
  uint8_t fru;
  uint8_t slot_12v[MAX_NUM_SLOTS + 1];
  uint32_t revised_pins, n_pin_val, o_pin_val[MAX_NUM_SLOTS + 1] = {0};
  gpio_pin_t *gpios;
  char pwr_state[MAX_VALUE_LEN];
  char path[128];

  uint32_t status;
  bic_gpio_t gpio = {0};

  /* Check for initial Asserts */
  for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
    if (GETBIT(fru_flag, fru) == 0)
      continue;

    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, BMC_READY_N, 0);

    ret = bic_get_gpio(fru, &gpio);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for "
        " fru %u", fru);
#endif
      continue;
    }

    gpios = get_struct_gpio_pin(fru);
    if  (gpios == NULL) {
      syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for"
          " fru %u", fru);
      continue;
    }

    memcpy(&status, (uint8_t *) &gpio, sizeof(status));

    slot_12v[fru] = 1;
    o_pin_val[fru] = 0;

    for (i = 0; i < MAX_GPIO_PINS; i++) {

      if (gpios[i].flag == 0)
        continue;

      gpios[i].status = GETBIT(status, i);

      if (gpios[i].status)
        o_pin_val[fru] = SETBIT(o_pin_val[fru], i);
    }
  }

  /* Keep monitoring each fru's gpio pins every 4 * GPIOD_READ_DELAY seconds */
  while(1) {
    for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
      if (!(GETBIT(fru_flag, fru))) {
        usleep(DELAY_GPIOD_READ);
        continue;
      }
      if (slot_12v[fru] == 0) {  // workaround, may get fake PWRGOOD_CPU status when slot12V is just turned on
        pal_is_server_12v_on(fru, &slot_12v[fru]);
        usleep(DELAY_GPIOD_READ);
        continue;
      }

      gpios = get_struct_gpio_pin(fru);
      if  (gpios == NULL) {
        syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for"
            " fru %u", fru);
        continue;
      }

      memset(pwr_state, 0, MAX_VALUE_LEN);
      pal_get_last_pwr_state(fru, pwr_state);

      /* Get the GPIO pins */
      if ((ret = bic_get_gpio(fru, (bic_gpio_t *) &n_pin_val)) < 0) {
#ifdef DEBUG
        /* log the error message only when the CPU is on but not reachable. */
        if (!(strcmp(pwr_state, "on"))) {
          syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for fru %u", fru);
        }
#endif

        if ((pal_is_server_12v_on(fru, &slot_12v[fru]) != 0) || slot_12v[fru]) {
          usleep(DELAY_GPIOD_READ);
          continue;
        }
        n_pin_val = CLEARBIT(o_pin_val[fru], PWRGOOD_CPU);
      }

      if (o_pin_val[fru] == n_pin_val) {
        usleep(DELAY_GPIOD_READ);
        continue;
      }

      revised_pins = (n_pin_val ^ o_pin_val[fru]);

      for (i = 0; i < MAX_GPIO_PINS; i++) {
        if (GETBIT(revised_pins, i) && (gpios[i].flag == 1)) {
          gpios[i].status = GETBIT(n_pin_val, i);

          // Check if the new GPIO val is ASSERT
          if (gpios[i].status == gpios[i].ass_val) {
            /*
             * GPIO - PWRGOOD_CPU assert indicates that the CPU is turned off or in a bad shape.
             * Raise an error and change the LPS from on to off or vice versa for deassert.
             */
            if (strcmp(pwr_state, "off")) {
              if ((pal_is_server_12v_on(fru, &slot_12v[fru]) != 0) || slot_12v[fru]) {
                // Check if power-util is still running to ignore getting incorrect power status
                sprintf(path, PWR_UTL_LOCK, fru);
                if (access(path, F_OK) != 0) {
                  pal_set_last_pwr_state(fru, "off");
                }
              }
            }
            syslog(LOG_CRIT, "FRU: %d, System powered OFF", fru);

            // Inform BIOS that BMC is ready
            bic_set_gpio(fru, BMC_READY_N, 0);
          } else {
            if (strcmp(pwr_state, "on")) {
              if ((pal_is_server_12v_on(fru, &slot_12v[fru]) != 0) || slot_12v[fru]) {
                // Check if power-util is still running to ignore getting incorrect power status
                sprintf(path, PWR_UTL_LOCK, fru);
                if (access(path, F_OK) != 0) {
                  pal_set_last_pwr_state(fru, "on");
                }
              }
            }
            syslog(LOG_CRIT, "FRU: %d, System powered ON", fru);
          }
        }
      }

      o_pin_val[fru] = n_pin_val;
      usleep(DELAY_GPIOD_READ);

    } /* For Loop for each fru */
  } /* while loop */
  return 0;
} /* function definition*/

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, char **argv) {
  int i, ret;
  uint8_t fru_flag, fru;

  /* Check for which fru do we need to monitor the gpio pins */
  fru_flag = 0;
  for (i = 1; i < argc; i++) {
    ret = pal_get_fru_id(argv[i], &fru);
    if (ret < 0) {
      print_usage();
      exit(-1);
    }
    fru_flag = SETBIT(fru_flag, fru);
  }

  gpio_monitor_poll(fru_flag);
}

int
main(int argc, char **argv) {
  int rc, pid_file;

  if (argc < 2) {
    print_usage();
    exit(-1);
  }

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
    run_gpiod(argc, argv);
  }

  return 0;
}
