#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
            "adm1275",
            "gpio_pca953x",
            "tmp421",
            "powr1220",
            "tpm",
            "tpm_i2c_infineon",
            "smb_syscpld",
            "smb_pwrcpld",
            "scmcpld",
            "fcbcpld",
            "psu",
            "isl68137",
            # "ir35215",  #Failing Test
            "domfpga",
            "i2c_dev_sysfs",
            "i2c_mux_pca954x",
            "i2c_mux",
            "aspeed_vhub",
            "libcomposite",
            "uhci_hcd",
            "udc_core",
            "configfs",
            "ext4",
        ]
