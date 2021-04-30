/*
 * ftditool
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#include "ftdi-eeprom.h"

static int verbose = 0;

static void usage(int argc, char **argv)
{
    printf("\nUsage command\n"
           "%s [option] <command>\n"
           " option  : -h  display this usage\n"
           "           -v  enable verbose message\n"
           "           -V  define VID (default:0x0403)\n"
           "           -P  define PID (default 0x6001)\n"
           "           -a  define FTDI_EEPROM address\n"
           "           -d  define FTDI_EEPROM data for writing\n"
           " command : list   - list all usb device\n"
           "           dump   - display FTDI_EEPROM values\n"
           "                    need to be defind VID, PID\n"
           "           write  - write specific value to FTDI_EEPROM\n"
           "                    need to be define VID, PID, addr, data\n"
           "           useExtOsc - enable(1)/disable(0) external oscillator\n"
           "                       need to be define VID, PID, data\n"
           "           restore   - restore the FTDI_EEPROM\n"
           "                       need to be define VID, PID\n"
           " example :\n"
           "   %s list\n"
           "   %s dump\n"
           "   %s restore\n"
           "   %s useExtOsc -d 1\n"
           "   %s useExtOsc -d 0\n"
           "   %s write -a 0x00 -d 0x6001\n"
           "   %s write -a 0x01 -d 0x0403\n\n",
           argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
}

static void usb_print_endpoint_comp(const struct libusb_ss_endpoint_companion_descriptor *ep_comp)
{
    printf("      USB 3.0 Endpoint Companion:\n");
    printf("        bMaxBurst:           %u\n", ep_comp->bMaxBurst);
    printf("        bmAttributes:        %02xh\n", ep_comp->bmAttributes);
    printf("        wBytesPerInterval:   %u\n", ep_comp->wBytesPerInterval);
}

static void usb_print_endpoint(const struct libusb_endpoint_descriptor *endpoint)
{
    int i, ret;

    printf("      Endpoint:\n");
    printf("        bEndpointAddress:    %02xh\n", endpoint->bEndpointAddress);
    printf("        bmAttributes:        %02xh\n", endpoint->bmAttributes);
    printf("        wMaxPacketSize:      %u\n", endpoint->wMaxPacketSize);
    printf("        bInterval:           %u\n", endpoint->bInterval);
    printf("        bRefresh:            %u\n", endpoint->bRefresh);
    printf("        bSynchAddress:       %u\n", endpoint->bSynchAddress);

    for (i = 0; i < endpoint->extra_length;) {
        if (LIBUSB_DT_SS_ENDPOINT_COMPANION == endpoint->extra[i + 1]) {
            struct libusb_ss_endpoint_companion_descriptor *ep_comp;

            ret = libusb_get_ss_endpoint_companion_descriptor(NULL, endpoint, &ep_comp);
            if (LIBUSB_SUCCESS != ret)
                continue;

            usb_print_endpoint_comp(ep_comp);

            libusb_free_ss_endpoint_companion_descriptor(ep_comp);
        }

        i += endpoint->extra[i];
    }
}

static void usb_print_altsetting(const struct libusb_interface_descriptor *interface)
{
    uint8_t i;

    printf("    Interface:\n");
    printf("      bInterfaceNumber:      %u\n", interface->bInterfaceNumber);
    printf("      bAlternateSetting:     %u\n", interface->bAlternateSetting);
    printf("      bNumEndpoints:         %u\n", interface->bNumEndpoints);
    printf("      bInterfaceClass:       %u\n", interface->bInterfaceClass);
    printf("      bInterfaceSubClass:    %u\n", interface->bInterfaceSubClass);
    printf("      bInterfaceProtocol:    %u\n", interface->bInterfaceProtocol);
    printf("      iInterface:            %u\n", interface->iInterface);

    for (i = 0; i < interface->bNumEndpoints; i++)
        usb_print_endpoint(&interface->endpoint[i]);
}

static void usb_print_2_0_ext_cap(struct libusb_usb_2_0_extension_descriptor *usb_2_0_ext_cap)
{
    printf("    USB 2.0 Extension Capabilities:\n");
    printf("      bDevCapabilityType:    %u\n", usb_2_0_ext_cap->bDevCapabilityType);
    printf("      bmAttributes:          %08xh\n", usb_2_0_ext_cap->bmAttributes);
}

static void usb_print_ss_usb_cap(struct libusb_ss_usb_device_capability_descriptor *ss_usb_cap)
{
    printf("    USB 3.0 Capabilities:\n");
    printf("      bDevCapabilityType:    %u\n", ss_usb_cap->bDevCapabilityType);
    printf("      bmAttributes:          %02xh\n", ss_usb_cap->bmAttributes);
    printf("      wSpeedSupported:       %u\n", ss_usb_cap->wSpeedSupported);
    printf("      bFunctionalitySupport: %u\n", ss_usb_cap->bFunctionalitySupport);
    printf("      bU1devExitLat:         %u\n", ss_usb_cap->bU1DevExitLat);
    printf("      bU2devExitLat:         %u\n", ss_usb_cap->bU2DevExitLat);
}

static void usb_print_bos(libusb_device_handle *handle)
{
    struct libusb_bos_descriptor *bos;
    uint8_t i;
    int ret;

    ret = libusb_get_bos_descriptor(handle, &bos);
    if (ret < 0)
        return;

    printf("  Binary Object Store (BOS):\n");
    printf("    wTotalLength:            %u\n", bos->wTotalLength);
    printf("    bNumDeviceCaps:          %u\n", bos->bNumDeviceCaps);

    for (i = 0; i < bos->bNumDeviceCaps; i++) {
        struct libusb_bos_dev_capability_descriptor *dev_cap = bos->dev_capability[i];

        if (dev_cap->bDevCapabilityType == LIBUSB_BT_USB_2_0_EXTENSION) {
            struct libusb_usb_2_0_extension_descriptor *usb_2_0_extension;

            ret = libusb_get_usb_2_0_extension_descriptor(NULL, dev_cap, &usb_2_0_extension);
            if (ret < 0)
                return;

            usb_print_2_0_ext_cap(usb_2_0_extension);
            libusb_free_usb_2_0_extension_descriptor(usb_2_0_extension);
        } else if (dev_cap->bDevCapabilityType == LIBUSB_BT_SS_USB_DEVICE_CAPABILITY) {
            struct libusb_ss_usb_device_capability_descriptor *ss_dev_cap;

            ret = libusb_get_ss_usb_device_capability_descriptor(NULL, dev_cap, &ss_dev_cap);
            if (ret < 0)
                return;

            usb_print_ss_usb_cap(ss_dev_cap);
            libusb_free_ss_usb_device_capability_descriptor(ss_dev_cap);
        }
    }

    libusb_free_bos_descriptor(bos);
}

static void usb_print_interface(const struct libusb_interface *interface)
{
    int i;

    for (i = 0; i < interface->num_altsetting; i++)
        usb_print_altsetting(&interface->altsetting[i]);
}

static void usb_print_configuration(struct libusb_config_descriptor *config)
{
    uint8_t i;

    printf("  Configuration:\n");
    printf("    wTotalLength:            %u\n", config->wTotalLength);
    printf("    bNumInterfaces:          %u\n", config->bNumInterfaces);
    printf("    bConfigurationValue:     %u\n", config->bConfigurationValue);
    printf("    iConfiguration:          %u\n", config->iConfiguration);
    printf("    bmAttributes:            %02xh\n", config->bmAttributes);
    printf("    MaxPower:                %u\n", config->MaxPower);

    for (i = 0; i < config->bNumInterfaces; i++)
        usb_print_interface(&config->interface[i]);
}

static void usb_print_device(libusb_device *dev)
{
    struct libusb_device_descriptor desc;
    libusb_device_handle *handle = NULL;
    unsigned char string[256];
    int ret;
    uint8_t i;

    ret = libusb_get_device_descriptor(dev, &desc);
    if (ret < 0) {
        fprintf(stderr, "failed to get device descriptor");
        return;
    }

    printf("Dev (bus %u, device %u): %04X - %04X\n",
           libusb_get_bus_number(dev), libusb_get_device_address(dev),
           desc.idVendor, desc.idProduct);

    ret = libusb_open(dev, &handle);
    if (LIBUSB_SUCCESS == ret) {
        if (desc.iManufacturer) {
            ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, string, sizeof(string));
            if (ret > 0)
                printf("  Manufacturer:              %s\n", (char *)string);
        }

        if (desc.iProduct) {
            ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string, sizeof(string));
            if (ret > 0)
                printf("  Product:                   %s\n", (char *)string);
        }

        if (desc.iSerialNumber && verbose) {
            ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, string, sizeof(string));
            if (ret > 0)
                printf("  Serial Number:             %s\n", (char *)string);
        }
    }

    if (verbose) {
        for (i = 0; i < desc.bNumConfigurations; i++) {
            struct libusb_config_descriptor *config;

            ret = libusb_get_config_descriptor(dev, i, &config);
            if (LIBUSB_SUCCESS != ret) {
                printf("  Couldn't retrieve descriptors\n");
                continue;
            }

            usb_print_configuration(config);

            libusb_free_config_descriptor(config);
        }

        if (handle && desc.bcdUSB >= 0x0201)
            usb_print_bos(handle);
    }

    if (handle)
        libusb_close(handle);
}

static int usb_control_transfer(struct libusb_device_handle *devh,
                                uint8_t bmRequestType,
                                uint8_t bRequest,
                                uint16_t wValue,
                                uint16_t wIndex,
                                unsigned char *data,
                                uint16_t wLength,
                                unsigned int timeout)
{
    int ret = 0;

    if (verbose >= 2) {
        printf(" bmReqType : 0x%02X\n", bmRequestType);
        printf(" bRequest  : 0x%02X (%d)\n", bRequest, bRequest);
        printf(" wValue    : 0x%04X\n", wValue);
        printf(" wIndex    : 0x%04X (%d)\n", wIndex, wIndex);
        printf(" wLength   : 0x%04X (%d)\n", wLength, wLength);
    }

    ret = libusb_control_transfer(devh,
                                 bmRequestType, //bmRequestType
                                 bRequest, //bRequest
                                 wValue, //wValue
                                 wIndex, //wIndex
                                 data, //data
                                 wLength, //wLength
                                 timeout //timeout
          );

    if (verbose >= 2) {
        printf(" return     : %d\n", ret);
        if (wLength > 0) {
            printf(" Data      : ");
            for(int i = 0; i < wLength; i++)
                printf("%02X ", data[i]);
            printf("\n");
        }
        printf("\n");
    }

    return ret;
}

static int ftdi_eeprom_checksum(union FTDI_EEPROM eeprom)
{
    int length = 0x7D;
    uint16_t checksum = 0xAAAA;
    for(int i = 0; i < length; i += 2) {
        uint16_t tmpchksum = ((eeprom.data[i + 1] << 8) | eeprom.data[i]) ^ checksum;
        checksum = ((tmpchksum & 0x7FFF) << 1) | ((tmpchksum >> 15) & 1);
        if (verbose >= 2)
            printf("[0x%02X]  %02X%02X   %04X  %04X\n", i, eeprom.data[i + 1],
                   eeprom.data[i], tmpchksum, checksum);
    }
    return checksum;

}

static void ftdi_eeprom_dump(union FTDI_EEPROM eeprom)
{
    for(int index = 0; index < 255; index += 2) {
        printf("%04X ", *(uint16_t*)&eeprom.data[index]);
        if (index % 16 == 14) {
            printf("\n");
        }
    }
}

static void ftdi_eeprom_print(union FTDI_EEPROM eeprom)
{

    printf("Header                  : %d\n", eeprom.info.header);
    printf("LoadDriver              : %d\n", eeprom.info.loadDriver);
    printf("HighDriveIO             : %d\n", eeprom.info.highDriveIO);

    printf("EndpointSize            : %d \n", eeprom.info.endpoint_size);
    printf("UseExternalOscillator   : %d \n", eeprom.info.useExtOsc);

    printf("VendorID                : %04X\n", eeprom.info.VendorID);
    printf("ProductID               : %04X\n", eeprom.info.ProductID);
    printf("ReleaseNumber           : %04X\n", eeprom.info.release_number);
    printf("ConfigDes               : %d\n", eeprom.info.config_des);
    printf("MaxPower                : %d\n", eeprom.info.max_power);
    printf("Invert_RI               : %d\n", eeprom.info.Invert_RI);
    printf("Invert_DCD              : %d\n", eeprom.info.Invert_DCD);
    printf("Invert_DSR              : %d\n", eeprom.info.Invert_DSR);
    printf("Invert_DTR              : %d\n", eeprom.info.Invert_DTR);
    printf("Invert_CTS              : %d\n", eeprom.info.Invert_CTS);
    printf("Invert_RTS              : %d\n", eeprom.info.Invert_RTS);
    printf("Invert_RXD              : %d\n", eeprom.info.Invert_RXD);
    printf("Invert_TXD              : %d\n", eeprom.info.Invert_TXD);
    printf("DoSerialNumber          : %d\n", eeprom.info.do_serialnumber);
    printf("PullDown_En             : %d\n", eeprom.info.pulldown_en);
    printf("USB_Version             : %04X\n", eeprom.info.usb_version);

    printf("ManString_ptr           : %02X\n", eeprom.info.ManString_ptr & 0x7F);
    printf("ManString_len           : %d\n", eeprom.info.ManString_len);
    printf("                     -- : ");
    for(int index = 0; index < eeprom.info.ManString_len; index += 2) {
        printf("%c", eeprom.data[(eeprom.info.ManString_ptr & 0x7F) + 2 + index]);
    }
    printf("\n");

    printf("PrdString_ptr           : %02X\n", eeprom.info.PrdString_ptr & 0x7F);
    printf("PrdString_len           : %d\n", eeprom.info.PrdString_len);
    printf("                     -- : ");
    for(int index = 0; index < eeprom.info.PrdString_len; index += 2) {
        printf("%c", eeprom.data[(eeprom.info.PrdString_ptr & 0x7F) + 2 + index]);
    }
    printf("\n");

    printf("SerString_ptr           : %02X\n" , eeprom.info.SerString_ptr & 0x7F);
    printf("SerString_len           : %d\n" , eeprom.info.SerString_len);
    printf("                     -- : ");
    for(int index = 0; index < eeprom.info.SerString_len; index += 2) {
        printf("%c", eeprom.data[(eeprom.info.SerString_ptr & 0x7F) + 2 + index]);
    }
    printf("\n");

    printf("Cbus0                   : %02X\n", eeprom.info.cbus0);
    printf("Cbus1                   : %02X\n", eeprom.info.cbus1);
    printf("Cbus2                   : %02X\n", eeprom.info.cbus2);
    printf("Cbus3                   : %02X\n", eeprom.info.cbus3);
    printf("Cbus4                   : %02X\n", eeprom.info.cbus4);

    printf("Checksum(0x7E)          : %04X\n", eeprom.info.checksum);
}

int main(int argc, char *argv[])
{
    struct libusb_device_handle *devh = NULL;
    libusb_device **devs;
    ssize_t cnt;
    int opt;
    int ret, i;
    int pid = 0x6001, vid = 0x0403;
    int reg_addr = 0, reg_data = 0;
    char reg_addr_set = 0, reg_data_set = 0;
    char vid_set = 0, pid_set = 0;
    unsigned char datas[32];
    union FTDI_EEPROM eeprom = {0};

    while((opt = getopt(argc, argv, "hvV:P:a:d:")) != -1) {
        switch(opt)
        {
        case 'h':
            usage(argc, argv);
            return 0;
        case 'v':
            verbose++;
            break;
        case 'P':
            pid_set = 1;
            pid = strtol(optarg, NULL, 0);
            break;
        case 'V':
            vid_set = 1;
            vid = strtol(optarg, NULL, 0);
            break;
        case 'a':
            reg_addr_set = 1;
            reg_addr = strtol(optarg, NULL, 0);
            break;
        case 'd':
            reg_data_set = 1;
            reg_data = strtol(optarg, NULL ,0);
            break;
        default:
            usage(argc, argv);
            return -1;
        }
    }

    if (optind >= argc) {
        usage(argc, argv);
        fprintf(stderr, "Error: No command\n");
        return -1;
    }

    if (!strcmp(argv[optind], "list")) {
        ret = libusb_init(NULL);
        if (ret < 0)
            return ret;
        cnt = libusb_get_device_list(NULL, &devs);
        if (cnt < 0)
            return (int)cnt;
        for (i = 0; devs[i]; i++) {
            //filter for FTDI PID VID
            struct libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(devs[i], &desc) < 0) {
                fprintf(stderr, "failed to get device descriptor\n");
                continue;
            }
            usb_print_device(devs[i]);
        }
        libusb_free_device_list(devs, 1);

        libusb_exit(NULL);
    } else if (!strcmp(argv[optind], "dump")) {
        if (!vid_set) {
            printf("VID use default value : 0x%04X\n", vid);
        }
        if (!pid_set) {
            printf("PID use default value : 0x%04X\n", pid);
        }

        ret = libusb_init(NULL);
        if (ret < 0)
            return ret;

        devh = libusb_open_device_with_vid_pid(NULL, vid, pid);
        if (!devh) {
            fprintf(stderr, "failed to open VID:%04X PID:%04X\n", vid, pid);
            return -1;
        }
        for(int index = 0; index <= 127; index++) {
            ret = libusb_control_transfer(devh,
                                         0xc0, //bmRequestType
                                         0x90, //bRequest
                                         0, //wValue
                                         index, //wIndex
                                         datas, //data
                                         2, //wLength
                                         100 //timeout
                 );
            if (ret < 0) {
                 fprintf(stderr," getting data error %d\n",ret);
                 break;
            }
            eeprom.data[index * 2] = datas[0];
            eeprom.data[index * 2 + 1] = datas[1];
        }
        ftdi_eeprom_dump(eeprom);
        ftdi_eeprom_print(eeprom);
        printf("checksum %04X\n",ftdi_eeprom_checksum(eeprom));

        libusb_exit(NULL);
    } else if (!strcmp(argv[optind], "write")) {
        if (!vid_set) {
            printf("VID use default value : 0x%04X\n", vid);
        }
        if (!pid_set) {
            printf("PID use default value : 0x%04X\n", pid);
        }

        if (!reg_addr_set || !reg_data_set) {
            usage(argc,argv);
            fprintf(stderr,"need define addr (-a), \n"
                           "            data (-d) \n");
            return -1;
        }

        ret = libusb_init(NULL);
        if (ret < 0)
            return ret;

        devh = libusb_open_device_with_vid_pid(NULL, vid, pid);
        if (!devh) {
            fprintf(stderr, "failed to open VID:%04X PID:%04X\n", vid, pid);
            return -1;
        }

        for(int index = 0; index <= 127; index++) {
            ret = libusb_control_transfer(devh,
                                          0xc0, //bmRequestType
                                          0x90, //bRequest
                                          0, //wValue
                                          index, //wIndex
                                          datas, //data
                                          2, //wLength
                                          100 //timeout
                 );
            if (ret < 0) {
                 fprintf(stderr, " getting data error %d\n", ret);
                 break;
            }
            eeprom.data[index * 2] = datas[0];
            eeprom.data[index * 2 + 1] = datas[1];
        }
        if (verbose) {
            ftdi_eeprom_dump(eeprom);
        }
        uint16_t *reg_ptr = (uint16_t*)&eeprom.data[reg_addr];
        printf("[current data] data[0x%02X] = 0x%04X\n", reg_addr, *reg_ptr);
        if (*reg_ptr != reg_data) {
            *reg_ptr = reg_data;
            printf("[new data] data[0x%02X] = 0x%04X\n", reg_addr, *reg_ptr);
            eeprom.info.checksum = ftdi_eeprom_checksum(eeprom);
            if (verbose) {
                ftdi_eeprom_dump(eeprom);
            }

            //Erase FTDI_EEPROM
            usb_control_transfer(devh, 0x40, 0x92, 0, 0x00, 0x00, 0, 100);
            for(int index = 0; index <= 127; index++) {
                reg_data = (eeprom.data[index * 2 + 1] << 8) | eeprom.data[index * 2];
                usb_control_transfer(devh, 0x40, 0x91, reg_data,index, NULL, 0, 100);
            }
        }else{
            printf(" data not changed, ignore write process\n");
        }
        libusb_exit(NULL);

    } else if (!strcmp(argv[optind], "useExtOsc")) {
        if (!reg_data_set) {
            usage(argc, argv);
            fprintf(stderr, "need define data (-d) \n"
                            "             -d 1   enable external oscillator\n"
                            "             -d 0   disable external oscillator\n");
            return -1;
        }

        ret = libusb_init(NULL);
        if (ret < 0)
            return ret;

        devh = libusb_open_device_with_vid_pid(NULL, vid, pid);
        if (!devh) {
            fprintf(stderr, "failed to open VID:%04X PID:%04X\n", vid, pid);
            return -1;
        }

        for(int index = 0; index <= 127; index++) {
            ret = libusb_control_transfer(devh,
                                          0xc0, //bmRequestType
                                          0x90, //bRequest
                                          0, //wValue
                                          index, //wIndex
                                          datas, //data
                                          2, //wLength
                                          100 //timeout
                 );
            if (ret < 0) {
                 fprintf(stderr, " getting data error %d\n", ret);
                 break;
            }
            eeprom.data[index * 2] = datas[0];
            eeprom.data[index * 2 + 1] = datas[1];
        }
        if (verbose) {
            ftdi_eeprom_dump(eeprom);
        }

        printf("[current config] useExtOsc = %d\n", eeprom.info.useExtOsc);
        if (eeprom.info.useExtOsc != reg_data) {
            eeprom.info.useExtOsc = 1 - eeprom.info.useExtOsc;
            printf("[new config] useExtOsc = %d\n", eeprom.info.useExtOsc);
            eeprom.info.checksum = ftdi_eeprom_checksum(eeprom);
            if (verbose) {
                ftdi_eeprom_dump(eeprom);
            }

            //Erase FTDI_EEPROM
            usb_control_transfer(devh, 0x40, 0x92, 0, 0x00, 0x00, 0, 100);
            for(int index = 0; index <= 127; index++) {
                reg_data = (eeprom.data[index * 2 + 1] << 8) | eeprom.data[index * 2];
                usb_control_transfer(devh, 0x40, 0x91, reg_data, index, NULL, 0, 100);
            }
        }else{
            printf(" data not changed, ignore write process\n");
        }

        libusb_exit(NULL);
    } else if (!strcmp(argv[optind], "restore")) {
        ret = libusb_init(NULL);
        if (ret < 0)
            return ret;

        devh = libusb_open_device_with_vid_pid(NULL, vid, pid);
        if (!devh) {
            fprintf(stderr, "failed to open VID:%04X PID:%04X\n", vid, pid);
            return -1;
        }

        uint16_t original[] = {
            0x4000, 0x0403, 0x6001, 0x0000, 0x2DA0, 0x0008, 0x0000, 0x0A98,
            0x20A2, 0x12C2, 0x1023, 0x0005, 0x030A, 0x0046, 0x0054, 0x0044,
            0x0049, 0x0320, 0x0046, 0x0054, 0x0032, 0x0033, 0x0032, 0x0052,
            0x0020, 0x0055, 0x0053, 0x0042, 0x0020, 0x0055, 0x0041, 0x0052,
            0x0054, 0x0312, 0x0041, 0x0043, 0x0030, 0x0031, 0x0036, 0x0032,
            0x0036, 0x0030, 0xEBD9, 0xC0B3, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x51B6,
            0x042C, 0xFBD3, 0x0000, 0xEBD9, 0xC0B3, 0x0042, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x4143, 0x5258, 0x5558, 0x4A41,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
        };
        memcpy(eeprom.data, original, FTDI_EEPROM_SIZE);
        if (verbose) {
            ftdi_eeprom_dump(eeprom);
            ftdi_eeprom_print(eeprom);
        }

        //0x40,0,0x0000,0x0,NULL,0
        usb_control_transfer(devh, 0x40, 0, 0x0000, 0, NULL, 0, 100);
        //0xC0,5,0x0000,0x0,datas,2 << 0160
        usb_control_transfer(devh, 0xC0, 5, 0x0000, 0, datas, 2, 100);
        //0x40,4,0x0008,0x0,NULL,0
        usb_control_transfer(devh, 0xC0, 4, 0x0008, 0, NULL, 0, 100);
        //0x40,2,0x0000,0x0,NULL,0
        usb_control_transfer(devh, 0x40, 2, 0x0000, 0, NULL, 0, 100);
        //0x40,3,0x4138,0x0,NULL,0
        usb_control_transfer(devh, 0x40, 3, 0x0000, 0, NULL, 0, 100);
        //0xC0,10,0x0000,0x0,datas,1
        usb_control_transfer(devh, 0xC0, 10, 0x0000, 0, datas, 1, 100);
        //0x40,9,0x0077,0x0,NULL,0
        usb_control_transfer(devh, 0x40, 9, 0x0077, 0, NULL, 0, 100);

        usb_control_transfer(devh, 0x40, 0x92, 0, 0x00, NULL, 0, 100);
        for(int index = 0; index <= 127; index++) {
            reg_data = (eeprom.data[index * 2 + 1] << 8) | eeprom.data[index * 2];
            libusb_control_transfer(devh, 0x40, 0x91, reg_data, index, NULL, 0, 100);
        }

        libusb_exit(NULL);
    } else {
        usage(argc, argv);
        fprintf(stderr, "Error: command invalid.\n");
        return -1;
    }

    return 0;
}