/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <openbmc/log.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */

#define FRONTPANELD_NAME                    "front-paneld"
#define FRONTPANELD_PID_FILE                "/var/run/front-paneld.pid"

#define FRONT_PANELD_DEBUG_CARD_THREAD      "debug_card"
#define FRONT_PANELD_UART_SEL_BTN_THREAD    "uart_sel_btn"
#define FRONT_PANELD_PWR_BTN_THREAD         "pwr_btn"
#define FRONT_PANELD_RST_BTN_THREAD         "rst_btn"
#define FRONT_PANELD_LED_MONITOR_THREAD     "led_monitor"
#define FRONT_PANELD_LED_CONTROL_THREAD     "led_control"
#define FRONT_PANELD_SCM_MONITOR_THREAD     "scm_monitor"

//Debug card presence check interval. Preferred range from ## milliseconds to ## milleseconds.
#define DBG_CARD_CHECK_INTERVAL             500
#define LED_MONITOR_INTERVAL_S              5
#define LED_CONTROL_INTERVAL_MS             100
#define BTN_POWER_OFF                       40

#define ADM1278_NAME                        "adm1278"
#define ADM1278_ADDR                        0x10
#define SCM_ADM1278_BUS                     24

static uint8_t leds[4] = {
  LED_OFF, //LED_SYS
  LED_OFF, //LED_FAN
  LED_OFF, //LED_PSU
  LED_OFF  //LED_SCM
};

static int
write_adm1278_conf(uint8_t bus, uint8_t addr, uint8_t cmd, uint16_t data) {
  int dev = -1, read_back = -1;

  dev = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
  if (dev < 0) {
    OBMC_ERROR(errno, "i2c_cdev_slave_open bus %u addr 0x%x failed",
               bus, addr);
    return dev;
  }

  i2c_smbus_write_word_data(dev, cmd, data);
  read_back = i2c_smbus_read_word_data(dev, cmd);
  if (read_back < 0) {
    OBMC_ERROR(errno, "failed to read from %u-00%02x", bus, addr);
    close(dev);
    return read_back;
  }

  if (((uint16_t) read_back) != data) {
    OBMC_WARN("unexpected data from %u-00%02x: (%04X, %04X)",
              bus, addr, data, read_back);
    close(dev);
    return -1;
  }

  close(dev);
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);

    if (ret) {
      goto debug_card_out;
    }
    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr != prev) {
      if (!curr) {
        // Debug Card was removed
        OBMC_WARN("Debug Card Extraction\n");

      } else {
        // Debug Card was inserted
        OBMC_WARN("Debug Card Insertion\n");
      }
      prev = curr;
    }
debug_card_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return 0;
}

/* Thread to handle Uart Selection Button */
static void *
uart_sel_btn_handler(void *unused) {
  uint8_t btn;
  uint8_t prsnt = 0;
  int ret;
  /* Clear Debug Card reset button at the first time */
  ret = pal_clr_dbg_uart_btn();
  if (ret) {
    OBMC_INFO("%s: %s clear uart button status error",  FRONTPANELD_NAME, __FUNCTION__);
  }

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);

  while (1) {
    /* Check the position of hand switch
     * and if reset button is pressed */
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto uart_sel_btn_out;
    }
    ret = pal_get_dbg_uart_btn(&btn);
    if (ret || !btn) {
      goto uart_sel_btn_out;
    }
    if (btn) {
      OBMC_INFO("Uart button pressed\n");
      pal_switch_uart_mux();
      pal_clr_dbg_uart_btn();
    }
uart_sel_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return 0;
}

/* Thread to handle Power Button to power on/off userver */
static void *
pwr_btn_handler(void *unused) {
  int i;
  int ret;
  uint8_t btn = 0;
  uint8_t cmd = 0;
  uint8_t power = 0;
  uint8_t prsnt = 0;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto pwr_btn_out;
    }

    ret = pal_get_dbg_pwr_btn(&btn);
    if (ret || !btn) {
        goto pwr_btn_out;
    }

    OBMC_INFO("Power button pressed\n");
    // Wati for the button to be released
    for (i = 0; i < BTN_POWER_OFF; i++) {
        ret = pal_get_dbg_pwr_btn(&btn);
        if (ret || btn) {
            msleep(100);
            continue;
        }
        break;
    }

    // Get the current power state (on or off)
    ret = pal_get_server_power(FRU_SCM, &power);
    if (ret) {
      goto pwr_btn_out;
    }

    // Set power command should reverse of current power state
    cmd = !power;
    
    // To determine long button press
    if (i >= BTN_POWER_OFF) {
      pal_update_ts_sled();
    } else {
      // If current power state is ON and it is not a long press,
      // the power off shoul dbe Graceful Shutdown
      if (power == SERVER_POWER_ON)
        cmd = SERVER_GRACEFUL_SHUTDOWN;

      pal_update_ts_sled();
      OBMC_INFO("Power button pressed for FRU: %d\n", FRU_SCM);
    }

    // Reverse the power state of the given server.
    ret = pal_set_server_power(FRU_SCM, cmd);
pwr_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }
  return 0;
}

/* Thread to handle Reset Button to reset userver */
static void *
rst_btn_handler(void *unused) {
  uint8_t btn;
  int ret;
  uint8_t prsnt = 0;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto rst_btn_out;
    }
    /* Check the position of hand switch
     * and if reset button is pressed */
    ret = pal_get_dbg_rst_btn(&btn);
    if (ret || !btn) {
      goto rst_btn_out;
    }

    if (btn) {
      OBMC_INFO("Rst button pressed\n");
      /* Reset userver */
      pal_set_server_power(FRU_SCM, SERVER_POWER_RESET);
    }
rst_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }
  return 0;
}

static void *
led_monitor_handler(void *unused) {
  uint8_t status_ug;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if ( last_time == 0 || curr_time < last_time ||
         curr_time - last_time > LED_MONITOR_INTERVAL_S * 1000 ) {
      last_time = curr_time;

      //SYS LED
      // BLUE            no out of range of SMB/BMC sensor
      // AMBER           one or more sensor out of range
      // ALTERNATE       firmware upgrade in process
      // AMBER FLASHING  need technicial required,
      pal_mon_fw_upgrade(&status_ug);
      if(status_ug) {
        OBMC_WARN("firmware is upgrading");
        leds[LED_SYS] = LED_STATE_ALT_BLINK;
      } else {
        if (smb_check()) {
          leds[LED_SYS] = LED_STATE_AMBER;
        } else {
          leds[LED_SYS] = LED_STATE_BLUE;
        }
      }

      //FAN LED
      // BLUE   all presence and sensor normal
      // AMBER  one or more absence or sensor out-of-range RPM
      if (fan_check() == 0) {
        leds[LED_FAN] = LED_STATE_BLUE;
      } else {
        leds[LED_FAN] = LED_STATE_AMBER;
      }

      //PSU LED
      // BLUE     all PSUs present and INPUT OK,PWR OK
      // AMBER    one or more not present or INPUT OK or PWR OK de-asserted
      if (psu_check() == 0) {
        leds[LED_PSU] = LED_STATE_BLUE;
      } else {
        leds[LED_PSU] = LED_STATE_AMBER;
      }

      //SCM LED
      // BLUE   SCM present and sensor normal
      // AMBER  SCM not present or sensor out-of-range
      if (scm_check() == 0) {
        leds[LED_SCM] = LED_STATE_BLUE;
      } else {
        leds[LED_SCM] = LED_STATE_AMBER;
      }
    }
    sleep(1);
  }

  return 0;
}

static void *
led_control_handler(void *unused) {
  static uint8_t led_alt = 0;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if ( last_time == 0 || curr_time < last_time ||
         curr_time - last_time > 500 ) {  // blinking every 500ms
      led_alt = 1-led_alt;
      last_time = curr_time;
    }

    for ( int led=0 ; led < sizeof(leds)/sizeof(leds[0]) ; led++ ) {
      int state = leds[led];
      if (state == LED_STATE_OFF) {
        set_sled(led, LED_COLOR_OFF);
      } else if (state == LED_STATE_BLUE) {
        set_sled(led, LED_COLOR_BLUE);
      } else if (state == LED_STATE_AMBER) {
        set_sled(led, LED_COLOR_AMBER);
      } else if (state == LED_STATE_ALT_BLINK) {
        if (led_alt) {
          set_sled(led, LED_COLOR_BLUE);
        } else {
          set_sled(led, LED_COLOR_AMBER);
        }
      } else if (state == LED_STATE_AMBER_BLINK) {
        if (led_alt) {
          set_sled(led, LED_COLOR_AMBER);
        } else {
          set_sled(led, LED_COLOR_OFF);
        }
      }
    }
    msleep(LED_CONTROL_INTERVAL_MS);
  }

  return 0;
}

// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
    if (ret) {
      goto scm_mon_out;
    }
    curr = prsnt;
    if (curr != prev) {
      if (curr) {
        // SCM was inserted
        OBMC_WARN("SCM Insertion\n");

        ret = pal_get_server_power(FRU_SCM, &power);
        if (ret) {
          goto scm_mon_out;
        }
        if (power == SERVER_POWER_OFF) {
          sleep(3);
          OBMC_WARN("SCM power on\n");
          pal_set_server_power(FRU_SCM, SERVER_POWER_ON);
          /* Config ADM1278 power monitor averaging */
          if (write_adm1278_conf(SCM_ADM1278_BUS, ADM1278_ADDR, 0xD4, 0x3F1E)) {
            OBMC_CRIT("SCM Bus:%d Addr:0x%x Device:%s "
                             "can't config register",
                             SCM_ADM1278_BUS, ADM1278_ADDR, ADM1278_NAME);
          }
          goto scm_mon_out;
        }
      } else {
        // SCM was removed
        OBMC_WARN("SCM Extraction\n");
      }
    }
scm_mon_out:
    prev = curr;
    sleep(1);
  }
  return 0;
}

void
exithandler(int signum) {
  set_sled(LED_SYS, LED_COLOR_AMBER);
  exit(0);
}

int
main (int argc, char * const argv[]) {
  int i,rc=0;
  int pid_file;
  struct {
    const char *name;
    void* (*handler)(void *args);
    bool initialized;
    pthread_t tid;
  } front_paneld_threads[7] = {
    {
      .name = FRONT_PANELD_DEBUG_CARD_THREAD,
      .handler = debug_card_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_UART_SEL_BTN_THREAD,
      .handler = uart_sel_btn_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_PWR_BTN_THREAD,
      .handler = pwr_btn_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_RST_BTN_THREAD,
      .handler = rst_btn_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_LED_MONITOR_THREAD,
      .handler = led_monitor_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_LED_CONTROL_THREAD,
      .handler = led_control_handler,
      .initialized = false,
    },
    {
      .name = FRONT_PANELD_SCM_MONITOR_THREAD,
      .handler = scm_monitor_handler,
      .initialized = false,
    },
  };

  signal(SIGTERM, exithandler);
  pid_file = open(FRONTPANELD_PID_FILE, O_CREAT | O_RDWR, 0666);
  if (pid_file < 0) {
    fprintf(stderr, "%s: failed to open %s: %s\n",
            FRONTPANELD_NAME, FRONTPANELD_PID_FILE, strerror(errno));
    return -1;
  }

  if (flock(pid_file, LOCK_EX | LOCK_NB) != 0) {
    if(EWOULDBLOCK == errno) {
      fprintf(stderr, "%s: another instance is running. Exiting..\n",
              FRONTPANELD_NAME);
    } else {
      fprintf(stderr, "%s: failed to lock %s: %s\n",
              FRONTPANELD_NAME, FRONTPANELD_PID_FILE, strerror(errno));
    }
    close(pid_file);
    return -1;
  }

  // initialize logging
  if( obmc_log_init(FRONTPANELD_NAME, LOG_INFO, 0) != 0) {
    fprintf(stderr, "%s: failed to initialize logger: %s\n",
            FRONTPANELD_NAME, strerror(errno));
    return -1;
  }

  if ( obmc_log_set_syslog(LOG_CONS, LOG_DAEMON) != 0) {
    fprintf(stderr, "%s: failed to setup syslog: %s\n",
            FRONTPANELD_NAME, strerror(errno));
    return -1;
  }
  obmc_log_unset_std_stream();

  OBMC_INFO("LED Initialize\n");
  init_led();

  for (i = 0; i < ARRAY_SIZE(front_paneld_threads); i++) {
    rc = pthread_create(&front_paneld_threads[i].tid, NULL,
                        front_paneld_threads[i].handler, NULL);
    if (rc != 0) {
      OBMC_ERROR(rc, "front_paneld: failed to create %s thread: %s\n",
                 front_paneld_threads[i].name, strerror(rc));
      goto cleanup;
    }
    front_paneld_threads[i].initialized = true;
  }

cleanup:
  for (i = ARRAY_SIZE(front_paneld_threads) - 1; i >= 0; i--) {
    if (front_paneld_threads[i].initialized) {
      pthread_join(front_paneld_threads[i].tid, NULL);
      front_paneld_threads[i].initialized = false;
    }
  }
  return rc;
}
