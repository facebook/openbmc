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
from tests.wedge400.helper.libpal import BoardRevision, pal_get_board_type
from tests.wedge400.test_data.gpio.gpio import (
    GPIOS_COMMON,
    GPIOS_W400,
    GPIOS_W400C,
)


class GpioTest(BaseGpioTest, unittest.TestCase):
    def set_gpios(self):
        self.gpios = GPIOS_COMMON
        platform_type = pal_get_board_type()
        Wedge400_type = BoardRevision.board_type.get(
            BoardRevision.BRD_TYPE_WEDGE400, ""
        )
        Wedge400C_type = BoardRevision.board_type.get(
            BoardRevision.BRD_TYPE_WEDGE400C, ""
        )

        if platform_type == Wedge400_type:
            self.gpios.update(GPIOS_W400)
        elif platform_type == Wedge400C_type:
            self.gpios.update(GPIOS_W400C)
        else:
            self.skipTest("Skip test on {} board".format(platform_type))
