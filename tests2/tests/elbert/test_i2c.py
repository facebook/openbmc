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

from common.base_i2c_test import BaseI2cTest
from tests.elbert.test_data.i2c.i2c import (
    plat_i2c_tree,
    pim16q,
    pim8ddm,
    psu_devices,
    secure_devices,
    pim_secure_devices,
)

from utils.shell_util import run_shell_cmd

# Map between PIM and SMBus channel
pim_bus_p1 = [16, 17, 18, 23, 20, 21, 22, 19]
pim_bus_p2 = [16, 17, 18, 19, 20, 21, 22, 23]


class ElbertI2cTest(BaseI2cTest, unittest.TestCase):
    def get_pim_bus(self):
        if not hasattr(self, "pim_bus"):
            if re.search(r"PCA014590", run_shell_cmd("weutil smb")):
                # P1 has different pim_bus map
                self.pim_bus = pim_bus_p1
            else:
                self.pim_bus = pim_bus_p2
            return self.pim_bus

    def get_pim_types(self):
        if not hasattr(self, "pim_types"):
            cmd = "/usr/local/bin/pim_types.sh"
            self.pim_types = run_shell_cmd(cmd)
        return self.pim_types

    def load_golden_i2c_tree(self):
        self.i2c_tree = plat_i2c_tree
        pim_bus = self.get_pim_bus()
        pim_types = self.get_pim_types()

        # Add PIM2-9 devices for available pims
        for pim in range(2, 9 + 1):
            if "PIM {}: PIM16Q".format(pim) in pim_types:
                for address, name, driver in pim16q:
                    self.i2c_tree["{:02d}-{}".format(pim_bus[pim - 2], address)] = {
                        "name": name,
                        "driver": driver,
                    }
            elif "PIM {}: PIM8DDM".format(pim) in pim_types:
                for address, name, driver in pim8ddm:
                    self.i2c_tree["{:02d}-{}".format(pim_bus[pim - 2], address)] = {
                        "name": name,
                        "driver": driver,
                    }

        # Add PSU1-4 devices for available psus
        for psu in range(1, 4 + 1):
            cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/psu{}_present"
            if "0x1" in run_shell_cmd(cmd.format(psu)):
                for address, name, driver in psu_devices:
                    self.i2c_tree["{:02d}-{}".format(23 + psu, address)] = {
                        "name": name,
                        "driver": driver,
                    }

    def verify_secure_devices(self, devices):
        for name, bus, dev, dev_type in devices:
            if dev_type == "ucd":
                output = run_shell_cmd("i2cget -f -y {} {} 0xf1 i".format(bus, dev))
                if "0x000000000001" not in output:
                    raise AssertionError(
                        "{}: unexpected output: {}".format(name, output)
                    )
            else:
                assert "pmbus" in dev_type
                secure_val = dev_type.split("-")[1]
                output = run_shell_cmd("i2cget -f -y {} {} 0x10 b".format(bus, dev))
                if secure_val not in output:
                    raise AssertionError(
                        "{}: unexpected output: {}".format(name, output)
                    )

    def test_i2c_security(self):
        self.verify_secure_devices(secure_devices)
        pim_bus = self.get_pim_bus()
        pim_types = self.get_pim_types()
        for pim in range(2, 9 + 1):
            devices = []
            bus = pim_bus[pim - 2]
            for name, dev, dev_type, pim_type in pim_secure_devices:
                if pim_type and "PIM {}: {}".format(pim, pim_type) not in pim_types:
                    continue
                devices.append(("PIM{} {}".format(pim, name), bus, dev, dev_type))
            self.verify_secure_devices(devices)
