#!/usr/bin/env python3
#
# Copyright (c) Facebook, Inc. and its affiliates.
#
# This software may be used and distributed according to the terms of
# the GNU General Public License version 2.
import random
import string
import syslog
import time


class BaseTestSyslog:
    MSG_LENGTH = 8

    def test_syslog_logs(self):
        # Choose a random string to avoid confusions with subsequent
        # executions
        msg = "".join(random.sample(string.ascii_lowercase, self.MSG_LENGTH))
        syslog.syslog(msg)
        # added sleep to ensure the random string is logged in syslog
        time.sleep(1)
        # Binary read and search in case there is garbage in the log file
        with open("/var/log/messages", "rb") as m:
            self.assertIn(msg.encode(), m.read())
