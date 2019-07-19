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

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import mac_verify


class BaseHostMacTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.host_mac_cmd = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_host_mac(self):
        pass

    def test_host_mac(self):
        self.set_host_mac()
        self.assertNotEqual(self.host_mac_cmd, None, "Host MAC command not set")
        Logger.info("Executing cmd={}".format(self.host_mac_cmd))
        info = run_shell_cmd(cmd=self.host_mac_cmd)
        self.assertTrue(
            mac_verify(info),
            "Host MAC is incorrectly formatted to \
                        {}".format(
                info
            ),
        )

    def test_host_mac_zero_test(self):
        self.set_host_mac()
        self.assertNotEqual(self.host_mac_cmd, None, "Host MAC command not set")
        Logger.info("Executing cmd={}".format(self.host_mac_cmd))
        info = run_shell_cmd(cmd=self.host_mac_cmd)
        zero_mac = "00:00:00:00:00:00"
        self.assertNotEqual(info, zero_mac, "Host MAC is zeros {}".format(info))
