#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import subprocess
import unittest

from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class SwitchUpgradeTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def run_cmd(self, cmd):

        f = subprocess.Popen(
            cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )

        err = f.communicate()[1]
        out = f.communicate()[0]
        if len(err) > 0:
            raise Exception(err)

        return out

    def test_switch_upgrade(self):

        cmd = "switch_upgrade.sh packaged_version /usr/share/Churchill-BMC-GESW_rev.bin"
        out = self.run_cmd(cmd)

        output = str(out.split()[-1])
        packaged_ver = output.split("\n")[0]

        cmd = "switch_upgrade.sh upgrade /usr/share/Churchill-BMC-GESW_rev.bin"
        out = self.run_cmd(cmd)

        cmd = "switch_upgrade.sh eeprom_version"
        out = self.run_cmd(cmd)

        output = str(out.split()[-1])
        upgraded_ver = output.split("\n")[0]

        self.assertIn(
            packaged_ver,
            upgraded_ver,
            "switch upgrade test is failed. Received={}".format(upgraded_ver),
        )

    def test_switch_upgrade_erase(self):

        cmd = "switch_upgrade.sh erase"
        out = self.run_cmd(cmd)

        cmd = "switch_upgrade.sh eeprom_version"
        out = self.run_cmd(cmd)

        output = str(out.split()[-1])
        erased_output = output.split("\n")[0]

        cmd = "switch_upgrade.sh upgrade /usr/share/Churchill-BMC-GESW_rev.bin"
        out = self.run_cmd(cmd)

        self.assertIn(
            ".",
            erased_output,
            "switch upgrade test is failed. Received={}".format(erased_output),
        )

    def test_switch_upgrade_disable(self):

        cmd = "switch_upgrade.sh disable"
        out = self.run_cmd(cmd)

        cmd = "switch_upgrade.sh check"
        out = self.run_cmd(cmd)

        output = str(out.split()[-1])
        disable_check = output.split("\n")[0]

        self.assertIn(
            "disabled",
            disable_check,
            "switch upgrade test is failed. Received={}".format(disable_check),
        )

    def test_switch_upgrade_enable(self):

        cmd = "switch_upgrade.sh enable"
        out = self.run_cmd(cmd)

        cmd = "switch_upgrade.sh check"
        out = self.run_cmd(cmd)

        output = str(out.split()[-1])
        enable_check = output.split("\n")[0]

        self.assertIn(
            "enabled",
            enable_check,
            "switch upgrade test is failed. Received={}".format(enable_check),
        )
