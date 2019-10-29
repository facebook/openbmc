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

typedef enum {
  INTF_I2C,
  INTF_JTAG
} cpld_intf_t;

int cpld_intf_open(unsigned int cpld_index, cpld_intf_t intf);
int cpld_intf_close(cpld_intf_t intf);
int cpld_get_ver(unsigned int *ver);
int cpld_get_device_id(unsigned int *dev_id);
int cpld_erase(void);
int cpld_program(char *file);
int cpld_verify(char *file);

struct cpld_dev_info {
	const char	*name;
	unsigned int	dev_id;
	int (*cpld_open)(cpld_intf_t intf, unsigned char id);
	int (*cpld_close)(cpld_intf_t intf);
	int (*cpld_ver)(unsigned int *ver);
	int (*cpld_erase)(void);
	int (*cpld_program)(FILE *fd);
	int (*cpld_verify)(FILE *fd);
	int (*cpld_dev_id)(unsigned int *dev_id);
};

enum {
  LCMXO2_2000HC = 0,
  LCMXO2_4000HC,
  LCMXO2_7000HC,
  MAX10_10M16_PFR,
  MAX10_10M16_MOD, 
  UNKNOWN_DEV
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __CPLD_H__ */
