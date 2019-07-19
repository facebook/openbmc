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

import subprocess

from utils.cit_logger import Logger


class BaseInterfaceTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_ifname(self, ifname):
        self.ifname = ifname

    def get_ipv4_address(self):
        """
        Get IPv4 address of a given interface
        """
        f = subprocess.Popen(
            ["ip", "addr", "show", self.ifname.encode("utf-8")],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        out, err = f.communicate()
        if len(out) == 0 or len(err) != 0:
            raise Exception(
                "Device " + self.ifname.encode("utf-8") + " does not exist [FAILED]"
            )
        out = out.decode("utf-8")
        ipv4 = out.split("inet ")[1].split("/")[0]
        Logger.debug("Got ip address for " + str(self.ifname))
        return ipv4

    def get_ipv6_address(self):
        """
        Get IPv6 address of a given interface
        """
        f = subprocess.Popen(
            ["ip", "addr", "show", self.ifname.encode("utf-8")],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        out, err = f.communicate()
        self.assertNotEqual(
            len(out), 0, "Device " + str(self.ifname) + " does not exist [FAILED]"
        )
        self.assertEqual(
            len(err), 0, "Device " + str(self.ifname) + " does not exist [FAILED]"
        )

        if len(out) == 0 or len(err) != 0:
            Logger.error("Device " + str(self.ifname) + " does not exist [FAILED]")
            return 1
            # raise Exception("Device " + self.ifname.encode('utf-8') + " does not exist [FAILED]")
        out = out.decode("utf-8")
        ipv6 = out.split("inet6 ")[1].split("/")[0]
        Logger.debug("Got ip address for " + str(self.ifname))
        return ipv6

    def run_ping(self, cmd):
        self.assertNotEqual(cmd, None, "run_ping cmd not set")
        if cmd != "":
            Logger.debug("Executing: " + str(cmd))
            f = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
            err = f.communicate()[1]
            if len(err) > 0:
                raise Exception(err)
            return f.returncode
        return -1

    def ping_v4(self):
        ip = self.get_ipv4_address()
        cmd = "ping -c 1 -q -w 1 " + ip
        return self.run_ping(cmd)

    def ping_v6(self):
        ip = self.get_ipv6_address()
        cmd = "ping6 -c 1 -q -w 1 " + ip
        return self.run_ping(cmd)


class CommonInterfaceTest(BaseInterfaceTest):
    def test_eth0_v4_interface(self):
        """
        Tests eth0 v4 interface
        """
        self.set_ifname("eth0")
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.ping_v4(), 0, "Ping test for %s v4 failed".format("eth0"))

    def test_eth0_v6_interface(self):
        """
        Tests eth0 v6 interface
        """
        self.set_ifname("eth0")
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.ping_v6(), 0, "Ping test for eth0 v6 failed")
