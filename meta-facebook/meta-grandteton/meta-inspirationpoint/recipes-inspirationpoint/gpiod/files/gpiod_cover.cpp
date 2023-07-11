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

#define AUTODUMP_BIN  "/usr/local/bin/autodump.sh"
#define AUTODUMP_LOCK "/var/run/autodump.lock"

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
  char cri_sel[128] = {0};

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

  
  strcat(cri_sel, cfg->shadow);
  if (curr) {
    syslog(LOG_CRIT, "FRU: %d ASSERT: %s - %s power status:%02X%02X%02X\n", FRU_MB, cfg->description, cfg->shadow, rbuf[0], rbuf[1], rbuf[2]);
    strcat(cri_sel, " ASSERT");
  } else {
    syslog(LOG_CRIT, "FRU: %d DEASSERT: %s - %s\n", FRU_MB, cfg->description, cfg->shadow);
    strcat(cri_sel, " DEASSERT");
  }
  pal_add_cri_sel(cri_sel);
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
  if (system("sh /etc/init.d/rebind-rt-mux.sh &") != 0) {
    syslog(LOG_CRIT, "Failed to run: %s", __FUNCTION__);
  }
}

static void
rst_perst_init_handler(gpiopoll_pin_t *desc, gpio_value_t curr) {
  if (curr) {
    rst_perst_event_handler(desc, curr, curr);
  }
}

static void
post_comp_init_handler(gpiopoll_pin_t *desc, gpio_value_t curr) {
  if (curr == GPIO_VALUE_LOW) {
    pal_get_cpu_id(0);
    pal_get_cpu_id(1);
  }
}

static void
post_comp_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  sgpio_event_handler(desc, last, curr);
  post_comp_init_handler(desc, curr);
}

static void
apml_alert_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  uint8_t soc_num, ras_status;

  soc_num = cfg->shadow[8] - '0';  // "APML_CPUx_ALERT_R_N"
  if (pal_check_apml_ras_status(soc_num, &ras_status)) {
    int fd = open(AUTODUMP_LOCK, O_CREAT | O_RDWR, 0666);
    if (flock(fd, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
      close(fd);
      return;
    }

    if (system(AUTODUMP_BIN)) {
      syslog(LOG_WARNING, "%s[%u] Failed to launch crashdump", __FUNCTION__, soc_num);
    }
    flock(fd, LOCK_UN);
    close(fd);
  }
  sleep(1);
}

static void
apml_alert_init_handler(gpiopoll_pin_t *desc, gpio_value_t curr) {
  if (curr == GPIO_VALUE_LOW) {
    apml_alert_event_handler(desc, curr, curr);
  }
}

static void*
delay_prochot_logging(void *arg) {
  pthread_detach(pthread_self());

  if (arg == NULL) {
    syslog(LOG_CRIT, "%s null parameter\n", __func__);
    pthread_exit(NULL);
  }

  struct delayed_prochot_log *log = (struct delayed_prochot_log*)arg;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(log->desc);
  char cmd[128] = {0};

  if (!sgpio_valid_check()) {
    goto end;
  }
  assert(cfg);

  /* prevent false alarm when prochot pin fallen 2~3 ms earlier
   * than CPU power good pin during host 12V off 
   */
  msleep(DELAY_LOG_CPU_PROCHOT_MS);

  if(!server_power_check(3)) {
    goto end;
  }

  //LCD debug card critical SEL support
  strcat(cmd, "CPU FPH");
  if (log->curr) {
    syslog(LOG_CRIT, "Record time delay %dms, FRU: %d %s: %s - %s\n",
      DELAY_LOG_CPU_PROCHOT_MS, FRU_MB, log->curr ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    strcat(cmd, " DEASSERT");
  } else {
    char reason[32] = "";
    strcat(cmd, " by ");
    prochot_reason(reason);
    strcat(cmd, reason);
    syslog(LOG_CRIT, "Record time delay %dms, FRU: %d ASSERT: %s - %s (reason: %s)\n",
           DELAY_LOG_CPU_PROCHOT_MS, FRU_MB, cfg->description, cfg->shadow, reason);
    strcat(cmd, " ASSERT");
  }
  pal_add_cri_sel(cmd);

end:
  if (log) {
    free(log);
  }

  pthread_exit(NULL);
}

static void
gta_cpu_prochot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  pthread_t tid_delay_prochot_log;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  struct delayed_prochot_log *log = (struct delayed_prochot_log *)malloc(sizeof(struct delayed_prochot_log));

  assert(cfg);

  if(!log) {
    syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, curr ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    return;
  }

  log->desc = desc;
  log->last = last;
  log->curr = curr;

  if (pthread_create(&tid_delay_prochot_log, NULL, delay_prochot_logging, (void *)log)) {
    syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, curr ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    free(log);
  }

  return;
}

static void
cpu_prochot_handler_wrapper(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (pal_is_artemis()) {
    gta_cpu_prochot_handler(desc, last, curr);
  } else {
    cpu_prochot_handler(desc, last, curr);
  }
}

// GPIO table to be monitored
static struct gpiopoll_config gpios_plat_list[] = {
  // shadow, description, edge, handler, oneshot
  {IRQ_UV_DETECT_N,             "SGPIO188",  GPIO_EDGE_BOTH,    uv_detect_handler,          NULL},
  {IRQ_OC_DETECT_N,             "SGPIO178",  GPIO_EDGE_BOTH,    oc_detect_handler,          NULL},
  {IRQ_HSC_ALERT_N,             "SGPIO2",    GPIO_EDGE_BOTH,    sml1_pmbus_alert_handler,   NULL},
  {FM_CPU0_PROCHOT_N,           "SGPIO202",  GPIO_EDGE_BOTH,    cpu_prochot_handler_wrapper,NULL},
  {FM_CPU1_PROCHOT_N,           "SGPIO186",  GPIO_EDGE_BOTH,    cpu_prochot_handler_wrapper,NULL},
  {FM_CPU0_THERMTRIP_N,         "SGPIO136",  GPIO_EDGE_BOTH,    cpu_thermtrip_handler,      NULL},
  {FM_CPU1_THERMTRIP_N,         "SGPIO118",  GPIO_EDGE_BOTH,    cpu_thermtrip_handler,      NULL},
  {FM_CPU_ERR0_N,               "SGPIO142",  GPIO_EDGE_BOTH,    cpu_error_handler,          NULL},
  {FM_CPU_ERR1_N,               "SGPIO144",  GPIO_EDGE_BOTH,    cpu_error_handler,          NULL},
  {FM_SYS_THROTTLE,             "SGPIO20" ,  GPIO_EDGE_BOTH,    gpio_event_pson_handler,    NULL},
  {RST_PLTRST_N,                "SGPIO200",  GPIO_EDGE_BOTH,    platform_reset_handle,      platform_reset_init},
  {FM_LAST_PWRGD,               "SGPIO240",  GPIO_EDGE_BOTH,    pwr_good_handler,           pwr_good_init},
  {FM_CPU0_SKTOCC,              "SGPIO112",  GPIO_EDGE_BOTH,    sgpio_event_handler,        cpu_skt_init},
  {FM_CPU1_SKTOCC,              "SGPIO114",  GPIO_EDGE_BOTH,    sgpio_event_handler,        cpu_skt_init},
  {FM_CPU0_PWR_FAIL,            "SGPIO174",  GPIO_EDGE_BOTH,    pwr_err_event_handler,      NULL},
  {FM_CPU1_PWR_FAIL,            "SGPIO176",  GPIO_EDGE_BOTH,    pwr_err_event_handler,      NULL},
  {FM_UV_ADR_TRIGGER,           "SGPIO26",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {RST_PERST_N,                 "SGPIO230",  GPIO_EDGE_RISING,  rst_perst_event_handler,    rst_perst_init_handler},
  {FM_CPU0_PRSNT,               "CPU0",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_CPU1_PRSNT,               "CPU1",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_OCP0_PRSNT,               "NIC0",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_OCP1_PRSNT,               "NIC1",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_E1S0_PRSNT,               "E1.S",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_PVDD11_S3_P0_OCP,         "SGPIO14",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {FM_PVDD11_S3_P1_OCP,         "SGPIO16",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {FM_BIOS_POST_CMPLT,          "SGPIO146",  GPIO_EDGE_BOTH,    post_comp_event_handler,    post_comp_init_handler},
  {APML_CPU0_ALERT,             "SGPIO10",   GPIO_EDGE_FALLING, apml_alert_event_handler,   apml_alert_init_handler},
  {APML_CPU1_ALERT,             "SGPIO12",   GPIO_EDGE_FALLING, apml_alert_event_handler,   apml_alert_init_handler},
  {HMC_READY,                   "SGPIO64",   GPIO_EDGE_BOTH,    hmc_ready_handler,          hmc_ready_init},
  // Add new GPIO from here
  // The reserved space will be used for optional GPIO pins in specific system config
  {RESERVED_GPIO,            RESERVED_GPIO,  GPIO_EDGE_NONE,    NULL,                       NULL},
};

// GPIO table to be monitored
static struct gpiopoll_config gta_gpios_plat_list[] = {
  // shadow, description, edge, handler, oneshot
  {IRQ_UV_DETECT_N,             "SGPIO188",  GPIO_EDGE_BOTH,    uv_detect_handler,          NULL},
  {IRQ_OC_DETECT_N,             "SGPIO178",  GPIO_EDGE_BOTH,    oc_detect_handler,          NULL},
  {IRQ_HSC_ALERT_N,             "SGPIO2",    GPIO_EDGE_BOTH,    sml1_pmbus_alert_handler,   NULL},
  {FM_CPU0_PROCHOT_N,           "SGPIO202",  GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU1_PROCHOT_N,           "SGPIO186",  GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU0_THERMTRIP_N,         "SGPIO136",  GPIO_EDGE_BOTH,    cpu_thermtrip_handler,      NULL},
  {FM_CPU1_THERMTRIP_N,         "SGPIO118",  GPIO_EDGE_BOTH,    cpu_thermtrip_handler,      NULL},
  {FM_CPU_ERR0_N,               "SGPIO142",  GPIO_EDGE_BOTH,    cpu_error_handler,          NULL},
  {FM_CPU_ERR1_N,               "SGPIO144",  GPIO_EDGE_BOTH,    cpu_error_handler,          NULL},
  {FM_SYS_THROTTLE,             "SGPIO20" ,  GPIO_EDGE_BOTH,    gpio_event_pson_handler,    NULL},
  {RST_PLTRST_N,                "SGPIO200",  GPIO_EDGE_BOTH,    platform_reset_handle,      platform_reset_init},
  {FM_LAST_PWRGD,               "SGPIO240",  GPIO_EDGE_BOTH,    pwr_good_handler,           pwr_good_init},
  {FM_CPU0_SKTOCC,              "SGPIO112",  GPIO_EDGE_BOTH,    sgpio_event_handler,        cpu_skt_init},
  {FM_CPU1_SKTOCC,              "SGPIO114",  GPIO_EDGE_BOTH,    sgpio_event_handler,        cpu_skt_init},
  {FM_CPU0_PWR_FAIL,            "SGPIO174",  GPIO_EDGE_BOTH,    pwr_err_event_handler,      NULL},
  {FM_CPU1_PWR_FAIL,            "SGPIO176",  GPIO_EDGE_BOTH,    pwr_err_event_handler,      NULL},
  {FM_UV_ADR_TRIGGER,           "SGPIO26",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {RST_PERST_N,                 "SGPIO230",  GPIO_EDGE_RISING,  rst_perst_event_handler,    rst_perst_init_handler},
  {FM_CPU0_PRSNT,               "CPU0",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_CPU1_PRSNT,               "CPU1",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_OCP0_PRSNT,               "NIC0",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_OCP1_PRSNT,               "NIC1",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_E1S0_PRSNT,               "E1.S",      GPIO_EDGE_BOTH,    device_prsnt_event_handler, NULL},
  {FM_PVDD11_S3_P0_OCP,         "SGPIO14",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {FM_PVDD11_S3_P1_OCP,         "SGPIO16",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {FM_BIOS_POST_CMPLT,          "SGPIO146",  GPIO_EDGE_BOTH,    post_comp_event_handler,    post_comp_init_handler},
  {APML_CPU0_ALERT,             "SGPIO10",   GPIO_EDGE_FALLING, apml_alert_event_handler,   apml_alert_init_handler},
  {APML_CPU1_ALERT,             "SGPIO12",   GPIO_EDGE_FALLING, apml_alert_event_handler,   apml_alert_init_handler},
  // Add new GPIO from here
  // The reserved space will be used for optional GPIO pins in specific system config
  {RESERVED_GPIO,            RESERVED_GPIO,  GPIO_EDGE_NONE,    NULL,                       NULL},
};

// This GPIO only monitored when HSC is not LTC4282
static struct gpiopoll_config gpios_hsc_list[] = {
  {IRQ_HSC_FAULT_N,             "SGPIO36",   GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
};

int get_gpios_plat_list(struct gpiopoll_config** list) {
  uint8_t source_id;
  uint8_t cnt = pal_is_artemis() ? ARRAY_SIZE(gta_gpios_plat_list)-1 : ARRAY_SIZE(gpios_plat_list)-1;
  *list = pal_is_artemis() ? gta_gpios_plat_list : gpios_plat_list;

  // LTC4282 doesn't have this GPIO
  if (get_comp_source(FRU_MB, MB_HSC_SOURCE, &source_id) == 0 && source_id != SECOND_SOURCE) {
    memcpy(*list + cnt, gpios_hsc_list, sizeof(gpios_hsc_list));
    cnt += ARRAY_SIZE(gpios_hsc_list);
  }

  return cnt;
}


pthread_t tid_gpio_timer;
pthread_t tid_iox_gpio_handle;

int gpiod_plat_thread_create(void) {

  //Create thread for platform reset event filter check
  if (pthread_create(&tid_gpio_timer, NULL, gpio_timer, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
    return -1;
  }

  //Create thread for platform reset event filter check
  if (pthread_create(&tid_iox_gpio_handle, NULL, iox_gpio_handle, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for iox_gpio_handler\n");
    return -1;
  }

  return 0;
}

void gpiod_plat_thread_finish(void) {
  pthread_join(tid_gpio_timer, NULL);
  pthread_join(tid_iox_gpio_handle, NULL);
}
