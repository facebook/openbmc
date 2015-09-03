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

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)

#define MAX_NUM_SLOTS       4
#define GPIOD_READ_DELAY    1
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"

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
      syslog(LOG_ALERT, "get_struct_gpio_pin: Wrong SLOT ID %d\n", fru);
      return NULL;
  }

  return gpios;
}

int
enable_gpio_intr_config(uint8_t fru, uint8_t gpio) {
  int ret;

  bic_gpio_config_t cfg = {0};
  bic_gpio_config_t verify_cfg = {0};


  ret =  bic_get_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: bic_get_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  cfg.ie = 1;

  ret = bic_set_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: bic_set_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  ret =  bic_get_gpio_config(fru, gpio, &verify_cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: verification bic_get_gpio_config"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  if (verify_cfg.ie != cfg.ie) {
    syslog(LOG_ALERT, "Slot_id: %u,Interrupt enabling FAILED for GPIO pin# %d",
        fru, gpio);
    return -1;
  }

  return 0;
}

/* Enable the interrupt mode for all the gpio sensors */
static void
enable_gpio_intr(uint8_t fru) {

  int i, ret;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_ALERT, "enable_gpio_intr: get_struct_gpio_pin failed.");
    return;
  }

  for (i = 0; i < gpio_pin_cnt; i++) {
    ret = enable_gpio_intr_config(fru, gpio_pin_list[i]);
    if (ret < 0) {
      gpios[i].flag = 0;
      syslog(LOG_ALERT, "enable_gpio_intr: Slot: %d, Pin %d interrupt enabling"
          " failed", fru, gpio_pin_list[i]);
      syslog(LOG_ALERT, "enable_gpio_intr: Disable check for Slot %d, Pin %d",
          fru, gpio_pin_list[i]);
    } else {
      gpios[i].flag = 1;
#ifdef DEBUG
      syslog(LOG_ALERT, "enable_gpio_intr: Enabled check for Slot: %d, Pin %d",
          fru, gpio_pin_list[i]);
#endif /* DEBUG */
    }
  }
}

static void
populate_gpio_pins(uint8_t fru) {

  int i, ret;

  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_ALERT, "populate_gpio_pins: get_struct_gpio_pin failed.");
    return;
  }

  for(i = 0; i < gpio_pin_cnt; i++) {
    gpios[gpio_pin_list[i]].flag = 1;
  }


  for(i = 0; i < MAX_GPIO_PINS; i++) {
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
  uint32_t revised_pins, n_pin_val, o_pin_val[MAX_NUM_SLOTS + 1] = {0};
  gpio_pin_t *gpios;

  uint32_t status;
  bic_gpio_t gpio = {0};

  /* Check for initial Asserts */
  for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
    if (GETBIT(fru_flag, fru) == 0)
      continue;

    ret = bic_get_gpio(fru, &gpio);
    if (ret) {
      syslog(LOG_ALERT, "populate_gpio_pins: bic_get_gpio failed for "
        " fru %u", fru);
      continue;
    }

    gpios = get_struct_gpio_pin(fru);
    if  (gpios == NULL) {
      syslog(LOG_ALERT, "gpio_monitor_poll: get_struct_gpio_pin failed for"
          " fru %u", fru);
      continue;
    }

    memcpy(&status, (uint8_t *) &gpio, sizeof(status));

    o_pin_val[fru] = 0;

    for (i = 0; i <= MAX_GPIO_PINS; i++) {

      if (gpios[i].flag == 0)
        continue;

      gpios[i].status = GETBIT(status, i);

      if (gpios[i].status)
        o_pin_val[fru] = SETBIT(o_pin_val[fru], i);

      if (gpios[i].status == gpios[i].ass_val) {
          syslog(LOG_CRIT, "ASSERT: fru: %u, gpio pin: %-20s, num: %d",
              fru, gpios[i].name, i);
      }
    }
  }

  /* Keep monitoring each fru's gpio pins every 4 * GPIOD_READ_DELAY seconds */
  while(1) {
    for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
      if (!(GETBIT(fru_flag, fru))) {
        sleep(GPIOD_READ_DELAY);
        continue;
      }

      gpios = get_struct_gpio_pin(fru);
      if  (gpios == NULL) {
        syslog(LOG_ALERT, "gpio_monitor_poll: get_struct_gpio_pin failed for"
            " fru %u", fru);
        continue;
      }

      if ((ret = bic_get_gpio(fru, (bic_gpio_t *) &n_pin_val)) < 0) {
        syslog(LOG_ALERT, "gpio_monitor_poll: bic_get_gpio failed for "
            " fru %u", fru);
        continue;
      }

      if (o_pin_val[fru] == n_pin_val) {
        o_pin_val[fru] = n_pin_val;
        sleep(GPIOD_READ_DELAY);
        continue;
      }

      revised_pins = (n_pin_val ^ o_pin_val[fru]);

      for (i = 0; i < MAX_GPIO_PINS; i++) {
        if (GETBIT(revised_pins, i) & gpios[i].flag) {
          gpios[i].status = GETBIT(n_pin_val, i);

          // Check if the new GPIO val is ASSERT
          if (gpios[i].status == gpios[i].ass_val) {
            syslog(LOG_CRIT, "ASSERT: fru: %u, gpio pin: %-20s, num: %d",
                fru, gpios[i].name, i);
          } else {
              syslog(LOG_CRIT, "DEASSERT: fru: %u, gpio pin: %-20s, num: %d",
                  fru, gpios[i].name, i);
          }
        }
      }

      o_pin_val[fru] = n_pin_val;
      sleep(GPIOD_READ_DELAY);

    } /* For Loop for each fru */
  } /* while loop */
} /* function definition*/

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", "slot1, slot2, slot3, slot4");
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, void **argv) {

  //gpio_monitor();

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
main(int argc, void **argv) {
  int dev, rc, pid_file;

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
