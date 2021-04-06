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
#include <openbmc/obmc-i2c.h>
#include <openbmc/libgpio.h>
#include <facebook/asic.h>

#define MAIN_CPLD_BUS  (4)
#define MAIN_CPLD_ADDR (0x84)

#define CPLD_EVENT_REG   (0xFE)
#define THERMTRIP_CTRL   (0x1 << 2)
#define RT_PWR_FAIL_CTRL (0x1 << 1)
#define BT_PWR_FAIL_CTRL (0x1 << 0)

#define LTC4282_REG_FAULT_LOG    0x04
#define FET_BAD_FAULT            (0x1 << 6)
#define FET_SHORT_FAULT          (0x1 << 5)
#define ON_FAULT                 (0x1 << 4)
#define POWER_BAD_FAULT          (0x1 << 3)
#define OC_FAULT                 (0x1 << 2)
#define UV_FAULT                 (0x1 << 1)
#define OV_FAULT                 (0x1 << 0)

#define ADM127x_REG_STATUS_IOUT  0x7B
#define IOUT_OC_FAULT            (0x1 << 7)
#define IOUT_OC_WARN             (0x1 << 5)

#define ADM127x_REG_STATUS_INPUT 0x7C
#define VIN_UV_WARN              (0x1 << 5)
#define VIN_UV_FAULT             (0x1 << 4)

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
  ERR_ASIC_THERMTRIP,
  ERR_PAX_0_ALERT,
  ERR_PAX_1_ALERT,
  ERR_PAX_2_ALERT,
  ERR_PAX_3_ALERT,
  ERR_CPLD_ALERT,
  ERR_UNKNOWN
};

enum {
  RUNTIME = 0,
  BOOTUP
};

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
    0x14, // ERR_ASIC_THERMTRIP
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

int check_power_seq(int type)
{
  struct power_seq {
    char *name;
    uint8_t offset;
    uint8_t bit;
  };
  struct power_seq *cpld_power_seq;
  struct power_seq runtime_seq[] = {
    {"RT_MODULE_PWRGD_ASIC3_R", 0, 0}, {"RT_MODULE_PWRGD_ASIC4_R", 0, 1},
    {"RT_MODULE_PWRGD_ASIC5_R", 0, 2}, {"RT_MODULE_PWRGD_ASIC6_R", 0, 3},
    {"RT_MODULE_PWRGD_ASIC7_R", 0, 4}, {"RT_P12V_HSC_PG_R", 1, 0},
    {"RT_VICOR_PG_R", 1, 1}, {"RT_P48V_HSC_PG_R", 1, 2},
    {"RT_PW_PAX_POWER_GOOD", 1, 3}, {"RT_ASIC_ALL_DONE", 1, 4},
    {"RT_MODULE_PWRGD_ASIC0", 1, 5}, {"RT_MODULE_PWRGD_ASIC1_R", 1, 6},
    {"RT_MODULE_PWRGD_ASIC2_R", 1, 7}
  };
  struct power_seq boot_seq[] = {
    {"BT_MODULE_PWRGD_ASIC0", 0, 0}, {"BT_MODULE_PWRGD_ASIC1", 0, 1},
    {"BT_MODULE_PWRGD_ASIC2", 0, 2}, {"BT_MODULE_PWRGD_ASIC3", 0, 3},
    {"BT_MODULE_PWRGD_ASIC4", 0, 4}, {"BT_MODULE_PWRGD_ASIC5", 0, 5},
    {"BT_MODULE_PWRGD_ASIC6", 0, 6}, {"BT_MODULE_PWRGD_ASIC7", 0, 7},
    {"BT_RST_CPLD_RSMRST_N", 1, 0}, {"BT_PWRGD_P5V_STBY_R", 1, 1},
    {"BT_PWRGD_CPLD_VREFF", 1, 2}, {"BT_PWRGD_P2V5_AUX_R", 1, 3},
    {"BT_PWRGD_P1V2_AUX_R", 1, 4}, {"BT_PWRGD_P1V15_AUX_R", 1, 5},
    {"BT_P12V_HSC_PG_R_SYN2", 1, 6}, {"BT_VICOR_PG_R", 1, 7},
    {"BT_P48V_HSC_PG_R_SYN2", 2, 0}, {"BT_PWRGD_P3V3_CPLD_R", 2, 1},
    {"BT_HOST_PWRGD_ASIC", 2, 2}, {"BT_PW_PAX_POWER_GOOD", 2, 3},
    {"BT_ASIC_ALL_DONE", 2, 4}
  };
  char dev_cpld[16] = {0};
  char event_str[64] = {0};
  int fd, i;
  int ret = 0, fail_addr = -1;
  uint8_t tbuf[8], rbuf[8], value;
  uint8_t event_reg, state_reg, ctrl_offset;
  uint8_t power_seq_num, read_bytes;

  if (type == RUNTIME) {
    cpld_power_seq = runtime_seq;
    event_reg = 0x2b;
    state_reg = 0x2b;
    ctrl_offset = RT_PWR_FAIL_CTRL;
    read_bytes = 2;
    power_seq_num = 13;
  } else {
    cpld_power_seq = boot_seq;
    event_reg = 0x2f;
    state_reg = 0x2d;
    ctrl_offset = BT_PWR_FAIL_CTRL;
    read_bytes = 3;
    power_seq_num = 21;
  }

  sprintf(dev_cpld, "/dev/i2c-%d", MAIN_CPLD_BUS);
  fd = open(dev_cpld, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  tbuf[0] = event_reg;
  ret = i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 1, rbuf, 1);
  if (ret < 0 || (rbuf[0] & 0x80) == 0x0) // Read error or power is turned off normally
    goto exit;

  tbuf[0] = state_reg;
  ret = i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 1, rbuf, read_bytes);
  if (ret < 0)
    goto exit;

  for (i = 0; i < power_seq_num; i++) {
    value = rbuf[cpld_power_seq[i].offset] & (1 << cpld_power_seq[i].bit);
    if (value == 0) {
      fail_addr = i;
      snprintf(event_str, sizeof(event_str), "%s power rail fails", cpld_power_seq[i].name);
      syslog(LOG_CRIT, "%s", event_str);
      pal_add_cri_sel(event_str);
    }
  }
  if (fail_addr < 0) {
      snprintf(event_str, sizeof(event_str), "Unknown power rail fails");
      syslog(LOG_CRIT, "Unknown power rail fails");
      pal_add_cri_sel(event_str);
  }
  if (type == BOOTUP) {
    tbuf[0] = 0x31; // CPLD state
    ret = i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 1, rbuf, 1);
    if (ret < 0)
      goto exit;
    syslog(LOG_CRIT, "CPLD bootup failed at state %d", rbuf[0]);
  }

  // Clear event
  tbuf[0] = CPLD_EVENT_REG;
  tbuf[1] = ctrl_offset;
  i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 2, rbuf, 0);
exit:
  close(fd);
  return ret;
}

int check_pwr_brake()
{
  const char* cpld_power_brake[] = {
    "PWRBRK_ASIC0", "PWRBRK_ASIC1",
    "PWRBRK_ASIC2", "PWRBRK_ASIC3",
    "PWRBRK_ASIC4", "PWRBRK_ASIC5",
    "PWRBRK_ASIC6", "PWRBRK_ASIC7"
  };
  char dev_cpld[16] = {0};
  char event_str[64] = {0};
  int fd, i;
  int ret = 0, fail_addr = -1;
  uint8_t tbuf[8], rbuf[8], value;

  // Check if power is on
  if (pal_is_server_off())
    return 0;

  sprintf(dev_cpld, "/dev/i2c-%d", MAIN_CPLD_BUS);
  fd = open(dev_cpld, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  tbuf[0] = 0x11; // Power Brake state
  ret = i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 1, rbuf, 1);
  if (ret < 0)
    goto exit;

  for (i = 0; i < 8; i++) {
    if (!is_asic_prsnt(i))
      continue;

    value = rbuf[0] & (0x1 << i);
    if (value == 0) {
      fail_addr = i;
      snprintf(event_str, sizeof(event_str), "%s power brake", cpld_power_brake[i]);
      syslog(LOG_CRIT, "%s", event_str);
    }
  }
  if (fail_addr < 0) {
      snprintf(event_str, sizeof(event_str), "Unknown GPU power brake");
      syslog(LOG_CRIT, "Unknown GPU power brake");
  }

exit:
  close(fd);
  return ret;
}

int check_hsc_alert(gpiopoll_pin_t *gp, int bus_id, uint8_t addr, uint8_t reg)
{
  char dev[16] = {0};
  int fd, ret;
  uint8_t tbuf[8], rbuf[8];
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);

  sprintf(dev, "/dev/i2c-%d", bus_id);
  fd = open(dev, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  tbuf[0] = reg;
  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 1);
  close(fd);
  if (ret < 0)
    return -1;

  switch (reg)
  {
    case LTC4282_REG_FAULT_LOG:
      if (rbuf[0] & FET_BAD_FAULT)
	syslog(LOG_CRIT, "%s: FET-BAD", cfg->description);
      if (rbuf[0] & FET_SHORT_FAULT)
	syslog(LOG_CRIT, "%s: FET-short", cfg->description);
      if (rbuf[0] & ON_FAULT)
	syslog(LOG_CRIT, "%s: ON pin changing", cfg->description);
      if (rbuf[0] & POWER_BAD_FAULT)
	syslog(LOG_CRIT, "%s: Power-bad", cfg->description);
      if (rbuf[0] & OC_FAULT)
	syslog(LOG_CRIT, "%s: Overcurrent", cfg->description);
      if (rbuf[0] & UV_FAULT)
	syslog(LOG_CRIT, "%s: Undervoltage", cfg->description);
      if (rbuf[0] & OV_FAULT)
	syslog(LOG_CRIT, "%s: Overvoltage", cfg->description);
      break;
    case ADM127x_REG_STATUS_IOUT:
      if (rbuf[0] & IOUT_OC_FAULT)
	syslog(LOG_CRIT, "%s: Overcurrent fault", cfg->description);
      if (rbuf[0] & IOUT_OC_WARN)
	syslog(LOG_CRIT, "%s: Overcurrent warning", cfg->description);
      break;
    case ADM127x_REG_STATUS_INPUT:
      if (rbuf[0] & VIN_UV_FAULT)
	syslog(LOG_CRIT, "%s: Undervoltage fault", cfg->description);
      if (rbuf[0] & VIN_UV_WARN)
	syslog(LOG_CRIT, "%s: Undervoltage warning", cfg->description);
      break;
    default:
      break;
  }
  return 0;
}

static bool is_gpu_thermal_trip()
{
  bool thermtrip[8] = {false, false, false, false, false, false, false, false};
  bool event = false;
  int fd, ret;
  char shadow[16] = {0};
  char dev_cpld[16] = {0};
  uint8_t tbuf[8], rbuf[8];

  for (uint8_t i = 0; i < 8; i++) {
    if (!is_asic_prsnt(i))
      continue;

    snprintf(shadow, sizeof(shadow), "OAM%d_THERMTRIP", (int)i);
    if (gpio_get(shadow) == GPIO_VALUE_LOW)
      thermtrip[i] = true;
  }

  sprintf(dev_cpld, "/dev/i2c-%d", MAIN_CPLD_BUS);
  fd = open(dev_cpld, O_RDWR);
  if (fd < 0) {
    return false;
  }

  tbuf[0] = 0x0a; // Themral trip event
  ret = i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 1, rbuf, 1);
  if (ret < 0)
    goto exit;

  event = rbuf[0] & 0x80? true: false;
  // Delay to avoid false alart during AC cycle due to hardware issue
  msleep(250);
  if (event) {
    for (int i = 0; i < 8; i++) {
      if (thermtrip[i])
        syslog(LOG_CRIT, "ASIC%d Thermal Trip\n", i);
    }
  }

  // Clear event
  tbuf[0] = CPLD_EVENT_REG;
  tbuf[1] = THERMTRIP_CTRL;
  i2c_rdwr_msg_transfer(fd, MAIN_CPLD_ADDR, tbuf, 2, rbuf, 0);
exit:
  close(fd);
  return event;
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
      curr == GPIO_VALUE_LOW && check_pwr_brake() < 0) {
      syslog(LOG_WARNING, "Failed to get power brake state from CPLD");
  }

  gpio_event_handle_low_active(gp, last, curr);
}

static void gpio_event_handle_pwr_good(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  msleep(250);
  if (curr == GPIO_VALUE_LOW && check_power_seq(RUNTIME) < 0)
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
  if (curr == GPIO_VALUE_LOW) {
    if (check_hsc_alert(gp, 16, 0x53, LTC4282_REG_FAULT_LOG) < 0)
      syslog(LOG_WARNING, "Failed to get alert status from P12V HSC_1");
    if (!pal_is_server_off() &&
	(check_hsc_alert(gp, 16, 0x13, ADM127x_REG_STATUS_IOUT) < 0 ||
        check_hsc_alert(gp, 16, 0x13, ADM127x_REG_STATUS_INPUT) < 0)) {
      syslog(LOG_WARNING, "Failed to get alert status from P48V HSC_1");
    }
  }
}

static void gpio_event_hsc_2_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  gpio_event_handle_pwr_brake(gp, last, curr);
  sync_dbg_led(ERR_HSC_2_ALERT, curr == GPIO_VALUE_LOW? true: false);
  if (curr == GPIO_VALUE_LOW) {
    if (check_hsc_alert(gp, 17, 0x40, LTC4282_REG_FAULT_LOG) < 0)
      syslog(LOG_WARNING, "Failed to get alert status from P12V HSC_2");
    if (!pal_is_server_off() &&
	(check_hsc_alert(gp, 17, 0x10, ADM127x_REG_STATUS_IOUT) < 0 ||
        check_hsc_alert(gp, 17, 0x10, ADM127x_REG_STATUS_INPUT) < 0)) {
      syslog(LOG_WARNING, "Failed to get alert status from P48V HSC_2");
    }
  }
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
  if (pal_is_server_off())
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_01_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic23_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_23_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic45_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_45_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_asic67_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_ASIC_67_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_0_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_0_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_1_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_1_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_2_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_2_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_pax_3_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (pal_is_server_off())
    return;
  if (pal_is_fw_update_ongoing(FRU_MB))
    return;

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_PAX_3_ALERT, curr == GPIO_VALUE_LOW? true: false);
}

static void gpio_event_cpld_alert(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  // Delay to avoid false alart during AC cycle due to hardware issue
  msleep(250);
  if (curr == GPIO_VALUE_LOW && check_power_seq(BOOTUP) < 0)
    syslog(LOG_WARNING, "Failed to get power state from CPLD");

  gpio_event_handle_low_active(gp, last, curr);
  sync_dbg_led(ERR_CPLD_ALERT, curr == GPIO_VALUE_LOW? true: false);
}


static void cpld_def_alert(gpiopoll_pin_t *gp, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_LOW) {
    if (check_power_seq(BOOTUP) < 0)
      syslog(LOG_WARNING, "Failed to get power state from CPLD");

    log_gpio_change(gp, curr, 0, false);
    sync_dbg_led(ERR_CPLD_ALERT, true);
  }
}

static void gpio_event_asic_thermtrip(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  if (!pal_is_server_off() && is_gpu_thermal_trip()) {
    gpio_event_handle_low_active(gp, last, curr);
    sync_dbg_led(ERR_ASIC_THERMTRIP, curr == GPIO_VALUE_LOW? true: false);
  }
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
  {"THERMTRIP_N_ASIC07", "ASIC Thermal Trip", GPIO_EDGE_BOTH, gpio_event_asic_thermtrip, NULL},
  {"CPLD_SMB_ALERT_N", "CPLD Alert", GPIO_EDGE_BOTH, gpio_event_cpld_alert, cpld_def_alert},
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

      if (pal_is_server_off())
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

      if (pal_is_server_off() || !is_asic_prsnt(i))
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
