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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <libgen.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <libftdi1/ftdi.h>
#include "cmd-common.h"

/* only list */
int only_list = 1;
/* usb vid */
uint16_t usb_vid = 0;
/* usb pid */
uint16_t usb_pid = 0;
/* usb description */
const char *usb_description = NULL;
/* usb serial */
const char *usb_serial = NULL;
/* usb id */
const char *usb_id = NULL;
/* interface (defaults to first one) */
int interface = INTERFACE_ANY;
/* reset flag, reset usb device if this is set */
int reset = 0;


void common_help(int argc, char *argv[])
{
    printf(
        "\n"
        "Usage:\n"
        " %s [options]\n"
        "\n"
        "Definitions for options:\n"
        " ID = hexadecimal word\n"
        " PIN = decimal between 0 and 15\n"
        " INTERFACE = integer between 1 and 4 depending on device type\n"
        "\n"
        "Options:\n"
        "  -h, --help                 display this help and exit\n"
        "  -V, --vid=ID               usb vendor id\n"
        "  -P, --pid=ID               usb product id\n"
        "                             as default vid and pid are zero, so any first compatible ftdi device is used\n"
        "  -D, --description=STRING   usb description (product) to use for opening right device, default none\n"
        "  -S, --serial=STRING        usb serial to use for opening right device, default none\n"
        "  -I, --interface=INTERFACE  ftx232 interface number, defaults to first\n"
        "  -U, --usbid=ID             usbid to use for opening right device (sysfs format, e.g. 1-2.3), default none\n"
        "  -R, --reset                do usb reset on the device at start\n"
        "  -L, --list                 list devices that can be found with given parameters\n"
        "\n"
        , basename(argv[0]));
    p_help();
}

int common_options(int argc, char *argv[], const char opts[], struct option longopts[], int need_args, int no_opts_needed)
{
    int err = 0;
    int longindex = 0, c;
    int i;

    while ((c = getopt_long(argc, argv, opts, longopts, &longindex)) > -1) {
        /* check for command specific options */
        err = p_options(c, optarg);
        if (err > 0) {
            only_list = only_list < 2 ? 0 : only_list;
            continue;
        } else if (err < 0) {
            common_help(argc, argv);
            p_exit(1);
        }
        /* check for common options */
        switch (c) {
        case 'V':
            i = (int)strtol(optarg, NULL, 16);
            if (errno == ERANGE || i < 0 || i > 0xffff) {
                fprintf(stderr, "invalid usb vid value\n");
                p_exit(1);
            }
            usb_vid = (uint16_t)i;
            break;
        case 'P':
            i = (int)strtol(optarg, NULL, 16);
            if (errno == ERANGE || i < 0 || i > 0xffff) {
                fprintf(stderr, "invalid usb pid value\n");
                p_exit(1);
            }
            usb_pid = (uint16_t)i;
            break;
        case 'D':
            usb_description = strdup(optarg);
            break;
        case 'S':
            usb_serial = strdup(optarg);
            break;
        case 'U':
            usb_id = strdup(optarg);
            break;
        case 'I':
            interface = atoi(optarg);
            if (interface < 0 || interface > 4) {
                fprintf(stderr, "invalid interface\n");
                p_exit(1);
            }
            break;
        case 'R':
            reset = 1;
            break;
        case 'L':
            only_list = 2;
            break;
        default:
        case '?':
        case 'h':
            common_help(argc, argv);
            p_exit(1);
        }
    }

    if (need_args) {
        if (argc <= optind) {
            common_help(argc, argv);
            p_exit(1);
        }
        only_list = 0;
    } else if (argc < optind) {
        only_list = only_list < 2 ? 0 : only_list;
    }

    if (only_list == 2 || (only_list == 1 && !no_opts_needed)) {
        common_ftdi_list_print();
        p_exit(0);
    }

    return 0;
}

/* @note needs to be freed */
static char *get_usbid(struct libusb_device *dev)
{
    int n, i = 0;
    uint8_t port_numbers[7];
    FILE *fh;
    char *buf;
    size_t len;

    n = libusb_get_port_numbers(dev, port_numbers, sizeof(port_numbers) / sizeof(port_numbers[0]));
    if (n == LIBUSB_ERROR_OVERFLOW) {
        fprintf(stderr, "device has too many port numbers\n");
        return NULL;
    }

    fh = open_memstream(&buf, &len);
    if (!fh)
        return NULL;

    fprintf(fh, "%d-", libusb_get_bus_number(dev));
    for (;;) {
        fprintf(fh, "%d", port_numbers[i]);
        if (++i == n)
            break;
        fputc('.', fh);
    }
    fclose(fh);

    return buf;
}

static int usbid_is_match(struct libusb_device *dev)
{
    char *id;
    int ret;

    if (!usb_id)
        return 1;

    id = get_usbid(dev);
    ret = id && strcmp(usb_id, id) == 0;
    free(id);

    return ret;
}

void common_ftdi_list_print()
{
    int i, n;
    struct ftdi_context *ftdi = NULL;
    struct ftdi_device_list *list;

    /* initialize ftdi */
    ftdi = ftdi_new();
    if (!ftdi) {
        fprintf(stderr, "ftdi_new() failed\n");
        return;
    }
    n = ftdi_usb_find_all(ftdi, &list, usb_vid, usb_pid);
    if (n < 1) {
        fprintf(stderr, "unable to find any matching device\n");
        return;
    }

    for (i = 0; i < n; i++) {
        char m[1024], d[1024], s[1024];
        char *id = get_usbid(list->dev);
        int usbid_match = usbid_is_match(list->dev);
        memset(m, 0, 1024);
        memset(d, 0, 1024);
        memset(s, 0, 1024);
        ftdi_usb_get_strings(ftdi, list->dev, m, 1024, d, 1024, s, 1024);
        list = list->next;

        if (usb_description) {
            if (strcmp(usb_description, d) != 0) {
                continue;
            }
        }
        if (usb_serial) {
            if (strcmp(usb_serial, s) != 0) {
                continue;
            }
        }

        if (!usbid_match)
            continue;

        printf("%s %s : %s / %s\n", id, s, d, m);
        free(id);


    }

    ftdi_list_free(&list);
    ftdi_free(ftdi);
}

struct ftdi_context *common_ftdi_init()
{
    int err = 0, i, n;
    struct ftdi_context *ftdi = NULL;
    struct ftdi_device_list *list, *match;

    /* initialize ftdi */
    ftdi = ftdi_new();
    if (!ftdi) {
        fprintf(stderr, "ftdi_new() failed\n");
        return NULL;
    }
    err = ftdi_set_interface(ftdi, interface);
    if (err < 0) {
        fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
        return NULL;
    }
    /* find first device if vid or pid is zero */
    n = ftdi_usb_find_all(ftdi, &list, usb_vid, usb_pid);
    if (n < 1) {
        fprintf(stderr, "unable to find any matching device\n");
        return NULL;
    }
    match = list;
    if (usb_description || usb_serial || usb_id) {
        for (i = 0; i < n; i++) {
            char m[1024], d[1024], s[1024];
            memset(m, 0, 1024);
            memset(d, 0, 1024);
            memset(s, 0, 1024);
            ftdi_usb_get_strings(ftdi, list->dev, m, 1024, d, 1024, s, 1024);
            if (usb_description) {
                if (strcmp(usb_description, d) != 0) {
                    match = match->next;
                    continue;
                }
            }
            if (usb_serial) {
                if (strcmp(usb_serial, s) != 0) {
                    match = match->next;
                    continue;
                }
            }
            if (!usbid_is_match(match->dev)) {
                match = match->next;
                continue;
            }
            break;
        }
        if (i >= n) {
            fprintf(stderr, "unable to find any matching device\n");
            ftdi_list_free(&list);
            return NULL;
        }
    }
    err = ftdi_usb_open_dev(ftdi, match->dev);
    ftdi_list_free(&list);
    if (err < 0) {
        fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
        return NULL;
    }
    /* reset chip */
    if (reset) {
        if (ftdi_usb_reset(ftdi)) {
            fprintf(stderr, "failed to reset device: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
        ftdi_set_bitmode(ftdi, 0x00, BITMODE_RESET);
    }

    return ftdi;
}

unsigned char *common_stdin_read(void)
{
    static unsigned char data[65536];
    struct stat st;
    size_t i;
    int c;

    if (fstat(fileno(stdin), &st)) {
        return NULL;
    }
    if (!S_ISFIFO(st.st_mode) && !S_ISREG(st.st_mode)) {
        return NULL;
    }

    /* remove whitespaces from start of data*/
    for (c = fgetc(stdin); isspace(c); c = fgetc(stdin));

    /* read line */
    for (i = 0; c >= 0 && c <= 255 && i < (sizeof(data) - 1) && !isspace(c); i++, c = fgetc(stdin)) {
        data[i] = (unsigned char)c;
    }

    if (i == 0) {
        return NULL;
    }

    data[i] = '\0';

    return data;
}
