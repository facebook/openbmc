#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


class OobEepromTest(unittest.TestCase):
    def setUp(self):
        self.eeprom_util = "/usr/local/bin/oob-eeprom-util.sh"
        self.img_file_0 = "/tmp/test_oob_eeprom_0.bin"
        self.img_file_1 = "/tmp/test_oob_eeprom_1.bin"
        self.file = None
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        if self.file is not None:
            self.file.close()
        run_shell_cmd("rm -rf {}".format(self.img_file_0))
        run_shell_cmd("rm -rf {}".format(self.img_file_1))
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_eeprom(self):
        # read
        run_shell_cmd("{} read {}".format(self.eeprom_util, self.img_file_0),
            ignore_err=True)
        self.assertEqual(os.path.isfile(self.img_file_0), True,
            "oob-eeprom-util read 1 failed")
        run_shell_cmd("{} read {}".format(self.eeprom_util, self.img_file_1),
            ignore_err=True)
        self.assertEqual(os.path.isfile(self.img_file_1), True,
            "oob-eeprom-util read 2 failed")
        diff = run_shell_cmd("diff {} {}".format(self.img_file_0, self.img_file_1))
        self.assertEqual(diff, "", "oob-eeprom-util reads are not equal")

        # Endianness
        self.file = open(self.img_file_0, "rb")
        self.assertIsNotNone(self.file, "Failed to open image file")
        endian = self.file.read(4)
        self.assertEqual(endian[2:], b"\xff\x01", "Unable to verify big endianness")

        # Version
        data = run_shell_cmd("{} version".format(self.eeprom_util), ignore_err=True)
        m = re.match(r"Version: (\d+\.\d+)", data)
        self.assertIsNotNone(m, "Could not parse version word")
        version = float(m.group(1))
        self.assertGreater(version, 0.0, "Invalid version: {}".format(version))

