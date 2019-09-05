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
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class FwFpgaVersionTest(unittest.TestCase):
    def setUp(self):
        self.fpga_version = "/usr/local/bin/fpga_ver.sh"
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_fw_sup_fpga_revision_format(self):
        data = run_shell_cmd(self.fpga_version).split("\n")
        self.assertIn("SUP_FPGA", data[1], "SUP_FPGA missing received={}".format(data))

        value = data[1].split(":")[1]
        self.assertIn("3.8", value, "SUP_FPGA value missing received={}".format(value))
