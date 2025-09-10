#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
import unittest
from subprocess import run
from typing import Dict, List, NamedTuple  # importing for older python support

from utils.cit_logger import Logger
from utils.test_utils import qemu_check, running_systemd


class ExpectedFailedUnits(NamedTuple):
    failed_units: List[str]


# Opt in each platform by adding it's settings to this dict
# Specify unit names so we know what we need to fix / exempt from qemu
PLATFORM_SETTINGS: Dict[str, ExpectedFailedUnits] = {
    "bletchley": ExpectedFailedUnits(
        failed_units=[
            "reconfig-interface-duid-ll@eth0.service",
        ],
    ),
}


@unittest.skipIf(not running_systemd(), "Not a systemd platform, skipping ...")
@unittest.skipIf(not qemu_check(), "test env is not QEMU, skipping ...")
class CommonSystemdTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info(f"Finished logging for {self._testMethodName}")

    def test_systemd_healthy(self):
        """
        Test systemd has limited failed units and is fully running if
        0 failed units
        """

        # Check if we're a supported unit
        if self.platform not in PLATFORM_SETTINGS:
            self.skipTest("No BMC systemd platform settings set, skipping ...")

        # Check for failed units and output full output if so
        failed_units_output = run(
            ["systemctl", "--failed", "-o", "json"],
            check=True,
            capture_output=True,
            universal_newlines=True,
        ).stdout
        failed_json = json.loads(failed_units_output)

        expected_failed_units = PLATFORM_SETTINGS[self.platform].failed_units
        expected_failed_count = len(expected_failed_units)
        failed_count = len(failed_json)
        if failed_count > expected_failed_count:
            pretty_json = json.dumps(failed_json, indent=4)
            self.fail(
                f"systemd has {failed_count} unexpected unit(s) FAILED:\n{pretty_json}"
            )

        # Check we only have known allowed failed unit(s)
        for failed_unit in failed_json:
            self.assertIn(
                failed_unit["unit"],
                expected_failed_units,
                f"Unexpected failed unit:\n{failed_unit}",
            )

        # Check for systemd to be fully booted if we have no failed units
        # only if we've got a system that has 0 failed units
        if failed_count == 0:
            system_state = run(
                ["systemctl", "is-system-running"],
                check=True,
                capture_output=True,
                universal_newlines=True,
            ).stdout.strip()
            self.assertEqual(
                "running",
                system_state,
                f"Systemd is not fully running! In systemd state {system_state!r}",
            )
