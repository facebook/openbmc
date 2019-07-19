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


from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseProcessRunningTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.process_cmd = None
        self.expected_process = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_process_cmd(self):
        self.process_cmd = ["ps"]

    @abstractmethod
    def set_processes(self):
        pass

    def get_installed_processes(self):
        self.set_process_cmd()
        self.assertNotEqual(
            self.process_cmd, None, "Command to get processes is not set"
        )
        Logger.info("Executing cmd={}".format(self.process_cmd))
        info = run_shell_cmd(cmd=self.process_cmd)
        return info

    def test_installed_processes(self):
        self.set_processes()
        info = self.get_installed_processes()
        self.assertNotEqual(
            self.expected_process, None, "Expected set of processes data not set"
        )

        for process in self.expected_process:
            with self.subTest(process=process):
                self.assertIn(process, info, "Process {} not running".format(process))
