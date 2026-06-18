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

import os

from pexpect import pxssh


class OpenBMCSSHSession:
    SYNC_MULTIPLIER = 5
    LOGIN_RETRIES = 3
    SSH_OPTS = (
        " -o 'StrictHostKeyChecking=no'"
        " -o 'UserKnownHostsFile=/dev/null'"
        " -o 'PreferredAuthentications=publickey'"
        " -o 'PasswordAuthentication=no'"
    )

    def __init__(self, hostname, username=None, ssh_key=None):
        self._hostname = hostname
        self._username = username or os.environ.get("TEST_USERNAME", "root")
        self._ssh_key = ssh_key or os.environ.get("TEST_SSH_KEY")

    def _new_session(self):
        self.session = pxssh.pxssh()
        self.session.force_password = False
        self.session.SSH_OPTS = self.SSH_OPTS

    def connect(self):
        self._new_session()

    def _do_login(self):
        if self._ssh_key:
            self.session.login(
                self._hostname,
                self._username,
                ssh_key=self._ssh_key,
                sync_multiplier=self.SYNC_MULTIPLIER,
            )
        else:
            self.session.login(
                self._hostname,
                self._username,
                sync_multiplier=self.SYNC_MULTIPLIER,
            )

    def login(self):
        last_exc = None
        for attempt in range(self.LOGIN_RETRIES):
            if attempt:
                self._new_session()
            try:
                self._do_login()
                return
            except pxssh.ExceptionPxssh as exc:
                last_exc = exc
        raise last_exc
