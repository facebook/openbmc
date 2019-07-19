#!/usr/bin/env python
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

from common.base_kernel_module_presence_test import BaseKernelModulePresenceTest


class KernelModulePresenceTest(BaseKernelModulePresenceTest, unittest.TestCase):
    def set_kmods(self):
        self.expected_kmod = [
            "usb_f_acm",
            "u_serial",
            "usb_f_ecm",
            "g_cdc",
            "u_ether",
            "pmbus",
            "at24",
            "adm1275",
            "pmbus_core",
            "tmp421",
            "lm75",
            "powr1220",
            "tpm",
            "tpm_i2c_infineon",
            "smbcpld",
            "scmcpld",
            "psu_driver",
            "pdbcpld",
            "max34461",
            "isl68127",
            "ir3595",
            "iobfpga",
            "fcmcpld",
            "domfpga",
            "i2c_dev_sysfs",
            "at25",
            "spidev",
            "i2c_mux_pca954x",
            "i2c_mux",
        ]
