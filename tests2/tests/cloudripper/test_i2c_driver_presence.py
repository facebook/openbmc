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

from common.base_i2c_driver_presence_test import BaseI2cDriverPresenceTest


class CloudripperI2cDriverPresenceTest(BaseI2cDriverPresenceTest, unittest.TestCase):
    def set_i2c_driver_list(self):
        self.i2c_driver_list = [
            "ir35215",
            "xdpe12284",
            "powr1220",
            "xdpe12284",
            "pxe1610",
            "adm1275",
            "lm75",
            "tmp421",
            "net_asic",
            "pca953x",
            "domfpga",
            "psu_driver",
            "smb_pwrcpld",
            "smb_syscpld",
            "scmcpld",
            "fcbcpld",
        ]
