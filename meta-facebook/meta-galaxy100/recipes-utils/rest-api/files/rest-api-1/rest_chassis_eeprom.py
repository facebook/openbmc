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

import eeprom_utils


def get_chassis_eeprom():
    return eeprom_utils.get_eeprom_data("/usr/local/bin/ceutil")


# For ease of debugging
if __name__ == "__main__":
    print(get_chassis_eeprom())
