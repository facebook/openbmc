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

#ifndef __EXPANDER_H__
#define __EXPANDER_H__

#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <facebook/fbgc_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXP_VERSION_RES_LEN   20

// NetFn: OEM (0x30)
enum NETFN_OEM_30 {
  CMD_OEM_EXP_ERROR_CODE             = 0x11,
  CMD_OEM_EXP_GET_SENSOR_READING     = 0x2D,
};

// NetFn: Storage (0x0A)
enum NETFN_STORAGE {
  CMD_GET_EXP_FRUID       = 0x11,
  CME_OEM_EXP_SLED_CYCLE  = 0x30,
  CMD_GET_EXP_VERSION     = 0x12,
};

typedef struct Version {
  uint8_t status;
  uint8_t major_ver;
  uint8_t minor_ver;
  uint8_t unit_ver;
  uint8_t dev_ver;
} ver_set;

typedef struct ExpanderVersion {
  ver_set bootloader;
  ver_set firmware_region1;
  ver_set firmware_region2;
  ver_set configuration;
} exp_ver;

typedef struct _ExpanderGetFruidCommand {
  uint8_t fruid;
  uint8_t offset_low;
  uint8_t offset_high;
  uint8_t query_size;
} ExpanderGetFruidCommand;

int expander_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);
int expander_get_fw_ver(uint8_t *ver, uint8_t ver_len);
int exp_read_fruid(const char *path, uint8_t fru_id);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __EXPANDER_H__ */
