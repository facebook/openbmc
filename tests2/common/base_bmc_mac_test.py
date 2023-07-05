#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
import re

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import mac_verify


class BaseBMCMacTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.bmc_mac_cmd = None
        self.bmc_interface = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_bmc_mac(self):
        pass

    @abstractmethod
    def set_bmc_interface(self):
        pass

    @abstractmethod
    def set_valid_mac_pattern(self):
        self.mac_pattern = [
            r"(..\:..\:..\:..\:..\:..)",
        ]

    def get_mac_by_interface(self, inf):
        file = open("/sys/class/net/{}/address".format(inf), "r")
        mac = file.read()
        file.close()
        return mac

    def test_bmc_mac(self):
        pattern = r"(..\:..\:..\:..\:..\:..)"
        self.set_bmc_mac()
        self.assertNotEqual(self.bmc_mac_cmd, None, "BMC MAC command not set")
        self.set_bmc_interface()
        self.assertNotEqual(self.bmc_interface, None, "BMC interface not set")
        bmc_mac = run_shell_cmd(cmd=self.bmc_mac_cmd)
        bmc_mac = re.search(pattern, bmc_mac)[1]
        bmc_mac = bmc_mac.lower()
        self.assertTrue(
            mac_verify(bmc_mac),
            "BMC MAC is incorrectly formatted to \
                        {}".format(
                bmc_mac
            ),
        )
        inf_mac = self.get_mac_by_interface(self.bmc_interface)
        inf_mac = re.search(pattern, inf_mac)[1]
        inf_mac = inf_mac.lower()
        self.assertTrue(
            mac_verify(inf_mac),
            "Interface MAC is incorrectly formatted to \
                        {}".format(
                inf_mac
            ),
        )
        self.assertEqual(
            bmc_mac,
            inf_mac,
            "Interface MAC is not same with BMC MAC {} != {}".format(inf_mac, bmc_mac),
        )

    def test_bmc_mac_oui(self):
        self.set_bmc_interface()
        self.assertNotEqual(self.bmc_interface, None, "BMC interface not set")
        mac = self.get_mac_by_interface(self.bmc_interface)
        self.assertTrue(
            mac_verify(mac),
            "BMC MAC is incorrectly formatted to \
                        {}".format(
                mac
            ),
        )

        self.set_valid_mac_pattern()
        self.assertNotEqual(self.mac_pattern, None, "MAC Pattern list not set")

        found = False
        for pattern in self.mac_pattern:
            found = re.match(pattern, mac)
            if found:
                break
        self.assertTrue(
            found,
            " the mac address not contain in pattern list \n"
            " {} not match in {}".format(mac, " ".join(self.mac_pattern)),
        )
