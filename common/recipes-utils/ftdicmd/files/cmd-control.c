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
#include <libusb-1.0/libusb.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "ENom:d:s:l:x:n:p:A:B:C:D:";
struct option longopts[] = {
    COMMON_LONG_OPTS
    { "ee-erase", no_argument, NULL, 'E' },
    { "ee-init", no_argument, NULL, 'N' },
    { "ee-decode", no_argument, NULL, 'o' },
    { "ee-manufacturer", required_argument, NULL, 'm' },
    { "ee-description", required_argument, NULL, 'd' },
    { "ee-serial", required_argument, NULL, 's' },
    { "ee-serial-len", required_argument, NULL, 'l' },
    { "ee-serial-hex", required_argument, NULL, 'x' },
    { "ee-serial-dec", required_argument, NULL, 'n' },
    { "ee-bus-power", required_argument, NULL, 'p' },
    { "ee-ch-a-rs485", required_argument, NULL, 'A' },
    { "ee-ch-b-rs485", required_argument, NULL, 'B' },
    { "ee-ch-c-rs485", required_argument, NULL, 'C' },
    { "ee-ch-d-rs485", required_argument, NULL, 'D' },
    { 0, 0, 0, 0 },
};

/* ftdi device context */
struct ftdi_context *ftdi = NULL;

/* if eeprom should be erased first */
int ee_erase = 0;
/* initialize eeprom to default values, erases eeprom */
int ee_initialize = 0;
/* if eeprom should be decoded */
int ee_decode = 0;
/* if eeprom should be read first */
int ee_rd = 0;
/* if eeprom should be written after */
int ee_wr = 0;

char *set_description = NULL;
char *set_serial = NULL;
int set_serial_len = 0;
int set_serial_mode = 0;
char *set_manufacturer = NULL;
int set_bus_power = 0;
int set_A_rs485 = -1;
int set_B_rs485 = -1;
int set_C_rs485 = -1;
int set_D_rs485 = -1;

int value=0;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
    if (set_description) {
        free(set_description);
    }
    if (set_serial) {
        free(set_serial);
    }
    if (set_manufacturer) {
        free(set_manufacturer);
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
        "  -E, --ee-erase             erase eeprom, sometimes needed if eeprom has already been initialized\n"
        "  -N, --ee-init              erase and initialize eeprom with defaults\n"
        "  -o, --ee-decode            read eeprom and print decoded information\n"
        "  -m, --ee-manufacturer=STRING\n"
        "                             write manufacturer string\n"
        "  -d, --ee-description=STRING\n"
        "                             write description (product) string\n"
        "  -s, --ee-serial=STRING     write serial string\n"
        "  -l, --ee-serial-len=LENGTH pad serial with randomized ascii letters and numbers to this length (upper case)\n"
        "  -x, --ee-serial-hex=LENGTH pad serial with randomized hex to this length (upper case)\n"
        "  -n, --ee-serial-dec=LENGTH pad serial with randomized numbers to this length\n"
        "  -p, --ee-bus-power=INT     bus power drawn by the device (100-500 mA)\n"
        "  -A, --ee-ch-a-rs485=(0,1)  enable/disable rs485 mode for channel A\n"
        "  -B, --ee-ch-b-rs485=(0,1)  enable/disable rs485 mode for channel B\n"
        "  -C, --ee-ch-c-rs485=(0,1)  enable/disable rs485 mode for channel C\n"
        "  -D, --ee-ch-d-rs485=(0,1)  enable/disable rs485 mode for channel D\n"
        "\n"
        "Basic control and eeprom routines for FTDI FTx232 chips.\n"
        "\n");
}

int p_options(int c, char *optarg)
{
    switch (c) {
    case 'E':
        ee_erase = 1;
        return 1;
    case 'N':
        ee_erase = 1;
        ee_initialize = 1;
        ee_wr = 1;
        return 1;
    case 'o':
        ee_decode = 1;
        ee_rd = 1;
        return 1;
    case 'm':
        set_manufacturer = strdup(optarg);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 'd':
        set_description = strdup(optarg);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 's':
        if (set_serial) {
            free(set_serial);
        }
        set_serial = strdup(optarg);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 'l': /* mode 0 */
    case 'x': /* mode 1 */
    case 'n': /* mode 2 */
        if (!set_serial) {
            set_serial = strdup("");
        }
        set_serial_len = atoi(optarg);
        set_serial_mode = c == 'n' ? 2 : (c == 'x' ? 1 : 0);
        return 1;
    case 'p':
        set_bus_power = atoi(optarg);
        if (set_bus_power < 100 || set_bus_power > 500) {
            fprintf(stderr, "invalid bus power value, not within 100-500 mA\n");
            return -1;
        }
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 'A':
        set_A_rs485 = atoi(optarg);
        if (set_A_rs485 != 0 && set_A_rs485 != 1) {
            fprintf(stderr, "invalid value, not is (0 or 1)\n");
            return -1;
        }
        printf("Set Channel A set RS485 to %d\n", set_A_rs485);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 'B':
        set_B_rs485 = atoi(optarg);
        if (set_B_rs485 != 0 && set_B_rs485 != 1) {
            fprintf(stderr, "invalid value, not is (0 or 1)\n");
            return -1;
        }
        printf("Set Channel B set RS485 to %d\n", set_B_rs485);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 'C':
        set_C_rs485 = atoi(optarg);
        if (set_C_rs485 != 0 && set_C_rs485 != 1) {
            fprintf(stderr, "invalid value, not is (0 or 1)\n");
            return -1;
        }
        printf("Set Channel C set RS485 to %d\n", set_C_rs485);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    case 'D':
        set_D_rs485 = atoi(optarg);
        if (set_D_rs485 != 0 && set_D_rs485 != 1) {
            fprintf(stderr, "invalid value, not is (0 or 1)\n");
            return -1;
        }
        printf("Set Channel D set RS485 to %d\n", set_D_rs485);
        ee_rd = 1;
        ee_wr = 1;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{

    /* random seed */
    srand(time(NULL));

    /* parse command line options */
    if (common_options(argc, argv, opts, longopts, 0, 0)) {
        fprintf(stderr, "invalid command line option(s)\n");
        p_exit(EXIT_FAILURE);
    }

    /* init ftdi things */
    ftdi = common_ftdi_init();
    if (!ftdi) {
        p_exit(EXIT_FAILURE);
    }

    if (ee_initialize) {
        /* initialize eeprom to defaults */
        if (ftdi_eeprom_initdefaults(ftdi, NULL, NULL, NULL)) {
            fprintf(stderr, "failed to init defaults to eeprom: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
    } else if (ee_rd) {
        /* read eeprom */
        if (ftdi_read_eeprom(ftdi)) {
            fprintf(stderr, "failed to read eeprom: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
        if (ftdi_eeprom_decode(ftdi, 0)) {
            fprintf(stderr, "failed to decode eeprom after read: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
    }

    /* erase eeprom */
    if (ee_erase) {
        if (ftdi_erase_eeprom(ftdi)) {
            fprintf(stderr, "failed to erase eeprom: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
    }

    /* set strings to eeprom */
    if (set_manufacturer || set_description || set_serial) {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(libusb_get_device(ftdi->usb_dev), &desc);
        if (!set_manufacturer) {
            set_manufacturer = calloc(1, 128);
            libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iManufacturer, (unsigned char*)set_manufacturer, 127);
        }
        if (!set_description) {
            set_description = calloc(1, 128);
            libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iProduct, (unsigned char*)set_description, 127);
        }
        if (!set_serial) {
            set_serial = calloc(1, 128);
            libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iSerialNumber, (unsigned char*)set_serial, 127);
        }
        if (strlen(set_serial) < set_serial_len) {
            int i;
            set_serial = realloc(set_serial, set_serial_len + 1);
            for (i = strlen(set_serial); i < set_serial_len; i++) {
                char c;
                char max = set_serial_mode == 2 ? 0 : (set_serial_mode == 1 ? 'F' : 'Z');
                /* generate random padding */
                do {
                    c = rand() & 0x7f;
                } while ((c < '0' || c > '9') && (c < 'A' || c > max));
                set_serial[i] = c;
            }
            set_serial[set_serial_len] = '\0';
        }
        ftdi_eeprom_set_strings(ftdi, set_manufacturer, set_description, set_serial);
    }

    /* bus power */
    if (set_bus_power > 100) {
        ftdi_set_eeprom_value(ftdi, MAX_POWER, set_bus_power);
    }

    /* set RS485 Enable/Disable*/
    if (set_A_rs485 == 0 || set_A_rs485 == 1){
        ftdi_set_eeprom_value(ftdi, CHANNEL_A_RS485, set_A_rs485);
    }
    if (set_B_rs485 == 0 || set_B_rs485 == 1){
        ftdi_set_eeprom_value(ftdi, CHANNEL_B_RS485, set_B_rs485);
    }
    if (set_C_rs485 == 0 || set_C_rs485 == 1){
        ftdi_set_eeprom_value(ftdi, CHANNEL_C_RS485, set_C_rs485);
    }
    if (set_D_rs485 == 0 || set_D_rs485 == 1){
        ftdi_set_eeprom_value(ftdi, CHANNEL_D_RS485, set_D_rs485);
    }

    /* write eeprom data */
    if (ee_wr) {
        if (ftdi_eeprom_build(ftdi) < 0) {
            fprintf(stderr, "failed to build eeprom: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
        if (ftdi_write_eeprom(ftdi)) {
            fprintf(stderr, "failed to write eeprom: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
    }

    /* decode eeprom */
    if (ee_decode) {
        if (ftdi_eeprom_decode(ftdi, 1)) {
            fprintf(stderr, "failed to decode eeprom: %s\n", ftdi_get_error_string(ftdi));
            p_exit(EXIT_FAILURE);
        }
        /* display RS485 mode */
        ftdi_get_eeprom_value(ftdi, CHANNEL_A_RS485, &value);
        printf("CHANNEL_A_RS485   %d\n", value ? 1 : 0);
        ftdi_get_eeprom_value(ftdi, CHANNEL_B_RS485, &value);
        printf("CHANNEL_B_RS485   %d\n", value ? 1 : 0);
        ftdi_get_eeprom_value(ftdi, CHANNEL_C_RS485, &value);
        printf("CHANNEL_C_RS485   %d\n", value ? 1 : 0);
        ftdi_get_eeprom_value(ftdi, CHANNEL_D_RS485, &value);
        printf("CHANNEL_D_RS485   %d\n", value ? 1 : 0);
    }

    p_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

