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
#define BIOS_VER_STR "F09_"
#define MAX_CHECK_DEVICE_TIME 5

int interface_ref = 0;
int alt_interface,interface_number;

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

int
update_bic_usb_bios(uint8_t slot_id, char *image)
{
  struct timeval start, end;
  struct libusb_device **devs;
  struct libusb_device_handle *handle = NULL, *hDevice_expected = NULL;
  struct libusb_device *dev,*dev_expected;
  struct libusb_device_descriptor desc;
  int config2;
  int i = 0;
  char str1[64], str2[64];
  char key[64];
  char found = 0;
  ssize_t cnt;
  int retries = 3, ret = -1;
  struct stat st;
  uint32_t fsize = 0;
  uint32_t record_offset = 0;
  uint32_t offset = 0;
  uint8_t data[USB_PKT_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int read_cnt;
  int fd;
  char fpath[128];
  uint8_t path[8];
  int recheck = MAX_CHECK_DEVICE_TIME;
  
  ret = libusb_init(NULL);
  if (ret < 0) {
    printf("Failed to initialise libusb\n");
    goto error_exit;
  } else {
    printf("Init libusb Successful!\n");
  }

  do {
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0) {
      printf("There are no USB devices on bus\n");
      goto error_exit;
    }
    while ((dev = devs[i++]) != NULL) {
      ret = libusb_get_device_descriptor(dev, &desc);
      if ( ret < 0 ) {
        printf("Failed to get device descriptor -- exit\n");
        libusb_free_device_list(devs,1);
        goto error_exit;
      }

      ret = libusb_open(dev,&handle);
      if ( ret < 0 ) {
        printf("Error opening device -- exit\n");
        libusb_free_device_list(devs,1);
        goto error_exit;
      }

      if( (TI_VENDOR_ID == desc.idVendor) && (TI_PRODUCT_ID == desc.idProduct) ) {
        ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*) str1, sizeof(str1));
        if ( ret < 0 ) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(devs,1);
          goto error_exit;
        }

        ret = libusb_get_port_numbers(dev, path, sizeof(path));
        if (ret < 0) {
          printf("Error get port number\n");
          libusb_free_device_list(devs,1);
          goto error_exit;
        }

        if ( path[1] != slot_id) {
          continue;
        }
        printf("%04x:%04x (bus %d, device %d)",desc.idVendor, desc.idProduct, libusb_get_bus_number(dev), libusb_get_device_address(dev));
        printf(" path: %d", path[0]);
        for (i = 1; i < ret; i++) {
          printf(".%d", path[i]);
        }
        printf("\n");

        ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*) str2, sizeof(str2));
        if ( ret < 0 ) {
          printf("Error get Product string descriptor -- exit\n");
          libusb_free_device_list(devs,1);
          goto error_exit;
        }

        printf("Manufactured : %s\n",str1);
        printf("Product : %s\n",str2);
        printf("----------------------------------------\n");
        printf("Device Descriptors:\n");
        printf("Vendor ID : %x\n",desc.idVendor);
        printf("Product ID : %x\n",desc.idProduct);
        printf("Serial Number : %x\n",desc.iSerialNumber);
        printf("Size of Device Descriptor : %d\n",desc.bLength);
        printf("Type of Descriptor : %d\n",desc.bDescriptorType);
        printf("USB Specification Release Number : %d\n",desc.bcdUSB);
        printf("Device Release Number : %d\n",desc.bcdDevice);
        printf("Device Class : %d\n",desc.bDeviceClass);
        printf("Device Sub-Class : %d\n",desc.bDeviceSubClass);
        printf("Device Protocol : %d\n",desc.bDeviceProtocol);
        printf("Max. Packet Size : %d\n",desc.bMaxPacketSize0);
        printf("No. of Configuraions : %d\n",desc.bNumConfigurations);

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
    libusb_free_device_list(devs,1);
    goto error_exit;
  }

  dev_expected = dev;
  hDevice_expected = handle;

  ret = libusb_get_configuration(handle,&config2);
  if ( ret != 0 ) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(devs,1);
    goto error_exit;
  }

  printf("Configured value : %d\n",config2);
  if ( config2 != 1 ) {
    libusb_set_configuration(handle, 1);
    if ( ret != 0 ) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(devs,1);
      goto error_exit;
    }
    printf("Device is in configured state!\n");
  }

  libusb_free_device_list(devs, 1);

  int ci = 1;
  uint8_t epaddr = 0x1; 
  if(libusb_kernel_driver_active(handle, ci) == 1) {
    printf("Kernel Driver Active\n");
    if(libusb_detach_kernel_driver(handle, ci) == 0) {
      printf("Kernel Driver Detached!");
    } else {
      printf("Couldn't detach kernel driver -- exit\n");
      libusb_free_device_list(devs,1);
      goto error_exit;
    }
  }

  ret = libusb_claim_interface(handle, ci);
  if ( ret < 0 ) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(devs,1);
    goto error_exit;
  }
  printf("Claimed Interface: %d, EP addr: 0x%02X\n", ci, epaddr);

  active_config(dev_expected,hDevice_expected);

  strcpy(fpath, image);
  printf("Input: %s, USB timeout: 3000ms\n", fpath);
  fd = open(fpath, O_RDONLY, 0666);
  if (fd < 0) {
    printf("ERROR: invalid file path!\n");
    syslog(LOG_ERR, "bic_update_fw: open fails for path: %s\n", image);
    goto error_exit;
  }
  stat(fpath, &st);
  fsize = st.st_size / 20;

  int transferlen = 0;
  int transferred = 0;
  int count = 1;
  int per_size = 0;
  gettimeofday(&start, NULL);
  while (1) {
    memset(data, 0, sizeof(data));
    
    //check size
    if ( (offset + USB_DAT_SIZE) > (count * BIOS_PKT_SIZE) ) {
      per_size = (count * BIOS_PKT_SIZE) - offset;
      transferlen = per_size + 7;
      count++;
    } else {
      per_size = USB_DAT_SIZE;
      transferlen = USB_PKT_SIZE;
    }

    read_cnt = read(fd, &data[7], per_size);

    if ( read_cnt <= 0 ) break;

    data[1] = (offset) & 0xFF;
    data[2] = (offset >> 8) & 0xFF;
    data[3] = (offset >> 16) & 0xFF;
    data[4] = (offset >> 24) & 0xFF;
    data[5] = read_cnt & 0xFF;
    data[6] = (read_cnt >> 8) & 0xFF;

resend:
    ret = libusb_bulk_transfer(handle, epaddr, data, transferlen, &transferred, 3000);
    if(((ret != 0) || (transferlen != transferred))) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n",ret ,transferred);
      printf("Retry since  %s\n", libusb_error_name(ret));
      retries--;
      if (!retries) {
        ret = -1;
        break;
      }
      msleep(100);
      goto resend;
    }

    offset += read_cnt;
    if ( (record_offset + fsize) <= offset ) {
      _set_fw_update_ongoing(slot_id, 60);
      printf("updated bios: %d %%\n", (offset/fsize)*5);
      record_offset += fsize;
    }
  }
  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  ret = libusb_release_interface(handle, ci);
  if ( ret < 0 ) {
    printf("Couldn't release the interface 0x%X\n", ci);
  }

  _set_fw_update_ongoing(slot_id, 60 * 2);
  if (verify_bios_image(slot_id, fd, st.st_size)) {
    ret = -1;
    goto error_exit;
  }

  if ( fd > 0 ) {
    close(fd);
  }

  ret = 0;
error_exit:
  sprintf(key, "fru%u_fwupd", slot_id);
  remove(key);

  if ( handle != NULL )
    libusb_close(handle);
  libusb_exit(NULL);

  return ret;
}
