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


# Intentionally NOT skipped under QEMU. yv2 (fby2) runs only on the QEMU
# conveyor node, where every functional *_util test is skipped (no FRUs /
# DIMMs / slots present). Without this, these binaries would never be
# exercised in CI -- a missing or unlinkable util would ship undetected.
#
# UTILS = the utilities fby2 ships that have a HW-only functional test and no
# other QEMU coverage. log-util and slot-util are deliberately excluded: their
# tests already invoke the binary in QEMU, so presence is covered there.
class UtilSmokeTest(BaseUtilSmokeTest, unittest.TestCase):
    UTILS = [
        "bios-util",
        "cfg-util",
        "dimm-util",
        "fw-util",
        "name-util",
        "power-util",
    ]
