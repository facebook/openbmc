#!/usr/bin/env python
import os.path
import re
import sys
from ctypes import *
from subprocess import PIPE, Popen

from bios_ipmi_util import *


boot_order_device = {
    0: "USB Device",
    1: "IPv4 Network",
    9: "IPv6 Network",
    2: "SATA HDD",
    3: "SATA-CDROM",
    4: "Other Removable Device",
    255: "Reserved",
}

"""
OEM Set BIOS Boot Order (NetFn:0x30, CMD: 0x52h)
Request:
   Byte 1 - Boot mode
     Bit 0 - 0 : Legacy, 1 : UEFI
     Bit 1 - CMOS clear (Optional, BIOS implementation dependent)
     Bit 2 - Force Boot into BIOS Setup (Optional, BIOS implementation dependent)
     Bit 6:3 - reserved
     Bit 7 - boot flags valid
  Byte 2-6 - Boot sequence
     Bit 2:0 - boot device id
         000b: USB device
         001b: Network
         010b: SATA HDD
         011b: SATA-CDROM
         100b: Other removable Device
     Bit 7:3 - reserve for boot device special request
           If Bit 2:0 is 001b (Network), Bit3 is IPv4/IPv6 order
              Bit3=0b: IPv4 first
              Bit3=1b: IPv6 first
Response:
Byte1 - Completion Code
"""


def boot_order(fru, argv):
    req_data = [""]
    option = argv[2]
    function = argv[3]
    data = argv[4:]
    boot_flags_valid = 1

    result = execute_IPMI_command(fru, 0x30, 0x53, "")

    boot_order_data = [int(n, 16) for n in result]

    req_data = do_boot_order_action(option, function, data, boot_order_data)
    if option != "get":
        if option == "disable":
            # Clear the 7th valid bit for disable clean CMOS, force boot to BIOS setup, and set boot order action
            boot_flags_valid = 0
        send_req_data = get_boot_order_req_data(req_data, boot_flags_valid)
        execute_IPMI_command(fru, 0x30, 0x52, send_req_data)


def do_boot_order_action(option, function, data, boot_order_data):
    req_data = [""]

    boot_mode = boot_order_data[0] & 0x1
    clear_CMOS = (boot_order_data[0] & 0x2) >> 1
    force_boot_BIOS_setup = (boot_order_data[0] & 0x4) >> 2
    boot_order = boot_order_data[1:]

    if option == "get":
        if function == "--boot_order":
            try:
                print(
                    "Boot Order: "
                    + ", ".join(boot_order_device[dev] for dev in boot_order)
                )
            except KeyError:
                print("Invalid Boot Device ID!")
                print(boot_order_device)
        elif function == "--clear_CMOS":
            print("Clear CMOS Function: " + status_decode(clear_CMOS))
        elif function == "--force_boot_BIOS_setup":
            print(
                "Force Boot to BIOS Setup Function: "
                + status_decode(force_boot_BIOS_setup)
            )
        elif function == "--boot_mode":
            if boot_mode == 0x0:
                print("Boot Mode: Legacy")
            else:
                print("Boot Mode: UEFI")

    elif option == "enable":
        if function == "--clear_CMOS":
            clear_CMOS = trans2opcode(option)
        elif function == "--force_boot_BIOS_setup":
            force_boot_BIOS_setup = trans2opcode(option)

    elif option == "disable":
        if function == "--clear_CMOS":
            clear_CMOS = trans2opcode(option)
        elif function == "--force_boot_BIOS_setup":
            force_boot_BIOS_setup = trans2opcode(option)

        # Clear the 7th valid bit for disable clean CMOS, force boot to BIOS setup, and set boot order action
        boot_flags_valid = 0

    elif option == "set":
        if function == "--boot_order":
            set_boot_order = data
            for num in set_boot_order:
                if not int(num) in boot_order_device:
                    print("Invalid Boot Device ID!")
                    exit(-1)
            boot_order = set_boot_order
        elif function == "--boot_mode":
            boot_mode = int(data[0])

    if option != "get":
        req_data[0] = (
            (boot_order_data[0] & ~0x07) | (boot_mode) | (clear_CMOS << 1)
        ) | (force_boot_BIOS_setup << 2)
        req_data[1:] = boot_order
        return req_data


def get_boot_order_req_data(boot_order_data, boot_flags_valid):
    req_data = [""]

    req_data[0] = (boot_order_data[0] & ~0x80) | (boot_flags_valid << 7)
    req_data[1:] = boot_order_data[1:]
    return req_data
