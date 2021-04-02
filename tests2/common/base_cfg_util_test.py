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
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BaseCfgUtilTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.bic_cmd = None
        self.bic_info = None
        self.regex = None

    def test_dump_all(self):
        """
        To test cfg-util dump-all
        Output should be match key:value
        Example:
        slot1_por_cfg:lps
        """
        cmd = ["cfg-util", "dump-all"]
        pattern = r"^.*:.*$"
        cli_out = run_cmd(cmd)
        for _out in cli_out.splitlines():
            with self.subTest(_out):
                self.assertIsNotNone(
                    re.match(pattern, _out), "unexpected out: {}".format(_out)
                )

    def test_cfg_util_set(self):
        """
        To test cfg-uti to set slot1_por_cfg value to on then back to original value
        """
        cmd_to_show = ["cfg-util", "slot1_por_cfg"]
        old_value = run_cmd(cmd_to_show).strip()
        try:
            cmd_to_set = ["cfg-util", "slot1_por_cfg", "on"]
            run_cmd(cmd_to_set)
            self.assertTrue(run_cmd(cmd_to_show), "on")
        finally:
            cmd_to_set = ["cfg-util", "slot1_por_cfg", old_value]
            run_cmd(cmd_to_set)
            self.assertTrue(run_cmd(cmd_to_show), old_value)

    def test_cfg_util_show(self):
        """
        To test cfg-uti to show individual key
        """
        cmd = ["cfg-util", "slot1_por_cfg"]
        pattern = r"^.*:.*$"
        cli_out = run_cmd(cmd)
        self.assertIsNone(
            re.match(pattern, cli_out), "unexpected out: {}".format(cli_out)
        )
