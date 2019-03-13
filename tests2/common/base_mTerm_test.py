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
import os
from abc import abstractmethod

from utils.ssh_util import OpenBMCSSHSession


class BaseMTermTest(object):
    USERVER_HOSTNAME = "DEFAULT"
    BMC_HOSTNAME = "DEFAULT"

    def setUp(self):
        self.mTerm_session_start_cmd = None
        self.mTerm_session_close_cmd = None
        self.bmc_hostname = None
        self.hostname = None
        self.set_mTerm_session_start_cmd()
        self.set_mTerm_session_bmc_hostname()
        self.assertNotEqual(
            self.mTerm_session_start_cmd, None, "mTerm session start command not set"
        )

    def tearDown(self):
        pass

    @abstractmethod
    def set_mTerm_session_start_cmd(self):
        pass

    def set_mTerm_session_bmc_hostname(self):
        self.hostname = os.environ.get("TEST_HOSTNAME", self.USERVER_HOSTNAME)
        self.bmc_hostname = os.environ.get("TEST_BMC_HOSTNAME", self.BMC_HOSTNAME)

    def test_external_mTerm_session_start_stop(self):
        bmc_ssh_session = OpenBMCSSHSession(self.bmc_hostname)
        bmc_ssh_session.connect()
        bmc_ssh_session.login()  # connect to BMC
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt
        bmc_ssh_session.session.sendline(
            self.mTerm_session_start_cmd
        )  # send mTerm start
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt
        self.assertIn(
            "TERMINAL MULTIPLEXER",
            bmc_ssh_session.session.before,
            "mTerm session did not start, failed to check for TERMINAL MULTIPLEXER",
        )

        test_cmd = "\r\n"
        bmc_ssh_session.session.sendline(test_cmd)  # send ENTER sequence
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt
        self.assertIn(
            "login",
            bmc_ssh_session.session.before,
            "mTerm session failed to send command to host. Received={}".format(
                bmc_ssh_session.session.before
            ),
        )

        bmc_ssh_session.session.sendcontrol("l")  # close connection
        bmc_ssh_session.session.sendline("x")
        bmc_ssh_session.session.logout()

    def test_external_mTerm_recording_from_host_kmsg(self):
        bmc_ssh_session = OpenBMCSSHSession(self.hostname)
        bmc_ssh_session.connect()
        bmc_ssh_session.login()  # connect to userver
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt
        test_cmd = 'echo "CIT TESTING" > /dev/kmsg'
        bmc_ssh_session.session.sendline(test_cmd)  # write a kmsg for sol testing
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt
        bmc_ssh_session.session.logout()  # exit

        bmc_ssh_session = OpenBMCSSHSession(self.bmc_hostname)
        bmc_ssh_session.connect()
        bmc_ssh_session.login()  # connect to BMC
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt
        bmc_ssh_session.session.sendline(
            "tail -30 /var/log/mTerm_wedge.log"
        )  # tail on mTerm logs
        bmc_ssh_session.session.prompt(timeout=5)  # wait for prompt

        self.assertIn(
            "CIT TESTING",
            bmc_ssh_session.session.before,
            "mTerm failed to record console log in {}".format(
                bmc_ssh_session.session.before
            ),
        )
        bmc_ssh_session.session.logout()  # exit
