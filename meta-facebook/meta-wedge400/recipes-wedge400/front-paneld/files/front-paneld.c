/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#define FRONTPANELD_NAME       "front-paneld"
#define FRONTPANELD_PID_FILE   "/var/run/front-paneld.pid"

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40

#define INTERVAL_MAX  5


// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused){
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME,__FUNCTION__);
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
          /* Setup management port LED */
          run_command("/usr/local/bin/setup_mgmt.sh led");
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
  int brd_rev;
  pal_get_board_rev(&brd_rev);
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SMB);
  exit(0);
}

// Thread for monitoring sim LED
static void *
simLED_monitor_handler(void *unused) {
  int brd_rev;
  uint8_t interval = 0;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME,__FUNCTION__);
  pal_get_board_rev(&brd_rev);
  init_led();
  while(1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if ( last_time == 0 || curr_time < last_time ||
         curr_time - last_time > 500 ){  // do sys led every 0.5 second
      // checking FRU Presence and display SYS LED
      set_sys_led(brd_rev);
      last_time = curr_time;
      if(interval++ > 4){   // check others led every 2 second to reduce message log
        interval = 0;
        // checking Sensor and display SMB LED
        set_smb_led(brd_rev);
        // checking FAN status and display FAN LED
        set_fan_led(brd_rev);
        // checking PSU status and display PSU LED
        set_psu_led(brd_rev);
      }
    }
    usleep(500000);
  }
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t lpc;

  OBMC_INFO("%s: %s started", FRONTPANELD_NAME,__FUNCTION__);
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
    }

    // If Debug Card is present
    if (curr) {

      // Enable POST codes for scm slot
      ret = pal_post_enable(IPMB_BUS);
      if (ret) {
        goto debug_card_done;
      }

      // Get last post code and display it
      ret = pal_post_get_last(IPMB_BUS, &lpc);
      if (ret) {
        goto debug_card_done;
      }

      ret = pal_post_handle(IPMB_BUS, lpc);
      if (ret) {
        goto debug_card_out;
      }
    }

debug_card_done:
    prev = curr;
debug_card_out:
    if (curr == 1)
      msleep(500);
    else
      sleep(1);
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
  while (1) {
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
        OBMC_INFO("Power button released\n");
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
    msleep(100);
  }
  return 0;
}

/* Thread to handle Reset Button to reset userver */
static void *
rst_btn_handler(void *unused) {
  uint8_t btn;
  while (1) {
    /* Check the position of hand switch
     * and if reset button is pressed */
    pal_get_dbg_rst_btn(&btn);
    if (btn) {
      /* Reset userver */
      pal_set_server_power(FRU_SCM, SERVER_POWER_RESET);
    }
  }
  return 0;
}


/* Thread to handle Reset Button to reset userver */
static void *
uart_sel_btn_handler(void *unused) {
  uint8_t btn;
  /* Clear Debug Card reset button at the first time */
  pal_clr_dbg_uart_btn();
  while (1) {
    /* Check the position of hand switch
     * and if reset button is pressed */
    pal_get_dbg_uart_btn(&btn);
    if (btn) {
      pal_switch_uart_mux();
      pal_clr_dbg_uart_btn();
    }
  }
  return 0;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_scm_monitor;
  pthread_t tid_debug_card;
  pthread_t tid_simLED_monitor;
  pthread_t tid_pwr_btn;
  pthread_t tid_rst_btn;
  pthread_t tid_uart_sel_btn;
  int pid_file;
  uint8_t brd_type_rev;
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

  if (pal_get_board_type_rev(&brd_type_rev)) {
    OBMC_WARN("Get board revision failed\n");
    exit(-1);
  }

  if (pthread_create(&tid_scm_monitor, NULL, scm_monitor_handler, NULL) != 0) {
    OBMC_WARN("pthread_create for scm monitor error\n");
    exit(1);
  }
    if (pthread_create(&tid_simLED_monitor, NULL, simLED_monitor_handler, NULL)	  != 0) {
    OBMC_WARN("pthread_create for simLED monitor error\n");
    exit(1);
  }

  if (brd_type_rev != BOARD_WEDGE400_EVT) {
    if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) != 0) {
        OBMC_WARN("pthread_create for debug card error\n");
        exit(1);
    }
    pthread_join(tid_debug_card, NULL);
  }

  if (pthread_create(&tid_uart_sel_btn, NULL, uart_sel_btn_handler, NULL) < 0) {
    OBMC_WARN("pthread_create for uart select button error\n");
    exit(1);
  }

  if (pthread_create(&tid_pwr_btn, NULL, pwr_btn_handler, NULL) < 0) {
    OBMC_WARN("pthread_create for power button error\n");
    exit(1);
  }

  if (pthread_create(&tid_rst_btn, NULL, rst_btn_handler, NULL) < 0) {
    OBMC_WARN("pthread_create for reset button error\n");
    exit(1);
  }

  pthread_join(tid_rst_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  pthread_join(tid_scm_monitor, NULL);
  pthread_join(tid_simLED_monitor, NULL);
  pthread_join(tid_uart_sel_btn, NULL);
  return 0;
}
