#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from common.base_uboot_version_test import BaseUbootVersionCheck


class UbootVersionCheck(BaseUbootVersionCheck, unittest.TestCase):
    def set_uboot_version_check_params(self):
        self.uboot_ver_regex = r"\"uboot_ver\":\s+\"(?P<uboot_ver>20\d{2}\.\d{2})\""
        self.uboot_ver_cmd = "/usr/local/bin/meta_info.sh flash0"
