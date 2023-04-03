#!/usr/bin/env python3
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

import unittest

from common.base_sysfs_test import BaseSysfsTest


class SCMCPLDSysfsTest(BaseSysfsTest, unittest.TestCase):
    def setPath(self):
        self.path = "/sys/bus/i2c/drivers/scmcpld/2-0035"

    def setFilesToTest(self):
        self.files_to_test = ["cpld_ver", "cpld_sub_ver", "board_ver"]
