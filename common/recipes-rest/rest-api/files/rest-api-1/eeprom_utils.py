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

import subprocess
import bmc_command


def _parse_eeprom_data(output):
    eeprom_data = {}
    output = output.decode('utf8')
    for line in output.splitlines():
        parts = line.split(':', 1)
        if len(parts) != 2:
            # This generally shouldn't happen, but it's possible there was some
            # error message logged on the console in the middle of the w|seutil
            # output.
            continue
        key = parts[0].strip()
        value = parts[1].strip()
        eeprom_data[key] = value
    return eeprom_data


def get_eeprom_data(util):
    proc = subprocess.Popen([util],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    data, err = bmc_command.timed_communicate(proc)
    return _parse_eeprom_data(data)
