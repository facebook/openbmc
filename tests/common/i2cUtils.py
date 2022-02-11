#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

from __future__ import print_function
from __future__ import unicode_literals
import sys
import subprocess

def str_chunk(input_str, width):
    """divide string to chunks with fixed width/size.
    """
    return (input_str[0+i:width+i] for i in range(0, len(input_str), width))

def str_is_hex(input_str):
    """check if given string is hex digits.
    """
    try:
        int(input_str, 16)
        return True
    except:
        return False

class I2cDetectTool:
    """class to run i2cdetect and parse command output.
    """
    _bus_num = None # i2c bus to be detected

    # "_probed_devices" contains the addresses of devices which are
    # successfully probed by "i2cdetect".
    _probed_devices = []

    # "_skipped_devices" contains the addresses of devices which are
    # skipped by "i2cdetect" because the devices are in use by drivers.
    # Per "i2cdetect" manual page, this strongly suggests that there is
    # a chip/device at this address.
    _skipped_devices = []

    def _parse_i2cdetect_output(self, cmd_output):
        """parse "i2cdetect" command output.
        """
        cmd_output = cmd_output.decode('utf-8')
        lines = cmd_output.splitlines()
        lines.pop(0) # skip first line

        start_addr = 0
        for line in lines:
            entries = list(str_chunk(line, 3))
            entries.pop(0) # skip first column

            offset = 0
            for entry in entries:
                entry = entry.strip()
                if entry == 'UU':
                    self._skipped_devices.append(start_addr + offset)
                elif str_is_hex(entry):
                    self._probed_devices.append(start_addr + offset)
                offset += 1
            start_addr += 16

    def __init__(self, bus_num, first_addr=None, last_addr=None):
        """Create an instance of I2cDetectTool.
        """
        self._bus_num = bus_num

        cmd = 'i2cdetect -y ' + str(bus_num)
        f = subprocess.Popen(cmd,
                             shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        info, err = f.communicate()
        if len(err) != 0:
            raise Exception(err)

        self._parse_i2cdetect_output(info)

    def list_probed_devices(self):
        """Returns the list (addresses) of found i2c devices.
        """
        return self._probed_devices

    def list_skipped_devices(self):
        """Returns the list (addresses) of skipped i2c devices.
        """
        return self._skipped_devices

    def is_device_present(self, i2c_addr):
        return (i2c_addr in self._probed_devices or
                i2c_addr in self._skipped_devices)

if __name__ == "__main__":
    """unit test of classes/functions in the file.
    """

    if len(sys.argv) != 3:
        print('Error: invalid command line arguments!')
        print('Usage: %s <i2c-bus-num> <dev-addr-hex>\n' % sys.argv[0])
        sys.exit(1)

    bus_num = int(sys.argv[1])
    dev_addr = int(sys.argv[2], 16)
    bus_info = I2cDetectTool(bus_num)

    # Dump probed devices if any
    probed = bus_info.list_probed_devices()
    if len(probed) == 0:
        print('no devices probed on i2c bus %d' % bus_num)
    else:
        print('%d devices probed on i2c bus %d:' % (len(probed), bus_num))
        for item in probed:
            print('  - 0x%x' % item)

    # Dump skipped devices if any
    skipped = bus_info.list_skipped_devices()
    if len(skipped) == 0:
        print('no devices skipped on i2c bus %d' % bus_num)
    else:
        print('%d devices skipped on i2c bus %d:' % (len(skipped), bus_num))
        for item in skipped:
            print('  - 0x%x' % item)

    # Test if device is present at given address
    if (bus_info.is_device_present(dev_addr)):
        print('i2c bus %d, address 0x%x: device detected' %
              (bus_num, dev_addr))
    else:
        print('i2c bus %d, address 0x%x: no device present' %
              (bus_num, dev_addr))
