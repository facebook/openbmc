#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class FmcWatchdogTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.fmc_watchdog_control_reg = None

    def tearDown(self):
        Logger.info(f"Finished logging for {self._testMethodName}")

    def set_fmc_watchdog_control_reg(self):
        # Default is AST2600 FMC_WDT2.
        self.fmc_watchdog_control_reg = "0x1E620064"

    def test_fmc_watchdog_disabled(self):
        self.set_fmc_watchdog_control_reg()
        self.assertNotEqual(
            self.fmc_watchdog_control_reg,
            None,
            "FMC watchdog control register not set",
        )
        cmd = f"/sbin/devmem {self.fmc_watchdog_control_reg}"
        cmd_out = run_shell_cmd(cmd)
        control_val = int(cmd_out, 16)
        self.assertEqual(
            control_val & 1,
            0,
            f"FMC watchdog is not disabled. Control register value is {control_val}",
        )
