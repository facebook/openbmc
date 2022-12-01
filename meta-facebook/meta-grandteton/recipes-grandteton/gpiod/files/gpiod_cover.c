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
#include <stdbool.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>
#include <openbmc/pal.h>
#include <openbmc/peci_sensors.h>
#include <openbmc/pal_def.h>
#include "gpiod.h"


static uint8_t g_caterr_irq = 0;
static bool g_mcerr_ierr_assert = false;
static pthread_mutex_t caterr_mutex = PTHREAD_MUTEX_INITIALIZER;


//IERR and MCERR Event Handler
void
cpu_caterr_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  uint8_t status = 0;

  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    g_caterr_irq++;
  }
}

static void
err_caterr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if(!server_power_check(1))
    return;

  pthread_mutex_lock(&caterr_mutex);
  g_caterr_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static int
ierr_mcerr_event_log(bool is_caterr, const char *err_type) {
  char temp_log[128] = {0};
  char temp_syslog[128] = {0};
  char cpu_str[32] = "";
  int num=0;
  int ret=0;

  ret = cmd_peci_get_cpu_err_num(PECI_CPU0_ADDR, &num, is_caterr);
  if (ret != 0) {
    syslog(LOG_ERR, "Can't Read MCA Log\n");
  }

  if (num == 2)
    strcpy(cpu_str, "0/1");
  else if (num != -1)
    sprintf(cpu_str, "%d", num);

  sprintf(temp_syslog, "ASSERT: CPU%s %s\n", cpu_str, err_type);
  sprintf(temp_log, "CPU%s %s", cpu_str, err_type);
  syslog(LOG_CRIT, "FRU: %d %s",FRU_MB, temp_syslog);
  pal_add_cri_sel(temp_log);

  return ret;
}

static void *
ierr_mcerr_event_handler() {
  uint8_t caterr_cnt = 0;
  gpio_value_t value;

  while (1) {
    if (g_caterr_irq > 0) {
      caterr_cnt++;
      if (caterr_cnt == 2) {
        if (g_caterr_irq == 1) {
          g_mcerr_ierr_assert = true;

          value = gpio_get_value_by_shadow(FM_CPU_CATERR_N);
          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(true, "IERR/CATERR");
          } else {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
          }

          pthread_mutex_lock(&caterr_mutex);
          g_caterr_irq--;
          pthread_mutex_unlock(&caterr_mutex);
          caterr_cnt = 0;
          if (system("/usr/local/bin/autodump.sh &")) {
            syslog(LOG_WARNING, "Failed to start crashdump\n");
          }

        } else if (g_caterr_irq > 1) {
          while (g_caterr_irq > 1) {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
            pthread_mutex_lock(&caterr_mutex);
            g_caterr_irq--;
            pthread_mutex_unlock(&caterr_mutex);
          }
          caterr_cnt = 1;
        }
      }
    }
    usleep(25000); //25ms
  }
  return NULL;
}

// GPIO table to be monitored
struct gpiopoll_config gpios_list[] = {
  // shadow, description, edge, handler, oneshot
  {FP_RST_BTN_IN_N,         "GPIOP2",   GPIO_EDGE_BOTH,    pwr_reset_handler,          NULL},
  {FP_PWR_BTN_IN_N,         "GPIOP0",   GPIO_EDGE_BOTH,    pwr_button_handler,         NULL},
  {FM_UARTSW_LSB_N,         "SGPIO106", GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_UARTSW_MSB_N,         "SGPIO108", GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_POST_CARD_PRES_N,     "GPIOZ6",   GPIO_EDGE_BOTH,    usb_dbg_card_handler,       NULL},
  {IRQ_UV_DETECT_N,         "SGPIO188", GPIO_EDGE_BOTH,    uv_detect_handler,          NULL},
  {IRQ_OC_DETECT_N,         "SGPIO178", GPIO_EDGE_BOTH,    oc_detect_handler,          NULL},
  {IRQ_HSC_FAULT_N,         "SGPIO36",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {IRQ_HSC_ALERT_N,         "SGPIO2",   GPIO_EDGE_BOTH,    sml1_pmbus_alert_handler,   NULL},
  {FM_CPU_CATERR_N,         "GPIOC5",   GPIO_EDGE_FALLING, err_caterr_handler,         cpu_caterr_init},
  {FM_CPU0_PROCHOT_N,       "SGPIO202", GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU1_PROCHOT_N,       "SGPIO186", GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU0_THERMTRIP_N,     "SGPIO136", GPIO_EDGE_FALLING, cpu_thermtrip_handler,      NULL},
  {FM_CPU1_THERMTRIP_N,     "SGPIO118", GPIO_EDGE_FALLING, cpu_thermtrip_handler,      NULL},
  {FM_CPU_ERR0_N,           "SGPIO144", GPIO_EDGE_FALLING, cpu_error_handler,          NULL},
  {FM_CPU_ERR1_N,           "SGPIO142", GPIO_EDGE_FALLING, cpu_error_handler,          NULL},
  {FM_CPU_ERR2_N,           "SGPIO140", GPIO_EDGE_FALLING, cpu_error_handler,          NULL},
  {FM_CPU0_MEM_HOT_N,       "SGPIO138", GPIO_EDGE_BOTH,    gpio_event_pson_3s_handler, NULL},
  {FM_CPU1_MEM_HOT_N,       "SGPIO124", GPIO_EDGE_BOTH,    gpio_event_pson_3s_handler, NULL},
  {FM_CPU0_MEM_THERMTRIP_N, "SGPIO122", GPIO_EDGE_FALLING, mem_thermtrip_handler,      NULL},
  {FM_CPU1_MEM_THERMTRIP_N, "SGPIO158", GPIO_EDGE_FALLING, mem_thermtrip_handler,      NULL},
  {FM_SYS_THROTTLE,         "SGPIO198", GPIO_EDGE_BOTH,    gpio_event_pson_handler,    NULL},
  {FM_PCH_THERMTRIP_N,      "SGPIO28",  GPIO_EDGE_BOTH,    pch_thermtrip_handler,      NULL},
  {FM_CPU0_VCCIN_VR_HOT,    "SGPIO154", GPIO_EDGE_BOTH,    cpu0_pvccin_handler,        cpu_vr_hot_init},
  {FM_CPU1_VCCIN_VR_HOT,    "SGPIO120", GPIO_EDGE_BOTH,    cpu1_pvccin_handler,        cpu_vr_hot_init},
  {FM_CPU0_VCCD_VR_HOT,     "SGPIO12",  GPIO_EDGE_BOTH,    cpu0_pvccd_handler,         cpu_vr_hot_init},
  {FM_CPU1_VCCD_VR_HOT,     "SGPIO18",  GPIO_EDGE_BOTH,    cpu1_pvccd_handler,         cpu_vr_hot_init},
  {RST_PLTRST_N,            "SGPIO200", GPIO_EDGE_BOTH,    platform_reset_handle,      platform_reset_init},
  {FM_LAST_PWRGD,           "SGPIO116", GPIO_EDGE_BOTH,    pwr_good_handler,           NULL},
  {FM_CPU0_SKTOCC,          "SGPIO112", GPIO_EDGE_BOTH,    gpio_event_handler,         cpu_skt_init},
  {FM_CPU1_SKTOCC,          "SGPIO114", GPIO_EDGE_BOTH,    gpio_event_handler,         cpu_skt_init},
  {FP_AC_PWR_BMC_BTN,       "GPIO18A0", GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
};

const uint8_t gpios_cnt = sizeof(gpios_list)/sizeof(gpios_list[0]);



int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
  pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;
  pthread_t tid_iox_gpio_handle;

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

    //Create thread for IERR/MCERR event detect
    if (pthread_create(&tid_ierr_mcerr_event, NULL, ierr_mcerr_event_handler, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for ierr_mcerr_event_handler\n");
      exit(1);
    }

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_gpio_timer, NULL, gpio_timer, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
      exit(1);
    }

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_iox_gpio_handle, NULL, iox_gpio_handle, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for iox_gpio_handler\n");
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

  return 0;
}
