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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/file.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>

#define POLL_TIMEOUT -1 /* Forever */

bool g_sys_pwr_off;

static void log_gpio_change(gpiopoll_pin_t *gp, gpio_value_t value, useconds_t log_delay, bool low_active)
{
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  if (low_active)
    syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
  else
    syslog(LOG_CRIT, "%s: %s - %s\n", value ? "ASSERT": "DEASSERT", cfg->description, cfg->shadow);
}

static gpio_value_t gpio_get(const char *shadow)
{
  gpio_value_t value = GPIO_VALUE_INVALID;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    syslog(LOG_CRIT, "Open failed for GPIO: %s\n", shadow);
    return GPIO_VALUE_INVALID;
  }
  if (gpio_get_value(desc, &value)) {
    syslog(LOG_CRIT, "Get failed for GPIO: %s\n", shadow);
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);
  return value;
}

// Generic Event Handler for GPIO changes
static void gpio_event_handle_low_active(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(gp, curr, 0, true);
}

static void gpio_event_handle_high_active(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(gp, curr, 0, false);
}

// Specific Event Handlers
static void gpio_event_handle_power_btn(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(gp, curr, 0, true);
}

static void gpio_event_handle_pwr_good(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  g_sys_pwr_off = (curr == GPIO_VALUE_HIGH)? false: true;
  if (curr == GPIO_VALUE_HIGH) {
    /* Enable ADM1272 Vout sampling (it is disabled by default) */
    if (system("i2cset -f -y 16 0x13 0xd4 0x3f37 w > /dev/null") < 0 ||
        system("i2cset -f -y 17 0x10 0xd4 0x3f37 w > /dev/null") < 0) {
      syslog(LOG_CRIT, "Failed to enable P48V VOUT monitoring");
    }
  }
}

static void asic_def_prsnt(gpiopoll_pin_t *gp, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH)
    log_gpio_change(gp, curr, 0, false);
}

// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"BMC_PWR_BTN_IN_N", "Power button", GPIO_EDGE_BOTH, gpio_event_handle_power_btn, NULL},
  {"SYS_PWR_READY", "System power off", GPIO_EDGE_BOTH, gpio_event_handle_pwr_good, NULL},
  {"PMBUS_BMC_1_ALERT_N", "HSC 1 Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"PMBUS_BMC_2_ALERT_N", "HSC 2 Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"PMBUS_BMC_3_ALERT_N", "HSC AUX Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"SMB_ALERT_ASIC01", "ASIC01 Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"SMB_ALERT_ASIC23", "ASIC23 Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"SMB_ALERT_ASIC45", "ASIC45 Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"SMB_ALERT_ASIC67", "ASIC67 Alert", GPIO_EDGE_BOTH, gpio_event_handle_low_active, NULL},
  {"PRSNT0_N_ASIC0", "ASIC0 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC0", "ASIC0 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC1", "ASIC1 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC1", "ASIC1 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC2", "ASIC2 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC2", "ASIC2 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC3", "ASIC3 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC3", "ASIC3 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC4", "ASIC4 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC4", "ASIC4 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC5", "ASIC5 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC5", "ASIC5 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC6", "ASIC6 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC6", "ASIC6 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT0_N_ASIC7", "ASIC7 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
  {"PRSNT1_N_ASIC7", "ASIC7 present off", GPIO_EDGE_RISING, gpio_event_handle_high_active, asic_def_prsnt},
};

// For monitoring GPIOs on IO expender
struct gpioexppoll_config {
  char shadow[64];
  char description[64];
  gpio_value_t last;
  gpio_value_t curr;
};

static void* fan_status_monitor()
{
  int i;
  struct gpioexppoll_config fan_gpios[] = {
    {"FAN0_PRESENT", "Fan0 present", GPIO_VALUE_LOW, GPIO_VALUE_INVALID},
    {"FAN1_PRESENT", "Fan1 present", GPIO_VALUE_LOW, GPIO_VALUE_INVALID},
    {"FAN2_PRESENT", "Fan2 present", GPIO_VALUE_LOW, GPIO_VALUE_INVALID},
    {"FAN3_PRESENT", "Fan3 present", GPIO_VALUE_LOW, GPIO_VALUE_INVALID},
    {"FAN0_PWR_GOOD", "Fan0 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"FAN1_PWR_GOOD", "Fan1 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"FAN2_PWR_GOOD", "Fan2 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"FAN3_PWR_GOOD", "Fan3 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
  };
  struct gpioexppoll_config *gp;

  while (1) {
    sleep(1);

    if (g_sys_pwr_off)
      continue;

    for (i = 0; i < 8; i++) {
      gp = &fan_gpios[i];
      if (i>>2)
        gp->curr = gpio_get(gp->shadow);
      else
        gp->curr = pal_is_fan_prsnt(i<<1)? GPIO_VALUE_LOW: GPIO_VALUE_HIGH;

      if (gp->last != gp->curr) {
        if (i>>2) {
          syslog(LOG_CRIT, "%s: %s - %s\n",
              gp->curr == GPIO_VALUE_HIGH? "ON": "OFF",
              gp->description,
              gp->shadow);
        } else {
          syslog(LOG_CRIT, "%s: %s - %s\n",
              gp->curr == GPIO_VALUE_LOW? "ON": "OFF",
              gp->description,
              gp->shadow);
        }
        gp->last = gp->curr;
      }
    }
  }

  return NULL;
}

static void* asic_status_monitor()
{
  int i;
  struct gpioexppoll_config asic_gpios[] = {
    {"OAM0_MODULE_PWRGD", "ASIC0 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM1_MODULE_PWRGD", "ASIC1 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM2_MODULE_PWRGD", "ASIC2 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM3_MODULE_PWRGD", "ASIC3 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM4_MODULE_PWRGD", "ASIC4 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM5_MODULE_PWRGD", "ASIC5 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM6_MODULE_PWRGD", "ASIC6 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
    {"OAM7_MODULE_PWRGD", "ASIC7 power", GPIO_VALUE_HIGH, GPIO_VALUE_INVALID},
  };
  struct gpioexppoll_config *gp;

  while (1) {
    sleep(1);

    if (g_sys_pwr_off)
      continue;

    for (i = 0; i < 8; i++) {
      if (!pal_is_asic_prsnt(i))
        continue;

      gp = &asic_gpios[i];
      gp->curr = gpio_get(gp->shadow);

      if (gp->last != gp->curr) {
        syslog(LOG_CRIT, "%s: %s - %s\n",
            gp->curr == GPIO_VALUE_HIGH? "ON": "OFF",
            gp->description,
            gp->shadow);
      }
      gp->last = gp->curr;
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  pthread_t tid_fan_monitor;
  pthread_t tid_asic_monitor;
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");

    g_sys_pwr_off = pal_is_server_off();
    if (pthread_create(&tid_fan_monitor, NULL, fan_status_monitor, NULL) < 0) {
      syslog(LOG_CRIT, "pthread_create for fan monitor error");
      exit(1);
    }

    if (pthread_create(&tid_asic_monitor, NULL, asic_status_monitor, NULL) < 0) {
      syslog(LOG_CRIT, "pthread_create for asic monitor error");
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
