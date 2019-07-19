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
import time
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseFscdTest(object):
    def setUp(self, config=None, test_data_path=None):
        """
        Series of tests that are driven by changing the temperature sensors
        """
        Logger.start(name=self._testMethodName)
        self.assertNotEqual(config, None, "FSC TEST config needs to be set")
        self.assertNotEqual(test_data_path, None, "FSC TEST data path needs to be set")

        # Backup original config
        run_shell_cmd("cp /etc/fsc-config.json /etc/fsc-config.json.orig")
        # Copy test config to fsc-config and restart fsc

        run_shell_cmd(
            "cp {}/{} /etc/fsc-config.json".format(test_data_path, str(config))
        )
        self.restart_fscd()

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        run_shell_cmd("cp /etc/fsc-config.json.orig /etc/fsc-config.json")
        self.restart_fscd()
        self.power_host_on()

    def restart_fscd(self):
        run_shell_cmd("sv restart fscd")
        time.sleep(20)

    @abstractmethod
    def power_host_on(self):
        """
        Method to power on host
        """
        pass

    @abstractmethod
    def is_host_on(self):
        """
        Method to test if host power is on
        """
        pass
