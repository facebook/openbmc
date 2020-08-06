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
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseEmmcTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.emmc_blkdev = "/dev/mmcblk0"

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    #
    # Below method needs to be defined in platform_test.py to override
    # "self.emmc_mountpoint" variable when eMMC is auto-mounted in the
    # platform.
    #
    @abstractmethod
    def set_emmc_mount_point(self):
        self.emmc_mountpoint = None

    def test_emmc(self):
        self.assertTrue(os.path.exists(self.emmc_blkdev),
                        msg="%s does not exist" % self.emmc_blkdev)

        self.set_emmc_mount_point()
        if self.emmc_mountpoint:
            mnt_cmd = "mount | grep %s" % self.emmc_mountpoint
            output = run_shell_cmd(cmd=mnt_cmd)
            if not output:
                self.fail("%s is not mounted" % self.emmc_blkdev)

            mnt_point = output.split(' ')[2]
            self.assertEqual(self.emmc_mountpoint, mnt_point,
                             msg="incorrect mount point: expect %s, actual %s" %
                                 (self.emmc_mountpoint, mnt_point))
