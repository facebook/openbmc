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

import unittest

from utils.shell_util import run_shell_cmd
from common.base_me_set_power_test import BaseMESetPowerTest


class MESetPowerTest(BaseMESetPowerTest, unittest.TestCase):
    def set_ssh_session_bmc_hostname(self):
        # Using IPMI to get BMC IP
        raw_ip = run_shell_cmd("ipmitool raw 0x0C 0x02 0x00 0xC5").split(" ")
        self.bmc_hostname = "{:02d}{:02d}::{:02d}{:02d}".format(
            int(raw_ip[2]),
            int(raw_ip[3]),
            int(raw_ip[16]),
            int(raw_ip[17]),
        )
