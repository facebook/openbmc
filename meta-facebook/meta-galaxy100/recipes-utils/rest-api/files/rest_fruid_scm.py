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


def get_fruid_scm():
    weutil_data = eeprom_utils.get_eeprom_data("weutil")
    seutil_data = eeprom_utils.get_eeprom_data("seutil")

    # Get location and BMC SN info from weutil, put in seutil data
    seutil_data["Location on Fabric"] = weutil_data["Location on Fabric"]
    seutil_data["BMC Serial"] = weutil_data["Product Serial Number"]

    return seutil_data
