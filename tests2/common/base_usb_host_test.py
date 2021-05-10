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

from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


#
# "BaseUSBHostTest" aims at testing if USB Host Controllers (and downstream
# devices if any) are initialized properly.
#
class BaseUSBHostTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    #
    # Each platform needs to initialize "self.usb_devices" in below method.
    #
    @abstractmethod
    def set_usb_devices(self):
        pass

    def test_usb_devices(self):
        self.set_usb_devices()
        self.assertNotEqual(
            self.usb_devices, None, "Golden usb device list not specified"
        )

        all_udevs = run_shell_cmd(cmd=["lsusb"])

        for udev in self.usb_devices:
            with self.subTest(udev=udev):
                self.assertIn(
                    udev, all_udevs, "USB device {} not installed".format(udev)
                )
