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
import os

from common.base_i2c_driver_presence_test import BaseI2cDriverPresenceTest


class FujiI2cDriverPresenceTest(BaseI2cDriverPresenceTest, unittest.TestCase):
    def set_i2c_driver_list(self):
        self.i2c_driver_list = [
            "xdpe132g5c",
            "mp2975",
            "scmcpld",
            "iobfpga",
            "lm75",
            "tmp421",
            "ucd9000",
            "psu_driver",
            "pca953x",
            "smb_pwrcpld",
            "fcbcpld",
            "smb_syscpld",
            "domfpga",
        ]

        """ To update alternate i2c driver """
        adm1275_path = "/sys/bus/i2c/devices/16-0010"
        lm25066_path = "/sys/bus/i2c/devices/16-0044"
        if os.path.isdir(adm1275_path):
            self.i2c_driver_list.append("adm1275")
        elif os.path.isdir(lm25066_path):
            self.i2c_driver_list.append("lm25066")
