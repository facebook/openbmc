/*
 * Thread benchmark
 *
 * Copyright 2021 Facebook. All Rights Reserved.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void busy_wait(unsigned int time_ms)
{
  struct timespec curr, end;
  clock_gettime(CLOCK_MONOTONIC_RAW, &curr);
  end = curr;
  if (time_ms > 1000) {
    end.tv_sec += (time_ms / 1000);
    time_ms %= 1000;
  }
  end.tv_nsec += 1000000ULL * (unsigned long long)time_ms;
  if (end.tv_nsec >= 1000000000ULL) {
    end.tv_sec += 1;
    end.tv_nsec %= 1000000000ULL;
  }
  do {
    clock_gettime(CLOCK_MONOTONIC_RAW, &curr);
  } while (curr.tv_sec < end.tv_sec || (curr.tv_sec == end.tv_sec && curr.tv_nsec < end.tv_nsec));
}

void sleep_wait(unsigned int time_ms)
{
  struct timespec ts;
  ts.tv_sec = time_ms / 1000;
  ts.tv_nsec = 1000000ULL  * (time_ms % 1000);
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

static unsigned int target_utilization = 3;

static void do_test() {
  // 100ms work
  busy_wait(target_utilization);
  sleep_wait(100 - target_utilization);
}

static void *thread(void *unused) {
  do_test();
  pthread_exit(NULL);
}

static void do_pthread() {
  pthread_t tid;
  if (pthread_create(&tid, NULL, thread, NULL)) {
    printf("Creating thread failed!\n");
    exit(1);
  }
  pthread_join(tid, NULL);
}

static void do_fork() {
  pid_t pid = fork();
  if (pid == 0) {
    do_test();
    exit(0);
  } else if (pid == -1) {
    printf("Fork failed!\n");
    exit(1);
  } else {
    waitpid(pid, NULL, 0);
  }
}

int main(int argc, char *argv[])
{
  void (*test)(void) = do_test;
  const char *what = "base";
  if (argc > 1)
    what = argv[1];
  if (!strcmp(what, "base")) {
    test = do_test;
  } else if (!strcmp(what, "fork")) {
    test = do_fork;
  } else if (!strcmp(what, "pthread")) {
    test = do_pthread;
  } else {
    printf("Incorrect test: %s\n", what);
    return 1;
  }
  int iter = 1000;
  if (argc > 2)
    iter = atoi(argv[2]);
  if (iter <= 0) {
    printf("Invalid iterations: %d\n", iter);
    return 1;
  }
  if (argc > 3) {
    target_utilization = atoi(argv[3]);
    if (target_utilization > 100) {
      printf("Invalid utilization: %u\n", target_utilization);
      return 1;
    }
  }

  printf("Doing %s test for %d iterations target util: %u\n",
      what, iter, target_utilization);

  for (int i = 0; i < iter; i++) {
    test();
  }
  return 0;
}
