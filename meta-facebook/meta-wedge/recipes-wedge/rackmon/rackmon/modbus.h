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

#ifndef MODBUS_H_
#define MODBUS_H_
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
uint16_t modbus_crc16(char* buffer, size_t length);

#define DEFAULT_TTY "/dev/ttyS3"

extern int verbose;
#define dbg(...) if(verbose) { fprintf(stderr, __VA_ARGS__); }
#define log(...) { fprintf(stderr, __VA_ARGS__); }

#define CHECK(expr) { int _check = expr; if((_check) < 0) { \
    error = _check;  \
    goto cleanup; \
} }
#define CHECKP(name, expr) { int _check = expr; if((_check) < 0) { \
    error = _check;  \
    perror(#name); \
    goto cleanup; \
} }
#define BAIL(...) { \
    fprintf(stderr, __VA_ARGS__); \
    fflush(stderr); \
    error = -1; \
    goto cleanup; \
}

int waitfd(int fd);
void decode_hex_in_place(char* buf, size_t* len);
void append_modbus_crc16(char* buf, size_t* len);
void print_hex(FILE* f, char* buf, size_t len);

// Read until maxlen bytes or no bytes in mdelay_us microseconds
size_t read_wait(int fd, char* dst, size_t maxlen, int mdelay_us);


typedef struct _modbus_req {
  int tty_fd;
  const char *modbus_cmd;
  size_t cmd_len;
  int timeout;
  size_t expected_len;
  char *dest_buf;
  size_t dest_limit;
  size_t dest_len;
  int scan;
} modbus_req;

int modbuscmd(modbus_req *req);
// Modbus errors

#define MODBUS_RESPONSE_TIMEOUT -4
#define MODBUS_BAD_CRC -5

const char* modbus_strerror(int mb_err);

// Modbus constants
#define MODBUS_READ_HOLDING_REGISTERS 3


#endif
