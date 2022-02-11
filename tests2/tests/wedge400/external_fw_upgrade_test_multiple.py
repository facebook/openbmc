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

from tests.wedge400.test_data.firmware_upgrade.firmware_upgrade_config import (
    FwUpgradeTest,
)


class CollectiveFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Collective test for all components which is found in json file
    """

    def test_collective_firmware_upgrade(self):
        """
            This test file will enable us to do all the upgrade and
            one power cycle at the end. While this has its advantages,
            it makes it impossible for us to catch which specific FW
            break a device in cases a box don't come up after upgrade.
            Therefore, we will skip this test for now. Please comment
            out the skipTest line and uncomment the line of code of
            that come right before pass to activate this test.
        """
        self.skipTest("collective upgrade with 1 power cycle skipped")
        # super().do_external_firmware_upgrade()
        pass
