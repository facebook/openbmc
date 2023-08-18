#!/usr/bin/python
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

import subprocess
import re
import sys
from functools import reduce

CMD = "weutil -e chassis_eeprom"
REGEX = r"Product Serial Number: (\S+)"


def run_shell_cmd(cmd=None, ignore_err=False, expected_return_code=0):
    if not cmd:
        raise Exception("cmd not set")
    try:

        f = subprocess.Popen(
            cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        data, err = f.communicate()
        if not ignore_err:
            err = err.decode("utf-8")
            if len(err) > 0:
                raise Exception(err + " [FAILED]")
        if f.returncode != expected_return_code:
            raise Exception(
                "{} exited with non {} exit code".format(cmd, expected_return_code)
            )
        info = data.decode("utf-8")
    except Exception as e:
        raise Exception("Failed to run command = {} and exception {}".format(cmd, e))
    return info


info = run_shell_cmd(CMD)
match = re.search(REGEX, info)

serial = ""
if match:
    serial = match[1]
else:
    print("cannot find platform serial number")
    sys.exit(1)

#
# At boot time, the OpenBMC should print a special string to the serial console used to
# automatically detect the location and port of the console server the device plugs into. The
# string begins with an exclamation mark, followed by the magic string “serfmon” and 3
# fields separated by colons - serial length, serial number, and checksum,
#
# Both serial number length and checksum are printed as a decimal number.
# Checksum is calculated by bitwise XORing all characters of the serial number together.
# Serial number refers to main serial number of this device, not the BMC’s serial number

checksum = reduce(lambda a, b: a ^ ord(b), serial, 0)
text = "!serfmon:{0}:{1}:{2}".format(len(serial), serial, checksum)
print(text)
run_shell_cmd("echo {} > /dev/console".format(text))
