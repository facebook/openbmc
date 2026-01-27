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

import os
import unittest
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


#
# Test persistent mtd datastore. Sample mtd_store_config:
# {
#     "mtd_label": "data0",
#     "mtd_size_kb": 8192,
#     "mount_point": "/mnt/data",
#     "fs_type": "jffs2",
# }
#
class BaseMtdStoreTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.mtd_store_config = {}

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    @abstractmethod
    def set_mtd_store_config(self):
        """
        Please set self.mtd_store_config in platform-level.
        """
        pass

    def mtd_label_to_blkdev(self, label):
        """
        Translate mtd label (such as data0) to /dev/mtdblock#.
        """
        symlink = os.path.join("/dev/mtd/by-name", label)
        if not os.path.exists(symlink):
            return None

        try:
            sym_target = os.readlink(symlink)
        except Exception:
            return None

        mtd_dev = os.path.basename(sym_target)
        mtdblock = mtd_dev.replace("mtd", "mtdblock")
        return os.path.join("/dev", mtdblock)

    def get_disk_size(self, dev_path):
        """
        Parse disk size from "df" output. Sample "df" output:
        /dev/mtdblock4            8192       388      7804   5% /mnt/data
        """
        df_cmd = "df | grep %s" % dev_path
        output = run_shell_cmd(cmd=df_cmd)
        if not output:
            return None

        # The 2nd column contains disk size in kilo bytes.
        columns = output.split()
        try:
            disk_size = int(columns[1])
        except Exception:
            return None

        return disk_size

    def get_mount_info(self, dev_path):
        """
        Parse mount info from /proc/mounts file. Sample lines as below:
        /dev/mtdblock4 /mnt/data jffs2 rw,relatime 0 0
        """
        proc_mounts = "/proc/mounts"
        try:
            with open(proc_mounts, "r") as f:
                for line in f:
                    columns = line.split()

                    # First column: device path
                    # 2nd column: mount point
                    # 3rd column: filesystem
                    if dev_path == columns[0]:
                        return (columns[1], columns[2])
        except Exception:
            Logger.error("failed to parse " + proc_mounts)

        return None

    def test_mtd_storage(self):
        """
        Test actual image layout on spi0.0 matches image meta data
        """
        self.set_mtd_store_config()

        if not self.mtd_store_config:
            self.skipTest("mtd storage config not supplied")
            return

        mtd_label = self.mtd_store_config["mtd_label"]
        mtd_blkdev = self.mtd_label_to_blkdev(mtd_label)
        if not mtd_blkdev:
            self.fail("failed to find mtdblock for %s" % mtd_label)

        size = self.get_disk_size(mtd_blkdev)
        if size is None:
            self.fail("failed to get disk size of %s" % mtd_blkdev)
        elif size != self.mtd_store_config["mtd_size_kb"]:
            self.fail(
                "%s: unexpected disk size (kb): expect %d, actual %d"
                % (mtd_blkdev, self.mtd_store_config["mtd_size_kb"], size)
            )

        mount_info = self.get_mount_info(mtd_blkdev)
        if mount_info is None:
            self.fail("failed to get mount info of %s" % mtd_blkdev)

        mount_point = mount_info[0]
        fs_type = mount_info[1]
        if mount_point != self.mtd_store_config["mount_point"]:
            self.fail(
                "%s: unexpected mount point: expect %s, actual %s"
                % (mtd_blkdev, self.mtd_store_config["mount_point"], mount_point)
            )
        elif fs_type != self.mtd_store_config["fs_type"]:
            self.fail(
                "%s: unexpected filesystem: expect %s, actual %s"
                % (mtd_blkdev, self.mtd_store_config["fs_type"], fs_type)
            )
