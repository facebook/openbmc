#ifndef __BIC_BIOS_FWUPDATE_H__
#define __BIC_BIOS_FWUPDATE_H__

#include "bios-update.hpp"

#include <libusb-1.0/libusb.h>

#include <string>

typedef struct
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
} usb_dev;

typedef struct
{
    uint8_t netfn;
    uint8_t cmd;
    uint8_t iana[IANA_ID_SIZE];
    uint8_t target;
    uint32_t offset;
    uint16_t length;
    // uint8_t data[];
} __attribute__((packed)) bic_usb_packet;
#define USB_PKT_HDR_SIZE (sizeof(bic_usb_packet))

typedef struct
{
    uint8_t netfn;
    uint8_t cmd;
    uint8_t cc;
} __attribute__((packed)) bic_usb_res_packet;
#define USB_PKT_RES_HDR_SIZE (sizeof(bic_usb_res_packet))

int update_bic_usb_bios(uint8_t slot_id, std::string const& imageFilePath);

#endif /* __BIC_BIOS_FWUPDATE_H__ */