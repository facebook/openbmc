#!/usr/bin/env python
import sys
import os.path
from subprocess import Popen, PIPE
from bios_ipmi_util import *
from time import sleep

def get_server_type(fru):
    try:
        f = open("/tmp/server_type" + repr(fru) + ".bin", 'r')
        retry = 3
        while retry != 0:
            value = f.read()
            if value:
                server_type = int(value.split()[0], 10)
                break
            retry = retry - 1
            sleep(0.01)
            continue
        f.close()
    except Exception:
        server_type = 3
    return server_type

'''
OEM Get Platform Info (NetFn:0x30, CMD: 0x7Eh)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - Node Slot Index
     Bit 7 - 1b: present, 0b: not present
     Bit 6 - 1b: Test Board, 0b: Non Test Board
     Bit 5:3  - SKU ID
         000b: Yosemite
         010b: Triton-Type 5A (Left sub-system)
         011b: Triton-Type 5B (Right sub-system)
         100b: Triton-Type 7 SS (IOC based IOM)
     Bit 2:0 - Slot Index, 1 based
'''
def plat_info(fru):
    presense = "Not Present"
    test_board = "Non Test Board"
    SKU = "Unknown"
    slot_index = ""
    result = execute_IPMI_command(fru, 0x30, 0x7E, "")

    data = int(result[0], 16)

    if ( data & 0x80 ):
        presense = "Present"

    if ( data & 0x40 ):
        test_board = "Test Board"

    SKU_ID = ((data & 0x38) >> 3 )
    if ( SKU_ID == 0 ):
        SKU = "Yosemite"
    elif ( SKU_ID == 1 ):
        SKU = "Yosemite V2"
    elif ( SKU_ID == 2 ):
        SKU = "Triton-Type 5A (Left sub-system)"
    elif ( SKU_ID == 3 ):
        SKU = "Triton-Type 5B (Right sub-system)"
    elif ( SKU_ID == 4 ):
        SKU = "Triton-Type 7 SS (IOC based IOM)"

    slot_index = str((data & 0x7))

    print("Presense: " + presense)
    print(test_board)
    print("SKU: " + SKU)
    print("Slot Index: " + slot_index)

    # Get PCIe config
    config = pcie_config(fru)
    print("PCIe Configuration:" + config)


'''
OEM Get PCIe Configuration (NetFn:0x30, CMD: 0xF4h)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - config number
      0x00: Empty/Unknown
      0x01: Glacier Point
      0x0F: Crane Flat
'''
def pcie_config(fru):
    server_type = get_server_type(fru)
    if (server_type == 0):
        server_name = "Twin Lakes"
    elif (server_type == 1):
        server_name = "RC"
    elif (server_type == 2):
        server_name = "EP"
    else:
        server_name = "Unknown"

    result = execute_IPMI_command(fru, 0x30, 0xF4, "")
    if ( result[0] == "00" ):
        config = "4x " + server_name
    elif ( result[0] == "01" ):
        config = "2x GP + 2x " + server_name
    elif ( result[0] == "0F" ):
        config = "2x CF + 2x " + server_name
    elif ( result[0] == "10" ):
        config = "2x GPv2 + 2x " + server_name
    else:
        config = "Unknown"

    return config
