#!/usr/bin/env python
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

# Handler for FRUID resource endpoint
def get_bmc():
    # Get BMC Reset Reason
    (wdt_counter, _) = Popen('devmem 0x1e785010', \
                             shell=True, stdout=PIPE).communicate()
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
    (uptime, _) = Popen('uptime', \
                        shell=True, stdout=PIPE).communicate()

    # Get Usage information
    (data, _) = Popen('top -b n1', \
                        shell=True, stdout=PIPE).communicate()
    adata = data.split('\n')
    mem_usage = adata[0]
    cpu_usage = adata[1]

    # Get OpenBMC version
    version = ""
    (data, _) = Popen('cat /etc/issue', \
                        shell=True, stdout=PIPE).communicate()
    ver = re.search(r'v([\w\d._-]*)\s', data)
    if ver:
        version = ver.group(1)

    result = {
                "Information": {
                    "Description": "Wedge BMC",
                    "Reset Reason": reset_reason,
                    "Uptime": uptime,
                    "Memory Usage": mem_usage,
                    "CPU Usage": cpu_usage,
                    "OpenBMC Version": version,
                },
                "Actions": [],
                "Resources": [],
             }

    return result;

