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

import subprocess

from common.base_interface_test import BaseInterfaceTest
from utils.cit_logger import Logger


class BaseSlaacTest(BaseInterfaceTest):
    def get_ipv6_address(self):
        """
        Get inet6 address with highest length of a given interface
        overriding this method of BaseInterfaceTest class because we want
        to have inet6 address with highest length
        """
        out = self.get_ip_addr_output_inet6()
        # trying to find inet6 address with highest length
        ipv6 = ""
        for value in out[1:]:
            if len(value.split("/")[0]) > len(ipv6):
                ipv6 = value.split("/")[0]
        Logger.debug("Got ip address for " + str(self.ifname))
        return ipv6.lower()

    def get_mac_address(self):
        """
        Get Ethernet MAC address
        """
        f = subprocess.Popen(
            ["fw_printenv", "-n", "ethaddr"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        out, err = f.communicate()
        self.assertEqual(
            f.returncode,
            0,
            "fw_printenv -n ethaddr "
            + "exited with returncode: "
            + str(f.returncode)
            + ", err: "
            + str(err),
        )
        if out:
            out = out.decode("utf-8").rstrip()
            return out.lower()
        else:
            raise Exception("Couldn't find MAC address [FAILED]")

    def generate_modified_eui_64_mac_address(self):
        """
        Get Modified EUI-64 Mac Address
        """
        mac_address = self.get_mac_address().split(":")
        # reversing the 7th bit of the mac address
        mac_address[0] = hex(int(mac_address[0], 16) ^ 2)[2:]
        mac_address[2] = mac_address[2] + "fffe"
        return "".join(mac_address)
