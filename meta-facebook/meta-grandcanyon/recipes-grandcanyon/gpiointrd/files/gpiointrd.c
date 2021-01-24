/*
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/libgpio.h>
#include <openbmc/pal.h>
#include <facebook/fbgc_gpio.h>

#define POLL_TIMEOUT -1 /* Forever */

#define STUCK_BUTTON_TIME  10  // second
#define LONG_PRESS_BUTTON_TIME   4  // second
#define SHORT_PRESS_BUTTON_TIME  1  // second

static struct timespec press_rst_btn_time;
static struct timespec press_pwr_btn_time;
static bool is_rst_btn_pressed = false;
static bool is_pwr_btn_pressed = false;
sem_t semaphore_button_stuck_event; // for monitor debug card power/reset button

static void
debug_card_btn_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = NULL;
  struct timespec now_timespec;
  uint8_t power_status = 0, uart_sel_value = DEBUG_UART_SEL_BMC;
  int ret = 0, press_time = 0;
  
  if (gp == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle debug card button because parameter: *gp is NULL\n", __func__);
    return;
  }
  
  cfg = gpio_poll_get_config(gp);
  if (cfg == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle debug card button because parameter: *cfg is NULL\n", __func__);
    return;
  }
  
  ret = pal_get_debug_card_uart_sel(&uart_sel_value);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): fail to get BMC UART selection\n", __func__);
    return;
  }
  
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);
  
  // UART console is selected to BMC
  if (uart_sel_value == DEBUG_UART_SEL_BMC) {
    if (strcmp(cfg->shadow, "DEBUG_RST_BTN_N") == 0) {
      //press reset button
      if (curr == GPIO_VALUE_LOW) {
        is_rst_btn_pressed = true;
        sem_post(&semaphore_button_stuck_event);
        press_rst_btn_time.tv_sec = now_timespec.tv_sec;
      
      //release reset button
      } else if (curr == GPIO_VALUE_HIGH) {
        is_rst_btn_pressed = false;
        press_time = now_timespec.tv_sec - press_rst_btn_time.tv_sec;
        
        if ((press_time >= SHORT_PRESS_BUTTON_TIME) && (press_time < STUCK_BUTTON_TIME)) {
          syslog(LOG_CRIT, "Debug card reset button pressed %d seconds: BMC RESET\n", press_time);
          run_command("reboot");
        } else if (press_time > STUCK_BUTTON_TIME) {
          syslog(LOG_CRIT, "DEASSERT: Debug card reset button seems to stuck long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
        }
      }
    }
  
  // UART console is selected to host
  } else if (uart_sel_value == DEBUG_UART_SEL_HOST) {
    if (pal_get_server_power(FRU_SERVER, &power_status) < 0) {
      syslog(LOG_WARNING, "%s(): fail to get server power status\n", __func__);
      return;
    }
    
    if (strcmp(cfg->shadow, "DEBUG_RST_BTN_N") == 0) {
      // press reset button
      if (curr == GPIO_VALUE_LOW) {
        is_rst_btn_pressed = true;
        sem_post(&semaphore_button_stuck_event);
        press_rst_btn_time.tv_sec = now_timespec.tv_sec;
      
      // release reset button
      } else if (curr == GPIO_VALUE_HIGH) {
        is_rst_btn_pressed = false;
        press_time = now_timespec.tv_sec - press_rst_btn_time.tv_sec;
        
        if ((press_time >= SHORT_PRESS_BUTTON_TIME) && (press_time < STUCK_BUTTON_TIME)) {
          if (power_status == SERVER_POWER_ON) {
            syslog(LOG_CRIT, "Debug card reset button pressed %d seconds: HOST RESET\n", press_time);
            ret = pal_set_server_power(FRU_SERVER, SERVER_POWER_RESET);
            if (ret < 0) {
              syslog(LOG_ERR, "%s(): fail to send host reset command\n", __func__);
              return;
            }
          } else {
            syslog(LOG_WARNING, "%s(): fail to send host reset command: HOST is DC OFF\n", __func__);
            return;
          }
        } else if (press_time > STUCK_BUTTON_TIME) {
          syslog(LOG_CRIT, "DEASSERT: Debug card reset button seems to stuck long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
        }
      }
    } else if (strcmp(cfg->shadow, "DEBUG_PWR_BTN_N") == 0) {
      // press power button
      if (curr == GPIO_VALUE_LOW) {
        is_pwr_btn_pressed = true;
        sem_post(&semaphore_button_stuck_event);
        press_pwr_btn_time.tv_sec = now_timespec.tv_sec;

      // release power button
      } else if (curr == GPIO_VALUE_HIGH) {
        is_pwr_btn_pressed = false;
        press_time = now_timespec.tv_sec - press_pwr_btn_time.tv_sec;     
        
        if ((press_time >= SHORT_PRESS_BUTTON_TIME) && (press_time < STUCK_BUTTON_TIME)) {
          if (power_status == SERVER_POWER_OFF) {
            syslog(LOG_CRIT, "Debug card power button pressed %d seconds: HOST POWER ON ", press_time);
            ret = pal_set_server_power(FRU_SERVER, SERVER_POWER_ON);
            if (ret < 0) {
              syslog(LOG_ERR, "%s(): fail to send host power on command\n", __func__);
              return;
            }

          } else if (power_status == SERVER_POWER_ON) {
            // press more than 4s: power off
            if (press_time > LONG_PRESS_BUTTON_TIME) {
              syslog(LOG_CRIT, "Debug card power button pressed %d seconds: HOST POWER OFF ", press_time);
              ret = pal_set_server_power(FRU_SERVER, SERVER_POWER_OFF);
              if (ret < 0) {
                syslog(LOG_ERR, "%s(): fail to send host power off command\n", __func__);
                return;
              }
            
            // press 1-4s: graceful shutdown
            } else {
              syslog(LOG_CRIT, "Debug card power button pressed %d seconds: HOST GRACEFUL SHUTDOWN ", press_time);
              ret = pal_set_server_power(FRU_SERVER, SERVER_GRACEFUL_SHUTDOWN);
              if (ret < 0) {
                syslog(LOG_ERR, "%s(): fail to send host graceful shutdown command\n", __func__);
                return;
              }  
            }
          } else {
            syslog(LOG_WARNING, "%s(): fail to send host power control command: HOST is AC OFF\n", __func__);
            return;
          }
        } else if (press_time > STUCK_BUTTON_TIME) {
          syslog(LOG_CRIT, "DEASSERT: Debug card power button seems to stuck long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
        }
      }
    }
  }
}

static void
*button_stuck_event() {
  int i = 0;
  bool is_rst_btn_stuck = false, is_pwr_btn_stuck = false;
  
  while (1) {
    sem_wait(&semaphore_button_stuck_event);

    is_rst_btn_stuck = true;
    if (is_rst_btn_pressed == true) {
      for (i = 0; i < STUCK_BUTTON_TIME; i++) {
        if (is_rst_btn_pressed == false) {
          is_rst_btn_stuck = false;
          break;
        }
        sleep(1);
      }
      if (is_rst_btn_stuck == true) {
        syslog(LOG_CRIT, "ASSERT: Debug card reset button seems to stuck long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
      }   
    } 
    
    is_pwr_btn_stuck = true;
    if (is_pwr_btn_pressed == true) {
      for (i = 0; i < STUCK_BUTTON_TIME; i++) {
        if (is_pwr_btn_pressed == false) {
          is_pwr_btn_stuck = false;
          break;
        }
        sleep(1);
      }
      if (is_pwr_btn_stuck == true) {
        syslog(LOG_CRIT, "ASSERT: Debug card power button seems to stuck long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
      }
    }
  }

  return NULL;
}

// GPIO table
static struct gpiopoll_config gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"DEBUG_RST_BTN_N",   "GPIOI7",   GPIO_EDGE_BOTH,  debug_card_btn_hndlr, NULL},
  {"DEBUG_PWR_BTN_N",   "GPION0",   GPIO_EDGE_BOTH,  debug_card_btn_hndlr, NULL},
};

int
main(int argc, char **argv) {
  int rc = 0;
  int pid_file = 0;
  size_t gpio_cnt = 0;
  gpiopoll_desc_t *poll_desc = NULL;
  pthread_t tid_button_stuck_event;
  
  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = pal_flock_retry(pid_file);
  if (rc < 0) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "another gpiointrd instance is running...\n");
    } else {
      syslog(LOG_ERR, "fail to execute beacuse %s\n", strerror(errno));
    }
    
    pal_unflock_retry(pid_file);
    close(pid_file);
    exit(EXIT_FAILURE);
  } else {
    
    sem_init(&semaphore_button_stuck_event, 0, 0);
    // Create a thread to monitor debug card power/reset button are stuck or not
    if (pthread_create(&tid_button_stuck_event, NULL, button_stuck_event, NULL) < 0) {
      syslog(LOG_ERR, "failed to create thread for monitoring debug card button status");
      exit(EXIT_FAILURE);
    }

    gpio_cnt = sizeof(gpios) / sizeof(gpios[0]);
    if (gpio_cnt > 0) {
      syslog(LOG_INFO, "daemon start");

      poll_desc = gpio_poll_open(gpios, gpio_cnt);
      if (poll_desc == NULL) {
        syslog(LOG_CRIT, "fail to start poll operation on GPIOs\n");
      } else {
        if (gpio_poll(poll_desc, POLL_TIMEOUT) < 0) {
          syslog(LOG_CRIT, "fail to poll gpio because gpio_poll() return error\n");
        }
        gpio_poll_close(poll_desc);
      }
    }
  }

  pal_unflock_retry(pid_file);
  close(pid_file);
  return 0;
}
