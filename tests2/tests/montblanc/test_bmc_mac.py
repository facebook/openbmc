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
import unittest

from common.base_bmc_mac_test import BaseBMCMacTest


class BMCMacTest(BaseBMCMacTest, unittest.TestCase):
    def set_bmc_mac(self):
        self.bmc_mac_cmd = [". openbmc-utils.sh && bmc_mac_addr"]

    def set_bmc_interface(self):
        self.bmc_interface = "eth0"

    def set_valid_mac_pattern(self):
        # Celestica Vendor OUI
        # ref: https://ouilookup.com/search/CELESTICA
        self.mac_pattern = [
            r"(b4\:db\:91\:..\:..\:..)",
            r"(00\:e0\:ec\:..\:..\:..)",
            r"(34\:ad\:61\:..\:..\:..)",
            r"(0c\:48\:c6\:..\:..\:..)",
        ]
