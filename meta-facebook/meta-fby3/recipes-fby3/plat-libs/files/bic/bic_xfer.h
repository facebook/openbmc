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

#ifndef __BIC_XFER_H__
#define __BIC_XFER_H__

#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include <facebook/fby3_common.h>
#include "bic.h"

#ifdef __cplusplus
extern "C" {
#endif

//Netfn, Cmd, IANA data1,2,3, intf
#define MIN_IPMB_BYPASS_LEN 6
extern const uint32_t IANA_ID;

enum {
  BIC_CMD_OEM_GET_SET_GPIO  = 0x41,
  BIC_CMD_OEM_BMC_FAN_CTRL  = 0x50,
  BIC_CMD_OEM_SET_FAN_DUTY  = 0xF1,
  BIC_CMD_OEM_FAN_CTRL_STAT = 0xF2,
  BIC_CMD_OEM_GET_FAN_DUTY  = 0x51,
  BIC_CMD_OEM_GET_FAN_RPM   = 0x52,
  BIC_CMD_OEM_SET_12V_CYCLE = 0x64,
  BIC_CMD_OEM_GET_BOARD_ID  = 0xA0,
};

enum {
  BIC_STATUS_SUCCESS =  0,
  BIC_STATUS_FAILURE = -1,
};

enum {
  FEXP_BIC_INTF = 0x05,
  BB_BIC_INTF   = 0x10,
  REXP_BIC_INTF = 0x15,
  NONE_INTF     = 0xff,
};

//It is used to check the signed image of CPLD/BIC
enum {
  BICDL  = 0x01,
  BICBB  = 0x02,
  BIC2OU = 0x04,
  BIC1OU = 0x05,
  NICEXP = 0x06,
};

void msleep(int msec);
int i2c_open(uint8_t bus_id);
int i2c_io(int fd, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount);
int is_bic_ready(uint8_t slot_id, uint8_t intf);
int bic_ipmb_send(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen, uint8_t intf);
int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_set_fan_auto_mode(uint8_t crtl, uint8_t *status);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_XFER_H__ */
