/*
 * watchdog
 *
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

#include "watchdog.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>

#define WATCHDOG_START_KEY "x"
#define WATCHDOG_STOP_KEY "X"
/* The "magic close" character (as defined in Linux watchdog specs). */
#define WATCHDOG_PERSISTENT_KEY "V"
#define WATCHDOG_NON_PERSISTENT_KEY "a"

static int watchdog_dev = -1;

/* This is needed to prevent rapid consecutive stop/start watchdog calls from
 * generating multiple threads. */
static int watchdog_started = 0;

static const char* watchdog_kick_key = WATCHDOG_PERSISTENT_KEY;
static pthread_t watchdog_tid;
static pthread_mutex_t watchdog_lock = PTHREAD_MUTEX_INITIALIZER;

/* Forward declarations. */
static void* watchdog_thread(void* args);
static int kick_watchdog_unsafe();

/*
 * When started, this background thread will constantly reset the watchdog
 * at every 5 second interval.
 */

static void* watchdog_thread(void* args) {

  pthread_detach(pthread_self());

  /* Make sure another instance of the thread hasn't already been started. */
  pthread_mutex_lock(&watchdog_lock);
  if (watchdog_started) {
    goto done;
  } else {
    watchdog_started = 1;
  }
  pthread_mutex_unlock(&watchdog_lock);

  /* Actual loop for refreshing the watchdog timer. */
  while (1) {
    pthread_mutex_lock(&watchdog_lock);
    if (watchdog_dev != -1) {
      kick_watchdog_unsafe();
    } else {
      break;
    }
    pthread_mutex_unlock(&watchdog_lock);
    sleep(5);
  }

  /* Broke out of loop because watchdog was stopped. */
  watchdog_started = 0;
done:
  pthread_mutex_unlock(&watchdog_lock);
  return NULL;
}

/*
 * Starts the watchdog timer. timer counts down and restarts the ARM chip
 * upon timeout. use kick_watchdog() to restart the timer.
 *
 * Returns: 1 on success; 0 otherwise.
 */

int start_watchdog(const int auto_mode) {
  int status;

  pthread_mutex_lock(&watchdog_lock);

  /* Don't start the watchdog again if it has already been started. */
  if (watchdog_dev != -1) {
    while ((status = write(watchdog_dev, WATCHDOG_START_KEY, 1)) == 0
           && errno == EINTR);
    pthread_mutex_unlock(&watchdog_lock);
    syslog(LOG_WARNING, "system watchdog already started.\n");
    return 0;
  }

  while (((watchdog_dev = open("/dev/watchdog", O_WRONLY)) == -1) &&
    errno == EINTR);

  /* Fail if watchdog device is invalid or if the user asked for auto
   * mode and the thread failed to spawn. */
  if ((watchdog_dev == -1) ||
    (auto_mode == 1 && watchdog_started == 0 &&
    pthread_create(&watchdog_tid, NULL, watchdog_thread, NULL) != 0)) {
    goto fail;
  }

  while ((status = write(watchdog_dev, WATCHDOG_START_KEY, 1)) == 0
         && errno == EINTR);
  pthread_mutex_unlock(&watchdog_lock);
  syslog(LOG_INFO, "system watchdog started.\n");
  return 1;

fail:
  if (watchdog_dev != -1) {
    close(watchdog_dev);
    watchdog_dev = -1;
  }

  pthread_mutex_unlock(&watchdog_lock);
  syslog(LOG_WARNING, "system watchdog failed to start!\n");
  return 0;
}

/*
 * Toggles between watchdog persistent modes. In persistent mode, the watchdog
 * timer will continue to tick even after process shutdown. Under non-
 * persistent mode, the watchdog timer will automatically be disabled when the
 * process shuts down.
 */
void set_persistent_watchdog(enum watchdog_persistent_en persistent) {
  switch (persistent) {
  case WATCHDOG_SET_PERSISTENT:
    watchdog_kick_key = WATCHDOG_PERSISTENT_KEY;
    break;
  default:
    watchdog_kick_key = WATCHDOG_NON_PERSISTENT_KEY;
    break;
  }
  kick_watchdog();
}

/*
 * Restarts the countdown timer on the watchdog, delaying restart by another
 * timeout period (default: 11 seconds as configured in the device driver).
 *
 * This function assumes the watchdog lock has already been acquired and is
 * only used internally within the watchdog code.
 *
 * Returns 1 on success; 0 or -1 indicates failure (check errno).
 */

static int kick_watchdog_unsafe() {
  int status = 0;
  if (watchdog_dev != -1) {
    while ((status = write(watchdog_dev, watchdog_kick_key, 1)) == 0
           && errno == EINTR);
  }
  return status;
}

/*
 * Acquires the watchdog lock and resets the watchdog atomically. For use by
 * library users.
 */

int kick_watchdog() {
  int result;
  pthread_mutex_lock(&watchdog_lock);
  result = kick_watchdog_unsafe();
  pthread_mutex_unlock(&watchdog_lock);

  return result;
}

/* Shuts down the watchdog gracefully and disables the watchdog timer so that
 * restarts no longer happen.
 */

void stop_watchdog() {
  int status;
  pthread_mutex_lock(&watchdog_lock);
  if (watchdog_dev != -1) {
    while ((status = write(watchdog_dev, WATCHDOG_STOP_KEY, 1)) == 0
           && errno == EINTR);
    close(watchdog_dev);
    watchdog_dev = -1;
    syslog(LOG_INFO, "system watchdog stopped.\n");
  }
  pthread_mutex_unlock(&watchdog_lock);
}
