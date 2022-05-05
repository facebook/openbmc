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
#include <facebook/fbgc_common.h>
#include "bic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_IANA_ID 3

extern const uint32_t IANA_ID;

enum NETFN_OEM_38 {
  BIC_CMD_OEM_GET_SET_GPIO      = 0x41,
  BIC_CMD_OEM_FW_CKSUM_SHA256   = 0x43,
  BIC_CMD_OEM_BMC_FAN_CTRL      = 0x50,
  BIC_CMD_OEM_SET_FAN_DUTY      = 0xF1,
  BIC_CMD_OEM_FAN_CTRL_STAT     = 0xF2,
  BIC_CMD_OEM_GET_FAN_DUTY      = 0x51,
  BIC_CMD_OEM_GET_FAN_RPM       = 0x52,
  BIC_CMD_OEM_SET_12V_CYCLE     = 0x64,
  BIC_CMD_OEM_INFORM_SLED_CYCLE = 0x66,
  BIC_CMD_OEM_GET_BOARD_ID      = 0xA0,
  BIC_CMD_OEM_SET_AMBER_LED     = 0xA1,
};

enum {
  BIC_STATUS_SUCCESS                =  0,
  BIC_STATUS_FAILURE                = -1,
  BIC_STATUS_NOT_SUPP_IN_CURR_STATE = -2,
  BIC_STATUS_12V_OFF                = -3,
};

// BIC GPIO action
enum {
  BIC_GPIO_GET_OUTPUT_STATUS,
  BIC_GPIO_SET_OUTPUT_STATUS,
  BIC_GPIO_GET_DIRECTION_STATUS,
  BIC_GPIO_SET_DIRECTION_STATUS,
};

int i2c_open(uint8_t bus_id, uint8_t addr_7bit);
int i2c_io(int fd, uint8_t *tbuf, uint8_t tcount, uint8_t *rbuf, uint8_t rcount);
int is_bic_ready();
int bic_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int open_and_get_size(char *path, int *file_size);
int send_image_data_via_bic(uint8_t comp, uint32_t offset, uint16_t len, uint32_t image_len, uint8_t *buf);
int _set_fw_update_ongoing(uint16_t tmout);
int bic_me_xmit(uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_XFER_H__ */
