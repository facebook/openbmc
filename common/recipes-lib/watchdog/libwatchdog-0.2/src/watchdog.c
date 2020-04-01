/*
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

#include "watchdog.h"

#define WDT_DEV_FILE          "/dev/watchdog"
#define WDT_KICK_KEY          "k" /* could be any character except 'V' */
#define WDT_MAGIC_CLOSE_KEY   "V"
#define WDT_AK_THREAD         "wdt-auto-kick-thread"
#define WDT_DFT_KICK_INTERVAL 5   /* default kick interval (in seconds) */

#define WDT_ERROR(err, fmt, args...) syslog(LOG_ERR, fmt ": %s", \
                                            ##args, strerror(err))
#define WDT_NOTICE(fmt, args...) syslog(LOG_NOTICE, fmt, ##args)
#define WDT_INFO(fmt, args...) syslog(LOG_INFO, fmt, ##args)
#ifdef VERBOSE_LOG
#define WDT_VERBOSE(fmt, args...) syslog(LOG_INFO, fmt, ##args)
#else
#define WDT_VERBOSE(fmt, args...)
#endif /* VERBOSE_LOG */

/*
 * Below global variables could be accessed from multiple threads, so
 * "wdt_mutex" is required to:
 *     1) protect data from being corrupted by concurrent access;
 *     2) make sure variables updated by one thread can be seen by other
 *        threads.
 */
static pthread_mutex_t wdt_mutex = PTHREAD_MUTEX_INITIALIZER;
static int wdt_dev_fd = -1;

/* "wdt_ak_ctrl" is only needed for auto-kick mode. */
#define AUTO_KICK_ENABLED() (wdt_ak_ctrl.enabled == 1)
#define SET_AUTO_KICK()     wdt_ak_ctrl.enabled = 1
#define CLEAR_AUTO_KICK()   wdt_ak_ctrl.enabled = 0
#define EXIT_FLAG_ENABLED() (wdt_ak_ctrl.exit_flag == 1)
#define SET_EXIT_FLAG()     wdt_ak_ctrl.exit_flag = 1
#define CLEAR_EXIT_FLAG()   wdt_ak_ctrl.exit_flag = 0
static struct {
	int enabled:1;
	int exit_flag:1;
	int kick_interval;
	pthread_t tid;
	pthread_cond_t cond;
} wdt_ak_ctrl = {
	.enabled = 0,
	.exit_flag = 0,
	.kick_interval = 0,
	.cond = PTHREAD_COND_INITIALIZER,
};

/*
 * What can we do if lock/unlock fail (given the lock is statically
 * initialized).. For now, let's just print a warning to let people
 * know something very bad happens.
 */
static void wdt_lock(void)
{
	int status = pthread_mutex_lock(&wdt_mutex);
	if (status != 0) {
		WDT_ERROR(status, "failed to acquire wdt_mutex");
	}
}

static void wdt_unlock(void)
{
	int status = pthread_mutex_unlock(&wdt_mutex);
	if (status != 0) {
		WDT_ERROR(status, "failed to release wdt_mutex");
	}
}

static int kick_watchdog_int(void)
{
	int len = strlen(WDT_KICK_KEY);

	if (write(wdt_dev_fd, WDT_KICK_KEY, len) != len) {
		WDT_ERROR(errno, "failed to kick watchdog");
		return -1;
	}

	WDT_VERBOSE("kicked watchdog");
	return 0;
}

static void* wdt_auto_kick_thread(void *args)
{
	long status = 0;
	struct timespec ts;

	WDT_VERBOSE("%s starts properly", WDT_AK_THREAD);

	wdt_lock();
	while (!EXIT_FLAG_ENABLED()) {
		if (kick_watchdog_int() != 0) {
			break;
		}

		if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
			WDT_ERROR(errno, "failed to get system time");
			break;
		}
		ts.tv_sec += wdt_ak_ctrl.kick_interval;

		/*
		 * pthread_cond_timedwait() may return earlier because
		 * of spurious wakeups or signals, but it's okay to kick
		 * watchdog earlier.
		 */
		status = pthread_cond_timedwait(&wdt_ak_ctrl.cond,
		                                &wdt_mutex, &ts);
		if (status != 0 && status != ETIMEDOUT) {
			WDT_ERROR(status, "failed to wait for wdt_cond");
			break;
		}
	}
	wdt_unlock();

	if (status == 0) {
		WDT_VERBOSE("%s exit normally", WDT_AK_THREAD);
	} else {
		WDT_NOTICE("%s exit with error %ld", WDT_AK_THREAD, status);
	}
	return (void*)status;
}

int open_watchdog(int auto_kick, int kick_interval)
{
	int status;

	if (auto_kick && kick_interval <= 0) {
		kick_interval = WDT_DFT_KICK_INTERVAL;
		WDT_INFO("set default kick interval: %d", kick_interval);
	}

	wdt_lock();

	if (wdt_dev_fd >= 0) {
		WDT_INFO("watchdog is already opened. Skipped.");
		wdt_unlock();
		return 0;
	}

	/*
	 * open /dev/watchdog, which starts watchdog timer automatically.
	 */
	wdt_dev_fd = open(WDT_DEV_FILE, O_RDWR);
	if (wdt_dev_fd < 0) {
		WDT_ERROR(errno, "failed to open %s", WDT_DEV_FILE);
		goto error;
	}
	WDT_VERBOSE("%s opened successfully, fd=%d",
	            WDT_DEV_FILE, wdt_dev_fd);

	/*
	 * set up auto-kick thread if needed.
	 */
	if (auto_kick) {
		wdt_ak_ctrl.kick_interval = kick_interval;
		status = pthread_create(&wdt_ak_ctrl.tid, NULL,
		                        wdt_auto_kick_thread, NULL);
		if (status != 0) {
			WDT_ERROR(status, "failed to create %s", WDT_AK_THREAD);
			goto error;
		}
		SET_AUTO_KICK();
		WDT_VERBOSE("watchdog auto-kick mode enabled");
	}

	wdt_unlock();
	return 0;

error:
	wdt_unlock();
	release_watchdog();
	return -1;
}

void release_watchdog(void)
{
	int status;
	void *exit_code;

	wdt_lock();

	if (AUTO_KICK_ENABLED()) {
		SET_EXIT_FLAG();
		status = pthread_cond_broadcast(&wdt_ak_ctrl.cond);
		if (status != 0) {
			/*
			 * We are not expecting failures because condvar
			 * is static initialized. Anyways, let's dump a
			 * warning just in case something goes wrong.
			 */
			WDT_ERROR(status, "failed to wakeup %s",
			          WDT_AK_THREAD);
		}

		/*
		 * We need to release lock so auto-kick thread can get
		 * chance to run; otherwise, pthread_join() will cause
		 * dead lock.
		 */
		wdt_unlock();

		WDT_VERBOSE("waiting for %s to exit", WDT_AK_THREAD);
		status = pthread_join(wdt_ak_ctrl.tid, &exit_code);
		if (status == 0) {
			WDT_VERBOSE("joined %s successfully: exit code %ld",
			            WDT_AK_THREAD, (long)exit_code);
		} else {
			WDT_ERROR(status, "failed to get %s exit status",
			          WDT_AK_THREAD);
		}

		wdt_lock();
		CLEAR_AUTO_KICK();
	}

	if (wdt_dev_fd >= 0) {
		if (close(wdt_dev_fd) != 0) {
			WDT_ERROR(errno, "failed to close %s (fd=%d)",
			          WDT_DEV_FILE, wdt_dev_fd);
		}
		wdt_dev_fd = -1;
		WDT_VERBOSE("%s closed", WDT_DEV_FILE);
	}

	wdt_unlock();
}


int kick_watchdog(void)
{
	int status;

	wdt_lock();
	status = kick_watchdog_int();
	wdt_unlock();

	return status;
}

int start_watchdog(void)
{
	int status;
	int flags = WDIOS_ENABLECARD;
	unsigned long cmd = WDIOC_SETOPTIONS;

	wdt_lock();
	WDT_NOTICE("starting watchdog explicitly");
	status = ioctl(wdt_dev_fd, cmd, &flags);
	if (status < 0) {
		WDT_ERROR(errno, "failed to start watchdog");
	}
	wdt_unlock();

	return status;
}

int stop_watchdog(void)
{
	int status;
	int flags = WDIOS_DISABLECARD;
	unsigned long cmd = WDIOC_SETOPTIONS;

	wdt_lock();
	WDT_NOTICE("stopping watchdog explicitly");
	status = ioctl(wdt_dev_fd, cmd, &flags);
	if (status < 0) {
		WDT_ERROR(errno, "failed to stop watchdog");
	}
	wdt_unlock();

	return status;
}

int watchdog_enable_magic_close(void)
{
	int status = 0;
	int len = strlen(WDT_MAGIC_CLOSE_KEY);

	wdt_lock();
	WDT_NOTICE("enabling watchdog magic close");
	if (write(wdt_dev_fd, WDT_MAGIC_CLOSE_KEY, len) != len) {
		WDT_ERROR(errno, "failed to enable watchdog magic close");
		status = -1;
	}
	wdt_unlock();

	return status;
}

int watchdog_disable_magic_close(void)
{
	int status = 0;
	char *disable_key = "d";
	int len = strlen(disable_key);

	/*
	 * The magic close feature can be disabled by simply writing the
	 * watchdog with any key(s) except the magic character "V".
	 * Although it can be achieved by kick_watchdog(), let's write
	 * something explicitly here just in case we change kick_watchdog() 
	 * implementation in the future.
	 */
	wdt_lock();
	WDT_NOTICE("disabling watchdog magic close");
	if (write(wdt_dev_fd, disable_key, len) != len) {
		WDT_ERROR(errno, "failed to disable watchdog magic close");
		status = -1;
	}
	wdt_unlock();

	return status;
}
