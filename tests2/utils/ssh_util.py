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
    # Set a bash-safe unique prompt ourselves; pxssh's probe can fall through to
    # a zsh prompt string that bash can't render, breaking sync on the session.
    UNIQUE_PROMPT = r"\[PEXPECT\][\$\#] "
    PROMPT_SET_BASH = r"PS1='[PEXPECT]\$ '"
    PROMPT_TIMEOUT = 10
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
                auto_prompt_reset=False,
            )
        else:
            self.session.login(
                self._hostname,
                self._username,
                sync_multiplier=self.SYNC_MULTIPLIER,
                auto_prompt_reset=False,
            )
        self._set_bash_prompt()

    def _set_bash_prompt(self):
        self.session.PROMPT = self.UNIQUE_PROMPT
        self.session.sendline("unset PROMPT_COMMAND")
        self.session.sendline(self.PROMPT_SET_BASH)
        if not self.session.prompt(timeout=self.PROMPT_TIMEOUT):
            raise pxssh.ExceptionPxssh("could not synchronize on bash prompt")

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
        if last_exc is not None:
            raise last_exc
        raise pxssh.ExceptionPxssh("login failed")
