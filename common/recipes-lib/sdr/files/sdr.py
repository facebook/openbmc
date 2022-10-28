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
Thin python wrappers over libsdr.so
Whenever possible, using the same name as the original function to
make tracing and debugging easier
"""
import ctypes
from functools import lru_cache

libsdr = ctypes.CDLL("libsdr.so.0")

# ThreshSensor is the representation of this struct
# typedef struct {
#   uint16_t flag;
#   float ucr_thresh;
#   float unc_thresh;
#   float unr_thresh;
#   float lcr_thresh;
#   float lnc_thresh;
#   float lnr_thresh;
#   float pos_hyst;
#   float neg_hyst;
#   int curr_state;
#   char name[32];
#   char units[64];
#   uint32_t poll_interval; // poll interval for each fru's sensor

# } thresh_sensor_t;
class ThreshSensor(ctypes.Structure):
    _fields_ = [
        ("flag", ctypes.c_uint16),
        ("ucr_thresh", ctypes.c_float),
        ("unc_thresh", ctypes.c_float),
        ("unr_thresh", ctypes.c_float),
        ("lcr_thresh", ctypes.c_float),
        ("lnc_thresh", ctypes.c_float),
        ("lnr_thresh", ctypes.c_float),
        ("pos_hyst", ctypes.c_float),
        ("neg_hyst", ctypes.c_float),
        ("curr_state", ctypes.c_int),
        ("name", ctypes.c_char * 32),
        ("units", ctypes.c_char * 64),
        ("poll_interval", ctypes.c_uint32),
    ]

    def __repr__(self):
        return "sdr.ThreshSensor(flag=%d, ucr_thresh=%f, unc_thresh=%f, unr_thresh=%f, \
        lcr_thresh=%f, lnc_thresh=%f, lnr_thresh=%f, pos_hyst=%f, neg_hyst=%f, curr_state=%d, name=%s, \
        units=%s, poll_interval=%d)" % (
            self.flag,
            self.ucr_thresh,
            self.unc_thresh,
            self.unr_thresh,
            self.lcr_thresh,
            self.lnc_thresh,
            self.lnr_thresh,
            self.pos_hyst,
            self.neg_hyst,
            self.curr_state,
            self.name,
            self.units,
            self.poll_interval,
        )


class LibSdrError(Exception):
    pass


@lru_cache(maxsize=None)
def sdr_get_sensor_name(fru: int, snr_num: int) -> str:
    "Given a (fru_id, snr_num) pair, return the sensor name"
    c_sensor_name = ctypes.create_string_buffer(32)

    ret = libsdr.sdr_get_sensor_name(
        ctypes.c_uint8(fru), ctypes.c_uint8(snr_num), ctypes.pointer(c_sensor_name)
    )
    if ret != 0:
        raise LibSdrError("sdr_get_sensor_name() returned " + str(ret))

    return c_sensor_name.value.decode("utf-8")


@lru_cache(maxsize=None)
def sdr_get_sensor_units(fru: int, snr_num: int) -> str:
    "Given a (fru_id, snr_num) pair, return the sensor units"
    c_sensor_units = ctypes.create_string_buffer(64)

    ret = libsdr.sdr_get_sensor_units(
        ctypes.c_uint8(fru), ctypes.c_uint8(snr_num), ctypes.pointer(c_sensor_units)
    )
    if ret != 0:
        raise LibSdrError("sdr_get_sensor_units() returned " + str(ret))

    return c_sensor_units.value.decode("utf-8")


def sdr_get_sensor_thresh(fru: int, snr_num: int) -> ThreshSensor:
    "Given a (fru_id, snr_num) pair, return the sensor threshold object"
    c_sensor_thresholds = ThreshSensor()
    ret = libsdr.sdr_get_snr_thresh(
        ctypes.c_uint8(fru),
        ctypes.c_uint8(snr_num),
        ctypes.pointer(c_sensor_thresholds),
    )
    if ret != 0:
        raise LibSdrError("sdr_get_snr_thresh() returned " + str(ret))
    return c_sensor_thresholds
