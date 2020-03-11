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

from ctypes import c_uint8, byref, CDLL


lpal_hndl = CDLL("libpal.so")


class BoardRevision:
    """ Board type """

    BRD_TYPE_WEDGE400 = 0x00
    BRD_TYPE_WEDGE400C = 0x01

    """ Board type and revision """
    BOARD_WEDGE400_EVT = 0x00
    BOARD_WEDGE400_EVT3 = 0x01
    BOARD_WEDGE400_DVT = 0x02
    BOARD_WEDGE400_DVT2 = 0x03
    BOARD_WEDGE400C_EVT = 0x10
    BOARD_WEDGE400C_EVT2 = 0x11
    BOARD_UNDEFINED = 0xFF

    board_type = {BRD_TYPE_WEDGE400: "Wedge400", BRD_TYPE_WEDGE400C: "Wedge400C"}

    board_type_rev = {
        BOARD_WEDGE400_EVT: "Wedge400-EVT",
        BOARD_WEDGE400_EVT3: "Wedge400-EVT3",
        BOARD_WEDGE400_DVT: "Wedge400-DVT",
        BOARD_WEDGE400_DVT2: "Wedge400-DVT2",
        BOARD_WEDGE400C_EVT: "Wedge400C-EVT",
        BOARD_WEDGE400C_EVT2: "Wedge400C-EVT2",
        BOARD_UNDEFINED: "Undefined",
    }


def pal_get_board_type():
    """ get board type """
    brd_type = c_uint8()
    ret = lpal_hndl.pal_get_board_type(byref(brd_type))
    if ret:
        return None
    else:
        return BoardRevision.board_type.get(brd_type.value)


def pal_get_board_type_rev():
    """ get board type and revision """
    board_type_rev = c_uint8()
    ret = lpal_hndl.pal_get_board_type_rev(byref(board_type_rev))
    if ret:
        return None
    else:
        return BoardRevision.board_type_rev.get(board_type_rev.value)
