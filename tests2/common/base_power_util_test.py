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
import re
import unittest
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BasePowerUtilTest(unittest.TestCase):
    @abstractmethod
    def set_slots(self):
        pass

    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.set_slots()

    def tearDown(self):
        Logger.info("turn all slots back to ON state")
        for slot in self.slots:
            if self.slot_status(slot, status="off"):
                self.turn_slot(slot, status="on")
            elif self.slot_12V_status(slot, status="off"):
                self.turn_12V_slot(slot, status="on")
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def slot_status(self, slot, status=None):
        cmd = ["power-util", slot, "status"]
        cli_out = run_cmd(cmd).split(":")
        if len(cli_out) < 2:
            raise Exception("unexpected output: {}".format(cli_out))
        if cli_out[1].strip() == status.upper():
            return True
        return False

    def slot_12V_status(self, slot, status=None):
        cmd = ["power-util", slot, "status"]
        cli_out = run_cmd(cmd).split(":")
        if len(cli_out) < 2:
            raise Exception("unexpected output: {}".format(cli_out))
        if cli_out[1].strip() == status.upper() + " (12V-{})".format(status.upper()):
            return True
        return False

    def turn_slot(self, slot, status=None):
        cmd = ["power-util", slot, status]
        Logger.info("turn {} {} now..".format(slot, status))
        cli_out = run_cmd(cmd)
        pattern = r"^Powering fru.*to {} state.*$"
        self.assertIsNotNone(
            re.match(pattern.format(status.upper()), cli_out),
            "unexpected output: {}".format(cli_out),
        )
        if self.slot_status(slot, status=status):
            return True
        return False

    def turn_12V_slot(self, slot, status=None):
        cmd = ["power-util", slot, "12V-{}".format(status)]
        Logger.info("turn {} {} now..".format(slot, status))
        cli_out = run_cmd(cmd)
        pattern = r"^12V Powering fru.*to {} state.*$"
        self.assertIsNotNone(
            re.match(pattern.format(status.upper()), cli_out),
            "unexpected output: {}".format(cli_out),
        )
        if self.slot_status(slot, status=status):
            return True
        return False

    def test_show_slot_status(self):
        """
        test power-util can show slot status
        """
        pattern = r"^Power status for fru.*: (ON|OFF)$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                cmd = ["power-util", slot, "status"]
                cli_out = run_cmd(cmd)
                self.assertIsNotNone(
                    r.match(cli_out),
                    "unexpected output: {}".format(cli_out),
                )

    def test_slot_off(self):
        """
        test power-util [slot] off
        """
        pattern = r"^Power status for fru.*: OFF$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                # turn on the slot if its OFF
                if self.slot_status(slot, status="off"):
                    self.turn_slot(slot, status="on")
                self.turn_slot(slot, status="off")
                cmd = ["power-util", slot, "status"]
                cli_out = run_cmd(cmd)
                self.assertIsNotNone(
                    r.match(cli_out),
                    "unexpected output: {}".format(cli_out),
                )

    def test_slot_on(self):
        """
        test power-util [slot] on
        """
        pattern = r"^Power status for fru.*: ON$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                # turn off the slot if its ON
                if self.slot_status(slot, status="on"):
                    self.turn_slot(slot, status="off")
                self.turn_slot(slot, status="on")
                cmd = ["power-util", slot, "status"]
                cli_out = run_cmd(cmd)
                self.assertIsNotNone(
                    r.match(cli_out),
                    "unexpected output: {}".format(cli_out),
                )

    def test_slot_reset(self):
        """
        test power-util [slot] reset
        """
        pattern = r"^Power reset fru.*$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                cmd = ["power-util", slot, "reset"]
                cli_out = run_cmd(cmd)
                # check if BMC is doing reset
                self.assertIsNotNone(
                    r.match(cli_out),
                    "unexpected output: {}".format(cli_out),
                )
                # check if slot back to ON status
                self.assertTrue(self.slot_status(slot, status="on"))

    def test_slot_cycle(self):
        """
        test power-util [slot] cycle
        """
        pattern = r"^Power cycling fru.*$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                cmd = ["power-util", slot, "cycle"]
                cli_out = run_cmd(cmd)
                # check if BMC is doing cycling
                self.assertIsNotNone(
                    r.match(cli_out),
                    "unexpected output: {}".format(cli_out),
                )
                # check if slot back to ON status
                self.assertTrue(self.slot_status(slot, status="on"))

    def test_12V_slot_off(self):
        """
        test power-util [slot] 12V-off
        """
        pattern = r"^Power status for fru.*: OFF \(12V-OFF\)$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                # turn on the slot if its OFF or 12V-off
                if self.slot_status(slot, status="off"):
                    self.turn_slot(slot, status="on")
                elif self.slot_12V_status(slot, status="off"):
                    self.turn_12V_slot(slot, status="on")
                self.turn_12V_slot(slot, status="off")
                cmd = ["power-util", slot, "status"]
                cli_out = run_cmd(cmd)
                self.assertIsNotNone(
                    r.match(run_cmd(cmd)),
                    "unexpected output: {}".format(cli_out),
                )

    def test_12V_slot_on(self):
        """
        test power-util [slot] 12V-on
        """
        pattern = r"^Power status for fru.*: ON$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                # turn off the slot if its ON
                if self.slot_status(slot, status="on"):
                    self.turn_12V_slot(slot, status="off")
                self.turn_12V_slot(slot, status="on")
                cmd = ["power-util", slot, "status"]
                cli_out = run_cmd(cmd)
                self.assertIsNotNone(
                    r.match(run_cmd(cmd)),
                    "unexpected output: {}".format(cli_out),
                )

    def test_12V_slot_cycle(self):
        """
        test power-util [slot] cycle
        """
        pattern = r"^12V Power cycling fru.*$"
        r = re.compile(pattern)
        for slot in self.slots:
            with self.subTest(slot=slot):
                cmd = ["power-util", slot, "12V-cycle"]
                cli_out = run_cmd(cmd)
                # check if BMC is doing cycling
                self.assertIsNotNone(
                    r.match(cli_out),
                    "unexpected output: {}".format(cli_out),
                )
                # check if slot back to ON status
                self.assertTrue(self.slot_status(slot, status="on"))
