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
import os
import subprocess
from unittest import TestCase

LOGROTATE_BIN = "/usr/sbin/logrotate"
LOGROTATE_CONF = "/etc/logrotate.conf"

NEEDED_PATHS = [
    LOGROTATE_BIN,
    LOGROTATE_CONF,
    "/etc/logrotate.d/syslog",
    "/etc/logrotate.d/wtmp",
]


class BaseLogRotateTest(TestCase):
    def test_logrotate_files(self):
        for pn in NEEDED_PATHS:
            self.assertTrue(os.path.exists(pn), "Path not found: " + pn)

    def test_logrotate_config(self):
        output = subprocess.check_output(
            LOGROTATE_BIN + " " + LOGROTATE_CONF,
            stderr=subprocess.STDOUT,
            shell=True,
            timeout=60,
        )
        stdout = output.decode("utf-8")
        self.assertFalse("error" in stdout, "error while rotating logs: " + stdout)
