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
#ifndef _OBMC_LOG_H_
#define _OBMC_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/*
 * Logging options to control message format.
 */
#define OBMC_LOG_FMT_IDENT	0x10
#define OBMC_LOG_FMT_TIMESTAMP	0x20
#define OBMC_LOG_FMT_MASK	(OBMC_LOG_FMT_IDENT | OBMC_LOG_FMT_TIMESTAMP)

/*
 * This library uses log priorities defined in <syslog.h>.
 */
#define IS_VALID_LOG_PRIO(p) (((p) >= LOG_EMERG) && ((p) <= LOG_DEBUG))

/*
 * Macros for frequently used log priorities.
 */
#define OBMC_CRIT(fmt, args...) obmc_log_by_prio(LOG_CRIT, fmt, ##args)
#define OBMC_ERROR(err, fmt, args...)	\
	obmc_log_by_prio(LOG_ERR, fmt ": %s", ##args, strerror(err))
#define OBMC_WARN(fmt, args...) obmc_log_by_prio(LOG_WARNING, fmt, ##args)
#define OBMC_INFO(fmt, args...) obmc_log_by_prio(LOG_INFO, fmt, ##args)
#ifdef OBMC_DEBUG_ENABLED
#define OBMC_DEBUG(fmt, args...) obmc_log_by_prio(LOG_DEBUG, fmt, ##args)
#else
#define OBMC_DEBUG(fmt, args...)
#endif /* OBMC_DEBUG_ENABLED */

/*
 * Initializes the logging facility. <ident> (normally program name) is
 * used to tell who prints the logs, and <min_prio> defines the minimum
 * priority of messages to be logged. <options> control whether messages
 * will be prefixed with timestamp and <ident>.
 *
 * Note:
 *   - although it is allowed to call obmc_log_by_prio() even though
 *     obmc_log_init() was never called, callers won't be able to adjust
 *     logging settings (by obmc_log_set_*) until obmc_log_init() is
 *     called.
 *   - logging to standard stream (stdout/stderr) is automatically enabled
 *     for easy use. To update logging destinations, please refer to
 *     obmc_log_set_* functions.
 *
 * Returns:
 *     0 for success, and -1 on failures.
 */
extern int obmc_log_init(const char *ident, int min_prio, int options);

/*
 * Release all the resources of the logging utility.
 */
extern void obmc_log_destroy(void);

/*
 * Function to print log messages.
 *
 * It is safe to call the function without calling obmc_log_init(): in
 * this case, only messages with INFO or lower priority (higher importance)
 * are logged to standard stream.
 *
 * Returns:
 *     0 for success, and -1 on failures.
 */
extern int obmc_log_by_prio(int prio, const char *fmt, ...)
	__attribute__ ((__format__ (__printf__, 2, 3)));

/*
 * Update the mininum priority of messages to be logged.
 *
 * Returns:
 *     0 for success, and -1 on failures.
 */
extern int obmc_log_set_prio(int new_prio);

/*
 * Configure the logger to dump messages to file.
 *
 * Returns:
 *     0 for success, and -1 on failures.
 */
extern int obmc_log_set_file(const char *log_file);

/*
 * Stop logging to file. It's no-op if "logging to file" was never
 * enabled.
 */
extern void obmc_log_unset_file(void);

/*
 * Configure the logger to dump messages to syslogd.
 *
 * Returns:
 *     0 for success, and -1 on failures.
 */
extern int obmc_log_set_syslog(int option, int facility);

/*
 * Stop logging to syslogd. It's no-op if "logging to syslogd" was never
 * enabled.
 */
extern void obmc_log_unset_syslog(void);

/*
 * Configure the logger to dump messages to standard stream.
 *
 * Returns:
 *     0 for success, and -1 on failures.
 */
extern int obmc_log_set_std_stream(void);

/*
 * Stop logging to standard. It's no-op if "logging to standard stream"
 * was never enabled.
 */
extern void obmc_log_unset_std_stream(void);

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* _OBMC_LOG_H_ */
