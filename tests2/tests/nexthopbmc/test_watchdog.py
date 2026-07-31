#!/usr/bin/env python3
#
# Copyright (c) 2026 Nexthop Systems Inc.
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

import unittest

from common.base_watchdog_test import WatchdogTest
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class WatchdogTest(WatchdogTest, unittest.TestCase):
    def set_kill_watchdog_daemon_cmd(self):
        """
        Override to use watchdog.service instead of fscd for AcctonBMC platforms.
        AcctonBMC uses the standard Linux watchdog daemon (watchdog.service) with
        OpenBMC-specific patches to kick the hardware watchdog.
        """
        self.kill_watchdog_daemon_cmd = ["systemctl stop watchdog"]

    def set_start_watchdog_daemon_cmd(self):
        """
        Override to use watchdog.service instead of fscd for AcctonBMC platforms.
        AcctonBMC uses the standard Linux watchdog daemon (watchdog.service) with
        OpenBMC-specific patches to kick the hardware watchdog.
        """
        self.start_watchdog_daemon_cmd = ["systemctl start watchdog"]

    def test_watchdog_start_stop(self):
        """
        Test if watchdog is configured properly on bmc.

        Note: Override because AcctonBMC uses the standard Linux watchdog daemon which
        during stop, it sends WDT_MAGIC_CLOSE_KEY to /dev/watchdog before closing the fd.
        This disables the hardware watchdog, which is different from fscd which leaves the
        watchdog running when they exit.
        """
        # Kill the watchdog daemon
        # Unlike fscd, the standard watchdog daemon will stop the hardware watchdog
        # when it exits gracefully (magic close feature)
        self.kill_watchdog_daemon()
        self.assertFalse(
            self.wdtUtils.watchdog_is_running(),
            "watchdog should be stopped when daemon exits gracefully (magic close)",
        )

        # Check if watchdog can be started
        Logger.info("Testing if watchdog can be started")
        self.wdtUtils.start_watchdog()
        self.assertTrue(
            self.wdtUtils.watchdog_is_running(check_counter=True),
            "watchdog cannot be started",
        )

        # Check if watchdog can be stopped
        Logger.info("Testing if watchdog can be stopped")
        self.wdtUtils.stop_watchdog()
        self.assertFalse(
            self.wdtUtils.watchdog_is_running(), "watchdog cannot be stopped"
        )
