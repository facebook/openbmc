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

import re
import subprocess
from abc import abstractmethod

from utils.cit_logger import Logger


class BaseVBootTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.set_vboot_type()

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    @abstractmethod
    def set_vboot_type(self):
        """[summary]To set isLegacy for different platform"""
        pass

    def _retrive_hsm_key(self, dev):
        """[summary] retrive img signing keys from vboot-check output

        Args:
            dev ([str]): flash0/flash1

        Returns:
            [dict]: img signing key name: img signing key value
        """
        cmd = ["vboot-check", "/dev/{}".format(dev)]
        if dev == "flash1":
            cmd.extend("--locked-bmc")
        regex = re.compile(
            r"(Subordinate keys signing key|U-Boot configuration signing key|"
            + "Kernel/ramdisk configuration signing key)"
        )
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        ret = {}
        while True:
            _line = proc.stdout.readline()
            if not _line:
                break
            _line = _line.decode("utf-8").strip()
            if regex.search(_line):
                split_line = _line.split(":")
                k = split_line[0].strip()
                v = split_line[1].strip()
                ret[k] = v
        return ret

    def _verify_key(self, signing_keys, dev):
        """[summary] verify HSM signing key
            if is legacy image, signing key will be kek-0/fb0
            if not legacy image, signing key will be obmc-[platform-types]-kek/sub-0
        Args:
            signing_keys ([dict]):
            dev ([str]): flash0/flash1
        """
        for k, v in signing_keys.items():
            with self.subTest("checking dev: {} key: {}".format(dev, k)):
                if k == "Subordinate keys signing key":
                    if self.isLegacy:
                        self.assertEqual(v, "kek0")
                    else:
                        regex = re.compile(r"^obmc-.*-kek-0")
                        # TODO: a more accurate check based on platform instead of regex
                        self.assertIsNotNone(
                            regex.search(v),
                            msg="signing key {} match error.".format(v)
                            + " expect obmc-.*-kek-0",
                        )
                elif (
                    k == "U-Boot configuration signing key"
                    or k == "Kernel/ramdisk configuration signing key"
                ):
                    if self.isLegacy:
                        self.assertEqual(v, "fb0")
                    else:
                        regex = re.compile(r"^obmc-.*-sub-0")
                        # TODO: a more accurate check based on platform instead of regex
                        self.assertIsNotNone(
                            regex.search(v),
                            msg="signing key {} match error.".format(v)
                            + " expect obmc-.*-sub-0",
                        )

    def test_img_signing_key(self):
        """
        Test all measures
        """
        devices = ["flash0", "flash1"]
        for dev in devices:
            signing_keys = self._retrive_hsm_key(dev)
            self._verify_key(signing_keys, dev)
