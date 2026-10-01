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
from tests.fby3.board_class import get_board_class, select_by_class
from utils.test_utils import qemu_check


slots = ["slot1", "slot2", "slot3", "slot4"]

# A class 2 single-host server has exactly one slot.
SLOTS_BY_CLASS = {
    1: slots,
    2: ["slot1"],
}


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class PowerUtilTest(BasePowerUtilTest):
    def set_slots(self):
        self.slots = select_by_class(SLOTS_BY_CLASS, slots)

    @unittest.skip("FIXME for HW CIT T163968017")
    def test_slot_reset(self):
        super.test_slot_reset()

    @unittest.skipIf(
        get_board_class() == 2,
        "class 2 12V-cycle takes the BMC down with the sled, killing this test",
    )
    def test_12V_slot_cycle(self):
        # On class 1 the BMC sits on the baseboard and survives a slot 12V
        # cycle. On class 2 pal_set_server_power() routes to bic_do_12V_cycle(),
        # which asks the baseboard BIC to drop 12V for the whole sled -- and
        # the BMC is powered from that rail, so it reboots mid-test. The CIT
        # symptom is cit_runner returning -1 with no verdict and no stderr,
        # because the connection the test ran over went away.
        super().test_12V_slot_cycle()
