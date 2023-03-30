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

#define _GNU_SOURCE
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
#include <openbmc/kv.h>
#include <facebook/fbgc_gpio.h>

#define POLL_TIMEOUT -1 /* Forever */

#define STUCK_BUTTON_TIME  10  // second
#define LONG_PRESS_BUTTON_TIME   4  // second
#define SHORT_PRESS_BUTTON_TIME  1  // second
#define DISPLAY_UART_SEL_TIMEOUT_INTERVAL    5  // second

static struct timespec press_dbg_rst_btn_time;
static struct timespec press_dbg_pwr_btn_time;
static struct timespec press_dbg_uart_sel_btn_time;
static bool is_dbg_rst_btn_pressed = false;
static bool is_dbg_pwr_btn_pressed = false;
static bool is_dbg_uart_sel_btn_pressed = false;
sem_t semaphore_button_stuck_event; // for monitoring button stuck
pthread_cond_t uart_sel_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t post_code_lock = PTHREAD_MUTEX_INITIALIZER;
static int post_code_stored = 0;
static bool is_post_code_need_restore = false;

static void
debug_card_pwr_rst_btn_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
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
        is_dbg_rst_btn_pressed = true;
        sem_post(&semaphore_button_stuck_event);
        press_dbg_rst_btn_time.tv_sec = now_timespec.tv_sec;
      
      //release reset button
      } else if (curr == GPIO_VALUE_HIGH) {
        is_dbg_rst_btn_pressed = false;
        press_time = now_timespec.tv_sec - press_dbg_rst_btn_time.tv_sec;
        
        if ((press_time >= SHORT_PRESS_BUTTON_TIME) && (press_time < STUCK_BUTTON_TIME)) {
          syslog(LOG_CRIT, "Debug card reset button pressed %d seconds: BMC RESET\n", press_time);
          run_command("reboot");
        } else if (press_time > STUCK_BUTTON_TIME) {
          syslog(LOG_CRIT, "DEASSERT: Debug card reset button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
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
        is_dbg_rst_btn_pressed = true;
        sem_post(&semaphore_button_stuck_event);
        press_dbg_rst_btn_time.tv_sec = now_timespec.tv_sec;
      
      // release reset button
      } else if (curr == GPIO_VALUE_HIGH) {
        is_dbg_rst_btn_pressed = false;
        press_time = now_timespec.tv_sec - press_dbg_rst_btn_time.tv_sec;
        
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
          syslog(LOG_CRIT, "DEASSERT: Debug card reset button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
        }
      }
    } else if (strcmp(cfg->shadow, "DEBUG_PWR_BTN_N") == 0) {
      // press power button
      if (curr == GPIO_VALUE_LOW) {
        is_dbg_pwr_btn_pressed = true;
        sem_post(&semaphore_button_stuck_event);
        press_dbg_pwr_btn_time.tv_sec = now_timespec.tv_sec;

      // release power button
      } else if (curr == GPIO_VALUE_HIGH) {
        is_dbg_pwr_btn_pressed = false;
        press_time = now_timespec.tv_sec - press_dbg_pwr_btn_time.tv_sec;     
        
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
          syslog(LOG_CRIT, "DEASSERT: Debug card power button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
        }
      }
    }
  }
}

static void
debug_card_uart_sel_btn_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = NULL;
  struct timespec now_timespec;
  uint8_t uart_sel_value = DEBUG_UART_SEL_BMC, current_post_code = 0;
  int press_time = 0;

  if (gp == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle debug card uart selection button because parameter: *gp is NULL\n", __func__);
    return;
  }

  cfg = gpio_poll_get_config(gp);
  if (cfg == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle debug card uart selection button because parameter: *cfg is NULL\n", __func__);
    return;
  }

  clock_gettime(CLOCK_MONOTONIC, &now_timespec);

  if (strcmp(cfg->shadow, "DEBUG_BMC_UART_SEL_R") == 0) {
    //press uart selection button
    if (curr == GPIO_VALUE_LOW) {
      is_dbg_uart_sel_btn_pressed = true;
      sem_post(&semaphore_button_stuck_event);
      press_dbg_uart_sel_btn_time.tv_sec = now_timespec.tv_sec;

    //release uart selection button
    } else if (curr == GPIO_VALUE_HIGH) {
      is_dbg_uart_sel_btn_pressed = false;
      press_time = now_timespec.tv_sec - press_dbg_uart_sel_btn_time.tv_sec;

      if (press_time < STUCK_BUTTON_TIME) {
        if (pal_get_debug_card_uart_sel(&uart_sel_value) < 0) {
          syslog(LOG_WARNING, "%s(): fail to get BMC UART selection\n", __func__);
          return;
        }
        pthread_mutex_lock(&post_code_lock);
        if (pal_get_current_led_post_code(&current_post_code) < 0) {
          syslog(LOG_WARNING, "%s(): fail to get current post code\n", __func__);
        }
        if (is_post_code_need_restore == false) {
          is_post_code_need_restore = true;
          post_code_stored = current_post_code;
        }
        pthread_mutex_unlock(&post_code_lock);
        pal_post_display(uart_sel_value);
        pthread_cond_signal(&uart_sel_condition);

      } else if (press_time > STUCK_BUTTON_TIME) {
        syslog(LOG_CRIT, "DEASSERT: Debug card uart selection button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
      }
    }
  }
}

static bool
is_button_stucked(bool *is_btn_pressed) {
  bool ret = false;
  struct timespec now_timespec, start_timespec;
 
  if (is_btn_pressed == NULL) {
    syslog(LOG_WARNING, "%s(): fail to check button stuck, parameter: *is_btn_pressed is NULL\n", __func__);
    return ret;
  }

  clock_gettime(CLOCK_MONOTONIC, &start_timespec);

  if ((*is_btn_pressed) == true) {
    do {
      clock_gettime(CLOCK_MONOTONIC, &now_timespec);
      if ((*is_btn_pressed) == false) {
        return ret;
      }
      sleep(1);
    } while ((now_timespec.tv_sec - start_timespec.tv_sec) < STUCK_BUTTON_TIME);

    ret = true;
  }

  return ret;
}

static void
*button_stuck_event() {
  while (1) {
    sem_wait(&semaphore_button_stuck_event);

    if (is_button_stucked(&is_dbg_rst_btn_pressed) == true) {
      syslog(LOG_CRIT, "ASSERT: Debug card reset button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
    }
    if (is_button_stucked(&is_dbg_pwr_btn_pressed) == true) {
      syslog(LOG_CRIT, "ASSERT: Debug card power button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
    }
    if (is_button_stucked(&is_dbg_uart_sel_btn_pressed) == true) {
      syslog(LOG_CRIT, "ASSERT: Debug card uart selection button seems to stuck for a long time (more than %d seconds)\n", STUCK_BUTTON_TIME);
    }
  }

  return NULL;
}

void *post_code_restorer(){
  int ret = 0;
  pthread_condattr_t attr;
  struct timespec timeout = {0};
   
  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  pthread_cond_init(&uart_sel_condition, &attr); 
   
  while(1) {  
    if (clock_gettime(CLOCK_MONOTONIC, &timeout) != 0) {
      syslog(LOG_WARNING, "%s(): fail to get current time\n", __func__);
      sleep(1);
      continue;
    }

    timeout.tv_sec += DISPLAY_UART_SEL_TIMEOUT_INTERVAL;
    pthread_mutex_lock(&post_code_lock);
    ret = pthread_cond_timedwait(&uart_sel_condition, &post_code_lock, &timeout);

    if (ret == ETIMEDOUT) {
      if (is_post_code_need_restore == true) {
        pal_post_display(post_code_stored);
        is_post_code_need_restore = false;
      }
    }
    pthread_mutex_unlock(&post_code_lock);
  }
   
  pthread_exit(0);
}

static void
fru_missing_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = NULL;
  int ret = 0;
  uint8_t server_power_status = 0;
  
  if (gp == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle fru missing because parameter: *gp is NULL\n", __func__);
    return;
  }
  
  cfg = gpio_poll_get_config(gp);
  if (cfg == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle fru missing because parameter: *cfg is NULL\n", __func__);
    return;
  }
  
  if (strcmp(cfg->shadow, "NIC_PRSNTB3_N") == 0) {
    if (curr == GPIO_VALUE_HIGH) {
      syslog(LOG_CRIT, "ASSERT: nic missing");
      pal_set_error_code(ERR_CODE_NIC_MISSING, ERR_CODE_ENABLE);

      if (pal_nic_poweroff_action() < 0) {
        syslog(LOG_WARNING, "%s(): Fail to handle actions after NIC is removed.", __func__);
      }
    } else if (curr == GPIO_VALUE_LOW) {
      syslog(LOG_CRIT, "DEASSERT: nic missing");
      pal_set_error_code(ERR_CODE_NIC_MISSING, ERR_CODE_DISABLE);

      if (pal_nic_poweron_action() < 0) {
        syslog(LOG_WARNING, "%s(): Fail to handle actions after NIC is inserted.", __func__);
      }
    }
    
    ret = pal_get_server_power(FRU_SERVER, &server_power_status);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s(): fail to get server power status", __func__);
      return;
    }
    if (server_power_status == SERVER_12V_OFF) {
        syslog(LOG_CRIT, "Server is AC OFF");
      } else if (server_power_status == SERVER_12V_ON) {
        syslog(LOG_CRIT, "Server is AC ON");
      } else if (server_power_status == SERVER_POWER_OFF) {
        syslog(LOG_CRIT, "Server is DC OFF");
      } else if (server_power_status == SERVER_POWER_ON) {
        syslog(LOG_CRIT, "Server is DC ON");
      }
  }
  
}

static void
fru_missing_init(gpiopoll_pin_t *gp, gpio_value_t value) {
  const struct gpiopoll_config *cfg = NULL;
  uint8_t server_power_status = 0;
  int ret = 0;
  
  if (gp == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle fru missing because parameter: *gp is NULL\n", __func__);
    return;
  }
  
  cfg = gpio_poll_get_config(gp);
  if (cfg == NULL) {
    syslog(LOG_WARNING, "%s(): fail to handle fru missing because parameter: *cfg is NULL\n", __func__);
    return;
  }
  
  if (strcmp(cfg->shadow, "NIC_PRSNTB3_N") == 0) {
    if (value == GPIO_VALUE_HIGH) {
      syslog(LOG_CRIT, "ASSERT: nic missing");
      pal_set_error_code(ERR_CODE_NIC_MISSING, ERR_CODE_ENABLE);

      if (pal_nic_poweroff_action() < 0) {
        syslog(LOG_WARNING, "%s(): Fail to handle actions after NIC is removed.", __func__);
      }

      ret = pal_get_server_power(FRU_SERVER, &server_power_status);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s(): fail to get server power status", __func__);
        return;
      }
      if (server_power_status == SERVER_12V_OFF) {
        syslog(LOG_CRIT, "Server is AC OFF");
      } else if (server_power_status == SERVER_12V_ON) {
        syslog(LOG_CRIT, "Server is AC ON");
      } else if (server_power_status == SERVER_POWER_OFF) {
        syslog(LOG_CRIT, "Server is DC OFF");
      } else if (server_power_status == SERVER_POWER_ON) {
        syslog(LOG_CRIT, "Server is DC ON");
      }
    }
  }
}

// GPIO table
static struct gpiopoll_config gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"DEBUG_RST_BTN_N",      "GPIOI7",   GPIO_EDGE_BOTH,  debug_card_pwr_rst_btn_hndlr,  NULL},
  {"DEBUG_PWR_BTN_N",      "GPION0",   GPIO_EDGE_BOTH,  debug_card_pwr_rst_btn_hndlr,  NULL},
  {"DEBUG_BMC_UART_SEL_R", "GPIOM4",   GPIO_EDGE_BOTH,  debug_card_uart_sel_btn_hndlr, NULL},
  {"NIC_PRSNTB3_N",        "GPIOS0",   GPIO_EDGE_BOTH,  fru_missing_hndlr,             fru_missing_init},
};

int
main(int argc, char **argv) {
  int rc = 0;
  int pid_file = 0;
  size_t gpio_cnt = 0;
  gpiopoll_desc_t *poll_desc = NULL;
  pthread_t tid_button_stuck_event;
  pthread_t tid_post_code_restorer;

  
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

    if (pthread_create(&tid_post_code_restorer, NULL, post_code_restorer, NULL) < 0) {
      syslog(LOG_ERR, "failed to create thread for restore debug card post code");
      exit(EXIT_FAILURE);
    }

    gpio_cnt = sizeof(gpios) / sizeof(gpios[0]);
    if (gpio_cnt > 0) {
      syslog(LOG_INFO, "daemon start");

      poll_desc = gpio_poll_open(gpios, gpio_cnt);
      if (poll_desc == NULL) {
        syslog(LOG_CRIT, "fail to start poll operation on GPIOs\n");
      } else {
        // set flag to notice BMC gpiointrd is ready
        kv_set("flag_gpiointrd", STR_VALUE_1, 0, 0);

        if (gpio_poll(poll_desc, POLL_TIMEOUT) < 0) {
          syslog(LOG_CRIT, "fail to poll gpio because gpio_poll() return error\n");
        }

        // clear the flag
        kv_set("flag_gpiointrd", STR_VALUE_0, 0, 0);
        gpio_poll_close(poll_desc);
      }
    }
  }

  pal_unflock_retry(pid_file);
  close(pid_file);
  return 0;
}
