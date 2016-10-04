#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

from subprocess import *
import re
from node import node
from pal import *
from uuid import getnode as get_mac

class bmcNode(node):
    def __init__(self, info = None, actions = None):
        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self):
        # Get Platform Name
        name = pal_get_platform_name()

        # Get MAC Address
        mac=get_mac()
        mac_addr=':'.join(("%012X" % mac)[i:i+2] for i in range(0, 12, 2))

        # Get BMC Reset Reason
        wdt_counter = Popen('devmem 0x1e785010', \
                            shell=True, stdout=PIPE).stdout.read()
        wdt_counter = int(wdt_counter, 0)
        wdt_counter &= 0xff00

        if wdt_counter:
            por_flag = 0
        else:
            por_flag = 1

        if por_flag:
            reset_reason = "Power ON Reset"
        else:
            reset_reason = "User Initiated Reset or WDT Reset"

        # Get BMC's Up Time
        uptime = Popen('uptime', \
                        shell=True, stdout=PIPE).stdout.read()

        # Get Usage information
        data = Popen('top -b n1', \
                            shell=True, stdout=PIPE).stdout.read()
        adata = data.split('\n')
        mem_usage = adata[0]
        cpu_usage = adata[1]

        # Get OpenBMC version
        obc_version = ""
        data = Popen('cat /etc/issue', \
                            shell=True, stdout=PIPE).stdout.read()

        # OpenBMC Version
        ver = re.search(r'[v|V]([\w\d._-]*)\s', data)
        if ver:
            obc_version = ver.group(1)

        # Get U-boot Version
        uboot_version = ""
        data = Popen( 'strings /dev/mtd0 | grep U-Boot | grep 20', \
                            shell=True, stdout=PIPE).stdout.read()

        # U-boot Version
        uboot_version = data.strip('\n')

        # Get kernel release and kernel version
        kernel_release = ""
        data = Popen( 'uname -r', \
                            shell=True, stdout=PIPE).stdout.read()
        kernel_release = data.strip('\n')

        kernel_version = ""
        data = Popen( 'uname -v', \
                            shell=True, stdout=PIPE).stdout.read()
        kernel_version = data.strip('\n')

        info = {
            "Description": name + " BMC",
            "MAC Addr": mac_addr,
            "Reset Reason": reset_reason,
            "Uptime": uptime,
            "Memory Usage": mem_usage,
            "CPU Usage": cpu_usage,
            "OpenBMC Version": obc_version,
            "u-boot version": uboot_version,
            "kernel version": kernel_release + " " + kernel_version,
            }

        return info

    def doAction(self, data):
        if (data["action"] != 'reboot'):
            result = 'failure'
        else:
            Popen('sleep 1; /sbin/reboot', shell=True, stdout=PIPE)
            result = 'success'

        result = {"result": result}

        return result

def get_node_bmc():
    actions = ["reboot"]
    return bmcNode(actions = actions)
