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
import subprocess
from unittest import TestCase

from utils.test_utils import running_systemd


class BaseSystemdTest(TestCase):
    def test_failed_units(self):
        if not running_systemd():
            self.skipTest("not using systemd")
            return

        # XXX tcsd is unreliable to start
        command = "systemctl list-units | grep failed | grep -v trousers"
        output = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            shell=True,
            timeout=60,
        )
        stdout = output.stdout.decode("utf-8")

        self.assertTrue(len(stdout) == 0, command + " returned: " + stdout)

    def test_setup_i2c_order(self):
        if not running_systemd():
            self.skipTest("not using systemd")
            return

        command = "systemctl show basic.target | grep After"
        output = subprocess.check_output(
            command,
            stderr=subprocess.STDOUT,
            shell=True,
            timeout=60,
        )
        stdout = output.decode("utf-8")
        self.assertTrue("setup_i2c", command + " returned: " + stdout)

    def test_tmp_no_cleanup(self):
        if not running_systemd():
            self.skipTest("not using systemd")
            return

        fn = "/usr/lib/tmpfiles.d/tmp.conf"
        with open(fn, "r") as f:
            for ln in f:
                fld = ln.strip().split(" ")
                # https://www.freedesktop.org/software/systemd/man/tmpfiles.d.html
                # The 6th field specifies minimum age before cleanup.  A single
                # dash or missing field signifies no cleanup, which is expected.
                # This is intended to match both /tmp and /var/tmp.
                if len(fld) < 6 or fld[0] == "#" or "/tmp" not in fld[1]:
                    continue
                self.assertEqual(fld[5], "-", "unexpected age config in " + ln)
