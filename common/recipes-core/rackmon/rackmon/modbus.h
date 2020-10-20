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
#include <string.h>
#include <errno.h>
#include <termios.h>

#include <openbmc/log.h>

#ifndef DEFAULT_TTY
#define DEFAULT_TTY "/dev/ttyS3"
#endif

// Modbus errors
#define MODBUS_RESPONSE_TIMEOUT -4
#define MODBUS_BAD_CRC -5

// Modbus constants
#define MODBUS_READ_HOLDING_REGISTERS 3
#define MODBUS_WRITE_HOLDING_REGISTERS 6

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif

extern int verbose;
#define dbg(fmt, args...) do {  \
    if(verbose)                 \
        OBMC_INFO(fmt, ##args); \
} while (0)
#define log(fmt, args...) OBMC_INFO(fmt, ##args)

#define ERR_EXIT(expr) do { \
    int _check = (expr);    \
    if(_check < 0) {        \
        error = _check;     \
        goto cleanup;       \
    }                       \
} while (0)

#define ERR_LOG_EXIT(expr, msg)  do { \
    int _check = (expr);              \
    if(_check < 0) {                  \
        error = _check;               \
        OBMC_ERROR(errno, msg);       \
        goto cleanup;                 \
    }                                 \
} while (0)

#define BAIL(fmt, args...) do { \
    OBMC_WARN(fmt, ##args);     \
    error = -1;                 \
    goto cleanup;               \
} while (0)

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

int waitfd(int fd);
void decode_hex_in_place(char* buf, size_t* len);
void append_modbus_crc16(char* buf, size_t* len);
void print_hex(FILE* f, char* buf, size_t len);
const char* baud_to_str(speed_t baudrate);

// Read until maxlen bytes or no bytes in mdelay_us microseconds
size_t read_wait(int fd, char* dst, size_t maxlen, int mdelay_us);

int modbuscmd(modbus_req *req, speed_t baudrate);
uint16_t modbus_crc16(char* buffer, size_t length);
const char* modbus_strerror(int mb_err);

#endif /* MODBUS_H_ */
