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
import re
import subprocess
import sys
import unittest


class PlatformInfo:
    """
    Base class for reading EEPROM and filtering platforms.
    """

    @classmethod
    def skip_unless_platform(cls, supported):
        platform_name = cls.get_platform()[0] or ""
        if platform_name not in supported:
            raise unittest.SkipTest(
                f"Test skipped: unsupported platform {platform_name}"
            )

    @classmethod
    def get_platform(cls):
        try:
            output = subprocess.check_output(
                ["/usr/bin/weutil", "-e", "chassis_eeprom"],
                encoding="utf-8",
                stderr=subprocess.DEVNULL,
            )
        except Exception as e:
            # stdout is the test-list channel during CIT discovery, so this
            # must never go there.
            print(f"Unable to read chassis_eeprom: {e}", file=sys.stderr)
            return None, None

        platform_name_match = re.search(r"Product Name:\s*(.+)", output)
        platform_name = (
            platform_name_match.group(1).strip().upper().split("_")[0]
            if platform_name_match
            else None
        )

        state_match = re.search(r"Production State:\s*(.+)", output)
        sub_state_match = re.search(r"Production Sub-State:\s*(.+)", output)

        state = state_match.group(1).strip().upper() if state_match else None
        sub_state = (
            sub_state_match.group(1).strip().upper() if sub_state_match else None
        )
        platform_rev = f"{state}{sub_state}"

        return platform_name, platform_rev
