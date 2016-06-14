#!/usr/bin/env python
#
# Copyright 2016-present Facebook. All Rights Reserved.
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


from subprocess import *
import re

# Handler for FRUID resource endpoint
def get_bmc():
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
    adata = Popen('/usr/local/bin/lsb_release |awk -F \': \' \'{print $2}\'', \
                        shell=True, stdout=PIPE).stdout.read()
    data = adata.split('\n')
    version = str(data[0])
    result = {
                "Information": {
                    "Description": "Galaxy100 BMC",
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

