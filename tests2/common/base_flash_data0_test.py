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


import os
import re
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseFlashData0Test(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.data0_dev = None
        self.data0_mountpoint = None
        self.data0_fs_type = None
        self.data0_size_mb = None

    def tearDown(self):
        Logger.info(f"Finished logging for {self._testMethodName}")

    @abstractmethod
    def set_data0_info(self):
        pass

    def test_data0_partition_mount(self):
        self.set_data0_info()
        self.assertNotEqual(self.data0_dev, None, "data0 dev not set")
        self.assertTrue(
            os.path.exists(self.data0_dev), f"{self.data0_dev} does not exist"
        )

        self.assertNotEqual(self.data0_mountpoint, None, "data0 mount point not set")
        self.assertTrue(
            os.path.exists(self.data0_mountpoint),
            f"{self.data0_mountpoint} does not exist",
        )

        # mount format is:
        # /dev/ubi0_0 on /mnt/data type ubifs
        # Don't pipe mount to grep because if data0 is not mounted then the command
        # returns 1 and run_shell_cmd fails, so the error isn't clearly reported by
        # assertTrue.
        mount_cmd = "mount"
        output = run_shell_cmd(cmd=mount_cmd)
        pattern = rf"([\w/]+)\s+on\s+{self.data0_mountpoint}\s+type\s+(\w+)"
        mount_match = re.search(pattern, output)
        self.assertTrue(mount_match, f"{self.data0_mountpoint} is not mounted")

        actual_dev = mount_match.group(1)
        self.assertEqual(
            self.data0_dev,
            actual_dev,
            f"{self.data0_mountpoint} is mounted on the wrong device. "
            f"Expected: {self.data0_dev}, "
            f"Actual: {actual_dev}",
        )

        if self.data0_fs_type:
            actual_fs_type = mount_match.group(2)
            self.assertEqual(
                self.data0_fs_type,
                actual_fs_type,
                f"{self.data0_mountpoint} filesystem type mismatch. "
                f"Expected: {self.data0_fs_type}, "
                f"Actual: {actual_fs_type}",
            )

    def get_data0_mtd_dev(self):
        # /proc/mtd file is formated like:
        # mtd4: 04000000 00010000 "data0"
        mtd_cmd = "cat /proc/mtd | grep data0 | awk '{print $1}' | tr -d :"
        mtd_dev = run_shell_cmd(cmd=mtd_cmd).strip()
        return mtd_dev

    def test_data0_partition_size(self):
        self.set_data0_info()
        if self.data0_size_mb:
            self.assertNotEqual(
                self.data0_fs_type, None, "data0 filesystem type not set"
            )

            if self.data0_fs_type == "ubifs":
                mtd_dev = self.get_data0_mtd_dev()
                self.assertTrue(mtd_dev, "data0 mtd device not found")

                # Use the ubiscan utility as it reports the total size.
                # ubiscan output is formated like:
                # size   : 67108864 bytes (64.0 MiB)
                size_cmd = f"ubiscan /dev/{mtd_dev} | grep size"
                output = run_shell_cmd(cmd=size_cmd)
                pattern = r"size\s+:\s+\d+\s+bytes\s+\((\d+\.\d+)\s+MiB\)"
                size_match = re.search(pattern, output)
                self.assertTrue(size_match, f"Failed reading size of {mtd_dev}")
                actual_size_mb = float(size_match.group(1))
                self.assertEqual(
                    float(self.data0_size_mb),
                    actual_size_mb,
                    f"data0 partition size mismatch. "
                    f"Expected: {self.data0_size_mb} MiB, "
                    f"Actual: {actual_size_mb} MiB",
                )
