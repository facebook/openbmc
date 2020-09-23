/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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

/*
 * This file is intended to contain platform specific definitions, and
 * it's set to empty intentionally. Each platform needs to override the
 * file if platform specific settings are required.
 */

#include <openbmc/log.h>
#include "bic.h"
#include "bic_platform.h"

#define SIZE_SYS_GUID 16

int bic_get_sys_guid(uint8_t slot_id, uint8_t *guid) {
  int ret;
  size_t rlen = 0;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_GET_SYSTEM_GUID, NULL, 0, guid, &rlen);
  if (rlen != SIZE_SYS_GUID) {
    OBMC_ERROR(ret, "bic_get_sys_guid: returned rlen of %d\n", rlen);
    return -1;
  }

  return ret;
}

int bic_set_sys_guid(uint8_t slot_id, uint8_t *guid) {

  int ret;
  size_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN]={0x00};

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_REQ, CMD_OEM_SET_SYSTEM_GUID, guid, SIZE_SYS_GUID, rbuf, &rlen);

  return ret;

}
