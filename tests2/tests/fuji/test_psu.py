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
from tests.fuji.helper.libpal import pal_is_fru_prsnt, pal_get_fru_id
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Psu1Test(CommonPsuTest, unittest.TestCase):
    def set_psu_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("psu1")):
            self.skipTest("psu1 is not present")
        self.psu_cmd = "/usr/bin/psu-util"
        self.psu_id = 1


class Psu2Test(Psu1Test, unittest.TestCase):
    def set_psu_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("psu2")):
            self.skipTest("psu2 is not present")
        self.psu_cmd = "/usr/bin/psu-util"
        self.psu_id = 2


class Psu3Test(Psu1Test, unittest.TestCase):
    def set_psu_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("psu3")):
            self.skipTest("psu3 is not present")
        self.psu_cmd = "/usr/bin/psu-util"
        self.psu_id = 3


class Psu4Test(Psu1Test, unittest.TestCase):
    def set_psu_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("psu4")):
            self.skipTest("psu4 is not present")
        self.psu_cmd = "/usr/bin/psu-util"
        self.psu_id = 4
