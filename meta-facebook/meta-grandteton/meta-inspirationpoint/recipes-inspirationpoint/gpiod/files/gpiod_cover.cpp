/*
 * sensord
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
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
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>
#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal_common.h>
#include <openbmc/pal_def.h>
#include <openbmc/aries_api.h>
#include "gpiod.h"

static void
pwr_err_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  char fn[32] = "/dev/i2c-7";
  int fd, ret;
  uint8_t tlen, rlen;
  uint8_t addr = 0x46;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t cmds[1] = {0x27};

  if (!sgpio_valid_check())
    return;
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_ERR, "can not open i2c device\n");
    return;
  }

  tlen = 1;
  rlen = 3;

  tbuf[0] = cmds[0];
  if ((ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen))) {
    syslog(LOG_ERR, "i2c transfer fail\n");
  }

  if (fd > 0) {
    close(fd);
  }

  if (curr) {
    syslog(LOG_CRIT, "FRU: %d ASSERT: %s - %s power status:%02X%02X%02X\n", FRU_MB, cfg->description, cfg->shadow, rbuf[0], rbuf[1], rbuf[2]);
  } else {
    syslog(LOG_CRIT, "FRU: %d DEASSERT: %s - %s\n", FRU_MB, cfg->description, cfg->shadow);
  }
}

static void
device_prsnt_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (curr) {
    syslog(LOG_CRIT, "FRU: %d %s LOST- %s\n", FRU_MB, cfg->description, cfg->shadow);
  } else {
    syslog(LOG_CRIT, "FRU: %d %s INSERT- %s\n", FRU_MB, cfg->description, cfg->shadow);
  }
}

static void
rst_perst_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  char rev_id[20] = {0};

  if (system("sh /etc/init.d/rebind-rt-mux.sh &") != 0) {
    syslog(LOG_CRIT, "Failed to run: %s", __FUNCTION__);
  }

  kv_get("mb_rev", rev_id, 0, 0);
  if (!strcmp(rev_id, "2")) {
    AriesInit(I2C_BUS_60, 0x24);
    AriesInit(I2C_BUS_64, 0x24);
  } else if (!strcmp(rev_id, "1")) {
    for (int bus = I2C_BUS_60; bus <= I2C_BUS_67; bus++) {
      AriesInit(bus, 0x24);
    }
  }
}


// GPIO table to be monitored
static struct gpiopoll_config gpios_list[] = {
  // shadow, description, edge, handler, oneshot
  {FP_RST_BTN_IN_N,             "GPIOP2",    GPIO_EDGE_BOTH,    pwr_reset_handler,          NULL},
  {FP_PWR_BTN_IN_N,             "GPIOP0",    GPIO_EDGE_BOTH,    pwr_button_handler,         NULL},
  {FM_UARTSW_LSB_N,             "SGPIO106",  GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_UARTSW_MSB_N,             "SGPIO108",  GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_POST_CARD_PRES_N,         "GPIOZ6",    GPIO_EDGE_BOTH,    usb_dbg_card_handler,       NULL},
  {IRQ_UV_DETECT_N,             "SGPIO188",  GPIO_EDGE_BOTH,    uv_detect_handler,          NULL},
  {IRQ_OC_DETECT_N,             "SGPIO178",  GPIO_EDGE_BOTH,    oc_detect_handler,          NULL},
  {IRQ_HSC_FAULT_N,             "SGPIO36",   GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {IRQ_HSC_ALERT_N,             "SGPIO2",    GPIO_EDGE_BOTH,    sml1_pmbus_alert_handler,   NULL},
  {FM_CPU0_PROCHOT_N,           "SGPIO202",  GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU1_PROCHOT_N,           "SGPIO186",  GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU0_THERMTRIP_N,         "SGPIO136",  GPIO_EDGE_BOTH,    cpu_thermtrip_handler,      NULL},
  {FM_CPU1_THERMTRIP_N,         "SGPIO118",  GPIO_EDGE_BOTH,    cpu_thermtrip_handler,      NULL},
  {FM_CPU_ERR0_N,               "SGPIO142",  GPIO_EDGE_BOTH,    cpu_error_handler,          NULL},
  {FM_CPU_ERR1_N,               "SGPIO144",  GPIO_EDGE_BOTH,    cpu_error_handler,          NULL},
  {FM_SYS_THROTTLE,             "SGPIO198",  GPIO_EDGE_BOTH,    gpio_event_pson_handler,    NULL},
  {RST_PLTRST_N,                "SGPIO200",  GPIO_EDGE_BOTH,    platform_reset_handle,      platform_reset_init},
  {FM_LAST_PWRGD,               "SGPIO116",  GPIO_EDGE_BOTH,    pwr_good_handler,           NULL},
  {FM_CPU0_SKTOCC,              "SGPIO112",  GPIO_EDGE_BOTH,    gpio_event_handler,         cpu_skt_init},
  {FM_CPU1_SKTOCC,              "SGPIO114",  GPIO_EDGE_BOTH,    gpio_event_handler,         cpu_skt_init},
  {FM_CPU0_PWR_FAIL,            "SGPIO174",  GPIO_EDGE_BOTH,    pwr_err_event_handler,      NULL},
  {FM_CPU1_PWR_FAIL,            "SGPIO176",  GPIO_EDGE_BOTH,    pwr_err_event_handler,      NULL},
  {FM_UV_ADR_TRIGGER,           "SGPIO26",   GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU0_P0_PMALERT,   "SGPIO154",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU0_P1_PMALERT,   "SGPIO156",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU1_P0_SMB_ALERT, "SGPIO158",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU1_P1_SMB_ALERT, "SGPIO160",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {RST_PERST_N,                 "SGPIO230",  GPIO_EDGE_RISING,  rst_perst_event_handler,    NULL},
  {FM_CPU0_PRSNT,               "CPU0",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_CPU1_PRSNT,               "CPU1",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_OCP0_PRSNT,               "NIC0",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_OCP1_PRSNT,               "NIC1",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_E1S0_PRSNT,               "E1.S",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_PVDD11_S3_P0_OCP,         "SGPIO14",   GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDD11_S3_P1_OCP,         "SGPIO16",   GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU0_P0_OCP,       "SGPIO122",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU0_P1_OCP,       "SGPIO124",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU1_P0_OCP,       "SGPIO126",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {FM_PVDDCR_CPU1_P1_OCP,       "SGPIO128",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {PVDD11_S3_P0_PMALERT,        "SGPIO182",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {PVDD11_S3_P1_PMALERT,        "SGPIO184",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {HMC_READY,                   "HMC_READY", GPIO_EDGE_BOTH,    hmc_ready_handler,          hmc_ready_init},
  {GPU_FPGA_THERM_OVERT,        "SGPIO40",   GPIO_EDGE_FALLING, gpio_event_handler,         NULL},
  {GPU_FPGA_DEVIC_OVERT,        "SGPIO42",   GPIO_EDGE_FALLING, gpio_event_handler,         NULL},
};

const uint8_t gpios_cnt = sizeof(gpios_list)/sizeof(gpios_list[0]);



int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
//   pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;

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

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_gpio_timer, NULL, gpio_timer, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
      exit(1);
    }

    polldesc = gpio_poll_open(gpios_list, gpios_cnt);
    if (!polldesc) {
      syslog(LOG_CRIT, "FRU: %d Cannot start poll operation on GPIOs", FRU_MB);
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "FRU: %d Poll returned error", FRU_MB);
      }
      gpio_poll_close(polldesc);
    }
  }

  pthread_join(tid_gpio_timer, NULL);

  return 0;
}
