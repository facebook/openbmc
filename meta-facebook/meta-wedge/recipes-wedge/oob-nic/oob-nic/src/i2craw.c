/*
 * Copyright 2004-present Facebook. All Rights Reserved.
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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include <openbmc/obmc-i2c.h>
#include "openbmc/log.h"

void usage(const char *prog) {
  printf("Usage: %s [options] <bus number> <slave address>\n", prog);
  printf("\n  Options:\n"
         "\n\t-w 'bytes to write':\n"
         "\t\t i2c write\n"
         "\n\t-r <number of bytes to read>:\n"
         "\t\t if 0 is provided, the first byte of read is used to determine\n"
         "\t\t how many bytes more to read\n"
         "\n\t-p:\n"
         "\t\t Use PEC\n"
         "\n\t-h:\n"
         "\t\t Print this help\n"
         "\n  Note: if both '-w' and '-r' are specified, write will be"
         "\n        performed first, followed by read\n");
}

#define MAX_BYTES 255

int g_use_pec = 0;
int g_has_write = 0;
int g_n_write = 0;
uint8_t g_write_bytes[MAX_BYTES];
int g_has_read = 0;
int g_n_read = -1;
uint8_t g_read_bytes[MAX_BYTES];
uint8_t g_bus = -1;
uint8_t g_slave_addr = 0xff;

static int parse_byte_string(const char *str) {
  const char *startptr = str;
  char *endptr;
  int total = 0;
  unsigned long val;

  do {
    val = strtoul(startptr, &endptr, 0);
    if (startptr == endptr) {
      printf("'%s' is invalid\n", str);
      return -1;
    }
    if (val > MAX_BYTES) {
      printf("'%s' is invalid\n", str);
      return -1;
    }
    g_write_bytes[total++] = val;
    if (*endptr == '\0') {
      break;
    }
    if (total >= MAX_BYTES) {
      printf("'%s' is invalid\n", str);
      return -1;
    }
    startptr = endptr;
  } while(1);

  return total;
}

static int i2c_open() {
  int fd;
  char fn[32];
  int rc;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", g_bus);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    LOG_ERR(errno, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, g_slave_addr);
  if (rc < 0) {
    LOG_ERR(errno, "Failed to open slave @ address 0x%x", g_slave_addr);
    close(fd);
  }

  return fd;
}

static int i2c_io(int fd) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int n_msg = 0;
  int rc;

  memset(&msg, 0, sizeof(msg));

  if (g_has_write) {
    msg[n_msg].addr = g_slave_addr;
    msg[n_msg].flags = (g_use_pec) ? I2C_CLIENT_PEC : 0;
    msg[n_msg].len = g_n_write;
    msg[n_msg].buf = g_write_bytes;
    n_msg++;
  }

  if (g_has_read) {
    msg[n_msg].addr = g_slave_addr;
    msg[n_msg].flags = I2C_M_RD
      | ((g_use_pec) ? I2C_CLIENT_PEC : 0)
      | ((g_n_read == 0) ? I2C_M_RECV_LEN : 0);
    /*
     * In case of g_n_read is 0, block length will be added by
     * the underlying bus driver.
     */
    msg[n_msg].len = (g_n_read) ? g_n_read : 256;
    msg[n_msg].buf = g_read_bytes;
    if (g_n_read == 0) {
      /* If we're using variable length block reads, we have to set the
       * first byte of the buffer to at least one or the kernel complains.
       */
      g_read_bytes[0] = 1;
    }
    n_msg++;
  }

  data.msgs = msg;
  data.nmsgs = n_msg;

  rc = ioctl(fd, I2C_RDWR, &data);
  if (rc < 0) {
    LOG_ERR(errno, "Failed to do raw io");
    return -1;
  }

  return 0;
}

int main(int argc, char * const argv[]) {
  int i;
  int fd;
  int opt;
  while ((opt = getopt(argc, argv, "hpw:r:")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'p':
      g_use_pec = 1;
      break;
    case 'w':
      g_has_write = 1;
      if ((g_n_write = parse_byte_string(optarg)) <= 0) {
        usage(argv[0]);
        return -1;
      }
      break;
    case 'r':
      g_has_read = 1;
      g_n_read = atoi(optarg);
      break;
    default:
      usage(argv[0]);
      return -1;
    }
  }

  /* make sure we still have arguments for bus and slave address */
  if (optind + 2 != argc) {
    printf("Bus or slave address is missing\n");
    usage(argv[0]);
    return -1;
  }

  g_bus = atoi(argv[optind]);
  g_slave_addr = strtoul(argv[optind + 1], NULL, 0);
  if ((g_slave_addr & 0x80)) {
    printf("Slave address must be 7-bit\n");
    return -1;
  }

  if (!g_has_write && !g_has_read) {
    /* by default, read, first byte read is the length */
    g_has_read = 1;
    g_n_read = 0;
  }

  printf("Bus: %d\nDevice address: 0x%x\n", g_bus, g_slave_addr);
  if (g_has_write) {
    printf("To write %d bytes:", g_n_write);
    for (i = 0; i < g_n_write; i++) {
      printf(" 0x%x", g_write_bytes[i]);
    }
    printf("\n");
  }
  if (g_has_read) {
    if (g_n_read) {
      printf("To read %d bytes.\n", g_n_read);
    } else {
      printf("To read data.\n");
    }
  }

  fd = i2c_open();
  if (fd < 0) {
    return -1;
  }

  if (i2c_io(fd) < 0) {
    return -1;
  }

  if (g_has_read) {
    printf("Received:\n ");
    if (g_n_read == 0) {
      g_n_read = g_read_bytes[0] + 1;
    }
    for (i = 0; i < g_n_read; i++) {
      printf(" 0x%x", g_read_bytes[i]);
    }
    printf("\n");
  }

  return 0;
}
