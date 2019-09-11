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
import os
import sys
import unittest

from utils.cit_logger import Logger


# When tests are discovered upgrader is not yet installed,
# catch the import failure
try:
    # Upgrader and binaries need to be installed in /tmp
    sys.path.append("/tmp/fw-upgrade")
    import fw_json_upgrader as fw_up
except Exception:
    pass


class FwUpgradeTest(unittest.TestCase):
    # <fw_entity: priority>
    _COMPONENTS = {
        "16q_fpga": 1,  # priority=1
        "4dd_fpga": 2,  # priority=2
        "bios": 3,  # priority=3
        "bic": 4,  # priority=4
        "scm": 5,  # priority=5
        "iob_fpga": 6,  # priority=6
        "smb": 7,  # priority=7
        "fcm": 8,  # priority=8
        "pdb": 9,  # priority=9
        "pim_spi_mux": 100,  # priority=100
    }

    def setUp(self):
        FwJsonObj = fw_up.FwJson(os.path.dirname(fw_up.__file__))
        self.json = FwJsonObj.get_priority_ordered_json()
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_fw_entiy_priority_in_ordered_json(self):
        Logger.info("FW Upgrade Ordered json= {}".format(self.json))

        for item, priority in self._COMPONENTS.items():
            # Test for fw entity presence in json
            with self.subTest(upgradable_component=item):
                self.assertIn(
                    item,
                    self.json,
                    "Upgradable component {} missing in ordered json {} ".format(
                        item, self.json
                    ),
                )
            # Test for fw entity priority set in json
            with self.subTest(upgradable_component=item):
                self.assertEqual(
                    priority,
                    self.json.get(item).get("priority"),
                    "Component {} contains priority expected={} found={} ".format(
                        item, priority, self.json.get(item).get("priority")
                    ),
                )
