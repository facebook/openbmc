/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <float.h>
#include <time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <openbmc/edb.h>
#include "obmc-pal.h"
#include "obmc-sensor.h"

#ifdef DBUS_SENSOR_SVC
#include <openbmc/sensor-svc-client.h>
#endif

#ifdef DEBUG
#define DEBUG_STR(...) syslog(LOG_WARNING, __VA_ARGS__)
#else
#define DEBUG_STR(...)
#endif

#define MAX_DATA_NUM    2000
/* Store enough coarse data for 30 days, configurable */
#define MAX_COARSE_DATA_NUM (30 * 24)
/* Any history more than an hour, loses its granularity and
 * it starts to get accounted in the COARSE grained calculations */
#define COARSE_THRESHOLD ((double)3600)

#define CACHE_READ_RETRY 5

typedef struct {
  long log_time;
  float value;
} sensor_data_t;

typedef struct {
  int index;
  sensor_data_t data[MAX_DATA_NUM];
} sensor_shm_t;

typedef struct {
  long log_time;
  float sum;
  float count;
  float avg;
  float max;
  float min;
} sensor_coarse_data_t;

typedef struct {
  int index;
  sensor_coarse_data_t data[MAX_COARSE_DATA_NUM];
} sensor_coarse_shm_t;

static int
sensor_key_get(uint8_t fru, uint8_t sensor_num, char *key)
{
  char fruname[32];

  if (fru == AGGREGATE_SENSOR_FRU_ID) {
    strcpy(fruname, AGGREGATE_SENSOR_FRU_NAME);
  } else {
    if (pal_get_fru_name(fru, fruname))
      return -1;
  }
  sprintf(key, "%s_sensor%d", fruname, sensor_num);
  return 0;
}

static int
sensor_coarse_key_get(uint8_t fru, uint8_t sensor_num, char *key)
{
  int ret = sensor_key_get(fru, sensor_num, key);
  if (ret) {
    return ret;
  }
  strcat(key, "_coarse");
  return 0;
}

static int
cache_set_coarse_history(char *key, float value) {
  int fd;
  int share_size = sizeof(sensor_coarse_shm_t);
  void *ptr;
  sensor_coarse_shm_t *snr_shm;
  long current_time;
  int ret = ERR_FAILURE;

  fd = shm_open(key, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    DEBUG_STR("%s: shm_open %s failed, errno = %d", __FUNCTION__, key, errno);
    return -1;
  }

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ftruncate(fd, share_size);

  ptr = mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    syslog(LOG_INFO, "%s: mmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }

  snr_shm = (sensor_coarse_shm_t *)ptr;
  current_time = time(NULL);
  if (snr_shm->data[snr_shm->index].log_time == 0) {
    sensor_coarse_data_t *s = &snr_shm->data[snr_shm->index];
    s->log_time = current_time;
    s->avg = s->sum = s->max = s->min = value;
    s->count = 1;
  } else {
    sensor_coarse_data_t *s = &snr_shm->data[snr_shm->index];
    /* If the log was started less than an hour ago, then 
     * continue to log to this entry */
    if (difftime(current_time, s->log_time) < COARSE_THRESHOLD) {
      s->sum += value;
      s->count += 1;
      if (value > s->max)
        s->max = value;
      if (value < s->min)
        s->min = value;
      s->avg = s->sum / s->count;
    } else {
      /* Start logging to the next entry */
      snr_shm->index = (snr_shm->index + 1) % MAX_COARSE_DATA_NUM;
      s = &snr_shm->data[snr_shm->index];
      memset(s, 0, sizeof(*s));
      s->log_time = current_time;
      s->avg = s->sum = s->max = s->min = value;
      s->count = 1;
    }
  }

  if (munmap(ptr, share_size) != 0) {
    syslog(LOG_INFO, "%s: munmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }
  ret = 0;

unlock_bail:
  if (flock(fd, LOCK_UN) < 0) {
    syslog(LOG_INFO, "%s: file-unlock %s failed errno = %d\n", __FUNCTION__, key, errno);
    ret = -1;
  }
close_bail:
  close(fd);
  return ret;
}

static int
cache_set_history(char *key, float value) {

  int fd;
  int share_size = sizeof(sensor_shm_t);
  void *ptr;
  sensor_shm_t *snr_shm;
  int ret = ERR_FAILURE;

  fd = shm_open(key, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    DEBUG_STR("%s: shm_open %s failed, errno = %d", __FUNCTION__, key, errno);
    return -1;
  }

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ftruncate(fd, share_size);

  ptr = mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    syslog(LOG_INFO, "%s: mmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }

  snr_shm = (sensor_shm_t *)ptr;
  snr_shm->data[snr_shm->index].log_time = time(NULL);
  snr_shm->data[snr_shm->index].value = value;
  snr_shm->index = (snr_shm->index + 1) % MAX_DATA_NUM;

  if (munmap(ptr, share_size) != 0) {
    syslog(LOG_INFO, "%s: munmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }
  ret = 0;

unlock_bail:
  if (flock(fd, LOCK_UN) < 0) {
    syslog(LOG_INFO, "%s: file-unlock %s failed errno = %d\n", __FUNCTION__, key, errno);
    ret = -1;
  }
close_bail:
  close(fd);
  return ret;
}

int __attribute__((weak))
sensor_cache_read(uint8_t fru, uint8_t sensor_num, float *value)
{
#ifndef DBUS_SENSOR_SVC
  int ret;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN];
  int retry = 0;

  pal_sensor_check(fru, sensor_num);

  if (sensor_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;
  for (retry = 0; retry < CACHE_READ_RETRY; retry++) {
    if (!(ret = edb_cache_get(key, str))) {
      break;
    }
  }
  if (ret < 0) {
    DEBUG_STR("sensor_cache_read: cache_get %s failed.\n", key);
    return ERR_SENSOR_NA;
  }
  if (0 == strcmp(str, "NA")) {
    return ERR_SENSOR_NA;
  }

  *((float*)value) = atof(str);
  return 0;
#else
  return sensor_svc_read(fru, sensor_num, value);
#endif
}

int
sensor_cache_write(uint8_t fru, uint8_t sensor_num, bool available, float value)
{
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN];
  int ret;

  if (sensor_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;

  if (available)
    sprintf(str, "%.2f", value);
  else
    strcpy(str, "NA");

  ret = edb_cache_set(key, str);
  if (ret) {
    DEBUG_STR("sensor_cache_write: cache_set %s failed.\n", key);
    return ERR_FAILURE;
  }
  if (available) {
    cache_set_history(key, value);
    if (sensor_coarse_key_get(fru, sensor_num, key) == 0) {
      cache_set_coarse_history(key, value);
    }
  }
  return 0;
}

int sensor_raw_read(uint8_t fru, uint8_t sensor_num, float *value)
{
#ifdef DBUS_SENSOR_SVC
  int ret = sensor_svc_raw_read(fru, sensor_num, value);
#else
  int ret = pal_sensor_read_raw(fru, sensor_num, value);
#endif

  if (!ret)
    sensor_cache_write(fru, sensor_num, true, *value);
  else if (ret == ERR_SENSOR_NA)
    sensor_cache_write(fru, sensor_num, false, 0.0);
  return ret;
}

static int
sensor_read_short_history(uint8_t fru, uint8_t sensor_num, float *min,
    float *average, float *max, int start_time)
{
  char key[MAX_KEY_LEN] = {0};
  int fd;
  int share_size = sizeof(sensor_shm_t);
  void *ptr;
  sensor_shm_t *snr_shm;
  int16_t read_index;
  uint16_t count = 0;
  float read_val;
  double total = 0;
  int ret = ERR_FAILURE;

  if (sensor_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;

  fd = shm_open(key, O_RDONLY, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    DEBUG_STR("%s: shm_open %s failed, errno = %d", __FUNCTION__, key, errno);
    return ERR_FAILURE;
  }

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ptr = mmap(NULL, share_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    syslog(LOG_INFO, "%s: mmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }

  snr_shm = (sensor_shm_t *)ptr;
  read_index = snr_shm->index - 1;
  if (read_index < 0) {
    read_index += MAX_DATA_NUM;
  }

  read_val = snr_shm->data[read_index].value;
  *min = read_val;
  *max = read_val;

  while ((snr_shm->data[read_index].log_time >= start_time) && (count < MAX_DATA_NUM)) {
    read_val = snr_shm->data[read_index].value;
    if (read_val > *max)
      *max = read_val;
    if (read_val < *min)
      *min = read_val;

    total += read_val;
    count++;
    if ((--read_index) < 0) {
      read_index += MAX_DATA_NUM;
    }
  }

  if (munmap(ptr, share_size) != 0) {
    syslog(LOG_INFO, "%s: munmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }

  /* If none found in history, just return the cached value */
  if (!count) {
    float read_value;
    ret = sensor_cache_read(fru, sensor_num, &read_value);
    if (ret)
      return ret;
    total = *min = *max = read_val;
    count = 1;
  }

  *average = total / count;
  ret = 0;
unlock_bail:
  if (flock(fd, LOCK_UN) < 0) {
    syslog(LOG_INFO, "%s: file-unlock %s failed errno = %d\n", __FUNCTION__, key, errno);
    ret = -1;
  }
close_bail:
  close(fd);
  return ret;
}

static int
sensor_read_long_history(uint8_t fru, uint8_t sensor_num, float *min,
    float *average, float *max, int start_time)
{
  char key[MAX_KEY_LEN] = {0};
  int fd;
  int share_size = sizeof(sensor_coarse_shm_t);
  void *ptr;
  sensor_coarse_shm_t *snr_shm;
  sensor_coarse_data_t *s;
  int16_t read_index;
  uint16_t count = 0;
  float read_val;
  double total = 0;
  int ret = ERR_FAILURE;

  if (sensor_coarse_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;

  fd = shm_open(key, O_RDONLY, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    DEBUG_STR("%s: shm_open %s failed, errno = %d", __FUNCTION__, key, errno);
    return ERR_FAILURE;
  }

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ptr = mmap(NULL, share_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    syslog(LOG_INFO, "%s: mmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }

  snr_shm = (sensor_coarse_shm_t *)ptr;
  read_index = snr_shm->index;
  total = 0;
  *max = -FLT_MAX;
  *min = FLT_MAX;
  while (count < MAX_COARSE_DATA_NUM) {
    s = &snr_shm->data[read_index];
    if (s->log_time < start_time) {
      break;
    }
    if (s->max > *max)
      *max = s->max;
    if (s->min < *min)
      *min = s->min;
    total += s->avg;
    count++;
    if ((--read_index) < 0) {
      read_index += MAX_COARSE_DATA_NUM;
    }
  }
  if (munmap(ptr, share_size) != 0) {
    syslog(LOG_INFO, "%s: munmap %s failed, errno = %d", __FUNCTION__, key, errno);
    goto unlock_bail;
  }

  /* If none found in history, just return the cached value */
  if (!count) {
    float read_value;
    ret = sensor_cache_read(fru, sensor_num, &read_value);
    if (ret)
      return ret;
    total = *min = *max = read_val;
    count = 1;
  }

  *average = total / count;
  ret = 0;
unlock_bail:
  if (flock(fd, LOCK_UN) < 0) {
    syslog(LOG_INFO, "%s: file-unlock %s failed errno = %d\n", __FUNCTION__, key, errno);
    ret = -1;
  }
close_bail:
  close(fd);
  return ret;
}

int
sensor_read_history(uint8_t fru, uint8_t sensor_num, float *min, float *average, float *max, int start_time)
{
  long current_time = time(NULL);
  
  /* If requested start is greater than the coarse threshold (mostly an hour),
   * then go through the coarse stats to compute the max,min avg. else use the
   * fine grained data to get the values */
  if (difftime(current_time, start_time) > COARSE_THRESHOLD) {
    return sensor_read_long_history(fru, sensor_num, min, average, max, start_time);
  }
  return sensor_read_short_history(fru, sensor_num, min, average, max, start_time);
}

static int sensor_clear_history_helper(char *shm_name, int share_size)
{
  int fd;
  char key[MAX_KEY_LEN] = {0};
  long *shm;
  int ret = ERR_FAILURE;

  fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    return ERR_FAILURE;
  }
  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, shm_name, errno);
    goto close_bail;
  }

  ftruncate(fd, share_size);

  shm = (long *)mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm == MAP_FAILED) {
    syslog(LOG_INFO, "%s: mmap %s failed, errno = %d", __FUNCTION__, shm_name, errno);
    goto unlock_bail;
  }

  memset(shm, 0, share_size);
  if (munmap(shm, share_size) != 0) {
    syslog(LOG_INFO, "%s: munmap %s failed, errno = %d", __FUNCTION__, shm_name, errno);
    goto unlock_bail;
  }
  ret = 0;
unlock_bail:
  if (flock(fd, LOCK_UN) < 0) {
    syslog(LOG_INFO, "%s: file-unlock %s failed errno = %d\n", __FUNCTION__, shm_name, errno);
    ret = -1;
  }
close_bail:
  close(fd);
  return ret;
}

int sensor_clear_history(uint8_t fru, uint8_t sensor_num)
{
  int share_size = sizeof(sensor_shm_t);
  int fd;
  char key[MAX_KEY_LEN] = {0};
  int ret1, ret2;

  if (sensor_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;

  ret1 = sensor_clear_history_helper(key, sizeof(sensor_shm_t));
  if (ret1) {
    syslog(LOG_INFO, "Clearing history failed: %d\n", ret1);
  }
  if (sensor_coarse_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;
  ret2 = sensor_clear_history_helper(key, sizeof(sensor_coarse_shm_t));
  if (ret2) {
    syslog(LOG_INFO, "Clearing coarse history failed: %d\n", ret2);
  }
  return ret1 || ret2 ? ERR_FAILURE : 0;
}
