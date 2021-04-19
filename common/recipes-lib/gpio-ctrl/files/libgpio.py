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
from enum import IntEnum


lgpio_hndl = ctypes.CDLL("libgpio-ctrl.so.0", use_errno=True)


class GPIOValue(IntEnum):
    INVALID = -1
    LOW = 0
    HIGH = 1


class GPIODirection(IntEnum):
    INVALID = -1
    IN = 0
    OUT = 1


class GPIOEdge(IntEnum):
    INVALID = -1
    NONE = 0
    RISING = 1
    FALLING = 2
    BOTH = 3


class GpioOperationFailure(Exception):
    def __init__(self, errno=0):
        self.errno = errno
        try:
            self.message = os.strerror(self.errno)
        except ValueError:
            self.message = "ERROR: %s" % (self.errno)


def gpio_export_by_name(chip, name, shadow):
    chip_e = ctypes.create_string_buffer(chip.encode())
    name_e = ctypes.create_string_buffer(name.encode())
    shadow_e = ctypes.create_string_buffer(shadow.encode())
    ret = lgpio_hndl.gpio_export_by_name(chip_e, name_e, shadow_e)
    if ret != 0:
        raise GpioOperationFailure(ctypes.get_errno())


def gpio_export_by_offset(chip, offset, shadow):
    chip_e = ctypes.create_string_buffer(chip.encode())
    shadow_e = ctypes.create_string_buffer(shadow.encode())
    ret = lgpio_hndl.gpio_export_by_offset(chip_e, ctypes.c_int(offset), shadow_e)
    if ret != 0:
        raise GpioOperationFailure(ctypes.get_errno())


def gpio_unexport(shadow):
    shadow_e = ctypes.create_string_buffer(shadow.encode())
    ret = lgpio_hndl.gpio_unexport(shadow_e)
    if ret != 0:
        raise GpioOperationFailure(ctypes.get_errno())


class GPIO:
    def __init__(self, shadow=None, chip=None, name=None, offset=None):
        if shadow is not None:
            shadow_e = ctypes.create_string_buffer(shadow.encode())
            open_by_shadow = lgpio_hndl.gpio_open_by_shadow
            open_by_shadow.restype = ctypes.c_void_p
            ret = open_by_shadow(shadow_e)
            if ret is None:
                raise GpioOperationFailure(ctypes.get_errno())
            self.desc = ret
        elif chip is not None and name is not None:
            chip_e = ctypes.create_string_buffer(chip.encode())
            name_e = ctypes.create_string_buffer(name.encode())
            open_by_name = lgpio_hndl.gpio_open_by_name
            open_by_name.restype = ctypes.c_void_p
            ret = open_by_name(chip_e, name_e)
            if ret is None:
                raise GpioOperationFailure(ctypes.get_errno())
            self.desc = ret
        elif chip is not None and offset is not None:
            chip_e = ctypes.create_string_buffer(chip.encode())
            open_by_offset = lgpio_hndl.gpio_open_by_offset
            open_by_offset.restype = ctypes.c_void_p
            ret = open_by_offset(chip_e, ctypes.c_int(offset))
            if ret is None:
                raise GpioOperationFailure(ctypes.get_errno())
            self.desc = ret
        else:
            GpioOperationFailure(1)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        if self.desc is not None:
            self.close()

    def close(self):
        if self.desc is None:
            raise GpioOperationFailure(1)
        _gpio_close = lgpio_hndl.gpio_close
        _gpio_close.argtypes = (ctypes.c_void_p,)
        ret = _gpio_close(self.desc)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())
        self.desc = None

    def get_value(self):
        if self.desc is None:
            raise GpioOperationFailure(1)
        value = ctypes.c_int(-1)
        pvalue = ctypes.pointer(value)
        get_value = lgpio_hndl.gpio_get_value
        get_value.argtypes = (ctypes.c_void_p, ctypes.POINTER(ctypes.c_int))
        get_value.restype = ctypes.c_int
        ret = get_value(self.desc, pvalue)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())
        return GPIOValue(value.value)

    def set_value(self, val, init=False):
        if self.desc is None:
            raise GpioOperationFailure(1)
        value = ctypes.c_int(val)
        if init:
            set_value = lgpio_hndl.gpio_set_init_value
        else:
            set_value = lgpio_hndl.gpio_set_value
        set_value.argtypes = (ctypes.c_void_p, ctypes.c_int)
        set_value.restype = ctypes.c_int
        ret = set_value(self.desc, value)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())

    def get_direction(self):
        if self.desc is None:
            raise GpioOperationFailure(1)
        value = ctypes.c_int(-1)
        pvalue = ctypes.pointer(value)
        get_value = lgpio_hndl.gpio_get_direction
        get_value.argtypes = (ctypes.c_void_p, ctypes.POINTER(ctypes.c_int))
        get_value.restype = ctypes.c_int
        ret = get_value(self.desc, pvalue)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())
        return GPIODirection(value.value)

    def set_direction(self, val):
        if self.desc is None:
            raise GpioOperationFailure(1)
        value = ctypes.c_int(val)
        set_value = lgpio_hndl.gpio_set_direction
        set_value.argtypes = (ctypes.c_void_p, ctypes.c_int)
        set_value.restype = ctypes.c_int
        ret = set_value(self.desc, value)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())

    def get_edge(self):
        if self.desc is None:
            raise GpioOperationFailure(1)
        value = ctypes.c_int(-1)
        pvalue = ctypes.pointer(value)
        get_value = lgpio_hndl.gpio_get_edge
        get_value.argtypes = (ctypes.c_void_p, ctypes.POINTER(ctypes.c_int))
        get_value.restype = ctypes.c_int
        ret = get_value(self.desc, pvalue)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())
        return GPIOEdge(value.value)

    def set_edge(self, val):
        if self.desc is None:
            raise GpioOperationFailure(1)
        value = ctypes.c_int(val)
        set_value = lgpio_hndl.gpio_set_edge
        set_value.argtypes = (ctypes.c_void_p, ctypes.c_int)
        set_value.restype = ctypes.c_int
        ret = set_value(self.desc, value)
        if ret != 0:
            raise GpioOperationFailure(ctypes.get_errno())
