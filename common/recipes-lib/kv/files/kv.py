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
import os


lkv_hndl = ctypes.CDLL("libkv.so.0", use_errno=True)

FPERSIST = 1
FCREATE = 2


class KeyOperationFailure(Exception):
    pass


class InvalidParameter(KeyOperationFailure):
    pass


class KeyNotFoundFailure(KeyOperationFailure):
    pass


class KeyAlreadyExists(KeyOperationFailure):
    pass


class KeyValueTooBig(KeyOperationFailure):
    pass


def _handle_error():
    e = ctypes.get_errno()
    if e == os.errno.ENOENT:
        raise KeyNotFoundFailure()
    elif e == os.errno.EINVAL:
        raise InvalidParameter()
    elif e == os.errno.E2BIG:
        raise KeyValueTooBig()
    elif e == os.errno.EEXIST:
        raise KeyAlreadyExists()
    else:
        raise KeyOperationFailure("KV operation failed with errno = " + str(e))


def kv_get(key, flags=0, binary=False):
    key_c = ctypes.create_string_buffer(key.encode())
    value = ctypes.create_string_buffer(256)
    ret = lkv_hndl.kv_get(key_c, value, 0, ctypes.c_uint(flags))
    if ret != 0:
        _handle_error()
    if binary:
        return value.value
    return value.value.decode()


def kv_set(key, value, flags=0):
    key_c = ctypes.create_string_buffer(key.encode())
    if isinstance(value, (bytes, bytearray)):
        value_c = ctypes.create_string_buffer(value)
    else:
        value_c = ctypes.create_string_buffer(value.encode())
    ret = lkv_hndl.kv_set(key_c, value_c, 0, ctypes.c_uint(flags))
    if ret != 0:
        _handle_error()


def kv_del(key, flags=0):
    key_c = ctypes.create_string_buffer(key.encode())
    ret = lkv_hndl.kv_del(key_c, ctypes.c_uint(flags))
    if ret != 0:
        _handle_error()
