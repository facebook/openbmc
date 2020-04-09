#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

from ctypes import CDLL, c_ubyte, create_string_buffer, pointer


lpal_hndl = CDLL("libpal.so.0")


def pal_get_fru_list():
    frulist = create_string_buffer(128)
    ret = lpal_hndl.pal_get_fru_list(frulist)
    if ret:
        return None
    else:
        return frulist.value


def pal_log_clear(fru):
    lpal_hndl.pal_log_clear(fru.encode("ascii"))


def pal_get_pair_fru(fru):
    if lpal_hndl is None:
        return 0
    pair_fru = c_ubyte()
    p_pair_fru = pointer(pair_fru)
    ret = lpal_hndl.pal_get_pair_fru(fru, p_pair_fru)
    if ret:
        return pair_fru.value
    else:
        return None
