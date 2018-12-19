/*
 * yosemite-sensors
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
#include <unistd.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-sensor.h>
#include <openbmc/aggregate-sensor.h>

#define STATUS_OK   "ok"
#define STATUS_NS   "ns"
#define STATUS_UNC  "unc"
#define STATUS_UCR  "ucr"
#define STATUS_UNR  "unr"
#define STATUS_LNC  "lnc"
#define STATUS_LCR  "lcr"
#define STATUS_LNR  "lnr"

#define SENSOR_ALL             -1
#ifdef CUSTOM_FRU_LIST
  static const char * pal_fru_list_sensor_history_t =  pal_fru_list_sensor_history;
#else
  static const char * pal_fru_list_sensor_history_t =  pal_fru_list;
#endif /* CUSTOM_FRU_LIST */

static pthread_mutex_t timer = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done = PTHREAD_COND_INITIALIZER;
static pthread_condattr_t done_attr;
static bool done_flag = false;

// This is for get_sensor_reading
typedef struct {
  uint8_t fru;
  int sensor_cnt;
  int sensor_num;
  bool threshold;
  bool force;
  uint8_t sensor_list[];
} get_sensor_reading_struct;

static void
print_usage() {
  printf("Usage: sensor-util [fru] <sensor num> <option> ..\n");
  printf("       sensor-util [fru] <option> ..\n\n");
  printf("       [fru]           : %s\n", pal_fru_list);
  printf("       [historical fru]: %s\n", pal_fru_list_sensor_history_t);
  printf("       Use \"aggregate\" as the [fru] to print just aggregated sensors defined for this platform\n");
  printf("       <sensor num>: 0xXX (Omit [sensor num] means all sensors.)\n");
  printf("       <option>:\n");
  printf("         --threshold               show all thresholds\n");
  printf("         --history <period>        show max, min and average values of last <period> seconds\n");
  printf("         --history <period>[m/h/d] show max, min and average values of last <period> minutes/hours/days\n");
  printf("              example --history 4d means history of 4 days\n");
  printf("         --history-clear           clear history values\n");
  printf("         --force                   read the sensor directly from the h/w (not cache).Ensure sensord is killed before executing this command\n");
}

static int convert_period(char *str, long *val) {
  char *endptr = NULL;
  long ret;
  int rc = 0;

  errno=0;
  ret = strtol(str, &endptr, 0);
  if ( ret < 0 ) {
    return -1;
  }
  if ( errno != 0 ) {
	  perror("Error");
	  return -1;
  }
  if ( strlen(endptr) > 1 )
	  return -1;

  switch(*endptr) {
    case 'd':
    case 'D':
      *val = ret * 24 * 3600;
      break;
    case 'h':
    case 'H':
      *val = ret * 3600;
      break;
    case 'm':
    case 'M':
      *val = ret * 60;
      break;
    case 's':
    case 'S':
    case '\0':
      *val = ret;
      break;
    default:
      rc = -1;
      break;
  }
  return rc;
}

static void
print_sensor_reading(float fvalue, uint16_t snr_num, thresh_sensor_t *thresh,
    bool threshold, char *status) {

  printf("%-28s (0x%X) : %7.2f %-5s | (%s)",
      thresh->name, snr_num, fvalue, thresh->units, status);
  if (threshold) {

    printf(" | UCR: ");
    thresh->flag & GETMASK(UCR_THRESH) ?
      printf("%.2f", thresh->ucr_thresh) : printf("NA");

    printf(" | UNC: ");
    thresh->flag & GETMASK(UNC_THRESH) ?
      printf("%.2f", thresh->unc_thresh) : printf("NA");

    printf(" | UNR: ");
    thresh->flag & GETMASK(UNR_THRESH) ?
      printf("%.2f", thresh->unr_thresh) : printf("NA");

    printf(" | LCR: ");
    thresh->flag & GETMASK(LCR_THRESH) ?
      printf("%.2f", thresh->lcr_thresh) : printf("NA");

    printf(" | LNC: ");
    thresh->flag & GETMASK(LNC_THRESH) ?
      printf("%.2f", thresh->lnc_thresh) : printf("NA");

    printf(" | LNR: ");
    thresh->flag & GETMASK(LNR_THRESH) ?
      printf("%.2f", thresh->lnr_thresh) : printf("NA");

  }

  printf("\n");
}

static void
get_sensor_status(float fvalue, thresh_sensor_t *thresh, char *status) {

  if (thresh->flag == 0)
    sprintf(status, STATUS_NS);
  else
    sprintf(status, STATUS_OK);

  if (GETBIT(thresh->flag, UNC_THRESH) && (FORMAT_CONV(fvalue) >= FORMAT_CONV(thresh->unc_thresh)))
    sprintf(status, STATUS_UNC);
  if (GETBIT(thresh->flag, UCR_THRESH) && (FORMAT_CONV(fvalue) >= FORMAT_CONV(thresh->ucr_thresh)))
    sprintf(status, STATUS_UCR);
  if (GETBIT(thresh->flag, UNR_THRESH) && (FORMAT_CONV(fvalue) >= FORMAT_CONV(thresh->unr_thresh)))
    sprintf(status, STATUS_UNR);
  if (GETBIT(thresh->flag, LNC_THRESH) && (FORMAT_CONV(fvalue) <= FORMAT_CONV(thresh->lnc_thresh)))
    sprintf(status, STATUS_LNC);
  if (GETBIT(thresh->flag, LCR_THRESH) && (FORMAT_CONV(fvalue) <= FORMAT_CONV(thresh->lcr_thresh)))
    sprintf(status, STATUS_LCR);
  if (GETBIT(thresh->flag, LNR_THRESH) && (FORMAT_CONV(fvalue) <= FORMAT_CONV(thresh->lnr_thresh)))
    sprintf(status, STATUS_LNR);
}

static void*
get_sensor_reading(void *sensor_data) {

  get_sensor_reading_struct *sensor_info = sensor_data;
  int i;
  uint8_t snr_num;
  float fvalue;
  char status[8];
  thresh_sensor_t thresh;
  int ret = 0;
  char fruname[32] = {0};

  pthread_detach(pthread_self());
  //Allow this thread to be killed at any time
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  for (i = 0; i < sensor_info->sensor_cnt; i++) {
    snr_num = sensor_info->sensor_list[i];
    /* If calculation is for a single sensor, ignore all others. */
    if (sensor_info->sensor_num != SENSOR_ALL && snr_num != sensor_info->sensor_num) {
      continue;
    }

    if (sensor_info->fru == AGGREGATE_SENSOR_FRU_ID) {
      if (aggregate_sensor_threshold(snr_num, &thresh)) {
        syslog(LOG_ERR, "agg_snr_thresh failed for agg num: 0x%X", snr_num);
        continue;
      }
    } else {
      ret = sdr_get_snr_thresh(sensor_info->fru, snr_num, &thresh);
      pal_alter_sensor_thresh_flag(sensor_info->fru, snr_num, &(thresh.flag));
      if (ret == ERR_NOT_READY) {
        pal_get_fru_name(sensor_info->fru, fruname);
        printf("%s SDR is missing!\n", fruname);

        //modify the flag to true if we get the sensor reading
        done_flag = true;

        //Tell caller it's done
        pthread_cond_signal(&done);

        return NULL;
      }
      else if (ret < 0) {
        syslog(LOG_ERR, "sdr_get_snr_thresh failed for FRU %d num: 0x%X", sensor_info->fru, snr_num);
        continue;
      }
    }

    usleep(50);
    if ((false == pal_sensor_is_cached(sensor_info->fru, snr_num)) || (true == sensor_info->force))
      ret = sensor_raw_read(sensor_info->fru, snr_num, &fvalue);
    else
      ret = sensor_cache_read(sensor_info->fru, snr_num, &fvalue);

    if (ret < 0) {
      printf("%-28s (0x%X) : NA | (na)\n", thresh.name, sensor_info->sensor_list[i]);
      continue;
    }
    else {
      get_sensor_status(fvalue, &thresh, status);
      print_sensor_reading(fvalue, (uint16_t)snr_num, &thresh, sensor_info->threshold, status);
    }
  }

  //if the mutex is already locked, the calling thread shall block until the mutex becomes available
  //wait for the mutex is released and then get the sensor reading
  pthread_mutex_lock(&timer);

  //modify the flag to true if we get the sensor reading
  done_flag = true;

  //Tell caller it's done
  pthread_cond_signal(&done);

  //release the mutex
  pthread_mutex_unlock(&timer);

  pthread_exit(0);
}

static void
get_sensor_history(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, int num, int period) {

  int start_time, i;
  uint8_t snr_num;
  float min, average, max;
  thresh_sensor_t thresh;
  int ret = 0;
  char fruname[32] = {0};

  start_time = time(NULL) - period;

  for (i = 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];
    if (num != SENSOR_ALL && snr_num != num) {
      continue;
    }

    if (fru == AGGREGATE_SENSOR_FRU_ID) {
      if (aggregate_sensor_threshold(snr_num, &thresh)) {
        syslog(LOG_ERR, "agg_snr_threshold failed 0x%x", snr_num);
        continue;
      }
    } else {
      ret = sdr_get_snr_thresh(fru, snr_num, &thresh);
      if (ret == ERR_NOT_READY) {
        pal_get_fru_name(fru, fruname);
        printf("%s SDR is missing!\n", fruname);
        return;
      }
      else if (ret < 0) {
        syslog(LOG_ERR, "sdr_get_snr_thresh failed for FRU %d num: 0x%X", fru, snr_num);
        continue;
      }
    }

    if (sensor_read_history(fru, snr_num, &min, &average, &max, start_time) < 0) {
      printf("%-18s (0x%X) min = NA, average = NA, max = NA\n", thresh.name, snr_num);
      continue;
    }

    printf("%-18s (0x%X) min = %.2f, average = %.2f, max = %.2f\n", thresh.name, snr_num, min, average, max);
  }
}

static void clear_sensor_history(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, int num) {
  int i;
  uint8_t snr_num;

  for (i = 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];
    if (num != SENSOR_ALL && snr_num != num) {
      continue;
    }
    if (sensor_clear_history(fru, snr_num)) {
      printf("Clearing fru:%u sensor[%u] failed!\n", fru, snr_num);
    }
  }
}

void get_sensor_reading_timer(struct timespec *timeout, get_sensor_reading_struct *sensor_data)
{
  struct timespec abs_time;
  pthread_t tid_get_sensor_reading;
  int err = 0;
  int thread_alive = 0;
  char fruname[32] = {0};

  err = pthread_condattr_init(&done_attr);
  if ( err != 0 )
  {
    syslog(LOG_WARNING, "[%s]init condattr fails. errno:%d", __func__, err);
    return;
  }

  err = pthread_condattr_setclock(&done_attr, CLOCK_MONOTONIC);
  if ( err != 0 )
  {
    syslog(LOG_WARNING, "[%s]set condattr clock fails. errno:%d", __func__, err);
    return;
  }

  err = pthread_cond_init(&done, &done_attr);
  if ( err != 0 )
  {
    syslog(LOG_WARNING, "[%s]init cond fails. errno:%d", __func__, err);
    return;
  }

  pthread_mutex_lock(&timer);

  //Assign the timeout time to abs_time
  clock_gettime(CLOCK_MONOTONIC, &abs_time);
  abs_time.tv_sec += timeout->tv_sec;
  abs_time.tv_nsec += timeout->tv_nsec;

   pal_get_fru_name(sensor_data->fru, fruname);

  //Make get_sensor_reading a thread
  if (pthread_create(&tid_get_sensor_reading, NULL, get_sensor_reading, (void *) sensor_data) < 0) {
    syslog(LOG_WARNING, "sensor-util FRU:%s, pthread_create failed\n", fruname);
    pthread_mutex_unlock(&timer);
    return;
  }

  //Check if thread is alive, if pthread_kill (thread_id, sig = 0) returns 0, means thread is alive
  thread_alive = pthread_kill(tid_get_sensor_reading, 0);

  if (thread_alive == 0)
  {
    while ( false == done_flag )
    {
      //Timer thread, continue only when get_sensor_reading send the done signal or abs_time timed out
      err = pthread_cond_timedwait(&done, &timer, &abs_time);
      if (err == ETIMEDOUT) {
        break;
      }
    }

    //Double Check if thread is still alive
    thread_alive = pthread_kill(tid_get_sensor_reading, 0);

    //Timeout actions
    //If Timed out, and Thread is still alive, it means the thread is really timed out, cancel the thread
    //If Timed out, and Thread is dead, it means the timed out is fake, do nothing
    if ( (err == ETIMEDOUT) && (thread_alive == 0) ) {
      printf("FRU:%s timed out...\n", fruname);
      pthread_cancel(tid_get_sensor_reading);
    }
  }

  //re-init for the next fru
  done_flag = false;

  pthread_mutex_unlock(&timer);

  return;
}

static int
print_sensor(uint8_t fru, int sensor_num, bool history, bool threshold, bool force, bool history_clear, long period) {
  int ret;
  uint8_t status;
  int sensor_cnt;
  uint8_t *sensor_list;
  char fruname[16] = {0};
  struct timespec timeout;
  get_sensor_reading_struct data;

  //Setup timeout for each fru get_sensor_reading
  memset(&timeout, 0, sizeof(timeout));
  timeout.tv_sec = pal_get_sensor_util_timeout(fru);

  if (fru == AGGREGATE_SENSOR_FRU_ID) {
    size_t cnt, i;
    if (aggregate_sensor_init(NULL)) {
      return -1;
    }
    if (aggregate_sensor_count(&cnt)) {
      return 0;
    }
    strcpy(fruname, AGGREGATE_SENSOR_FRU_NAME);
    sensor_list = malloc(sizeof(uint8_t) * cnt);
    if (!sensor_list) {
      return -1;
    }
    for (i = 0; i < cnt; i++) {
      sensor_list[i] = i;
    }
    sensor_cnt = (int)cnt;
  } else {
    if (pal_get_fru_name(fru, fruname)) {
      sprintf(fruname, "fru%d", fru);
    }
    ret = pal_is_fru_prsnt(fru, &status);
    if (ret < 0) {
      printf("pal_is_fru_prsnt failed for fru: %s\n", fruname);
      return ret;
    }
    if (status == 0) {
      printf("%s is not present!\n\n", fruname);
      return -1;
    }

    ret = pal_is_fru_ready(fru, &status);
    if ((ret < 0) || (status == 0)) {
      printf("%s is unavailable!\n\n", fruname);
      return ret;
    }

    ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
    if (ret < 0) {
      printf("%s get sensor list failed!\n", fruname);
      return ret;
    }
  }

  if (sensor_cnt == 0) {
    return 0;
  }

  if (history_clear) {
    clear_sensor_history(fru, sensor_list, sensor_cnt, sensor_num);
  } else if (history) {
    get_sensor_history(fru, sensor_list, sensor_cnt, sensor_num, period);
  } else {
    data.fru = fru;
    data.sensor_cnt = sensor_cnt;
    data.sensor_num = sensor_num;
    data.threshold = threshold;
    data.force = force;
    memcpy(data.sensor_list, sensor_list, sizeof(uint8_t)*sensor_cnt);
    get_sensor_reading_timer(&timeout, &data);
  }

  //Print Empty Line to separate frus,
  //only when sensor_cnt greater than 0, not history-clear, and sensor_num is not specified
  if ( (sensor_cnt > 0) && (!history_clear) && (sensor_num == SENSOR_ALL) ){
    printf("\n");
  }

  return 0;
}

int parse_args(int argc, char *argv[], char *fruname,
    bool *history_clear, bool *history, bool *threshold, bool *force, long *period, int *snr)
{
  int ret;
  int num;
  char *end;
  int index;
  static struct option long_opts[] = {
    {"history-clear", no_argument, 0, 'c'},
    {"history", required_argument, 0, 'h'},
    {"threshold", no_argument,     0, 't'},
    {"force", no_argument, 0, 'f'},
    {0,0,0,0},
  };

  /* Set defaults */
  *history_clear = false;
  *history = false;
  *threshold = false;
  *force = false;
  *period = 60;
  *snr = -1;

  while(-1 != (ret = getopt_long(argc, argv, "ch:t", long_opts, &index))) {
    switch(ret) {
      case 'f':
        *force = true;
        break;
      case 'c':
        *history_clear = true;
        break;
      case 't':
        *threshold = true;
        break;
      case 'h':
        *history = true;
        if (convert_period(optarg, period)) {
          return -1;
        }
        if (*period > (MAX_COARSE_DATA_NUM * COARSE_THRESHOLD)) {
          *period = MAX_COARSE_DATA_NUM * COARSE_THRESHOLD;
          if (!(*period % 86400)) {
            printf("max period: %ldd\n", (*period / 86400));
          } else {
            printf("max period: %lds\n", *period);
          }
          return -1;
        }
        break;
      default:
        return -1;
    }
  }
  if (argc < optind + 1) {
    return -1;
  }
  strcpy(fruname, argv[optind]);

  if (argc > optind + 1) {
    *snr = strtol(argv[optind + 1], &end, 0);
    if (!end || *end != '\0') {
      printf("Invalid sensor: %s\n", argv[optind + 1]);
      return -1;
    }
  }
  /* Only one of these flags should be on at
   * any time */
  num = (int)*threshold + (int)*history_clear + (int)*history;
  if (num > 1) {
    return -1;
  }

  return 0;
}

int
main(int argc, char **argv) {

  int ret = 0;
  uint8_t fru;
  int num;
  bool threshold;
  bool history;
  bool history_clear;
  bool force;
  long period;
  char fruname[32];

  if (parse_args(argc, argv, fruname,
        &history_clear, &history,
        &threshold, &force, &period, &num)) {
    print_usage();
    exit(-1);
  }

  if (!strcmp(fruname, AGGREGATE_SENSOR_FRU_NAME)) {
    fru = AGGREGATE_SENSOR_FRU_ID;
  } else {
    ret = pal_get_fru_id(fruname, &fru);
    if (ret < 0) {
      print_usage();
      return ret;
    }
    if (history_clear || history) {
      //Check if the input FRU is exist in sensor history list
      if (NULL == strstr(pal_fru_list_sensor_history_t, fruname)) {
        print_usage();
        exit(-1);
      }
    }
  }

  if (fru == 0) {
    for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {
      ret |= print_sensor(fru, num, history, threshold, force, history_clear, period);
    }
    ret |= print_sensor(AGGREGATE_SENSOR_FRU_ID, num, history, threshold, false, history_clear, period);
  } else {
    ret = print_sensor(fru, num, history, threshold, fru == AGGREGATE_SENSOR_FRU_ID ? false : force, history_clear, period);
  }
  return ret;
}
