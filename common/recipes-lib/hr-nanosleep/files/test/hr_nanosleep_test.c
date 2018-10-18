/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <openbmc/hr_nanosleep.h>

uint64_t get_diff(const struct timespec *start,
                  const struct timespec *end)
{
  uint64_t diff = 0;
  diff = (end->tv_sec - start->tv_sec) * NANOSEC_IN_SEC;
  if (end->tv_nsec > start->tv_nsec) {
    diff += end->tv_nsec - start->tv_nsec;
  } else {
    diff -= start->tv_nsec - end->tv_nsec;
  }
  return diff;
}

int get_random_range(int low, int high) {
  return low + (random() % (high - low));
}

void test_hr_nanosleep(int n, int low, int high) {
  int i;
  uint64_t diff;
  int to_sleep;
  struct timespec start, end;
  uint64_t min, max, total;

  assert(low <= high);

  n = 1000;
  min = -1;
  max = 0;
  total = 0;
  printf("\nGoing to call hr_nanosleep() %d times (%dns-%dns) ...\n",
         n, low, high);
  for (i = 0; i < n; i++) {
    to_sleep = get_random_range(low, high);
    clock_gettime(CLOCK_MONOTONIC, &start);
    hr_nanosleep(to_sleep);
    clock_gettime(CLOCK_MONOTONIC, &end);
    diff = get_diff(&start, &end);
    assert(diff >= to_sleep);
    diff -= to_sleep;
    min = (diff < min) ? diff : min;
    max = (diff > max) ? diff : max;
    total += diff;
  }

  printf("hr_nanosleep() off: min=%lluns max=%lluns avg=%lluns\n",
         min, max, total / n);
}

int main(int argc, const char *argv[])
{
  int n;
  int i;
  uint64_t diff;
  struct timespec start, end;

  srandom(clock());

  /* First, call clock_gettime() N times */
  n = 5000;
  printf("Going to call clock_gettime() system call %d times ...\n", n);
  clock_gettime(CLOCK_MONOTONIC, &start);
  end = start;
  for (i = 0; i < n - 1; i++) {
    clock_gettime(CLOCK_MONOTONIC, &end);
  }
  diff = get_diff(&start, &end);
  printf("%d clock_gettime() system calls take %llu nanoseconds.\n", n, diff);
  printf("Each clock_gettime() system call takes %llu nanoseconds.\n",
         diff / n);

  /* Now test hr_nanosleep() from 100ns to 1000ns */
  test_hr_nanosleep(1000, 100, 1000);

  /* Now test hr_nanosleep() from 1000ns to 1ms */
  test_hr_nanosleep(1000, 1000, 1000 * 1000);

  return 0;
}
