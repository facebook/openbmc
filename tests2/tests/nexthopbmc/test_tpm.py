#!/usr/bin/env python3
#
# Copyright (c) 2026 Nexthop Systems Inc.
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

import os
import unittest

from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class TpmTest(unittest.TestCase):
    def test_tpm_device_exists(self):
        """
        Tests that the TPM device /dev/tpm0 was created successfully.
        """
        self.assertTrue(
            os.path.exists("/dev/tpm0"),
            "TPM device /dev/tpm0 does not exist"
        )
