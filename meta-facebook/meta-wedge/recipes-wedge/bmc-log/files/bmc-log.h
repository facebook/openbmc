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

#ifndef BMC_LOG_H_
#define BMC_LOG_H_

/* IP Version */
#define IPV4 (4)
#define IPV6 (6)

/* Search string for the kernel version */
#define KERNEL_SEARCH_STR "Linux version "

/* Size constants */
#define TIME_FORMAT_SIZE (100)
#define MAX_LOG_FILE_SIZE (1024*1024*5) //5MB
#define TTY_LEN (50)
#define LINE_LEN (257)
#define MSG_LEN (1025)
#define COMMAND_LEN (100)
#define KERNEL_VERSION_LEN (100)

static char *uS_console = "/usr/local/fbpackages/utils/us_console.sh";

static char *error_log_file = "/var/log/bmc-log";

static char *pseudo_tty_save_file = "/etc/us_pseudo_tty";

static int kernel_search_len = sizeof(KERNEL_SEARCH_STR) - 1;

#endif
