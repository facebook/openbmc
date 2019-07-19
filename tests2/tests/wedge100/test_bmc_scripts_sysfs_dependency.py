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
import os
import unittest

from common.base_bmc_scripts_sysfs_dependency_test import (
    BaseBmcShellScriptsSysfsDependencyTest,
)
from utils.cit_logger import Logger


class BmcShellScriptsSysfsDependencyTest(
    BaseBmcShellScriptsSysfsDependencyTest, unittest.TestCase
):
    """
    Most of these scripts can reset HW. Verify that the sysfs paths are accessible
    Testing the scripts itself will be disruptive
    """

    def test_board_utils_sysfs_dependency(self):
        set = [
            "/sys/bus/i2c/devices/i2c-12/12-0031/pwr_main_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/pwr_usrv_en",
            "/sys/bus/i2c/devices/i2c-12/12-0031/pwr_usrv_btn_en",
            "/sys/bus/i2c/devices/i2c-12/12-0031/slotid",
        ]
        self.verify_if_path_exists(set)

    def test_cp2112_i2c_flush_sysfs_dependency(self):
        set = ["/sys/bus/i2c/devices/i2c-12/12-0031/i2c_flush_en"]
        self.verify_if_path_exists(set)

    def test_reset_bcrm_sysfs_dependency(self):
        set = ["/sys/bus/i2c/devices/i2c-12/12-0031/th_sys_rst_n"]
        self.verify_if_path_exists(set)

    def test_reset_cp2112_sysfs_dependency(self):
        set = ["/sys/bus/i2c/devices/i2c-12/12-0031/usb2cp2112_rst_n"]
        self.verify_if_path_exists(set)

    def test_reset_qsfp_mux_sysfs_dependency(self):
        set = [
            "/sys/bus/i2c/devices/i2c-12/12-0031/i2c_mux0_rst_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/i2c_mux1_rst_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/i2c_mux2_rst_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/i2c_mux3_rst_n",
        ]
        self.verify_if_path_exists(set)

    def test_setup_board_sysfs_dependency(self):
        set = ["/sys/bus/i2c/devices/i2c-12/12-0031/com-e_i2c_isobuf_en"]
        self.verify_if_path_exists(set)

    def test_wedge_power_sysfs_dependency(self):
        set = [
            "/sys/bus/i2c/devices/i2c-12/12-0031/pwr_cyc_all_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/usrv_rst_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/th_sys_rst_n",
            "/sys/bus/i2c/devices/i2c-12/12-0031/pwr_main_n",
        ]
        self.verify_if_path_exists(set)
