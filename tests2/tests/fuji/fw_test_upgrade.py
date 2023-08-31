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
import os
import sys
import unittest

from utils.cit_logger import Logger


# When tests are discovered upgrader is not yet installed,
# catch the import failure
try:
    # Upgrader and binaries need to be installed in /tmp
    sys.path.append("/tmp/fw_upgrade")
    import fw_json as fw_up
except Exception:
    pass


class FwUpgradeTest(unittest.TestCase):
    # <fw_entity: priority>
    _COMPONENTS = {
        "iob_fpga": [
            3,
            "/usr/local/bin/spi_util.sh write spi1 IOB_FPGA {filename}",
        ],  # priority=3, upgrade_cmd
        "16q_fpga": [
            1,
            "/usr/local/bin/spi_util.sh write spi1 DOM_FPGA_PIM{entity} {filename}",
        ],  # priority=1, upgrade_cmd
        "bios": [
            5,
            "/usr/bin/fw-util scm --update bios {filename}",
        ],  # priority=5, upgrade_cmd
        "bic": [
            4,
            "/usr/bin/fw-util scm --update bic {filename}",
        ],  # priority=4, upgrade_cmd
        "scm": [
            6,
            "/usr/local/bin/cpld_update.sh -s SCM -f {filename} sw",
        ],  # priority=6, upgrade_cmd
        "smb": [
            7,
            "/usr/local/bin/cpld_update.sh -s SMB -f {filename} sw",
        ],  # priority=7, upgrade_cmd
        "fcm": [
            8,
            "/usr/local/bin/cpld_update.sh -s {entity} -f {filename} sw",
        ],  # priority=8, upgrade_cmd
        "pdb": [
            9,
            "/usr/local/bin/cpld_update.sh -s {entity} -f {filename} sw",
        ],  # priority=9, upgrade_cmd
    }

    def setUp(self):
        FwJsonObj = fw_up.FwJson(os.path.dirname(fw_up.__file__))
        self.json = FwJsonObj.get_priority_ordered_json()
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_fw_entiy_component_cmd_in_ordered_json(self):
        Logger.info("FW Upgrade Ordered json= {}".format(self.json))
        for item, _x in self._COMPONENTS.items():
            # Test for fw entity presence in json
            with self.subTest(upgradable_component=item):
                self.assertIn(
                    item,
                    self.json,
                    "Upgradable component {} missing in ordered json {} ".format(
                        item, self.json
                    ),
                )

    def test_fw_entiy_priority_in_ordered_json(self):
        Logger.info("FW Upgrade Ordered json= {}".format(self.json))
        for item, attributes in self._COMPONENTS.items():
            # Test for fw entity priority set in json
            with self.subTest(upgradable_component=item):
                self.assertEqual(
                    attributes[0],
                    self.json.get(item).get("priority"),
                    "Component {} contains priority expected={} found={} ".format(
                        item, attributes[0], self.json.get(item).get("priority")
                    ),
                )

    def test_fw_entiy_upgrade_cmd_in_ordered_json(self):
        Logger.info("FW Upgrade Ordered json= {}".format(self.json))
        for item, attributes in self._COMPONENTS.items():
            # Test for correct command in json
            with self.subTest(upgradable_component=item):
                self.assertEqual(
                    attributes[1],
                    self.json.get(item).get("upgrade_cmd"),
                    "Component {} contains upgrade_cmd expected={} found={} ".format(
                        item, attributes[1], self.json.get(item).get("upgrade_cmd")
                    ),
                )
