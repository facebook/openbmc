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

/*
 * This is the source file of GPIO Control Command Line Interface. Run
 * "gpiocli -h|--help" on openbmc for usage information.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <getopt.h>

#include <openbmc/libgpio.h>
#include <openbmc/misc-utils.h>

/*
 * Logging utilities.
 */
#define GCLI_DEBUG(fmt, args...)		\
	do {					\
		if (verbose_flag)		\
			printf(fmt, ##args);	\
	} while (0)
#define GCLI_ERR(fmt, args...)	fprintf(stderr, fmt, ##args)

/*
 * Macros for validating and accessing "gcli_cmd_args" structure.
 */
#define SHADOW_ENTRY(args)	(args)->gpio_pin.shadow
#define CHIP_NAME_PAIR(args)	(args)->gpio_pin.chip, (args)->gpio_pin.name
#define CHIP_OFFSET_PAIR(args)	(args)->gpio_pin.chip, (args)->gpio_pin.offset

#define IS_VALID_SHADOW(args)		((args)->gpio_pin.shadow != NULL)
#define IS_VALID_CHIP_NAME_PAIR(args)	((args)->gpio_pin.chip != NULL && \
					 (args)->gpio_pin.name != NULL)
#define IS_VALID_CHIP_OFFSET_PAIR(args)	((args)->gpio_pin.chip != NULL && \
					 (args)->gpio_pin.offset >= 0)

struct gcli_cmd_args {
	bool dump_usage;

	char *gpio_cmd;
	char *gpio_value;
	struct {
		char *shadow;
		char *chip;
		char *name;
		int offset;
	} gpio_pin;
};

struct gcli_cmd_info {
	char *cmd;
	char *desc;
	int (*func)(struct gcli_cmd_args *args);
};

static bool verbose_flag = false;

static int gcli_cmd_export(struct gcli_cmd_args *args)
{
	int error;

	if (!IS_VALID_SHADOW(args)) {
		GCLI_ERR("Error: shadow is required for %s command\n",
			 args->gpio_cmd);
		return -1;
	}

	if (IS_VALID_CHIP_NAME_PAIR(args)) {
		GCLI_DEBUG("export (%s, %s) pin and set up shadow %s\n",
			   CHIP_NAME_PAIR(args), SHADOW_ENTRY(args));
		error = gpio_export_by_name(CHIP_NAME_PAIR(args),
					    SHADOW_ENTRY(args));
		if (error != 0)
			GCLI_ERR("failed to export (%s, %s): %s\n",
				 CHIP_NAME_PAIR(args), strerror(errno));
	} else if (IS_VALID_CHIP_OFFSET_PAIR(args)) {
		GCLI_DEBUG("export (%s, %d) pin and set up shadow %s\n",
			   CHIP_OFFSET_PAIR(args), SHADOW_ENTRY(args));
		error = gpio_export_by_offset(CHIP_OFFSET_PAIR(args),
					      SHADOW_ENTRY(args));
		if (error != 0)
			GCLI_ERR("failed to export (%s, %d): %s\n",
				 CHIP_OFFSET_PAIR(args), strerror(errno));
	} else {
		GCLI_ERR("Error: either (chip, name) or (chip, offset) "
			 "is needed for %s command\n",
			 args->gpio_cmd);
		error = -1;
	}

	return error;
}

static int gcli_cmd_unexport(struct gcli_cmd_args *args)
{
	int error;

	if (!IS_VALID_SHADOW(args)) {
		GCLI_ERR("Error: shadow is required for %s command\n",
			 args->gpio_cmd);
		return -1;
	}

	GCLI_DEBUG("unexport gpio pin %s\n", SHADOW_ENTRY(args));
	error = gpio_unexport(SHADOW_ENTRY(args));
	if (error != 0)
		GCLI_ERR("failed to unexport gpio (shadow=%s): %s\n",
			 SHADOW_ENTRY(args), strerror(errno));

	return error;
}

static int gcli_cmd_list_chips(struct gcli_cmd_args *args)
{
	int i, num_chips;
	char type[64], device[64];
	gpiochip_desc_t *chips[GPIO_CHIP_MAX];

	num_chips = gpiochip_list(chips, ARRAY_SIZE(chips));
	if (num_chips < 0) {
		GCLI_ERR("failed to list gpio chips: %s\n",
			 strerror(errno));
		return -1;
	}

	printf("Total %d gpio chips:\n", num_chips);
	for (i = 0; i < num_chips; i++) {
		printf("[%d] type=%s, device=%s, base=%d, ngpio=%d\n",
			i + 1,
			gpiochip_get_type(chips[i], type, sizeof(type)),
			gpiochip_get_device(chips[i], device, sizeof(device)),
			gpiochip_get_base(chips[i]),
			gpiochip_get_ngpio(chips[i]));
	}
	return 0;
}

static gpio_desc_t* gcli_open_pin(struct gcli_cmd_args *args)
{
	gpio_desc_t *gdesc = NULL;

	if (IS_VALID_SHADOW(args)) {
		GCLI_DEBUG("open gpio pin %s\n", SHADOW_ENTRY(args));
		gdesc = gpio_open_by_shadow(SHADOW_ENTRY(args));
		if (gdesc == NULL)
			GCLI_ERR("failed to open gpio (shadow=%s): %s\n",
				 SHADOW_ENTRY(args), strerror(errno));
	} else if (IS_VALID_CHIP_NAME_PAIR(args)) {
		GCLI_DEBUG("open gpio pin (%s, %s)\n",
			   CHIP_NAME_PAIR(args));
		gdesc = gpio_open_by_name(CHIP_NAME_PAIR(args));
		if (gdesc == NULL)
			GCLI_ERR("failed to open (%s, %s): %s\n",
				 CHIP_NAME_PAIR(args), strerror(errno));
	} else if (IS_VALID_CHIP_OFFSET_PAIR(args)) {
		GCLI_DEBUG("open gpio pin (%s, %d)\n",
			   CHIP_OFFSET_PAIR(args));
		gdesc = gpio_open_by_offset(CHIP_OFFSET_PAIR(args));
		if (gdesc == NULL)
			GCLI_ERR("failed to open (%s, %d): %s\n",
				 CHIP_OFFSET_PAIR(args), strerror(errno));
	} else {
		GCLI_ERR("Error: either shadow or (chip, name/offset) "
			 "is needed for <%s> command\n",
			 args->gpio_cmd);
	}

	return gdesc;
}

static int gcli_cmd_get_value(struct gcli_cmd_args *args)
{
	gpio_desc_t *gdesc;
	gpio_value_t value;

	gdesc = gcli_open_pin(args);
	if (gdesc == NULL)
		return -1;

	GCLI_DEBUG("read gpio pin value\n");
	if (gpio_get_value(gdesc, &value) != 0) {
		GCLI_ERR("failed to get gpio value: %s\n",
			 strerror(errno));
		goto error;
	}

	printf("gpio_value=%d\n", (int)value);
	return 0;

error:
	gpio_close(gdesc);
	return -1;
}

static int gcli_cmd_set_value(struct gcli_cmd_args *args)
{
	int input;
	gpio_desc_t *gdesc;
	gpio_value_t value;

	if (args->gpio_value == NULL) {
		GCLI_ERR("gpio value is required for %s command\n",
			 args->gpio_cmd);
		return -1;
	}

	gdesc = gcli_open_pin(args);
	if (gdesc == NULL)
		return -1;

	input = (int)strtol(args->gpio_value, NULL, 0);
	value = (input ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
	GCLI_DEBUG("set gpio pin value to %d\n", (int)value);
	if (gpio_set_value(gdesc, value) != 0) {
		GCLI_ERR("failed to set gpio value: %s\n",
			 strerror(errno));
		goto error;
	}

	return 0;

error:
	gpio_close(gdesc);
	return -1;
}

static int gcli_cmd_get_direction(struct gcli_cmd_args *args)
{
	gpio_desc_t *gdesc;
	gpio_direction_t dir;

	gdesc = gcli_open_pin(args);
	if (gdesc == NULL)
		return -1;

	if (gpio_get_direction(gdesc, &dir) != 0) {
		GCLI_ERR("failed to get gpio direction: %s\n",
			 strerror(errno));
		goto error;
	}

	printf("gpio_direction=%s\n",
		dir == GPIO_DIRECTION_IN ? "in" : "out");
	return 0;

error:
	gpio_close(gdesc);
	return -1;
}

static int gcli_cmd_set_direction(struct gcli_cmd_args *args)
{
	gpio_desc_t *gdesc;
	gpio_direction_t dir;

	if (args->gpio_value == NULL) {
		GCLI_ERR("gpio direction is required for %s command\n",
			 args->gpio_cmd);
		return -1;
	}
	if (strcmp(args->gpio_value, "in") == 0) {
		dir = GPIO_DIRECTION_IN;
	} else if (strcmp(args->gpio_value, "out") == 0) {
		dir = GPIO_DIRECTION_OUT;
	} else {
		GCLI_ERR("invalid direction %s: "
			 "valid input is 'in' or 'out'\n",
			 args->gpio_value);
		return -1;
	}

	gdesc = gcli_open_pin(args);
	if (gdesc == NULL)
		return -1;

	if (gpio_set_direction(gdesc, dir) != 0) {
		GCLI_ERR("failed to set gpio direction: %s\n",
			 strerror(errno));
		goto error;
	}

	return 0;

error:
	gpio_close(gdesc);
	return -1;
}

static struct {
	gpio_edge_t val;
	const char *str;
} gpio_edge_types[] = {
	{GPIO_EDGE_NONE,	"none"},
	{GPIO_EDGE_RISING,	"rising"},
	{GPIO_EDGE_FALLING,	"falling"},
	{GPIO_EDGE_BOTH,	"both"},
	{GPIO_EDGE_INVALID,	NULL},
};

static const char* gpio_edge_val_to_str(gpio_edge_t edge)
{
	int i;

	for (i = 0; gpio_edge_types[i].str != NULL; i++) {
		if (gpio_edge_types[i].val == edge)
			return gpio_edge_types[i].str;
	}

	return "invalid-gpio-edge";
}

static gpio_edge_t gpio_edge_str_to_val(const char *str)
{
	int i;

	for (i = 0; gpio_edge_types[i].str != NULL; i++) {
		if (strcmp(str, gpio_edge_types[i].str) == 0)
			return gpio_edge_types[i].val;
	}

	GCLI_ERR("Invalid gpio edge %s: valid types are: \n", str);
	for (i = 0; gpio_edge_types[i].str != NULL; i++)
		GCLI_ERR("    %s\n", gpio_edge_types[i].str);
	
	return GPIO_EDGE_INVALID;
}

static int gcli_cmd_get_edge(struct gcli_cmd_args *args)
{
	gpio_desc_t *gdesc;
	gpio_edge_t edge;

	gdesc = gcli_open_pin(args);
	if (gdesc == NULL)
		return -1;

	if (gpio_get_edge(gdesc, &edge) != 0) {
		GCLI_ERR("failed to get gpio edge: %s\n",
			 strerror(errno));
		goto error;
	}

	printf("gpio_edge=%s\n", gpio_edge_val_to_str(edge));
	return 0;

error:
	gpio_close(gdesc);
	return -1;
}

static int gcli_cmd_set_edge(struct gcli_cmd_args *args)
{
	gpio_desc_t *gdesc;
	gpio_edge_t edge;

	if (args->gpio_value == NULL) {
		GCLI_ERR("gpio edge is required for %s command\n",
			 args->gpio_cmd);
		return -1;
	}

	edge = gpio_edge_str_to_val(args->gpio_value);
	if (edge == GPIO_EDGE_INVALID)
		return -1;

	gdesc = gcli_open_pin(args);
	if (gdesc == NULL)
		return -1;

	if (gpio_set_edge(gdesc, edge) != 0) {
		GCLI_ERR("failed to set gpio edge: %s\n",
			 strerror(errno));
		goto error;
	}

	return 0;

error:
	gpio_close(gdesc);
	return -1;
}

static int gcli_cmd_pin_name_to_offset(struct gcli_cmd_args *args)
{
	int offset;
	gpiochip_desc_t *gcdesc;

	if (!IS_VALID_CHIP_NAME_PAIR(args)) {
		GCLI_ERR("(chip, name) pair is required for %s command\n",
			 args->gpio_cmd);
		return -1;
	}

	gcdesc = gpiochip_lookup(args->gpio_pin.chip);
	if (gcdesc == NULL) {
		GCLI_ERR("failed to locate gpio chip %s\n",
			 args->gpio_pin.chip);
		return -1;
	}

	offset = gpiochip_pin_name_to_offset(gcdesc, args->gpio_pin.name);
	if (offset < 0) {
		GCLI_ERR("failed to map (%s, %s) to offset: %s\n",
			 CHIP_NAME_PAIR(args), strerror(errno));
		return -1;
	}

	printf("(%s, %s), offset=%d\n", CHIP_NAME_PAIR(args), offset);
	return 0;
}

static struct gcli_cmd_info gcli_cmds[] = {
	{
		"export",
		"export control of the gpio pin to user space",
		gcli_cmd_export,
	},
	{
		"unexport",
		"unexport control of the gpio pin to user space",
		gcli_cmd_unexport,
	},
	{
		"list-chips",
		"list all the gpio chips found on the system",
		gcli_cmd_list_chips,
	},
	{
		"get-value",
		"get the value of the given gpio pin",
		gcli_cmd_get_value,
	},
	{
		"set-value",
		"set the value of the given gpio pin",
		gcli_cmd_set_value,
	},
	{
		"get-direction",
		"get the direction of the given gpio pin",
		gcli_cmd_get_direction,
	},
	{
		"set-direction",
		"set the direction of the given gpio pin",
		gcli_cmd_set_direction,
	},
	{
		"get-edge",
		"get the edge of the given gpio pin",
		gcli_cmd_get_edge,
	},
	{
		"set-edge",
		"set the edge of the given gpio pin",
		gcli_cmd_set_edge,
	},
	{
		"map-name-to-offset",
		"translate pin name to offset within the gpio chip",
		gcli_cmd_pin_name_to_offset,
	},

	/* This is the last entry. */
	{
		NULL,
		NULL,
		NULL,
	},
};

static struct gcli_cmd_info*
gcli_match_cmd(const char *cmd)
{
	int i;

	for (i = 0; gcli_cmds[i].cmd != NULL; i++) {
		if (strcmp(cmd, gcli_cmds[i].cmd) == 0) {
			return &gcli_cmds[i];
		}
	}

	return NULL;
}

static void gcli_dump_cmds(void)
{
	int i;

	for (i = 0; gcli_cmds[i].cmd != NULL; i++) {
		printf("    %-18s - %s\n",
		       gcli_cmds[i].cmd, gcli_cmds[i].desc);
	}
}

static void gcli_usage(const char *prog_name)
{
	int i;
	struct {
		char *opt;
		char *desc;
	} options[] = {
		{"-h|--help", "print this help message"},
		{"-v|--verbose", "enable verbose logging"},
		{"-s|--shadow", "specify shadow name of the gpio pin"},
		{"-c|--chip", "specify the name of gpio chip"},
		{"-n|--pin-name", "specify the name of the gpio pin"},
		{"-o|--pin-offset", "specify the offset of the gpio pin"},
		{NULL, NULL},
	};

	/* Print available options */
	printf("Usage: %s [options] <gpio-command>\n", prog_name);

	printf("\nAvailable options:\n");
	for (i = 0; options[i].opt != NULL; i++) {
		printf("    %-16s  - %s\n", options[i].opt, options[i].desc);
	}

	/* Print available commands */
	printf("\nAvailable gpio commands:\n");
	gcli_dump_cmds();
}

static int parse_cmdline_args(int argc, char **argv,
			      struct gcli_cmd_args *cmd_args)
{
	int ret, offset, opt_index;
	struct option long_opts[] = {
		{"help",	no_argument,		NULL,	'h'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"shadow",	required_argument,	NULL,	's'},
		{"chip",	required_argument,	NULL,	'c'},
		{"pin-name",	required_argument,	NULL,	'n'},
		{"pin-offset",	required_argument,	NULL,	'o'},
		{NULL,		0,			NULL,	0},
	};

	while (1) {
		opt_index = 0;
		ret = getopt_long(argc, argv, "hvs:c:n:o:",
				  long_opts, &opt_index);
		if (ret == -1)
			break;	/* end of arguments */

		switch (ret) {
		case 'h':
			cmd_args->dump_usage = true;
			return 0;

		case 'v':
			verbose_flag = true;
			break;

		case 's':
			cmd_args->gpio_pin.shadow = optarg;
			break;

		case 'c':
			cmd_args->gpio_pin.chip = optarg;
			break;

		case 'n':
			cmd_args->gpio_pin.name = optarg;
			break;

		case 'o':
			offset = (int)strtol(optarg, NULL, 0);
			cmd_args->gpio_pin.offset = offset;
			break;

		default:
			return -1;
		}
	} /* while */

	if (optind >= argc) {
		GCLI_ERR("Error: gpio command is missing!\n\n");
		return -1;
	}

	cmd_args->gpio_cmd = argv[optind++];
	if (optind < argc)
		cmd_args->gpio_value = argv[optind];
	return 0;
}

int main(int argc, char **argv)
{
	struct gcli_cmd_args cmd_args;
	struct gcli_cmd_info *cmd_info;

	memset(&cmd_args, 0, sizeof(cmd_args));
	cmd_args.gpio_pin.offset = -1;
	if (parse_cmdline_args(argc, argv, &cmd_args) != 0) {
		gcli_usage(argv[0]);
		return -1;
	} else if (cmd_args.dump_usage) {
		gcli_usage(argv[0]);
		return 0;
	}

	cmd_info = gcli_match_cmd(cmd_args.gpio_cmd);
	if (cmd_info == NULL) {
		GCLI_ERR("Error: invalid command <%s> received!\n\n",
			 cmd_args.gpio_cmd);
		gcli_usage(argv[0]);
		return -1;
	}

	GCLI_DEBUG("run %s command, value=%s, "
		   "args=(shadow=%s, chip=%s, name=%s, offset=%d)\n",
		   cmd_args.gpio_cmd,
		   cmd_args.gpio_value,
		   cmd_args.gpio_pin.shadow,
		   cmd_args.gpio_pin.chip,
		   cmd_args.gpio_pin.name,
		   cmd_args.gpio_pin.offset);
	return cmd_info->func(&cmd_args);
}
