#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
from common.base_psu_test import CommonPsuTest


class Psu1Test(CommonPsuTest, unittest.TestCase):
    def set_psu_cmd(self):
        self.psu_cmd = "/usr/bin/psu-util"
        self.psu_id = 1


class Psu2Test(CommonPsuTest, unittest.TestCase):
    def set_psu_cmd(self):
        self.psu_cmd = "/usr/bin/psu-util"
        self.psu_id = 2
