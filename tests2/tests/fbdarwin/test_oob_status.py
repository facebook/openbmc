#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


class OobStatusTest(unittest.TestCase):
    def setUp(self):
        self.eeprom_util = "/usr/local/bin/oob-status.sh"

        # "cases" maps command arguments to return code
        self.cases = {
            "" : 0,
            "0" : 0,
            "1" : 0,
            "IMP" : 0,
            "1 2" : 0,
            "2 1 0" : 0,
            "IMP 0" : 0,
            "0 1 2 IMP" : 0,
            "3" : 1,
            "0 1 3" : 1,
            "0 1 2 3 IMP" : 1
        }

        # "status_dict" maps commands and port arguments to expected output
        self.status_dict = {
            "link_status" : {
                "0" : "Link Up",
                "1" : "Link Up",
                "2" : "Link Down",
                "IMP" : "Link Up",
            },
            "link_speed" : {
                "0" : "1000 Mb/s",
                "1" : "1000 Mb/s",
                "2" : "1000 Mb/s",
                "IMP" : "1000 Mb/s",
            }
        }
        self.invalid_text = "Invalid port 3"

        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def oobStatusTesterOutput(self, cmd, output, args):
        for arg in args.split(" "):
            self.assertTrue("Port: {} {}".format(
                arg, self.status_dict[cmd][arg]) in output,
                "Port {} not found in output".format(arg))

    def oobStatusTester(self, cmd):
        for args, code in self.cases.items():
            output = run_shell_cmd("{} {} {}".format(self.eeprom_util, cmd, args),
                expected_return_code=code)
            output = [x.strip() for x in output.split("\n") if x]

            if code == 0:
                if not args: # no ports specified: print all ports
                    args = " ".join(self.status_dict["link_status"])
                if cmd:
                    self.oobStatusTesterOutput(cmd, output, args)
                else: # no cmd; all info is printed
                    for k in self.status_dict:
                        self.oobStatusTesterOutput(k, output, args)
            else: # an invalid port was passed in
                self.assertEqual(output[0], self.invalid_text,
                    "Error response \"{}\" not found".format(self.invalid_text))

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_oob_status(self):
        self.oobStatusTester("link_status")
        self.oobStatusTester("link_speed")
        self.oobStatusTester("")

