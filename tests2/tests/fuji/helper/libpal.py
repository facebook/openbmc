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

    BOARD_FUJI_EVT1 = 0x40
    BOARD_FUJI_EVT2 = 0x41
    BOARD_FUJI_EVT3 = 0x42
    BOARD_FUJI_DVT1 = 0x43
    BOARD_FUJI_PVT = 0x45
    BOARD_UNDEFINED = 0xFF

    board_rev = {
        BOARD_FUJI_EVT1: "Fuji-EVT1",
        BOARD_FUJI_EVT2: "Fuji-EVT2",
        BOARD_FUJI_EVT3: "Fuji-EVT3",
        BOARD_FUJI_DVT1: "Fuji-DVT1",
        BOARD_FUJI_PVT: "Fuji-PVT",
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
