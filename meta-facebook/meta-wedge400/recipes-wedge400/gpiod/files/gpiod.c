/*
 * gpiod
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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <getopt.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/log.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/wedge400_gpio.h>

#define GPIOD_NAME          "gpiod"
#define GPIOD_PID_FILE      "/var/run/gpiod.pid"
#define DELAY_GPIOD_READ    500000 // Polls each slot gpio values every 4*x usec
#define SLOT_NUM_MAX        1

#define MAX_NUM_SLOTS       1
#define DELAY_GPIOD_READ    500000 // Polls each slot gpio values every 4*x usec
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"
#define IPMB_BUS 			0
#define GPIOD_VERBOSE(fmt, args...) \
  do {                              \
    if (verbose_logging)            \
      OBMC_INFO(fmt, ##args);       \
  } while (0)

/* To hold the gpio info and status */
typedef struct {
  uint8_t flag:1;
  uint8_t status:1;
  uint8_t ass_val:1;
  char name[32];
} gpio_pin_t;

static bool verbose_logging;
static gpio_pin_t gpio_slot1[MAX_GPIO_PINS] = {0};

/* Returns the pointer to the struct holding all gpio info for the fru#. */
static gpio_pin_t *
get_struct_gpio_pin(uint8_t fru) {

  gpio_pin_t *gpios;

  switch (fru) {
    case IPMB_BUS:
      gpios = gpio_slot1;
      break;
    default:
      return NULL;
  }

  return gpios;
}

static int
enable_gpio_intr_config(uint8_t fru, uint8_t gpio) {
  int ret;

  bic_gpio_config_t cfg = {0};
  bic_gpio_config_t verify_cfg = {0};


  ret =  bic_get_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    OBMC_ERROR(ret, "enable_gpio_intr_config: bic_get_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  cfg.ie = 1;

  ret = bic_set_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    OBMC_ERROR(ret, "enable_gpio_intr_config: bic_set_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  ret =  bic_get_gpio_config(fru, gpio, &verify_cfg);
  if (ret < 0) {
    OBMC_ERROR(ret, "enable_gpio_intr_config: verification bic_get_gpio_config"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  if (verify_cfg.ie != cfg.ie) {
    OBMC_WARN("Slot_id: %u,Interrupt enabling FAILED for GPIO pin# %d",
        fru, gpio);
    return -1;
  }

  return 0;
}

static void
populate_gpio_pins(uint8_t fru) {

  int i, ret;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    OBMC_WARN("populate_gpio_pins: get_struct_gpio_pin failed.");
    return;
  }

  // Only monitor the PWRGD_COREPWR pin
  gpios[PWRGOOD_CPU].flag = 1;

  for (i = 0; i < BIC_GPIO_MAX; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GETBIT(gpio_ass_val, i);
      GPIOD_VERBOSE("start monitoring '%s', ass_val=%u\n",
                    wedge400_gpio_type_to_name(i), gpios[i].ass_val);
    }
  }
}

/* Wrapper function to configure and get all gpio info */
static void
init_gpio_pins(void) {
  populate_gpio_pins(IPMB_BUS);
}


/* Monitor the gpio pins */
static int
gpio_monitor_poll(void) {
  int i, ret;
  uint32_t revised_pins, n_pin_val, o_pin_val[MAX_NUM_SLOTS] = {0};
  gpio_pin_t *gpios;
  char pwr_state[MAX_VALUE_LEN];
  char path[128];

  uint32_t status;
  bic_gpio_t gpio = {0};

  // Inform BIOS that BMC is ready

  ret = bic_set_gpio(IPMB_BUS, BMC_READY_N, 0);
  if (ret) {
    OBMC_WARN("bic_set_gpio failed for fru %u", IPMB_BUS);
  }
  ret = bic_get_gpio(IPMB_BUS, &gpio);
  if (ret) {
#ifdef OBMC_DEBUG
    OBMC_WARN("gpio_monitor_poll: bic_get_gpio failed for "
      " fru %u", IPMB_BUS);
#endif
    goto end;
  }

  gpios = get_struct_gpio_pin(IPMB_BUS);
  if  (gpios == NULL) {
    OBMC_WARN("gpio_monitor_poll: get_struct_gpio_pin failed for"
        " IPMB_BUS %u", IPMB_BUS);
    goto end;
  }

  memcpy(&status, (uint8_t *) &gpio, sizeof(status));

  for (i = 0; i < MAX_GPIO_PINS; i++) {

    if (gpios[i].flag == 0)
      continue;

    gpios[i].status = GETBIT(status, i);

    if (gpios[i].status)
      o_pin_val[0] = SETBIT(o_pin_val[0], i);
  }
  
  pal_light_scm_led(SCM_LED_AMBER);
  o_pin_val[0] = 0;

  /* Keep monitoring each fru's gpio pins every 4 * GPIOD_READ_DELAY seconds */
  while(1) {

    gpios = get_struct_gpio_pin(IPMB_BUS);
    if  (gpios == NULL) {
      OBMC_WARN("gpio_monitor_poll: get_struct_gpio_pin failed for"
          " fru %u", IPMB_BUS);
      break;
    }

    memset(pwr_state, 0, MAX_VALUE_LEN);
    pal_get_last_pwr_state(FRU_SCM, pwr_state);

    /* Get the GPIO pins */
    if ((ret = bic_get_gpio(IPMB_BUS, (bic_gpio_t *) &n_pin_val)) < 0) {
      n_pin_val = CLEARBIT(o_pin_val[0], PWRGOOD_CPU);
    }

    if (o_pin_val[0] == n_pin_val) {
      GPIOD_VERBOSE("pin_val not changed. Sleeping for %u microseconds",
                    DELAY_GPIOD_READ);
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
          OBMC_CRIT("FRU: %d, GPIO: %d, System powered OFF", IPMB_BUS, i);
          pal_light_scm_led(SCM_LED_AMBER);
        } else {
          // Inform BIOS that BMC is ready
          bic_set_gpio(IPMB_BUS, BMC_READY_N, 0);
          if (strcmp(pwr_state, "on")) {
            pal_set_last_pwr_state(FRU_SCM, "on");
          }
          OBMC_CRIT("FRU: %d, GPIO: %d, System powered ON", IPMB_BUS, i);
          pal_light_scm_led(SCM_LED_BLUE);
        }
      }
    }

    o_pin_val[0] = n_pin_val;
    usleep(DELAY_GPIOD_READ);
  } /* while loop */

end:
  return 0;
} /* function definition*/

static void
dump_usage(const char *prog_name)
{
  int i;
  struct {
    const char *opt;
    const char *desc;
  } options[] = {
    {"-h|--help", "print this help message"},
    {"-v|--verbose", "enable verbose logging"},
    {"-f|--foreground", "run the process in foreground"},
    {NULL, NULL},
  };

  printf("Usage: %s [options]\n", prog_name);
  for (i = 0; options[i].opt != NULL; i++) {
    printf("    %-18s - %s\n", options[i].opt, options[i].desc);
  }
}

int
main(int argc, char **argv) {
  int ret, pid_file;
  bool foreground = false;
  struct option long_opts[] = {
    {"help",       no_argument, NULL, 'h'},
    {"verbose",    no_argument, NULL, 'v'},
    {"foreground", no_argument, NULL, 'f'},
    {NULL,         0,           NULL, 0},
  };

  while (1) {
    int opt_index = 0;
    int ret = getopt_long(argc, argv, "hvf", long_opts, &opt_index);
    if (ret == -1)
      break; /* end of arguments */

    switch (ret) {
    case 'h':
      dump_usage(argv[0]);
      return 0;

    case 'v':
      verbose_logging = true;
      break;

    case 'f':
      foreground = true;
      break;

    default:
      return -1;
    }
  } /* while */


  /*
   * Make sure only 1 instance of gpiod is running.
   */
  pid_file = open(GPIOD_PID_FILE, O_CREAT | O_RDWR, 0666);
  if (pid_file < 0) {
    fprintf(stderr, "%s: failed to open %s: %s\n",
            GPIOD_NAME, GPIOD_PID_FILE, strerror(errno));
    return -1;
  }
  if (flock(pid_file, LOCK_EX | LOCK_NB) != 0) {
    if(EWOULDBLOCK == errno) {
      fprintf(stderr, "%s: another instance is running. Exiting..\n",
              GPIOD_NAME);
    } else {
      fprintf(stderr, "%s: failed to lock %s: %s\n",
              GPIOD_NAME, GPIOD_PID_FILE, strerror(errno));
    }
    close(pid_file);
    return -1;
  }

  /*
   * Initialize logging facility.
   */
  if (verbose_logging) {
    ret = obmc_log_init(GPIOD_NAME, LOG_DEBUG, 0);
  } else {
    ret = obmc_log_init(GPIOD_NAME, LOG_INFO, 0);
  }
  if (ret != 0) {
    fprintf(stderr, "%s: failed to initialize logger: %s\n",
            GPIOD_NAME, strerror(errno));
    return -1;
  }

  /*
   * Enter daemon mode if required.
   */
  if (!foreground) {
    obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
    obmc_log_unset_std_stream();
    if (daemon(0, 1) != 0) {
      OBMC_ERROR(errno, "failed to enter daemon mode");
      return -1;
    }
  }

  init_gpio_pins();

  OBMC_INFO("%s: daemon started", GPIOD_NAME);
  gpio_monitor_poll(); /* main loop */

  flock(pid_file, LOCK_UN);
  close(pid_file);
  return 0;
}
