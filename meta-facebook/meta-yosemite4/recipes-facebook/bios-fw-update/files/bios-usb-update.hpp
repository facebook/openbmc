#pragma once

#include "bios-update.hpp"

#include <libusb-1.0/libusb.h>

struct usb_dev
{
    struct libusb_device** devs;
    struct libusb_device* dev;
    struct libusb_device_handle* handle;
    struct libusb_device_descriptor desc;
    char manufacturer[64];
    char product[64];
    int config;
    int ci;
    uint8_t epaddr;
    uint8_t path[8];
};

struct bic_usb_packet
{
    uint8_t netfn;
    uint8_t cmd;
    uint8_t iana[IANA_ID_SIZE];
    uint8_t target;
    uint32_t offset;
    uint16_t length;
    // uint8_t data[];
} __attribute__((packed));
#define USB_PKT_HDR_SIZE (sizeof(bic_usb_packet))

struct bic_usb_res_packet
{
    uint8_t netfn;
    uint8_t cmd;
    uint8_t cc;
} __attribute__((packed));
#define USB_PKT_RES_HDR_SIZE (sizeof(bic_usb_res_packet))

struct bic_usb_bios_erase_res_packet
{
    uint8_t netfn;
    uint8_t cmd;
    uint8_t cc;
    uint8_t data[1];
} __attribute__((packed));
#define USB_PKT_RES_BIOS_ERASE_HDR_SIZE (sizeof(bic_usb_bios_erase_res_packet))

int update_bic_usb_bios(uint8_t slot_id, const std::string& imageFilePath,
                        const std::string& cpuType, bool eraseOnly = false);

int get_bios_erase_progress(usb_dev* udev);