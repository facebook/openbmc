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

from common.base_dimm_util_test import BaseDimmUtilTest
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class AllDimmUtilTest(BaseDimmUtilTest, unittest.TestCase):
    FRU = "all"

    @unittest.skip("not available")
    def testSerialJson(self):
        """
        by pass serial json test if FRU == "all"
        since its output can't be load into JSON
        """
        pass


class Slot1DimmUtilTest(AllDimmUtilTest):
    FRU = "slot1"


class Slot2DimmUtilTest(AllDimmUtilTest):
    FRU = "slot2"


class Slot3DimmUtilTest(AllDimmUtilTest):
    FRU = "slot3"


class Slot4DimmUtilTest(AllDimmUtilTest):
    FRU = "slot4"
