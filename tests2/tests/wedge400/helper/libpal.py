#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

from ctypes import CDLL, byref, c_uint8

# When tests are discovered out of BMC there is no libpal
# catch the import failure
try:
    lpal_hndl = CDLL("libpal.so.0")
except Exception:
    pass

class BoardRevision:
    """ Board type """

    BRD_TYPE_WEDGE400 = 0x00
    BRD_TYPE_WEDGE400C = 0x01

    """ Board type and revision """
    BOARD_WEDGE400_EVT_EVT3 = 0x00
    BOARD_WEDGE400_DVT = 0x02
    BOARD_WEDGE400_DVT2 = 0x03
    BOARD_WEDGE400_PVT3 = 0x04
    BOARD_WEDGE400_MP = 0x05
    BOARD_WEDGE400_MP_RESPIN = 0x06
    BOARD_WEDGE400C_EVT = 0x10
    BOARD_WEDGE400C_EVT2 = 0x11
    BOARD_WEDGE400C_DVT = 0x12
    BOARD_WEDGE400C_DVT2 = 0x13
    BOARD_WEDGE400C_MP_RESPIN = 0x14
    BOARD_UNDEFINED = 0xFF

    POWER_MODULE_PEM1 = 0x03
    POWER_MODULE_PEM2 = 0x04
    POWER_MODULE_PSU1 = 0x05
    POWER_MODULE_PSU2 = 0x06

    board_type = {BRD_TYPE_WEDGE400: "Wedge400", BRD_TYPE_WEDGE400C: "Wedge400C"}
    power_type = {
        POWER_MODULE_PEM1: "pem1",
        POWER_MODULE_PEM2: "pem2",
        POWER_MODULE_PSU1: "psu1",
        POWER_MODULE_PSU2: "psu2",
    }

    board_type_rev = {
        BOARD_WEDGE400_EVT_EVT3: "Wedge400-EVT/EVT3",
        BOARD_WEDGE400_DVT: "Wedge400-DVT",
        BOARD_WEDGE400_DVT2: "Wedge400-DVT2/PVT/PVT2",
        BOARD_WEDGE400_PVT3: "Wedge400-PVT3",
        BOARD_WEDGE400_MP: "Wedge400-MP",
        BOARD_WEDGE400_MP_RESPIN: "Wedge400-MP-Respin",
        BOARD_WEDGE400C_EVT: "Wedge400C-EVT",
        BOARD_WEDGE400C_EVT2: "Wedge400C-EVT2",
        BOARD_WEDGE400C_DVT: "Wedge400C-DVT",
        BOARD_WEDGE400C_DVT2: "Wedge400C-DVT2",
        BOARD_WEDGE400C_MP_RESPIN : "Wedge400C-MP-Respin",
        BOARD_UNDEFINED: "Undefined",
    }


def pal_get_board_type():
    """ get board type """
    brd_type = c_uint8()
    ret = lpal_hndl.pal_get_board_type(byref(brd_type))
    if ret:
        return None
    else:
        return BoardRevision.board_type.get(brd_type.value, None)


def pal_get_board_type_rev():
    """ get board type and revision """
    board_type_rev = c_uint8()
    ret = lpal_hndl.pal_get_board_type_rev(byref(board_type_rev))
    if ret:
        return None
    else:
        return BoardRevision.board_type_rev.get(board_type_rev.value, None)


def pal_detect_power_supply_present(fru):
    """get power module type """
    power_type_rev = c_uint8()
    lpal_hndl.pal_is_fru_prsnt(fru, byref(power_type_rev))
    if power_type_rev.value:
        return BoardRevision.power_type.get(fru, None)
    else:
        return None
