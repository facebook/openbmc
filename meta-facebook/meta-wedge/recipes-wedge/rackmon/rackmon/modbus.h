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
#define DEFAULT_GPIO 45

#define CHECK(x) { if((x) < 0) { \
    error = x;  \
    goto cleanup; \
} }

void waitfd(int fd);
void gpio_on(int fd);
void gpio_off(int fd);
void decode_hex_in_place(char* buf, size_t* len);
void append_modbus_crc16(char* buf, size_t* len);
void print_hex(FILE* f, char* buf, size_t len);

// Read until maxlen bytes or no bytes in mdelay_us microseconds
size_t read_wait(int fd, char* dst, size_t maxlen, int mdelay_us);

extern int verbose;

#endif
