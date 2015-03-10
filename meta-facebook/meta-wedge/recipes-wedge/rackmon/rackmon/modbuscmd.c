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

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "modbus.h"

int verbose;

void usage() {
  fprintf(stderr,
      "modbuscmd [-v] [-t <tty>] [-g <gpio>] modbus_command\n"
      "\ttty defaults to %s\n"
      "\tgpio defaults to %d\n"
      "\tmodbus command should be specified in hex\n"
      "\teg:\ta40300000008\n",
      DEFAULT_TTY, DEFAULT_GPIO);
  exit(1);
}

int main(int argc, char **argv) {
    int error = 0;
    int fd;
    struct termios tio;
    char gpio_filename[255];
    int gpio_fd = 0;
    int gpio_n = DEFAULT_GPIO;
    char *tty = DEFAULT_TTY;
    char *modbus_cmd = NULL;
    size_t cmd_len = 0;
    verbose = 0;

    int opt;
    while((opt = getopt(argc, argv, "t:g:v")) != -1) {
      switch (opt) {
      case 't':
        tty = optarg;
        break;
      case 'g':
        gpio_n = atoi(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        usage();
        break;
      }
    }
    if(optind < argc) {
      modbus_cmd = argv[optind];
    }
    if(modbus_cmd == NULL) {
      usage();
    }

    if (verbose)
      fprintf(stderr, "[*] Opening TTY\n");
    fd = open(tty, O_RDWR | O_NOCTTY);
    CHECK(fd);

    if (verbose)
      fprintf(stderr, "[*] Opening GPIO %d\n", gpio_n);
    snprintf(gpio_filename, 255, "/sys/class/gpio/gpio%d/value", gpio_n);
    gpio_fd = open(gpio_filename, O_WRONLY | O_SYNC);
    CHECK(gpio_fd);

    if (verbose)
      fprintf(stderr, "[*] Setting TTY flags!\n");
    memset(&tio, 0, sizeof(tio));
    cfsetspeed(&tio,B19200);
    tio.c_cflag |= PARENB;
    tio.c_cflag |= CLOCAL;
    tio.c_cflag |= CS8;
    tio.c_iflag |= INPCK;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    CHECK(tcsetattr(fd,TCSANOW,&tio));

    //convert hex to bytes
    cmd_len = strlen(modbus_cmd);
    if(cmd_len < 4) {
      fprintf(stderr, "Modbus command too short!\n");
      exit(1);
    }
    decode_hex_in_place(modbus_cmd, &cmd_len);
    append_modbus_crc16(modbus_cmd, &cmd_len);
    // print command as sent
    if (verbose)  {
      fprintf(stderr, "Will send:  ");
      print_hex(stderr, modbus_cmd, cmd_len);
      fprintf(stderr, "\n");
    }

    if (verbose)
      fprintf(stderr, "[*] Writing!\n");

    // gpio on, write, wait, gpio off
    gpio_on(gpio_fd);
    write(fd, modbus_cmd, cmd_len);
    waitfd(fd);
    gpio_off(gpio_fd);

    // Enable UART read
    tio.c_cflag |= CREAD;
    CHECK(tcsetattr(fd,TCSANOW,&tio));

    if(verbose)
      fprintf(stderr, "[*] reading any response...\n");
    // Read back response
    char modbus_buf[255];
    size_t mb_pos = 0;
    memset(modbus_buf, 0, sizeof(modbus_buf));
    mb_pos = read_wait(fd, modbus_buf, sizeof(modbus_buf), 90000);
    if(mb_pos >= 4) {
      uint16_t crc = modbus_crc16(modbus_buf, mb_pos - 2);
      if(verbose)
        fprintf(stderr, "Modbus response CRC: %04X\n ", crc);
      if((modbus_buf[mb_pos - 2] == (crc >> 8)) &&
          (modbus_buf[mb_pos - 1] == (crc & 0x00FF))) {
        if(verbose)
          fprintf(stderr, "CRC OK!\n");
        print_hex(stdout, modbus_buf, mb_pos);
        printf("\n");
      } else {
        fprintf(stderr, "BAD CRC :(\n");
        return 5;
      }
    } else {
      fprintf(stderr, "No response :(\n");
      return 4;
    }

cleanup:
    if(error != 0) {
      error = 1;
      fprintf(stderr, "%s\n", strerror(errno));
    }
    return error;
}
