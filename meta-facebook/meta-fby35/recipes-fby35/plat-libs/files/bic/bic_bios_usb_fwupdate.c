/*
 * libusb example program to list devices on the bus
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <openssl/sha.h>
#include "bic_bios_fwupdate.h"

#define USB_PKT_SIZE 0x200
#define USB_DAT_SIZE (USB_PKT_SIZE - USB_PKT_HDR_SIZE)
#define USB_PKT_SIZE_BIG 0x1000
#define USB_DAT_SIZE_BIG (USB_PKT_SIZE_BIG - USB_PKT_HDR_SIZE)
#define BIOS_PKT_SIZE (64 * 1024)
#define SIZE_IANA_ID 3
#define BIOS_VERIFY_PKT_SIZE (32*1024)
#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define BIOS_UPDATE_BLK_SIZE (64*1024)
#define BIOS_UPDATE_IMG_SIZE (32*1024*1024)
#define SIMPLE_DIGEST_LENGTH 4
#define STRONG_DIGEST_LENGTH SHA256_DIGEST_LENGTH

int interface_ref = 0;
int alt_interface,interface_number;

int print_configuration(struct libusb_device_handle *hDevice,struct libusb_config_descriptor *config)
{
  char *data;
  int index;

  data = (char *)malloc(512);
  memset(data,0,512);
  index = config->iConfiguration;

  libusb_get_string_descriptor_ascii(hDevice, index, (unsigned char*) data,512);
  printf("-----------------------------------\n\n");
  printf("Interface Descriptors:\n");
  printf("  Number of Interfaces : 0x%x\n",config->bNumInterfaces);
  printf("  Length : 0x%x\n",config->bLength);
  printf("  Desc_Type : 0x%x\n",config->bDescriptorType);
  printf("  Config_index : 0x%x\n",config->iConfiguration);
  printf("  Total length : %u\n",config->wTotalLength);
  printf("  Configuration Value  : 0x%x\n",config->bConfigurationValue);
  printf("  Configuration Attributes : 0x%x\n",config->bmAttributes);
  printf("  MaxPower(mA) : %d\n",config->MaxPower);

  free(data);
  data = NULL;
  return 0;
}

int active_config(struct libusb_device *dev,struct libusb_device_handle *handle)
{
  struct libusb_device_handle *hDevice_req;
  struct libusb_config_descriptor *config;
  int altsetting_index = 0, endpoint_index = 0, interface_index = 0;

  hDevice_req = handle;
  libusb_get_active_config_descriptor(dev, &config);
  print_configuration(hDevice_req, config);

  for ( interface_index=0;interface_index < config->bNumInterfaces;interface_index++) {
    const struct libusb_interface *iface = &config->interface[interface_index];
    for ( altsetting_index=0; altsetting_index < iface->num_altsetting ;altsetting_index++) {
      const struct libusb_interface_descriptor *altsetting = &iface->altsetting[altsetting_index];
      printf("    Interface:\n");
      printf("      bInterfaceNumber:   %d\n", altsetting->bInterfaceNumber);
      printf("      bAlternateSetting:  %d\n", altsetting->bAlternateSetting);
      printf("      bNumEndpoints:      %d\n", altsetting->bNumEndpoints);
      printf("      bInterfaceClass:    %d\n", altsetting->bInterfaceClass);
      printf("      bInterfaceSubClass: %d\n", altsetting->bInterfaceSubClass);
      printf("      bInterfaceProtocol: %d\n", altsetting->bInterfaceProtocol);
      printf("      iInterface:         %d\n\n", altsetting->iInterface);

      for(endpoint_index=0; endpoint_index < altsetting->bNumEndpoints ;endpoint_index++) {
        const struct libusb_endpoint_descriptor *endpoint = &altsetting->endpoint[endpoint_index];
        //endpoint = ep;
        alt_interface = altsetting->bAlternateSetting;
        interface_number = altsetting->bInterfaceNumber;
        printf("      EndPoint Descriptors:\n");
        printf("        bLength: %d\n",endpoint->bLength);
        printf("        bDescriptorType: 0x%x\n",endpoint->bDescriptorType);
        printf("        bEndpointAddress: 0x%x\n",endpoint->bEndpointAddress);
        printf("        Maximum Packet Size: 0x%x\n",endpoint->wMaxPacketSize);
        printf("        bmAttributes: 0x%x\n",endpoint->bmAttributes);
        printf("        bInterval: 0x%x\n",endpoint->bInterval);
      }
    }
    printf("-----------------------------------\n");
  }

  libusb_free_config_descriptor(NULL);
  return 0;
}

int
send_bic_usb_packet(usb_dev* udev, bic_usb_packet *pkt)
{
  const int transferlen = pkt->length + USB_PKT_HDR_SIZE;
  int transferred = 0;
  int retries = 3;
  int ret;

  // _debug_bic_usb_packet(pkt);
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t*)pkt, transferlen, &transferred, 3000);
    if(((ret != 0) || (transferlen != transferred))) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length %d)\n",ret ,transferred, transferlen);
      printf("Retry since  %s\n", libusb_error_name(ret));
      retries--;
      if (!retries) {
        return -1;
      }
      msleep(100);
    } else
      break;
  }
  return 0;
}

int
receive_bic_usb_packet(usb_dev* udev, bic_usb_packet *pkt)
{
  const int receivelen = USB_PKT_SIZE;
  int received = 0;
  int retries = 3;
  int ret;

  // _debug_bic_usb_packet(pkt);
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t*)pkt, receivelen, &received, 3000);
    if(ret != 0) {
      printf("Error in receiving data! err = %d (%s)\n", ret, libusb_error_name(ret));
      retries--;
      if (!retries) {
        return -1;
      }
      msleep(100);
    } else
      break;
  }

  // Return CC code
  return pkt->iana[0];
}

int
bic_init_usb_dev(uint8_t slot_id, usb_dev* udev, const uint16_t product_id, const uint16_t vendor_id)
{
  int ret;
  int index = 0;
  char found = 0;
  ssize_t cnt;
  uint8_t bmc_location = 0;
  int recheck = MAX_CHECK_DEVICE_TIME;

  ret = libusb_init(NULL);
  if (ret < 0) {
    printf("Failed to initialise libusb\n");
    goto error_exit;
  } else {
    printf("Init libusb Successful!\n");
  }

  do {
    cnt = libusb_get_device_list(NULL, &udev->devs);
    if (cnt < 0) {
      printf("There are no USB devices on bus\n");
      goto error_exit;
    }
    index = 0;
    while ((udev->dev = udev->devs[index++]) != NULL) {
      ret = libusb_get_device_descriptor(udev->dev, &udev->desc);
      if ( ret < 0 ) {
        printf("Failed to get device descriptor -- exit\n");
        libusb_free_device_list(udev->devs,1);
        goto error_exit;
      }

      ret = libusb_open(udev->dev,&udev->handle);
      if ( ret < 0 ) {
        printf("Error opening device -- exit\n");
        libusb_free_device_list(udev->devs,1);
        goto error_exit;
      }

      if( (vendor_id == udev->desc.idVendor) && (product_id == udev->desc.idProduct) ) {
        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*) udev->manufacturer, sizeof(udev->manufacturer));
        if ( ret < 0 ) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        ret = fby35_common_get_bmc_location(&bmc_location);
        if (ret < 0) {
          syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
          goto error_exit;
        }

        ret = libusb_get_port_numbers(udev->dev, udev->path, sizeof(udev->path));
        if (ret < 0) {
          printf("Error get port number\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        if ( bmc_location == BB_BMC ) {
          if ( udev->path[1] != slot_id) {
            continue;
          }
        }
        printf("%04x:%04x (bus %d, device %d)",udev->desc.idVendor, udev->desc.idProduct, libusb_get_bus_number(udev->dev), libusb_get_device_address(udev->dev));
        printf(" path: %d", udev->path[0]);
        for (index = 1; index < ret; index++) {
          printf(".%d", udev->path[index]);
        }
        printf("\n");

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iProduct, (unsigned char*) udev->product, sizeof(udev->product));
        if ( ret < 0 ) {
          printf("Error get Product string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        printf("Manufactured : %s\n",udev->manufacturer);
        printf("Product : %s\n",udev->product);
        printf("----------------------------------------\n");
        printf("Device Descriptors:\n");
        printf("Vendor ID : %x\n",udev->desc.idVendor);
        printf("Product ID : %x\n",udev->desc.idProduct);
        printf("Serial Number : %x\n",udev->desc.iSerialNumber);
        printf("Size of Device Descriptor : %d\n",udev->desc.bLength);
        printf("Type of Descriptor : %d\n",udev->desc.bDescriptorType);
        printf("USB Specification Release Number : %d\n",udev->desc.bcdUSB);
        printf("Device Release Number : %d\n",udev->desc.bcdDevice);
        printf("Device Class : %d\n",udev->desc.bDeviceClass);
        printf("Device Sub-Class : %d\n",udev->desc.bDeviceSubClass);
        printf("Device Protocol : %d\n",udev->desc.bDeviceProtocol);
        printf("Max. Packet Size : %d\n",udev->desc.bMaxPacketSize0);
        printf("No. of Configuraions : %d\n",udev->desc.bNumConfigurations);

        found = 1;
        break;
      }
    }

    if ( found != 1) {
      sleep(3);
    } else {
      break;
    }
  } while ((--recheck) > 0);


  if ( found == 0 ) {
    printf("Device NOT found -- exit\n");
    libusb_free_device_list(udev->devs,1);
    ret = -1;
    goto error_exit;
  }

  ret = libusb_get_configuration(udev->handle, &udev->config);
  if ( ret != 0 ) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(udev->devs,1);
    goto error_exit;
  }

  printf("Configured value : %d\n", udev->config);
  if ( udev->config != 1 ) {
    libusb_set_configuration(udev->handle, 1);
    if ( ret != 0 ) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(udev->devs,1);
      goto error_exit;
    }
    printf("Device is in configured state!\n");
  }

  libusb_free_device_list(udev->devs, 1);

  if(libusb_kernel_driver_active(udev->handle, udev->ci) == 1) {
    printf("Kernel Driver Active\n");
    if(libusb_detach_kernel_driver(udev->handle, udev->ci) == 0) {
      printf("Kernel Driver Detached!");
    } else {
      printf("Couldn't detach kernel driver -- exit\n");
      libusb_free_device_list(udev->devs,1);
      goto error_exit;
    }
  }

  ret = libusb_claim_interface(udev->handle, udev->ci);
  if ( ret < 0 ) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(udev->devs,1);
    goto error_exit;
  }
  printf("Claimed Interface: %d, EP addr: 0x%02X\n", udev->ci, udev->epaddr);

  active_config(udev->dev, udev->handle);
  return 0;
error_exit:
  return -1;
}

static int
calc_checksum_simple(const uint8_t *buf, size_t len, uint8_t *out) {
  uint32_t cs = 0;
  while (len-- > 0) {
    cs += *buf++;
  }
  *((uint32_t *) out) = cs;
  return 0;
}

static int
calc_checksum_sha256(const void *buf, size_t len, uint8_t *out) {
  SHA256_CTX ctx = {0};
  memset(out, 0, STRONG_DIGEST_LENGTH);
  if (SHA256_Init(&ctx) != 1) return -1;
  if (SHA256_Update(&ctx, buf, len) != 1) return -2;
  if (SHA256_Final(out, &ctx) != 1) return -3;
  return 0;
}

static bool
bic_have_checksum_sha256(uint8_t slot_id) {
  uint8_t cs[STRONG_DIGEST_LENGTH];
  return (bic_get_fw_cksum_sha256(slot_id, UPDATE_BIOS, 0, BIOS_UPDATE_BLK_SIZE, cs) == 0);
}

static int
get_block_checksum(uint8_t slot_id, size_t offset, int cs_len, uint8_t *out) {
  int rc;
  if (cs_len == STRONG_DIGEST_LENGTH) {
    rc = bic_get_fw_cksum_sha256(slot_id, UPDATE_BIOS, offset, BIOS_UPDATE_BLK_SIZE, out);
  } else {
    rc = bic_get_fw_cksum(slot_id, UPDATE_BIOS, offset, BIOS_VERIFY_PKT_SIZE, out);
    if (rc == 0) {
      rc = bic_get_fw_cksum(slot_id, UPDATE_BIOS, offset + BIOS_VERIFY_PKT_SIZE,
                            BIOS_VERIFY_PKT_SIZE, out + SIMPLE_DIGEST_LENGTH);
    }
  }
  return rc;
}

int
bic_update_fw_usb(uint8_t slot_id, uint8_t comp, int fd, usb_dev* udev)
{
  int ret = -1, rc = 0;
  uint8_t *buf = NULL;
  size_t write_offset = 0;

  const char *what = NULL;
  if (comp == FW_BIOS) {
    what = "BIOS";
    write_offset = 0;
  } else {
    fprintf(stderr, "ERROR: not supported component [comp:%u]!\n", comp);
    goto out;
  }
  const char *dedup_env = getenv("FW_UTIL_DEDUP");
  const char *verify_env = getenv("FW_UTIL_VERIFY");
  bool dedup = (dedup_env != NULL ? (*dedup_env == '1' || *dedup_env == '2') : true);
  bool verify = (verify_env != NULL ? (*verify_env == '1') : false);

  buf = malloc(USB_PKT_HDR_SIZE + BIOS_UPDATE_BLK_SIZE);
  if (buf == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
    goto out;
  }

  int num_blocks_written = 0, num_blocks_skipped = 0;
  uint8_t fcs[STRONG_DIGEST_LENGTH], cs[STRONG_DIGEST_LENGTH];
  int cs_len = STRONG_DIGEST_LENGTH;
  if (!bic_have_checksum_sha256(slot_id)) {
    if (dedup && !(dedup_env != NULL && *dedup_env == '2')) {
      fprintf(stderr, "Strong checksum function is not available, disabling "
              "deduplication.\n");
      dedup = false;
    }
    cs_len = SIMPLE_DIGEST_LENGTH * 2;
  }
  fprintf(stderr, "Updating %s, dedup is %s, verification is %s.\n",
          what, (dedup ? "on" : "off"), (verify ? "on" : "off"));
  int attempts = NUM_ATTEMPTS;
  size_t file_buf_num_bytes = 0;
  while (attempts > 0) {
    uint8_t *file_buf = buf + USB_PKT_HDR_SIZE;
    size_t file_buf_pos = 0;
    bool send_packet_fail = false;
    fprintf(stderr, "\r%d blocks (%d written, %d skipped)...",
        num_blocks_written + num_blocks_skipped,
        num_blocks_written, num_blocks_skipped);
    fflush(stderr);
    // Read a block of data from file.
    while (file_buf_num_bytes < BIOS_UPDATE_BLK_SIZE) {
      size_t num_to_read = BIOS_UPDATE_BLK_SIZE - file_buf_num_bytes;
      ssize_t num_read = read(fd, file_buf, num_to_read);
      if (num_read < 0) {
        if (errno == EINTR) {
          continue;
        }
        fprintf(stderr, "read error: %d\n", errno);
        goto out;
      }
      if (num_read == 0) {
        break;
      }
      file_buf_num_bytes += num_read;
    }
    // Finished.
    if (file_buf_num_bytes == 0) {
      break;
    }
    // Pad to 64K with 0xff, if needed.
    for (size_t i = file_buf_num_bytes; i < BIOS_UPDATE_BLK_SIZE; i++) {
      file_buf[i] = '\xff';
    }
    // Check if we need to write this block at all.
    if (dedup || verify) {
      if (cs_len == STRONG_DIGEST_LENGTH) {
        rc = calc_checksum_sha256(file_buf, BIOS_UPDATE_BLK_SIZE, fcs);
      } else {
        rc = calc_checksum_simple(file_buf, BIOS_VERIFY_PKT_SIZE, fcs);
        if (rc == 0) {
          rc = calc_checksum_simple(file_buf + BIOS_VERIFY_PKT_SIZE,
                                    BIOS_VERIFY_PKT_SIZE, fcs + SIMPLE_DIGEST_LENGTH);
        }
      }
      if (rc != 0) {
        fprintf(stderr, "calc_checksum error: %d (cs_len %d)\n", rc, cs_len);
        goto out;
      }
    }
    if (dedup) {
      rc = get_block_checksum(slot_id, write_offset, cs_len, cs);
      if (rc == 0 && memcmp(cs, fcs, cs_len) == 0) {
        write_offset += BIOS_UPDATE_BLK_SIZE;
        file_buf_num_bytes = 0;
        num_blocks_skipped++;
        attempts = NUM_ATTEMPTS;
        continue;
      }
    }
    while (file_buf_pos < file_buf_num_bytes) {
      size_t count = file_buf_num_bytes - file_buf_pos;
      // 4K USB packets and SHA256 checksums were added together,
      // so if we have SHA256 checksum, we can use big packets as well.
      size_t limit = (cs_len == STRONG_DIGEST_LENGTH ? USB_DAT_SIZE_BIG : USB_DAT_SIZE);
      if (count > limit) count = limit;
      bic_usb_packet *pkt = (bic_usb_packet *) (file_buf + file_buf_pos - sizeof(bic_usb_packet));
      pkt->netfn = NETFN_OEM_1S_REQ << 2;
      pkt->cmd = CMD_OEM_1S_UPDATE_FW;
      pkt->iana[0] = 0x9c;
      pkt->iana[1] = 0x9c;
      pkt->iana[2] = 0x0;
      pkt->target = UPDATE_BIOS;
      pkt->offset = write_offset + file_buf_pos;
      pkt->length = count;
      udev->epaddr = USB_INPUT_PORT;
      rc = send_bic_usb_packet(udev, pkt);
      if (rc < 0) {
        fprintf(stderr, "failed to write %zu bytes @ %zu: %d\n", count, write_offset, rc);
        send_packet_fail = true;
        break;  //prevent the endless while loop.
      }

      udev->epaddr = USB_OUTPUT_PORT;
      rc = receive_bic_usb_packet(udev, pkt);
      if (rc < 0) {
        fprintf(stderr, "Return code : %d\n", rc);
        send_packet_fail = true;
        break;  //prevent the endless while loop.
      }

      file_buf_pos += count;
    }

    if (send_packet_fail) {
      attempts--;
      continue;
    }

    // Verify written data.
    if (verify) {
      rc = get_block_checksum(slot_id, write_offset, cs_len, cs);
      if (rc != 0) {
        fprintf(stderr, "get_block_checksum @ %zu failed (cs_len %d)\n", write_offset, cs_len);
        attempts--;
        continue;
      }
      if (memcmp(cs, fcs, cs_len) != 0) {
        fprintf(stderr, "Data checksum mismatch @ %zu (cs_len %d, 0x%016"PRIx64" vs 0x%016"PRIx64")\n",
            write_offset, cs_len, *((uint64_t *) cs), *((uint64_t *) fcs));
        attempts--;
        continue;
      }
    }
    write_offset += BIOS_UPDATE_BLK_SIZE;
    file_buf_num_bytes = 0;
    num_blocks_written++;
    attempts = NUM_ATTEMPTS;
  }
  if (attempts == 0) {
    fprintf(stderr, "failed.\n");
    goto out;
  }

  fprintf(stderr, "finished.\n");

  ret = 0;

out:
  free(buf);
  return ret;
}

int
bic_close_usb_dev(usb_dev* udev)
{
  if (libusb_release_interface(udev->handle, udev->ci) < 0) {
    printf("Couldn't release the interface 0x%X\n", udev->ci);
  }

  if (udev->handle != NULL )
    libusb_close(udev->handle);
  libusb_exit(NULL);

  return 0;
}

int
update_bic_usb_bios(uint8_t slot_id, uint8_t comp, int fd)
{
  struct timeval start, end;
  char key[64];
  int ret = -1;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  udev->ci = 1;
  udev->epaddr = USB_INPUT_PORT;

  // init usb device
  ret = bic_init_usb_dev(slot_id, udev, SB_USB_PRODUCT_ID, SB_USB_VENDOR_ID);
  if (ret < 0) {
    goto error_exit;
  }

  gettimeofday(&start, NULL);

  // sending file
  ret = bic_update_fw_usb(slot_id, comp, fd, udev);
  if (ret < 0)
    goto error_exit;

  gettimeofday(&end, NULL);
  if (comp == FW_BIOS) {
    fprintf(stderr, "Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  }

  ret = 0;
error_exit:
  sprintf(key, "fru%u_fwupd", slot_id);
  remove(key);

  // close usb device
  bic_close_usb_dev(udev);
  return ret;
}
