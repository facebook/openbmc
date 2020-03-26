#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
import ctypes


lkv_hndl = ctypes.CDLL("libkv.so.0")

FPERSIST = 1
FCREATE = 2


class KeyOperationFailure(Exception):
    pass


def kv_get(key, flags=0):
    key_c = ctypes.create_string_buffer(key.encode())
    value = ctypes.create_string_buffer(64)
    ret = lkv_hndl.kv_get(key_c, value, 0, ctypes.c_uint(flags))
    if ret != 0:
        raise KeyOperationFailure
    return value.value.decode()


def kv_set(key, value, flags=0):
    key_c = ctypes.create_string_buffer(key.encode())
    value_c = ctypes.create_string_buffer(value.encode())
    ret = lkv_hndl.kv_set(key_c, value_c, 0, ctypes.c_uint(flags))
    if ret != 0:
        raise KeyOperationFailure
