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
import os
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


#
# Base class to verify if given GPIO pins can be exported, accessed and
# unexported successfully.
#
class BaseGpioOpsTest(object):

    GPIOCLI = "/usr/local/bin/gpiocli"
    GPIO_SHADOW_ROOT = "/tmp/gpionames"

    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.gpio_lines = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    #
    # "self.gpio_lines" is a list of gpio pins that need to be provided
    # in platform test script.
    # Note: please pick up gpio pins that are NOT being used, because the
    # pins will be exported and unexported by the test script.
    #
    @abstractmethod
    def set_gpio_lines(self):
        pass

    def run_gpiocli(self, action, shadow, chip=None, name=None, offset=None):
        cmd = "%s %s --shadow %s" % (BaseGpioOpsTest.GPIOCLI, action, shadow)

        if chip:
            cmd += " -c %s" % chip
        if name:
            cmd += " -n %s" % name
        if offset:
            cmd += " -o %s" % offset

        return run_shell_cmd(cmd)

    def gpio_do_export(self, gpio_info):
        name = None
        offset = None
        chip = gpio_info["chip"]
        shadow = gpio_info["shadow"]

        if "pin-name" in gpio_info:
            name = gpio_info["pin-name"]
        elif "pin-offset" in gpio_info:
            offset = gpio_info["pin-offset"]
        else:
            self.fail("pin name/offset is missing for %s" % shadow)

        self.run_gpiocli("export", shadow, chip=chip, name=name, offset=offset)

    def gpio_check_value(self, gpio_info):
        shadow = gpio_info["shadow"]

        output = self.run_gpiocli("get-value", shadow)
        val = output.split("=")[1]
        val = val.strip()

        self.assertIn(val, ["0", "1"], "gpio %s: unexpected value %s" % (shadow, val))

    def gpio_check_direction(self, gpio_info):
        shadow = gpio_info["shadow"]

        output = self.run_gpiocli("get-direction", shadow)
        val = output.split("=")[1]
        val = val.strip()

        self.assertIn(
            val, ["in", "out"], "gpio %s: unexpected direction %s" % (shadow, val)
        )

    def gpio_do_unexport(self, gpio_info):
        shadow = gpio_info["shadow"]
        shadow_path = os.path.join(BaseGpioOpsTest.GPIO_SHADOW_ROOT, shadow)
        gpio_path = os.readlink(shadow_path)

        self.run_gpiocli("unexport", shadow)

        self.assertFalse(
            os.path.exists(gpio_path),
            "gpio %s unexport failed: %s still exists" % (shadow, gpio_path),
        )
        self.assertFalse(
            os.path.exists(shadow_path),
            "gpio %s unexport failed: shadow path not destroyed" % shadow,
        )

    def gpio_run_ops(self, gpio_info):
        self.assertIn("shadow", gpio_info, "gpio shadow is missing")
        self.assertIn("chip", gpio_info, "gpio chip is missing")

        self.gpio_do_export(gpio_info)
        self.gpio_check_value(gpio_info)
        self.gpio_check_direction(gpio_info)
        self.gpio_do_unexport(gpio_info)

    def test_gpio_ops(self):
        """
        Python's subTest will run the test 'gpio_shadow_path' for every gpio
        """
        self.set_gpio_lines()
        self.assertIsNotNone(self.gpio_lines, "gpio_lines is not initialized")

        for gpio_info in self.gpio_lines:
            with self.subTest(gpio_info=gpio_info):
                self.gpio_run_ops(gpio_info)
