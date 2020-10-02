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

#define MAX_DATA_NUM    2000

typedef struct {
  long log_time;
  float value;
} sensor_data_t;

typedef struct {
  int index;
  sensor_data_t data[MAX_DATA_NUM];
} sensor_shm_t;

int shm_print(const char *key)
{
	sensor_shm_t *snr_shm;
	void *ptr;
	int share_size = sizeof(sensor_shm_t);
	int i, idx;
	int fd = shm_open(key, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		printf("shm open failed");
		return -1;
	}
	if (flock(fd, LOCK_EX) < 0) {
		printf("lock failed\n");
		close(fd);
		return -1;
	}
	if (ftruncate(fd, share_size) != 0) {
		printf("truncate failed\n");
		flock(fd, LOCK_UN);
		close(fd);
		return -1;
	}
	ptr = mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		printf("map failed!\n");
		flock(fd, LOCK_UN);
		close(fd);
	}
	snr_shm = (sensor_shm_t *)ptr;
	for (i = 0; i < MAX_DATA_NUM; i++) {
		sensor_data_t *snr;
		idx = (snr_shm->index - i - 1);
		if (idx < 0)
			idx += MAX_DATA_NUM;
		snr = &snr_shm->data[idx];
		if (snr->log_time == 0)
			break;
		printf("%lu: %f\n", snr->log_time, snr->value);
	}
	munmap(ptr, share_size);
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}

void usage(const char *prog)
{
  printf("%s FRU_NAME SENSOR_ID\n", prog);
  printf(" Example: %s mb 42\n", prog);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
    usage(argv[0]);
		return -1;
  }
  const char *fru = argv[1];
  int snr;
  if (!strncmp(argv[2], "0x", 2)) {
    snr = (int)strtol(argv[2], NULL, 16);
  } else {
    snr = atoi(argv[2]);
  }
  char key[256];
  snprintf(key, sizeof(key), "%s_sensor%d", fru, snr);
	shm_print(key);
	return 0;
}
