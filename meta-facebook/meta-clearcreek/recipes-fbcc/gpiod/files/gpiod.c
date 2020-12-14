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
#include <openbmc/obmc-i2c.h>

#define POLL_TIMEOUT -1 /* Forever */

static void
log_gpio_change(gpiopoll_pin_t *desc, gpio_value_t value, useconds_t log_delay) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  assert(cfg);
  syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
}

static void
read_throttle_err_cnt(gpiopoll_pin_t *desc, gpio_value_t value, useconds_t log_delay) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  char fn[32] = "/dev/i2c-23";
  int fd, ret;
  uint8_t tlen, rlen, cnt[4];
  uint8_t addr = 0x2A;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t cmds[4] = {0x10, 0x11, 0x12, 0x13};

  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "can not open i2c device\n");
    return;
  }

  tlen = 1;
  rlen = 1;

  for (int i = 0; i < sizeof(cmds); i++) {
    tbuf[0] = cmds[i];
    if ((ret =  i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen))) {
      syslog(LOG_ERR, "i2c transfer fail\n");
      break;
    }
    cnt[i] = rbuf[0];
  }

  syslog(LOG_CRIT, "%s - %s PMBUS_HSC_Cnt:%02x HSC_OC_Cnt:%02x HSC_UV_Cnt:%02x HSC_TIMER_Cnt:%02x\n", cfg->description, cfg->shadow, cnt[0], cnt[1], cnt[2], cnt[3]);

  if (fd > 0) {
    close(fd);
  }
}

static void
read_throttle_err_seq(gpiopoll_pin_t *desc, gpio_value_t value, useconds_t log_delay) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  char fn[32] = "/dev/i2c-23";
  int fd, ret;
  uint8_t tlen, rlen, seq[4];
  uint8_t addr = 0x2A;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t cmds[4] = {0x30, 0x31, 0x32, 0x33};

  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "can not open i2c device\n");
    return;
  }

  tlen = 1;
  rlen = 1;

  for (int i = 0; i < sizeof(cmds); i++) {
    tbuf[0] = cmds[i];
    if ((ret =  i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen))) {
      syslog(LOG_ERR, "i2c transfer fail\n");
      break;
    }
    seq[i] = rbuf[0];
  }
  syslog(LOG_CRIT, "%s - %s PMBUS_HSC_Seq:%02x HSC_OC_Seq:%02x HSC_UV_Seq:%02x HSC_TIMER_Seq:%02x\n", cfg->description, cfg->shadow, seq[0], seq[1], seq[2], seq[3]);

  if (fd > 0) {
    close(fd);
  }
}

static void gpio_event_log_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(desc, curr, 0);
}

static void gpio_throttle_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(desc, curr, 0);
  read_throttle_err_cnt(desc, curr, 0);
  read_throttle_err_seq(desc, curr, 0);
}

// Specific Event Handlers
static void gpio_event_handle_power_state(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  int ret = system("sh /etc/init.d/setup-m2carrier.sh");
  if(ret != 0)
    syslog(LOG_ERR, "setup m2carrier fail\n");
}

static void gpio_event_handle_power_btn(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(desc, curr, 0);
}

// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"CPLD_BMC_GPIO_R_01", "POWER STSTE MON", GPIO_EDGE_RISING, gpio_event_handle_power_state, NULL},
  {"FM_SYS_THROTTLE_R", "GPIOP7", GPIO_EDGE_BOTH, gpio_event_log_handler, NULL},
  {"HSC_TIMER_R_N", "GPIOP2", GPIO_EDGE_FALLING, gpio_throttle_handler, NULL},
  {"HSC_OC_R_N", "GPIOP3", GPIO_EDGE_FALLING, gpio_throttle_handler, NULL},
  {"HSC_UV_R_N", "GPIOP4", GPIO_EDGE_FALLING, gpio_throttle_handler, NULL},
  {"PMBUS_HSC_ALERT_R_N", "GPIOP5", GPIO_EDGE_FALLING, gpio_throttle_handler, NULL},
  {"BMC_PWR_BTN_IN_N", "Power button", GPIO_EDGE_BOTH, gpio_event_handle_power_btn, NULL},
  {"CARRIER_0_ALERT_R_N", "CARRIER_0", GPIO_EDGE_BOTH, gpio_event_log_handler, NULL},
  {"CARRIER_1_ALERT_R_N", "CARRIER_1", GPIO_EDGE_BOTH, gpio_event_log_handler, NULL},
  {"SYS_PWR_READY", "SYS_PWR_READY", GPIO_EDGE_BOTH, gpio_event_log_handler, NULL},
};

int main(int argc, char **argv)
{
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
