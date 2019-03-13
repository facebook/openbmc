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

import pxssh


class OpenBMCSSHSession:
    def __init__(self, hostname):
        self._hostname = hostname
        self._username = "root"
        self._password = "0penBmc"

    def connect(self):
        self.session = pxssh.pxssh()
        self.session.SSH_OPTS = (
            " -o 'StrictHostKeyChecking=no'" + " -o 'UserKnownHostsFile /dev/null' "
        )

    def login(self):
        if not self.session.login(self._hostname, self._username, self._password):
            print("Login [FAILED]")
        return
