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
import unittest

from common.base_process_running_test import BaseProcessRunningTest
from tests.wedge100.power_supply import get_power_type
from utils.test_utils import running_systemd


class ProcessRunningTest(BaseProcessRunningTest, unittest.TestCase):
    def set_processes(self):
        self.expected_process = [
            "fscd",
            "rackmond",
            "rest.py",
            "mTerm_server",
        ]
        if not running_systemd():
            self.expected_process.extend(
                ["/var/run/dhclient.eth0.pid", "/var/run/dhclient6.eth0.pid"]
            )

        # "psumuxmon" is not needed on switches with PSU power supplies.
        power_type = get_power_type()
        if power_type.startswith("pem"):
            self.expected_process.append("psumuxmon")
