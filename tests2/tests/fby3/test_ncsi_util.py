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

from common.base_ncsi_util_test import CommonNcsiUtilTest, Requirement, Comparison
from utils.test_utils import qemu_check

# should we check for specific FRUs here or anything?


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class NcsiUtilTest(CommonNcsiUtilTest, unittest.TestCase):
    def test_ncsi_util(self):
        # Here is where we should set Requirements once those are defined for Yv3.

        print("in test_ncsi_util for fby3")

        self.adapter_fields = [
            Requirement("Total Bytes Received", 0, Comparison.GREATER_THAN),
            Requirement("Total Bytes Transmitted", 0, Comparison.GREATER_THAN),
            Requirement("FCS Receive Errors", 1, Comparison.LESS_THAN_OR_EQUAL),
            Requirement("Alignment Errors", 1, Comparison.LESS_THAN_OR_EQUAL),
        ]

        super().test_ncsi_util()
