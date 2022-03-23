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


class Agc032AI2cDriverPresenceTest(BaseI2cDriverPresenceTest, unittest.TestCase):
    def set_i2c_driver_list(self):
        self.i2c_driver_list = [
            "adm1275",
            "at24",
            "bmp280",
            "dps310",
            "dps_driver",
            "dummy",
            "max1363",
            "ibm-cffps",
            "lm75",
            "occ-hwmon",
            "pca953x",
            "psu_driver",
            "leds-pca955x",
            "swpld_1",
            "ucd9000",
            "rtc-pcf8523",
            "max31785",
            "pmbus",
        ]
