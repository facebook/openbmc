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

#ifndef __MCTP_H__
#define __MCTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_RETRIES_MAX 5

//Type7 IOM IOC
#define IOM_IOC_BUS_NUM 1
#define IOM_IOC_SLAVE_ADDRESS 0x5

int mctp_i2c_write(uint8_t dev_number, uint8_t slave_addr,uint8_t *buf, uint8_t len);
int mctp_i2c_read(uint8_t dev_number, uint8_t slave_addr,uint8_t *buf, uint8_t len);
int mctp_get_iom_ioc_ver(uint8_t *vaule);
int mctp_get_iom_ioc_temp(float *value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __MCTP_H__ */
