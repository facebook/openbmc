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

#ifndef __BIC_XFER_H__
#define __BIC_XFER_H__

#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include <facebook/fbwc_common.h>
#include "bic.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  BIC_STATUS_SUCCESS                =  0,
  BIC_STATUS_FAILURE                = -1,
  BIC_STATUS_NOT_SUPP_IN_CURR_STATE = -2,
};

#define IPMB_RETRY_DELAY_TIME 20*1000 // 20 millisecond

int bic_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);

extern const uint32_t IANA_ID;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_XFER_H__ */
