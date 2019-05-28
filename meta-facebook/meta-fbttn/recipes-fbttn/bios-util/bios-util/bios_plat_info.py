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
    result = execute_IPMI_command(fru, 0x30, 0x7E, "")

    data = int(result[0], 16)
    do_plat_info_action(data)

    # Get PCIe config
    config = pcie_config(fru)
    print("PCIe Configuration: " + config)


def do_plat_info_action(data):
    presense = "Not Present"
    test_board = "Non Test Board"
    SKU = "Unknown"
    slot_index = ""

    if data & 0x80:
        presense = "Present"

    if data & 0x40:
        test_board = "Test Board"

    SKU_ID = (data & 0x38) >> 3
    if SKU_ID == 0:
        SKU = "Yosemite"
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


"""
OEM Get PCIe Configuration (NetFn:0x30, CMD: 0xF4h)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - config number
     0x6: Triton-Type 5A/5B
     0x8: Triton-Type 7
     0xA: Yosemite
"""


def pcie_config(fru):
    result = execute_IPMI_command(fru, 0x30, 0xF4, "")
    return do_pcie_config_action(result)


def do_pcie_config_action(result):
    if result[0] == "06":
        config = "Triton-Type 5"
    elif result[0] == "08":
        config = "Triton-Type 7"
    elif result[0] == "0A":
        config = "Yosemite"
    else:
        config = "Unknown"

    return config
