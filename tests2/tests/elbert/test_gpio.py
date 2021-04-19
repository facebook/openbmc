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
import unittest

from common.base_gpio_test import BaseGpioTest
from tests.elbert.test_data.gpio.gpio import GPIOS
from tests.elbert.test_data.gpio.gpio import pim_gpios
from utils.shell_util import run_shell_cmd


class GpioTest(BaseGpioTest, unittest.TestCase):
    def set_gpios(self):
        self.gpios = GPIOS

        # Add PIM2-9 GPIOs for available pims
        for pim in range(2, 9 + 1):
            cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/pim{}_present"
            if "0x1" in run_shell_cmd(cmd.format(pim)):
                # Create GPIOS for inserted PIMs only
                self.gpios.update(pim_gpios(pim))
