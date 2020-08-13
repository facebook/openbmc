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
#include <openbmc/kv.h>
#include <libusb-1.0/libusb.h>
#include "bic_fwupdate.h"
#include "bic_bios_fwupdate.h"

#define TI_VENDOR_ID  0x1CBE
#define TI_PRODUCT_ID 0x0007

#define USB_PKT_SIZE 0x200
#define USB_DAT_SIZE (USB_PKT_SIZE-7)
#define BIOS_PKT_SIZE (64 * 1024)
#define SIZE_IANA_ID 3
#define BIOS_ERASE_PKT_SIZE (64*1024)
#define BIOS_VERIFY_PKT_SIZE (32*1024)
#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define MAX_CHECK_DEVICE_TIME 8

int interface_ref = 0;
int alt_interface,interface_number;

typedef struct 
{
  struct libusb_device**          devs;
  struct libusb_device*           dev;
  struct libusb_device_handle*    handle;
  struct libusb_device_descriptor desc;
  char    manufacturer[64];
  char    product[64];
  int     config;
  int     ci;
  uint8_t epaddr;
  uint8_t path[8];
} usb_dev;

typedef struct {
  uint8_t dummy;
  uint32_t offset;
  uint16_t length;
  uint8_t data[USB_DAT_SIZE];
} __attribute__((packed)) bic_usb_packet;

static int
_set_fw_update_ongoing(uint8_t slot_id, uint16_t tmout) {
  char key[64];
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "fru%u_fwupd", slot_id);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

static int
verify_bios_image(uint8_t slot_id, int fd, long size) {
  int ret = -1;
  int rc, i;
  uint32_t offset;
  uint32_t tcksum, gcksum;
  volatile uint16_t count;
  uint8_t target, last_pkt = 0x00;
  uint8_t *tbuf = NULL;

  // Checksum calculation for BIOS image
  tbuf = malloc(BIOS_VERIFY_PKT_SIZE * sizeof(uint8_t));
  if (!tbuf) {
    return -1;
  }

  if ((offset = lseek(fd, 0, SEEK_SET))) {
    syslog(LOG_ERR, "%s: fail to init file offset %d, errno=%d", __func__, offset, errno);
    return -1;
  }
  while (1) {
    count = read(fd, tbuf, BIOS_VERIFY_PKT_SIZE);
    if (count <= 0) {
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
    rc = bic_get_fw_cksum(slot_id, target, offset, count, (uint8_t*)&gcksum);
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

static int
_send_bic_usb_packet(usb_dev* udev, bic_usb_packet *pkt)
{
  const int transferlen = pkt->length + 7;
  int transferred = 0;
  int retries = 3;
  int ret;

  // _debug_bic_usb_packet(pkt);
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t*)pkt, transferlen, &transferred, 3000);
    if(((ret != 0) || (transferlen != transferred))) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n",ret ,transferred);
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
bic_init_usb_dev(uint8_t slot_id, usb_dev* udev)
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

      if( (TI_VENDOR_ID == udev->desc.idVendor) && (TI_PRODUCT_ID == udev->desc.idProduct) ) {
        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*) udev->manufacturer, sizeof(udev->manufacturer));
        if ( ret < 0 ) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        ret = fby3_common_get_bmc_location(&bmc_location);
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

        if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
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

int
bic_update_fw_usb(uint8_t slot_id, uint8_t comp, const char *image_file, usb_dev* udev)
{
  struct stat st;
  uint32_t chunk_sz = BIOS_PKT_SIZE;  // bic usb write block size
  uint32_t file_sz = 0;
  uint32_t actual_send_sz = 0;
  uint32_t align_pkt_cnt = 0;
  uint32_t dsize, offset, last_offset, shift_offset;
  uint16_t count, read_count;
  uint8_t buf[USB_PKT_SIZE] = {0};
  bic_usb_packet *pkt = (bic_usb_packet *) buf;
  int i, fd = -1;
  char update_target[64] = {0};
  uint32_t usb_pkt_cnt = 0;
  int ret = -1;

  if (comp == FW_BIOS) {
    shift_offset = 0;
    strcpy(update_target, "bios\0");
  } else if ( (comp == FW_BIOS_CAPSULE) || (comp == FW_BIOS_RCVY_CAPSULE) ){
    shift_offset = BIOS_CAPSULE_OFFSET;
    strcpy(update_target, "bios capsule to PCH\0");
  } else if ( (comp == FW_CPLD_CAPSULE) || (comp == FW_CPLD_RCVY_CAPSULE) ) {
    shift_offset = CPLD_CAPSULE_OFFSET;
    strcpy(update_target, "cpld capsule to PCH\0");
  } else {
    printf("ERROR: not supported component [comp:%u]!\n", comp);
    goto error_exit;
  }

  fd = open(image_file, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", image_file);
    goto error_exit;
  }
  fstat(fd, &st);
  file_sz = st.st_size;  
  align_pkt_cnt = (st.st_size/chunk_sz);
  if (st.st_size % chunk_sz)
    align_pkt_cnt += 1;
  actual_send_sz = align_pkt_cnt * chunk_sz;
  dsize = actual_send_sz / 20;

  // Write chunks of binary data in a loop
  offset = 0;
  last_offset = 0;
  i = 1;
  while (1) {
    memset(buf, 0xFF, sizeof(buf));

    // For bic usb, send packets in blocks of 64K
    if ((offset + USB_DAT_SIZE) > (i * BIOS_PKT_SIZE)) {
      read_count = (i * BIOS_PKT_SIZE) - offset;
      i++;
    } else {
      read_count = USB_DAT_SIZE; 
    }

    // Read from file
    if (offset < file_sz) {
      count = read(fd, &buf[7], read_count);
      if (count < 0) {
        syslog(errno, "failed to read %s", image_file);
        goto error_exit;
      }
    } else if (offset < actual_send_sz) { /* padding */
      count = read_count;
    } else {
      break;
    }

    pkt->offset = (offset + shift_offset);
    pkt->length = count;

    if (_send_bic_usb_packet(udev, pkt))
      goto error_exit;

    usb_pkt_cnt += 1;

    offset += count;
    if ( (last_offset + dsize) <= offset ) {
      _set_fw_update_ongoing(slot_id, 60);
      printf("updated %s: %d %%\n", update_target, (offset/dsize)*5);
      last_offset += dsize;
    }
  }

  if (comp != FW_BIOS_CAPSULE && comp != FW_CPLD_CAPSULE && comp != FW_BIOS_RCVY_CAPSULE && comp != FW_CPLD_RCVY_CAPSULE) {
    _set_fw_update_ongoing(slot_id, 60 * 2);
    if (verify_bios_image(slot_id, fd, file_sz)) {
      goto error_exit;
    }
  }

  ret = 0;
error_exit:
  if (fd > 0)
    close(fd);
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
update_bic_usb_bios(uint8_t slot_id, uint8_t comp, char *image)
{
  struct timeval start, end;
  char key[64];
  int ret = -1;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  ret = bic_init_usb_dev(slot_id, udev);
  if (ret < 0) {
    goto error_exit;
  }

  printf("Input: %s, USB timeout: 3000ms\n", image);
  gettimeofday(&start, NULL);

  // sending file
  ret = bic_update_fw_usb(slot_id, comp, image, udev);
  if (ret < 0)
    goto error_exit;

  gettimeofday(&end, NULL);
  if (comp == FW_BIOS) {
    printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  }

  ret = 0;
error_exit:
  sprintf(key, "fru%u_fwupd", slot_id);
  remove(key);

  // close usb device
  bic_close_usb_dev(udev);
  return ret;
}
