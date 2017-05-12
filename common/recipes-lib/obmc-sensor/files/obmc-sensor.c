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
#include <time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <openbmc/pal.h>
#include <openbmc/edb.h>
#include "obmc-sensor.h"

#ifdef DEBUG
#define DEBUG_STR(...) syslog(LOG_WARNING, __VA_ARGS__)
#else
#define DEBUG_STR(...)
#endif

#define MAX_DATA_NUM    2000

typedef struct {
  long log_time;
  float value;
} sensor_data_t;

typedef struct {
  int index;
  sensor_data_t data[MAX_DATA_NUM];
} sensor_shm_t;

static int
sensor_key_get(uint8_t fru, uint8_t sensor_num, char *key)
{
  char fruname[32];

  if (pal_get_fru_name(fru, fruname))
    return -1;
  sprintf(key, "%s_sensor%d", fruname, sensor_num);
  return 0;
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
    DEBUG_STR("cache_set_history: shm_open %s failed, errno = %d", key, errno);
    return -1;
  }

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ftruncate(fd, share_size);

  ptr = mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    syslog(LOG_INFO, "cache_set_history: mmap %s failed, errno = %d", key, errno);
    goto unlock_bail;
  }

  snr_shm = (sensor_shm_t *)ptr;
  snr_shm->data[snr_shm->index].log_time = time(NULL);
  snr_shm->data[snr_shm->index].value = value;
  snr_shm->index = (snr_shm->index + 1) % MAX_DATA_NUM;

  if (munmap(ptr, share_size) != 0) {
    syslog(LOG_INFO, "cache_set_history: munmap %s failed, errno = %d", key, errno);
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



int
sensor_cache_read(uint8_t fru, uint8_t sensor_num, float *value)
{
  int ret;

  ret = pal_sensor_read(fru, sensor_num, value);
  if (ret < 0) {
    DEBUG_STR("sensor_cache_read: cache_get %s failed.\n", key);
    return ERR_FAILURE;
  }
  return 0;
}

static int
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
  cache_set_history(key, value);
  return 0;
}

int sensor_raw_read(uint8_t fru, uint8_t sensor_num, float *value)
{
  int ret = pal_sensor_read_raw(fru, sensor_num, value);
  if (!ret)
    sensor_cache_write(fru, sensor_num, true, *value);
  else if (ret == ERR_SENSOR_NA)
    sensor_cache_write(fru, sensor_num, false, 0.0);
  return ret;
}

int
sensor_read_history(uint8_t fru, uint8_t sensor_num, float *min, float *average, float *max, int start_time)
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
    DEBUG_STR("cache_get_history: shm_open %s failed, errno = %d", key, errno);
    return ERR_FAILURE;
  }

  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ptr = mmap(NULL, share_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    syslog(LOG_INFO, "cache_get_history: mmap %s failed, errno = %d", key, errno);
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
    syslog(LOG_INFO, "cache_get_history: munmap %s failed, errno = %d", key, errno);
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

int sensor_clear_history(uint8_t fru, uint8_t sensor_num)
{
  sensor_shm_t *snr_shm;
  int share_size = sizeof(sensor_shm_t);
  int fd;
  char key[MAX_KEY_LEN] = {0};
  int ret = ERR_FAILURE;

  if (sensor_key_get(fru, sensor_num, key))
    return ERR_UNKNOWN_FRU;

  fd = shm_open(key, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    return ERR_FAILURE;
  }
  if (flock(fd, LOCK_EX) < 0) {
    syslog(LOG_INFO, "%s: file-lock %s failed errno = %d\n", __FUNCTION__, key, errno);
    goto close_bail;
  }

  ftruncate(fd, share_size);

  snr_shm = (sensor_shm_t *)mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (snr_shm == MAP_FAILED) {
    syslog(LOG_INFO, "cache_set_history: mmap %s failed, errno = %d", key, errno);
    goto unlock_bail;
  }

  memset(snr_shm, 0, share_size);
  if (munmap(snr_shm, share_size) != 0) {
    syslog(LOG_INFO, "cache_set_history: munmap %s failed, errno = %d", key, errno);
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
