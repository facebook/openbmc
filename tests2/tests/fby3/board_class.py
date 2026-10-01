#!/usr/bin/env python3
#
# Copyright 2026-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
"""Detect which fby3 board class the BMC is running on.

A single fby3 image boots two very different machines:

  class 1  a four-slot baseboard chassis (model YV3_DL_T1_CHASSIS) with the
           Medusa board, the baseboard HSC, the PDB and per-slot GPIOs
  class 2  a single-host NIC-BMC server (model DELTALAKE_CPL_CONFIGC_T8) with
           one slot and none of the above

fruid-util cannot tell them apart -- both report "Board Product: BMC Storage
Module" / "Product Name: T8 MP" -- so any test that asserts a sensor or GPIO
inventory has to detect the class itself.

This mirrors fby3_common_get_bmc_location() in
meta-facebook/meta-fby3/recipes-fby3/plat-libs/files/fby3_common/fby3_common.c:
read the four BOARD_BMC_ID shadows LSB first. Using the firmware's own
discriminator means the tests branch exactly where pal_set_server_power() and
friends branch, and it needs no IPMB round trip to the BIC, so it still works
when the host is off.
"""

import os

from utils.cit_logger import Logger

GPIO_SHADOW_ROOT = "/tmp/gpionames"

BOARD_ID_SHADOWS = [
    "BOARD_BMC_ID0_R",
    "BOARD_BMC_ID1_R",
    "BOARD_BMC_ID2_R",
    "BOARD_BMC_ID3_R",
]

# fby3_common.h:217-219
NIC_BMC = 0x09
DVT_BB_BMC = 0x07
BB_BMC = 0x0E

BOARD_CLASS_UNKNOWN = 0

# Verified 2026-10-01: rtptest162-oob.rva3 reads 0x09, and
# sled2403-oob.r0007.p0009.f0001.03.rva3 reads 0x07.
BMC_LOCATION_TO_CLASS = {
    BB_BMC: 1,
    DVT_BB_BMC: 1,
    NIC_BMC: 2,
}

_cached_class = None


def get_board_class() -> int:
    """Return 1 or 2, or BOARD_CLASS_UNKNOWN if the board ID is unreadable."""
    global _cached_class
    if _cached_class is not None:
        return _cached_class

    bmc_location = 0
    try:
        for bit, shadow in enumerate(BOARD_ID_SHADOWS):
            path = os.path.join(GPIO_SHADOW_ROOT, shadow, "value")
            with open(path, "r") as f:
                bmc_location |= (1 if f.readline().strip() == "1" else 0) << bit
    except OSError as e:
        Logger.info("Could not read fby3 board ID GPIOs: {}".format(e))
        _cached_class = BOARD_CLASS_UNKNOWN
        return _cached_class

    board_class = BMC_LOCATION_TO_CLASS.get(bmc_location, BOARD_CLASS_UNKNOWN)
    if board_class == BOARD_CLASS_UNKNOWN:
        Logger.info("Unrecognised fby3 board ID 0x{:02X}".format(bmc_location))
    _cached_class = board_class
    return _cached_class


def select_by_class(by_class: dict, default):
    """Pick the entry for this board class, falling back to default.

    The fallback is deliberately the class 1 superset: if detection ever
    breaks, the test should go loud rather than quietly assert a thinner
    inventory and pass on hardware it never actually checked.
    """
    board_class = get_board_class()
    if board_class == BOARD_CLASS_UNKNOWN:
        return default
    return by_class[board_class]
