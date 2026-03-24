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

from abc import abstractmethod
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class BaseOobStatusTest(object):

    # "cases" maps command arguments to return code
    @abstractmethod
    def setCases(self):
        pass

    # "status_dict" maps commands and port arguments to expected output
    @abstractmethod
    def setStatusDict(self):
        pass

    def setUp(self):
        self.eeprom_util = "/usr/local/bin/oob-status.sh"
        self.cases = None
        self.invalid_text = None
        self.status_dict = None
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def oobStatusTesterOutput(self, cmd, output, args):
        for arg in args.split(" "):
            self.assertTrue(
                "Port: {} {}".format(arg, self.status_dict[cmd][arg]) in output,
                "Port {} not found in output\n{}".format(arg, output),
            )

    def oobStatusTester(self, cmd):
        for args, code in self.cases.items():
            output = run_shell_cmd(
                "{} {} {}".format(self.eeprom_util, cmd, args),
                expected_return_code=code,
            )
            output = [x.strip() for x in output.split("\n") if x]

            if code == 0:
                if not args:  # no ports specified: print all ports
                    args = " ".join(self.status_dict["link_status"])
                if cmd:
                    self.oobStatusTesterOutput(cmd, output, args)
                else:  # no cmd; all info is printed
                    for k in self.status_dict:
                        self.oobStatusTesterOutput(k, output, args)
            else:  # an invalid port was passed in
                self.assertEqual(
                    output[0],
                    self.invalid_text,
                    'Error response "{}" not found'.format(self.invalid_text),
                )

    def test_oob_status(self):
        self.setCases()
        self.assertNotEqual(
            self.cases, None, "oob-status.sh cases not set"
        )
        self.assertNotEqual(
            self.invalid_text, None, "oob-status.sh invalid text not set"
        )
        self.setStatusDict()
        self.assertNotEqual(
            self.status_dict, None, "oob-status.sh status dict not set"
        )
        self.oobStatusTester("link_status")
        self.oobStatusTester("link_speed")
        self.oobStatusTester("")
