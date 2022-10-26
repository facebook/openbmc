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

#include <libusb-1.0/libusb.h>
#include "bic_xfer.h"

/*2OU BIC USB ID*/
#define EXP2_TI_VENDOR_ID 0x1CC0
#define EXP2_TI_PRODUCT_ID 0x0007

#define NUM_ATTEMPTS 5

typedef struct {
  uint8_t netfn;
  uint8_t cmd;
  uint8_t iana[3];
  uint8_t target;
  uint32_t offset;
  uint16_t length;
  uint8_t data[];
} __attribute__((packed)) bic_usb_packet;
#define USB_PKT_HDR_SIZE (sizeof(bic_usb_packet))

typedef struct {
  uint8_t netfn;
  uint8_t cmd;
  uint8_t cc;
} __attribute__((packed)) bic_usb_res_packet;
#define USB_PKT_RES_HDR_SIZE (sizeof(bic_usb_res_packet))

typedef struct {
  uint8_t netfn;
  uint8_t cmd;
  uint8_t iana[3];
  uint8_t target;
  uint32_t offset;
  uint8_t length;
} __attribute__((packed)) bic_usb_dump_req_packet;

typedef struct {
  uint8_t netfn;
  uint8_t cmd;
  uint8_t cc;
  uint8_t iana[3];
  uint8_t data[];
} __attribute__((packed)) bic_usb_dump_res_packet;

typedef struct {
  uint8_t dummy;
  uint32_t offset;
  uint16_t length;
  uint32_t image_size;
  uint8_t data[];
} __attribute__((packed)) bic_usb_ext_packet;
#define USB_PKT_EXT_HDR_SIZE (sizeof(bic_usb_ext_packet))

int print_configuration(struct libusb_device_handle *hDevice,struct libusb_config_descriptor *config);
int bic_get_fw_cksum(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *cksum);
int bic_get_fw_cksum_sha256(uint8_t slot_id, uint8_t target, uint32_t offset, uint32_t len, uint8_t *cksum);
int send_bic_usb_packet(usb_dev* udev, uint8_t *pkt, const int transferlen);
int bic_init_usb_dev(uint8_t slot_id, uint8_t comp, usb_dev* udev, const uint16_t product_id, const uint16_t vendor_id);
int bic_close_usb_dev(usb_dev* udev);
int update_bic_bios(uint8_t slot_id, uint8_t comp, char *image, uint8_t force);
int update_bic_usb_bios(uint8_t slot_id, uint8_t comp, int fd);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_FWUPDATE_H__ */
