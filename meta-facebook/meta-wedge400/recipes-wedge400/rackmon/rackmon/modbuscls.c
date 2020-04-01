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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*
 *   non version  : initial version
 *   0.1.1        : add -s to specific serial device file
 *                : fix bug -t unit value is wrong from us to ms
 *                : fix default timeout to 50 ms
 */

uint8_t checksum;

void usage() {
  fprintf(stderr, " version 0.1.1\n"
                  "modbuscls [-v] [-c] [-x expected length] [-s serial-file] "
                  "[-t <timeout in ms>] modbus_command\n"
                  "\tmodbus command should be specified in hex\n"
                  "\teg:\t000300000002\n"
                  "after receiving that many bytes\n");
  exit(1);
}

int verbose = 0;
typedef struct _modbus_req {
  int tty_fd;
  const char *modbus_cmd;
  size_t cmd_len;
  int timeout;
  size_t expected_len;
  char *dest_buf;
  size_t dest_limit;
  size_t dest_len;
  int checksum;
  int scan;
} modbus_req;

int waitfd(int fd) {
  int loops = 0;
  while (1) {
    int lsr;
    int ret = ioctl(fd, TIOCSERGETLSR, &lsr);
    if (ret == -1) {
      break;
    }
    if (lsr & TIOCSER_TEMT)
      break;
    // never should hit this if kernel supports RS485 mode
    loops++;
  }
  return loops;
}

void print_hex(FILE *f, char *buf, size_t len) {
  for (int i = 0; i < len; i++)
    fprintf(f, "%02x ", buf[i]);
}

void decode_hex_in_place(char *buf, size_t *len) {
  for (int i = 0; i < *len; i += 2) {
    sscanf(buf + i, "%2hhx", buf + (i / 2));
  }
  *len = *len / 2;
}

const char *modbus_strerror(int mb_err) {
  if (mb_err < 0) {
    mb_err = -mb_err;
  }
  switch (mb_err) {
  case 4:
    return "timed out";
  case 5:
  default:
    return "unknown";
  }
}

double ts_diff(struct timespec *begin, struct timespec *end) {
  return 1000.0 * (end->tv_sec) + (1e-6 * end->tv_nsec) -
         (1000.0 * (begin->tv_sec) + (1e-6 * begin->tv_nsec));
}

/* From libmodbus, https://github.com/stephane/libmodbus
 * Under LGPL. */
/* Table of CRC values for high-order byte */
static const uint8_t table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40};

/* Table of CRC values for low-order byte */
static const uint8_t table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40};

uint16_t modbus_crc16(char *buffer, size_t buffer_length) {
  uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
  uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
  unsigned int i;        /* will index into CRC lookup */

  /* pass through message buffer */
  while (buffer_length--) {
    i = crc_hi ^ *((uint8_t *)(buffer++)); /* calculate the CRC  */
    crc_hi = crc_lo ^ table_crc_hi[i];
    crc_lo = table_crc_lo[i];
  }

  return (crc_hi << 8 | crc_lo);
}

void append_modbus_crc16(char *buf, size_t *len) {
  uint16_t crc = modbus_crc16(buf, *len);
  buf[(*len)++] = crc >> 8;
  buf[(*len)++] = crc & 0x00FF;
}

size_t read_wait(int fd, char *dst, size_t maxlen, int mdelay_us) {
  fd_set fdset;
  struct timeval timeout;
  char read_buf[16];
  size_t read_size = 0;
  size_t pos = 0;
  memset(dst, 0, maxlen);
  while (pos < maxlen) {
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    timeout.tv_sec = 0;
    timeout.tv_usec = mdelay_us;
    int rv = select(fd + 1, &fdset, NULL, NULL, &timeout);
    if (rv == -1) {
      perror("select()");
    } else if (rv == 0) {
      break;
    }
    read_size = read(fd, read_buf, 1);
    if (read_size < 0) {
      if (errno == EAGAIN)
        continue;
      fprintf(stderr, "read error: %s\n", strerror(errno));
      exit(1);
    }
    if ((pos + read_size) <= maxlen) {
      memcpy(dst + pos, read_buf, read_size);
      pos += read_size;
    } else {
      return pos;
      fprintf(stderr, "Response buffer overflowed!\n");
    }
  }
  return pos;
}

int modbuscmd(modbus_req *req) {
  struct termios tio;
  char modbus_cmd[req->cmd_len + 2];
  size_t cmd_len = req->cmd_len;

  if (verbose)
    fprintf(stderr, "[*] Setting TTY flags!\n");
  memset(&tio, 0, sizeof(tio));
  // CREAD should be left *off* until we've confirmed THRE
  // to avoid catching false character starts
  cfsetspeed(&tio, B9600);
  tio.c_cflag &= ~PARENB;
  tio.c_cflag &= ~CSTOPB;
  tio.c_cflag &= ~CSIZE;
  tio.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */
  tio.c_cflag |= CS8;      /* 8-bit characters */
  // tio.c_iflag |= INPCK;
  tio.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tio.c_oflag &= ~OPOST;

  tio.c_cc[VMIN] = 1;
  tio.c_cc[VTIME] = 1;

  tio.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
  tcsetattr(req->tty_fd, TCSANOW, &tio);

  memcpy(modbus_cmd, req->modbus_cmd, cmd_len);
  if (req->checksum)
    append_modbus_crc16(modbus_cmd, &cmd_len);

  // print command as sent
  fprintf(stderr, "Will send:  ");
  print_hex(stderr, modbus_cmd, cmd_len);
  fprintf(stderr, "\n");

  // hoped adding the ioctl to do the switching would have alleviated the
  // need to do SCHED_FIFO, but we still get preempted between the write and
  // ioctl syscalls w/o it often enough to break f/w updates.
  // struct sched_param sp;
  // sp.sched_priority = 50;
  // int policy = SCHED_FIFO;
  // pthread_setschedparam(pthread_self(), policy, &sp);

  struct timespec write_begin;
  struct timespec wait_begin;
  struct timespec wait_end;
  struct timespec read_end;
  clock_gettime(CLOCK_MONOTONIC_RAW, &write_begin);
  if (write(req->tty_fd, modbus_cmd, cmd_len) != cmd_len) {
    fprintf(stderr, "Failed to write modbus command!\n");
    return -1;
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, &wait_begin);
  int waitloops = waitfd(req->tty_fd);
  clock_gettime(CLOCK_MONOTONIC_RAW, &wait_end);

  if (verbose) {
    printf(" waitfd loop: %d\n", waitloops);
  }

  // Read back response
  size_t mb_pos = 0;
  memset(req->dest_buf, 0, req->dest_limit);
  if (req->expected_len == 0) {
    req->expected_len = req->dest_limit;
  }
  if (req->expected_len > req->dest_limit) {
    return -1;
  }
  mb_pos =
      read_wait(req->tty_fd, req->dest_buf, req->expected_len, req->timeout);
  clock_gettime(CLOCK_MONOTONIC_RAW, &read_end);
  req->dest_len = mb_pos;
  if (verbose) {
    printf(" mb_pos = %d\n", mb_pos);
    printf("Time capture\n");
    printf("  write: %.2f ms\n", ts_diff(&write_begin, &wait_begin));
    printf("  wait: %.2f ms\n", ts_diff(&wait_begin, &wait_end));
    printf("  read: %.2f ms\n", ts_diff(&wait_end, &read_end));
  }
  return 0;
}

int open_rs485_dev(const char *tty_filename, int *tty_fd) {
  if (verbose) {
    printf("Open dev tty %s\n", tty_filename);
  }
  syslog(LOG_INFO, "open_rs485_dev %s", tty_filename);

  *tty_fd = open(tty_filename, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
  if (*tty_fd < 0) {
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {

  int opt, ret;
  int tty_fd;
  size_t cmd_len;
  modbus_req req;
  char *modbus_cmd = NULL;
  char serialfile[64];
  char destbuf[128];
  int timeout, expected = 0;

  strcpy(serialfile, "/dev/ttyUSB0");
  timeout = 50;
  while ((opt = getopt(argc, argv, "x:t:s:vc")) != -1) {
    switch (opt) {
    case 'x':
      expected = atoi(optarg);
      break;
    case 't':
      timeout = atol(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    case 's':
      strcpy(serialfile, optarg);
      break;
    case 'c':
      req.checksum = 1;
      break;
    default:
      fprintf(stderr, "opt err %c\n", opt);
      usage();
      break;
    }
  }
  if (verbose) {
    fprintf(stdout, "cmd %s\n", argv[optind]);
  }
  if (optind < argc) {
    modbus_cmd = argv[optind];
  }
  if (!modbus_cmd) {
    fprintf(stderr, "modbus_cmd = NULL\n");
    usage();
  }

  // convert hex to bytes
  cmd_len = strlen(modbus_cmd);
  if (cmd_len < 4) {
    fprintf(stderr, "Modbus command too short!\n");
    exit(1);
  }
  decode_hex_in_place(modbus_cmd, &cmd_len);

  if (open_rs485_dev(serialfile, &tty_fd) < 0) {
    fprintf(stderr, "Open dev tty error %s\n", serialfile);
    return -1;
  }
  req.tty_fd = tty_fd;
  req.modbus_cmd = modbus_cmd;
  req.cmd_len = cmd_len;
  req.dest_buf = destbuf;
  req.dest_limit = 128;
  req.timeout = timeout * 1000;
  req.expected_len = expected != 0 ? expected : 128;
  req.scan = 0;

  ret = modbuscmd(&req);
  close(tty_fd);

  if (ret) {
    printf(" modbuscmd Error\n");
    return -1;
  }

  printf("Response:  ");
  print_hex(stdout, req.dest_buf, req.dest_len);
  printf("\n");

  if (req.dest_len > 0 && req.checksum == 1) {
    uint16_t crc_check = modbus_crc16(req.dest_buf, req.dest_len - 2);
    if (((crc_check & 0xFF) == req.dest_buf[req.dest_len - 1]) &&
        ((crc_check >> 8) == req.dest_buf[req.dest_len - 2])) {
      printf("Checksum: OK\n");
    } else {
      printf("Checksum: Fail\n");
    }
  }

  return 0;
}
