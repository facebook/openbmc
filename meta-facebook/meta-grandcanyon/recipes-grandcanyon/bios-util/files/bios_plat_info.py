#!/usr/bin/env python
import os.path
import sys
from subprocess import PIPE, Popen

from bios_ipmi_util import *

NETFN_OEM_REQ             = 0x30
CMD_OEM_GET_PLAT_INFO     = 0x7E
CMD_OEM_GET_PCIE_CONFIG   = 0xF4

PLAT_INFO_TYPE5_A         = 2
PLAT_INFO_TYPE5_B         = 3
PLAT_INFO_TYPE7           = 4

PLAT_INFO_PRESENT_MASK    = 0x80
PLAT_INFO_SKU_MASK        = 0x38
PLAT_INFO_SLOT_INDEX_MASK = 0x7

PLAT_INFO_SKU_SHIFT       = 3

PCIE_CONFIG_TYPE5         = "06"
PCIE_CONFIG_TYPE7         = "08"

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
         010b: Grand Canyon-Type5 A (Left sub-system)
         011b: Grand Canyon-Type5 B (Right sub-system)
         100b: Grand Canyon-Type 7
     Bit 2:0 - Slot Index, 1 based
"""


def plat_info(fru):
    result = execute_IPMI_command(fru, NETFN_OEM_REQ, CMD_OEM_GET_PLAT_INFO, "")

    data = int(result[0], 16)
    do_plat_info_action(data)

    # Get PCIe config
    config = pcie_config(fru)
    print("PCIe Configuration: " + config)


def do_plat_info_action(data):
    presense = "Not Present"
    SKU = "Unknown"
    slot_index = ""

    if data & PLAT_INFO_PRESENT_MASK:
        presense = "Present"

    SKU_ID = (data & PLAT_INFO_SKU_MASK) >> PLAT_INFO_SKU_SHIFT

    if SKU_ID == PLAT_INFO_TYPE5_A:
        SKU = "Grand Canyon-Type5 A (Left sub-system)"
    elif SKU_ID == PLAT_INFO_TYPE5_B:
        SKU = "Grand Canyon-Type5 B (Right sub-system)"
    elif SKU_ID == PLAT_INFO_TYPE7:
        SKU = "Grand Canyon-Type 7"
    else:
        SKU = "Unknown"

    slot_index = str((data & PLAT_INFO_SLOT_INDEX_MASK))

    print("Presense: " + presense)
    print("SKU: " + SKU)
    print("Slot Index: " + slot_index)


"""
OEM Get PCIe Configuration (NetFn:0x30, CMD: 0xF4h)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - config number
     0x6: Grand Canyon-Type 5
     0x8: Grand Canyon-Type 7
"""


def pcie_config(fru):
    result = execute_IPMI_command(fru, NETFN_OEM_REQ, CMD_OEM_GET_PCIE_CONFIG, "")
    return do_pcie_config_action(result)


def do_pcie_config_action(result):
    if result[0] == PCIE_CONFIG_TYPE5:
        config = "Grand Canyon-Type 5"
    elif result[0] == PCIE_CONFIG_TYPE7:
        config = "Grand Canyon-Type 7"
    else:
        config = "Unknown"

    return config

