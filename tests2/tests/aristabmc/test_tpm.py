#!/usr/bin/env python3
#
# Copyright 2026-present Facebook. All Rights Reserved.
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

from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class TPM2Test(unittest.TestCase):
    def test_tpm2_access(self):
        self.assertTrue(
            os.path.exists("/dev/tpmrm0") or os.path.exists("/dev/tpm0"),
            "TPM character device is missing",
        )
        output = run_shell_cmd("tpm2_getcap properties-fixed", ignore_err=True)
        self.assertIn("TPM2_PT_FAMILY_INDICATOR", output)
