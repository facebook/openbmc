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
#include <openbmc/pal.h>
#include <openbmc/gpio.h>

#define POLL_TIMEOUT -1 /* Forever */
static uint8_t CATERR_irq = 0;
static uint8_t MSMI_irq = 0;
static bool log_enable = true;

// Return GPIO number from given string
// e.g. GPIOA0 -> 0, GPIOB6->14, GPIOAA2->210
static int
gpio_num(char *str)
{
  int base_2 = 0; // Starting GPIOA0
  int base_3 = 208; // Starting @ GPIOAA0
  int base = 0;
  int len = strlen(str);
  int ret = 0;

  if (len != 6 && len != 7) {
    printf("len is %d\n", len);
    return -1;
  }

  if (len == 6) {
    base = base_2;
  } else {
    base = base_3;
  }

  ret = (base + str[len-1] - '0' + ((str[len-2] - 'A') * 8));
  return ret;
}


// Generic Event Handler for GPIO changes
static void gpio_event_handle(void *p)
{
  gpio_poll_st *gp = (gpio_poll_st*) p;

  // Use GPIOB6 or GPIOR5 to disable logging
  // Either host is suddenly reset or powered off
  if (gp->gs.gs_gpio == gpio_num("GPIOB6")  || // Power OK
      gp->gs.gs_gpio == gpio_num("GPIOE0")  || // Reset Button
      gp->gs.gs_gpio == gpio_num("GPIOE2")  || // Power Button
      gp->gs.gs_gpio == gpio_num("GPIOR5")) { // Platform Reset
    if (gp->value == 0x0) {
      log_enable = false;
    }
    return;
  }
  syslog(LOG_CRIT, "%s: %s\n", (gp->value?"DEASSERT":"ASSERT"), gp->desc);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON
static void gpio_event_handle_power(void *p)
{
  uint8_t status = 0;
  gpio_poll_st *gp = (gpio_poll_st*) p;

  pal_get_server_power(1, &status);
  if (status != SERVER_POWER_ON) {
    return;
  }

  if (gp->gs.gs_gpio == gpio_num("GPIOG1")) {
    CATERR_irq++;
  } else if (gp->gs.gs_gpio == gpio_num("GPION3")) {
    MSMI_irq++;
  } else if (gp->gs.gs_gpio == gpio_num("GPIOAA7") && gp->value == 0x0) {
    log_enable = true;
  } else if (log_enable) {
      syslog(LOG_CRIT, "%s: %s\n", (gp->value?"DEASSERT":"ASSERT"), gp->desc);
  }
}

// GPIO table to be monitored when MB is OFF
static gpio_poll_st g_gpios_off[] = {
  // {{gpio, fd}, gpioValue, call-back function, GPIO description}
  {{0, 0}, 0, gpio_event_handle, "GPIOB6 - PWRGD_SYS_PWROK" },
  {{0, 0}, 0, gpio_event_handle, "GPIOD2 - FM_SOL_UART_CH_SEL"},
  {{0, 0}, 0, gpio_event_handle, "GPIOE0 - RST_SYSTEM_BTN_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOE2 - FM_PWR_BTN_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOE4 - FP_NMI_BTN_N"},
};

// GPIO table to be monitored when MB is ON
static gpio_poll_st g_gpios[] = {
  // {{gpio, fd}, gpioValue, call-back function, GPIO description}
  {{0, 0}, 0, gpio_event_handle_power, "GPIOB0 - PWRGD_PCH_PWROK" },
  {{0, 0}, 0, gpio_event_handle, "GPIOB6 - PWRGD_SYS_PWROK" },
  {{0, 0}, 0, gpio_event_handle_power, "GPIOB7 - IRQ_PVDDQ_GHJ_VRHOT_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOD2 - FM_SOL_UART_CH_SEL"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOD4 - IRQ_DIMM_SAVE_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOD6 - FM_CPU_ERR0_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOD7 - FM_CPU_ERR1_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOE0 - RST_SYSTEM_BTN_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOE2 - FM_PWR_BTN_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOE4 - FP_NMI_BTN_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOE6 - FM_CPU0_PROCHOT_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOE7 - FM_CPU1_PROCHOT_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOF0 - IRQ_PVDDQ_ABC_VRHOT_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOF2 - IRQ_PVCCIN_CPU0_VRHOT_LVC3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOF3 - IRQ_PVCCIN_CPU1_VRHOT_LVC3_N"},
  {{0, 0},0, gpio_event_handle_power, "GPIOF4 - IRQ_PVDDQ_KLM_VRHOT_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOG0 - FM_CPU_ERR2_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOG1 - FM_CPU_CATERR_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOG2 - FM_PCH_BMC_THERMTRIP_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOG3 - FM_CPU0_SKTOCC_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOI0 - FM_CPU0_FIVR_FAULT_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOI1 - FM_CPU1_FIVR_FAULT_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOL0 - IRQ_UV_DETECT_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOL1 - IRQ_OC_DETECT_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOL2 - FM_HSC_TIMER_EXP_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOL4 - FM_MEM_THERM_EVENT_PCH_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOM0 - FM_CPU0_RC_ERROR_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOM1 - FM_CPU1_RC_ERROR_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOM4 - FM_CPU0_THERMTRIP_LATCH_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOM5 - FM_CPU1_THERMTRIP_LATCH_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPION3 - FM_CPU_MSMI_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOQ6 - FM_POST_CARD_PRES_BMC_N"},
  {{0, 0}, 0, gpio_event_handle, "GPIOR5 - RST_BMC_PLTRST_BUF_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOS0 - FM_THROTTLE_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOX4 - H_CPU0_MEMABC_MEMHOT_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOX5 - H_CPU0_MEMDEF_MEMHOT_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOX6 - H_CPU1_MEMGHJ_MEMHOT_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOX7 - H_CPU1_MEMKLM_MEMHOT_LVT3_BMC_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOY0 - FM_SLPS3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOY1 - FM_SLPS4_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOZ2 - IRQ_PVDDQ_DEF_VRHOT_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOAA0 - FM_CPU1_SKTOCC_LVT3_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOAA1 - IRQ_SML1_PMBUS_ALERT_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOAA7 - FM_BIOS_POST_CMPLT_N"},
  {{0, 0}, 0, gpio_event_handle_power, "GPIOAB0 - IRQ_HSC_FAULT_N"},
};

static int g_count_off = sizeof(g_gpios_off) / sizeof(gpio_poll_st);
static int g_count = sizeof(g_gpios) / sizeof(gpio_poll_st);

// Initalize the gpio# using the helper function
static void
gpio_init_off(void) {
  int i = 0;
  // Initialize gpio numbers
  g_gpios_off[i++].gs.gs_gpio = gpio_num("GPIOB6");
  g_gpios_off[i++].gs.gs_gpio = gpio_num("GPIOD2");
  g_gpios_off[i++].gs.gs_gpio = gpio_num("GPIOE0");
  g_gpios_off[i++].gs.gs_gpio = gpio_num("GPIOE2");
  g_gpios_off[i++].gs.gs_gpio = gpio_num("GPIOE4");
}

// Initalize the gpio# using the helper function
static void
gpio_init(void) {
  int i = 0;
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOB0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOB6");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOB7");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOD2");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOD4");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOD6");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOD7");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOE0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOE2");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOE4");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOE6");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOE7");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOF0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOF2");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOF3");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOF4");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOG0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOG1");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOG2");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOG3");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOI0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOI1");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOL0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOL1");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOL2");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOL4");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOM0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOM1");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOM4");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOM5");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPION3");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOQ6");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOR5");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOS0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOX4");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOX5");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOX6");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOX7");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOY0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOY1");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOZ2");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOAA0");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOAA1");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOAA7");
  g_gpios[i++].gs.gs_gpio = gpio_num("GPIOAB0");
}

// Thread for IERR/MCERR event detect
static void *
ierr_mcerr_event_handler() {
  uint8_t CATERR_ierr_time_count = 0;
  uint8_t MSMI_ierr_time_count = 0;
  uint8_t status = 0;

  while (1) {
    if ( CATERR_irq > 0 ){
      CATERR_ierr_time_count++;
      if ( CATERR_ierr_time_count == 2 ){
        if ( CATERR_irq == 1 ){
          pal_get_CPU_CATERR(1, &status);
          if (status == 0) {
            syslog(LOG_CRIT, "ASSERT: IERR/CATERR\n");
          } else {
              syslog(LOG_CRIT, "ASSERT: MCERR/CATERR\n");
          }
            CATERR_irq--;
            CATERR_ierr_time_count = 0;
          } else if ( CATERR_irq > 1 ){
                   while (CATERR_irq > 1){
                        syslog(LOG_CRIT, "ASSERT: MCERR/CATERR\n");
                        CATERR_irq = CATERR_irq - 1;
                   }
                   CATERR_ierr_time_count = 1;
                 }
        }
    }

    if ( MSMI_irq > 0 ){
      MSMI_ierr_time_count++;
      if ( MSMI_ierr_time_count == 2 ){
        if ( MSMI_irq == 1 ){
          pal_get_CPU_MSMI(1, &status);
          if (status == 0) {
            syslog(LOG_CRIT, "ASSERT: IERR/MSMI\n");
          } else {
              syslog(LOG_CRIT, "ASSERT: MCERR/MSMI\n");
          }
          MSMI_irq--;
          MSMI_ierr_time_count = 0;
        } else if ( MSMI_irq > 1 ){
                 while (MSMI_irq > 1){
                      syslog(LOG_CRIT, "ASSERT: MCERR/MSMI\n");
                      MSMI_irq = MSMI_irq - 1;
                 }
                 MSMI_ierr_time_count = 1;
               }
       }
    }
    usleep(25000);
   }
}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  uint8_t status = 0;
  pthread_t tid_ierr_mcerr_event;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {

    daemon(0,1);
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");

    // Wait until the GPIO signal settle down
    sleep(150);

    // Check power status and start monitoring different set of GPIOs
    pal_get_server_power(1, &status);

    if (status == SERVER_POWER_ON) {
      //Create thread for IERR/MCERR event detect
      if (pthread_create(&tid_ierr_mcerr_event, NULL, ierr_mcerr_event_handler, NULL) < 0) {
         syslog(LOG_WARNING, "pthread_create for ierr_mcerr_event_handler\n");
         exit(1);
       }

      gpio_init();
      gpio_poll_open(g_gpios, g_count);
      gpio_poll(g_gpios, g_count, POLL_TIMEOUT);
      gpio_poll_close(g_gpios, g_count);
    } else {
      gpio_init_off();
      gpio_poll_open(g_gpios_off, g_count_off);
      gpio_poll(g_gpios_off, g_count_off, POLL_TIMEOUT);
      gpio_poll_close(g_gpios_off, g_count_off);
    }
  }

  return 0;
}
