////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 PMC-Sierra, Inc.
// Copyright (c) 2017, Microsemi Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
//   Author: Logan Gunthorpe
//
//   Date:   Oct 23 2014
//
//   Description:
//     Functions for parsing command line options.
//
////////////////////////////////////////////////////////////////////////

#include "argconfig.h"
#include "suffix.h"

#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>

static argconfig_help_func *help_funcs[MAX_HELP_FUNC] = { NULL };

static char append_usage_str[100] = "";

void argconfig_append_usage(const char *str)
{
	strncat(append_usage_str, str, sizeof(append_usage_str) -
		strlen(append_usage_str) - 1);
}

void print_word_wrapped(const char *s, int indent, int start)
{
	const int width = 76;
	const char *c, *t;
	int next_space = -1;
	int last_line = indent;

	while (start < indent) {
		putc(' ', stderr);
		start++;
	}

	for (c = s; *c != 0; c++) {
		if (*c == '\n')
			goto new_line;

		if (*c == ' ' || next_space < 0) {
			next_space = 0;
			for (t = c + 1; *t != 0 && *t != ' '; t++)
				next_space++;

			if (((int)(c - s) + start + next_space) > (last_line - indent + width)) {
				int i;
new_line:
				last_line = (int) (c-s) + start;
				putc('\n', stderr);
				for (i = 0; i < indent; i++)
					putc(' ', stderr);
				start = indent;
				continue;
			}
		}
		putc(*c, stderr);
	}
}

static int is_positional(const struct argconfig_options *s)
{
	return s->argument_type == optional_positional ||
		s->argument_type == required_positional;
}

static int is_file_option(const struct argconfig_options * s)
{
	switch (s->cfg_type) {
	case CFG_FILE_A:
	case CFG_FILE_W:
	case CFG_FILE_R:
	case CFG_FILE_AP:
	case CFG_FILE_WP:
	case CFG_FILE_RP:
	case CFG_FD_RD:
	case CFG_FD_WR:
		return 1;
	default:
		return 0;
	}
}

static int num_positional(const struct argconfig_options *options)
{
	int count = 0;
	const struct argconfig_options *s;

	for (s = options; s->option; s++) {
		if (!is_positional(s))
			continue;
		count++;
	}

	return count;
}

static int num_env_variables(const struct argconfig_options *options)
{
	int count = 0;
	const struct argconfig_options *s;

	for (s = options; s->option; s++) {
		if (!is_positional(s))
			continue;
		if (!s->env)
			continue;
		count++;
	}

	return count;
}

static int num_options(const struct argconfig_options *options)
{
	int count = 0;
	const struct argconfig_options *s;

	for (s = options; s->option; s++) {
		if (is_positional(s))
			continue;
		count++;
	}

	return count;
}

static void show_option(const struct argconfig_options *option)
{
	char buffer[0x4000];
	char *b = buffer;

	if (is_positional(option))
	    return;

	b += sprintf(b, "  [ ");
	if (option->option) {
		b += sprintf(b, " --%s", option->option);
		if (option->argument_type == optional_argument)
			b += sprintf(b, "[=<%s>]", option->meta ? option->meta : "arg");
		if (option->argument_type == required_argument)
			b += sprintf(b, "=<%s>", option->meta ? option->meta : "arg");
		if (option->short_option)
			b += sprintf(b, ",");
	}

	if (option->short_option) {
		if ((b - buffer) >= 30) {
			fprintf(stderr, "%s\n", buffer);
			b = buffer;
			b += sprintf(b, "     ");
		}

		b += sprintf(b, " -%c", option->short_option);
		if (option->argument_type == optional_argument)
			b += sprintf(b, " [<%s>]", option->meta ? option->meta : "arg");
		if (option->argument_type == required_argument)
			b += sprintf(b, " <%s>", option->meta ? option->meta : "arg");
	}
	b += sprintf(b, " ] ");

	fprintf(stderr, "%s", buffer);
	if (option->help) {
		print_word_wrapped("--- ", 40, b - buffer);
		print_word_wrapped(option->help, 44, 44);
	}
	fprintf(stderr, "\n");
}

static void show_positional(const struct argconfig_options *option)
{
	char buffer[0x1000];
	char *b = buffer;

	if (!is_positional(option))
		return;

	if (option->argument_type == optional_positional)
		b += snprintf(buffer, sizeof(buffer), "  [<%s>] ", option->option);
	else
		b += snprintf(buffer, sizeof(buffer), "   <%s>  ", option->option);

	fprintf(stderr, "%s", buffer);
	if (option->help) {
		print_word_wrapped("--- ", 40, b - buffer);
		print_word_wrapped(option->help, 44, 44);
	}
	fprintf(stderr, "\n");
}

static void show_env(const struct argconfig_options *option)
{
	char buffer[0x1000];
	char *b = buffer;

	if (!is_positional(option))
		return;
	if (!option->env)
		return;

	b += snprintf(buffer, sizeof(buffer), "    %s  ", option->env);

	fprintf(stderr, "%s", buffer);

	sprintf(buffer, "if set, the value will be used for the <%s> argument",
		option->option);

	if (option->help) {
		print_word_wrapped("--- ", 40, b - buffer);
		print_word_wrapped(buffer, 44, 44);
	}
	fprintf(stderr, "\n");
}

static void show_choices(const struct argconfig_options *option)
{
	const struct argconfig_choice *c;

	if (!option->choices)
		return;

	printf("\033[1mChoices for %s:\033[0m\n", option->meta);

	for (c = option->choices; c->name; c++) {
		char buffer[0x1000];
		char *b = buffer;

		b += snprintf(buffer, sizeof(buffer), "    %s ", c->name);

		fprintf(stderr, "%s", buffer);
		if (option->help) {
			print_word_wrapped("--- ", 40, b - buffer);
			print_word_wrapped(c->help, 44, 44);
		}
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "\n");
}

void argconfig_print_usage(const struct argconfig_options *options)
{
	const struct argconfig_options *s;
	printf("\033[1mUsage:\033[0m %s", append_usage_str);

	for (s = options; (s->option != NULL) && (s != NULL); s++) {
		if (!is_positional(s) && !s->require_in_usage)
			continue;

		if (s->argument_type == optional_positional)
			printf(" [<%s>]", s->option);
		else if (s->argument_type == required_positional)
			printf(" <%s>", s->option);
		else
			printf(" --%s=<%s>", s->option, s->meta);

	}

	if (num_options(options))
		printf(" [OPTIONS]");

	printf("\n");
}

void argconfig_print_help(const char *program_desc,
			  const struct argconfig_options *options)
{
	const struct argconfig_options *s;
	int num_opt = num_options(options);
	int num_pos = num_positional(options);
	int num_env = num_env_variables(options);

	argconfig_print_usage(options);
	printf("\n");

	print_word_wrapped(program_desc, 0, 0);
	printf("\n\n");

	if (num_pos) {
		printf("\n\033[1mPositional Arguments:\033[0m\n");

		for (s = options; (s->option != NULL) && (s != NULL); s++)
			show_positional(s);

		printf("\n");
	}

	if (num_env) {
		printf("\n\033[1mEnvironment Variables:\033[0m\n");

		for (s = options; (s->option != NULL) && (s != NULL); s++)
			show_env(s);

		printf("\n");
	}

	if (num_opt) {
		printf("\n\033[1mOptions:\033[0m\n");

		for (s = options; (s->option != NULL) && (s != NULL); s++)
			show_option(s);

		printf("\n");
	}


	for (s = options; (s->option != NULL) && (s != NULL); s++)
		show_choices(s);
}

static int cfg_none_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	*((int *)value_addr) = 1;
	return 0;
}

static int cfg_string_handler(const char *optarg, void *value_addr,
			      const struct argconfig_options *opt)
{
	*((const char **)value_addr) = optarg;
	return 0;
}

static int cfg_int_handler(const char *optarg, void *value_addr,
			   const struct argconfig_options *opt)
{
	char *endptr;

	*((int *)value_addr) = strtol(optarg, &endptr, 0);

	if (errno || optarg == endptr) {
		fprintf(stderr,
			"Expected integer argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}

	return 0;
}

static int cfg_size_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	char *endptr;

	*((size_t *) value_addr) = strtol(optarg, &endptr, 0);
	if (errno || optarg == endptr) {
		fprintf(stderr,
			"Expected integer argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}

	return 0;
}

static int cfg_long_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	char *endptr;

	*((unsigned long *)value_addr) = strtoul(optarg, &endptr, 0);
	if (errno || optarg == endptr) {
		fprintf(stderr,
			"Expected long integer argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}

	return 0;
}

static int cfg_long_long_handler(const char *optarg, void *value_addr,
                       const struct argconfig_options *opt)
{
	char *endptr;

	*((unsigned long long *)value_addr) = strtoull(optarg, &endptr, 0);
	if (errno || optarg == endptr) {
		fprintf(stderr,
			"Expected long long integer argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}

	return 0;
}

static int cfg_long_suffix_handler(const char *optarg, void *value_addr,
				   const struct argconfig_options *opt)
{
	*((long *)value_addr) = suffix_binary_parse(optarg);
	if (errno) {
		fprintf(stderr,
			"Expected suffixed integer argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}

	return 0;
}

static int cfg_double_handler(const char *optarg, void *value_addr,
			      const struct argconfig_options *opt)
{
	char *endptr;

	*((double *)value_addr) = strtod(optarg, &endptr);
	if (errno || optarg == endptr) {
		fprintf(stderr,
			"Expected float argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}
	return 0;
}

static int cfg_bool_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	char *endptr;

	int tmp = strtol(optarg, &endptr, 0);
	if (errno || tmp < 0 || tmp > 1 || optarg == endptr) {
		fprintf(stderr,
			"Expected 0 or 1 argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}
	*((int *)value_addr) = tmp;

	return 0;
}

static int cfg_byte_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	char *endptr;

	unsigned long tmp = strtoul(optarg, &endptr, 0);
	if (errno || tmp >= (1 << 8)  || optarg == endptr) {
		fprintf(stderr,
			"Expected byte argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}
	*((uint8_t *) value_addr) = tmp;
	return 0;
}

static int cfg_short_handler(const char *optarg, void *value_addr,
			     const struct argconfig_options *opt)
{
	char *endptr;

	unsigned long tmp = strtoul(optarg, &endptr, 0);
	if (errno || tmp >= (1 << 16)  || optarg == endptr) {
		fprintf(stderr,
			"Expected short argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}
	*((uint16_t *) value_addr) = tmp;
	return 0;
}

static int cfg_positive_handler(const char *optarg, void *value_addr,
				const struct argconfig_options *opt)
{
	char *endptr;

	unsigned long tmp = strtoul(optarg, &endptr, 0);
	if (errno || optarg == endptr) {
		fprintf(stderr,
			"Expected positive argument for '--%s/-%c' "
			"but got '%s'!\n",
			opt->option, opt->short_option, optarg);
		return 1;
	}
	*((unsigned *) value_addr) = tmp;
	return 0;
}

static int cfg_increment_handler(const char *optarg, void *value_addr,
				 const struct argconfig_options *opt)
{
	(*((int *)value_addr))++;
	return 0;
}

static int cfg_file_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	const char *fopts = "";
	switch(opt->cfg_type) {
	case CFG_FILE_A: fopts = "ab"; break;
	case CFG_FILE_R: fopts = "rb"; break;
	case CFG_FILE_W: fopts = "wb"; break;
	case CFG_FILE_AP: fopts = "a+b"; break;
	case CFG_FILE_RP: fopts = "r+b"; break;
	case CFG_FILE_WP: fopts = "w+b"; break;
	default: return 1;
	}

	FILE *f = fopen(optarg, fopts);
	if (f == NULL) {
		perror(optarg);
		return 1;
	}
	*((FILE **)value_addr) = f;
	value_addr += sizeof(FILE *);
	*((const char **)value_addr) = optarg;

	return 0;
}

static void set_fd_path(void *value_addr, int fd, const char *path)
{
	*((int *) value_addr) = fd;
	value_addr += sizeof(int *);
	*((const char **) value_addr) = path;
}

static int cfg_fd_handler(const char *optarg, void *value_addr,
			  const struct argconfig_options *opt)
{
	int fd;
	int flags;

	if (strcmp(optarg, "-") == 0) {
		if (opt->cfg_type == CFG_FD_WR)
			set_fd_path(value_addr, STDOUT_FILENO, "stdout");
		else
			set_fd_path(value_addr, STDIN_FILENO, "stdin");

		return 0;
	}

	switch(opt->cfg_type) {
	case CFG_FD_WR: flags = O_CREAT | O_TRUNC | O_WRONLY; break;
	case CFG_FD_RD: flags = O_RDONLY; break;
	default: return 1;
	}

	fd = open(optarg, flags, 0666);
	if (fd < 0) {
		perror(optarg);
		return 1;
	}

	set_fd_path(value_addr, fd, optarg);

	return 0;
}

static int cfg_choices_handler(const char *optarg, void *value_addr,
			       const struct argconfig_options *opt)
{
	unsigned *ivalue = value_addr;
	const struct argconfig_choice *c;

	if (!opt->choices) {
		fprintf(stderr,
			"No choices set for '--%s/-%c' \n",
			opt->option, opt->short_option);
		return 1;
	}

	for (c = opt->choices; c->name; c++) {
		if (strcasecmp(optarg, c->name) == 0)
			break;
	}

	if (!c->name) {
		fprintf(stderr,
			"Unexpected choice '%s' for '--%s/-%c' \n",
			optarg, opt->option, opt->short_option);
		return 1;
	}

	if (opt->cfg_type == CFG_MULT_CHOICES)
		*ivalue |= c->value;
	else
		*ivalue = c->value;

	return 0;
}

static int cfg_mask_handler(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt)
{
	int nums[sizeof(uint64_t) * 8];
	int cnt;
	int i;

	cnt = argconfig_parse_comma_range(optarg, nums,
					  sizeof(nums) / sizeof(*nums));

	if (cnt < 0) {
		fprintf(stderr,
			"Invalid number or range for '--%s/-%c' \n",
			opt->option, opt->short_option);
		return 1;
	}

	for (i = 0; i < cnt; i++) {
		if (nums[i] < 0)
			goto range_error;

		switch(opt->cfg_type) {
		case CFG_MASK_64:
			if (nums[i] >= sizeof(uint64_t) * 8)
				goto range_error;
			*((long long *) value_addr) |= 1 << nums[i];
			break;
		case CFG_MASK_32:
			if (nums[i] >= sizeof(uint32_t) * 8)
				goto range_error;
			*((uint32_t *) value_addr) |= 1 << nums[i];
			break;
		case CFG_MASK_16:
			if (nums[i] >= sizeof(uint16_t) * 8)
				goto range_error;
			*((uint16_t *) value_addr) |= 1 << nums[i];
			break;
		case CFG_MASK_8:
			if (nums[i] >= sizeof(uint8_t) * 8)
				goto range_error;
			*((uint8_t *) value_addr) |= 1 << nums[i];
			break;

		default:
			if (nums[i] >= sizeof(int) * 8)
				goto range_error;
			*((int *) value_addr) |= 1 << nums[i];
		}
	}

	return 0;

range_error:
	fprintf(stderr,
		"%d out of range for '--%s/-%c' \n",
		nums[i], opt->option, opt->short_option);
	return 1;
}

static int cfg_custom_handler(const char *optarg, void *value_addr,
			      const struct argconfig_options *opt)
{
	if (opt->custom_handler == NULL) {
		fprintf(stderr, "FATAL: custom handler not specified "
			"for CFG_CUSTOM!\n");
		return 1;
	}

	return opt->custom_handler(optarg, value_addr, opt);
}

typedef int (*type_handler)(const char *optarg, void *value_addr,
			    const struct argconfig_options *opt);

static type_handler cfg_type_handlers[_CFG_MAX_TYPES] = {
	[CFG_NONE] = cfg_none_handler,
	[CFG_STRING] = cfg_string_handler,
	[CFG_INT] = cfg_int_handler,
	[CFG_SIZE] = cfg_size_handler,
	[CFG_LONG] = cfg_long_handler,
	[CFG_LONG_LONG] = cfg_long_long_handler,
	[CFG_LONG_SUFFIX] = cfg_long_suffix_handler,
	[CFG_DOUBLE] = cfg_double_handler,
	[CFG_BOOL] = cfg_bool_handler,
	[CFG_BYTE] = cfg_byte_handler,
	[CFG_SHORT] = cfg_short_handler,
	[CFG_POSITIVE] = cfg_positive_handler,
	[CFG_INCREMENT] = cfg_increment_handler,
	[CFG_FILE_A] = cfg_file_handler,
	[CFG_FILE_A] = cfg_file_handler,
	[CFG_FILE_W] = cfg_file_handler,
	[CFG_FILE_R] = cfg_file_handler,
	[CFG_FILE_AP] = cfg_file_handler,
	[CFG_FILE_WP] = cfg_file_handler,
	[CFG_FILE_RP] = cfg_file_handler,
	[CFG_FD_WR] = cfg_fd_handler,
	[CFG_FD_RD] = cfg_fd_handler,
	[CFG_CHOICES] = cfg_choices_handler,
	[CFG_MULT_CHOICES] = cfg_choices_handler,
	[CFG_MASK] = cfg_mask_handler,
	[CFG_MASK_8] = cfg_mask_handler,
	[CFG_MASK_16] = cfg_mask_handler,
	[CFG_MASK_32] = cfg_mask_handler,
	[CFG_MASK_64] = cfg_mask_handler,
	[CFG_CUSTOM] = cfg_custom_handler,
};

static int handle(const char *optarg, void *value_addr,
		  const struct argconfig_options *s)
{
	if (!value_addr)
		return 0;

	if (s->cfg_type >= _CFG_MAX_TYPES ||
	    cfg_type_handlers[s->cfg_type] == NULL) {
		fprintf(stderr, "FATAL: unknown config type: %d\n",
			s->cfg_type);
		return 1;
	}

	return cfg_type_handlers[s->cfg_type](optarg, value_addr, s);
}

static const struct argconfig_options *
get_option(int c, int option_index,
	    const struct argconfig_options *options)
{
	const struct argconfig_options *s;

	if (c) {
		for (s = options; s->option; s++) {
			if (c == s->short_option)
				break;
		}

		if (s == NULL)
			return NULL;
	} else {
		for (s = options; s->option; s++) {
			if (is_positional(s))
				continue;
			if (!option_index--)
				return s;
		}
	}

	return s;

}

static const struct argconfig_options *
find_option(char *arg, const struct argconfig_options *options)
{
	const struct argconfig_options *s;

	while(arg[0]=='-') arg++;
	while(arg[strlen(arg)-1] == '=') arg[strlen(arg)-1] = 0;

	for (s = options; s->option; s++) {
		if (strcmp(s->option, arg) == 0)
			return s;
	}

	return NULL;
}


static void print_option_completions(const struct argconfig_options *s)
{
	const struct argconfig_choice *c;

	if (!s)
		exit(0);

	if (is_file_option(s))
		exit(2);

	switch(s->cfg_type) {
	case CFG_CHOICES:
	case CFG_MULT_CHOICES:
		if (!s->choices)
			exit(0);

		for (c = s->choices; c->name; c++) {
			printf(" %s", c->name);
		}


		break;
	default:
		break;
	}

	printf("\n");
	exit(0);
}


static void print_completions(int argc, char *argv[],
			      const char *short_opts,
			      const struct option *long_opts,
			      const struct argconfig_options *options)
{
	int pos_args = 0;
	const struct argconfig_options *s = NULL;
	int accept_file = 0;
	int c;
	int option_index = 0;
	char *last_optarg = NULL;

	opterr = 0;
	while ((c = getopt_long_only(argc, argv, short_opts, long_opts,
				     &option_index)) != -1) {
		if (c == ':') {
			if (optopt == 0)
				s = find_option(argv[optind-1], options);
			else
				s = get_option(optopt, 0, options);
			print_option_completions(s);
		} else {
			s = get_option(optopt, option_index, options);
			last_optarg = optarg;
		}
	};

	if (last_optarg && s && strlen(last_optarg) == 0)
		print_option_completions(s);

	pos_args = argc - optind;

	for (s = options; s->option; s++) {
		if (is_positional(s))
			continue;

		if (s->argument_type == no_argument)
			printf(" --%s", s->option);
		else
			printf(" --%s=", s->option);

		printf(" -%c", s->short_option);
	}

	for (s = options; s->option; s++) {
		if (!is_positional(s))
			continue;

		if (s->env && getenv(s->env))
			continue;

		if(pos_args-- > 0)
			continue;

		if (s->complete)
			printf(" %s", s->complete);

		if (pos_args != -1)
			continue;

		if (is_file_option(s))
			accept_file = 2;
	}

	printf("\n");

	exit(accept_file);
}

int argconfig_parse(int argc, char *argv[], const char *program_desc,
		    const struct argconfig_options *options,
		    void *config_out, size_t config_size)
{
	char *short_opts;
	struct option *long_opts;
	const struct argconfig_options *s;
	int c, option_index = 0, short_index = 0, options_count = 0;
	void *value_addr;

	errno = 0;
	options_count = num_options(options);
	long_opts = malloc(sizeof(struct option) * (options_count + 2));
	short_opts = malloc(sizeof(*short_opts) * (options_count * 3 + 4));

	short_opts[short_index++] = ':';

	for (s = options; (s->option != NULL) && (option_index < options_count);
	     s++) {

		if (is_positional(s))
			continue;

		if (s->short_option != 0) {
			short_opts[short_index++] = s->short_option;
			if (s->argument_type == required_argument ||
			    s->argument_type == optional_argument)
				short_opts[short_index++] = ':';
			if (s->argument_type == optional_argument)
				short_opts[short_index++] = ':';
		}
		if (s->option && strlen(s->option)) {
			long_opts[option_index].name = s->option;
			long_opts[option_index].has_arg = s->argument_type;

			if (s->argument_type == no_argument
			    && s->value_addr != NULL) {
				long_opts[option_index].flag = s->value_addr;
				long_opts[option_index].val = 1;
			} else {
				long_opts[option_index].flag = NULL;
				long_opts[option_index].val = 0;
			}
		}
		option_index++;
	}

	long_opts[option_index].name = "help";
	long_opts[option_index].flag = NULL;
	long_opts[option_index].val = 'h';
	option_index++;

	long_opts[option_index].name = NULL;
	long_opts[option_index].flag = NULL;
	long_opts[option_index].val = 0;

	short_opts[short_index++] = '?';
	short_opts[short_index++] = 'h';
	short_opts[short_index] = 0;

	if (getenv(COMPLETE_ENV))
		print_completions(argc, argv, short_opts, long_opts, options);

	optind = 0;
	while ((c = getopt_long_only(argc, argv, short_opts, long_opts,
				&option_index)) != -1) {
		if (c == '?' || c == 'h' || c == ':') {
			argconfig_print_help(program_desc, options);
			goto exit;
		}

		s = get_option(c, option_index, options);

		if (handle(optarg, s->value_addr, s))
		    goto exit;
	}

	for (s = options; (s->option != NULL); s++) {
		if (!is_positional(s))
			continue;

		if (s->env && getenv(s->env)) {
			if (handle(getenv(s->env), s->value_addr, s))
				goto exit;
			continue;
		}

		if (s->argument_type == required_positional &&
		    optind >= argc)
		{
			argconfig_print_usage(options);
			goto exit;
		}

		if (optind >= argc)
			continue;

		if (handle(argv[optind], s->value_addr, s))
		    goto exit;

		optind++;
	}

	for (s = options; (s->option != NULL); s++) {
		if (!s->force_default)
			continue;

		value_addr = (void *)(char *)s->value_addr;

		if (*(int *)value_addr)
			continue;

		if (handle(s->force_default, s->value_addr, s))
		    goto exit;
	}


	free(short_opts);
	free(long_opts);

	return 0;
exit:
	free(short_opts);
	free(long_opts);
	exit(1);
}

void argconfig_register_help_func(argconfig_help_func * f)
{
	int i;
	for (i = 0; i < MAX_HELP_FUNC; i++) {
		if (help_funcs[i] == NULL) {
			help_funcs[i] = f;
			help_funcs[i + 1] = NULL;
			break;
		}
	}
}

int argconfig_parse_comma_range(const char *str, int *res, int max_nums)
{
	char buf[strlen(str)];
	char *tok;
	const char *delims = " ,";
	int start, end;
	int cnt = 0;

	strcpy(buf, str);
	tok = strtok(buf, delims);

	while (tok != NULL) {
		start = strtol(tok, &tok, 0);
		if (*tok == '-') {
			end = strtol(tok+1, &tok, 0);
			for (; start <= end; start++)
				res[cnt++] = start;
		} else {
			res[cnt++] = start;
		}

		if (cnt == max_nums)
			return -1;

		if (*tok != 0)
			return -1;

		tok = strtok(NULL, delims);
	}

	return cnt;
}
