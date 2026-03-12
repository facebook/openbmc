#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class BaseKernelVersionTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.kernel_version_cmd = "uname -r"
        self.kernel_version = None

    def tearDown(self):
        Logger.info(f"Finished logging for {self._testMethodName}")

    @abstractmethod
    def set_kernel_version(self):
        pass

    @staticmethod
    def _parse_version(version_str):
        """Parse version string into a comparable tuple.

        Examples:
            '6.18.6-fbdarwin-xxx' -> (6, 18, 6)
            '6.12' -> (6, 12)
        """
        match = re.match(r"(\d+(?:\.\d+)*)", version_str)
        if not match:
            return ()
        return tuple(int(x) for x in match.group(1).split("."))

    def test_kernel_version(self):
        self.set_kernel_version()
        self.assertNotEqual(self.kernel_version, None, "Kernel version not set")
        Logger.info("Executing cmd={}".format(self.kernel_version_cmd))
        info = run_shell_cmd(cmd=self.kernel_version_cmd).strip()
        actual = self._parse_version(info)
        minimum = self._parse_version(self.kernel_version)
        self.assertGreaterEqual(
            actual,
            minimum,
            f"Kernel version too old. Minimum: {self.kernel_version}, Actual: {info}",
        )
