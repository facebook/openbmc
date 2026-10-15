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
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
import os
import subprocess
import syslog
import time
import unittest
import uuid


LOGROTATE_BIN = "/usr/sbin/logrotate"
SYSLOG_LOGROTATE_CONF = "/etc/logrotate.d/syslog"
MESSAGES_FILE = "/var/log/messages"
ROTATED_MESSAGES_FILE = MESSAGES_FILE + ".1"

TARGET_LOG_SIZE = 6 * 1024 * 1024
FILLER_CHUNK = (b"aristabmc-cit-logrotate-stress " + (b"x" * 960) + b"\n") * 64
WAIT_TIMEOUT = 30


class LargeSyslogRotationTest(unittest.TestCase):
    def _write_syslog_and_wait(self, token):
        deadline = time.monotonic() + WAIT_TIMEOUT
        while time.monotonic() < deadline:
            # Retrying the write also covers the short window in which
            # rsyslog is reopening the file after logrotate sends SIGHUP.
            syslog.syslog(token)
            time.sleep(1)
            try:
                with open(MESSAGES_FILE, "rb") as log_file:
                    if token.encode() in log_file.read():
                        return
            except FileNotFoundError:
                pass

        self.fail("{} was not written to {}".format(token, MESSAGES_FILE))

    def _grow_messages_file(self):
        bytes_needed = max(TARGET_LOG_SIZE - os.path.getsize(MESSAGES_FILE), 0)
        # Fill the file directly so the test is deterministic even if rsyslog
        # rate limiting is enabled. Markers still exercise rsyslog before and
        # after rotation.
        with open(MESSAGES_FILE, "ab") as messages:
            while bytes_needed > 0:
                chunk = FILLER_CHUNK[:bytes_needed]
                messages.write(chunk)
                bytes_needed -= len(chunk)

    def test_large_syslog_rotation(self):
        for path in (LOGROTATE_BIN, SYSLOG_LOGROTATE_CONF, MESSAGES_FILE):
            self.assertTrue(os.path.exists(path), "Path not found: " + path)

        before_token = "aristabmc-cit-before-rotation-{}".format(uuid.uuid4())
        self._write_syslog_and_wait(before_token)

        self._grow_messages_file()
        self.assertGreaterEqual(os.path.getsize(MESSAGES_FILE), TARGET_LOG_SIZE)

        subprocess.check_call(
            [LOGROTATE_BIN, SYSLOG_LOGROTATE_CONF],
            timeout=60,
        )

        with open(ROTATED_MESSAGES_FILE, "rb") as rotated_messages:
            self.assertIn(before_token.encode(), rotated_messages.read())

        after_token = "aristabmc-cit-after-rotation-{}".format(uuid.uuid4())
        self._write_syslog_and_wait(after_token)


if __name__ == "__main__":
    unittest.main()
