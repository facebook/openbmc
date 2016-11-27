#!/usr/bin/env python
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

def _get_version(version_file):
    with open(version_file) as f:
        return f.read()


def _firmware_json(ver, sub_ver, int_base=16):
    return {
        'version' : int(ver, int_base),
        'sub_version' : int(sub_ver, int_base)
    }


def _get_syscpld_info():
    SYS_CPLD_PATH = '/sys/class/i2c-dev/i2c-13/device/13-003e'
    ver = _get_version(os.path.join(SYS_CPLD_PATH, 'cpld_ver'))
    sub_ver = _get_version(os.path.join(SYS_CPLD_PATH, 'cpld_sub_ver'))
    return _firmware_json(ver, sub_ver)


def get_firmware_info():
    return {
        'SYS_CPLD': _get_syscpld_info(),
    }


# main for ease of standalone testing
if __name__ == '__main__':
    print('{}'.format(get_firmware_info()))
