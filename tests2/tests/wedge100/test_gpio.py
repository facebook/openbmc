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
import unittest

from common.base_gpio_test import BaseGpioTest
from tests.wedge100.test_data.gpio.gpio import GPIOS
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class GpioTest(BaseGpioTest, unittest.TestCase):
    def set_gpios(self):
        self.gpios = GPIOS

    def test_romcs1(self):
        rom_cmd = "/usr/local/bin/openbmc_gpio_util.py config ROMCS1#"
        Logger.info("Executing cmd={}".format(rom_cmd))
        info = run_shell_cmd(cmd=rom_cmd)
        self.assertIn(
            'Function "ROMCS1#" is set', info, "ROMCS1 gpio not setup properly"
        )
