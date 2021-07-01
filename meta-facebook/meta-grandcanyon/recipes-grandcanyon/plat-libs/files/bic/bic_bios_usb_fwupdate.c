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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include "bic_fwupdate.h"
#include "bic_bios_fwupdate.h"


#define TI_VENDOR_ID  0x1CBE
#define TI_PRODUCT_ID 0x0007

#define USB_PKT_SIZE 0x200
#define USB_DAT_SIZE (USB_PKT_SIZE - USB_PKT_HDR_SIZE)
#define USB_PKT_SIZE_BIG 0x1000
#define USB_DAT_SIZE_BIG (USB_PKT_SIZE_BIG - USB_PKT_HDR_SIZE)

#define MAX_USB_CFG_DATA_SIZE 512
#define MAX_USB_UPDATE_TARGET_LEN 64

#define USB_PKT_DATA_OFFSET 7

#define BIOS_UPDATE_BLK_SIZE (64*1024)
#define BIOS_UPDATE_IMG_SIZE (32*1024*1024)
#define SIMPLE_DIGEST_LENGTH 4
#define STRONG_DIGEST_LENGTH SHA256_DIGEST_LENGTH

typedef struct {
  uint8_t dummy;
  uint32_t offset;
  uint16_t length;
  uint8_t data[0];
} __attribute__((packed)) bic_usb_packet;
#define USB_PKT_HDR_SIZE (sizeof(bic_usb_packet))

#define NUM_ATTEMPTS 5

int interface_ref = 0;
int alt_interface = 0 , interface_number = 0;

struct bic_get_fw_cksum_sha256_req {
  uint8_t iana_id[SIZE_IANA_ID];
  uint8_t target;
  uint32_t offset;
  uint32_t length;
} __attribute__((packed));

struct bic_get_fw_cksum_sha256_res {
  uint8_t iana_id[SIZE_IANA_ID];
  uint8_t cksum[32];
} __attribute__((packed));


// Read checksum of various components
int
bic_get_fw_cksum(uint8_t target, uint32_t offset, uint32_t len, uint8_t *ver) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x9c, 0x9c, 0x00}; // IANA ID
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;
  int retries = 3;

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  // Fill the offset
  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  // Fill the length
  tbuf[8] = (len) & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;
  tbuf[10] = (len >> 16) & 0xFF;
  tbuf[11] = (len >> 24) & 0xFF;

bic_send:
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_CKSUM, tbuf, 12, rbuf, &rlen);
  if ((ret || (rlen != 4 + SIZE_IANA_ID)) && (retries--)) {  // checksum has to be 4 bytes
    sleep(1);
    syslog(LOG_ERR, "bic_get_fw_cksum: target %d, offset: %d, ret: %d, rlen: %d\n", target, offset, ret, rlen);
    goto bic_send;
  }
  if (ret || (rlen != 4+SIZE_IANA_ID)) {
    return -1;
  }

#ifdef DEBUG
  printf("cksum returns: %x:%x:%x::%x:%x:%x:%x\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6]);
#endif

  //Ignore IANA ID
  memcpy(ver, &rbuf[SIZE_IANA_ID], rlen - SIZE_IANA_ID);

  return ret;
}

int
bic_get_fw_cksum_sha256(uint8_t target, uint32_t offset, uint32_t len, uint8_t *cksum) {
  int ret = 0;
  struct bic_get_fw_cksum_sha256_req req = {
    .iana_id = {0x9c, 0x9c, 0x00},
    .target = target,
    .offset = offset,
    .length = len,
  };
  struct bic_get_fw_cksum_sha256_res res = {0};
  uint8_t rlen = sizeof(res);

  if (cksum == NULL) {
    syslog(LOG_ERR, "%s(): NULL checksum parameter", __func__);
    return -1;
  }

  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_FW_CKSUM_SHA256, (uint8_t *) &req, sizeof(req), (uint8_t *) &res, &rlen);
  if ((ret != 0) || (rlen != sizeof(res))) {
    syslog(LOG_ERR, "%s(): unexpected response, length = %u, ret = %d", __func__, rlen, ret);
    return -1;
  }
  memcpy(cksum, res.cksum, sizeof(res.cksum));

  return 0;
}

int
verify_bios_image(int fd, long size) {
  int ret = -1;
  int rc, i;
  uint32_t offset = 0;
  uint32_t tcksum, gcksum = 0;
  volatile uint16_t count;
  uint8_t target, last_pkt = 0x00;
  uint8_t *tbuf = NULL;

  // Checksum calculation for BIOS image
  tbuf = malloc(BIOS_VERIFY_PKT_SIZE * sizeof(uint8_t));
  if (tbuf == NULL) {
    return -1;
  }

  if ((offset = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno = %d", __func__, offset, errno);
    free(tbuf);
    return -1;
  }
  while (1) {
    count = read(fd, tbuf, BIOS_VERIFY_PKT_SIZE);
    if (count == 0) {
      if (offset >= size) {
        ret = 0;
      }
      break;
    }

    tcksum = 0;
    for (i = 0; i < count; i++) {
      tcksum += tbuf[i];
    }

    target = ((offset + count) >= size) ? (UPDATE_BIOS | last_pkt) : UPDATE_BIOS;

    // Get the checksum of binary image
    rc = bic_get_fw_cksum(target, offset, count, (uint8_t*)&gcksum);
    if (rc) {
      printf("get checksum failed, offset:0x%x\n", offset);
      break;
    }

    // Compare both and see if they match or not
    if (gcksum != tcksum) {
      printf("checksum does not match, offset:0x%x, 0x%x:0x%x\n", offset, tcksum, gcksum);
      break;
    }

    offset += count;
  }

  free(tbuf);
  return ret;
}

int print_configuration(struct libusb_device_handle *hDevice, struct libusb_config_descriptor *config) {
  char *data;
  int index = 0;

  data = (char *)malloc(MAX_USB_CFG_DATA_SIZE);
  memset(data, 0, MAX_USB_CFG_DATA_SIZE);
  index = config->iConfiguration;

  libusb_get_string_descriptor_ascii(hDevice, index, (unsigned char*) data, sizeof(data));
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

int active_config(struct libusb_device *dev,struct libusb_device_handle *handle) {
  struct libusb_device_handle *hDevice_req;
  struct libusb_config_descriptor *config;
  int altsetting_index, endpoint_index, interface_index = 0;

  hDevice_req = handle;
  libusb_get_active_config_descriptor(dev, &config);
  print_configuration(hDevice_req, config);

  for (interface_index = 0;interface_index < config->bNumInterfaces; interface_index++) {
    const struct libusb_interface *iface = &config->interface[interface_index];
    for (altsetting_index=0; altsetting_index < iface->num_altsetting; altsetting_index++) {
      const struct libusb_interface_descriptor *altsetting = &iface->altsetting[altsetting_index];
      printf("      Interface:\n");
      printf("      bInterfaceNumber:   %d\n", altsetting->bInterfaceNumber);
      printf("      bAlternateSetting:  %d\n", altsetting->bAlternateSetting);
      printf("      bNumEndpoints:      %d\n", altsetting->bNumEndpoints);
      printf("      bInterfaceClass:    %d\n", altsetting->bInterfaceClass);
      printf("      bInterfaceSubClass: %d\n", altsetting->bInterfaceSubClass);
      printf("      bInterfaceProtocol: %d\n", altsetting->bInterfaceProtocol);
      printf("      iInterface:         %d\n\n", altsetting->iInterface);

      for(endpoint_index = 0; endpoint_index < altsetting->bNumEndpoints; endpoint_index++) {
        const struct libusb_endpoint_descriptor *endpoint = &altsetting->endpoint[endpoint_index];
        alt_interface = altsetting->bAlternateSetting;
        interface_number = altsetting->bInterfaceNumber;
        printf("        EndPoint Descriptors:\n");
        printf("        bLength: %d\n", endpoint->bLength);
        printf("        bDescriptorType: 0x%x\n", endpoint->bDescriptorType);
        printf("        bEndpointAddress: 0x%x\n", endpoint->bEndpointAddress);
        printf("        Maximum Packet Size: 0x%x\n", endpoint->wMaxPacketSize);
        printf("        bmAttributes: 0x%x\n", endpoint->bmAttributes);
        printf("        bInterval: 0x%x\n", endpoint->bInterval);
      }
    }
    printf("-----------------------------------\n");
  }

  libusb_free_config_descriptor(NULL);

  return 0;
}

static int
_send_bic_usb_packet(usb_dev* udev, bic_usb_packet *pkt) {
  const int transferlen = pkt->length + USB_PKT_HDR_SIZE;
  int transferred = 0;
  int retries = 3;
  int ret;

  // _debug_bic_usb_packet(pkt);
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t*)pkt, transferlen, &transferred, 3000);
    if((ret != 0) || (transferlen != transferred)) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n", ret, transferred);
      printf("Retry since  %s\n", libusb_error_name(ret));
      retries--;
      if (retries == 0) {
        return -1;
      }
      msleep(100);
    } else {
      break;
    }
  }

  return 0;
}

int
bic_init_usb_dev(usb_dev* udev) {
  int ret = 0;
  int index = 0;
  char found = 0;
  ssize_t cnt = 0 ;
  int recheck = MAX_CHECK_USB_DEV_TIME;

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

      if((TI_VENDOR_ID == udev->desc.idVendor) && (TI_PRODUCT_ID == udev->desc.idProduct)) {
        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*) udev->manufacturer, sizeof(udev->manufacturer));
        if ( ret < 0 ) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }
        
        ret = libusb_get_port_numbers(udev->dev, udev->path, sizeof(udev->path));
        if (ret < 0) {
          printf("Error get port number\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        // TODO: if BMC multiple usb device node, check udev->path[1] to select port to BIC

        printf("%04x:%04x (bus %d, device %d)", udev->desc.idVendor, udev->desc.idProduct, libusb_get_bus_number(udev->dev), libusb_get_device_address(udev->dev));
        printf(" path: %d", udev->path[0]);
        for (index = 1; index < ret; index++) {
          printf(".%d", udev->path[index]);
        }
        printf("\n");

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iProduct, (unsigned char*) udev->product, sizeof(udev->product));
        if ( ret < 0 ) {
          printf("Error get Product string descriptor -- exit\n");
          libusb_free_device_list(udev->devs, 1);
          goto error_exit;
        }

        printf("Manufactured : %s\n", udev->manufacturer);
        printf("Product : %s\n", udev->product);
        printf("----------------------------------------\n");
        printf("Device Descriptors:\n");
        printf("Vendor ID : %x\n", udev->desc.idVendor);
        printf("Product ID : %x\n", udev->desc.idProduct);
        printf("Serial Number : %x\n", udev->desc.iSerialNumber);
        printf("Size of Device Descriptor : %d\n", udev->desc.bLength);
        printf("Type of Descriptor : %d\n", udev->desc.bDescriptorType);
        printf("USB Specification Release Number : %d\n", udev->desc.bcdUSB);
        printf("Device Release Number : %d\n", udev->desc.bcdDevice);
        printf("Device Class : %d\n", udev->desc.bDeviceClass);
        printf("Device Sub-Class : %d\n", udev->desc.bDeviceSubClass);
        printf("Device Protocol : %d\n", udev->desc.bDeviceProtocol);
        printf("Max. Packet Size : %d\n", udev->desc.bMaxPacketSize0);
        printf("No. of Configuraions : %d\n", udev->desc.bNumConfigurations);

        found = 1;
        break;
      }
    }

    if (found != 1) {
      sleep(3);
    } else {
      break;
    }
  } while ((--recheck) > 0);


  if (found == 0) {
    printf("Device NOT found -- exit\n");
    libusb_free_device_list(udev->devs, 1);
    ret = -1;
    goto error_exit;
  }

  ret = libusb_get_configuration(udev->handle, &udev->config);
  if (ret != 0) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(udev->devs, 1);
    goto error_exit;
  }

  printf("Configured value : %d\n", udev->config);
  if (udev->config != 1) {
    libusb_set_configuration(udev->handle, 1);
    if ( ret != 0 ) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(udev->devs, 1);
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
      libusb_free_device_list(udev->devs, 1);
      goto error_exit;
    }
  }

  ret = libusb_claim_interface(udev->handle, udev->ci);
  if (ret < 0) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(udev->devs, 1);
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

  if ((buf == NULL) || (out == NULL)) {
    syslog(LOG_ERR, "%s(): NULL parameter", __func__);
    return -1;
  }
  while (len-- > 0) {
    cs += *buf++;
  }
  *((uint32_t *) out) = cs;
  return 0;
}

static int
calc_checksum_sha256(const void *buf, size_t len, uint8_t *out) {
  if ((buf == NULL) || (out == NULL)) {
    syslog(LOG_ERR, "%s(): NULL parameter", __func__);
    return -1;
  }
  SHA256_CTX ctx = {0};
  memset(out, 0, STRONG_DIGEST_LENGTH);
  if (SHA256_Init(&ctx) != 1) {
    return -1;
  }
  if (SHA256_Update(&ctx, buf, len) != 1) {
    return -1;
  }
  if (SHA256_Final(out, &ctx) != 1) {
    return -1;
  }
  return 0;
}

static bool
bic_have_checksum_sha256() {
  uint8_t cs[STRONG_DIGEST_LENGTH] = {0};
  return (bic_get_fw_cksum_sha256(UPDATE_BIOS, 0, BIOS_UPDATE_BLK_SIZE, cs) == 0);
}

static int
get_block_checksum(size_t offset, int cs_len, uint8_t *out) {
  int rc = 0;

  if (out == NULL) {
    syslog(LOG_ERR, "%s(): NULL parameter", __func__);
    return -1;
  }
  if (cs_len == STRONG_DIGEST_LENGTH) {
    rc = bic_get_fw_cksum_sha256(UPDATE_BIOS, offset, BIOS_UPDATE_BLK_SIZE, out);
  } else {
    rc = bic_get_fw_cksum(UPDATE_BIOS, offset, BIOS_VERIFY_PKT_SIZE, out);
    if (rc == 0) {
      rc = bic_get_fw_cksum(UPDATE_BIOS, offset + BIOS_VERIFY_PKT_SIZE,
                            BIOS_VERIFY_PKT_SIZE, out + SIMPLE_DIGEST_LENGTH);
    }
  }
  return rc;
}

int
bic_update_fw_usb(uint8_t comp, const char *image_file, usb_dev* udev) {
  struct stat st;
  int ret = -1, fd = -1, rc = 0, i = 0;
  int num_blocks_written = 0, num_blocks_skipped = 0, num_blocks;
  uint8_t *buf = NULL;
  uint32_t file_sz = 0, write_offset = 0, file_offset = 0, num_to_read;
  ssize_t num_read;
  uint8_t fcs[STRONG_DIGEST_LENGTH] = {0}, cs[STRONG_DIGEST_LENGTH] = {0};
  int cs_len = STRONG_DIGEST_LENGTH;
  int attempts = NUM_ATTEMPTS;
  const char *dedup_env = getenv("DEDUP");
  const char *verify_env = getenv("VERIFY");
  bool dedup = (dedup_env != NULL ? (!strcmp(dedup_env, "on")) : true);
  bool verify = (verify_env != NULL ? (!strcmp(verify_env, "on")) : true);

  if (comp == FW_BIOS) {
    write_offset = 0;
  } else {
    printf("ERROR: not supported component [comp:%u]!\n", comp);
    goto error_exit;
  }

  fd = open(image_file, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "%s: open fails for path: %s\n", __func__, image_file);
    goto error_exit;
  }
  fstat(fd, &st);
  file_sz = st.st_size;  
  buf = malloc(USB_PKT_HDR_SIZE + BIOS_UPDATE_BLK_SIZE);
  if (buf == NULL) {
    printf("Failed to allocate memory\n");
    goto error_exit;
  }

  num_blocks = file_sz / BIOS_UPDATE_BLK_SIZE;
  
  if (!bic_have_checksum_sha256()) {
    printf("Strong checksum function is not available, disabling "
            "deduplication. Update BIC firmware to at least v21.01\n");
    dedup = false;
    cs_len = SIMPLE_DIGEST_LENGTH * 2;
  }
  printf("Updating from %s, dedup is %s, verification is %s.\n",
            image_file, (dedup ? "on" : "off"), (verify ? "on" : "off"));  
  while (attempts > 0) {
    uint8_t *file_buf = buf + USB_PKT_HDR_SIZE;
    size_t file_buf_pos = 0;
    size_t file_buf_num_bytes = 0;
    if (write_offset > 0) {
      printf("\r%d/%d blocks (%d written, %d skipped)...",
          num_blocks_written + num_blocks_skipped,
          num_blocks, num_blocks_written, num_blocks_skipped);
      fflush(stderr);
    }
    if (file_offset >= file_sz) {
      break;
    }
    // Read a block of data from file.
    if (attempts < NUM_ATTEMPTS) {
      // If retrying, seek back to the correct position.
      lseek(fd, file_offset, SEEK_SET);
    }
    file_buf_num_bytes = 0;
    do {
      num_to_read = BIOS_UPDATE_BLK_SIZE - file_buf_num_bytes;
      num_read = read(fd, file_buf, num_to_read);
      if (num_read < 0) {
        printf("read error: %d\n", errno);
        goto error_exit;
      }
      file_buf_num_bytes += num_read;
    } while (file_buf_num_bytes < BIOS_UPDATE_BLK_SIZE &&
             errno == EINTR);
    // Pad to 64K with 0xff, if needed.
    for (i = file_buf_num_bytes; i < BIOS_UPDATE_BLK_SIZE; i++) {
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
        printf("calc_checksum error: %d (cs_len %d)\n", rc, cs_len);
        goto error_exit;
      }
    }
    if (dedup) {
      rc = get_block_checksum(write_offset, cs_len, cs);
      if (rc == 0 && memcmp(cs, fcs, cs_len) == 0) {
        write_offset += BIOS_UPDATE_BLK_SIZE;
        file_offset += file_buf_num_bytes;
        num_blocks_skipped++;
        attempts = NUM_ATTEMPTS;
        continue;
      }
    }
    while ((file_buf_pos < file_buf_num_bytes) && (attempts > 0)) {
      int count = file_buf_num_bytes - file_buf_pos;
      // 4K USB packets and SHA256 checksums were added together,
      // so if we have SHA256 checksum, we can use big packets as well.
      size_t limit = (cs_len == STRONG_DIGEST_LENGTH ? USB_DAT_SIZE_BIG : USB_DAT_SIZE);
      if (count > limit) count = limit;
      bic_usb_packet *pkt = (bic_usb_packet *) (file_buf + file_buf_pos - sizeof(bic_usb_packet));
      pkt->dummy = CMD_OEM_1S_UPDATE_FW;
      pkt->offset = write_offset + file_buf_pos;
      pkt->length = count;
      rc = _send_bic_usb_packet(udev, pkt);
      if (rc < 0) {
        printf("failed to write %d bytes @ %x: %d\n", count, write_offset, rc);
        attempts--;
        continue;
      }
      file_buf_pos += count;
    }
    // Verify written data.
    if (verify) {
      rc = get_block_checksum( write_offset, cs_len, cs);
      if (rc != 0) {
        printf("get_block_checksum @ %x failed (cs_len %d)\n", write_offset, cs_len);
        attempts--;
        continue;
      }
      if (memcmp(cs, fcs, cs_len) != 0) {
        printf("Data checksum mismatch @ %x (cs_len %d, 0x%016llx vs 0x%016llx)\n",
            write_offset, cs_len, *((uint64_t *) cs), *((uint64_t *) fcs));
        attempts--;
        continue;
      }
    }
    write_offset += BIOS_UPDATE_BLK_SIZE;
    file_offset += file_buf_num_bytes;
    num_blocks_written++;
    attempts = NUM_ATTEMPTS;
  }
  if (attempts == 0) {
    printf(" Failed after %d retry.\n", NUM_ATTEMPTS);
    goto error_exit;
  }

  printf(" finished.\n");
  ret = 0;

error_exit:
  if (fd >= 0) {
    close(fd);
  }
  free(buf);

  return ret;
}

int
bic_close_usb_dev(usb_dev* udev) {
  if (libusb_release_interface(udev->handle, udev->ci) < 0) {
    printf("Couldn't release the interface 0x%X\n", udev->ci);
  }

  if (udev->handle != NULL) {
    libusb_close(udev->handle);
  }
  libusb_exit(NULL);

  return 0;
}

int
update_bic_usb_bios(uint8_t comp, char *image) {
  struct timeval start, end;
  int ret = -1;
  char key[MAX_KEY_LEN] = {0};
  usb_dev bic_udev;
  usb_dev* udev = &bic_udev;

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  ret = bic_init_usb_dev(udev);
  if (ret < 0) {
    goto error_exit;
  }

  printf("Input: %s, USB timeout: 3000ms\n", image);
  gettimeofday(&start, NULL);

  // sending file
  ret = bic_update_fw_usb(comp, image, udev);
  if (ret < 0)
    goto error_exit;

  gettimeofday(&end, NULL);
  if (comp == FW_BIOS) {
    printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  }

  ret = 0;

error_exit:
  snprintf(key, sizeof(key), "fru%d_fwupd", FRU_SERVER);
  remove(key);  // close usb device
  bic_close_usb_dev(udev);

  return ret;
}
