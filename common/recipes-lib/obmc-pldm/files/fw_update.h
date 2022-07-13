/*
 * Copyright 2020-present Meta Platforms, Inc. All Rights Reserved.
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

#ifndef _OBMC_MCTP_H_
#define _OBMC_MCTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ncsi.h>
#include <openbmc/pldm.h>
#include "pldm.h"

// PLDM Struct
struct pldm_hdr {
  uint8_t RQD_IID;
  uint8_t Command_Type;
  uint8_t Command_Code;
} __attribute__((packed));

int obmc_pldm_fw_update (uint8_t bus, uint8_t dst_eid, char *path);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_MCTP_H_ */
