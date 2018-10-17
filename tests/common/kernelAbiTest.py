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

#
# This file aims at detecting and reporting kernel ABI changes which
# may be caused by kernel upgrade, defconfig update, and etc.
#

from __future__ import print_function
import os
import sys

# sysfs dependency list
sysfs_dep_list = [
    '/sys/devices',
    '/sys/class/hwmon',
    '/sys/class/gpio',
    '/sys/class/gpio/export',
    '/sys/bus/i2c/devices',
    '/sys/bus/i2c/drivers',
]

# devfs dependency list
devfs_dep_list = [
    '/dev/mem',
    '/dev/watchdog',
    '/dev/mtd0',
    '/dev/mtdblock0',
    '/dev/ttyS0',
]

# procfs dependency list
procfs_dep_list = [
    '/proc/mtd',
]

if __name__ == "__main__":
    """launch all tests in the file.
    """
    total = 0
    error = 0
    all_files = sysfs_dep_list + devfs_dep_list + procfs_dep_list
    for pathname in all_files:
        total += 1
        if not os.path.exists(pathname):
            print('Error: <%s> file not found' % pathname)
            error += 1

    if error == 0:
        print('kernel ABI test [PASSED]')
        sys.exit(0)
    else:
        print('kernel ABI test [FAILED]: %d out of %d tests failed' %
              (error, total))
        sys.exit(1)
