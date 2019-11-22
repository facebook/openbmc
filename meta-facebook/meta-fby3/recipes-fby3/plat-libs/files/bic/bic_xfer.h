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

#ifdef __cplusplus
extern "C" {
#endif

extern const uint32_t IANA_ID;

enum {
  FEXP_BIC_INTF = 0x05,
  BB_BIC_INTF   = 0x10,
  REXP_BIC_INTF = 0x15,
  NONE_INTF     = 0xff,
};

void msleep(int msec);
int i2c_open(uint8_t bus_id);
int i2c_io(int fd, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount);
int get_ipmb_bus_id(uint8_t slot_id);
int is_bic_ready(uint8_t slot_id, uint8_t intf);
int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_XFER_H__ */
