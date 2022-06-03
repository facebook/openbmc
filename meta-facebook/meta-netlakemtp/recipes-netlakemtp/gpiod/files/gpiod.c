/*
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
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>

#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"
#define POLL_TIMEOUT        -1 /* Forever */

// Thread for gpio timer
static void *
server_power_monitor() {
  uint8_t status = 0;
  uint8_t fru = 1;
  char str[MAX_VALUE_LEN] = {0};
  int tread_time = 0 ;

  // Wait power-on.sh finished, then update last power state
  sleep(20);

  while (1) {
    pal_get_server_power(fru, &status);
    if (pal_get_last_pwr_state(fru, str) < 0) {
      str[0] = '\0';
    }
    if (status == SERVER_POWER_ON) {
      if (strncmp(str, POWER_ON_STR, strlen(POWER_ON_STR)) != 0) {
        pal_set_last_pwr_state(fru, POWER_ON_STR);
        syslog(LOG_INFO, "last pwr state updated to on\n");
      }
    } else {
      if (strncmp(str, POWER_OFF_STR, strlen(POWER_OFF_STR)) != 0) {
        pal_set_last_pwr_state(fru, POWER_OFF_STR);
        syslog(LOG_INFO, "last pwr state updated to off\n");
      }
    }
    sleep(1);
  }

  return NULL;
}

static void
cpu_thermal_trip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d CPU Thermal Trip Warning %s\n", 
         FRU_SERVER, (curr == GPIO_VALUE_LOW) ? "Assertion" : "Deassertion");
}

static void
cpu_fail_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d CPU Fail Warning %s\n", 
         FRU_SERVER, (curr == GPIO_VALUE_LOW) ? "Assertion" : "Deassertion");
}

static void
dimm_hot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d DIMM Hot Warning %s\n",
         FRU_SERVER, (curr == GPIO_VALUE_LOW) ? "Assertion" : "Deassertion");
}

static void
vr_hot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d VR Hot Warning %s\n",
         FRU_SERVER, (curr == GPIO_VALUE_LOW) ? "Assertion" : "Deassertion");
}

static void
cpu_throttle_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d CPU Throttle Warning %s\n",
         FRU_SERVER, (curr == GPIO_VALUE_LOW) ? "Assertion" : "Deassertion");
}

static void
uart_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  gpio_desc_t *gpio;
  gpio_value_t val;

  gpio = gpio_open_by_shadow("UART_BMC_MUX_CTRL");
  if (gpio == NULL) {
     syslog(LOG_WARNING, "%s() Open GPIO UART_BMC_MUX_CTRL failed\n", __func__);
  }
  else {
    if (gpio_get_value(gpio, &val))  {
      syslog(LOG_WARNING, "%s() gpio_get_value_by_shadow failed\n", __func__);
      gpio_close(gpio);
      return;
    }
    if (gpio_set_value(gpio, !val)) {
      syslog(LOG_WARNING, "%s() gpio_set_value failed\n", __func__);
      gpio_close(gpio);
      return;
    }
  }
}

static void
power_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  gpio_desc_t *gpio;

  gpio = gpio_open_by_shadow("PWR_BTN_COME_R_N");
  if (gpio == NULL) {
     syslog(LOG_WARNING, "%s() Open GPIO PWR_BTN_COME_R_N failed\n", __func__);
  }
  else {
    if (gpio_set_value(gpio, curr) != 0) {
      syslog(LOG_WARNING, "%s() gpio_set_value failed\n", __func__);
      gpio_close(gpio);
      return;
    }
  }
}

static void
reset_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  gpio_desc_t *uart_mux_gpio, *reset_button_gpio;
  gpio_value_t val;

  uart_mux_gpio = gpio_open_by_shadow("UART_BMC_MUX_CTRL");
  if (uart_mux_gpio == NULL) {
    syslog(LOG_WARNING, "%s() Open GPIO UART_BMC_MUX_CTRL failed\n", __func__);
  } else {
    if (gpio_get_value(uart_mux_gpio, &val) != 0) {
      syslog(LOG_WARNING, "%s() gpio_get_value_by_shadow failed\n", __func__);
    } else {
      if (val == GPIO_LOW) {
        if (run_command("reboot") < 0) {
          syslog(LOG_ERR, "%s() Failed to do bmc power reset\n", __func__);
        }
      } else {
        reset_button_gpio = gpio_open_by_shadow("RST_BTN_COME_R_N");
        if (reset_button_gpio == NULL) {
           syslog(LOG_WARNING, "%s() Open GPIO RST_BTN_COME_R_N failed\n", __func__);
        } else {
          if (gpio_set_value(reset_button_gpio, curr) != 0) {
            syslog(LOG_WARNING, "%s() gpio_set_value failed\n", __func__);
          }
          gpio_close(reset_button_gpio);
        }
      }
    }
    gpio_close(uart_mux_gpio);
  }
}

// GPIO table
static struct
gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"FM_THERMTRIP_R_N",                "GPIOF3",   GPIO_EDGE_BOTH,     cpu_thermal_trip_handler,  NULL},
  {"UART_CH_SELECT",                  "GPIOG0",   GPIO_EDGE_FALLING,  uart_button_handler,       NULL},
  {"RST_BTN_N",                       "GPIOP2",   GPIO_EDGE_BOTH,     reset_button_handler,      NULL},
  {"PWR_BTN_N",                       "GPIOP4",   GPIO_EDGE_BOTH,     power_button_handler,      NULL},
  {"IRQ_PVCCIN_CPU_VRHOT_LVC3_R_N",   "GPIOH3",   GPIO_EDGE_BOTH,     vr_hot_handler,            NULL},
  {"FM_CPU_MSMI_CATERR_LVT3_R_N",     "GPIOM3",   GPIO_EDGE_BOTH,     cpu_fail_handler,          NULL},
  {"FM_CPU_PROCHOT_LATCH_LVT3_R_N",   "GPIOV3",   GPIO_EDGE_BOTH,     cpu_throttle_handler,      NULL},
  {"H_MEMHOT_OUT_FET_R_N",            "GPIOY2",   GPIO_EDGE_BOTH,     dimm_hot_handler,          NULL},
};

int
main(int argc, char **argv) {
  int rc, pid_file, ret;
  pthread_t tid_server_power_monitor;
  void *retval;
  gpiopoll_desc_t *polldesc;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {

    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");

    //Create thread for server power monitor check
    if (pthread_create(&tid_server_power_monitor, NULL, server_power_monitor, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
    }
  }

  polldesc = gpio_poll_open(g_gpios, sizeof(g_gpios)/sizeof(g_gpios[0]));
  if (!polldesc) {
    syslog(LOG_CRIT, "FRU: %d Cannot start poll operation on GPIOs", FRU_SERVER);
  } else {
    if (gpio_poll(polldesc, POLL_TIMEOUT)) {
      syslog(LOG_CRIT, "FRU: %d Poll returned error", FRU_SERVER);
    }
    gpio_poll_close(polldesc);
  }

  return 0;
}
