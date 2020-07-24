/*
 *
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

#ifndef __ALERT_CONTROL_H__
#define __ALERT_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
  FLAG_DISABLE = 0x00,
  FLAG_ENABLE,
} e_flag_t;

typedef enum {
  FBID_SMS_KCS = 0x00,
  FBID_SMM_KCS,
  FBID_COM1_DATA,
  FBID_COM2_DATA,
  FBID_COM1_STAT_CTRL,
  FBID_COM2_STAT_CTRL,
  FBID_POST,
  FBID_I2C_PROXY1_MASTER,
  FBID_I2C_PROXY1_STAT,
  FBID_I2C_PROXY2_MASTER,
  FBID_I2C_PROXY2_STAT,
  FBID_GPIO_CONFIG,
  FBID_GPIO_OUTPUT,
  FBID_GPIO_INPUT,
  FBID_GPIO_INTR,
  FBID_GPIO_EVENT,
  FBID_REG_READ,
  FBID_ALERT_CTRL = 0xFD,
  FBID_AERT_STAT = 0xFE,
  FBID_DISCOVERY = 0xFF,
} e_fbid_t;

int alert_control(e_fbid_t id, e_flag_t cflag);
bool is_alert_present(e_fbid_t id);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __ALERT_CONTROL_H__ */
