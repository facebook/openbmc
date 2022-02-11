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

#ifndef __EXPANDER_H__
#define __EXPANDER_H__

#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXPANDER_IPMB_BUS_NUM 9
#define EXPANDER_SLAVE_ADDR 0x13 
#define CMD_EXP_GET_SENSOR_READING 0x2D
#define CMD_GET_EXP_VERSION 0x12
#define CMD_GET_IOC_VERSION 0x56
#define CMD_GET_EXP_FRUID 0x11
#define EXPANDER_FRUID_SIZE 0x7F
#define EXPANDER_HDD_STATUS 0xC0
#define EXPANDER_ERROR_CODE 0x11


int expander_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int exp_get_fw_ver(uint8_t *ver);
int exp_get_ioc_fw_ver(uint8_t *ver);
int exp_read_fruid(const char *path, unsigned char FRUID);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __EXPANDER_H__ */
