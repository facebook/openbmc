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
#include <pthread.h>
#include <jansson.h>
#include <sys/sysinfo.h>
#include <sys/reboot.h>
#include <openbmc/pal.h>
#include "watchdog.h"

#define MIN_THRESHOLD          60.0
#define MAX_THRESHOLD          90.0


#define DEFAULT_WINDOW_SIZE 120
#define DEFAULT_MONITOR_INTERVAL 1
#define HEALTHD_MAX_RETRY 10
#define CONFIG_PATH "/etc/healthd-config.json"

struct threshold_s {
  float value;
  float hysteresis;
  bool asserted;
  bool log;
  int log_level;
  bool reboot;
  bool bmc_error_trigger;
  bool assertion_logged;
};

#define BMC_HEALTH_FILE "bmc_health"
#define HEALTH "1"
#define NOT_HEALTH "0"

enum ASSERT_BIT {
  BIT_CPU_OVER_THRESHOLD = 0,
  BIT_MEM_OVER_THRESHOLD = 1,
  BIT_RECOVERABLE_ECC    = 2,
  BIT_UNRECOVERABLE_ECC  = 3,
};



/* Memory monitor enabled */
static char *mem_monitor_name = "BMC Memory utilization";
static bool mem_monitor_enabled = false;
static unsigned int mem_window_size = DEFAULT_WINDOW_SIZE;
static unsigned int mem_monitor_interval = DEFAULT_MONITOR_INTERVAL;
static struct threshold_s *mem_threshold;
static size_t mem_threshold_num = 0;

static pthread_mutex_t global_error_mutex = PTHREAD_MUTEX_INITIALIZER;
static int bmc_health = 0; // CPU/MEM/ECC error flag


static void
initialize_threshold(const char *target, json_t *thres, struct threshold_s *t) {
  json_t *tmp;
  size_t i;
  size_t act_size;

  tmp = json_object_get(thres, "value");
  if (!tmp || !json_is_real(tmp)) {
    return;
  }
  t->value = json_real_value(tmp);
  t->assertion_logged = false;
  /* Do not let the value (CPU/MEM thresholds) exceed these safe ranges */
  if ((strcasestr(target, "CPU") != 0ULL) ||
      (strcasestr(target, "MEM") != 0ULL)) {
    if (t->value > MAX_THRESHOLD) {
      syslog(LOG_WARNING,
             "%s: user setting %s threshold %f is too high and set threshold as %f",
             __func__, target, t->value, MAX_THRESHOLD);
      t->value = MAX_THRESHOLD;
    }
    if (t->value < MIN_THRESHOLD) {
      syslog(LOG_WARNING,
             "%s: user setting %s threshold %f is too low and set threshold as %f",
             __func__, target, t->value, MIN_THRESHOLD);
      t->value = MIN_THRESHOLD;
    }
  }

  tmp = json_object_get(thres, "hysteresis");
  if (tmp && json_is_real(tmp)) {
    t->hysteresis = json_real_value(tmp);
  }
  tmp = json_object_get(thres, "action");
  if (!tmp || !json_is_array(tmp)) {
    return;
  }
  act_size = json_array_size(tmp);
  for(i = 0; i < act_size; i++) {
    const char *act;
    json_t *act_o = json_array_get(tmp, i);
    if (!act_o || !json_is_string(act_o)) {
      continue;
    }
    act = json_string_value(act_o);
    if (!strcmp(act, "log-warning")) {
      t->log_level = LOG_WARNING;
      t->log = true;
    } else if(!strcmp(act, "log-critical")) {
      t->log_level = LOG_CRIT;
      t->log = true;
    } else if (!strcmp(act, "reboot")) {
      t->reboot = true;
    }
#if 0   // bmc-error-trigger not supported
    else if(!strcmp(act, "bmc-error-trigger")) {
      t->bmc_error_trigger = true;
    }
#endif
  }

  if (tmp && json_is_boolean(tmp)) {
    t->reboot = json_is_true(tmp);
  }
}



static void initialize_thresholds(const char *target, json_t *array, struct threshold_s **out_arr, size_t *out_len) {
  size_t size = json_array_size(array);
  size_t i;
  struct threshold_s *thres;

  if (size == 0) {
    return;
  }
  thres = *out_arr = calloc(size, sizeof(struct threshold_s));
  if (!thres) {
    return;
  }
  *out_len = size;

  for(i = 0; i < size; i++) {
    json_t *e = json_array_get(array, i);
    if (!e) {
      continue;
    }
    initialize_threshold(target, e, &thres[i]);
  }
}


static void
initialize_mem_config(json_t *conf) {
  json_t *tmp;

  tmp = json_object_get(conf, "enabled");
  if (!tmp || !json_is_boolean(tmp)) {
    return;
  }
  mem_monitor_enabled = json_is_true(tmp);
  if (!mem_monitor_enabled) {
    return;
  }
  tmp = json_object_get(conf, "window_size");
  if (tmp && json_is_number(tmp)) {
    mem_window_size = json_integer_value(tmp);
  }
  tmp = json_object_get(conf, "monitor_interval");
  if (tmp && json_is_number(tmp)) {
    mem_monitor_interval = json_integer_value(tmp);
  }
  tmp = json_object_get(conf, "threshold");
  if (!tmp || !json_is_array(tmp)) {
    mem_monitor_enabled = false;
    return;
  }
  initialize_thresholds(mem_monitor_name, tmp, &mem_threshold, &mem_threshold_num);
}


static int
initialize_configuration(void) {
  json_error_t error;
  json_t *conf;
  json_t *v;

  conf = json_load_file(CONFIG_PATH, 0, &error);
  if (!conf) {
    syslog(LOG_CRIT, "HEALTHD configuration load failed");
    return -1;
  }
  v = json_object_get(conf, "version");
  if (v && json_is_string(v)) {
    syslog(LOG_INFO, "Loaded configuration version: %s\n", json_string_value(v));
  }

  initialize_mem_config(json_object_get(conf, "bmc_mem_utilization"));

  json_decref(conf);

  return 0;
}

static void threshold_assert_check(const char *target, float value, struct threshold_s *thres) {
  if (!thres->asserted && value >= thres->value) {
    thres->asserted = true;
    if (thres->log && !thres->assertion_logged) {
      // ensure we only log a particular assertion once
      thres->assertion_logged = true;
      syslog(thres->log_level, "ASSERT: %s (%.2f%%) exceeds the threshold (%.2f%%).\n", target, value, thres->value);
    }
    if (thres->reboot) {
      sleep(1);
      reboot(RB_AUTOBOOT);
    }
#if 0  // bmc_error_trigger not supported
    if (thres->bmc_error_trigger) {
      pthread_mutex_lock(&global_error_mutex);
      if (!bmc_health) { // assert bmc_health key only when not yet set
        pal_set_key_value(BMC_HEALTH_FILE, NOT_HEALTH);
      }
      if (strcasestr(target, "CPU") != 0ULL) {
        bmc_health = SETBIT(bmc_health, BIT_CPU_OVER_THRESHOLD);
      } else if (strcasestr(target, "Mem") != 0ULL) {
        bmc_health = SETBIT(bmc_health, BIT_MEM_OVER_THRESHOLD);
      } else {
        pthread_mutex_unlock(&global_error_mutex);
        return;
      }
      pthread_mutex_unlock(&global_error_mutex);
      pal_bmc_err_enable(target);
    }
#endif
  }
}

static void threshold_deassert_check(const char *target, float value, struct threshold_s *thres) {
  if (thres->asserted && value < (thres->value - thres->hysteresis)) {
    thres->asserted = false;
    if (thres->log) {
      syslog(thres->log_level, "DEASSERT: %s (%.2f%%) is under the threshold (%.2f%%).\n", target, value, thres->value);
    }
#if 0  // bmc_error_trigger not supported
    if (thres->bmc_error_trigger) {
      pthread_mutex_lock(&global_error_mutex);
      if (strcasestr(target, "CPU") != 0ULL) {
        bmc_health = CLEARBIT(bmc_health, BIT_CPU_OVER_THRESHOLD);
      } else if (strcasestr(target, "Mem") != 0ULL) {
        bmc_health = CLEARBIT(bmc_health, BIT_MEM_OVER_THRESHOLD);
      } else {
        pthread_mutex_unlock(&global_error_mutex);
        return;
      }
      if (!bmc_health) { // deassert bmc_health key if no any error bit assertion
        pal_set_key_value(BMC_HEALTH_FILE, HEALTH);
      }
      pthread_mutex_unlock(&global_error_mutex);
      pal_bmc_err_disable(target);
    }
#endif
  }
}

static void
threshold_check(const char *target, float value, struct threshold_s *thresholds, size_t num) {
  size_t i;

  for(i = 0; i < num; i++) {
    threshold_assert_check(target, value, &thresholds[i]);
    threshold_deassert_check(target, value, &thresholds[i]);
  }
}


static void
initilize_all_kv() {
  pal_set_def_key_value();
}

static void *
hb_led_handler() {

  while(1) {
    /* Turn ON the HB Led*/
    pal_set_hb_led(1);
    msleep(500);

    /* Turn OFF the HB led */
    pal_set_hb_led(0);
    msleep(500);
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


static void *
memory_usage_monitor() {
  struct sysinfo s_info;
  int i, error, timer = 0, ready_flag = 0, retry = 0;
  float mem_util_avg, mem_util_total;
  float mem_utilization[mem_window_size];

  memset(mem_utilization, 0, sizeof(float) * mem_window_size);

  while (1) {

    if (retry > HEALTHD_MAX_RETRY) {
      syslog(LOG_CRIT, "Cannot get sysinfo. Stop the %s\n", __func__);
      return NULL;
    }

    timer %= mem_window_size;

    // Need more data to cacluate the avg. utilization. We average 60 records here.
    if (timer == (mem_window_size-1) && !ready_flag)
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
      for (i=0; i<mem_window_size; i++)
        mem_util_total += mem_utilization[i];

      mem_util_avg = (mem_util_total/mem_window_size) * 100.0;

      threshold_check(mem_monitor_name, mem_util_avg, mem_threshold, mem_threshold_num);
    }

    timer++;
    sleep(mem_monitor_interval);
  }
  return NULL;
}


int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  pthread_t tid_watchdog;
  pthread_t tid_hb_led;
  pthread_t tid_mem_monitor;

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

  initialize_configuration();

// For current platforms, we are using WDT from either fand or fscd
// TODO: keeping this code until we make healthd as central daemon that
//  monitors all the important daemons for the platforms.
  if (pthread_create(&tid_watchdog, NULL, watchdog_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for watchdog error\n");
    exit(1);
  }

  if (pthread_create(&tid_hb_led, NULL, hb_led_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for heartbeat error\n");
    exit(1);
  }

  if (mem_monitor_enabled) {
    if (pthread_create(&tid_mem_monitor, NULL, memory_usage_monitor, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for monitor memory usage\n");
      exit(1);
    }
  }


  pthread_join(tid_watchdog, NULL);

  pthread_join(tid_hb_led, NULL);

  if (mem_monitor_enabled) {
    pthread_join(tid_mem_monitor, NULL);
  }


  return 0;
}
