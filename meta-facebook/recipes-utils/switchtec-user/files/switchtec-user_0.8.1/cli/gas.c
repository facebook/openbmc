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
#include "common.h"

#include <switchtec/switchtec.h>

#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

static int spawn_proc(int fd_in, int fd_out, int fd_close,
		      const char *cmd)
{
	pid_t pid;

	pid = fork();

	if (pid != 0)
		return pid;

	close(fd_close);

	if (fd_in != STDIN_FILENO) {
		dup2(fd_in, STDIN_FILENO);
		close(fd_in);
	}

	if (fd_out != STDOUT_FILENO) {
		dup2(fd_out, STDOUT_FILENO);
		close(fd_out);
	}

	return execlp(cmd, cmd, NULL);
}

static int pipe_to_hd_less(void *map, size_t map_size)
{
	int hd_fds[2];
	int less_fds[2];
	int less_pid, hd_pid;
	int ret;

	ret = pipe(less_fds);
	if (ret) {
		perror("pipe");
		return -1;
	}

	less_pid = spawn_proc(less_fds[0], STDOUT_FILENO, less_fds[1], "less");
	if (less_pid < 0) {
		perror("less");
		return -1;
	}
	close(STDOUT_FILENO);
	close(less_fds[0]);

	ret = pipe(hd_fds);
	if (ret) {
		perror("pipe");
		return -1;
	}

	hd_pid = spawn_proc(hd_fds[0], less_fds[1], hd_fds[1], "hd");
	if (hd_pid < 0) {
		perror("hd");
		return -1;
	}

	close(hd_fds[0]);
	close(less_fds[1]);

	ret = write(hd_fds[1], map, map_size);
	close(hd_fds[1]);
	waitpid(less_pid, NULL, 0);
	return ret;
}

static int gas_dump(int argc, char **argv)
{
	const char *desc = "Dump all gas registers";
	void *map;
	size_t map_size;
	int ret;

	static struct {
		struct switchtec_dev *dev;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	map = switchtec_gas_map(cfg.dev, 0, &map_size);
	if (map == MAP_FAILED) {
		switchtec_perror("gas_map");
		return 1;
	}

	if (!isatty(STDOUT_FILENO)) {
		ret = write(STDOUT_FILENO, map, map_size);
		return ret > 0;
	}

	return pipe_to_hd_less(map, map_size);
}

static int print_hex(void *addr, int offset, int bytes)
{
	unsigned long long x;

	offset = offset & ~(bytes - 1);

	switch (bytes) {
	case 1: x = *((uint8_t *)(addr + offset));  break;
	case 2: x = *((uint16_t *)(addr + offset)); break;
	case 4: x = *((uint32_t *)(addr + offset)); break;
	case 8: x = *((uint64_t *)(addr + offset)); break;
	default:
		fprintf(stderr, "invalid access width\n");
		return -1;
	}

	printf("%06X - 0x%0*llX\n", offset, bytes * 2, x);
	return 0;
}

static int print_dec(void *addr, int offset, int bytes)
{
	unsigned long long x;

	offset = offset & ~(bytes - 1);

	switch (bytes) {
	case 1: x = *((uint8_t *)(addr + offset));  break;
	case 2: x = *((uint16_t *)(addr + offset)); break;
	case 4: x = *((uint32_t *)(addr + offset)); break;
	case 8: x = *((uint64_t *)(addr + offset)); break;
	default:
		fprintf(stderr, "invalid access width\n");
		return -1;
	}

	printf("%06X - %lld\n", offset, x);
	return 0;
}

static int print_str(void *addr, int offset, int bytes)
{
	char buf[bytes + 1];

	memset(buf, 0, bytes + 1);
	memcpy(buf, addr + offset, bytes);

	printf("%06X - %s\n", offset, buf);
	return 0;
}

enum {
	HEX,
	DEC,
	STR,
};

int (*print_funcs[])(void *addr, int offset, int bytes) = {
	[HEX] = print_hex,
	[DEC] = print_dec,
	[STR] = print_str,
};

static int gas_read(int argc, char **argv)
{
	const char *desc = "Read a gas register";
	int i;
	int ret = 0;

	struct argconfig_choice print_choices[] = {
		{"hex", HEX, "print in hexadecimal"},
		{"dec", DEC, "print in decimal"},
		{"str", STR, "print as an ascii string"},
		{0},
	};

	static struct {
		struct switchtec_dev *dev;
		uint64_t addr;
		unsigned count;
		unsigned bytes;
		unsigned print_style;
	} cfg = {
		.bytes=4,
		.count=1,
		.print_style=HEX,
	};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"addr", 'a', "ADDR", CFG_LONG_LONG, &cfg.addr, required_argument,
		 "address to read"},
		{"bytes", 'b', "NUM", CFG_POSITIVE, &cfg.bytes, required_argument,
		 "number of bytes to read per access (default 4)"},
		{"count", 'n', "NUM", CFG_POSITIVE, &cfg.count, required_argument,
		 "number of accesses to performe (default 1)"},
		{"print", 'p', "STYLE", CFG_CHOICES, &cfg.print_style, required_argument,
		 "printing style", .choices=print_choices},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if ((1 != cfg.bytes)
			&& (2 != cfg.bytes)
			&& (4 != cfg.bytes)
			&& (8 != cfg.bytes)) {
		fprintf(stderr, "invalid access width\n");
		return -1;
	}

	for (i = 0; i < cfg.count; i++) {
		uint32_t offset;
		uint8_t buf[8];
		offset = cfg.addr & ~(cfg.bytes - 1);
		ret = switchtec_gas_read(cfg.dev, buf, offset, cfg.bytes);

		ret = print_funcs[cfg.print_style](buf, 0, cfg.bytes);
		cfg.addr += cfg.bytes;
		if (ret)
			break;
	}

	return ret;
}

static int gas_write(int argc, char **argv)
{
	const char *desc = "Write a gas register";
	int ret = 0;

	static struct {
		struct switchtec_dev *dev;
		uint64_t addr;
		unsigned bytes;
		uint64_t value;
		int assume_yes;
	} cfg = {
		.bytes=4,
	};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"addr", 'a', "ADDR", CFG_LONG_LONG, &cfg.addr, required_argument,
		 "address to read"},
		{"bytes", 'b', "NUM", CFG_POSITIVE, &cfg.bytes, required_argument,
		 "number of bytes to read per access (default 4)"},
		{"value", 'v', "ADDR", CFG_LONG_LONG, &cfg.value, required_argument,
		 "value to write"},
		{"yes", 'y', "", CFG_NONE, &cfg.assume_yes, no_argument,
		 "assume yes when prompted"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if ((1 != cfg.bytes)
			&& (2 != cfg.bytes)
			&& (4 != cfg.bytes)
			&& (8 != cfg.bytes)) {
		fprintf(stderr, "invalid access width\n");
		return -1;
	}

	if (!cfg.assume_yes)
		fprintf(stderr,
				"Writing 0x%llx to %06llx (%d bytes).\n",
				(unsigned long long)cfg.value, (unsigned long long)cfg.addr, cfg.bytes);

	ret = ask_if_sure(cfg.assume_yes);
	if (ret)
		return ret;

	ret = switchtec_gas_write(cfg.dev, (uint8_t *)&cfg.value,
			cfg.addr, cfg.bytes);

	if (ret)
		fprintf(stderr, "Write failed.\n");

	return ret;
}

static const struct cmd commands[] = {
	{"dump", gas_dump, "dump the global address space"},
	{"read", gas_read, "read a register from the global address space"},
	{"write", gas_write, "write a register in the global address space"},
	{}
};

static struct subcommand subcmd = {
	.name = "gas",
	.cmds = commands,
	.desc = "Global Address Space Access (dangerous)",
	.long_desc = "These functions should be used with extreme caution only "
	      "if you know what you are doing. Any register accesses through "
	      "this interface is unsupported by Microsemi unless specifically "
	      "otherwise specified.",
};

REGISTER_SUBCMD(subcmd);
