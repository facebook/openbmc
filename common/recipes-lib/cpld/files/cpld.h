/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
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

#ifndef __CPLD_H__
#define __CPLD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
  INTF_I2C,
  INTF_JTAG
} cpld_intf_t;

typedef struct {
  uint8_t bus_id;
  uint8_t slv_addr;
  int (*xfer)(uint8_t bus, uint8_t addr, uint8_t *tbuf, uint8_t tcnt, uint8_t *rbuf, uint8_t rcnt);
} i2c_attr_t;

typedef struct {
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t img_type;
  uint32_t start_addr;
  uint32_t end_addr;
  uint32_t csr_base;
  uint32_t data_base;
  uint32_t boot_base;
} altera_max10_attr_t;

int cpld_intf_open(uint8_t cpld_index, cpld_intf_t intf, void *attr);
int cpld_intf_close(void);
int cpld_get_ver(uint32_t *ver);
int cpld_get_device_id(uint32_t *dev_id);
int cpld_erase(void);
int cpld_program(char *file, char* key, char is_signed);
int cpld_verify(char *file);

struct cpld_dev_info {
  const char *name;
  uint32_t dev_id;
  uint8_t intf;
  int (*cpld_open)(void *attr);
  int (*cpld_close)(void);
  int (*cpld_ver)(uint32_t *ver);
  int (*cpld_erase)(void);
  int (*cpld_program)(FILE *fd, char *key, char is_signed);
  int (*cpld_verify)(FILE *fd);
  int (*cpld_dev_id)(uint32_t *dev_id);
};

enum {
  LCMXO2_2000HC = 0,
  LCMXO2_4000HC,
  LCMXO2_7000HC,
  LCMXO3_2100C,
  LCMXO3_4300C,
  LCMXO3_6900C,
  LCMXO3_9400C,
  LFMNX_50_I2C,
  MAX10_10M16,
  MAX10_10M25,
  MAX10_10M04,
  MAX10_10M08,
  UNKNOWN_DEV
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __CPLD_H__ */
