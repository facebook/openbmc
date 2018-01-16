#!/usr/bin/env python
import sys
import re
import os.path
from subprocess import Popen, PIPE
from ctypes import *
from bios_ipmi_util import *


boot_order_device = { 0: "USB Device", 1: "IPv4 Network", 9: "IPv6 Network", 2: "SATA HDD", 3: "SATA-CDROM", 4: "Other Removable Device", 255: "Reserved" }

'''
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
'''
def boot_order(fru, argv):
    req_data = [""]
    option = argv[2]
    function = argv[3]
    boot_flags_valid = 1

    result = execute_IPMI_command(fru, 0x30, 0x53, "")

    data = [int(n, 16) for n in result]

    clear_CMOS = ((data[0] & 0x2) >> 1)
    force_boot_BIOS_setup = ((data[0] & 0x4) >> 2)
    boot_order = data[1:]

    if ( option == "get" ):
        if ( function == "--boot_order" ):
            try:
                print("Boot Order: " + ", ".join(boot_order_device[dev] for dev in boot_order))
            except KeyError:
                print("Invalid Boot Device ID!")
                print(boot_order_device)
        elif ( function == "--clear_CMOS" ):
            print("Clear CMOS Function: " + status_decode(clear_CMOS))
            exit(0)
        elif ( function == "--force_boot_BIOS_setup" ):
            print("Force Boot to BIOS Setup Function: " + status_decode(force_boot_BIOS_setup))
            exit(0)

    elif ( option == "enable" ):
        if ( function == "--clear_CMOS" ):
            clear_CMOS = trans2opcode(option)
        elif ( function == "--force_boot_BIOS_setup" ):
            force_boot_BIOS_setup = trans2opcode(option)

    elif ( option == "disable" ):
        if ( function == "--clear_CMOS" ):
            clear_CMOS = trans2opcode(option)
        elif ( function == "--force_boot_BIOS_setup" ):
            force_boot_BIOS_setup = trans2opcode(option)

        #Clear the 7th valid bit for disable clean CMOS, force boot to BIOS setup, and set boot order action
        boot_flags_valid = 0

    elif ( option == "set" ):
        if ( function == "--boot_order" ):
            set_boot_order = argv[4:]
            for num in set_boot_order:
                if ( not int(num) in boot_order_device ):
                    print("Invalid Boot Device ID!")
                    exit(-1)
            boot_order = set_boot_order

    if ( option != "get" ):
        req_data[0] = ((((data[0] & ~0x86) | (clear_CMOS << 1)) | (force_boot_BIOS_setup << 2)) | (boot_flags_valid << 7))
        req_data[1:] = boot_order
        execute_IPMI_command(fru, 0x30, 0x52, req_data)
