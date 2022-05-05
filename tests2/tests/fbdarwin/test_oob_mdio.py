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
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


class OobMdioTest(unittest.TestCase):
    def setUp(self):
        self.mdio_util = "/usr/local/bin/oob-mdio-util.sh"
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_mdio_read(self):
        # Test the IMP port state register
        data = run_shell_cmd("{} read8 0x0 0xe".format(self.mdio_util)).split("\n")
        self.assertIn(
            "0x0/0xe 0x8b", data[0], "unexpected OOB MDIO IMP port state value"
        )

        # Test writing/reading to unused port-2
        run_shell_cmd("{} write8 0x0 0x2 0x3".format(self.mdio_util))
        data = run_shell_cmd("{} read8 0x0 0x2".format(self.mdio_util)).split("\n")
        self.assertIn("0x0/0x2 0x3", data[0], "unexpected OOB MDIO port 2 state value")
        run_shell_cmd("{} write8 0x0 0x2 0x0".format(self.mdio_util))
        data = run_shell_cmd("{} read8 0x0 0x2".format(self.mdio_util)).split("\n")
        self.assertIn("0x0/0x2 0x0", data[0], "unexpected OOB MDIO port 2 state value")

        # Validate 64-bit counter register. Only validate the format, not the value itself.
        data = run_shell_cmd("{} read64 0x28 0x0".format(self.mdio_util)).split("\n")
        m = re.match(r"0x28/0x0 \S+ ==( [01]{4}){16}", data[0])
        self.assertIsNotNone(m, "Failed to retrieve OOB MDIO 64-bit TX counter")
