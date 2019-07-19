#!/usr/bin/env python
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

from abc import abstractmethod
from time import sleep

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.watchdog_util import WatchdogUtils


class WatchdogTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.wdtUtils = WatchdogUtils()
        self.kill_watchdog_daemon_cmd = None
        self.start_watchdog_daemon_cmd = None
        #
        # Step 1: make sure watchdog is enabled after bmc bootup.
        #
        if not self.wdtUtils.watchdog_is_running():
            raise Exception("watchdog is not enabled after bmc bootup")

    def tearDown(self):
        #
        # Step 6: restore watchdog-kicking daemon process.
        #
        Logger.info("Finished logging for {}".format(self._testMethodName))
        Logger.info("Restoring watchdog-kicking process")
        self.set_start_watchdog_daemon_cmd()
        self.assertNotEqual(
            self.start_watchdog_daemon_cmd,
            None,
            "Starting watchdog daemon controller cmd not set",
        )
        for cmd in self.start_watchdog_daemon_cmd:
            run_shell_cmd(cmd)
        sleep(10)  # wait before daemon started back

    @abstractmethod
    def set_kill_watchdog_daemon_cmd(self):
        pass

    @abstractmethod
    def set_start_watchdog_daemon_cmd(self):
        pass

    def kill_watchdog_daemon(self):
        """
        Helper method to kill watchdog daemon: FSCD, fand, healthd
        """
        self.set_kill_watchdog_daemon_cmd()
        self.assertNotEqual(
            self.kill_watchdog_daemon_cmd,
            None,
            "Kill watchdog daemon controller cmd not set",
        )
        Logger.info("killing watchdog-kicking daemon processes")
        for cmd in self.kill_watchdog_daemon_cmd:
            run_shell_cmd(cmd)

    def test_watchdog_start_stop(self):
        """
        Test if watchdog is configured properly on bmc.
        """
        #
        # Step 2: kill the program which is responsible for kicking watchdog,
        # and make sure watchdog is not stopped when the program exit.
        #
        self.kill_watchdog_daemon()
        self.assertTrue(
            self.wdtUtils.watchdog_is_running(check_counter=True),
            "watchdog is stopped when daemon process exit",
        )

        #
        # Step 3: check if watchdog can be stopped.
        #
        Logger.info("Testing if watchdog can be stopped")
        self.wdtUtils.stop_watchdog()
        self.assertFalse(
            self.wdtUtils.watchdog_is_running(), "watchdog cannot be stopped"
        )

        #
        # Step 4: check if watchdog can be started.
        #
        Logger.info("Testing if watchdog can be started")
        self.wdtUtils.start_watchdog()
        self.assertTrue(
            self.wdtUtils.watchdog_is_running(check_counter=True),
            "watchdog cannot be started",
        )

    def test_watchdog_kick(self):
        self.kill_watchdog_daemon()
        #
        # Step 5: check if watchdog can be kicked properly. The bmc will
        # reboot if watchdog cannot be kicked properly.
        #
        interval = 6
        iterations = 6
        Logger.info("testing if watchdog can be kicked properly")
        for i in range(iterations):
            self.wdtUtils.kick_watchdog()
            Logger.info(
                "[%d/%d] watchdog kicked. Sleeping for %d seconds"
                % (i, iterations, interval)
            )
            sleep(interval)
        self.assertEqual(
            i,
            iterations - 1,
            "Failed to kick watchdog for {} iterations".format(iterations),
        )
