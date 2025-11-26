#!/usr/bin/env python3
# ------------------------------------------------------------------------------
# Copyright 2025 MaxLinear, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ------------------------------------------------------------------------------

import sys

import usb.core
import usb.util

# Only enable the lines below if the libusb backend cannot be correctly
# found and need specific path to libusb manually
# import usb.backend.libusb1
# LIB_PATH = "PATH TO libusb (i.e.:/lib/x86_64-linux-gnu/libusb-1.0.so.0)"
# backend = usb.backend.libusb1.get_backend(find_library=lambda x:LIB_PATH)
# if not backend:
#    print("Error: libusb backend cound not be loaded.")

VENDOR_ID = 0x04E2
PRODUCT_ID = 0x1424
TIMEOUT = 5000


def find_device():
    dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
    if dev is None:
        print(f"Cannot find device {VENDOR_ID:04x}:{PRODUCT_ID:04x}")
        sys.exit(1)
    return dev


def write_reg(dev, addr, value):
    try:
        dev.ctrl_transfer(
            bmRequestType=usb.util.build_request_type(
                usb.util.CTRL_OUT,
                usb.util.CTRL_TYPE_VENDOR,
                usb.util.CTRL_RECIPIENT_INTERFACE,
            ),
            bRequest=0,
            wValue=value,
            wIndex=addr,
            data_or_wLength=None,
            timeout=TIMEOUT,
        )
    except usb.core.USBError as e:
        print(f"Cannot write address {addr:03x} (error {e})")
        sys.exit(1)


def read_reg(dev, addr):
    try:
        data = dev.ctrl_transfer(
            bmRequestType=usb.util.build_request_type(
                usb.util.CTRL_IN,
                usb.util.CTRL_TYPE_VENDOR,
                usb.util.CTRL_RECIPIENT_INTERFACE,
            ),
            bRequest=0,
            wValue=0,
            wIndex=addr,
            data_or_wLength=2,
            timeout=TIMEOUT,
        )
        return (data[1] << 8) | data[0]
    except usb.core.USBError as e:
        print(f"Cannot read address {addr:03x} (error {e})")
        sys.exit(1)


def main():
    print(
        "This utility is intended for XR21B1424 GPIO mode overwrite on Linux platform"
    )
    dev = find_device()
    for i in range(4):
        reg_addr = 0x0C | ((i * 2) << 8)
        reg_rd_val = read_reg(dev, reg_addr)
        print(f"Pre-Write GPIO Reg[0x{reg_addr:02x}] = 0x{reg_rd_val:04x}")
        write_reg(dev, reg_addr, 0x30B)
        print(f"Channel {i} GPIO_MODE Reg Write Done")
        reg_rd_val = read_reg(dev, reg_addr)
        print(f"Post-Write GPIO Reg[0x{reg_addr:02x}] = 0x{reg_rd_val:04x}")


if __name__ == "__main__":
    main()
