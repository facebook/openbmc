/*
 * bicmond
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
#include <string.h>
#include <getopt.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/log.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <openbmc/misc-utils.h>

#define BICMOND_NAME          "bicmond"
#define DELAY_BICMOND_READ    5 // minimum interval of GPIO read, in seconds.

#define GPIOD_VERBOSE(fmt, args...) \
  do {                              \
    if (verbose_logging)            \
      OBMC_INFO(fmt, ##args);       \
  } while (0)

/* To hold the gpio info and status */
typedef struct bic_gpio_pin {
  uint32_t flag:1;
  uint32_t status:1;
  uint32_t ass_val:1;

  int (*handler)(struct bic_gpio_pin *gpio, void *args);
} gpio_pin_t;

static bool verbose_logging;

static const uint32_t gpio_ass_val = (1 << FM_CPLD_FIVR_FAULT);

static int bic_cpu_pwr_handler(gpio_pin_t *gpio, void *args)
{
  char pwr_state[MAX_VALUE_LEN] = {0};

  pal_get_last_pwr_state(FRU_SCM, pwr_state);

  // Check if the new GPIO val is ASSERT
  if (gpio->status == gpio->ass_val) {
    if (strcmp(pwr_state, "off")) {
      pal_set_last_pwr_state(FRU_SCM, "off");
    }

    OBMC_CRIT("FRU: %d, System powered OFF", IPMB_BUS);
    pal_light_scm_led(SCM_LED_AMBER);
  } else {
    // Inform BIOS that BMC is ready
    bic_set_gpio(IPMB_BUS, BMC_READY_N, 0);

    if (strcmp(pwr_state, "on")) {
      pal_set_last_pwr_state(FRU_SCM, "on");
    }

    OBMC_CRIT("FRU: %d, System powered ON", IPMB_BUS);
    pal_light_scm_led(SCM_LED_BLUE);
  }

  return 0;
}

/*
 * List of GPIOs to be monitored in this daemon.
 */
static struct {
  unsigned int monitored_gpios;
  gpio_pin_t gpio_table[BIC_GPIO_MAX];
} bic_gpio_config = {
  .monitored_gpios = 1,
  .gpio_table = {
    [PWRGOOD_CPU] = {
      .flag = 1,
      .ass_val = GETBIT(gpio_ass_val, PWRGOOD_CPU),
      .handler = bic_cpu_pwr_handler,
    },
  },
};

/* Monitor the gpio pins */
static int
gpio_monitor_poll(void) {
  int i, ret;
  uint32_t n_pin_val, o_pin_val;
  bic_gpio_t curr_gpio;

  // Inform BIOS that BMC is ready
  ret = bic_set_gpio(IPMB_BUS, BMC_READY_N, 0);
  if (ret) {
    OBMC_WARN("bic_set_gpio failed for fru %u", IPMB_BUS);
  }
  ret = bic_get_gpio(IPMB_BUS, &curr_gpio);
  if (ret) {
    OBMC_WARN("bic_get_gpio failed for fru %u", IPMB_BUS);
    return -1;
  }

  pal_light_scm_led(SCM_LED_AMBER);
  o_pin_val = 0;

  while (1) {
    int handled = 0;
    uint32_t revised_pins;

    /* Get the GPIO pins */
    ret = bic_get_gpio(IPMB_BUS, (bic_gpio_t *)&n_pin_val);
    if (ret < 0) {
      OBMC_WARN("failed to get bic gpio (ret=%d): retrying..", ret);
      sleep(DELAY_BICMOND_READ);
      continue;
    }

    if (o_pin_val == n_pin_val) {
      GPIOD_VERBOSE("pin_val not changed. Sleeping for %u seconds",
                    DELAY_BICMOND_READ);
      sleep(DELAY_BICMOND_READ);
      continue;
    }

    revised_pins = (n_pin_val ^ o_pin_val);
    for (i = 0;
         i < BIC_GPIO_MAX && handled < bic_gpio_config.monitored_gpios;
         i++) {
      gpio_pin_t *gpio = &bic_gpio_config.gpio_table[i];

      if (GETBIT(revised_pins, i) && (gpio->flag == 1)) {
        gpio->status = GETBIT(n_pin_val, i);
        gpio->handler(gpio, NULL);
        handled++;
      }
    }

    o_pin_val = n_pin_val;
    sleep(DELAY_BICMOND_READ);
  } /* while loop */

  return 0; /* never reached */
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
    {"-v|--verbose", "enable verbose logging"},
    {"-D|--daemon", "run the process in daemon mode"},
    {NULL, NULL},
  };

  printf("Usage: %s [options]\n", prog_name);
  for (i = 0; options[i].opt != NULL; i++) {
    printf("    %-18s - %s\n", options[i].opt, options[i].desc);
  }
}

int
main(int argc, char **argv) {
  int ret;
  bool daemon_mode = false;
  struct option long_opts[] = {
    {"help",       no_argument, NULL, 'h'},
    {"verbose",    no_argument, NULL, 'v'},
    {"daemon",     no_argument, NULL, 'D'},
    {NULL,         0,           NULL, 0},
  };

  while (1) {
    int opt_index = 0;
    int ret = getopt_long(argc, argv, "hvD", long_opts, &opt_index);
    if (ret == -1)
      break; /* end of arguments */

    switch (ret) {
    case 'h':
      dump_usage(argv[0]);
      return 0;

    case 'v':
      verbose_logging = true;
      break;

    case 'D':
      daemon_mode = true;
      break;

    default:
      return -1;
    }
  } /* while */

  /*
   * Make sure only 1 instance of bicmond is running.
   */
  if (single_instance_lock(BICMOND_NAME) < 0) {
    if (errno == EWOULDBLOCK) {
      syslog(LOG_ERR, "Another %s instance is running. Exiting..\n",
             BICMOND_NAME);
    } else {
      syslog(LOG_ERR, "unable to ensure single %s instance: %s\n",
             BICMOND_NAME, strerror(errno));
    }

    return -1;
  }

  /*
   * Initialize logging facility.
   */
  if (verbose_logging) {
    ret = obmc_log_init(BICMOND_NAME, LOG_DEBUG, 0);
  } else {
    ret = obmc_log_init(BICMOND_NAME, LOG_INFO, 0);
  }
  if (ret != 0) {
    fprintf(stderr, "%s: failed to initialize logger: %s\n",
            BICMOND_NAME, strerror(errno));
    return -1;
  }

  /*
   * Enter daemon mode if required.
   */
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  obmc_log_unset_std_stream();
  if (daemon_mode) {
    if (daemon(0, 1) != 0) {
      OBMC_ERROR(errno, "failed to enter daemon mode");
      return -1;
    }
  }

  OBMC_INFO("%s service started", BICMOND_NAME);
  gpio_monitor_poll(); /* main loop */

  return 0;
}
