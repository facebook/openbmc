#!/usr/bin/env python3
#
# Copyright 2025-present Facebook. All Rights Reserved.
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
import time
import unittest

from utils.shell_util import run_shell_cmd

from utils.test_utils import qemu_check

TEST_SCRIPT = "/tmp/test_dmidecode.sh"
ACONF_SMB_SERIAL_CACHE = "/mnt/data/.aconf_smb_serial"
ACONF_SMB_SERIAL_CACHE_STUB = "/tmp/aconf_smb_serial"


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Dmidecode(unittest.TestCase):
    def run_test_script(self, weutil_contents, aconfutil_contents):
        # Create test script and run it
        # (replace WEUTIL_CMD after sourcing)
        with open(TEST_SCRIPT, "w") as f:
            print("#!/bin/bash", file=f)
            print(". /usr/local/bin/board-utils.sh", file=f)
            print("function weutil_stub() {", file=f)
            for line in weutil_contents.splitlines():
                print(f"    echo {line!r}", file=f)  # Use !r for repr conversion
            print("}", file=f)
            print("WEUTIL_CMD=weutil_stub", file=f)
            print("function aconfutil_stub() {", file=f)
            print('    if [ $1 == "show" ]; then', file=f)
            for line in aconfutil_contents.splitlines():
                print(f"        echo {line!r}", file=f)  # Use !r for repr conversion
            print("    fi", file=f)
            print("}", file=f)
            print("ACONFUTIL_CMD=aconfutil_stub", file=f)
            print(f"ACONF_SMB_SERIAL_CACHE={ACONF_SMB_SERIAL_CACHE_STUB}", file=f)
            print("maybe_fix_dmi_config", file=f)
        os.chmod(TEST_SCRIPT, 0o755)
        return run_shell_cmd(TEST_SCRIPT)

    def wait_power_status(self, exp_status, timeout=10):
        deadline = time.time() + timeout
        while time.time() < deadline:
            output = run_shell_cmd("wedge_power.sh status").splitlines()[0]
            assert "Microserver power is" in output
            status = output.split()[3]
            if status == exp_status:
                return
            time.sleep(0.5)
        else:
            raise AssertionError(f"Timed out waiting for power status {exp_status}")

    def test_dmidecode_initial(self):
        output = run_shell_cmd("weutil -e chassis_eeprom")
        product_name = [x for x in output.splitlines() if "Product Name:" in x][
            0
        ].split()[2]
        serial_number = [
            x for x in output.splitlines() if "Product Serial Number:" in x
        ][0].split()[3]
        run_shell_cmd("wedge_power.sh off")
        self.wait_power_status("off")
        output = run_shell_cmd("aconf_util.sh show")
        dmi_board_name = [x for x in output.splitlines() if "DMI_BOARD_NAME=" in x][
            0
        ].split("=")[1]
        assert dmi_board_name.lower() == product_name.lower()

        # The cache may not exist if a cold boot has not been done since
        # the BMC was updated with aconf cache logic. Therefore only verify
        # the cached serial number if the cache file exists.
        if os.path.exists(ACONF_SMB_SERIAL_CACHE):
            cached_serial = open(ACONF_SMB_SERIAL_CACHE, "r").read().splitlines()[0]
            assert cached_serial == serial_number

    def test_dmidecode_pon(self):
        # Tests cases where the power is on.
        run_shell_cmd("wedge_power.sh on")
        self.wait_power_status("on")
        if os.path.exists(ACONF_SMB_SERIAL_CACHE_STUB):
            os.unlink(ACONF_SMB_SERIAL_CACHE_STUB)

        # No serial number case
        output = self.run_test_script(
            "Product Name: Meru80012345", "DMI_BOARD_NAME=Foo"
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" not in output
        assert not os.path.exists(ACONF_SMB_SERIAL_CACHE_STUB)

        # Serial cache file case
        with open(ACONF_SMB_SERIAL_CACHE_STUB, "w") as f:
            print("JAS12345", file=f)
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: Meru80012345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" not in output

        # Invalid product name case
        os.unlink(ACONF_SMB_SERIAL_CACHE_STUB)
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: ABC12345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Not fixing aboot_conf DMI_BOARD_NAME" in output

        # Powered on case
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: Meru80012345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" not in output

    def test_dmidecode_poff(self):
        # Tests cases where the power is off.
        run_shell_cmd("wedge_power.sh off")
        self.wait_power_status("off")
        if os.path.exists(ACONF_SMB_SERIAL_CACHE_STUB):
            os.unlink(ACONF_SMB_SERIAL_CACHE_STUB)

        # No serial number case
        output = self.run_test_script(
            "Product Name: Meru80012345", "DMI_BOARD_NAME=Foo"
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" not in output
        assert not os.path.exists(ACONF_SMB_SERIAL_CACHE_STUB)

        # Serial cache file case
        with open(ACONF_SMB_SERIAL_CACHE_STUB, "w") as f:
            print("JAS12345", file=f)
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: Meru80012345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" not in output

        # Invalid product name case
        os.unlink(ACONF_SMB_SERIAL_CACHE_STUB)
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: ABC12345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Not fixing aboot_conf DMI_BOARD_NAME" in output

        # Fix-up case, no cache file
        if os.path.exists(ACONF_SMB_SERIAL_CACHE_STUB):
            os.unlink(ACONF_SMB_SERIAL_CACHE_STUB)
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: Meru80012345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" in output

        # Fix-up case, serial number in cache different
        with open(ACONF_SMB_SERIAL_CACHE_STUB, "w") as f:
            print("JAS67890", file=f)
        output = self.run_test_script(
            "Product Serial Number: JAS12345\nProduct Name: Meru80012345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" in output

        # Fix-up case 2, serial number in cache is a superset
        with open(ACONF_SMB_SERIAL_CACHE_STUB, "w") as f:
            print("JAS67890X", file=f)
        output = self.run_test_script(
            "Product Serial Number: JAS67890\nProduct Name: Meru80012345",
            "DMI_BOARD_NAME=Foo",
        )
        assert "Fixing aboot_conf DMI_BOARD_NAME" in output
