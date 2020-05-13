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
import os
from abc import abstractmethod

from utils.cit_logger import Logger


class BaseGpioTest(object):

    GPIO_SHADOW_ROOT = "/tmp/gpionames"

    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.gpios = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_gpios(self):
        pass

    def gpio_shadow_path(self, gpioname, attributes):
        self.assertNotEqual(gpioname, None, "GPIO name not set")
        gpio_path = os.path.join(BaseGpioTest.GPIO_SHADOW_ROOT, str(gpioname))
        self.assertTrue(
            os.path.exists(gpio_path), "GPIO missing={}".format(gpioname)
        )

        Logger.info("GPIO name={} present".format(gpioname))
        for item, _value in attributes.items():
            path = os.path.join(gpio_path, str(item))
            self.assertTrue(
                os.path.exists(path),
                "GPIO attribute missing={} attribute={}".format(
                    gpioname, str(item)
                ),
            )

    def test_gpios(self):
        """
        Python's subTest will run the test 'gpio_shadow_path' for every gpio
        """
        self.set_gpios()
        self.assertNotEqual(self.gpios, None, "GPIOs data not set")

        for gpioname, attributes in self.gpios.items():
            with self.subTest(gpioname=gpioname):
                self.gpio_shadow_path(gpioname, attributes)
