#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

from ctypes import CDLL, c_uint8, pointer, c_char_p

# When tests are discovered out of BMC there is no libpal
# catch the import failure
try:
    lpal_hndl = CDLL("libpal.so.0")
except Exception:
    pass


def pal_get_fru_id(fru_name):
    """Given a FRU name, return the corresponding fru id"""
    c_fru_name = c_char_p(fru_name.encode("utf-8"))  # noqa
    c_fru_id = c_uint8(0)

    ret = lpal_hndl.pal_get_fru_id(c_fru_name, pointer(c_fru_id))
    if ret != 0:
        raise ValueError("Invalid FRU name: " + repr(fru_name))

    return c_fru_id.value
