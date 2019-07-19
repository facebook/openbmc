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


class BaseKernelModulePresenceTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.kmod_cmd = None
        self.expected_kmod = None
        self.new_kmods = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_kmod_cmd(self):
        self.kmod_cmd = ["lsmod"]

    @abstractmethod
    def set_kmods(self):
        pass

    def get_installed_kmods(self):
        self.set_kmod_cmd()
        self.assertNotEqual(self.kmod_cmd, None, "Command to get kmod is not set")
        Logger.info("Executing cmd={}".format(self.kmod_cmd))
        info = run_shell_cmd(cmd=self.kmod_cmd)
        return info

    def test_installed_kmods(self):
        self.set_kmods()
        info = self.get_installed_kmods()
        self.assertNotEqual(
            self.expected_kmod, None, "Expected set of kmods data not set"
        )

        for kmod in self.expected_kmod:
            with self.subTest(kmod=kmod):
                self.assertIn(kmod, info, "Kernel module {} not installed".format(kmod))
