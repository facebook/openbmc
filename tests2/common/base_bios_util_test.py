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
import re
import unittest
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_cmd
from utils.test_utils import check_fru_availability


class BaseBiosUtilTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.set_fru_list()

    @abstractmethod
    def set_fru_list(self):
        pass

    def test_boot_order_get(self):
        """
        Test CLI: bios-util server --boot_order get --boot_order
        """
        boot_options = [
            "USB Device",
            "IPv4 Network",
            "SATA HDD",
            "SATA-CDROM",
            "Other Removable Device",
            "IPv6 Network",
            "Reserved",
        ]
        for fru in self.fru_list:
            with self.subTest(fru=fru):
                if not check_fru_availability(fru):
                    self.skipTest("skip test due to {} not available".format(fru))
                cmd = ["/usr/local/bin/bios-util", fru, "--boot_order", "get", "--boot_order"]
                pattern = r"Boot Order: ([a-zA-Z0-9,\s\-]+?)\n"
                out = run_cmd(cmd)
                m = re.search(pattern, out)
                self.assertIsNotNone(m, "unexpected output: {}".format(out))
                boot_orders = m.group(1).split(",")
                for opt in boot_orders:
                    self.assertIn(
                        opt.strip(),
                        boot_options,
                        "{} not found in boot_options".format(opt),
                    )

    def checkBootOrderStatus(self, fru, opt, status):
        cmd = ["/usr/local/bin/bios-util", fru, "--boot_order", "get", opt]
        out = run_cmd(cmd)
        pattern = r"^.*: (.*?)\n$"
        m = re.search(pattern, out)
        self.assertIsNotNone(m, "unexpect output {}".format(out))
        self.assertEqual(m.group(1), status)

    def disableBootOrder(self, fru, opt):
        cmd = ["/usr/local/bin/bios-util", fru, "--boot_order", "disable", opt]
        run_cmd(cmd)
        self.checkBootOrderStatus(fru, opt, "Disabled")

    def enableBootOrder(self, fru, opt):
        cmd = ["/usr/local/bin/bios-util", fru, "--boot_order", "enable", opt]
        run_cmd(cmd)
        self.checkBootOrderStatus(fru, opt, "Enabled")

    def test_boot_order_enable(self):
        """
        Test CLI: bios-util server --boot_order enable --clear_CMOS
        """
        for fru in self.fru_list:
            with self.subTest(fru=fru):
                if not check_fru_availability(fru):
                    self.skipTest("skip test due to {} not available".format(fru))
                self.disableBootOrder(fru, "--clear_CMOS")
                self.enableBootOrder(fru, "--clear_CMOS")

    def test_boot_order_disable(self):
        """
        Test CLI: bios-util server --boot_order disable --clear_CMOS
        """
        for fru in self.fru_list:
            with self.subTest(fru=fru):
                if not check_fru_availability(fru):
                    self.skipTest("skip test due to {} not available".format(fru))
                self.enableBootOrder(fru, "--clear_CMOS")
                self.disableBootOrder(fru, "--clear_CMOS")

    def test_boot_order_set_bootmode(self):
        """
        Test CLI: bios-util server --boot_order set --boot_mode
        """
        for fru in self.fru_list:
            with self.subTest(fru=fru):
                if not check_fru_availability(fru):
                    self.skipTest("skip test due to {} not available".format(fru))
                try:
                    # DEFAULT boot_mode is UEFI
                    # test set bootmode: UEFI -> Legacy -> UEFI
                    self.checkBootOrderStatus(fru, "--boot_mode", "UEFI")
                    cmd = [
                        "/usr/local/bin/bios-util",
                        fru,
                        "--boot_order",
                        "set",
                        "--boot_mode",
                        "0",
                    ]
                    run_cmd(cmd)
                    self.checkBootOrderStatus(fru, "--boot_mode", "Legacy")
                    cmd = [
                        "/usr/local/bin/bios-util",
                        fru,
                        "--boot_order",
                        "set",
                        "--boot_mode",
                        "1",
                    ]
                    run_cmd(cmd)
                    self.checkBootOrderStatus(fru, "--boot_mode", "UEFI")
                except Exception:
                    # DEFAULT boot_mode is Legacy
                    # test set bootmode: Legacy -> UEFI -> Legacy
                    cmd = [
                        "/usr/local/bin/bios-util",
                        fru,
                        "--boot_order",
                        "set",
                        "--boot_mode",
                        "1",
                    ]
                    run_cmd(cmd)
                    self.checkBootOrderStatus(fru, "--boot_mode", "UEFI")
                    cmd = [
                        "/usr/local/bin/bios-util",
                        fru,
                        "--boot_order",
                        "set",
                        "--boot_mode",
                        "0",
                    ]
                    run_cmd(cmd)
                    self.checkBootOrderStatus(fru, "--boot_mode", "Legacy")

    def test_postcode_get(self):
        for fru in self.fru_list:
            with self.subTest(fru=fru):
                if not check_fru_availability(fru):
                    self.skipTest("skip test due to {} not available".format(fru))
                cmd = ["/usr/local/bin/bios-util", fru, "--postcode", "get"]
                out = run_cmd(cmd)
                pattern = r"^([A-F0-9\s\n]+)$|^$"
                self.assertRegex(out, pattern)
