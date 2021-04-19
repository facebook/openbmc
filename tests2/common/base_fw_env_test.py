#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

from utils.shell_util import run_shell_cmd


class BaseFwEnvTest(TestCase):
    def setUp(self):
        self.setenv_cmd = "/sbin/fw_setenv"
        self.printenv_cmd = "/sbin/fw_printenv"

    def run_fw_printenv_test(self, cmd: str = "/sbin/fw_printenv"):
        ret = subprocess.run(
            cmd, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )

        self.assertTrue(
            ret.returncode == 0,
            cmd + " returned: " + str(ret.returncode),
        )

    def test_fw_printenv_status(self):
        self.run_fw_printenv_test(self.printenv_cmd)

    def test_fw_printenv_noheader_status(self):
        self.run_fw_printenv_test("%s -n bootargs" % self.printenv_cmd)

    def test_fw_setenv(self):
        test_key = "cit_test_tag"
        test_val = "yes"

        #
        # Set a new env and read it back: this is to make sure the env
        # can be set and get properly.
        #
        run_shell_cmd("%s %s %s" % (self.setenv_cmd, test_key, test_val))
        output = run_shell_cmd("%s -n %s" % (self.printenv_cmd, test_key))
        val = output.strip()
        self.assertEqual(
            val,
            test_val,
            "Error: %s is set to %s: expect %s" % (test_key, val, test_val),
        )

        # Clear the test env
        run_shell_cmd("%s %s" % (self.setenv_cmd, test_key))
