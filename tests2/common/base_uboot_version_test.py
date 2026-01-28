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

import json
import re
import subprocess
import unittest
from abc import abstractmethod
from shlex import quote

from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class BaseUbootVersionCheck(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.PROC_MTD_PATH = "/proc/mtd"
        self.uboot_ver_regex = None
        self.uboot_ver_cmd = None

    def tearDown(self):
        Logger.info(f"Finished logging for {self._testMethodName}")
        pass

    @abstractmethod
    def set_uboot_version_check_params(self):
        pass

    def getMTD(self, name):
        mtd_name = quote(name)
        with open(self.PROC_MTD_PATH) as f:
            lines = f.readlines()
            for line in lines:
                if mtd_name in line:
                    return line.split(":")[0]
        return None

    def test_uboot_version_check(self):

        self.set_uboot_version_check_params()
        uboot_version = ""

        uboot_ver_re = re.compile(self.uboot_ver_regex)
        mtd_meta = self.getMTD("meta")

        if mtd_meta is None:
            stdout = subprocess.check_output(self.uboot_ver_cmd.split())

            for line in stdout.splitlines():
                matched = uboot_ver_re.fullmatch(line.decode().strip())
                if matched:
                    uboot_version = matched.group("uboot_ver")
                    break

        else:
            try:
                mtd_dev = "/dev/" + mtd_meta
                with open(mtd_dev, "r") as f:
                    raw_data = f.readline()
                    uboot_version = json.loads(raw_data)["version_infos"]["uboot_ver"]
            except Exception:
                uboot_version = " "

        self.assertIn(
            "2019.04",
            uboot_version,
            "uboot-version check is failed. Received={}".format(uboot_version),
        )
