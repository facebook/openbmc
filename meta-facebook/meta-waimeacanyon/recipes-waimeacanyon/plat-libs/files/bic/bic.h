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

#ifndef __BIC_H__
#define __BIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bic_ipmi.h"
#include "bic_xfer.h"
#include "bic_fwupdate.h"
#include <facebook/fbwc_common.h>

enum {
  //this is defined by the BIC
  BIC_FRU_ID_MB = 0,
  BIC_FRU_ID_IOM = 1,
};

enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

enum {
  // bic definition
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC,
  UPDATE_BIC_BOOTLOADER,
  UPDATE_VR,
  UPDATE_PCIE_SWITCH,
};

int bic_read_fruid(uint8_t bic_fru_id, char *path);
int bic_write_fruid(uint8_t bic_fru_id, char *path);
int bic_get_sensor_reading(uint8_t sensor_num, float *value);

enum {
  FM_SPI_PCH_MASTER_SEL_R = 14,
};


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
