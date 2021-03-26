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

// Modbus retry count
#define MAX_RETRY   5

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

#define timespec_to_ms(_t) (((_t)->tv_sec * 1000) + ((_t)->tv_nsec / 1000000))
#define timespec_to_us(_t) (((_t)->tv_sec * 1000000) + ((_t)->tv_nsec / 1000))

#ifdef RACKMON_PROFILING
#include <time.h>
#include <sys/times.h>

/*
 * CPU_USAGE_START|END() is designed to calculate the average cpu usage
 * of a specific code section, and here is the formula:
 *
 *   cpu_usage(%) = (user_time + sys_time) / real_time;
 *
 * Note: the sys/user/real time unit is "tick", which is 10 milliseconds
 *       by default. As a result, it's not recommended to use the macro
 *       to measure a "tiny" code section (finished within a few ticks).
 */
#define CPU_USAGE_START()                                                 \
do {                                                                      \
    clock_t _clk_start, _clk_end;                                         \
    struct tms _tms_start, _tms_end;                                      \
    long _clk_diff, _utime_diff, _stime_diff;                             \
    _clk_start = times(&_tms_start);                                      \
    if (_clk_start == (clock_t)-1) {                                      \
        _clk_start = 0;                                                   \
        memset(&_tms_start, 0, sizeof(_tms_start));                       \
    }

#define CPU_USAGE_END(fmt, args...)                                       \
    _clk_end = times(&_tms_end);                                          \
    if (_clk_end == (clock_t)-1)                                          \
        break;                                                            \
    _clk_diff = (long)(_clk_end - _clk_start);                            \
    _utime_diff = (long)(_tms_end.tms_utime - _tms_start.tms_utime);      \
    _stime_diff = (long)(_tms_end.tms_stime - _tms_start.tms_stime);      \
    OBMC_INFO(fmt ": user=%ld, sys=%ld, real=%ld, cpu_%%=%ld%%", ##args,  \
              _utime_diff, _stime_diff, _clk_diff,                        \
              ((_utime_diff + _stime_diff) * 100) / _clk_diff);           \
} while (0)

/*
 * CHECK_LATENCY_START|END() is designed to measure the latency of a
 * specific code section.
 *
 * Note: latency may vary greatly depending on system workload, scheduling
 * policy, and etc. In order to minimize the noise, it's recommended to
 * keep system idle when using the macros.
 */
#define CHECK_LATENCY_START()                                              \
do {                                                                      \
    struct timespec _start, _end, _diff;                                  \
    if (clock_gettime(CLOCK_REALTIME, &_start) != 0)                      \
        memset(&_start, 0, sizeof(_start))

#define CHECK_LATENCY_END(fmt, args...)                                   \
    if (clock_gettime(CLOCK_REALTIME, &_end) != 0)                        \
        break;                                                            \
    if (timespec_sub(&_start, &_end, &_diff) == 0)                        \
        OBMC_INFO(fmt ": took %ld usec", ##args, timespec_to_us(&_diff)); \
} while (0)
#else /* !RACKMON_PROFILING */
#define CPU_USAGE_START()
#define CPU_USAGE_END(fmt, args...)
#define CHECK_LATENCY_START()
#define CHECK_LATENCY_END(fmt, args...)
#endif /* RACKMON_PROFILING */

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

int timespec_sub(struct timespec *start, struct timespec *end,
                 struct timespec *result);

#endif /* MODBUS_H_ */
