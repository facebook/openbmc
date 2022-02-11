#!/usr/bin/env python3
#
# Copyright 2016-present Facebook. All Rights Reserved.
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
import os


CPLD_VER = "cpld_ver"
CPLD_SUB_VER = "cpld_sub_ver"

FCB_NUM_TO_SUBPATH = {
    1: "i2c-32/i2c-171/171-0033",
    2: "i2c-33/i2c-179/179-0033",
    3: "i2c-34/i2c-187/187-0033",
    4: "i2c-35/i2c-195/195-0033",
}


def _get_version(version_file):
    with open(version_file) as f:
        return f.readline()


def _firmware_json(ver, sub_ver, int_base=16):
    return {"version": int(ver, int_base), "sub_version": int(sub_ver, int_base)}


def _get_syscpld_info():
    SYS_CPLD_PATH = "/sys/class/i2c-dev/i2c-13/device/13-003e"
    ver = _get_version(os.path.join(SYS_CPLD_PATH, CPLD_VER))
    sub_ver = _get_version(os.path.join(SYS_CPLD_PATH, CPLD_SUB_VER))
    return _firmware_json(ver, sub_ver)


def _get_fcbcpld_info(fcb_num):
    BASE_FCB_DIR = "/sys/class/i2c-dev/i2c-8/device/"
    fcb_path = os.path.join(BASE_FCB_DIR, FCB_NUM_TO_SUBPATH[fcb_num])
    ver = _get_version(os.path.join(fcb_path, CPLD_VER))
    sub_ver = _get_version(os.path.join(fcb_path, CPLD_SUB_VER))
    return _firmware_json(ver, sub_ver)


def get_firmware_info():
    firmware_info = {}
    firmware_info["SYS_CPLD"] = _get_syscpld_info()
    for i in list(FCB_NUM_TO_SUBPATH.keys()):
        firmware_info["FCB{}_CPLD".format(i)] = _get_fcbcpld_info(i)

    return firmware_info


# main for ease of standalone testing
if __name__ == "__main__":
    print("{}".format(get_firmware_info()))
