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
import glob
import subprocess
import time
from contextlib import suppress
from unittest import TestCase

from utils.test_utils import running_systemd


SYSTEMD_RESTART_COMMAND = ["/bin/systemctl", "restart", "rsyslog"]
SYSV_RESTART_COMMAND = ["/etc/init.d/syslog", "restart"]

PIDFILE = "/var/run/rsyslogd.pid"

NUM_SIMULTANEOUS_RESTARTS = 40


class SyslogRestartTestError(Exception):
    pass


class SyslogRestartTest(TestCase):
    def setUp(self):
        if running_systemd():
            self.syslog_restart_command = SYSTEMD_RESTART_COMMAND
        else:
            self.syslog_restart_command = SYSV_RESTART_COMMAND

        self._assert_preconditions()

    def test_syslog_restart_no_duplicates(self):
        # Issue many syslog restart commands at the same time
        restart_cmds = [
            subprocess.Popen(self.syslog_restart_command)
            for i in range(NUM_SIMULTANEOUS_RESTARTS)
        ]

        # Wait for restart commands to return
        for p in restart_cmds:
            p.communicate()

        time.sleep(5)

        # Ensure we continue with exactly one syslog process after restart
        self.assertEqual(
            self._get_syslog_process_count(),
            1,
            "expected exactly 1 syslog process after restart",
        )

    ## Utils
    def _get_syslog_process_count(self):
        total_syslog_processes = 0

        # Get reference syslog cmdline
        reference_cmdline = self._get_syslog_cmdline()

        # Count all processes with syslog cmdline
        for cmdline_file in glob.glob("/proc/[0-9]*/cmdline"):
            # /proc/$pid/cmdline might not exist anymore, but that's OK
            with suppress(FileNotFoundError):
                with open(cmdline_file) as f:
                    cmdline = f.read()

                if cmdline == reference_cmdline:
                    total_syslog_processes += 1

        return total_syslog_processes

    def _get_syslog_cmdline(self):
        max_tries = 5
        for i in range(max_tries):
            try:
                with open(PIDFILE) as f:
                    reference_pid = f.read().strip()

                with open("/proc/{}/cmdline".format(reference_pid)) as f:
                    reference_cmdline = f.read()

            except FileNotFoundError:
                # max_tries reached, raise
                if i == max_tries - 1:
                    raise SyslogRestartTestError(
                        "syslog daemon doesn't seem to be running. Check if '{PIDFILE}'"
                        " exists and that it corresponds to a running process".format(
                            PIDFILE=PIDFILE
                        )
                    ) from None

                time.sleep(5)

        return reference_cmdline

    def _assert_preconditions(self):
        # Ensure we only exactly one syslog process
        self.assertNotEqual(
            self._get_syslog_process_count(), 0, "syslog daemon was not detected"
        )
        self.assertEqual(
            self._get_syslog_process_count(),
            1,
            "system was running more than one syslogd process",
        )
