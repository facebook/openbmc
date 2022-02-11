/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "log.h"

/*
 * Flags to control the destination of log messages.
 */
#define LOG_DEV_FILE		0x01
#define LOG_DEV_SYSLOG		0x02
#define LOG_DEV_STD_STREAM	0x04

#define LOG_BUF_MAX_SIZE	512

struct obmclog_desc {
	char ident[NAME_MAX];

	/*
	 * Minimum log priority: messages won't be logged unless its
	 * priority is equal to or less than <min_prio>.
	 */
	int min_prio;

	/* Bit fields of logging devices (destination of messages). */
	unsigned log_devices;
#define LOG_DEVICE_SET(ldesc, dev)	(ldesc)->log_devices |= (dev)
#define LOG_DEVICE_UNSET(ldesc, dev)	(ldesc)->log_devices &= ~(dev)
#define LOG_DEVICE_IS_SET(ldesc, dev)	((ldesc)->log_devices & (dev))

	/* Properties required for syslog. */
	int syslog_facility;

	/* Properties required for printing messages to file. */
	char file_path[PATH_MAX];
	FILE *file_fp;

	/* Flags for internal use. */
	unsigned priv_flags;
#define LOG_FLAG_CONFIGURED	0x01
};

static struct obmclog_desc my_ldesc = {
	.ident = "fbobmc",
        .min_prio = LOG_INFO,
        .log_devices = LOG_DEV_STD_STREAM,
};

int obmc_log_init(const char *ident, int min_prio, int options)
{
	if (ident == NULL || !IS_VALID_LOG_PRIO(min_prio)) {
		errno = EINVAL;
		return -1;
	}
	if (my_ldesc.priv_flags & LOG_FLAG_CONFIGURED) {
		errno = EBUSY;
		return -1;
	}

	strncpy(my_ldesc.ident, ident, sizeof(my_ldesc.ident) - 1);
	my_ldesc.min_prio = min_prio;
	my_ldesc.log_devices = LOG_DEV_STD_STREAM;
	my_ldesc.priv_flags = (LOG_FLAG_CONFIGURED |
			       (options & OBMC_LOG_FMT_MASK));
	return 0;
}

void obmc_log_destroy(void)
{
	if (my_ldesc.priv_flags & LOG_FLAG_CONFIGURED) {
		if (LOG_DEVICE_IS_SET(&my_ldesc, LOG_DEV_SYSLOG))
			closelog();

		if (LOG_DEVICE_IS_SET(&my_ldesc, LOG_DEV_FILE)) {
			assert(my_ldesc.file_fp != NULL);
			fclose(my_ldesc.file_fp);
		}

		memset(&my_ldesc, 0, sizeof(my_ldesc));
	}
}

static void format_log_message(char *buf,
			       int size,
			       const char *fmt,
			       va_list vargs)
{
	int len, offset = 0;

	/* Add time stamp. */
	if (my_ldesc.priv_flags & OBMC_LOG_FMT_TIMESTAMP) {
		time_t t_now = time(NULL);
		struct tm *tm_now = localtime(&t_now);
		if (tm_now != NULL) {
			len = strftime(buf, size, "%D %T ", tm_now);
			assert(len != 0); /* no buffer overflow */
			offset += len;
			size -= len;
		}
	}

	/* Add message ident. */
	if ((my_ldesc.priv_flags & OBMC_LOG_FMT_IDENT) && (size > 0)) {
		len = snprintf(&buf[offset], size,
			       "%s: ", my_ldesc.ident);
		offset += len;
		size -= len;
	}

	/* Include message body. */
	if (size > 0) {
		len = vsnprintf(&buf[offset], size, fmt, vargs);
		offset += len;
		size -= len;
	}

	/* Append '\n' if needed. */
	if (size > 1 && buf[offset - 1] != '\n') {
		buf[offset] = '\n';
		buf[offset + 1] = '\0';
	}
}

int obmc_log_by_prio(int prio, const char *fmt, ...)
{
	va_list vargs, dup_vargs;
	char buf[LOG_BUF_MAX_SIZE];

	if (!IS_VALID_LOG_PRIO(prio) || fmt == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (prio > my_ldesc.min_prio || my_ldesc.log_devices == 0)
		return 0;

	va_start(vargs, fmt);

	/* Dump log to syslogd. */
	if (LOG_DEVICE_IS_SET(&my_ldesc, LOG_DEV_SYSLOG)) {
		int sprio = LOG_MAKEPRI(my_ldesc.syslog_facility, prio);
		va_copy(dup_vargs, vargs);
		vsyslog(sprio, fmt, dup_vargs);
		va_end(dup_vargs);
	}

	if (my_ldesc.log_devices & ~LOG_DEV_SYSLOG) {
		va_copy(dup_vargs, vargs);
		format_log_message(buf, sizeof(buf), fmt, dup_vargs);
		va_end(dup_vargs);

		/* Dump log to standard stream. */
		if (LOG_DEVICE_IS_SET(&my_ldesc, LOG_DEV_STD_STREAM)) {
			FILE *stream = (prio >= LOG_INFO ?
					stdout : stderr);
			fprintf(stream, "%s", buf);
			fflush(stream);
		}

		/* Dump log to file. */
		if (LOG_DEVICE_IS_SET(&my_ldesc, LOG_DEV_FILE)) {
			assert(my_ldesc.file_fp != NULL);
			fprintf(my_ldesc.file_fp, "%s", buf);
			fflush(my_ldesc.file_fp);
		}
	}

	va_end(vargs);
	return 0;
}

#define CHECK_IF_CONFIGURED(ldesc)					\
	do {								\
		if (!((ldesc)->priv_flags & LOG_FLAG_CONFIGURED)) {	\
			errno = EBADF;					\
			return -1;					\
		}							\
	} while (0)
#define CHECK_SET_DEVICE(ldesc, dev)					\
	do {								\
		CHECK_IF_CONFIGURED(ldesc);				\
		if (LOG_DEVICE_IS_SET((ldesc), (dev))) {		\
			errno = EBUSY;					\
			return -1;					\
		}							\
	} while (0)
#define CHECK_UNSET_DEVICE(ldesc, dev)					\
	do {								\
		if (!((ldesc)->priv_flags & LOG_FLAG_CONFIGURED) ||	\
		    !LOG_DEVICE_IS_SET((ldesc), (dev)))			\
			return;						\
	} while (0)

int obmc_log_set_prio(int new_prio)
{
	CHECK_IF_CONFIGURED(&my_ldesc);
	if (!IS_VALID_LOG_PRIO(new_prio)) {
		errno = EINVAL;
		return -1;
	}

	my_ldesc.min_prio = new_prio;
	return 0;
}

int obmc_log_set_syslog(int option, int facility)
{
	CHECK_SET_DEVICE(&my_ldesc, LOG_DEV_SYSLOG);

	my_ldesc.syslog_facility = facility;
	openlog(my_ldesc.ident, option, facility);
	LOG_DEVICE_SET(&my_ldesc, LOG_DEV_SYSLOG);
	return 0;
}

void obmc_log_unset_syslog(void)
{
	CHECK_UNSET_DEVICE(&my_ldesc, LOG_DEV_SYSLOG);

	LOG_DEVICE_UNSET(&my_ldesc, LOG_DEV_SYSLOG);
	my_ldesc.syslog_facility = 0;
	closelog();
}

int obmc_log_set_file(const char *log_file)
{
	FILE *fp;

	CHECK_SET_DEVICE(&my_ldesc, LOG_DEV_FILE);
	if (log_file == NULL) {
		errno = EINVAL;
		return -1;
	}

	fp = fopen(log_file, "a+");
	if (fp == NULL)
		return -1;

	strncpy(my_ldesc.file_path, log_file,
		sizeof(my_ldesc.file_path) - 1);
	my_ldesc.file_fp = fp;
	LOG_DEVICE_SET(&my_ldesc, LOG_DEV_FILE);
	return 0;
}

void obmc_log_unset_file(void)
{
	CHECK_UNSET_DEVICE(&my_ldesc, LOG_DEV_FILE);

	assert(my_ldesc.file_fp != NULL);

	LOG_DEVICE_UNSET(&my_ldesc, LOG_DEV_FILE);
	fclose(my_ldesc.file_fp);
	my_ldesc.file_fp = NULL;
	my_ldesc.file_path[0] = '\0';
}

int obmc_log_set_std_stream(void)
{
	CHECK_SET_DEVICE(&my_ldesc, LOG_DEV_STD_STREAM);

	LOG_DEVICE_SET(&my_ldesc, LOG_DEV_STD_STREAM);
	return 0;
}

void obmc_log_unset_std_stream(void)
{
	CHECK_UNSET_DEVICE(&my_ldesc, LOG_DEV_STD_STREAM);

	LOG_DEVICE_UNSET(&my_ldesc, LOG_DEV_STD_STREAM);
}


#ifdef OBMC_LOG_UNITTEST

#define TEST_LOG_FILE	"/tmp/obmc-log-test.txt"

#define DUMP_TEST_MESSAGES()						\
	do {								\
		OBMC_DEBUG(MSG_PREFIX "debug: hi there\n");		\
		OBMC_INFO(MSG_PREFIX "info: argc=%d, argv[0]=<%s>",	\
			  argc, argv[0]);				\
		OBMC_WARN(MSG_PREFIX "warning: test continued..");	\
		OBMC_ERROR(EBUSY, MSG_PREFIX "error: %s says oops",	\
			   argv[0]);					\
	} while (0)

int main(int argc, char **argv)
{
	/*
	 * Log messages without calling obmc_log_init().
	 */
#define MSG_PREFIX "[no_init]"
	DUMP_TEST_MESSAGES();
#undef MSG_PREFIX

	if (obmc_log_init("logtest", LOG_INFO,
			  OBMC_LOG_FMT_IDENT | OBMC_LOG_FMT_TIMESTAMP) != 0) {
		perror("obmc_log_init failed");
		return -1;
	}

#define MSG_PREFIX "[prio=info,dev=std]"
	DUMP_TEST_MESSAGES();
#undef MSG_PREFIX

	if (obmc_log_set_file(TEST_LOG_FILE) != 0) {
		perror("obmc_log_set_file failed");
		return -1;
	}
#define MSG_PREFIX "[prio=info,dev=std+file]"
	DUMP_TEST_MESSAGES();
#undef MSG_PREFIX

	if (obmc_log_set_syslog(0, LOG_USER) != 0) {
		perror("obmc_log_set_syslog failed");
		return -1;
	}
	if (obmc_log_set_prio(LOG_DEBUG) != 0) {
		perror("obmc_log_set_prio failed");
		return -1;
	}
#define MSG_PREFIX "[prio=debug,dev=all]"
	DUMP_TEST_MESSAGES();
#undef MSG_PREFIX

	obmc_log_unset_syslog();
	obmc_log_unset_file();
	obmc_log_unset_std_stream();
	obmc_log_destroy();

	return 0;
}
#endif /* OBMC_LOG_UNITTEST */
