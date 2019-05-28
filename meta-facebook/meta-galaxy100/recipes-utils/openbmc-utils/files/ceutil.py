#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

import json
import subprocess


def _ceutil():
    proc = subprocess.Popen(
        [
            "/usr/bin/wget",
            "--timeout",
            "5",
            "-q",
            "-O",
            "-",
            "http://[fe80::c1:1%eth0.4088]:8080/api/sys/mb/chassis_eeprom",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        data, err = proc.communicate()
        data = data.decode()
        err = err.decode()
        if proc.returncode:
            raise Exception("Command failed, returncode: {}".format(proc.returncode))
    except Exception as e:
        print("Error getting chassis eeprom from CMM {}".format(e))
        raise

    for key, val in list(json.loads(data).items()):
        print("{}: {}".format(key, val))


if __name__ == "__main__":
    _ceutil()
