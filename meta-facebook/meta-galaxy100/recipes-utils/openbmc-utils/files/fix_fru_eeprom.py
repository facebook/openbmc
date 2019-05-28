#!/usr/bin/env python3
#
# Copyright 2004-present Facebook. All rights reserved.
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

import os


EEPROMS = {
    "Main board": "/sys/class/i2c-adapter/i2c-6/6-0051/eeprom",
    "SCM": "/sys/class/i2c-adapter/i2c-0/0-0052/eeprom",
}


if __name__ == "__main__":
    for name, path in list(EEPROMS.items()):
        print("Fixing %s EEPROM at %s..." % (name, path))
        if not os.path.isfile(path):
            print("%s does not exist. Skip" % path)
            continue
        try:
            with open(path, "rb+") as f:
                # fix the version #
                f.seek(2)
                v = f.read(1)
                if v == b"\x02":
                    print("%s EEPROM has been fixed. Skip" % name)
                    continue
                f.seek(2)
                f.write(b"\x02")
            print("%s EEPROM is fixed" % name)
        except Exception:
            print("Failed to fix %s EEPROM" % name)
