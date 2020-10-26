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
#include <facebook/asic.h>

#define POLL_TIMEOUT -1 /* Forever */

enum {
  ERR_HSC_1_ALERT = 0,
  ERR_HSC_2_ALERT,
  ERR_HSC_AUX_ALERT,
  ERR_HSC_1_THROT,
  ERR_HSC_2_THROT,
  ERR_ASIC_01_ALERT,
  ERR_ASIC_23_ALERT,
  ERR_ASIC_45_ALERT,
  ERR_ASIC_67_ALERT,
  ERR_PAX_0_ALERT,
  ERR_PAX_1_ALERT,
  ERR_PAX_2_ALERT,
  ERR_PAX_3_ALERT,
  ERR_CPLD_ALERT,
  ERR_UNKNOWN
};

bool g_sys_pwr_off;
pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static int set_dbg_led(uint8_t num)
{
  const char *shadows[] = {
    "LED_POSTCODE_0", "LED_POSTCODE_1", "LED_POSTCODE_2", "LED_POSTCODE_3",
    "LED_POSTCODE_4", "LED_POSTCODE_5", "LED_POSTCODE_6", "LED_POSTCODE_7"
  };
  gpio_desc_t *gpio;

  for (int i = 0; i < 8; i++) {
    gpio = gpio_open_by_shadow(shadows[i]);
    if (!gpio) {
      syslog(LOG_WARNING, "%s: Open GPIO %s failed", __func__, shadows[i]);
      return -1;
    }
    if (gpio_set_value(gpio, (num & (0x1 << i))? GPIO_VALUE_HIGH: GPIO_VALUE_LOW) < 0) {
      syslog(LOG_WARNING, "%s: Set GPIO %s failed", __func__, shadows[i]);
      gpio_close(gpio);
      return -1;
    }
    gpio_close(gpio);
  }
  return 0;
}

static int sync_dbg_led(uint8_t err_bit, bool assert)
{
  static int err = 0;
  static uint8_t err_code[] = {
    0x01, // ERR_HSC_1_ALERT
    0x02, // ERR_HSC_2_ALERT
    0x03, // ERR_HSC_AUX_ALERT
    0x04, // ERR_HSC_1_THROT
    0x05, // ERR_HSC_2_THROT
    0x10, // ERR_ASIC_01_ALERT
    0x11, // ERR_ASIC_23_ALERT
    0x12, // ERR_ASIC_45_ALERT
    0x13, // ERR_ASIC_67_ALERT
    0x20, // ERR_PAX_0_ALERT
    0x21, // ERR_PAX_1_ALERT
    0x22, // ERR_PAX_2_ALERT
    0x23, // ERR_PAX_3_ALERT
    0x30, // ERR_CPLD_ALERT
  };

  pthread_mutex_lock(&led_mutex);
  if (assert)
    err |= (0x1 << err_bit);
  else
    err &= ~(0x1 << err_bit);
  pthread_mutex_unlock(&led_mutex);

  for (int i = 0; i < ERR_UNKNOWN; i++) {
    if (err & (0x1 << i))
      return set_dbg_led(err_code[i]);
  }
  return set_dbg_led(0x00);
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

static void gpio_event_handle_pwr_brake(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (gpio_get("OAM_FAST_BRK_ON_N") == GPIO_VALUE_LOW &&
      curr == GPIO_VALUE_LOW && pal_check_pwr_brake() < 0) {
      syslog(LOG_WARNING, "Failed to get power brake state from CPLD");
  }

  gpio_event_handle_low_active(gp, last, curr);
}

static void gpio_event_handle_pwr_good(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  g_sys_pwr_off = (curr == GPIO_VALUE_HIGH)? false: true;

  if (curr == GPIO_VALUE_LOW && pal_check_power_seq() < 0)
    syslog(LOG_WARNING, "Failed to get power state from CPLD");

  gpio_event_handle_low_active(gp, last, curr);
}

// Specific Event Handlers
static void gpio_event_handle_power_btn(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(gp, curr, 0, true);
}

static void asic_def_prsnt(gpiopoll_pin_t *gp, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH)
    log_gpio_change(gp, curr, 0, false);
}

static void gpio_event_hsc_1_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_pwr_brake(gp, last, curr);
  sync_dbg_led(ERR_HSC_1_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_hsc_2_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_pwr_brake(gp, last, curr);
  sync_dbg_led(ERR_HSC_2_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_hsc_aux_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_pwr_brake(gp, last, curr);
  sync_dbg_led(ERR_HSC_AUX_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_hsc_1_throt(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_pwr_brake(gp, last, curr);
  sync_dbg_led(ERR_HSC_1_THROT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_hsc_2_throt(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_pwr_brake(gp, last, curr);
  sync_dbg_led(ERR_HSC_2_THROT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic01_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_01_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic23_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_23_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic45_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_45_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic67_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_67_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_0_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_0_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_1_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_1_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_2_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_2_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_3_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (g_sys_pwr_off)
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_3_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_cpld_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_CPLD_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"BMC_PWR_BTN_IN_N", "Power button", GPIO_EDGE_BOTH, gpio_event_handle_power_btn, NULL},
  {"SYS_PWR_READY", "System power off", GPIO_EDGE_BOTH, gpio_event_handle_pwr_good, NULL},
  {"PMBUS_BMC_1_ALERT_N", "HSC 1 Alert", GPIO_EDGE_BOTH, gpio_event_hsc_1_alert, NULL},
  {"PMBUS_BMC_2_ALERT_N", "HSC 2 Alert", GPIO_EDGE_BOTH, gpio_event_hsc_2_alert, NULL},
  {"PMBUS_BMC_3_ALERT_N", "HSC AUX Alert", GPIO_EDGE_BOTH, gpio_event_hsc_aux_alert, NULL},
  {"HSC1_THROT_N", "HSC 1 Throttle", GPIO_EDGE_BOTH, gpio_event_hsc_1_throt, NULL},
  {"HSC2_THROT_N", "HSC 2 Throttle", GPIO_EDGE_BOTH, gpio_event_hsc_2_throt, NULL},
  {"SMB_ALERT_ASIC01", "ASIC01 Alert", GPIO_EDGE_BOTH, gpio_event_asic01_alert, NULL},
  {"SMB_ALERT_ASIC23", "ASIC23 Alert", GPIO_EDGE_BOTH, gpio_event_asic23_alert, NULL},
  {"SMB_ALERT_ASIC45", "ASIC45 Alert", GPIO_EDGE_BOTH, gpio_event_asic45_alert, NULL},
  {"SMB_ALERT_ASIC67", "ASIC67 Alert", GPIO_EDGE_BOTH, gpio_event_asic67_alert, NULL},
  {"CPLD_SMB_ALERT_N", "CPLD Alert", GPIO_EDGE_BOTH, gpio_event_cpld_alert, NULL},
  {"PAX0_ALERT", "PAX0 Alert", GPIO_EDGE_BOTH, gpio_event_pax_0_alert, NULL},
  {"PAX1_ALERT", "PAX1 Alert", GPIO_EDGE_BOTH, gpio_event_pax_1_alert, NULL},
  {"PAX2_ALERT", "PAX2 Alert", GPIO_EDGE_BOTH, gpio_event_pax_2_alert, NULL},
  {"PAX3_ALERT", "PAX3 Alert", GPIO_EDGE_BOTH, gpio_event_pax_3_alert, NULL},
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

    for (i = 0; i < 8; i++) {

      if (g_sys_pwr_off)
	continue;

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

    for (i = 0; i < 8; i++) {

      if (g_sys_pwr_off || !is_asic_prsnt(i))
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
