/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __BIC_POWER_H__
#define __BIC_POWER_H__

#include "bic_xfer.h"
#include "bic_ipmi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BIC_I2C_FPGA_BUS 2
#define BIC_I2C_FPGA_ADDR 0x42 // 8 bit
#define BIC_FPGA_SEVER_PWR_REG 0x0
#define BIC_PWR_STATUS_GPIO_NUM 36

int bic_server_power_ctrl(int action);
int bic_server_power_cycle();
int bic_get_server_power_status(uint8_t *power_status);

// The action of power control
enum {
  SET_DC_POWER_OFF = 0x0,
  SET_DC_POWER_ON,
  SET_GRACEFUL_POWER_OFF,  
  SET_HOST_RESET,
};

typedef struct {
  uint8_t offset;
  uint8_t val;
} i2c_master_rw_command;

typedef struct {
  uint8_t bus_id;
  uint8_t slave_addr;
  uint8_t rw_count;
  uint8_t offset;
  uint8_t val;
} bic_pwr_command;

typedef struct {
  uint8_t iana_id[3];
  uint8_t gpio_action;
  uint8_t gpio_number;
} bic_pwr_status_command;

typedef struct {
  uint8_t iana_id[3];
  uint8_t gpio_addr;
  uint8_t gpio_status;
} bic_pwr_status_result;


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_POWER_H__ */
