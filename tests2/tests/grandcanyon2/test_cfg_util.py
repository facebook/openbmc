#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

from common.base_cfg_util_test import BaseCfgUtilTest
from utils.shell_util import run_cmd


class CfgUtilTest(BaseCfgUtilTest):
    def test_cfg_util_set(self):
        """
        To test cfg-uti to set server_por_cfg value to on then back to original value
        """
        cmd_to_show = ["cfg-util", "server_por_cfg"]
        old_value = run_cmd(cmd_to_show).strip()
        try:
            cmd_to_set = ["cfg-util", "server_por_cfg", "on"]
            run_cmd(cmd_to_set)
            self.assertTrue(run_cmd(cmd_to_show), "on")
        finally:
            cmd_to_set = ["cfg-util", "server_por_cfg", old_value]
            run_cmd(cmd_to_set)
            self.assertTrue(run_cmd(cmd_to_show), old_value)

    def test_cfg_util_show(self):
        """
        To test cfg-uti to show individual key
        """
        cmd = ["cfg-util", "server_por_cfg"]
        pattern = r"^.*:.*$"
        cli_out = run_cmd(cmd)
        self.assertIsNone(
            re.match(pattern, cli_out), "unexpected out: {}".format(cli_out)
        )
