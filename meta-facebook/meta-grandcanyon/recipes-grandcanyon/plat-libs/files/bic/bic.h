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

#include "bic_xfer.h"
#include "bic_ipmi.h"
#include "bic_fwupdate.h"
#include "bic_vr_fwupdate.h"
#include "bic_power.h"
#include "error.h"
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <openbmc/kv.h>

#define MAX_RETRY 3

#define IPMB_RETRY_DELAY_TIME 500

#define MAX_CHECK_USB_DEV_TIME 8

#define MAX_USB_MANUFACTURER_LEN 64
#define MAX_USB_PRODUCT_LEN      64

#define MAX_BIC_VER_STR_LEN      32

/*IFX VR pages*/
#define VR_PAGE   0x00
#define VR_PAGE32 0x32
#define VR_PAGE50 0x50
#define VR_PAGE60 0x60
#define VR_PAGE62 0x62

typedef struct {
  struct  libusb_device**          devs;
  struct  libusb_device*           dev;
  struct  libusb_device_handle*    handle;
  struct  libusb_device_descriptor desc;
  char    manufacturer[MAX_USB_MANUFACTURER_LEN];
  char    product[MAX_USB_PRODUCT_LEN];
  int     config;
  int     ci;
  uint8_t epaddr;
  uint8_t path[MAX_PATH_LEN];
} usb_dev;

enum {
  RECOVERY_MODE = 1,
  RESTORE_FACTORY_DEFAULT,
};

enum {
  FW_BS_FPGA = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_VR,
  FW_BIOS,
};

// VR vendor
enum {
  VR_ISL = 0x0,
  VR_TI  = 0x1,
  VR_IFX = 0x2,
};

enum {
  FORCE_UPDATE_UNSET = 0x0,
  FORCE_UPDATE_SET,
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_H__ */
