#!/usr/bin/env python3
#
# Copyright 2025-present Facebook. All Rights Reserved.
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
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


def flashrom_supported_devices():
    flashrom_cmd = "flashrom -L"
    return run_shell_cmd(flashrom_cmd)


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FlashromSupportedDevicesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.flashrom_output = flashrom_supported_devices()

    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def test_devices_supported(self):
        expected_devices = ["MX25U25635F", "MT25QU256", "W25Q256JW_DTR", "GD25LQ256D"]

        for device in expected_devices:
            device_supported_match = re.search(
                r"{}\s+".format(device), self.flashrom_output
            )
            self.assertTrue(
                device_supported_match, "{} flash device not supported".format(device)
            )
