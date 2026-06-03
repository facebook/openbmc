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
import unittest

from common.base_util_smoke_test import BaseUtilSmokeTest


# Intentionally NOT skipped under QEMU: validates these utilities are installed
# and link cleanly. The functional *_util tests are skipped under QEMU (no
# FRUs/DIMMs/slots), so without this the binaries are never exercised there.
# UTILS confirmed present on this platform's image via QEMU probe (2026-06-02).
class UtilSmokeTest(BaseUtilSmokeTest, unittest.TestCase):
    UTILS = [
        "ncsi-util",
    ]
