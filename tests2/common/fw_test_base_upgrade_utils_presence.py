#!/usr/bin/env python
#
# Copyright 2019-present Facebook. All Rights Reserved.
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


from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseFwUpgradeUtilsPresenceTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.fw_upgrade_utils_cmd = None
        self.expected_fw_upgrade_utils = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_fw_upgrade_utils_cmd(self):
        self.fw_upgrade_utils_cmd = ["ls /usr/local/bin && ls /usr/bin"]

    @abstractmethod
    def set_fw_upgrade_utils(self):
        pass

    def get_fw_upgrade_utils(self):
        self.set_fw_upgrade_utils_cmd()
        self.assertNotEqual(
            self.fw_upgrade_utils_cmd,
            None,
            "Command to get fw upgrade utils is not set",
        )
        Logger.info("Executing cmd={}".format(self.fw_upgrade_utils_cmd))
        info = run_shell_cmd(cmd=self.fw_upgrade_utils_cmd)
        return info

    def test_fw_upgrade_utils(self):
        self.set_fw_upgrade_utils()
        info = self.get_fw_upgrade_utils()
        self.assertNotEqual(
            self.expected_fw_upgrade_utils,
            None,
            "Expected set of processes data not set",
        )

        for fw_upgrade_util in self.expected_fw_upgrade_utils:
            with self.subTest(fw_upgrade_util=fw_upgrade_util):
                self.assertIn(
                    fw_upgrade_util,
                    info,
                    "Switch has {} missing".format(fw_upgrade_util),
                )
