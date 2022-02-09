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
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class OobEepromTest(unittest.TestCase):
    def setUp(self):
        self.eeprom_util = "/usr/local/bin/oob-eeprom-util.sh"
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_eeprom_read(self):
        data = run_shell_cmd("{} read 0x0".format(self.eeprom_util)).split("\n")
        self.assertIn("0x0: 0xa8", data[0], "OOB EEPROM magic code missing")
        data = run_shell_cmd("{} read 0x3ff".format(self.eeprom_util)).split("\n")
        m = re.match(r"0x3ff: (0x[0-9a-f]+)", data[0])
        self.assertIsNotNone(m, "Could not parse version word: {}".format(data[0]))
        version = int(m.group(1), 0)
        self.assertGreater(version, 0, "Invalid version: {}".format(version))
