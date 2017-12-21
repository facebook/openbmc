/*
 * Microsemi Switchtec(tm) PCIe Management Command Line Interface
 * Copyright (c) 2017, Microsemi Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COMMANDS_H
#define COMMANDS_H

struct cmd {
	const char *name;
	int (*fn)(int argc, char **argv);
	const char *help;
};

#define CMD(name_fn, help_str) {.name=#name_fn, .fn=name_fn, .help=help_str}

struct subcommand {
	const char *name;
	const char *desc;
	const char *long_desc;
	const struct cmd *cmds;
	struct subcommand *next;
};

#define REGISTER_SUBCMD(subcmd) \
	__attribute__((constructor)) \
	static void init(void) \
	{ \
		commands_register(&subcmd); \
	}

struct prog_info {
	const char *exe;
	const char *usage;
	const char *desc;
};

void commands_register(struct subcommand *subcmd);
int commands_handle(int argc, char **argv, struct prog_info *prog_info);

#endif
