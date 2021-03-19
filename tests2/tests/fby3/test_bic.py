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

from common.base_bic_test import CommonBicTest

slots = ["slot1", "slot2", "slot3", "slot4"]


class BicTest(CommonBicTest, unittest.TestCase):
    def set_bic_cmd(self):
        pass

    def test_bic_dev_id(self):
        for slot in slots:
            with self.subTest(slot=slot):
                self.bic_cmd = "/usr/bin/bic-util {}".format(slot)
                super().test_bic_dev_id()

    def test_bic_gpio(self):
        for slot in slots:
            with self.subTest(slot=slot):
                self.bic_cmd = "/usr/bin/bic-util {}".format(slot)
                super().test_bic_gpio()

    def test_bic_gpio_config(self):
        for slot in slots:
            with self.subTest(slot=slot):
                self.bic_cmd = "/usr/bin/bic-util {}".format(slot)
                super().test_bic_gpio_config()

    @unittest.skip("test not available on platform")
    def test_bic_config(self):
        pass

    def test_bic_post_code(self):
        for slot in slots:
            with self.subTest(slot=slot):
                self.bic_cmd = "/usr/bin/bic-util {}".format(slot)
                super().test_bic_post_code()

    def test_bic_sdr(self):
        regex = r"((^Server Board:.*)|(^(.+:\s([0-9]+|((0x|0X)[0-9a-zA-Z]+)),)\s*))"
        for slot in slots:
            with self.subTest(slot=slot):
                self.bic_cmd = "/usr/bin/bic-util {}".format(slot)
                super().test_bic_sdr(regex=regex)

    def test_bic_sensor(self):
        regex = r"((^Server Board:.*)|(^(.+:\s([0-9]+|((0x|0X)[0-9a-zA-Z]+)),)\s*))"
        for slot in slots:
            with self.subTest(slot=slot):
                self.bic_cmd = "/usr/bin/bic-util {}".format(slot)
                super().test_bic_sensor(regex=regex)

    @unittest.skip("test not available on platform")
    def test_bic_fruid(self):
        pass

    @unittest.skip("test not available on platform")
    def test_bic_mac(self):
        pass
