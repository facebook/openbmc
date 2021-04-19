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
import unittest

from common.base_switch_test import BaseSwitchTest
from tests.wedge400.helper.libpal import pal_get_board_type


class GibraltarTest(BaseSwitchTest, unittest.TestCase):
    """
    Test for smb_syscpld node gb_turn_on
    """

    def set_swtich_node_cmd(self):
        brd_type = pal_get_board_type()
        if brd_type == "Wedge400":
            self.path = "/sys/bus/i2c/drivers/smb_syscpld/12-003e/th3_turn_on"
        elif brd_type == "Wedge400C":
            self.path = "/sys/bus/i2c/drivers/smb_syscpld/12-003e/gb_turn_on"
        else:
            self.assertTrue(brd_type, "Detect board type fail, {} ".format(brd_type))
