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

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


class SystemAirflowConfigTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        # (pim16q_count, pim8ddm_count, psu_count) : config
        # If config not here, expect CONFIG_UNKNOWN = 4
        self.configs = {
            (8, 0, 2): 0,  # CONFIG_8PIM16Q_0PIM8DDM_2PSU
            (5, 3, 2): 1,  # CONFIG_5PIM16Q_3PIM8DDM_2PSU
            (2, 6, 4): 2,  # CONFIG_2PIM16Q_6PIM8DDM_4PSU
            (0, 8, 4): 3,  # CONFIG_0PIM16Q_8PIM8DDM_4PSU
        }

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def get_pim_count(self):
        pim16q_count = 0
        pim8ddm_count = 0
        for pim in range(2, 10):
            cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/pim{}_present"
            if "0x1" in run_shell_cmd(cmd.format(pim)):
                cmd = "/usr/bin/kv get pim{}_type".format(pim)
                pim_type = run_shell_cmd(cmd)
                if pim_type.strip() == "16q2":
                    pim16q_count += 1
                elif pim_type.strip() == "16q":
                    pim16q_count += 1
                elif pim_type.strip() == "8ddm":
                    pim8ddm_count += 1
        return pim16q_count, pim8ddm_count

    def get_psu_count(self):
        psu_count = 0
        for psu in range(1, 5):
            syspath = "/sys/bus/i2c/devices/4-0023/psu{}_present".format(psu)
            cmd = "cat {} | head -n 1".format(syspath)
            psu_present = run_shell_cmd(cmd)
            if psu_present.strip() == "0x1":
                psu_count += 1
        return psu_count

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_system_airflow_config(self):
        pim16q_count, pim8ddm_count = self.get_pim_count()
        psu_count = self.get_psu_count()
        exp_config = self.configs.get((pim16q_count, pim8ddm_count, psu_count), 4)
        cmd = "/usr/bin/kv get sys_airflow_cfg"
        reported_config = run_shell_cmd(cmd)
        self.assertEqual(int(reported_config), exp_config)
