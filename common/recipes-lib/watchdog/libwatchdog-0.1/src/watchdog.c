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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include "watchdog.h"

#define WATCHDOG_DEV_FILE		"/dev/watchdog"
#define WATCHDOG_START_KEY		"x"
#define WATCHDOG_STOP_KEY		"X"
/* The "magic close" character (as defined in Linux watchdog specs). */
#define WATCHDOG_PERSISTENT_KEY		"a"
#define WATCHDOG_NON_PERSISTENT_KEY	"V"

static int watchdog_dev = -1;
static const char* watchdog_kick_key = WATCHDOG_PERSISTENT_KEY;

/* This is needed to prevent rapid consecutive open_watchdog() calls from
 * generating multiple threads. */
static int kick_thread_running;

static pthread_t watchdog_tid;
static pthread_mutex_t watchdog_lock = PTHREAD_MUTEX_INITIALIZER;
static int auto_kick_interval = 5; /* default auto kick interval */

/*
 * Write 1 byte character to the watchdog device.
 *
 * Returns: 0 for success, and -1 on failures.
 */
static int write_watchdog(const char *key)
{
  int nwrite;
  int retries = 10;

  while ((nwrite = write(watchdog_dev, key, 1)) < 1) {
    if (errno != EINTR || --retries < 0) {
      return -1;
    }
  }

  return 0;
}

/*
 * Restarts the watchdog timer by sending <watchdog_kick_key>.
 * This internal function assumes 1) the watchdog device is opened 2) the
 * watchdog lock has already been acquired.
 *
 * Returns 0 on success; -1 indicates failure.
 */
static int kick_watchdog_unsafe(void)
{
  return write_watchdog(watchdog_kick_key);
}

/*
 * When started, this background thread will constantly kick the watchdog
 * at <auto-kick-interval> interval.
 */
static void* watchdog_kick_thread(void* args)
{
  pthread_detach(pthread_self());

  /* Make sure another instance of the thread hasn't already been started. */
  pthread_mutex_lock(&watchdog_lock);
  if (kick_thread_running != 0)
    goto done;

  kick_thread_running = 1;
  pthread_mutex_unlock(&watchdog_lock);

  /* Actual loop for refreshing the watchdog timer. */
  while (1) {
    pthread_mutex_lock(&watchdog_lock);
    if (watchdog_dev < 0)
      break;

    kick_watchdog_unsafe();
    pthread_mutex_unlock(&watchdog_lock);
    sleep(auto_kick_interval);
  }

  /* Broke out of loop because watchdog was stopped. */
  kick_thread_running = 0;
done:
  pthread_mutex_unlock(&watchdog_lock);
  return NULL;
}

/*
 * Open watchdog device and also start the watchdog timer. The watchdog
 * timer will reset the system upon timeout. Use kick_watchdog() to kick
 * the timer.
 *
 * Returns: 0 on success; -1 otherwise.
 */
int open_watchdog(const int auto_mode, int kick_interval)
{
  int status;

  pthread_mutex_lock(&watchdog_lock);

  if (watchdog_dev < 0) {
    while (((watchdog_dev = open(WATCHDOG_DEV_FILE, O_WRONLY)) == -1) &&
           errno == EINTR);
    if (watchdog_dev < 0) {
      syslog(LOG_WARNING, "failed to open %s: %s\n",
             WATCHDOG_DEV_FILE, strerror(errno));
      goto error;
    }
  }

  /* Start auto-kick thread if needed. */
  if (auto_mode != 0 && kick_thread_running == 0) {
    if (kick_interval > 0) {
      auto_kick_interval = kick_interval;
    }
    status = pthread_create(&watchdog_tid, NULL,
                            watchdog_kick_thread, NULL);
    if (status != 0) {
      syslog(LOG_WARNING, "failed to create auto-kick thread: %s\n",
             strerror(status));
      errno = status;
      goto error;
    }
  }

  status = write_watchdog(WATCHDOG_START_KEY);
  if (status != 0) {
      syslog(LOG_WARNING, "failed to start watchdog: %s\n",
             strerror(errno));
      goto error;
  }

  pthread_mutex_unlock(&watchdog_lock);
  syslog(LOG_DEBUG, "system watchdog opened and started.\n");
  return 0;

error:
  if (watchdog_dev >= 0) {
    close(watchdog_dev);
    watchdog_dev = -1;
  }
  pthread_mutex_unlock(&watchdog_lock);
  return -1;
}

/*
 * Release all the resources allocated by open_watchdog(). The watchdog
 * timer may or may NOT be stopped, depending on whether magic close
 * feature is enabled right before closing the device file.
 */
void release_watchdog(void)
{
  /*
   * The auto-kick thread (if created) will quit soon after the file
   * descriptor is closed.
   */
  pthread_mutex_lock(&watchdog_lock);
  if (watchdog_dev >= 0) {
    close(watchdog_dev);
    watchdog_dev = -1;
  }
  pthread_mutex_unlock(&watchdog_lock);
}

/*
 * Enable magic close feature. If the watchdog is closed right after
 * enabling the feature, the watchdog timer will be disabled at close.
 *
 * Returns: 0 on success, -1 on failures.
 */
int watchdog_enable_magic_close(void)
{
  watchdog_kick_key = WATCHDOG_NON_PERSISTENT_KEY;
  return kick_watchdog();
}

/*
 * Disable magic close feature. This is the default behavior: watchdog
 * timer will continue ticking after being closed.
 *
 * Returns: 0 on success, -1 on failures.
 */
int watchdog_disable_magic_close(void)
{
  watchdog_kick_key = WATCHDOG_PERSISTENT_KEY;
  return kick_watchdog();
}

/*
 * Kick the watchdog, which resets the watchdog timer.
 *
 * Returns 0 on success; -1 indicates failure (check errno).
 */
int kick_watchdog(void)
{
  int status;

  pthread_mutex_lock(&watchdog_lock);
  if (watchdog_dev >= 0) {
    status = kick_watchdog_unsafe();
  } else {
    errno = EBADFD;
    status = -1;
  }
  pthread_mutex_unlock(&watchdog_lock);

  if (status != 0) {
    syslog(LOG_WARNING, "failed to kick watchdog: %s\n",
           strerror(errno));
  }
  return status;
}

/*
 * Stop the watchdog timer. The watchdog timer will stop ticking if this
 * function returns successfully.
 *
 * Returns: 0 for success, or -1 on failures.
 */
int stop_watchdog(void)
{
  int status;

  pthread_mutex_lock(&watchdog_lock);
  if (watchdog_dev >= 0) {
    status = write_watchdog(WATCHDOG_STOP_KEY);
  } else {
    errno = EBADFD;
    status = -1;
  }
  pthread_mutex_unlock(&watchdog_lock);

  if (status == 0) {
    syslog(LOG_INFO, "system watchdog stopped.\n");
  } else {
    syslog(LOG_WARNING, "failed to stop watchdog: %s\n",
           strerror(errno));
  }
  return status;
}

/*
 * Start the watchdog timer. If the timer is already running, then this
 * function just resets the timer.
 *
 * Returns: 0 for success, -1 on failures.
 */
int start_watchdog(void)
{
  int status;

  pthread_mutex_lock(&watchdog_lock);
  if (watchdog_dev >= 0) {
    status = write_watchdog(WATCHDOG_START_KEY);
  } else {
    errno = EBADFD;
    status = -1;
  }
  pthread_mutex_unlock(&watchdog_lock);

  if (status == 0) {
    syslog(LOG_INFO, "system watchdog started.\n");
  } else {
    syslog(LOG_WARNING, "failed to start watchdog: %s\n",
           strerror(errno));
  }
  return status;
}
