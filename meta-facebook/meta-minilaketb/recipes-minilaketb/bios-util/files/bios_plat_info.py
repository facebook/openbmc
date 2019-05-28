#!/usr/bin/env python
import os.path
import sys
from subprocess import PIPE, Popen

from bios_ipmi_util import *


"""
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
"""


def plat_info(fru):
    req_data = [""]
    presense = "Not Present"
    test_board = "Non Test Board"
    SKU = "Unknown"
    slot_index = ""
    result = execute_IPMI_command(fru, 0x30, 0x7E, "")

    data = int(result[0], 16)

    if data & 0x80:
        presense = "Present"

    if data & 0x40:
        test_board = "Test Board"

    SKU_ID = (data & 0x38) >> 3
    if SKU_ID == 0:
        SKU = "Yosemite"
    elif SKU_ID == 1:
        SKU = "Yosemite V2"
    elif SKU_ID == 2:
        SKU = "Triton-Type 5A (Left sub-system)"
    elif SKU_ID == 3:
        SKU = "Triton-Type 5B (Right sub-system)"
    elif SKU_ID == 4:
        SKU = "Triton-Type 7 SS (IOC based IOM)"

    slot_index = str((data & 0x7))

    print("Presense: " + presense)
    print(test_board)
    print("SKU: " + SKU)
    print("Slot Index: " + slot_index)

    # Get PCIe config
    config = pcie_config(fru)
    print("PCIe Configuration:" + config)


"""
OEM Get PCIe Configuration (NetFn:0x30, CMD: 0xF4h)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - config number
      0x00: Empty/Unknown
      0x01: Glacier Point
      0x0F: Crane Flat
"""


def pcie_config(fru):
    req_data = [""]
    result = execute_IPMI_command(fru, 0x30, 0xF4, "")

    if result[0] == "00":
        config = "4x Twin Lakes/Unknown"
    elif result[0] == "01":
        config = "2x GP + 2x Twin Lakes"
    elif result[0] == "0F":
        config = "2x CF + 2x Twin Lakes"
    else:
        config = "Unknown"

    return config
