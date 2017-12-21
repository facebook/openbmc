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

#include "commands.h"
#include "argconfig.h"
#include "version.h"

#include <errno.h>

#include <stdlib.h>
#include <stdio.h>

static struct subcommand *subcmd_default;
static struct subcommand *subcmd_list;

void commands_register(struct subcommand *s)
{
	struct subcommand **list = &subcmd_list;

	if (!s->name) {
		subcmd_default = s;
		return;
	}

	while (*list)
		list = &((*list)->next);

	*list = s;
}

static void strcpy_repl(char *dest, const char *src, size_t n,
			char from, char to)
{
	int i;

	for (i = 0; i < n && src[i] != '\0'; i++) {
		if (src[i] == from)
			dest[i] = to;
		else
			dest[i] = src[i];
	}

	dest[i] = 0;
	dest[n - 1] = 0;
}

static const char *underscore_to_dash(const char *str)
{
	static char buf[64];

	strcpy_repl(buf, str, sizeof(buf), '_', '-');
	return buf;
}

static const char *dash_to_underscore(const char *str)
{
	static char buf[64];

	strcpy_repl(buf, str, sizeof(buf), '-', '_');
	return buf;
}

static int print_version(void)
{
	printf("switchtec cli version %s\n", VERSION);
	return 0;
}

static void print_completions(const struct cmd *cmds)
{
	if (!getenv(COMPLETE_ENV))
		return;

	for (; cmds->name; cmds++)
		printf(" %s", underscore_to_dash(cmds->name));
	printf(" version help\n");
	exit(0);
}

static void usage(const struct subcommand *subcmd,
		  const struct prog_info *prog_info)
{
	if (subcmd->name)
		printf("usage: %s %s %s\n", prog_info->exe, subcmd->name,
		       prog_info->usage);
	else
		printf("usage: %s %s\n", prog_info->exe, prog_info->usage);
}

static void general_help(const struct subcommand *subcmd,
			 const struct prog_info *prog_info)
{
	struct subcommand *ext = subcmd_list;
	unsigned i;

	print_completions(subcmd->cmds);

	printf("switchtec-%s\n", VERSION);

	usage(subcmd, prog_info);

	printf("\n");
	print_word_wrapped(prog_info->desc, 0, 0);
	printf("\n");

	if (subcmd->desc) {
		printf("\n");
		print_word_wrapped(subcmd->desc, 0, 0);
		printf("\n");
	}

	if (subcmd->long_desc) {
		printf("\n");
		print_word_wrapped(subcmd->long_desc, 0, 0);
		printf("\n");
	}

	printf("\nThe following are all implemented sub-commands:\n");

	for (i = 0; subcmd->cmds[i].name; i++)
		printf("  %-15s %s\n", underscore_to_dash(subcmd->cmds[i].name),
		       subcmd->cmds[i].help);

	printf("  %-15s %s\n", "version", "Shows the program version");
	printf("  %-15s %s\n", "help", "Display this help");
	printf("\n");

	if (subcmd->name)
		printf("See '%s %s help <command>' for more information on a specific command\n",
		       prog_info->exe, subcmd->name);
	else
		printf("See '%s help <command>' for more information on a specific command\n",
		       prog_info->exe);

	if (subcmd->name)
		return;

	printf("\nThe following are all installed extensions:\n");
	while (ext) {
		printf("  %-15s %s\n", underscore_to_dash(ext->name),
		       ext->desc);
		ext = ext->next;
	}
	printf("\nSee '%s <extension> help' for more information on a extension\n",
	       prog_info->exe);
}

static int do_command(int argc, char **argv, struct subcommand *subcmd,
		      const struct prog_info *prog_info);

static int help(int argc, char **argv, struct subcommand *subcmd,
		const struct prog_info *prog_info)
{
	char *new_argv[2];

	if (getenv(COMPLETE_ENV))
		print_completions(subcmd->cmds);

	if (argc == 1) {
		general_help(subcmd, prog_info);
		return 0;
	}

	new_argv[0] = argv[1];
	new_argv[1] = "--help";

	do_command(2, new_argv, subcmd, prog_info);

	return 0;
}

static int do_command(int argc, char **argv, struct subcommand *subcmd,
		      const struct prog_info *prog_info)
{
	struct subcommand *ext = subcmd_list;
	const char *cmd = argv[0];
	char usage[100];
	int i;

	if (!argc) {
		general_help(subcmd, prog_info);
		return 0;
	}

	if (subcmd->name)
		snprintf(usage, sizeof(usage), "%s %s %s", prog_info->exe,
			 subcmd->name, cmd);
	else
		snprintf(usage, sizeof(usage), "%s %s", prog_info->exe, cmd);
	argconfig_append_usage(usage);

	while (*cmd == '-')
		cmd++;

	cmd = dash_to_underscore(cmd);

	if (!strcmp(cmd, "help"))
		return help(argc, argv, subcmd, prog_info);
	if (!strcmp(cmd, "version"))
		return print_version();

	for (i = 0; subcmd->cmds[i].name; i++) {
		const struct cmd *c = &subcmd->cmds[i];

		if (!strcmp(cmd, c->name))
			return c->fn(argc, argv);
	}

	if (subcmd->name) {
		printf("ERROR: Invalid sub-command '%s' for command '%s'\n",
		       cmd, subcmd->name);
		return -ENOTSUP;
	}

	while (ext) {
		if (!strcmp(cmd, ext->name))
			return do_command(argc - 1, &argv[1], ext, prog_info);

		ext = ext->next;
	}

	print_completions(subcmd->cmds);
	printf("ERROR: Invalid command '%s'\n", cmd);
	return -ENOTSUP;
}

int commands_handle(int argc, char **argv, struct prog_info *prog_info)
{
	prog_info->exe = argv[0];

	return do_command(argc - 1, &argv[1], subcmd_default, prog_info);
}
