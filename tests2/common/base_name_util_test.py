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


import json
from unittest import TestCase

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BaseNameUtilTest(TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def testNameUtil(self):
        """
        test if name-util and verify if header exist in the output
        """
        cmd = ["name-util", "all"]
        header = ["version", "server_fru", "nic_fru", "bmc_fru"]
        out = run_cmd(cmd)
        try:
            out = json.loads(out)
            for h in header:
                self.assertIn(h, out.keys(), "{} not found in {}".format(h, header))
        except Exception:
            self.assertTrue(False, "non json output: {}".format(out))
