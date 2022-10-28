/*
 * Copyright 2018-present Facebook. All Rights Reserved.
 *
 * This is the source file of Watchdog Control Command Line Interface.
 * Run "wdtcli -h" on openbmc for usage information.
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
#include <syslog.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <openbmc/watchdog.h>

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#endif /* ARRAY_SIZE */

#define WDT_ERROR(...)	fprintf(stderr, __VA_ARGS__)
#define WDT_INFO(...)	fprintf(stdout, __VA_ARGS__)
#define WDT_VERBOSE(...)		\
	do {					\
		if (verbose_logging) {		\
			printf(__VA_ARGS__); \
		}				\
	} while (0)

typedef struct wdtcli_cmd_info {
	char *cmd;
	char *desc;
	int (*func)(struct wdtcli_cmd_info *info, int argc, char **argv);
} wdtcli_cmd_t;

static int verbose_logging;

static int wdtcli_start_watchdog(wdtcli_cmd_t *info UNUSED, int argc UNUSED, char **argv UNUSED)
{
	return start_watchdog();
}

static int wdtcli_stop_watchdog(wdtcli_cmd_t *info UNUSED, int argc UNUSED, char **argv UNUSED)
{
	return stop_watchdog();
}

static int wdtcli_kick_watchdog(wdtcli_cmd_t *info UNUSED, int argc UNUSED, char **argv UNUSED)
{
	return kick_watchdog();
}

static int wdtcli_set_timeout(wdtcli_cmd_t *info, int argc, char **argv)
{
	unsigned long timeout;

	if (argc <= 0) {
		fprintf(stderr,
			"Error: <timeout> is missing for %s command!\n",
			info->cmd);
		return -1;
	}

	timeout = strtoul(argv[0], NULL, 0);
	if (timeout > UINT_MAX) {
		fprintf(stderr,
			"Error: <timeout> integer overflow (%lu > %u)!\n",
			timeout, UINT_MAX);
		return -1;
	}

	WDT_VERBOSE("set watchdog timeout value to %lu\n", timeout);
	return watchdog_set_timeout((unsigned int)timeout);
}

static int wdtcli_get_timeout(wdtcli_cmd_t *info UNUSED, int argc UNUSED, char **argv UNUSED)
{
	int ret;
	unsigned int timeout;

	WDT_VERBOSE("read the current watchdog timeout value\n");
	ret = watchdog_get_timeout(&timeout);
	if (ret == 0) {
		WDT_INFO("watchdog_timeout=%u\n", timeout);
	}

	return ret;
}

static wdtcli_cmd_t wdtcli_cmds[] = {
	{
		"start",
		"start watchdog",
		wdtcli_start_watchdog,
	},
	{
		"stop",
		"stop watchdog",
		wdtcli_stop_watchdog,
	},
	{
		"kick",
		"kick watchdog",
		wdtcli_kick_watchdog,
	},
	{
		"set-timeout",
		"set a new watchdog timeout value (in seconds)",
		wdtcli_set_timeout,
	},
	{
		"get-timeout",
		"get the current watchdog timeout value (in seconds)",
		wdtcli_get_timeout,
	},

	/* This is the last element. */
	{
		NULL,
		NULL,
		NULL,
	},
};

static void wdtcli_list_cmd(void)
{
	int i;

	printf("Supported commands:\n");
	for (i = 0; wdtcli_cmds[i].cmd != NULL; i++) {
		printf("    %-6s - %s\n",
		       wdtcli_cmds[i].cmd, wdtcli_cmds[i].desc);
	}
}

static wdtcli_cmd_t*
wdtcli_match_cmd(const char *cmd)
{
	int i;

	for (i = 0; wdtcli_cmds[i].cmd != NULL; i++) {
		if (strcmp(cmd, wdtcli_cmds[i].cmd) == 0) {
			return &wdtcli_cmds[i];
		}
	}

	return NULL;
}

static void wdtcli_usage(const char *prog_name)
{
	size_t i;
	struct {
		char *opt;
		char *desc;
	} options[] = {
		{"-l", "list available commands"},
		{"-v", "enable verbose logging"},
		{"-h", "print this help message"},
	};

	printf("Usage: %s [-lvh] <command> [arguments..]\n", prog_name);
	for (i = 0; i < ARRAY_SIZE(options); i++) {
		printf("    %s  %s\n", options[i].opt, options[i].desc);
	}
}


int main(int argc, char **argv)
{
	int opt, ret;
	char *user_cmd = NULL;
	wdtcli_cmd_t *cmd_info;
	int arg_count = 0;
	char **arg_list = NULL;

	while ((opt = getopt(argc, argv, "vlh")) != -1) {
		switch (opt) {
		case 'v':
			verbose_logging = 1;
			break;
		case 'l':
			wdtcli_list_cmd();
			return 0;
		case 'h':
			wdtcli_usage(argv[0]);
			return 0;
		default:
			wdtcli_usage(argv[0]);
			return -1;
		}
	}
	if (optind < argc) {
		user_cmd = argv[optind++];
		arg_count = argc - optind;
		if (arg_count > 0) {
			arg_list = &argv[optind];
		}
	} else {
		fprintf(stderr,
			"Error: <command> argument is missing!\n\n");
		wdtcli_usage(argv[0]);
		return -1;
	}

	openlog(NULL, 0, LOG_USER);
	setlogmask(LOG_UPTO(LOG_INFO));

	cmd_info = wdtcli_match_cmd(user_cmd);
	if (cmd_info == NULL) {
		fprintf(stderr,
			"Error: invalid command <%s> received!\n\n",
			user_cmd);
		wdtcli_usage(argv[0]);
		return -1;
	}

	/* Open watchdog device. */
	ret = open_watchdog(0, 0);
	if (ret < 0) {
		WDT_ERROR("open watchdog: failed\n");
		return ret;
	}
	WDT_VERBOSE("open watchdog: passed\n");

	/* Execute user supplied command. */
	assert(cmd_info->desc != NULL);
	assert(cmd_info->func != NULL);
	ret = cmd_info->func(cmd_info, arg_count, arg_list);
	if (ret < 0) {
		WDT_ERROR("%s: failed\n", cmd_info->desc);
	} else {
		WDT_VERBOSE("%s: passed\n", cmd_info->desc);
	}

	release_watchdog();
	WDT_VERBOSE("close watchdog: passed\n");

	return ret;
}
