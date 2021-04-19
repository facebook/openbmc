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

#ifndef __BIC_BIOS_FWUPDATE_H__
#define __BIC_BIOS_FWUPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bic_xfer.h"
#include "bic.h"
#include "bic_ipmi.h"
#include <libusb-1.0/libusb.h>

#define SHIFT_BYTE               8
#define INITIAL_BYTE             0xFF
#define MAX_READ_BUFFER_SIZE     256
#define BIOS_PKT_SIZE            (64 * 1024)
#define BIOS_ERASE_PKT_SIZE      (64*1024)
#define BIOS_VERIFY_PKT_SIZE     (32*1024)
#define BIOS_VER_REGION_SIZE     (4*1024*1024)

//Firmware update ongoing time out (second)
#define FW_UPDATE_TIMEOUT_1M     60
#define FW_UPDATE_TIMEOUT_2M     (FW_UPDATE_TIMEOUT_1M * 2)

#define UPDATE_REQ_OFFSET_SIZE   4
#define UPDATE_REQ_LENGTH_SIZE   2
#define UPDATE_REQ_READ_BUF_SIZE (MAX_IPMB_BUFFER - \
                                 (SIZE_IANA_ID + \
                                  UPDATE_REQ_OFFSET_SIZE + \
                                  UPDATE_REQ_LENGTH_SIZE + 1))

typedef struct _update_request {
  uint8_t IANA[SIZE_IANA_ID];
  uint8_t component;
  uint8_t offset[UPDATE_REQ_OFFSET_SIZE];
  uint8_t length[UPDATE_REQ_LENGTH_SIZE];
  uint8_t read_buffer[UPDATE_REQ_READ_BUF_SIZE];
} update_request;

int print_configuration(struct libusb_device_handle *hDevice, struct libusb_config_descriptor *config);
int active_config(struct libusb_device *dev, struct libusb_device_handle *handle);
int bic_get_fw_cksum(uint8_t target, uint32_t offset, uint32_t len, uint8_t *ver);
int verify_bios_image(int fd, long size);
int update_bic_bios(uint8_t comp, char *image, uint8_t force);
int update_bic_usb_bios(uint8_t comp, char *image);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_FWUPDATE_H__ */
