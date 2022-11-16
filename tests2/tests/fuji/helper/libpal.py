#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

from ctypes import CDLL, byref, c_uint8, pointer, c_char_p

# When tests are discovered out of BMC there is no libpal
# catch the import failure
try:
    lpal_hndl = CDLL("libpal.so.0")
except Exception:
    pass


class BoardRevision:
    """Board revision"""

    BOARD_FUJI_EVT1 = 0
    BOARD_FUJI_EVT2 = 1
    BOARD_FUJI_EVT3 = 2
    BOARD_FUJI_DVT1_DVT2 = 3
    BOARD_FUJI_PVT1_PVT2 = 4
    BOARD_FUJI_MP0SKU1 = 5
    BOARD_FUJI_MP0SKU2 = 6
    BOARD_FUJI_MP1SKU1 = 7
    BOARD_FUJI_MP1SKU2 = 8
    BOARD_FUJI_MP1SKU3 = 9
    BOARD_FUJI_MP1SKU4 = 10
    BOARD_FUJI_MP1SKU5 = 11
    BOARD_FUJI_MP1SKU6 = 12
    BOARD_FUJI_MP1SKU7 = 13
    BOARD_FUJI_MP2SKU1 = 14
    BOARD_FUJI_MP2SKU2 = 15
    BOARD_FUJI_MP2SKU3 = 16
    BOARD_FUJI_MP2SKU4 = 17
    BOARD_UNDEFINED = 0xFF

    board_rev = {
        BOARD_FUJI_EVT1: "Fuji-EVT1",
        BOARD_FUJI_EVT2: "Fuji-EVT2",
        BOARD_FUJI_EVT3: "Fuji-EVT3",
        BOARD_FUJI_DVT1_DVT2: "Fuji-DVT1/DVT2",
        BOARD_FUJI_PVT1_PVT2: "Fuji-PVT1/PVT2",
        BOARD_FUJI_MP0SKU1: "Fuji-MP0SKU1",
        BOARD_FUJI_MP0SKU2: "Fuji-MP0SKU2",
        BOARD_FUJI_MP1SKU1: "Fuji-MP1SKU1",
        BOARD_FUJI_MP1SKU2: "Fuji-MP1SKU2",
        BOARD_FUJI_MP1SKU3: "Fuji-MP1SKU3",
        BOARD_FUJI_MP1SKU4: "Fuji-MP1SKU4",
        BOARD_FUJI_MP1SKU5: "Fuji-MP1SKU5",
        BOARD_FUJI_MP1SKU6: "Fuji-MP1SKU6",
        BOARD_FUJI_MP1SKU7: "Fuji-MP1SKU7",
        BOARD_FUJI_MP2SKU1: "Fuji-MP2SKU1",
        BOARD_FUJI_MP2SKU2: "Fuji-MP2SKU2",
        BOARD_FUJI_MP2SKU3: "Fuji-MP2SKU3",
        BOARD_FUJI_MP2SKU4: "Fuji-MP2SKU4",
        BOARD_UNDEFINED: "Undefined",
    }


def pal_get_board_rev():
    """get board revision"""
    brd_rev = c_uint8()
    ret = lpal_hndl.pal_get_board_rev(byref(brd_rev))
    if ret:
        return None
    else:
        return BoardRevision.board_rev.get(brd_rev.value, None)


def pal_is_fru_prsnt(fru_id: int) -> bool:
    """Returns whether a fru is present"""
    c_status = c_uint8()

    ret = lpal_hndl.pal_is_fru_prsnt(fru_id, pointer(c_status))
    if ret != 0:
        raise ValueError("pal_is_fru_prsnt() returned " + str(ret))

    return bool(c_status.value)


def pal_get_fru_id(fru_name: str) -> int:
    """Given a FRU name, return the corresponding fru id"""
    c_fru_name = c_char_p(fru_name.encode("utf-8"))  # noqa
    c_fru_id = c_uint8(0)

    ret = lpal_hndl.pal_get_fru_id(c_fru_name, pointer(c_fru_id))
    if ret != 0:
        raise ValueError("Invalid FRU name: " + repr(fru_name))

    return c_fru_id.value
