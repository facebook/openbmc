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
#include <fcntl.h>
#include <sys/time.h>
#include <openssl/sha.h>
#include "bic_bios_fwupdate.h"
#include "bic_ipmi.h"

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

//PRoT update
#define XFR_STAGING_OFFSET (48*1024*1024)
#define XFR_WORKING_OFFSET (56*1024*1024)

#define CL_SB_USB_PORT  4
#define CL_1OU_USB_PORT 2
#define HD_SB_USB_PORT  3
#define HD_1OU_USB_PORT 1

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
  struct libusb_config_descriptor *config = NULL;
  int altsetting_index = 0, endpoint_index = 0, interface_index = 0;

  hDevice_req = handle;
  libusb_get_active_config_descriptor(dev, &config);
  print_configuration(hDevice_req, config);

  for (interface_index=0;interface_index < config->bNumInterfaces;interface_index++) {
    const struct libusb_interface *iface = &config->interface[interface_index];
    for (altsetting_index=0; altsetting_index < iface->num_altsetting ;altsetting_index++) {
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

  libusb_free_config_descriptor(config);
  return 0;
}

int
send_bic_usb_packet(usb_dev* udev, uint8_t* pkt, const int transferlen)
{
  int transferred = 0;
  int retries = 3;
  int ret;

  // _debug_bic_usb_packet(pkt);
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, pkt, transferlen, &transferred, 3000);
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
receive_bic_usb_packet(usb_dev* udev, uint8_t* pkt, const int receivelen)
{
  int received = 0;
  int total_received = 0;
  int retries = 3;
  int ret;
  bic_usb_res_packet* res_hdr = (bic_usb_res_packet*)pkt;

  // _debug_bic_usb_packet(pkt);
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, pkt, receivelen, &received, 0);
    if(ret != 0) {
      printf("Error in receiving data! err = %d (%s)\n", ret, libusb_error_name(ret));
      retries--;
      if (!retries) {
        return -1;
      }
      msleep(100);
      continue;
    }

    total_received += received;
    if (total_received >= receivelen) {
      break;
    }

    //Expected data may not received completely in one bulk transfer
    //continue to get remaining data
    pkt += received;

  }

  // Return CC code
  return res_hdr->cc;
}

int
bic_init_usb_dev(uint8_t slot_id, uint8_t comp, usb_dev* udev, const uint16_t product_id, const uint16_t vendor_id)
{
  int ret, index, recheck;
  int sb_type = fby35_common_get_slot_type(slot_id);
  ssize_t cnt;
  bool found = false;
  uint8_t bmc_location = 0, port = 0;

  if (udev == NULL) {
    syslog(LOG_ERR, "%s[%u] udev shouldn't be null!", __func__, slot_id);
    return -1;
  }

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    syslog(LOG_ERR, "%s[%u] Cannot get the location of BMC", __func__, slot_id);
    return -1;
  }

  if (libusb_init(NULL) < 0) {
    printf("Failed to initialise libusb\n");
    return -1;
  }
  printf("Init libusb Successful!\n");

  for (recheck = 0; recheck < MAX_CHECK_DEVICE_TIME; ++recheck) {
    cnt = libusb_get_device_list(NULL, &udev->devs);
    if (cnt < 0) {
      printf("There are no USB devices on bus -- exit\n");
      return -1;
    }
    index = 0;
    while ((udev->dev = udev->devs[index++]) != NULL) {
      ret = libusb_get_device_descriptor(udev->dev, &udev->desc);
      if (ret < 0) {
        printf("Failed to get device descriptor\n");
        continue;
      }

      if ((vendor_id == udev->desc.idVendor) && (product_id == udev->desc.idProduct)) {
        ret = libusb_get_port_numbers(udev->dev, udev->path, sizeof(udev->path));
        if (ret < 0) {
          printf("Error get port number\n");
          continue;
        }

        if (bmc_location == BB_BMC) {
          if (udev->path[1] != slot_id) {
            continue;
          }
          switch (comp) {
            case FW_BIOS:
            case FW_BIOS_SPIB:
            case FW_PROT:
            case FW_PROT_SPIB:
              port = (sb_type == SERVER_TYPE_HD) ? HD_SB_USB_PORT : CL_SB_USB_PORT;
              break;
            case FW_1OU_CXL:
              port = (sb_type == SERVER_TYPE_HD) ? HD_1OU_USB_PORT : CL_1OU_USB_PORT;
              break;
            default:
              fprintf(stderr, "ERROR: not supported component [comp:%u]!\n", comp);
              libusb_free_device_list(udev->devs, 1);
              return -1;
          }
          if (udev->path[2] != port) {
            continue;
          }
        }
        printf("%04x:%04x (bus %d, device %d)", udev->desc.idVendor, udev->desc.idProduct, libusb_get_bus_number(udev->dev), libusb_get_device_address(udev->dev));
        printf(" path: %d", udev->path[0]);
        for (index = 1; index < ret; index++) {
          printf(".%d", udev->path[index]);
        }
        printf("\n");

        ret = libusb_open(udev->dev, &udev->handle);
        if (ret < 0) {
          printf("Error opening device\n");
          break;
        }

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*)udev->manufacturer, sizeof(udev->manufacturer));
        if (ret < 0) {
          printf("Error obtaining the Manufacturer string descriptor\n");
          break;
        }

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iProduct, (unsigned char*)udev->product, sizeof(udev->product));
        if (ret < 0) {
          printf("Error obtaining the Product string descriptor\n");
          break;
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

        found = true;
        break;
      }
    }
    if (found) {
      break;
    }

    libusb_free_device_list(udev->devs, 1);
    if (udev->handle != NULL) {
      libusb_close(udev->handle);
      udev->handle = NULL;
    }
    sleep(3);
  }

  if (found == false) {
    printf("Device NOT found -- exit\n");
    return -1;
  }

  ret = libusb_get_configuration(udev->handle, &udev->config);
  if (ret != 0) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(udev->devs, 1);
    return -1;
  }

  printf("Configured value : %d\n", udev->config);
  if (udev->config != 1) {
    ret = libusb_set_configuration(udev->handle, 1);
    if (ret != 0) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(udev->devs, 1);
      return -1;
    }
    printf("Device is in configured state!\n");
  }

  if (libusb_kernel_driver_active(udev->handle, udev->ci) == 1) {
    printf("Kernel Driver Active\n");
    if (libusb_detach_kernel_driver(udev->handle, udev->ci) == 0) {
      printf("Kernel Driver Detached!");
    } else {
      printf("Couldn't detach kernel driver -- exit\n");
      libusb_free_device_list(udev->devs, 1);
      return -1;
    }
  }

  ret = libusb_claim_interface(udev->handle, udev->ci);
  if (ret < 0) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(udev->devs, 1);
    return -1;
  }

  printf("Claimed Interface: %d, EP addr: 0x%02X\n", udev->ci, udev->epaddr);
  active_config(udev->dev, udev->handle);
  libusb_free_device_list(udev->devs, 1);

  return 0;
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
  int write_target = 0;
  size_t image_size = 0;
  bool last_block = false;
  struct stat finfo;
  fstat(fd,&finfo);
  image_size = finfo.st_size;

  const char *what = NULL;
  switch (comp) {
    case FW_BIOS:
      what = "BIOS";
      write_offset = 0;
      write_target = UPDATE_BIOS;
      break;
    case FW_BIOS_SPIB:
      what = "BIOS SPIB";
      write_offset = 0;
      write_target = UPDATE_BIOS_SPIB;
      break;
    case FW_PROT:
    case FW_PROT_SPIB:
      what = "PRoT";
      if(bic_is_prot_bypass(slot_id)) {
        printf("PRoT is Bypass mode, updating to offset : 0x%08X \n",XFR_WORKING_OFFSET);
        write_offset = XFR_WORKING_OFFSET;
      } else {
        printf("PRoT is PFR mode, updating to offset : 0x%08X \n",XFR_STAGING_OFFSET);
        write_offset = XFR_STAGING_OFFSET;
      }
      write_target = (comp == FW_PROT) ? UPDATE_BIOS : UPDATE_BIOS_SPIB;
      break;

    case FW_1OU_CXL:
      what = "CXL";
      write_offset = 0;
      write_target = UPDATE_CXL;
      break;

    default:
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

    if ((num_blocks_written * BIOS_UPDATE_BLK_SIZE + file_buf_num_bytes) >= image_size) {
      last_block = true;
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

      if( last_block && (file_buf_pos + count >= file_buf_num_bytes)) {
        //Enable update end flag for last packet
        write_target |= 0x80;
      }

      bic_usb_packet *pkt = (bic_usb_packet *) (file_buf + file_buf_pos - sizeof(bic_usb_packet));
      pkt->netfn = NETFN_OEM_1S_REQ << 2;
      pkt->cmd = CMD_OEM_1S_UPDATE_FW;
      // Fill the IANA ID
      memcpy(pkt->iana, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
      pkt->target = write_target;
      pkt->offset = write_offset + file_buf_pos;
      pkt->length = count;
      udev->epaddr = USB_INPUT_PORT;
      rc = send_bic_usb_packet(udev, (uint8_t *)pkt, pkt->length + USB_PKT_HDR_SIZE);
      if (rc < 0) {
        fprintf(stderr, "failed to write %zu bytes @ %zu: %d\n", count, write_offset, rc);
        send_packet_fail = true;
        break;  //prevent the endless while loop.
      }

      udev->epaddr = USB_OUTPUT_PORT;
      rc = receive_bic_usb_packet(udev, (uint8_t *)pkt, USB_PKT_RES_HDR_SIZE);
      if (rc != 0) {
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
bic_dump_fw_usb(uint8_t slot_id, uint8_t comp, char *path, usb_dev* udev) {
  int ret = -1, rc = -1, fd = -1;
  uint32_t offset = 0, next_doffset;
  uint32_t dsize;
  uint32_t img_size = 0;
  uint8_t read_count;
  uint8_t buf[256];
  bic_usb_dump_req_packet *pkt = (bic_usb_dump_req_packet *)malloc(sizeof(bic_usb_dump_req_packet));
  bic_usb_dump_res_packet *res = (bic_usb_dump_res_packet*)buf;
  uint8_t res_packet_size = udev->desc.bMaxPacketSize0;
  uint8_t res_header_size = &(res->data[0]) - &(res->netfn);
  uint8_t res_payload_max_size = res_packet_size - res_header_size;

  switch (fby35_common_get_slot_type(slot_id)) {
    case SERVER_TYPE_HD:
      img_size = 0x1000000;
      break;
    case SERVER_TYPE_CL:
    case SERVER_TYPE_GL:
      img_size = 0x4000000;
      break;
    default:
      syslog(LOG_WARNING, "%s() Unknown slot type, dmup full flash(64MB)", __func__);
      img_size = 0x4000000;
  }

  if (comp != FW_BIOS) {
    printf("ERROR: only support dump BIOS image!\n");
    goto error_exit;
  }
  printf("dumping fw on slot %d:\n", slot_id);

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    goto error_exit;
  }

  // Write chunks of binary data in a loop
  dsize = img_size / 100;
  next_doffset = offset + dsize;
  while (1) {
    read_count = ((offset + res_payload_max_size) <= img_size) ? res_payload_max_size : (img_size - offset);

    // read image from Bridge-IC
    pkt->netfn = NETFN_OEM_1S_REQ << 2;
    pkt->cmd = CMD_OEM_1S_READ_FW_IMAGE;
    // Fill the IANA ID
    memcpy(pkt->iana, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
    pkt->target = DUMP_BIOS;
    pkt->offset = offset;
    pkt->length = read_count;
    udev->epaddr = USB_INPUT_PORT;
    rc = send_bic_usb_packet(udev, (uint8_t *)pkt, sizeof(bic_usb_dump_req_packet));
    if (rc != 0) {
      fprintf(stderr, "failed to write %u bytes @ %u: %d\n", read_count, offset, rc);
      goto error_exit;
    }

    memset(buf, 0xFF, 256);
    udev->epaddr = USB_OUTPUT_PORT;
    rc = receive_bic_usb_packet(udev, buf, read_count + res_header_size);
    if (rc != 0) {
      fprintf(stderr, "Return code : %d\n", rc);
      goto error_exit;
    }

    // Write to file
    rc = write(fd, res->data, read_count);
    if (rc <= 0) {
      goto error_exit;
    }

    // Update counter
    offset += rc;
    if (offset >= next_doffset) {
      switch (comp) {
        case FW_BIOS:
          _set_fw_update_ongoing(slot_id, 60);
          printf("\rdumped bios: %u %%", offset/dsize);
          break;
      }
      fflush(stdout);
      next_doffset += dsize;
    }

    if (offset >= img_size)
      break;
  }

  ret = 0;

error_exit:
  printf("\n");
  if (fd > 0 ) {
    close(fd);
  }
  free(pkt);

  return ret;
}

int
bic_close_usb_dev(usb_dev* udev)
{
  if (udev->handle != NULL) {
    if (libusb_release_interface(udev->handle, udev->ci) < 0) {
      printf("Couldn't release the interface 0x%X\n", udev->ci);
    }
    libusb_close(udev->handle);
  }
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

  udev->handle = NULL;
  udev->ci = 1;
  udev->epaddr = USB_INPUT_PORT;

  // init usb device
  ret = bic_init_usb_dev(slot_id, comp, udev, SB_USB_PRODUCT_ID, SB_USB_VENDOR_ID);
  if (ret < 0) {
    goto error_exit;
  }

  gettimeofday(&start, NULL);

  // sending file
  ret = bic_update_fw_usb(slot_id, comp, fd, udev);
  if (ret < 0)
    goto error_exit;

  gettimeofday(&end, NULL);

  fprintf(stderr, "Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  ret = 0;
error_exit:
  sprintf(key, "fru%u_fwupd", slot_id);
  remove(key);

  // close usb device
  bic_close_usb_dev(udev);
  return ret;
}

int
dump_bic_usb_bios(uint8_t slot_id, uint8_t comp, char *path)
{
  struct timeval start, end;
  char key[64];
  int ret = -1;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  udev->handle = NULL;
  udev->ci = 1;
  udev->epaddr = USB_INPUT_PORT;

  // init usb device
  ret = bic_init_usb_dev(slot_id, comp, udev, SB_USB_PRODUCT_ID, SB_USB_VENDOR_ID);
  if (ret < 0) {
    goto error_exit;
  }

  gettimeofday(&start, NULL);

  // dump into file
  ret = bic_dump_fw_usb(slot_id, comp, path, udev);
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
