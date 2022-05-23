#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import json
import re
import subprocess
import unittest

from shlex import quote

from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class UbootVersionCheck(unittest.TestCase):
    def setUp(self):
        self.PROC_MTD_PATH = "/proc/mtd"
        pass

    def tearDown(self):
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

        uboot_version = ""

        uboot_ver_regex = r"^U-Boot\W+(?P<uboot_ver>20\d{2}\.\d{2})\W+.*$"
        uboot_ver_re = re.compile(uboot_ver_regex)
        mtd_meta = self.getMTD("meta")

        if mtd_meta is None:
            stdout = subprocess.check_output(["/usr/bin/strings", "/dev/mtd0"])

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
