#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

"""
Thin python wrappers over libpal.so
Whenever possible, using the same name as the original function to
make tracing and debugging easier
"""

import ctypes
import re
from contextlib import suppress
from functools import lru_cache
from typing import Dict, Tuple, List


libpal = ctypes.CDLL("libpal.so.0")

FRU_LIST_BUF_MAX_LEN = 1024
SENSOR_NAME_MAX_LEN = 256
PLATFORM_NAME_MAX_LEN = 256


class LibPalError(Exception):
    pass


@lru_cache(1)
def pal_get_platform_name() -> str:
    c_plat_name = ctypes.create_string_buffer(PLATFORM_NAME_MAX_LEN)

    ret = libpal.pal_get_platform_name(ctypes.pointer(c_plat_name))
    if ret != 0:
        raise LibPalError("pal_get_platform_name() returned " + str(ret))

    return c_plat_name.value.decode("utf-8")


@lru_cache(maxsize=None)
def pal_get_fru_id(fru_name: str) -> int:
    "Given a FRU name, return the corresponding fru id"
    c_fru_name = ctypes.c_char_p(fru_name.encode("utf-8"))
    c_fru_id = ctypes.c_uint8(0)

    ret = libpal.pal_get_fru_id(c_fru_name, ctypes.pointer(c_fru_id))
    if ret != 0:
        raise ValueError("Invalid FRU name: " + repr(fru_name))

    return c_fru_id.value


@lru_cache(1)
def pal_get_fru_list() -> Tuple[str, ...]:
    "Returns a list of FRU names registered in libpal"
    c_pal_fru_list = ctypes.create_string_buffer(FRU_LIST_BUF_MAX_LEN)

    libpal.pal_get_fru_list(c_pal_fru_list)

    pal_fru_list = re.findall("[^, ]+", c_pal_fru_list.value.decode("utf-8"))
    # Remove catchall FRU name as it's not a real FRU
    with suppress(ValueError):
        pal_fru_list.remove("all")

    return tuple(pal_fru_list)


@lru_cache(1)
def pal_fru_name_map() -> Dict[str, int]:
    "Returns a {fru_name: fru_id, ...} map for all FRUs in the system"
    pal_fru_list = pal_get_fru_list()

    return {fru_name: pal_get_fru_id(fru_name) for fru_name in pal_fru_list}


@lru_cache(maxsize=None)
def pal_get_fru_sensor_list(fru_id: int) -> Tuple[int, ...]:
    "Returns all sensor identifiers (snr_num) given a fru_id"
    c_sensor_list = ctypes.pointer(ctypes.c_uint8())
    c_cnt = ctypes.c_int()

    ret = libpal.pal_get_fru_sensor_list(
        fru_id, ctypes.pointer(c_sensor_list), ctypes.pointer(c_cnt)
    )
    if ret != 0:
        raise LibPalError("pal_get_fru_sensor_list() returned " + str(ret))

    return tuple(c_sensor_list[i] for i in range(c_cnt.value))


@lru_cache(maxsize=None)
def pal_get_sensor_name(fru_id: int, snr_num: int) -> str:
    "Given a (fru_id, snr_num) pair, return the sensor name"
    c_sensor_name = ctypes.create_string_buffer(SENSOR_NAME_MAX_LEN)

    ret = libpal.pal_get_sensor_name(
        fru_id, ctypes.c_uint8(snr_num), ctypes.pointer(c_sensor_name)
    )
    if ret != 0:
        raise LibPalError("pal_get_sensor_name() returned " + str(ret))

    return c_sensor_name.value.decode("utf-8")


def pal_is_fru_prsnt(fru_id: int) -> bool:
    "Returns whether a fru is present"
    c_status = ctypes.c_uint8()

    ret = libpal.pal_is_fru_prsnt(fru_id, ctypes.pointer(c_status))
    if ret != 0:
        raise ValueError("pal_is_fru_prsnt() returned " + str(ret))

    return bool(c_status.value)


def pal_is_slot_server(fru_id: int) -> bool:
    "Return whether a FRU is a server type or not"
    return libpal.pal_is_slot_server(fru_id) != 0


def pal_get_postcode(fru: int) -> List[int]:
    "Return the list of postcodes of server since boot"
    postcodes = (ctypes.c_ubyte * 256)()
    plen = ctypes.c_uint(0)
    status = libpal.pal_get_80port_record(
        fru, ctypes.byref(postcodes), 256, ctypes.byref(plen)
    )
    if status != 0:
        raise ValueError("Error %d returned by PAL API" % (status))
    ret = []
    for i in range(0, plen.value):
        ret.append((postcodes[i]))
    return ret


def pal_get_4byte_postcode(fru: int):
    ret = []
    MAX_PAGE_NUM = int(16)
    for i in range(1, (MAX_PAGE_NUM + 1)):
        postcodes = (ctypes.c_uint * 256)()
        page = ctypes.c_uint8(i)
        plen = ctypes.c_uint(0)
        status = libpal.pal_get_80port_page_record(
            fru, page, ctypes.byref(postcodes), 256, ctypes.byref(plen)
        )

        if status != 0:
            raise ValueError("Error %d returned by PAL API" % (status))
        for i in range(0, int(plen.value / 4)):
            ret.append((postcodes[i]))

    return ret


## Sensor reading functions
def pal_sensor_is_cached(fru_id: int, snr_num: int) -> bool:
    return bool(libpal.pal_sensor_is_cached(fru_id, snr_num))


def sensor_cache_read(fru_id: int, snr_num: int) -> float:
    c_val = ctypes.c_float()

    ret = libpal.sensor_cache_read(fru_id, snr_num, ctypes.pointer(c_val))
    if ret != 0:
        raise LibPalError("sensor_cache_read() returned " + str(ret))

    return c_val.value


def sensor_raw_read(fru_id: int, snr_num: int) -> float:
    c_val = ctypes.c_float()

    ret = libpal.sensor_raw_read(fru_id, snr_num, ctypes.pointer(c_val))
    if ret != 0:
        raise LibPalError("sensor_raw_read() returned " + str(ret))

    return c_val.value


def sensor_read(fru_id: int, snr_num: int) -> float:
    """
    Convenience function that tries reading sensor values from the cache before
    falling back to a raw read
    """
    if libpal.pal_sensor_is_cached(fru_id, snr_num):
        return sensor_cache_read(fru_id, snr_num)

    return sensor_raw_read(fru_id, snr_num)
