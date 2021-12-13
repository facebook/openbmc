/*
 * ftdi-bitbang
 *
 * Common routines for all command line utilies.
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 * https://github.com/aehparta/ftdi-bitbang
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#ifndef __CMD_COMMON_H__
#define __CMD_COMMON_H__

#include <getopt.h>

#define COMMON_SHORT_OPTS "hV:P:D:S:I:U:RL"
#define COMMON_LONG_OPTS \
    { "help", no_argument, NULL, 'h' }, \
    { "vid", required_argument, NULL, 'V' }, \
    { "pid", required_argument, NULL, 'P' }, \
    { "description", required_argument, NULL, 'D' }, \
    { "serial", required_argument, NULL, 'S' }, \
    { "interface", required_argument, NULL, 'I' }, \
    { "usbid", required_argument, NULL, 'U' }, \
    { "reset", no_argument, NULL, 'R' }, \
    { "list", no_argument, NULL, 'L' },


/**
 * Command specific option parsing.
 *
 * @param  c        option character
 * @param  optarg   optional argument
 * @return          1 if match, 0 if not
 */
int p_options(int c, char *optarg);

/**
 * Command specific exit.
 */
void p_exit(int return_code);

/**
 * Command specific help.
 */
void p_help(void);

/**
 * Print common help.
 */
void common_help(int argc, char *argv[]);

/**
 * Parse common options.
 */
int common_options(int argc, char *argv[], const char opts[], struct option longopts[], int need_args, int no_opts_needed);

/**
 * Print list of matching devices.
 */
void common_ftdi_list_print(void);

/**
 * Initialize ftdi resources.
 *
 * @param argc Argument count.
 * @param argv Argument array.
 * @return 0 on success, -1 on errors.
 */
struct ftdi_context *common_ftdi_init(void);

/**
 * Read data from stdin until whitespace if it is a pipe or file ( | or < is used ).
 */
unsigned char *common_stdin_read(void);

#endif /* __CMD_COMMON_H__ */
