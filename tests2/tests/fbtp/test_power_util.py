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

from common.base_power_util_test import BasePowerUtilTest

slots = ["mb"]


class PowerUtilTest(BasePowerUtilTest):
    def set_slots(self):
        self.slots = slots

    @unittest.skip("not available")
    def test_12V_slot_off(self):
        pass

    @unittest.skip("not available")
    def test_12V_slot_on(self):
        pass

    @unittest.skip("not available")
    def test_12V_slot_cycle(self):
        pass
