/*
 * healthd
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

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <pthread.h>
#include <openbmc/pal.h>
#include "watchdog.h"


// TODO: Remove macro once we plan to enable it for other platforms.

#if defined(CONFIG_FBTTN) || defined(CONFIG_LIGHTNING)
#define I2C_BUS_NUM            14
#define AST_I2C_BASE           0x1E78A000  /* I2C */
#define I2C_CMD_REG            0x14
#define AST_I2CD_SCL_LINE_STS  (0x1 << 18)
#define AST_I2CD_SDA_LINE_STS  (0x1 << 17)
#define AST_I2CD_BUS_BUSY_STS  (0x1 << 16)
#define PAGE_SIZE              0x1000

struct AST_I2C_DEV_OFFSET {
  uint32_t offset;
  char     *name;
};
struct AST_I2C_DEV_OFFSET ast_i2c_dev_offset[I2C_BUS_NUM] = {
  {0x040,  "I2C DEV1 OFFSET"},
  {0x080,  "I2C DEV2 OFFSET"},
  {0x0C0,  "I2C DEV3 OFFSET"},
  {0x100,  "I2C DEV4 OFFSET"},
  {0x140,  "I2C DEV5 OFFSET"},
  {0x180,  "I2C DEV6 OFFSET"},
  {0x1C0,  "I2C DEV7 OFFSET"},
  {0x300,  "I2C DEV8 OFFSET"},
  {0x340,  "I2C DEV9 OFFSET"},
  {0x380,  "I2C DEV10 OFFSET"},
  {0x3C0,  "I2C DEV11 OFFSET"},
  {0x400,  "I2C DEV12 OFFSET"},
  {0x440,  "I2C DEV13 OFFSET"},
  {0x480,  "I2C DEV14 OFFSET"},
};
#endif

static void
initilize_all_kv() {
  pal_set_def_key_value();
}

static void *
hb_handler() {
  int hb_interval;

#ifdef HB_INTERVAL
  hb_interval = HB_INTERVAL;
#else
  hb_interval = 500;
#endif

  while(1) {
    /* Turn ON the HB Led*/
    pal_set_hb_led(1);
    msleep(hb_interval);

    /* Turn OFF the HB led */
    pal_set_hb_led(0);
    msleep(hb_interval);
  }
}

static void *
watchdog_handler() {

  /* Start watchdog in manual mode */
  start_watchdog(0);

  /* Set watchdog to persistent mode so timer expiry will happen independent
   * of this process's liveliness.
   */
  set_persistent_watchdog(WATCHDOG_SET_PERSISTENT);

  while(1) {

    sleep(5);

    /*
     * Restart the watchdog countdown. If this process is terminated,
     * the persistent watchdog setting will cause the system to reboot after
     * the watchdog timeout.
     */
    kick_watchdog();
  }
}

#if defined(CONFIG_FBTTN) || defined(CONFIG_LIGHTNING)
static void *
i2c_mon_handler() {
  uint32_t i2c_fd;
  uint32_t i2c_cmd_sts[I2C_BUS_NUM];
  void *i2c_reg;
  void *i2c_cmd_reg;
  bool is_error_occur[I2C_BUS_NUM] = {false};
  char str_i2c_log[64];
  int timeout;
  int i;

  while (1) {
    i2c_fd = open("/dev/mem", O_RDWR | O_SYNC );
    if (i2c_fd >= 0) {  
      i2c_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, i2c_fd, AST_I2C_BASE);
      for (i = 0; i < I2C_BUS_NUM; i++) {
        i2c_cmd_reg = (char*)i2c_reg + ast_i2c_dev_offset[i].offset + I2C_CMD_REG;
        i2c_cmd_sts[i] = *(volatile uint32_t*) i2c_cmd_reg;

        timeout = 20;
        if ((i2c_cmd_sts[i] & AST_I2CD_SDA_LINE_STS) && !(i2c_cmd_sts[i] & AST_I2CD_SCL_LINE_STS)) {
          //if SDA == 1 and SCL == 0, it means the master is locking the bus.
          if (is_error_occur[i] == false) {
            while (i2c_cmd_sts[i] & AST_I2CD_BUS_BUSY_STS) {
              i2c_cmd_reg = (char*)i2c_reg + ast_i2c_dev_offset[i].offset + I2C_CMD_REG;
              i2c_cmd_sts[i] = *(volatile uint32_t*) i2c_cmd_reg;
              if (timeout < 0) {
                break;
              }
              timeout--;
              msleep(10);
            }
            // If the bus is busy over 200 ms, means the I2C transaction is abnormal.
            // To confirm the bus is not workable.
            if (timeout < 0) {
              pal_err_code_enable(0xE9 + i);            
              memset(str_i2c_log, 0, sizeof(char) * 64); 
              sprintf(str_i2c_log, "ASSERT: I2C bus %d crashed", i);          
              syslog(LOG_CRIT, str_i2c_log);
              is_error_occur[i] = true;
              pal_i2c_crash_assert_handle(i);
            }
          }
        } else {
          if (is_error_occur[i] == true) {
            pal_err_code_disable(0xE9 + i);
            memset(str_i2c_log, 0, sizeof(char) * 64); 
            sprintf(str_i2c_log, "DEASSERT: I2C bus %d crashed", i);          
            syslog(LOG_CRIT, str_i2c_log);
            is_error_occur[i] = false;
            pal_i2c_crash_deassert_handle(i);
          }
        }
      }
      munmap(i2c_reg, PAGE_SIZE);
      close(i2c_fd);
    }
    sleep(1);
  }
}
#endif

int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  pthread_t tid_watchdog;
  pthread_t tid_hb_led;
#if defined(CONFIG_FBTTN) || defined(CONFIG_LIGHTNING)
  pthread_t tid_i2c_mon;
#endif

  if (argc > 1) {
    exit(1);
  }

  pid_file = open("/var/run/healthd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another healthd instance is running...\n");
      exit(-1);
    }
  } else {

    daemon(1,1);

    openlog("healthd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "healthd: daemon started");
  }

  initilize_all_kv();

// For current platforms, we are using WDT from either fand or fscd
// TODO: keeping this code until we make healthd as central daemon that
//  monitors all the important daemons for the platforms.
  if (pthread_create(&tid_watchdog, NULL, watchdog_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for watchdog error\n");
    exit(1);
  }

  if (pthread_create(&tid_hb_led, NULL, hb_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for heartbeat error\n");
    exit(1);
  }

#if defined(CONFIG_FBTTN) || defined(CONFIG_LIGHTNING)
  // Add a thread for monitoring all I2C buses crash or not
  if (pthread_create(&tid_i2c_mon, NULL, i2c_mon_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for I2C errorr\n");
    exit(1);
  }
#endif

  pthread_join(tid_watchdog, NULL);

  pthread_join(tid_hb_led, NULL);

#if defined(CONFIG_FBTTN) || defined(CONFIG_LIGHTNING)
  pthread_join(tid_i2c_mon, NULL);
#endif

  return 0;
}

