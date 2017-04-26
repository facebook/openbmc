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
#include <linux/serial.h>
#include "modbus.h"

void usage() {
  fprintf(stderr,
      "modbussim [-v] [-t <tty>] modbus_request modbus_reply\n"
      "\ttty defaults to %s\n"
      "\tmodbus request/reply should be specified in hex\n"
      "\teg:\ta40300000008\n",
      DEFAULT_TTY);
  exit(1);
}

int main(int argc, char **argv) {
    int error = 0;
    int fd;
    struct termios tio;
    char *tty = DEFAULT_TTY;
    char *modbus_cmd = NULL;
    char *modbus_reply = NULL;
    size_t cmd_len = 0;
    size_t reply_len = 0;
    verbose = 0;

    int opt;
    while((opt = getopt(argc, argv, "t:g:v"))) {
      if (opt == -1) break;
      switch (opt) {
      case 't':
        tty = optarg;
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
      modbus_cmd = argv[optind++];
      modbus_reply = argv[optind++];
    }
    if(modbus_cmd == NULL || modbus_reply == NULL) {
      usage();
    }

    if (verbose)
      fprintf(stderr, "[*] Opening TTY\n");
    fd = open(tty, O_RDWR | O_NOCTTY);
    CHECK(fd);

    struct serial_rs485 rs485conf = {};
    rs485conf.flags |= SER_RS485_ENABLED;
    if (verbose)
      fprintf(stderr, "[*] Putting TTY in RS485 mode\n");
    error = ioctl(fd, TIOCSRS485, &rs485conf);
    if (error < 0) {
      fprintf(stderr, "FATAL: could not set TTY to RS485 mode: %d %s\n",
          error, strerror(error));
      goto cleanup;
    }

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
    if(cmd_len < 2) {
      fprintf(stderr, "Command too short!\n");
      exit(1);
    }
    decode_hex_in_place(modbus_cmd, &cmd_len);
    append_modbus_crc16(modbus_cmd, &cmd_len);
    if (verbose)  {
      fprintf(stderr, "expect: ");
      print_hex(stderr, modbus_cmd, cmd_len);
      fprintf(stderr, "\n");
    }
    reply_len = strlen(modbus_reply);
    decode_hex_in_place(modbus_reply, &reply_len);
    append_modbus_crc16(modbus_reply, &reply_len);
    // print full expected reply
    if (verbose)  {
      fprintf(stderr, "reply: ");
      print_hex(stderr, modbus_reply, reply_len);
      fprintf(stderr, "\n");
    }

    // Enable UART read
    tio.c_cflag |= CREAD;
    CHECK(tcsetattr(fd,TCSANOW,&tio));

    if(verbose)
      fprintf(stderr, "[*] Wait for matching command...\n");

    char modbus_buf[255];
    size_t mb_pos;
wait_for_command:
    mb_pos = read_wait(fd, modbus_buf, sizeof(modbus_buf), 30000);
    if(mb_pos >= 4) {
      printf("Received: ");
      print_hex(stdout, modbus_buf, mb_pos);
      uint16_t crc = modbus_crc16(modbus_buf, mb_pos - 2);
      if((modbus_buf[mb_pos - 2] == (crc >> 8)) &&
          (modbus_buf[mb_pos - 1] == (crc & 0x00FF))) {
        if(verbose)
          fprintf(stderr, "CRC OK!\n");
        if(memcmp(modbus_buf, modbus_cmd, cmd_len) == 0) {
          fprintf(stderr, "Command matched!\n");
        } else {
          fprintf(stderr, "Got modbus cmd that didn't match.\n");
          goto wait_for_command;
        }
      } else {
        fprintf(stderr, "Got data that failed modbus CRC.\n");
        goto wait_for_command;
      }
    } else {
      goto wait_for_command;
    }


    if (verbose)
      fprintf(stderr, "[*] Writing reply!\n");

    // Disable UART read
    tio.c_cflag &= ~CREAD;
    CHECK(tcsetattr(fd,TCSANOW,&tio));
    write(fd, modbus_reply, reply_len);
    waitfd(fd);

cleanup:
    if(error != 0) {
      error = 1;
      fprintf(stderr, "%s\n", strerror(errno));
    }
    return error;
}
