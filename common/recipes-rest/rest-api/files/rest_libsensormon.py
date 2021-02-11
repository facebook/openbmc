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

import ctypes
import re
from functools import lru_cache
from typing import Dict, Tuple


libsensormon = ctypes.CDLL("/usr/local/fbpackages/sensor-mon/libsensormon.so")
libpal = ctypes.CDLL("/usr/lib/libpal.so.0")


class RestLibSensorMonError(Exception):
    pass


class RestSensorReadError(RestLibSensorMonError):
    pass


@lru_cache(maxsize=None)
def pal_get_fru_id(fru_name: str) -> ctypes.c_uint8:
    c_fru_name = ctypes.c_char_p(fru_name.encode("utf-8"))
    c_fru_id = ctypes.c_uint8(0)
    ret = libpal.pal_get_fru_id(c_fru_name, ctypes.pointer(c_fru_id))
    if ret != 0:
        raise ValueError("Invalid FRU name: " + fru_name)
    return c_fru_id


def pal_get_fru_list():
    c_pal_fru_list = (ctypes.c_char * 100)(b"\x00")
    libpal.pal_get_fru_list(c_pal_fru_list)
    pal_fru_list = re.findall("[^, ]+", c_pal_fru_list.value.decode("utf-8"))
    if pal_fru_list and pal_fru_list[0] == "all":
        pal_fru_list.pop("all")
    return pal_fru_list


@lru_cache(1)
def pal_fru_name_map() -> Dict[str, ctypes.c_uint8]:
    pal_fru_list = pal_get_fru_list()
    return {fru_name: pal_get_fru_id(fru_name) for fru_name in pal_fru_list}


def pal_get_fru_sensor_list(c_fru_id: ctypes.c_uint8) -> Tuple[ctypes.c_uint8]:
    c_sensor_list = ctypes.pointer(ctypes.c_uint8())
    c_cnt = ctypes.c_int()
    ret = libpal.pal_get_fru_sensor_list(
        c_fru_id, ctypes.pointer(c_sensor_list), ctypes.pointer(c_cnt)
    )
    if ret != 0:
        raise RestSensorReadError("pal_get_fru_sensor_list() returned " + str(ret))
    return tuple(c_sensor_list[i] for i in range(c_cnt.value))


@lru_cache(maxsize=None)
def pal_get_sensor_name(fru_id: int, snr_num: int):
    c_sensor_name = (ctypes.c_char * 100)(b"\x00")
    ret = libpal.pal_get_sensor_name(
        fru_id, ctypes.c_uint8(snr_num), ctypes.pointer(c_sensor_name)
    )
    if ret != 0:
        raise RestSensorReadError("pal_get_sensor_name() returned " + str(ret))
    return c_sensor_name.value.decode("utf-8")


def sensor_read(c_fru_id: ctypes.c_uint8, snr_num) -> float:
    c_val = ctypes.c_float()
    if libpal.pal_sensor_is_cached(c_fru_id, snr_num):
        ret = libpal.sensor_cache_read(c_fru_id, snr_num, ctypes.pointer(c_val))
    else:
        ret = libsensormon.sensor_raw_read_helper(
            c_fru_id, snr_num, ctypes.pointer(c_val)
        )
    if ret != 0:
        raise RestSensorReadError("Sensor reading function returned " + str(ret))

    return c_val.value


def pal_is_fru_prsnt(c_fru_id) -> bool:
    c_status = ctypes.c_uint8()
    ret = libpal.pal_is_fru_prsnt(c_fru_id, ctypes.pointer(c_status))
    if ret != 0:
        raise ValueError("pal_is_fru_prsnt() returned " + str(ret))
    return bool(c_status.value)
