/*
 * ftdi-bitbang
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "m:s:c:i:r";
struct option longopts[] = {
    COMMON_LONG_OPTS
    { "mode", required_argument, NULL, 'm' },
    { "set", required_argument, NULL, 's' },
    { "clr", required_argument, NULL, 'c' },
    { "inp", required_argument, NULL, 'i' },
    { "read", optional_argument, NULL, 'r' },
    { 0, 0, 0, 0 },
};

/* which pin state to read or -1 for all (hex output then) */
int read_pin = -2;
int pins[16];

/* ftdi device context */
struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_context *device = NULL;
int bitmode = 0;
int no_option = 1;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
    if (device) {
        ftdi_bitbang_save_state(device);
        ftdi_bitbang_free(device);
    }
    if (ftdi) {
        ftdi_free(ftdi);
    }
    /* terminate program instantly */
    exit(return_code);
}

void p_help()
{
    printf(
        "  -m, --mode=STRING          set device bitmode, use 'bitbang' or 'mpsse', default is 'bitbang'\n"
        "                             for bitbang mode the baud rate is fixed to 1 MHz for now\n"
        "  -s, --set=PIN              given pin as output and one\n"
        "  -c, --clr=PIN              given pin as output and zero\n"
        "  -i, --inp=PIN              given pin as input\n"
        "                             multiple -s, -c and -i options can be given\n"
        "  -r, --read                 read pin states, output hexadecimal word\n"
        "      --read=PIN             read single pin, output binary 0 or 1\n"
        "\n"
        "Simple command line bitbang interface to FTDI FTx232 chips.\n"
        "\n");
}

int p_options(int c, char *optarg)
{
    int i;
    switch (c) {
    case 'm':
        if (strcmp("bitbang", optarg) == 0) {
            bitmode = BITMODE_BITBANG;
        } else if (strcmp("mpsse", optarg) == 0) {
            bitmode = BITMODE_MPSSE;
        } else {
            fprintf(stderr, "invalid bitmode\n");
            return -1;
        }
        return 1;
    case 'c':
    case 's':
    case 'i':
        no_option=0;
        i = atoi(optarg);
        if (i < 0 || i > 15) {
            fprintf(stderr, "invalid pin number: %d\n", i);
            p_exit(1);
        } else {
            /* s = out&one, c = out&zero, i = input */
            pins[i] = c == 's' ? 1 : (c == 'i' ? 2 : 0);
        }
        return 1;
    case 'r':
        no_option=0;
        read_pin = -1;
        if (optarg) {
            read_pin = atoi(optarg);
        }
        if (read_pin < -1 || read_pin > 15) {
            fprintf(stderr, "invalid pin number for read parameter: %d\n", i);
            p_exit(1);
        }
        return 1;
    }

    return 0;
}

int read_pins(int pin)
{
    if (pin == -1) {
        int pins = ftdi_bitbang_read(device);
        if (pins < 0) {
            fprintf(stderr, "failed reading pin states\n");
            p_exit(EXIT_FAILURE);
        }
        printf("%04x\n", pins);
    } else if (pin >= 0 && pin < 8) {
        int pins = ftdi_bitbang_read_low(device);
        if (pins < 0) {
            fprintf(stderr, "failed reading pin state\n");
            p_exit(EXIT_FAILURE);
        }
        printf("%d\n", pins & (1 << pin) ? 1 : 0);
    } else if (pin > 7 && pin < 16) {
        int pins = ftdi_bitbang_read_high(device);
        if (pins < 0) {
            fprintf(stderr, "failed reading pin state\n");
            p_exit(EXIT_FAILURE);
        }
        pin -= 8;
        printf("%d\n", pins & (1 << pin) ? 1 : 0);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int err = 0, i, changed = 0;

    for (i = 0; i < 16; i++) {
        pins[i] = -1;
    }

    /* parse command line options */
    if (common_options(argc, argv, opts, longopts, 0, 1)) {
        fprintf(stderr, "invalid command line option(s)\n");
        p_exit(EXIT_FAILURE);
    }

    if (no_option) {
        fprintf(stderr, "no action, need command line option(s)\n");
        common_help(argc, argv);
        p_exit(EXIT_FAILURE);
    }

    /* init ftdi things */
    ftdi = common_ftdi_init();
    if (!ftdi) {
        p_exit(EXIT_FAILURE);
    }

    /* initialize to bitbang mode */
    device = ftdi_bitbang_init(ftdi, bitmode, 1);
    if (!device) {
        fprintf(stderr, "ftdi_bitbang_init() failed\n");
        p_exit(EXIT_FAILURE);
    }

    /* write changes */
    for (i = 0; i < 16; i++) {
        err = 0;
        if (pins[i] == 0 || pins[i] == 1) {
            err += ftdi_bitbang_set_io(device, i, 1);
            err += ftdi_bitbang_set_pin(device, i, pins[i] ? 1 : 0);
            changed++;
        } else if (pins[i] == 2) {
            err += ftdi_bitbang_set_io(device, i, 0);
            err += ftdi_bitbang_set_pin(device, i, 0);
            changed++;
        }
        if (err != 0) {
            fprintf(stderr, "invalid pin #%d (you are propably trying to use upper pins in bitbang mode)\n", i);
        }
    }
    if (ftdi_bitbang_write(device) < 0) {
        fprintf(stderr, "failed writing pin states\n");
        p_exit(EXIT_FAILURE);
    }

    /* read pin(s) if set so in options */
    if (read_pin > -2) {
        read_pins(read_pin);
        changed++;
    }

    /* apply arguments */
    // for (int i = optind; i < argc; i++) {
    //     printf("use: %s\n", argv[i]);
    // }

    /* apply stdin */
    for (;;) {
        unsigned char *line = common_stdin_read();
        if (!line) {
            break;
        }
        printf("line: \"%s\"\n", line);
        changed++;
    }

    /* print error if nothing was done */
    if (!changed) {
        fprintf(stderr, "nothing done, no actions given.\n");
        p_exit(EXIT_FAILURE);
    }

    p_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

