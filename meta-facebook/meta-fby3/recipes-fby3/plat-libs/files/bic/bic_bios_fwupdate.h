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

#ifndef __BIC_BIOS_FWUPDATE_H__
#define __BIC_BIOS_FWUPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bic_xfer.h"
#include "bic.h"
#include <libusb-1.0/libusb.h>

#define BIOS_CAPSULE_OFFSET 0x7F0000
#define CPLD_CAPSULE_OFFSET 0x17F0000

int print_configuration(struct libusb_device_handle *hDevice,struct libusb_config_descriptor *config);
int active_config(struct libusb_device *dev,struct libusb_device_handle *handle);
int bic_get_fw_cksum(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *ver);
int update_bic_bios(uint8_t slot_id, uint8_t comp, char *image, uint8_t force);
int update_bic_usb_bios(uint8_t slot_id, uint8_t comp, char *image);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_FWUPDATE_H__ */
