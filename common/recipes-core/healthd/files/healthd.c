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
#include <sys/sysinfo.h>
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

#define CPU_INFO_PATH "/proc/stat"
#define CPU_THRESHOLD_PATH "/mnt/data/bmc_cpu_threshold"
#define MEM_THRESHOLD_PATH "/mnt/data/bmc_mem_threshold"
#define CPU_NAME_LENGTH 10
#define SLIDING_WINDOW_SIZE 120
#define MONITOR_INTERVAL 1
#define DEFAULT_CPU_THRESHOLD 70
#define DEFAULT_MEM_THRESHOLD 70
#define MAX_RETRY 10

static int cpu_over_threshold = 0, mem_over_threshold = 0;

enum {
  CPU = 0,
  MEM,
};

static void
initilize_all_kv() {
  pal_set_def_key_value();
}

int
get_threshold(uint8_t name, float *threshold) {
  char threshold_path[MAX_KEY_LEN], str[MAX_KEY_LEN];
  int file_non_exist = 0, rc;
  FILE *fp;

  switch (name) {
    case CPU:
      *threshold = DEFAULT_CPU_THRESHOLD;
      sprintf(threshold_path, "%s", CPU_THRESHOLD_PATH);
      break;

    case MEM:
      *threshold = DEFAULT_MEM_THRESHOLD;
      sprintf(threshold_path, "%s", MEM_THRESHOLD_PATH);
      break;

    default:
      return -1;
  }
  
  
  fp = fopen(threshold_path, "r");
  if (!fp && (errno == ENOENT)) {
    fp = fopen(threshold_path, "w");
    file_non_exist = 1;
  }
  if (!fp) {
    syslog(LOG_WARNING, "%s: failed to open %s", __func__, threshold_path);
    return -1;
  }

  // If there is no file can get threshold, we use the default value
  if (file_non_exist) {
    sprintf(str, "%.2f", *threshold);
    rc = fwrite(str, 1, sizeof(float), fp);
    if (rc < 0) {
      syslog(LOG_WARNING, "%s: failed to write threshold to %s",__func__, threshold_path);
      fclose(fp);
      return -1;
    }
  } else {
    // Get threshold from file
    rc = fread(str, 1, sizeof(float), fp);
    if (rc < 0) {
      syslog(LOG_WARNING, "%s: failed to read threshold from %s",__func__, threshold_path);
      fclose(fp);
      return -1;
    }
    *threshold = atof(str);
  }
  fclose(fp);
  
  return 0;
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
  uint32_t i2c_cmd_sts[I2C_BUS_NUM] = {false};
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

        timeout = 3000;
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
              usleep(10);
            }
            // If the bus is busy over 200 ms, means the I2C transaction is abnormal.
            // To confirm the bus is not workable.
            if (timeout < 0) {
              memset(str_i2c_log, 0, sizeof(char) * 64); 
              sprintf(str_i2c_log, "ASSERT: I2C bus %d crashed", i);          
              syslog(LOG_CRIT, str_i2c_log);
              is_error_occur[i] = true;
              pal_i2c_crash_assert_handle(i);
            }
          }
        } else {
          if (is_error_occur[i] == true) {
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

static void *
CPU_usage_monitor() {
  unsigned long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
  unsigned long long total_diff, idle_diff, non_idle, idle_time = 0, total = 0, pre_total = 0, pre_idle = 0;
  char cpu[CPU_NAME_LENGTH] = {0};
  int i, ready_flag = 0, timer = 0, rc, retry = 0;
  float cpu_threshold = 0;
  static float cpu_util_avg, cpu_util_total;
  static float cpu_utilization[SLIDING_WINDOW_SIZE] = {0};
  FILE *fp;

  rc = get_threshold(CPU, &cpu_threshold);
  if (rc < 0) {
    syslog(LOG_WARNING, "%s: Failed to get CPU threshold\n", __func__);
    cpu_threshold = DEFAULT_CPU_THRESHOLD;
  }

  if (cpu_threshold > 90)
    cpu_threshold = 90;

  while (1) {

    if (retry > MAX_RETRY) {
      syslog(LOG_CRIT, "Cannot get CPU statistics. Stop %s\n", __func__);
      return -1;
    }

    // Get CPU statistics. Time unit: jiffies
    fp = fopen(CPU_INFO_PATH, "r");
    if(!fp) {
      syslog(LOG_WARNING, "Failed to get CPU statistics.\n");
      retry++;
      continue;
    }    
    retry = 0; 
    
    fscanf(fp, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", 
                cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);

    fclose(fp);

    timer %= SLIDING_WINDOW_SIZE;

    // Need more data to cacluate the avg. utilization. We average 60 records here.
    if (timer == (SLIDING_WINDOW_SIZE-1) && !ready_flag) 
      ready_flag = 1;
    

    // guset and guest_nice are already accounted in user and nice so they are not included in total caculation
    idle_time = idle + iowait;
    non_idle = user + nice + system + irq + softirq + steal;
    total = idle_time + non_idle;

    // For runtime caculation, we need to take into account previous value. 
    total_diff = total - pre_total;
    idle_diff = idle_time - pre_idle;

    // These records are used to caculate the avg. utilization.
    cpu_utilization[timer] = (float) (total_diff - idle_diff)/total_diff;

    // Start to average the cpu utilization
    if (ready_flag) {
      cpu_util_total = 0;
      for (i=0; i<SLIDING_WINDOW_SIZE; i++) {
        cpu_util_total += cpu_utilization[i];
      }
      cpu_util_avg = cpu_util_total/SLIDING_WINDOW_SIZE;
      
      if (((cpu_util_avg*100) >= cpu_threshold) && !cpu_over_threshold)  {
        syslog(LOG_CRIT, "ASSERT: BMC CPU utilization (%.2f%%) exceeds the threshold (%.2f%%).\n", cpu_util_avg*100, cpu_threshold);
        cpu_over_threshold = 1;
        pal_bmc_err_enable();
      } else if (((cpu_util_avg*100) < cpu_threshold) && cpu_over_threshold)  {
        syslog(LOG_CRIT, "DEASSERT: BMC CPU utilization (%.2f%%) is under the threshold (%.2f%%).\n", cpu_util_avg*100, cpu_threshold);
        cpu_over_threshold = 0;
        // We can only disable BMC error code when both CPU and memory are fine.
        if (!mem_over_threshold)
          pal_bmc_err_disable();
      }
    }

    // Record current value for next caculation
    pre_total = total;
    pre_idle  = idle_time;

    timer++;
    sleep(MONITOR_INTERVAL);
  }
}

static void *
memory_usage_monitor() {
  struct sysinfo s_info;
  int i, error, timer = 0, ready_flag = 0, rc, retry = 0;
  float mem_threshold = 0;
  static float mem_util_avg, mem_util_total;
  static float mem_utilization[SLIDING_WINDOW_SIZE];

  rc = get_threshold(MEM, &mem_threshold);
  if (rc < 0) {
    syslog(LOG_WARNING, "%s: Failed to get memory threshold\n", __func__);
    mem_threshold = DEFAULT_MEM_THRESHOLD;
  }

  if (mem_threshold > 90)
    mem_threshold = 90;

  while (1) {

    if (retry > MAX_RETRY) {
      syslog(LOG_CRIT, "Cannot get sysinfo. Stop the %s\n", __func__);
      return -1;  
    }

    timer %= SLIDING_WINDOW_SIZE;

    // Need more data to cacluate the avg. utilization. We average 60 records here.
    if (timer == (SLIDING_WINDOW_SIZE-1) && !ready_flag) 
      ready_flag = 1;

    // Get sys info
    error = sysinfo(&s_info);
    if (error) {
      syslog(LOG_WARNING, "%s Failed to get sys info. Error: %d\n", __func__, error);
      retry++;
      continue;
    }
    retry = 0;

    // These records are used to caculate the avg. utilization.
    mem_utilization[timer] = (float) (s_info.totalram - s_info.freeram)/s_info.totalram;

    // Start to average the memory utilization
    if (ready_flag) {
      mem_util_total = 0;
      for (i=0; i<SLIDING_WINDOW_SIZE; i++)
        mem_util_total += mem_utilization[i];

      mem_util_avg = mem_util_total/SLIDING_WINDOW_SIZE;
      
      if (((mem_util_avg*100) >= mem_threshold) && !mem_over_threshold) {
        syslog(LOG_CRIT, "ASSERT: BMC Memory utilization (%.2f%%) exceeds the threshold (%.2f%%).\n", mem_util_avg*100, mem_threshold);
        mem_over_threshold = 1;
        pal_bmc_err_enable();
      } else if ((mem_util_avg*100) < mem_threshold && mem_over_threshold) {
        syslog(LOG_CRIT, "DEASSERT: BMC Memory utilization (%.2f%%) is under the threshold (%.2f%%).\n", mem_util_avg*100, mem_threshold);
        mem_over_threshold = 0;
        // We can only disable BMC error code when both CPU and memory are fine.
        if (!cpu_over_threshold)
          pal_bmc_err_disable();
      }
    }

    timer++;
    sleep(MONITOR_INTERVAL);
  }

}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  pthread_t tid_watchdog;
  pthread_t tid_hb_led;
#if defined(CONFIG_FBTTN) || defined(CONFIG_LIGHTNING)
  pthread_t tid_i2c_mon;
#endif
  pthread_t tid_cpu_monitor;
  pthread_t tid_mem_monitor;

  if (argc > 1) {
    exit(1);
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
  
  if (pthread_create(&tid_hb_led, NULL, CPU_usage_monitor, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for monitor CPU usage\n");
    exit(1);
  }

  if (pthread_create(&tid_hb_led, NULL, memory_usage_monitor, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for monitor memory usage\n");
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
  pthread_join(tid_cpu_monitor, NULL);

  pthread_join(tid_mem_monitor, NULL);

  return 0;
}

