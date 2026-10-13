#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
from tests.fby3.board_class import select_by_class
from tests.fby3.test_data.gpio.gpio import GPIOS, GPIOS_BY_CLASS
from utils.test_utils import qemu_check


class GpioTest(BaseGpioTest, unittest.TestCase):
    def set_gpios(self):
        # A class 2 single-host server has one slot and no baseboard, so none
        # of the per-slot shadows exist there.
        self.gpios = select_by_class(GPIOS_BY_CLASS, GPIOS)

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_gpios(self):
        super().test_gpios()
