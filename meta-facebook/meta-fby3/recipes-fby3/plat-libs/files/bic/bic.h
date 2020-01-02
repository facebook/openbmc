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

#ifndef __BIC_H__
#define __BIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bic_xfer.h"
#include "bic_power.h"
#include "bic_fwupdate.h"
#include "bic_ipmi.h"
#include "error.h"

enum {
  FW_CPLD = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_VR,
  FW_EXP_CPLD,
};

enum {
  BIC_EOK = 0,
  BIC_ENOTSUP = -ENOTSUP,
  BIC_ENOTREADY = -EAGAIN,
  /* non system errors start from -256 downwards */
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
