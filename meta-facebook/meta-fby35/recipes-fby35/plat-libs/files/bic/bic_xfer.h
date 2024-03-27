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

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>
#include <facebook/fby35_common.h>
#include "bic.h"

//Netfn, Cmd, IANA data1,2,3, intf
#define MIN_IPMB_BYPASS_LEN 6

extern const uint32_t IANA_ID;
extern const uint32_t VF_IANA_ID;

enum {
  BIC_CMD_OEM_SET_AMBER_LED     = 0x39,
  BIC_CMD_OEM_GET_AMBER_LED     = 0x3A,
  BIC_CMD_OEM_GET_SET_GPIO      = 0x41,
  BIC_CMD_OEM_FW_CKSUM_SHA256   = 0x43,
  BIC_CMD_OEM_BMC_FAN_CTRL      = 0x50,
  BIC_CMD_OEM_GET_FAN_DUTY      = 0x51,
  BIC_CMD_OEM_GET_FAN_RPM       = 0x52,
  BIC_CMD_OEM_SET_12V_CYCLE     = 0x64,
  BIC_CMD_OEM_INFORM_SLED_CYCLE = 0x66,
  BIC_CMD_OEM_BIC_SNR_MONITOR   = 0x68,
  BIC_CMD_OEM_GET_HSC_STATUS    = 0x6C,
  BIC_CMD_OEM_GET_BOARD_ID      = 0xA0,
  BIC_CMD_OEM_GET_MB_INDEX      = 0xF0,
  BIC_CMD_OEM_SET_FAN_DUTY      = 0xF1,
  BIC_CMD_OEM_FAN_CTRL_STAT     = 0xF2,
  BIC_CMD_OEM_CABLE_STAT        = 0xCB,
  BIC_CMD_OEM_CARD_TYPE         = 0xA1,
  BIC_CMD_OEM_NOTIFY_PMIC_ERR   = 0xB0,
  BIC_CMD_OEM_GET_EXTENDED_SDR  = 0xC0,
  BIC_CMD_OEM_BIOS_VER          = 0xA2,
  BIC_CMD_OEM_READ_WRITE_DIMM   = 0xB1,
  BIC_CMD_OEM_GET_I3C_MUX       = 0xB2,
  BIC_CMD_OEM_NOTIFY_DC_OFF     = 0xD1,
  BIC_CMD_OEM_READ_WRITE_SSD    = 0x36,
  BIC_CMD_OEM_STOP_VR_MONITOR   = 0x14,
  BIC_CMD_OEM_GET_RETIMER_TYPE  = 0x79,
};

typedef enum {
  BIC_STATUS_SUCCESS                =  0,
  BIC_STATUS_FAILURE                = -1,
  BIC_STATUS_NOT_SUPP_IN_CURR_STATE = -2,
  BIC_STATUS_NOT_READY              = -3,
  BIC_STATUS_1OU_FAILURE            = -4, //can't get BIC 10U vale
  BIC_STATUS_2OU_FAILURE            = -5, //can't get BIC 20U vale
  BIC_STATUS_3OU_FAILURE            = -6, //can't get BIC 30U vale
  BIC_STATUS_4OU_FAILURE            = -7, //can't get BIC 40U vale
} BIC_STATUS;

enum {
  FEXP_BIC_INTF = 0x05, // 1OU
  BB_BIC_INTF   = 0x10,
  BMC_INTF      = 0x1a,
  REXP_BIC_INTF = 0x15, // 2OU
  EXP3_BIC_INTF = 0x25, // 3OU
  EXP4_BIC_INTF = 0x30, // 4OU
  NONE_INTF     = 0xff,
};

//It is used to check the signed image of CPLD/BIC
enum {
  BICDL  = 0x01,
  BICBB  = 0x02,
  BIC2OU = 0x04,
  BIC1OU = 0x05,
  NICEXP = 0x06,
  BICSPE = 0x07,
  BICGPV3 = 0x08,
  BIC1OU_E1S = 0x09,
};

typedef enum {
  DIMM_SPD = 0x0,
  DIMM_SPD_NVM,
  DIMM_PMIC,
} DIMM_DEVICE_TYPE;

typedef enum {
  I3C_MUX_TO_CPU = 0x0,
  I3C_MUX_TO_BIC = 0x1,
} I3C_MUX_POSITION;

typedef struct {
  uint8_t dev_id;
  uint8_t dev_index;
  uint8_t intf;
} dev_info;

static const dev_info op_dev_info[] = {
  //dev_id, dev_index, intf
  {DEV_ID0_1OU, 0, FEXP_BIC_INTF},
  {DEV_ID1_1OU, 1, FEXP_BIC_INTF},
  {DEV_ID2_1OU, 2, FEXP_BIC_INTF},
  {DEV_ID0_2OU, 0, REXP_BIC_INTF},
  {DEV_ID1_2OU, 1, REXP_BIC_INTF},
  {DEV_ID2_2OU, 2, REXP_BIC_INTF},
  {DEV_ID3_2OU, 3, REXP_BIC_INTF},
  {DEV_ID4_2OU, 4, REXP_BIC_INTF},
  {DEV_ID0_3OU, 0, EXP3_BIC_INTF},
  {DEV_ID1_3OU, 1, EXP3_BIC_INTF},
  {DEV_ID2_3OU, 2, EXP3_BIC_INTF},
  {DEV_ID0_4OU, 0, EXP4_BIC_INTF},
  {DEV_ID1_4OU, 1, EXP4_BIC_INTF},
  {DEV_ID2_4OU, 2, EXP4_BIC_INTF},
  {DEV_ID3_4OU, 3, EXP4_BIC_INTF},
  {DEV_ID4_4OU, 4, EXP4_BIC_INTF}
};

static const dev_info vf_dev_info[] = {
  //dev_id, dev_index, intf
  {DEV_ID0_1OU, 0, FEXP_BIC_INTF},
  {DEV_ID1_1OU, 1, FEXP_BIC_INTF},
  {DEV_ID2_1OU, 2, FEXP_BIC_INTF},
  {DEV_ID3_1OU, 3, FEXP_BIC_INTF}
};

void msleep(int msec);
int i2c_open(uint8_t bus_id, uint8_t addr_7bit);
int is_bic_ready(uint8_t slot_id, uint8_t intf);
int bic_data_send(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen, uint8_t intf);
int bic_data_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_pldm_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_me_xmit(uint8_t slot_id, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int bic_set_fan_auto_mode(uint8_t crtl, uint8_t *status);
int _set_fw_update_ongoing(uint8_t slot_id, uint16_t tmout);
int send_image_data_via_bic(uint8_t slot_id, uint8_t comp, uint8_t intf, uint32_t offset, uint16_t len, uint32_t image_len, uint8_t *buf);
int open_and_get_size(char *path, size_t *file_size);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_XFER_H__ */
